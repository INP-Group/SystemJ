#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "timeval_utils.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/xcandac16_drv_i.h"


/* CANDAC16 specifics */

enum
{
    DEVTYPE   = 1, /* CANDAC16 is 1 */
};

enum
{
    DESC_WRITE_n_BASE  = 0,
    DESC_READ_n_BASE   = 0x10,

    DESC_WRITE_FILE_AT = 0xF2,
    DESC_CREATE_FILE   = 0xF3,
    DESC_WRITE_FILE    = 0xF4,
    DESC_CLOSE_FILE    = 0xF5,
    DESC_READ_FILE     = 0xF6,
    DESC_START_FILE    = 0xF7,

    DESC_B_FILE_STOP   = 1,
    DESC_B_FILE_START  = 2,
    DESC_B_ADC_STOP    = 3,
    DESC_B_ADC_START   = 4,
    DESC_B_FILE_PAUSE  = 6,
    DESC_B_FILE_RESUME = 7,
    
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
};


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

    int32            ch_cfg[XCANDAC16_MODE_CHAN_count];  // Cache of mode channels
    int32            ch_cur[XCANDAC16_CHAN_OUT_n_count]; // Current value
    int32            ch_rcv[XCANDAC16_CHAN_OUT_n_count]; // Current value was received

    int32            ch_spd[XCANDAC16_CHAN_OUT_n_count]; // Speed
    int32            ch_trg[XCANDAC16_CHAN_OUT_n_count]; // Target
    
    int8             ch_lkd[XCANDAC16_CHAN_OUT_n_count]; // Is locked by table
    int8             ch_isc[XCANDAC16_CHAN_OUT_n_count]; // Is changing now
    int8             ch_nxt[XCANDAC16_CHAN_OUT_n_count]; // Next write may be performed (previous was acknowledged)
    int8             ch_fst[XCANDAC16_CHAN_OUT_n_count]; // This is the FirST step
    int8             ch_fin[XCANDAC16_CHAN_OUT_n_count]; // This is a FINal write

    int              t_aff_c [XCANDAC16_CHAN_OUT_n_count];
    int              t_mode;
} privrec_t;

static psp_paramdescr_t xcandac16_params[] =
{
    PSP_P_INT ("spd",  privrec_t, ch_spd[0], 0, -20000000, +20000000),
    PSP_P_END()
};


/*=== ZZZ ===*/

static int8 UNSUPPORTED_chans[XCANDAC16_MODE_CHAN_count] =
{
    [XCANDAC16_CHAN_RESERVED6]  = 1,
    [XCANDAC16_CHAN_RESERVED7]  = 1,
    [XCANDAC16_CHAN_RESERVED8]  = 1,
    [XCANDAC16_CHAN_RESERVED16] = 1,
    [XCANDAC16_CHAN_RESERVED17] = 1,
    [XCANDAC16_CHAN_RESERVED18] = 1,
    [XCANDAC16_CHAN_RESERVED19] = 1,
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
  static int32 table_ctls[][XCANDAC16_CHAN_DO_TAB_cnt] =
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
    memcpy(me->ch_cfg + XCANDAC16_CHAN_DO_TAB_base,
           &(table_ctls[mode]), sizeof(table_ctls[mode]));

    for (x = 0;  x < XCANDAC16_CHAN_DO_TAB_cnt;  x++)
        ReturnCtlCh(me, XCANDAC16_CHAN_DO_TAB_base + x);

    me->ch_cfg[XCANDAC16_CHAN_OUT_MODE] = (mode < TMODE_LOAD? XCANDAC16_OUT_MODE_NORM
                                                            : XCANDAC16_OUT_MODE_TABLE);
    ReturnCtlCh(me, XCANDAC16_CHAN_OUT_MODE);
}

static void DoSoftReset(privrec_t *me, int flags)
{
  int  x;
  
    if (flags & DSRF_RESET_CHANS)
    {
        me->ch_cfg[XCANDAC16_CHAN_ADC_BEG]      = me->ch_beg;
        me->ch_cfg[XCANDAC16_CHAN_ADC_END]      = me->ch_end;
        me->ch_cfg[XCANDAC16_CHAN_ADC_TIMECODE] = me->timecode;
        me->ch_cfg[XCANDAC16_CHAN_ADC_GAIN]     = me->gain;
    }
    
    /*!!! bzero() something? ch_nxt[]:=1? */

    if (flags & DSRF_RETMODES)
        for (x = 0;  x < XCANDAC16_MODE_CHAN_count;  x++)
            ReturnCtlCh(me, x);

    if (flags & DSRF_CUR2OUT)
        for (x = 0;  x < XCANDAC16_CHAN_OUT_n_count;  x++)
            ReturnChanGroup(me->devid, XCANDAC16_CHAN_OUT_n_base + x, 1,
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
    item.data[0]  = DESC_WRITE_n_BASE + l;
    item.data[1]  = code         & 0xFF;
    item.data[2]  = (code >> 8)  & 0xFF;
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
    item.data[0]  = DESC_READ_n_BASE + l;
    me->iface->q_enqueue(me->handle,
                         &item, QFE_IF_NONEORFIRST);
}


/*=== ZZZ ===*/

static void xcandac16_rst(int devid, void *devptr, int is_a_reset);
static void xcandac16_in (int devid, void *devptr,
                          int desc, int datasize, uint8 *data);
static void xcandac16_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action);
static void xcandac16_hbt(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr);

static int xcandac16_init_d(int devid, void *devptr, 
                            int businfocount, int businfo[],
                            const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
  int        l;       // Line #
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    for (l = 1;  l < XCANDAC16_CHAN_OUT_n_count;  l++)
        me->ch_spd[l] = me->ch_spd[0];

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->devid  = devid;
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                xcandac16_rst, xcandac16_in,
                                XCANDAC16_NUMCHANS * 2,
                                XCANDAC16_CHAN_REGS_base);
    if (me->handle < 0) return me->handle;

    DoSoftReset(me, DSRF_RESET_CHANS);
    SetTmode   (me, TMODE_NONE);
    
    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, xcandac16_hbt, NULL);

    return DEVSTATE_OPERATING;
}

static void xcandac16_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void xcandac16_rst(int   devid    __attribute__((unused)),
                          void *devptr,
                          int   is_a_reset)
{
  privrec_t  *me     = (privrec_t *) devptr;

    if (is_a_reset)
    {
        bzero(me->ch_rcv, sizeof(me->ch_rcv));
        DoSoftReset(me, 0);
    }
}

static void xcandac16_hbt(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         l;       // Line #
  int32       val;     // Value -- in volts
  struct timeval  now;

    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, xcandac16_hbt, NULL);

    /*  */
    if (me->num_isc > 0)
        for (l = 0;  l < XCANDAC16_CHAN_OUT_n_count;  l++)
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
        for (l = 0;  l < XCANDAC16_CHAN_OUT_n_count;  l++)
            if (me->t_aff_c[l]) SendRdRq(me, l);
}

static void xcandac16_in (int devid, void *devptr __attribute__((unused)),
                          int desc, int datasize, uint8 *data)
{
  privrec_t  *me       = (privrec_t *) devptr;
  int         l;       // Line #
  int32       code;    // Code -- in device/Kozak encoding
  int32       val;     // Value -- in volts
  rflags_t    rflags;

    switch (desc)
    {
        case DESC_READ_n_BASE ... DESC_READ_n_BASE + XCANDAC16_CHAN_OUT_n_count-1:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 3) return;

            l      = desc - DESC_READ_n_BASE;
            code   = data[1] + (data[2] << 8);
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
                            XCANDAC16_CHAN_OUT_CUR_n_base + l,
                            1, me->ch_cur + l, &rflags);
            if      (!me->ch_isc[l]  ||  !SHOW_SET_IMMED)
                ReturnChanGroup(devid,
                                XCANDAC16_CHAN_OUT_n_base + l,
                                1, me->ch_cur + l, &rflags);
            else if (me->ch_fst[l])
            {
                me->ch_fst[l] = 0;
                val = kozdac_c16_to_val(kozdac_val_to_c16(me->ch_trg[l]));
                ReturnChanGroup(devid,
                                XCANDAC16_CHAN_OUT_n_base + l,
                                1, &val, &rflags);
            }

            break;

        default:
            ////if (desc != 0xF8) DoDriverLog(devid, 0, "desc=%02x len=%d", desc, datasize);
            ;
    }
}

static void xcandac16_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
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

        if      (x >= XCANDAC16_CHAN_REGS_base  &&  x <= XCANDAC16_CHAN_REGS_last)
            me->iface->regs_rw(me->handle, action, x, values + n);
        else if (
                 ( // Is it a config channel?
                  x >= XCANDAC16_MODE_CHAN_base  &&
                  x <  XCANDAC16_MODE_CHAN_base + XCANDAC16_MODE_CHAN_count
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
        else if (x == XCANDAC16_CHAN_DO_RESET)
        {
            if (val == CX_VALUE_COMMAND)
            {
                DoSoftReset(me, DSRF_RESET_CHANS | DSRF_RETMODES | DSRF_CUR2OUT);
            }
            ReturnCtlCh(me, x);
        }
        else if (x == XCANDAC16_CHAN_OUT_MODE)
        {
            /* These two are in fact read channels */
            ReturnCtlCh(me, x);
        }
        else if (x == XCANDAC16_CHAN_HW_VER)
        {
            me->iface->get_dev_ver(me->handle, &iv, NULL, NULL);
            val = iv;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == XCANDAC16_CHAN_SW_VER)
        {
            me->iface->get_dev_ver(me->handle, NULL, &iv, NULL);
            val = iv;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x >= XCANDAC16_CHAN_OUT_n_base  &&
                 x <  XCANDAC16_CHAN_OUT_n_base + XCANDAC16_CHAN_OUT_n_count)
        {
            l = x - XCANDAC16_CHAN_OUT_n_base;
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
        else if (x >= XCANDAC16_CHAN_OUT_RATE_n_base  &&
                 x <  XCANDAC16_CHAN_OUT_RATE_n_base + XCANDAC16_CHAN_OUT_n_count)
        {
            l = x - XCANDAC16_CHAN_OUT_RATE_n_base;
            if (action == DRVA_WRITE)
            {
                /* Note: no bounds-checking for value, since it isn't required */
                if (abs(val) < 305/*!!!*/) val = 0;
                me->ch_spd[l] = val; 
            }
  
            ReturnChanGroup(devid, x, 1, me->ch_spd + l, &zero_rflags);
        }
        else if (x >= XCANDAC16_CHAN_OUT_CUR_n_base  &&
                 x <  XCANDAC16_CHAN_OUT_CUR_n_base + XCANDAC16_CHAN_OUT_n_count)
        {
            l = x - XCANDAC16_CHAN_OUT_CUR_n_base;
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

static void xcandac16_bigc_p(int devid, void *devptr,
                             int chan, int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
}

/* Metric */

static CxsdMainInfoRec xcandac16_main_info[] =
{
    {XCANDAC16_MODE_CHAN_count + XCANDAC16_CHAN_STD_WR_count,      1}, // Params and table control
    {XCANDAC16_CHAN_STD_RD_count,                                0}, // 
    CANKOZ_REGS_MAIN_INFO,
    {XCANDAC16_CHAN_REGS_RSVD_E-XCANDAC16_CHAN_REGS_RSVD_B+1,      0},
    {KOZDEV_CHAN_ADC_n_maxcnt,                                 0}, // ADC_n
    {KOZDEV_CHAN_OUT_n_maxcnt*2,                               1}, // OUT_n, OUT_RATE_n
    {KOZDEV_CHAN_OUT_n_maxcnt+XCANDAC16_CHAN_OUT_RESERVED_count, 0}, // OUT_CUR_n
};

DEFINE_DRIVER(xcandac16, "eXtended CDAC20 support",
              NULL, NULL,
              sizeof(privrec_t), xcandac16_params,
              2, 3,
              NULL, 0,
              countof(xcandac16_main_info), xcandac16_main_info,
              -1, NULL,
              xcandac16_init_d, xcandac16_term_d,
              xcandac16_rw_p, xcandac16_bigc_p);
