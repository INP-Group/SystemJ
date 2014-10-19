#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/xceac124_drv_i.h"


/*=== CEAC124 specifics ============================================*/

enum
{
    DEVTYPE   = 20, /* CEAC124 is 20 */
};

enum
{
    DESC_STOP          = 0x00,
    DESC_MULTICHAN     = 0x01,
    DESC_OSCILL        = 0x02,
    DESC_READONECHAN   = 0x03,
    DESC_READRING      = 0x04,
    DESC_WR_DAC_n_BASE = 0x80,
    DESC_RD_DAC_n_BASE = 0x90,

    DESC_FILE_WR_AT    = 0xF2,
    DESC_FILE_CREATE   = 0xF3,
    DESC_FILE_WR_SEQ   = 0xF4,
    DESC_FILE_CLOSE    = 0xF5,
    DESC_FILE_READ     = 0xF6,
    DESC_FILE_START    = 0xF7,

    DESC_U_FILE_STOP   = 0xFB,
    DESC_U_FILE_RESUME = 0xE7,
    DESC_U_FILE_PAUSE  = 0xEB,
    
    DESC_GET_DAC_STAT  = 0xFD,

    DESC_B_FILE_STOP   = 1,
    DESC_B_FILE_START  = 2,
    DESC_B_ADC_STOP    = 3,
    DESC_B_ADC_START   = 4,
    DESC_B_FILE_PAUSE  = 6,
    DESC_B_FILE_RESUME = 7,
    
    MODE_BIT_LOOP = 1 << 4,
    MODE_BIT_SEND = 1 << 5,

    DAC_STAT_EXECING = 1 << 0,
    DAC_STAT_EXEC_RQ = 1 << 1,
    DAC_STAT_PAUSED  = 1 << 2,
    DAC_STAT_PAUS_RQ = 1 << 3,
    DAC_STAT_CONT_RQ = 1 << 4,
    DAC_STAT_NEXT_RQ = 1 << 5,
};

enum
{
    MIN_TIMECODE = 0,
    MAX_TIMECODE = 7,
    DEF_TIMECODE = 4,

    MIN_GAIN     = 0,
    MAX_GAIN     = 1,
    DEF_GAIN     = 0,

    TABLE_NO = 7,
    TABLE_ID = 15,
};

enum
{
    MAX_ALWD_VAL =   9999999,
    MIN_ALWD_VAL = -10000305
};

enum
{
    XCEAC124_MAX_NSTEPS     = 29,
    XCEAC124_MAX_STEP_COUNT = 65535, // ==655.35s
    XCEAC124_USECS_IN_STEP  = 10000,
};

enum
{
    HEARTBEAT_FREQ  = 10,
    HEARTBEAT_USECS = 1000000 / HEARTBEAT_FREQ,
        
    ALIVE_SECONDS   = 60,
    ALIVE_USECS     = ALIVE_SECONDS * 1000000,
};


static inline uint8 PackMode(int gain_evn, int gain_odd)
{
    return gain_evn | (gain_odd << 2) | MODE_BIT_LOOP | MODE_BIT_SEND;
}

static inline void  decode_attr(uint8 attr, int *chan_p, int *gain_code_p)
{
    *chan_p      = attr & 63;
    *gain_code_p = (attr >> 6) & 3;
}

static rflags_t kozadc_decode_le24(uint8 *bytes,  int    gain_code,
                                   int32 *code_p, int32 *val_p)
{
  int32  code;

  static int ranges[4] =
        {
            10000000 / 1,
            10000000 / 10,
            10000000 / 100,
            10000000 / 1000
        };

    code = bytes[0] | ((uint32)(bytes[1]) << 8) | ((uint32)(bytes[2]) << 16);
    if ((code &  0x800000) != 0)
        code -= 0x1000000;
    
    *code_p = code;
    *val_p  = scale32via64(code, ranges[gain_code], 0x3fffff);
    
    return (code >= -0x3FFFFF && code <= 0x3FFFFF)? 0
                                                  : CXRF_OVERLOAD;

}

static inline int32  kozdac_c16_to_val(uint32 kozak16)
{
    return scale32via64((int32)kozak16 - 0x8000, 10000000, 0x8000);
}

static inline uint32 kozdac_val_to_c16(int32 val)
{
    return scale32via64(val, 0x8000, 10000000) + 0x8000;
}


/*=== ZZZ ===*/

#define SHOW_SET_IMMED 1

enum
{
    TMODE_NONE = 0,
    TMODE_PREP = 1, // In fact, NONE and PREP are identical
    TMODE_LOAD = 2,
    TMODE_ACTV = 3,
    TMODE_RUNN = 4,
    TMODE_PAUS = 5,
};

typedef struct
{
    cankoz_liface_t *iface;
    int              devid;
    int              handle;
    
    int              rd_rcvd;
    
    int              ch_beg;
    int              ch_end;
    int              timecode;
    int              gain;
    int              fast;

    int              num_isc;
    
    int32            ch_cfg[XCEAC124_CONFIG_CHAN_COUNT]; // Cache of config channels
    int32            ch_cur[XCEAC124_CHAN_OUT_n_COUNT]; // Current value
    int32            ch_rcv[XCEAC124_CHAN_OUT_n_COUNT]; // Current value was received

    int32            ch_spd[XCEAC124_CHAN_OUT_n_COUNT]; // Speed
    int32            ch_trg[XCEAC124_CHAN_OUT_n_COUNT]; // Target
    
    int8             ch_lkd[XCEAC124_CHAN_OUT_n_COUNT]; // Is locked by table
    int8             ch_isc[XCEAC124_CHAN_OUT_n_COUNT]; // Is changing now
    int8             ch_nxt[XCEAC124_CHAN_OUT_n_COUNT]; // Next write may be performed (previous was acknowledged)
    int8             ch_fst[XCEAC124_CHAN_OUT_n_COUNT]; // This is the FirST step
    int8             ch_fin[XCEAC124_CHAN_OUT_n_COUNT]; // This is a FINal write

    int32            t_nsteps[XCEAC124_CHAN_OUT_n_COUNT];
    int              t_aff_c [XCEAC124_CHAN_OUT_n_COUNT];
    int32            t_times [XCEAC124_CHAN_OUT_n_COUNT][XCEAC124_MAX_NSTEPS];
    int32            t_vals  [XCEAC124_CHAN_OUT_n_COUNT][XCEAC124_MAX_NSTEPS];
    int              t_mode;
    uint8            t_file[XCEAC124_MAX_NSTEPS * (2 + 4*XCEAC124_CHAN_OUT_n_COUNT)];
    size_t           t_file_bytes;
} privrec_t;

static psp_paramdescr_t xceac124_params[] =
{
    PSP_P_INT ("beg",  privrec_t, ch_beg,   0,                           0, XCEAC124_CHAN_ADC_n_COUNT-1),
    PSP_P_INT ("end",  privrec_t, ch_end,   XCEAC124_CHAN_ADC_n_COUNT-1, 0, XCEAC124_CHAN_ADC_n_COUNT-1),
    PSP_P_INT ("time", privrec_t, timecode, DEF_TIMECODE,                MIN_TIMECODE, MAX_TIMECODE),
    PSP_P_INT ("gain", privrec_t, gain,     DEF_GAIN,                    MIN_GAIN,     MAX_GAIN),
    PSP_P_FLAG("fast", privrec_t, fast,     1, 0),
    PSP_P_INT ("spd",  privrec_t, ch_spd[0], 0, -20000000, +20000000),
    PSP_P_END()
};


#define ADC_MODE_IS_NORM() \
    (me->ch_cfg[XCEAC124_CHAN_ADC_MODE] == XCEAC124_ADC_MODE_NORM)


/*=== ZZZ ===*/

static int8 UNSUPPORTED_chans[XCEAC124_CONFIG_CHAN_COUNT] =
{
    [XCEAC124_CHAN_RESERVED6]    = 1,
    [XCEAC124_CHAN_RESERVED7]    = 1,
    [XCEAC124_CHAN_RESERVED8]    = 1,
    [XCEAC124_CHAN_DO_CALIBRATE] = 1,
    [XCEAC124_CHAN_DIGCORR_MODE] = 1,
    [XCEAC124_CHAN_DIGCORR_V]    = 1,
    [XCEAC124_CHAN_RESERVED19]   = 1,
};

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;
static rflags_t  invl_rflags = CXRF_INVAL;

enum
{
    DSRF_RESET_CHANS = 1 << 0,
    DSRF_RETCONFIGS  = 1 << 1,
    DSRF_CUR2OUT     = 1 << 2,
};

static void ReturnCtlCh(privrec_t *me, int chan)
{
    ReturnChanGroup(me->devid, chan, 1,
                    me->ch_cfg + chan,
                    UNSUPPORTED_chans[chan]? &unsp_rflags : &zero_rflags);
}

static void ReturnErrAc(privrec_t *me)
{
    ReturnChanGroup(me->devid, XCEAC124_CHAN_DO_TAB_ACTIVATE, 1,
                    me->ch_cfg + XCEAC124_CHAN_DO_TAB_ACTIVATE,
                    &invl_rflags);
}

/*
 *  Note:
 *    SetTmode should NOT do anything but mangle with DO_TAB_ and OUT_MODE channels
 */

static void SetTmode(privrec_t *me, int mode)
{
  static int32 table_ctls[][XCEAC124_CHAN_DO_TAB_CNT] =
  {
      [TMODE_NONE] = {0, 0, 2, 2, 2, 2},
      [TMODE_PREP] = {0, 0, 2, 2, 2, 2},
      [TMODE_LOAD] = {2, 2, 2, 2, 2, 2},
      [TMODE_ACTV] = {0, 3, 0, 0, 2, 2},
      [TMODE_RUNN] = {2, 3, 3, 0, 0, 2},
      [TMODE_PAUS] = {2, 3, 2, 0, 3, 0},
  };
  
  int          x;
    
    me->t_mode = mode;
    memcpy(me->ch_cfg + XCEAC124_CHAN_DO_TAB_BASE,
           &(table_ctls[mode]), sizeof(table_ctls[mode]));

    for (x = 0;  x < XCEAC124_CHAN_DO_TAB_CNT;  x++)
        ReturnCtlCh(me, XCEAC124_CHAN_DO_TAB_BASE + x);

    me->ch_cfg[XCEAC124_CHAN_OUT_MODE] = (mode < TMODE_LOAD? XCEAC124_OUT_MODE_NORM
                                                           : XCEAC124_OUT_MODE_TABLE);
    ReturnCtlCh(me, XCEAC124_CHAN_OUT_MODE);
}

static void SendMULTICHAN(privrec_t *me)
{
  canqelem_t  item;
  
    item.prio     = CANKOZ_PRIO_UNICAST;
    if (!me->fast  ||
        me->ch_cfg[XCEAC124_CHAN_ADC_BEG] != me->ch_cfg[XCEAC124_CHAN_ADC_END])
    {
        item.datasize = 6;
        item.data[0]  = DESC_MULTICHAN;
        item.data[1]  = me->ch_cfg[XCEAC124_CHAN_ADC_BEG];
        item.data[2]  = me->ch_cfg[XCEAC124_CHAN_ADC_END];
        item.data[3]  = me->ch_cfg[XCEAC124_CHAN_ADC_TIMECODE];
        item.data[4]  = PackMode(me->ch_cfg[XCEAC124_CHAN_ADC_GAIN],
                                 me->ch_cfg[XCEAC124_CHAN_ADC_GAIN]);
        item.data[5] = 0; // Label
        DoDriverLog(me->devid, 0,
                    "%s:MULTICHAN beg=%d end=%d time=%d gain=%d", __FUNCTION__,
                    me->ch_cfg[XCEAC124_CHAN_ADC_BEG],
                    me->ch_cfg[XCEAC124_CHAN_ADC_END],
                    me->ch_cfg[XCEAC124_CHAN_ADC_TIMECODE],
                    me->ch_cfg[XCEAC124_CHAN_ADC_GAIN]);
    }
    else
    {
        item.datasize = 4;
        item.data[0]  = DESC_OSCILL;
        item.data[1]  = me->ch_cfg[XCEAC124_CHAN_ADC_BEG] |
                       (me->ch_cfg[XCEAC124_CHAN_ADC_GAIN] << 6);
        item.data[2]  = me->ch_cfg[XCEAC124_CHAN_ADC_TIMECODE];
        item.data[3]  = PackMode(0, 0);
        DoDriverLog(me->devid, 0,
                    "%s:OSCILL chan=%d time=%d gain=%d", __FUNCTION__,
                    me->ch_cfg[XCEAC124_CHAN_ADC_BEG],
                    me->ch_cfg[XCEAC124_CHAN_ADC_TIMECODE],
                    me->ch_cfg[XCEAC124_CHAN_ADC_GAIN]);
    }

    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
}

static void DoSoftReset(privrec_t *me, int flags)
{
  int  x;
  
    if (flags & DSRF_RESET_CHANS)
    {
        me->ch_cfg[XCEAC124_CHAN_ADC_BEG]      = me->ch_beg;
        me->ch_cfg[XCEAC124_CHAN_ADC_END]      = me->ch_end;
        me->ch_cfg[XCEAC124_CHAN_ADC_TIMECODE] = me->timecode;
        me->ch_cfg[XCEAC124_CHAN_ADC_GAIN]     = me->gain;
    }
    
    /*!!! bzero() something? ch_nxt[]:=1? */

    if (flags & DSRF_RETCONFIGS)
        for (x = 0;  x < XCEAC124_CONFIG_CHAN_COUNT;  x++)
            ReturnCtlCh(me, x);

    if (flags & DSRF_CUR2OUT)
        for (x = 0;  x < XCEAC124_CHAN_OUT_n_COUNT;  x++)
            ReturnChanGroup(me->devid, XCEAC124_CHAN_OUT_n_BASE + x, 1,
                            me->ch_cur + x,
                            &zero_rflags);
}

static void SendWrRq(privrec_t *me, int l, int32 val)
{
  uint32      code;    // Code -- in device/Kozak encoding
  canqelem_t  item;
  
    code = kozdac_val_to_c16(val);
    DoDriverLog(me->devid, 0 | DRIVERLOG_C_DATACONV,
                "%s: w=%d => [%d]:=0x%04X", __FUNCTION__, val, l, code);
    
    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 5;
    item.data[0]  = DESC_WR_DAC_n_BASE + l;
    item.data[1]  = (code >> 8) & 0xFF;
    item.data[2]  = code & 0xFF;
    item.data[3]  = 0;
    item.data[4]  = 0;
    me->iface->q_enq_ons(me->handle,
                         &item, QFE_ALWAYS); /*!!! And should REPLACE if such packet is present...  */
}

static void SendRdRq(privrec_t *me, int l)
{
  canqelem_t  item;

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 1;
    item.data[0]  = DESC_RD_DAC_n_BASE + l;
    me->iface->q_enqueue(me->handle,
                         &item, QFE_IF_NONEORFIRST);
}

static void SetToStopped(privrec_t *me)
{
  int        l;
  uint8      data[8];

    /* Erase file */
    data[0] = DESC_FILE_CREATE;
    data[1] = (TABLE_NO << 4) | TABLE_ID;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
    data[0] = DESC_FILE_CLOSE;
    data[1] = (TABLE_NO << 4) | TABLE_ID;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
    
    /* Request current values of table-driven channels */
    for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
        if (me->t_aff_c[l]) SendRdRq(me, l);
    
    /* Unlock channels */
    bzero(me->ch_lkd, sizeof(me->ch_lkd));

    SetTmode(me, TMODE_NONE);
}


/*=== ZZZ ===*/

static void xceac124_rst(int devid, void *devptr, int is_a_reset);
static void xceac124_in (int devid, void *devptr,
                         int desc, int datasize, uint8 *data);
static void xceac124_alv(int devid, void *devptr,
                         sl_tid_t tid,
                         void *privptr);
static void xceac124_hbt(int devid, void *devptr,
                         sl_tid_t tid,
                         void *privptr);

static int xceac124_init_d(int devid, void *devptr, 
                           int businfocount, int businfo[],
                           const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
  int        l;       // Line #
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    for (l = 1;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
        me->ch_spd[l] = me->ch_spd[0];

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->devid  = devid;
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                xceac124_rst, xceac124_in,
                                XCEAC124_NUMCHANS * 2 +
                                    (XCEAC124_MAX_NSTEPS * (2 + 4*XCEAC124_CHAN_OUT_n_COUNT) + 7-1) / 7 * 2 /* For table*/,
                                XCEAC124_CHAN_REGS_BASE);
    if (me->handle < 0) return me->handle;

    DoSoftReset(me, DSRF_RESET_CHANS);
    SetTmode   (me, TMODE_NONE);
    
    sl_enq_tout_after(devid, devptr, ALIVE_USECS,     xceac124_alv, NULL);
    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, xceac124_hbt, NULL);

    return DEVSTATE_OPERATING;
}

static void xceac124_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_STOP;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, data);
  
    me->iface->disconnect(devid);
}

static void xceac124_rst(int   devid    __attribute__((unused)),
                         void *devptr,
                         int   is_a_reset)
{
  privrec_t  *me     = (privrec_t *) devptr;

    if (ADC_MODE_IS_NORM()) SendMULTICHAN(me);
    else /*!!!*/;

    if (is_a_reset)
    {
        bzero(me->ch_rcv, sizeof(me->ch_rcv));
        DoSoftReset(me, 0);
    }
}

static void xceac124_alv(int devid, void *devptr,
                         sl_tid_t tid,
                         void *privptr)
{
  privrec_t  *me     = (privrec_t *) devptr;
  canqelem_t  item;

    sl_enq_tout_after(devid, devptr, ALIVE_USECS,     xceac124_alv, NULL);
    
    if (me->rd_rcvd == 0)
    {
        item.prio     = CANKOZ_PRIO_UNICAST;
        item.datasize = 1;
        item.data[0]  = CANKOZ_DESC_GETDEVATTRS;
        me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
    }
    
    me->rd_rcvd = 0;
}

static void xceac124_hbt(int devid, void *devptr,
                         sl_tid_t tid,
                         void *privptr)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         l;       // Line #
  int32       val;     // Value -- in volts

    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, xceac124_hbt, NULL);

    /*  */
    if (me->num_isc > 0)
        for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
            if (me->ch_isc[l]  &&  me->ch_nxt[l])
            {
                /* Guard against dropping speed to 0 in process */
                if      (me->ch_spd[l] == 0)
                {
                    val =  me->ch_trg[l];
                    me->ch_fin[l] = 1;
                }
                /* We use separate branches for ascend and descend
                   to simplify arithmetics and escape juggling with sign */
                else if (me->ch_trg[l] > me->ch_cur[l])
                {
                    val = me->ch_cur[l] + abs(me->ch_spd[l]) / HEARTBEAT_FREQ;
                    if (val >= me->ch_trg[l])
                    {
                        val =  me->ch_trg[l];
                        me->ch_fin[l] = 1;
                    }
                }
                else
                {
                    val = me->ch_cur[l] - abs(me->ch_spd[l]) / HEARTBEAT_FREQ;
                    if (val <= me->ch_trg[l])
                    {
                        val =  me->ch_trg[l];
                        me->ch_fin[l] = 1;
                    }
                }

                SendWrRq(me, l, val);
                SendRdRq(me, l);
                me->ch_nxt[l] = 0; // Drop the "next may be sent..." flag
            }

    /* Request current values of table-driven channels */
    if (me->t_mode == TMODE_RUNN)
        for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
            if (me->t_aff_c[l]) SendRdRq(me, l);
}

static void xceac124_in (int devid, void *devptr __attribute__((unused)),
                         int desc, int datasize, uint8 *data)
{
  privrec_t  *me       = (privrec_t *) devptr;
  int         l;       // Line #
  int32       code;    // Code -- in device/Kozak encoding
  int32       val;     // Value -- in volts
  rflags_t    rflags;
  int         gain_code;

    switch (desc)
    {
        case DESC_MULTICHAN:
        case DESC_OSCILL:
        case DESC_READONECHAN:
            if (datasize < 5) return;
            me->rd_rcvd = 1;
            decode_attr(data[1], &l, &gain_code);
            rflags = kozadc_decode_le24(data + 2, gain_code, &code, &val);
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "in: RCHAN=%-2d code=0x%08X gain=%d val=%duV",
                        l, code, gain_code, val);
            if (l < 0  ||  l >= XCEAC124_CHAN_ADC_n_COUNT) return;
            ReturnChanGroup(devid,
                            XCEAC124_CHAN_ADC_n_BASE + l,
                            1, &val, &rflags);
            break;

        case DESC_RD_DAC_n_BASE ... DESC_RD_DAC_n_BASE + XCEAC124_CHAN_OUT_n_COUNT-1:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 3) return;

            l = desc - DESC_RD_DAC_n_BASE;
            code   = data[1]*256 + data[2];
            val    = kozdac_c16_to_val(code);
            rflags = 0;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "in: WCHAN=%d code=0x%04X val=%duv",
                        l, code, val);
            me->ch_cur[l] = val;
            me->ch_rcv[l] = 1;
            me->ch_nxt[l] = 1;

            if (me->ch_fin[l])
            {
                if (me->ch_isc[l]) me->num_isc--;
                me->ch_isc[l] = 0;
            }
            
            ReturnChanGroup(devid,
                            XCEAC124_CHAN_OUT_CUR_n_BASE + l,
                            1, me->ch_cur + l, &rflags);
            if (!me->ch_isc[l]  ||  !SHOW_SET_IMMED)
                ReturnChanGroup(devid,
                                XCEAC124_CHAN_OUT_n_BASE + l,
                                1, me->ch_cur + l, &rflags);
            else if (me->ch_fst[l])
            {
                me->ch_fst[l] = 0;
                val = kozdac_c16_to_val(kozdac_val_to_c16(me->ch_trg[l]));
                ReturnChanGroup(devid,
                                XCEAC124_CHAN_OUT_n_BASE + l,
                                1, &val, &rflags);
            }

            break;

        case DESC_FILE_CLOSE:
            me->iface->q_erase_and_send_next(me->handle, 2, desc, (TABLE_NO << 4) | TABLE_ID);
            if (datasize < 4) return;
            
            DoDriverLog(devid, 0, "DESC_FILE_CLOSE: ID=%02X size=%d (file_bytes=%zu)",
                        data[1], data[2] + data[3]*256, me->t_file_bytes);
            if (me->t_mode == TMODE_LOAD)
            {
                if (data[2] + data[3]*256 == me->t_file_bytes)
                    SetTmode(me, TMODE_ACTV);
                else
                {
                    DoDriverLog(devid, 0, "file size mismatch!!!");
                    SetTmode(me, TMODE_NONE);
                }
            }
            break;

        case DESC_GET_DAC_STAT:
            DoDriverLog(devid, 0, "0xFD, ID=0x%x, mode=%d stat=0x%02x", data[2], me->t_mode, data[1]);
            if (me->t_mode == TMODE_RUNN  &&
                ((data[1] & DAC_STAT_EXECING) == 0))
                SetToStopped(me); 
            break;

        case CANKOZ_DESC_GETDEVSTAT:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            break;
            
        default:
            ////if (desc != 0xF8) DoDriverLog(devid, 0, "desc=%02x len=%d", desc, datasize);
            ;
    }
}

static void xceac124_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;     // channel N in values[] (loop ctl var)
  int         x;     // channel indeX
  int         l;     // Line #
  int32       val;   // Value -- in volts
  uint8       data[8];

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x >= XCEAC124_CHAN_REGS_BASE  &&  x <= XCEAC124_CHAN_REGS_LAST)
            me->iface->regs_rw(me->handle, action, x, values + n);
        else if (x >= XCEAC124_CHAN_REGS_RSVD_B  &&  x <= XCEAC124_CHAN_REGS_RSVD_E)
        {
            // Non-existent register channels...
        }
        else if (
                 ( // Is it a config channel?
                  x >= XCEAC124_CONFIG_CHAN_BASE  &&
                  x <  XCEAC124_CONFIG_CHAN_BASE + XCEAC124_CONFIG_CHAN_COUNT
                 )
                 &&
                 (
                  action == DRVA_READ  ||  // Reads from any cfg channels...
                  UNSUPPORTED_chans[x]     // ...and any actions with unsupported ones
                 )
                )
            ReturnCtlCh(me, x);            // Should be performed via cache
        /* Note: all config channels below shouldn't check "action",
                 since DRVA_READ was already processed above */
        else if (x == XCEAC124_CHAN_ADC_BEG)
        {
            if (ADC_MODE_IS_NORM())
            {
                if (val < 0)
                    val = 0;
                if (val > XCEAC124_CHAN_ADC_n_COUNT - 1)
                    val = XCEAC124_CHAN_ADC_n_COUNT - 1;
                if (val > me->ch_cfg[XCEAC124_CHAN_ADC_END])
                    val = me->ch_cfg[XCEAC124_CHAN_ADC_END];
                me->ch_cfg[x] = val;
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCEAC124_CHAN_ADC_END)
        {
            if (ADC_MODE_IS_NORM())
            {
                if (val < 0)
                    val = 0;
                if (val > XCEAC124_CHAN_ADC_n_COUNT - 1)
                    val = XCEAC124_CHAN_ADC_n_COUNT - 1;
                if (val < me->ch_cfg[XCEAC124_CHAN_ADC_BEG])
                    val = me->ch_cfg[XCEAC124_CHAN_ADC_BEG];
                me->ch_cfg[x] = val;
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCEAC124_CHAN_ADC_TIMECODE)
        {
            if (ADC_MODE_IS_NORM())
            {
                if (val < MIN_TIMECODE)
                    val = MIN_TIMECODE;
                if (val > MAX_TIMECODE)
                    val = MAX_TIMECODE;
                me->ch_cfg[x] = val;
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCEAC124_CHAN_ADC_GAIN)
        {
            if (ADC_MODE_IS_NORM())
            {
                if (val < MIN_GAIN)
                    val = MIN_GAIN;
                if (val > MAX_GAIN)
                    val = MAX_GAIN;
                me->ch_cfg[x] = val;
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCEAC124_CHAN_DO_RESET)
        {
            if (val == CX_VALUE_COMMAND)
            {
                DoSoftReset(me, DSRF_RESET_CHANS | DSRF_RETCONFIGS | DSRF_CUR2OUT);
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCEAC124_CHAN_ADC_MODE  ||  x == XCEAC124_CHAN_OUT_MODE)
        {
            /* These two are in fact read channels */
            ReturnCtlCh(me, x);
        }
        /* Process table-related channels similarly */
        else if (x == XCEAC124_CHAN_DO_TAB_DROP      ||
                 x == XCEAC124_CHAN_DO_TAB_ACTIVATE  ||
                 x == XCEAC124_CHAN_DO_TAB_START     ||
                 x == XCEAC124_CHAN_DO_TAB_STOP      ||
                 x == XCEAC124_CHAN_DO_TAB_PAUSE     ||
                 x == XCEAC124_CHAN_DO_TAB_RESUME)
        {
            if (action == DRVA_WRITE        &&
                val    == CX_VALUE_COMMAND  &&
                (me->ch_cfg[x] & CX_VALUE_DISABLED_MASK) == 0)
            {
                if      (x == XCEAC124_CHAN_DO_TAB_DROP)
                {
                    bzero(me->t_nsteps, sizeof(me->t_nsteps));
                    bzero(me->t_aff_c,  sizeof(me->t_aff_c));
                    bzero(me->t_times,  sizeof(me->t_times));
                    bzero(me->t_vals,   sizeof(me->t_vals));
                    bzero(me->ch_lkd,   sizeof(me->ch_lkd));
                    SetTmode(me, TMODE_NONE);

                    data[0] = DESC_FILE_CREATE;
                    data[1] = (TABLE_NO << 4) | TABLE_ID;
                    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
                    data[0] = DESC_FILE_CLOSE;
                    data[1] = (TABLE_NO << 4) | TABLE_ID;
                    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
                }
                else if (x == XCEAC124_CHAN_DO_TAB_ACTIVATE)
                {
                  int     fc; // "First Channel" -- among affected
                  int     tr;
                  int32   step_csecs;
                  
                  uint32  this_targ;
                  uint32  prev_code;
                  uint32  delta;
                  uint32  this_incr;

                  uint32  f_accls[XCEAC124_CHAN_OUT_n_COUNT][XCEAC124_MAX_NSTEPS];
                  uint32  f_incrs[XCEAC124_CHAN_OUT_n_COUNT][XCEAC124_MAX_NSTEPS];

                  uint8  *wp;
                  int32   code;
                  int     f_ofs;
                  int     f_wrn;

                  canqelem_t  item;
                  canqelem_t  isep;
                  
                    /* First, find first affected channel */
                    for (fc = -1, l = XCEAC124_CHAN_OUT_n_COUNT-1;  l >= 0;  l--)
                    {
                        if (me->t_aff_c[l]) fc = l;
                    }
                    if (fc < 0)
                    {
                        ReturnCtlCh(me, x);
                        goto NEXT_CHANNEL;
                    }

                    /* Next, check if per-channel tables are consistent */
                    /*  "l = fc+1" could also be okay */
                    for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                        if (me->t_aff_c[l])
                        {
                            if (me->t_nsteps[l] != me->t_nsteps[fc])
                            {
                                DoDriverLog(devid, 0 /*!!! Or what?*/,
                                            "DO_TAB_ACTIVATE: inconsistent nsteps, [#%d]=%d, !=[#%d]=%d",
                                            l, me->t_nsteps[l], fc, me->t_nsteps[fc]);
                                ReturnErrAc(me);
                                goto NEXT_CHANNEL;
                            }

                            for (tr = 1;  tr < me->t_nsteps[fc];  tr++)
                            {
                                if (me->t_times[l][tr] != me->t_times[fc][tr])
                                {
                                    DoDriverLog(devid, 0 /*!!! Or what?*/,
                                                "DO_TAB_ACTIVATE: inconsistent times, [#%d][@%d]=%d, !=[#%d][@%d]=%d",
                                                l, tr, me->t_times[l][tr]*XCEAC124_USECS_IN_STEP, fc, tr, me->t_times[fc][tr]*XCEAC124_USECS_IN_STEP);
                                    
                                    ReturnErrAc(me);
                                    goto NEXT_CHANNEL;
                                }
                            }

                            /*!!! Will SUCH approach be ok?
                                  Or should we also check if there are
                                  not-yet-finished writes? */
                            if (0)
                                me->t_vals[l][0] = me->ch_cur[l];
                            else
                            {
                                /*!!! Check jump from current to requested
                                      initial values against ch_spd[] */
                            }
                            
                            /*!!! Here should also check speeds against ch_spd[] */
                        }

                    /* Data is okay, let's crunch it... */
                    /*  "l = fc+1" could also be okay */
                    for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                        for (tr = 0;  tr < me->t_nsteps[fc];  tr++)
                            if (me->t_aff_c[l])
                            {
                                val = me->t_vals[l][tr];
                                this_targ = (kozdac_val_to_c16(val)) << 16;

                                DoDriverLog(devid, 0, "val=%d this_targ=%08x", val, this_targ);
                                
                                if (tr == 0)
                                    f_accls[l][tr] = this_targ;
                                else
                                {
                                    prev_code  = f_accls[l][tr-1];
                                    step_csecs = me->t_times[fc][tr];
                                    
                                    /* We handle positive and negative changes separately,
                                       in order to perform "direct/linear encoding" arithmetics correctly */
                                    if (prev_code < this_targ)
                                    {
                                        delta = this_targ - prev_code;
                                        this_incr = delta / step_csecs;
                                        f_incrs[l][tr] = this_incr;
                                        f_accls[l][tr] = prev_code + this_incr * step_csecs;
                                    }
                                    else
                                    {
                                        delta = prev_code - this_targ;
                                        this_incr = delta / step_csecs;
                                        f_incrs[l][tr] = -this_incr;
                                        f_accls[l][tr] = prev_code - this_incr * step_csecs;
                                    }

                                    DoDriverLog(devid, 0, "prev_code=%08x this_targ=%08x delta=%08x this_incr=%08x",
                                                prev_code, this_targ, delta, this_incr);
                                }
                            }
                            else
                                f_incrs[l][tr] = 0;

#if 1
                    DoDriverLog(devid, 0, "%s(): table of %d steps:", __FUNCTION__, me->t_nsteps[fc]);
                    for (tr = 1;  tr < me->t_nsteps[fc];  tr++)
                        DoDriverLog(devid, 0,
                                    "    %5d: %08X %08X %08X %08X %08X %08X %08X %08X",
                                    me->t_times[fc][tr],
                                    f_incrs[0][tr],
                                    f_incrs[1][tr],
                                    f_incrs[2][tr],
                                    f_incrs[3][tr],
                                    f_incrs[4][tr],
                                    f_incrs[5][tr],
                                    f_incrs[6][tr],
                                    f_incrs[7][tr]);
#endif

                    /* ...and create the file */
                    for (wp = me->t_file,
                         tr = 1;  tr < me->t_nsteps[fc];  tr++)
                    {
                        *wp++ =  me->t_times[fc][tr]       & 0xFF;
                        *wp++ = (me->t_times[fc][tr] >> 8) & 0xFF;

                        for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                        {
                            code = f_incrs[l][tr];
                            *wp++ =   code        & 0xFF;
                            *wp++ =  (code >>  8) & 0xFF;
                            *wp++ =  (code >> 16) & 0xFF;
                            *wp++ =  (code >> 24) & 0xFF;
                        }
                    }
                    me->t_file_bytes = wp - me->t_file;

                    SetTmode(me, TMODE_LOAD);
                    
                    /* Lock participating channels */
                    for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                    {
                        me->ch_lkd[l] = me->t_aff_c[l];
                        if (me->t_aff_c[l]  &&  me->ch_isc[l])
                        {
                            me->num_isc--;
                            me->ch_isc[l] = 0;
                        }
                    }

                    /* Pre-program initial values */
                    for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                        if (me->t_aff_c[l])
                        {
                            SendWrRq(me, l, me->t_vals[l][0]);
                            SendRdRq(me, l);
                        }

                    /////////////////////////////////////////////////
                    item.prio     = CANKOZ_PRIO_UNICAST;

                    item.datasize = 2;
                    item.data[0]  = DESC_FILE_CREATE;
                    item.data[1]  = (TABLE_NO << 4) | TABLE_ID;
                    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);

                    isep.prio     = CANKOZ_PRIO_UNICAST;
                    isep.datasize = 1;
                    isep.data[0]  = CANKOZ_DESC_GETDEVSTAT;

                    item.data[0]  = DESC_FILE_WR_SEQ;
                    
                    for (f_ofs = 0;  f_ofs < me->t_file_bytes;  f_ofs += f_wrn)
                    {
                        f_wrn = me->t_file_bytes - f_ofs;
                        if (f_wrn > 7) f_wrn = 7;
                        item.datasize = 1 + f_wrn;
                        memcpy(item.data + 1, me->t_file + f_ofs, f_wrn);
                        me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
                        me->iface->q_enqueue(me->handle, &isep, QFE_ALWAYS);
                    }

                    item.datasize = 2;
                    item.data[0]  = DESC_FILE_CLOSE;
                    item.data[1]  = (TABLE_NO << 4) | TABLE_ID;
                    me->iface->q_enqueue(me->handle, &item, QFE_ALWAYS);
                }
                else if (x == XCEAC124_CHAN_DO_TAB_START)
                {
                    data[0] = DESC_FILE_START;
                    data[1] = (TABLE_NO << 4) | TABLE_ID;
                    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
                    
                    SetTmode(me, TMODE_RUNN);
                }
                else if (x == XCEAC124_CHAN_DO_TAB_STOP)
                {
                    data[0] = DESC_B_FILE_STOP;
                    me->iface->send_frame(me->handle, CANKOZ_PRIO_BROADCAST, 1, data);
                    
                    SetToStopped(me);
                }
                else if (x == XCEAC124_CHAN_DO_TAB_PAUSE)
                {
                    SetTmode(me, TMODE_PAUS);

                    data[0] = DESC_B_FILE_PAUSE;
                    data[1] = (TABLE_NO << 4) | TABLE_ID;
                    me->iface->send_frame(me->handle, CANKOZ_PRIO_BROADCAST, 2, data);
                    
                    /* Request current values of table-driven channels */
                    for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                        if (me->t_aff_c[l]) SendRdRq(me, l);
                }
                else if (x == XCEAC124_CHAN_DO_TAB_RESUME)
                {
                    SetTmode(me, TMODE_RUNN);

                    data[0] = DESC_B_FILE_RESUME;
                    data[1] = (TABLE_NO << 4) | TABLE_ID;
                    data[2] = 0;
                    me->iface->send_frame(me->handle, CANKOZ_PRIO_BROADCAST, 3, data);
                }
            }
            
            ReturnCtlCh(me, x);
        }
     /* else if(x ... XCEAC124_CHAN_ADC_n_BASE...) isn't required since those are returned upon measurement */
        else if (x >= XCEAC124_CHAN_OUT_n_BASE  &&
                 x <  XCEAC124_CHAN_OUT_n_BASE + XCEAC124_CHAN_OUT_n_COUNT)
        {
            l = x - XCEAC124_CHAN_OUT_n_BASE;
            /* May we touch this channel now? */
            if (me->ch_lkd[l]) goto NEXT_CHANNEL;
            
            if (action == DRVA_WRITE)
            {
                /* Convert volts to code, preventing integer overflow/slip */
                if (val > MAX_ALWD_VAL) val = MAX_ALWD_VAL;
                if (val < MIN_ALWD_VAL) val = MIN_ALWD_VAL;
                
                /* ...and how should we perform the change: */
                if (!me->ch_isc[l]  &&  // No mangling with now-changing channels...
                    (
                     /* No speed limit? */
                     me->ch_spd[l] == 0
                     ||
                     /* Or is it an absolute-value-decrement? */
                     (
                      me->ch_spd[l] > 0  &&
                      abs(val) < abs(me->ch_cur[l])
                     )
                     ||
                     /* Or is this step less than speed? */
                     (
                      abs(val - me->ch_cur[l]) < abs(me->ch_spd[l]) / HEARTBEAT_FREQ
                     )
                    )
                   )
                /* Just do write?... */
                {
                    SendWrRq(me, l, val);
                }
                else
                /* ...or initiate slow change? */
                {
                    if (me->ch_isc[l] == 0) me->num_isc++;
                    me->ch_trg[l] = val;
                    me->ch_isc[l] = 1;
                    me->ch_fst[l] = 1;
                    me->ch_fin[l] = 0;
                }
            }

            /* Perform read anyway */
            SendRdRq(me, l);
        }
        else if (x >= XCEAC124_CHAN_OUT_RATE_n_BASE  &&
                 x <  XCEAC124_CHAN_OUT_RATE_n_BASE + XCEAC124_CHAN_OUT_n_COUNT)
        {
            l = x - XCEAC124_CHAN_OUT_RATE_n_BASE;
            if (action == DRVA_WRITE)
            {
                /* Note: no bounds-checking for value, since it isn't required */
                if (abs(val) < 305/*!!!*/) val = 0;
                me->ch_spd[l] = val; 
            }
  
            ReturnChanGroup(devid, x, 1, me->ch_spd + l, &zero_rflags);
        }
        else if (x >= XCEAC124_CHAN_OUT_CUR_n_BASE  &&
                 x <  XCEAC124_CHAN_OUT_CUR_n_BASE + XCEAC124_CHAN_OUT_n_COUNT)
        {
            l = x - XCEAC124_CHAN_OUT_CUR_n_BASE;
            if (me->ch_rcv[l])
                ReturnChanGroup(devid, x, 1, me->ch_cur + l, &zero_rflags);
        }
 NEXT_CHANNEL:;
    }
}

static void xceac124_bigc_p(int devid, void *devptr,
                            int chan, int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
  privrec_t  *me       = (privrec_t *) devptr;
  int32      *table    = data;
  
  int32       nsteps;
  size_t      rqd_datasize;
  int         l;
  int         tr;
  int         fc;
  int         base;
  int32       step_msecs;
  int32       step_csecs;
  int32       val;

#define RETDATACOUNT(nsteps, nchans) (nsteps * (1 + nchans))
  
  int         t_aff_c [XCEAC124_CHAN_OUT_n_COUNT];
  int32       t_times [XCEAC124_CHAN_OUT_n_COUNT][XCEAC124_MAX_NSTEPS];
  int32       t_vals  [XCEAC124_CHAN_OUT_n_COUNT][XCEAC124_MAX_NSTEPS];

  int32       retinfo[1];
  int32       retdata[RETDATACOUNT(XCEAC124_MAX_NSTEPS, XCEAC124_CHAN_OUT_n_COUNT)];
  size_t      retdatasize;

#define CHECK_IF(cond, format, args...)                                     \
    do {                                                                    \
        if (cond)                                                           \
        {                                                                   \
            DoDriverLog(devid, 0, "%s(%d): " format,                        \
                       __FUNCTION__, chan, ##args);                         \
            ReturnBigc(devid, chan, NULL, 0, NULL, 0, 0, CXRF_INVAL);       \
            return;                                                         \
        }                                                                   \
    } while (0)

    CHECK_IF(chan < 0  ||  chan >= XCEAC124_NUM_BIGCS, "bigc=%d is unknown", chan);
    CHECK_IF(nargs != 0  &&  nargs     != 1, "nargs=%d, !=1",     nargs);
    CHECK_IF(nargs != 0  &&  dataunits != 4, "dataunits=%zu, !=4", dataunits);
    nsteps = 0;
    if (nargs != 0) nsteps = args[0];
    CHECK_IF(nargs != 0  &&  (nsteps < 1  ||  nsteps > XCEAC124_MAX_NSTEPS),
             "nsteps=%d, out_of[1...%d]", nsteps, XCEAC124_MAX_NSTEPS);
    
    if (chan >= XCEAC124_BIGC_ITAB_n_BASE  &&
        chan <  XCEAC124_BIGC_ITAB_n_BASE + XCEAC124_CHAN_OUT_n_COUNT)
    {
        l = chan - XCEAC124_BIGC_ITAB_n_BASE;
        
        /* Treat nargs==0 as "read" */
        if (nargs != 0  ||  0)
        {
            rqd_datasize = RETDATACOUNT(nsteps, 1) * 4;
            CHECK_IF(datasize != rqd_datasize,
                     "datasize=%zu, !=%zu=nsteps*(1+1)*4",
                     datasize, rqd_datasize);

            /* Determine if this channel is affected or unused */
            t_aff_c[l] = abs(table[1]) < XCEAC124_VAL_DISABLE_TABLE_CHAN;
            
            /* Now, walk through rows */
            for (tr = 0;  tr < nsteps;  tr++)
            {
                step_msecs = table[tr * (1 + 1) + 0];
                step_csecs = step_msecs / XCEAC124_USECS_IN_STEP;
                /* Check validity of time. */
                if (tr != 0)
                    CHECK_IF(step_csecs < 1  ||  step_csecs > XCEAC124_MAX_STEP_COUNT,
                             "time[#%d]=%d, out_of[%d...%d]",
                             tr, step_msecs, XCEAC124_USECS_IN_STEP, XCEAC124_MAX_STEP_COUNT * XCEAC124_USECS_IN_STEP);
                else
                    step_csecs = 0;
                t_times[l][tr] = step_csecs;

                val        = table[tr * (1 + 1) + 1];
                if (val > MAX_ALWD_VAL) val = MAX_ALWD_VAL;
                if (val < MIN_ALWD_VAL) val = MIN_ALWD_VAL;
                if (!t_aff_c[l]) val = XCEAC124_VAL_DISABLE_TABLE_CHAN;
                t_vals[l][tr] = val;
            }

            /* Pad unspecified data */
            for (tr = nsteps;  tr < XCEAC124_MAX_NSTEPS;  tr++)
            {
                t_times[l][tr] = 0;
                t_vals [l][tr] = XCEAC124_VAL_DISABLE_TABLE_CHAN;
            }
            
            /* Finally, copy data to privrec */
            me->t_nsteps[l] = nsteps;
            me->t_aff_c [l] = t_aff_c[l];
            memcpy(me->t_times[l], t_times[l], sizeof(t_times[l]));
            memcpy(me->t_vals [l], t_vals [l], sizeof(t_vals [l]));
        }

        /* Now prepare read */
        retinfo[0] = nsteps = me->t_nsteps[l];
        retdatasize = RETDATACOUNT(nsteps, 1) * 4;
        for (tr = 0;  tr < nsteps;  tr++)
        {
            retdata[tr * (1 + 1) + 0] = me->t_times[l][tr] * XCEAC124_USECS_IN_STEP;
            retdata[tr * (1 + 1) + 1] = me->t_vals [l][tr];
        }
    }
    else /*if (chan == XCEAC124_BIGC_TAB_WHOLE)*/
    {
        /* Treat nargs==0 as "read" */
        if (nargs != 0  ||  0)
        {
            rqd_datasize = RETDATACOUNT(nsteps, XCEAC124_CHAN_OUT_n_COUNT) * 4;
            CHECK_IF(datasize != rqd_datasize,
                     "datasize=%zu, !=%zu=nsteps*(1+%d)*4",
                     datasize, rqd_datasize, XCEAC124_CHAN_OUT_n_COUNT);
    
            /* Determine which channels are affected and which aren't */
            for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                t_aff_c[l] = abs(table[1 + l]) < XCEAC124_VAL_DISABLE_TABLE_CHAN;
    
            /* Now, walk through rows... */
            for (tr = 0;  tr < nsteps;  tr++)
            {
                base = tr * (1 + XCEAC124_CHAN_OUT_n_COUNT);
                step_msecs = table[base + 0];
                step_csecs = step_msecs / XCEAC124_USECS_IN_STEP;
                /* Check validity of times */
                if (tr != 0)
                    CHECK_IF(step_csecs < 1  ||  step_csecs > XCEAC124_MAX_STEP_COUNT,
                             "time[#%d]=%d, out_of[%d...%d]",
                             tr, step_msecs, XCEAC124_USECS_IN_STEP, XCEAC124_MAX_STEP_COUNT * XCEAC124_USECS_IN_STEP);
                else
                    step_csecs = 0;
                for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                    t_times[l][tr] = step_csecs;
    
                /* ...and through channels */
                for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                {
                    val = table[base + 1 + l];
                    if (val > MAX_ALWD_VAL) val = MAX_ALWD_VAL;
                    if (val < MIN_ALWD_VAL) val = MIN_ALWD_VAL;
                    if (!t_aff_c[l]) val = XCEAC124_VAL_DISABLE_TABLE_CHAN;
                    t_vals[l][tr] = val;
                }
            }

            /* Pad unspecified data */
            for (tr = nsteps;  tr < XCEAC124_MAX_NSTEPS;  tr++)
                for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                {
                    t_times[l][tr] = 0;
                    t_vals [l][tr] = XCEAC124_VAL_DISABLE_TABLE_CHAN;
                }
            
            /* Finally, copy data to privrec */
            for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                me->t_nsteps[l] = nsteps;
            memcpy(me->t_aff_c, t_aff_c, sizeof(t_aff_c));
            memcpy(me->t_times, t_times, sizeof(t_times));
            memcpy(me->t_vals,  t_vals,  sizeof(t_vals));
        }
        
        /* Now prepare read */
        nsteps = 0;
        for (fc = -1, l = XCEAC124_CHAN_OUT_n_COUNT-1;  l >= 0;  l--)
        {
            if (me->t_aff_c[l]) fc = l;
            if (nsteps < me->t_nsteps[l])
                nsteps = me->t_nsteps[l];
        }
        retinfo[0] = nsteps;
        retdatasize = RETDATACOUNT(nsteps, XCEAC124_CHAN_OUT_n_COUNT) * 4;
        for (tr = 0;  tr < nsteps;  tr++)
        {
            base = tr * (1 + XCEAC124_CHAN_OUT_n_COUNT);
            retdata[base + 0] = me->t_times[fc][tr] * XCEAC124_USECS_IN_STEP;
            
            for (l = 0;  l < XCEAC124_CHAN_OUT_n_COUNT;  l++)
                retdata[base + 1 + l] = me->t_vals[l][tr];
        }
    }
    
    ReturnBigc(devid, chan,
               retinfo, 1,
               retdata, retdatasize, sizeof(int32), 0);
}

/* Metric */

static CxsdMainInfoRec xceac124_main_info[] =
{
    {XCEAC124_CONFIG_CHAN_COUNT,  1}, // Params and table control
    CANKOZ_REGS_MAIN_INFO,
    {XCEAC124_CHAN_REGS_RSVD_E-XCEAC124_CHAN_REGS_RSVD_B+1, 0},
    {XCEAC124_CHAN_ADC_n_COUNT,   0}, // ADC_n
    {XCEAC124_CHAN_OUT_n_COUNT*2, 1}, // OUT_n, OUT_RATE_n
    {XCEAC124_CHAN_OUT_n_COUNT,   0}, // OUT_CUR_n
};

static CxsdBigcInfoRec xceac124_bigc_info[] =
{
    // Individual tables
    {XCEAC124_CHAN_OUT_n_COUNT,
        1, sizeof(int32)*XCEAC124_MAX_NSTEPS*2, sizeof(int32),
        1, sizeof(int32)*XCEAC124_MAX_NSTEPS*2, sizeof(int32)},
    // Whole-channels table
    {1,
        1, XCEAC124_CHAN_OUT_n_COUNT*sizeof(int32)*XCEAC124_MAX_NSTEPS*2, sizeof(int32),
        1, XCEAC124_CHAN_OUT_n_COUNT*sizeof(int32)*XCEAC124_MAX_NSTEPS*2, sizeof(int32)},
};

DEFINE_DRIVER(xceac124, "eXtended CEAC124 support",
              NULL, NULL,
              sizeof(privrec_t), xceac124_params,
              2, 3,
              NULL, 0,
              countof(xceac124_main_info), xceac124_main_info,
              countof(xceac124_bigc_info)*0-1, xceac124_bigc_info,
              xceac124_init_d, xceac124_term_d,
              xceac124_rw_p, xceac124_bigc_p);
