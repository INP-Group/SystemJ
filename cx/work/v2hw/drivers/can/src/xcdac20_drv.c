#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "timeval_utils.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/xcdac20_drv_i.h"


/*=== CDAC20 specifics =============================================*/

enum
{
    DEVTYPE        = -1, /* Free the layer from this check, we'll do it ourselves */
    DEVTYPE_CDAC20 = 3,
    DEVTYPE_CEAC51 = 18,
};

enum
{
    DESC_STOP          = 0x00,
    DESC_MULTICHAN     = 0x01,
    DESC_OSCILL        = 0x02,
    DESC_READONECHAN   = 0x03,
    DESC_READRING      = 0x04,
    DESC_WRITEDAC     = 0x05,
    DESC_READDAC      = 0x06,
    DESC_CALIBRATE    = 0x07,
    DESC_WR_DAC_n_BASE = 0x80,
    DESC_RD_DAC_n_BASE = 0x90,

    DESC_DIGCORR_SET  = 0xE0,
    DESC_DIGCORR_STAT = 0xE1,

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
};

enum
{
    MAX_ALWD_VAL =   9999999,
    MIN_ALWD_VAL = -10000305
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

static inline int32 cdac20_daccode_to_val(uint32 kozak32)
{
    return scale32via64((int32)kozak32 - 0x800000, 10000000, 0x800000);
}

static inline int32 cdac20_val_to_daccode(int32 val)
{
    return scale32via64(val, 0x800000, 10000000) + 0x800000;
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

    int32            autocalb_onoff;
    int32            autocalb_secs;
    struct timeval   last_calibrate_time;

    int32            ch_cfg[XCDAC20_MODE_CHAN_count];  // Cache of mode channels
    int32            ch_cur[XCDAC20_CHAN_OUT_n_count]; // Current value
    int32            ch_rcv[XCDAC20_CHAN_OUT_n_count]; // Current value was received

    int32            ch_spd[XCDAC20_CHAN_OUT_n_count]; // Speed
    int32            ch_trg[XCDAC20_CHAN_OUT_n_count]; // Target
    
    int8             ch_lkd[XCDAC20_CHAN_OUT_n_count]; // Is locked by table
    int8             ch_isc[XCDAC20_CHAN_OUT_n_count]; // Is changing now
    int8             ch_nxt[XCDAC20_CHAN_OUT_n_count]; // Next write may be performed (previous was acknowledged)
    int8             ch_fst[XCDAC20_CHAN_OUT_n_count]; // This is the FirST step
    int8             ch_fin[XCDAC20_CHAN_OUT_n_count]; // This is a FINal write

    int              t_aff_c [XCDAC20_CHAN_OUT_n_count];
    int              t_mode;
} privrec_t;

static psp_paramdescr_t xcdac20_params[] =
{
    PSP_P_INT ("beg",  privrec_t, ch_beg,   0,                          0, XCDAC20_CHAN_ADC_n_count-1),
    PSP_P_INT ("end",  privrec_t, ch_end,   XCDAC20_CHAN_ADC_n_count-1, 0, XCDAC20_CHAN_ADC_n_count-1),
    PSP_P_INT ("time", privrec_t, timecode, DEF_TIMECODE,               MIN_TIMECODE, MAX_TIMECODE),
    PSP_P_INT ("gain", privrec_t, gain,     DEF_GAIN,                   MIN_GAIN,     MAX_GAIN),
    PSP_P_FLAG("fast", privrec_t, fast,     1, 0),
    PSP_P_INT ("spd",  privrec_t, ch_spd[0], 0, -20000000, +20000000),
    PSP_P_END()
};


#define ADC_MODE_IS_NORM() \
    (me->ch_cfg[XCDAC20_CHAN_ADC_MODE] == XCDAC20_ADC_MODE_NORM)


/*=== ZZZ ===*/

static int8 UNSUPPORTED_chans[XCDAC20_MODE_CHAN_count] =
{
    [XCDAC20_CHAN_RESERVED6]  = 1,
    [XCDAC20_CHAN_RESERVED7]  = 1,
    [XCDAC20_CHAN_RESERVED8]  = 1,
    [XCDAC20_CHAN_RESERVED16] = 1,
    [XCDAC20_CHAN_RESERVED17] = 1,
    [XCDAC20_CHAN_RESERVED18] = 1,
    [XCDAC20_CHAN_RESERVED19] = 1,
};

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;
static rflags_t  invl_rflags = CXRF_INVAL;

enum
{
    DSRF_RESET_CHANS = 1 << 0,
    DSRF_RETMODES    = 1 << 1,
    DSRF_CUR2OUT     = 1 << 2,
};

static void ReturnCtlCh(privrec_t *me, int chan)
{
    ReturnChanGroup(me->devid, chan, 1,
                    me->ch_cfg + chan,
                    UNSUPPORTED_chans[chan]? &unsp_rflags : &zero_rflags);
}

/*
 *  Note:
 *    SetTmode should NOT do anything but mangle with DO_TAB_ and OUT_MODE channels
 */

static void SetTmode(privrec_t *me, int mode)
{
  static int32 table_ctls[][XCDAC20_CHAN_DO_TAB_cnt] =
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
    memcpy(me->ch_cfg + XCDAC20_CHAN_DO_TAB_base,
           &(table_ctls[mode]), sizeof(table_ctls[mode]));

    for (x = 0;  x < XCDAC20_CHAN_DO_TAB_cnt;  x++)
        ReturnCtlCh(me, XCDAC20_CHAN_DO_TAB_base + x);

    me->ch_cfg[XCDAC20_CHAN_OUT_MODE] = (mode < TMODE_LOAD? XCDAC20_OUT_MODE_NORM
                                                          : XCDAC20_OUT_MODE_TABLE);
    ReturnCtlCh(me, XCDAC20_CHAN_OUT_MODE);
}

static void SendMULTICHAN(privrec_t *me)
{
  canqelem_t  item;
  
    item.prio     = CANKOZ_PRIO_UNICAST;
    if (!me->fast  ||
        me->ch_cfg[XCDAC20_CHAN_ADC_BEG] != me->ch_cfg[XCDAC20_CHAN_ADC_END])
    {
        item.datasize = 6;
        item.data[0]  = DESC_MULTICHAN;
        item.data[1]  = me->ch_cfg[XCDAC20_CHAN_ADC_BEG];
        item.data[2]  = me->ch_cfg[XCDAC20_CHAN_ADC_END];
        item.data[3]  = me->ch_cfg[XCDAC20_CHAN_ADC_TIMECODE];
        item.data[4]  = PackMode(me->ch_cfg[XCDAC20_CHAN_ADC_GAIN],
                                 me->ch_cfg[XCDAC20_CHAN_ADC_GAIN]);
        item.data[5] = 0; // Label
        DoDriverLog(me->devid, 0,
                    "%s:MULTICHAN beg=%d end=%d time=%d gain=%d", __FUNCTION__,
                    me->ch_cfg[XCDAC20_CHAN_ADC_BEG],
                    me->ch_cfg[XCDAC20_CHAN_ADC_END],
                    me->ch_cfg[XCDAC20_CHAN_ADC_TIMECODE],
                    me->ch_cfg[XCDAC20_CHAN_ADC_GAIN]);
    }
    else
    {
        item.datasize = 4;
        item.data[0]  = DESC_OSCILL;
        item.data[1]  = me->ch_cfg[XCDAC20_CHAN_ADC_BEG] |
                       (me->ch_cfg[XCDAC20_CHAN_ADC_GAIN] << 6);
        item.data[2]  = me->ch_cfg[XCDAC20_CHAN_ADC_TIMECODE];
        item.data[3]  = PackMode(0, 0);
        DoDriverLog(me->devid, 0,
                    "%s:OSCILL chan=%d time=%d gain=%d", __FUNCTION__,
                    me->ch_cfg[XCDAC20_CHAN_ADC_BEG],
                    me->ch_cfg[XCDAC20_CHAN_ADC_TIMECODE],
                    me->ch_cfg[XCDAC20_CHAN_ADC_GAIN]);
    }

    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
}

static void DoSoftReset(privrec_t *me, int flags)
{
  int  x;
  
    if (flags & DSRF_RESET_CHANS)
    {
        me->ch_cfg[XCDAC20_CHAN_ADC_BEG]      = me->ch_beg;
        me->ch_cfg[XCDAC20_CHAN_ADC_END]      = me->ch_end;
        me->ch_cfg[XCDAC20_CHAN_ADC_TIMECODE] = me->timecode;
        me->ch_cfg[XCDAC20_CHAN_ADC_GAIN]     = me->gain;
    }
    
    /*!!! bzero() something? ch_nxt[]:=1? */

    if (flags & DSRF_RETMODES)
        for (x = 0;  x < XCDAC20_MODE_CHAN_count;  x++)
            ReturnCtlCh(me, x);

    if (flags & DSRF_CUR2OUT)
        for (x = 0;  x < XCDAC20_CHAN_OUT_n_count;  x++)
            ReturnChanGroup(me->devid, XCDAC20_CHAN_OUT_n_base + x, 1,
                            me->ch_cur + x,
                            &zero_rflags);
}

static void SendWrRq(privrec_t *me, int l, int32 val)
{
  uint32      code;    // Code -- in device/Kozak encoding
  canqelem_t  item;
  
    code = cdac20_val_to_daccode(val);
    DoDriverLog(me->devid, 0 | DRIVERLOG_C_DATACONV,
                "%s: w=%d => [%d]:=0x%04X", __FUNCTION__, val, l, code);
    
    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 7;
    item.data[0]  = /*DESC_WR_DAC_n_BASE + l*/DESC_WRITEDAC;
    item.data[1] = code         & 0xFF;
    item.data[2] = (code >> 8)  & 0xFF;
    item.data[3] = (code >> 16) & 0xFF;
    item.data[4] = 0;
    item.data[5] = 0;
    item.data[6] = 0;
    me->iface->q_enq_ons(me->handle,
                         &item, QFE_ALWAYS); /*!!! And should REPLACE if such packet is present...  */
}

static void SendRdRq(privrec_t *me, int l)
{
  canqelem_t  item;

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 1;
    item.data[0]  = /*DESC_RD_DAC_n_BASE + l*/DESC_READDAC;
    me->iface->q_enqueue(me->handle,
                         &item, QFE_IF_NONEORFIRST);
}


/*=== ZZZ ===*/

static void xcdac20_rst(int devid, void *devptr, int is_a_reset);
static void xcdac20_in (int devid, void *devptr,
                        int desc, int datasize, uint8 *data);
static void xcdac20_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action);
static void xcdac20_alv(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr);
static void xcdac20_hbt(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr);

static int xcdac20_init_d(int devid, void *devptr, 
                          int businfocount, int businfo[],
                          const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->devid  = devid;
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                xcdac20_rst, xcdac20_in,
                                XCDAC20_NUMCHANS * 2,
                                XCDAC20_CHAN_REGS_base);
    if (me->handle < 0) return me->handle;

    DoSoftReset(me, DSRF_RESET_CHANS);
    SetTmode   (me, TMODE_NONE);
    
    sl_enq_tout_after(devid, devptr, ALIVE_USECS,     xcdac20_alv, NULL);
    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, xcdac20_hbt, NULL);

    return DEVSTATE_OPERATING;
}

static void xcdac20_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_STOP;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, data);
  
    me->iface->disconnect(devid);
}

static void xcdac20_rst(int   devid    __attribute__((unused)),
                        void *devptr,
                        int   is_a_reset)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         devcode;

    me->iface->get_dev_ver(me->handle, NULL, NULL, &devcode);
    if (devcode != DEVTYPE_CDAC20  &&  devcode != DEVTYPE_CEAC51)
    {
        DoDriverLog(devid, 0,
                    "%s: DevCode=%d is neither CDAC20 (%d) nor CEAC51 (%d). Terminating device.",
                    __FUNCTION__, devcode, DEVTYPE_CDAC20, DEVTYPE_CEAC51);
        SetDevState(devid, DEVSTATE_OFFLINE, CXRF_WRONG_DEV, "wrong device");
        return;
    }

    if (ADC_MODE_IS_NORM()) SendMULTICHAN(me);
    else /*!!!*/;

    if (is_a_reset)
    {
        bzero(me->ch_rcv, sizeof(me->ch_rcv));
        DoSoftReset(me, 0);
    }
}

static void xcdac20_alv(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr)
{
  privrec_t  *me     = (privrec_t *) devptr;
  canqelem_t  item;

    sl_enq_tout_after(devid, devptr, ALIVE_USECS,     xcdac20_alv, NULL);
    
    if (me->rd_rcvd == 0)
    {
        item.prio     = CANKOZ_PRIO_UNICAST;
        item.datasize = 1;
        item.data[0]  = CANKOZ_DESC_GETDEVATTRS;
        me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
    }
    
    me->rd_rcvd = 0;
}

static void xcdac20_hbt(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         l;       // Line #
  int32       val;     // Value -- in volts
  struct timeval  now;

    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, xcdac20_hbt, NULL);

    /*  */
    if (me->num_isc > 0)
        for (l = 0;  l < XCDAC20_CHAN_OUT_n_count;  l++)
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
        for (l = 0;  l < XCDAC20_CHAN_OUT_n_count;  l++)
            if (me->t_aff_c[l]) SendRdRq(me, l);

    /* Auto-calibration */
    if (me->autocalb_onoff  &&  me->autocalb_secs > 0)
    {
        gettimeofday(&now, NULL);
        now.tv_sec -= me->autocalb_secs;
        if (TV_IS_AFTER(now, me->last_calibrate_time))
        {
            val = CX_VALUE_COMMAND;
            xcdac20_rw_p(devid, me,
                         XCDAC20_CHAN_DO_CALB_DAC, 1, &val, DRVA_WRITE);
        }
    }
}

static void xcdac20_in (int devid, void *devptr __attribute__((unused)),
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
            if (l < 0  ||  l >= XCDAC20_CHAN_ADC_n_count) return;
            ReturnChanGroup(devid,
                            XCDAC20_CHAN_ADC_n_base + l,
                            1, &val, &rflags);
            break;

        case DESC_READDAC/*DESC_RD_DAC_n_BASE ... DESC_RD_DAC_n_BASE + XCDAC20_CHAN_OUT_n_count-1*/:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 4) return;

            l = desc - DESC_READDAC/*DESC_RD_DAC_n_BASE*/;
            code   = data[1] + (data[2] << 8) + (data[3] << 16);
            val    = cdac20_daccode_to_val(code);
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
                            XCDAC20_CHAN_OUT_CUR_n_base + l,
                            1, me->ch_cur + l, &rflags);
            if      (!me->ch_isc[l]  ||  !SHOW_SET_IMMED)
                ReturnChanGroup(devid,
                                XCDAC20_CHAN_OUT_n_base + l,
                                1, me->ch_cur + l, &rflags);
            else if (me->ch_fst[l])
            {
                me->ch_fst[l] = 0;
                val = cdac20_daccode_to_val(cdac20_val_to_daccode(me->ch_trg[l]));
                ReturnChanGroup(devid,
                                XCDAC20_CHAN_OUT_n_base + l,
                                1, &val, &rflags);
            }

            break;

        case DESC_DIGCORR_STAT:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 5) return;

            code = data[4] + (data[3] << 8) + (data[2] << 16);
            if (code & 0x00800000) code -= 0x01000000;
            val  = code; // That's in unknown encoding, for Kozak only
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "DIGCORR_STAT: mode=%d code=0x%04X",
                        data[1], code);
            val = data[1] & 1;
            ReturnChanGroup(devid,
                            XCDAC20_CHAN_DIGCORR_MODE,   1, &val, &zero_rflags);
            val = (data[1] >> 1) & 1;
            ReturnChanGroup(devid,
                            XCDAC20_CHAN_DIGCORR_VALID,  1, &val, &zero_rflags);
            val = code;
            ReturnChanGroup(devid,
                            XCDAC20_CHAN_DIGCORR_FACTOR, 1, &val, &zero_rflags);
            break;
            
        default:
            ////if (desc != 0xF8) DoDriverLog(devid, 0, "desc=%02x len=%d", desc, datasize);
            ;
    }
}

static void xcdac20_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;     // channel N in values[] (loop ctl var)
  int         x;     // channel indeX
  int         l;     // Line #
  int32       val;   // Value -- in volts
  int         iv;    // Int Value -- to be passable for hw_ver/sw_ver reading
  int         sr;
  uint8       data[8];
  canqelem_t  item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x >= XCDAC20_CHAN_REGS_base  &&  x <= XCDAC20_CHAN_REGS_last)
            me->iface->regs_rw(me->handle, action, x, values + n);
        else if (
                 ( // Is it a config channel?
                  x >= XCDAC20_MODE_CHAN_base  &&
                  x <  XCDAC20_MODE_CHAN_base + XCDAC20_MODE_CHAN_count
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
        else if (x == XCDAC20_CHAN_ADC_BEG)
        {
            if (ADC_MODE_IS_NORM())
            {
                if (val < 0)
                    val = 0;
                if (val > XCDAC20_CHAN_ADC_n_count - 1)
                    val = XCDAC20_CHAN_ADC_n_count - 1;
                if (val > me->ch_cfg[XCDAC20_CHAN_ADC_END])
                    val = me->ch_cfg[XCDAC20_CHAN_ADC_END];
                me->ch_cfg[x] = val;
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCDAC20_CHAN_ADC_END)
        {
            if (ADC_MODE_IS_NORM())
            {
                if (val < 0)
                    val = 0;
                if (val > XCDAC20_CHAN_ADC_n_count - 1)
                    val = XCDAC20_CHAN_ADC_n_count - 1;
                if (val < me->ch_cfg[XCDAC20_CHAN_ADC_BEG])
                    val = me->ch_cfg[XCDAC20_CHAN_ADC_BEG];
                me->ch_cfg[x] = val;
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCDAC20_CHAN_ADC_TIMECODE)
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
        else if (x == XCDAC20_CHAN_ADC_GAIN)
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
        else if (x == XCDAC20_CHAN_DO_RESET)
        {
            if (val == CX_VALUE_COMMAND)
            {
                DoSoftReset(me, DSRF_RESET_CHANS | DSRF_RETMODES | DSRF_CUR2OUT);
                SendMULTICHAN(me);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCDAC20_CHAN_ADC_MODE  ||  x == XCDAC20_CHAN_OUT_MODE)
        {
            /* These two are in fact read channels */
            ReturnCtlCh(me, x);
        }
        else if (x == XCDAC20_CHAN_DO_CALB_DAC)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 2;
                item.data[0]  = DESC_CALIBRATE;
                item.data[1]  = 0;
                me->iface->q_enq_ons(me->handle, &item, QFE_IF_ABSENT);
                gettimeofday(&(me->last_calibrate_time), NULL);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == XCDAC20_CHAN_DIGCORR_MODE)
        {
            //fprintf(stderr, "%s(XCDAC20_CHAN_DIGCORR_MODE:%d)\n", __FUNCTION__, action);
            sr = QFE_NOTFOUND;
            if (action == DRVA_WRITE)
            {
                val = val != 0;
                me->ch_cfg[x] = val;

                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 3;
                item.data[0]  = DESC_DIGCORR_SET;
                item.data[1]  = me->ch_cfg[x];
                item.data[2]  = 0;
                sr = me->iface->q_enq_ons(me->handle, &item, QFE_IF_ABSENT);
            }
            if (sr == QFE_NOTFOUND)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = DESC_DIGCORR_STAT;
                me->iface->q_enqueue(me->handle, &item, QFE_ALWAYS);
            }
        }
        else if (x == XCDAC20_CHAN_DIGCORR_VALID  ||
                 x == XCDAC20_CHAN_DIGCORR_FACTOR)
        {
            //fprintf(stderr, "%s(XCDAC20_CHAN_DIGCORR_V:%d)\n", __FUNCTION__, action);
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_DIGCORR_STAT;
            me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
        }
        else if (x == XCDAC20_CHAN_AUTOCALB_ONOFF)
        {
            if (action == DRVA_WRITE)
            {
                val = val != 0;
                me->autocalb_onoff = val;
            }
            ReturnChanGroup(devid, x, 1, &(me->autocalb_onoff), &zero_rflags);
        }
        else if (x == XCDAC20_CHAN_AUTOCALB_SECS)
        {
            if (action == DRVA_WRITE)
            {
                if (val < 0)         val = 0;
                if (val > 86400*365) val = 86400*365;
                me->autocalb_secs = val;
            }
            ReturnChanGroup(devid, x, 1, &(me->autocalb_secs), &zero_rflags);
        }
        else if (x == XCDAC20_CHAN_HW_VER)
        {
            me->iface->get_dev_ver(me->handle, &iv, NULL, NULL);
            val = iv;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == XCDAC20_CHAN_SW_VER)
        {
            me->iface->get_dev_ver(me->handle, NULL, &iv, NULL);
            val = iv;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x >= XCDAC20_CHAN_ADC_n_base  &&
                 x <  XCDAC20_CHAN_ADC_n_base + XCDAC20_CHAN_ADC_n_count)
        {
        }
        else if (x >= XCDAC20_CHAN_OUT_n_base  &&
                 x <  XCDAC20_CHAN_OUT_n_base + XCDAC20_CHAN_OUT_n_count)
        {
            l = x - XCDAC20_CHAN_OUT_n_base;
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
        else if (x >= XCDAC20_CHAN_OUT_RATE_n_base  &&
                 x <  XCDAC20_CHAN_OUT_RATE_n_base + XCDAC20_CHAN_OUT_n_count)
        {
            l = x - XCDAC20_CHAN_OUT_RATE_n_base;
            if (action == DRVA_WRITE)
            {
                /* Note: no bounds-checking for value, since it isn't required */
                if (abs(val) < 305/*!!!*/) val = 0;
                me->ch_spd[l] = val; 
            }
  
            ReturnChanGroup(devid, x, 1, me->ch_spd + l, &zero_rflags);
        }
        else if (x >= XCDAC20_CHAN_OUT_CUR_n_base  &&
                 x <  XCDAC20_CHAN_OUT_CUR_n_base + XCDAC20_CHAN_OUT_n_count)
        {
            l = x - XCDAC20_CHAN_OUT_CUR_n_base;
            if (me->ch_rcv[l])
                ReturnChanGroup(devid, x, 1, me->ch_cur + l, &zero_rflags);
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
 NEXT_CHANNEL:;
    }
}

static void xcdac20_bigc_p(int devid, void *devptr,
                           int chan, int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
}

/* Metric */

static CxsdMainInfoRec xcdac20_main_info[] =
{
    {XCDAC20_MODE_CHAN_count + XCDAC20_CHAN_STD_WR_count,      1}, // Params and table control
    {XCDAC20_CHAN_STD_RD_count,                                0}, // 
    CANKOZ_REGS_MAIN_INFO,
    {XCDAC20_CHAN_REGS_RSVD_E-XCDAC20_CHAN_REGS_RSVD_B+1,      0},
    {KOZDEV_CHAN_ADC_n_maxcnt,                                 0}, // ADC_n
    {KOZDEV_CHAN_OUT_n_maxcnt*2,                               1}, // OUT_n, OUT_RATE_n
    {KOZDEV_CHAN_OUT_n_maxcnt+XCDAC20_CHAN_OUT_RESERVED_count, 0}, // OUT_CUR_n
};

DEFINE_DRIVER(xcdac20, "eXtended CDAC20 support",
              NULL, NULL,
              sizeof(privrec_t), xcdac20_params,
              2, 3,
              NULL, 0,
              countof(xcdac20_main_info), xcdac20_main_info,
              -1, NULL,
              xcdac20_init_d, xcdac20_term_d,
              xcdac20_rw_p, xcdac20_bigc_p);
