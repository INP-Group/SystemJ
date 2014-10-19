#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/xcead20_drv_i.h"


/* XCEAD20 specifics */

enum
{
    DEVTYPE   = 23, /* CEAD20 is 2 */
};

enum
{
    DESC_STOP        = 0x00,
    DESC_MULTICHAN   = 0x01,
    DESC_OSCILL      = 0x02,
    DESC_READONECHAN = 0x03,
    DESC_READRING    = 0x04,

    MODE_BIT_LOOP = 1 << 4,
    MODE_BIT_SEND = 1 << 5,
};

static inline uint8 PackMode(int magn_evn, int magn_odd)
{
    return magn_evn | (magn_odd << 2) | MODE_BIT_LOOP | MODE_BIT_SEND;
}

static inline void  decode_attr(uint8 attr, int *chan_p, int *magn_code_p)
{
    *chan_p      = attr & 63;
    *magn_code_p = (attr >> 6) & 3;
}

static inline int32 xcead20_24_to_code(int32 kozak24)
{
    if ((kozak24 &  0x800000) != 0)
        kozak24 -= 0x1000000;

    return kozak24;
}

static int32 xcead20_code24_to_val(int32 code, int magn_code)
{
  static int ranges[4] =
        {
            10000000 / 1,
            10000000 / 10,
            10000000 / 100,
            10000000 / 1000
        };
    
    return scale32via64(code, ranges[magn_code], 0x3fffff);
}


/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
    
    int                  ch_beg;
    int                  ch_end;
    int                  timecode;
    int                  magn_evn;
    int                  magn_odd;
    int                  fast;
} privrec_t;

static psp_paramdescr_t xcead20_params[] =
{
    PSP_P_INT ("beg",  privrec_t, ch_beg,   0,                          0, XCEAD20_CHAN_ADC_n_count-1),
    PSP_P_INT ("end",  privrec_t, ch_end,   XCEAD20_CHAN_ADC_n_count-1, 0, XCEAD20_CHAN_ADC_n_count-1),
    PSP_P_INT ("time", privrec_t, timecode, 4,                          0, 7),
    PSP_P_INT ("magn", privrec_t, magn_evn, 0,                          0, 1),
    PSP_P_FLAG("fast", privrec_t, fast,     1, 0),
    PSP_P_END()
};


static void xcead20_rst(int devid, void *devptr, int is_a_reset);
static void xcead20_in(int devid, void *devptr,
                       int desc, int datasize, uint8 *data);

static int xcead20_init_d(int devid, void *devptr, 
                          int businfocount, int businfo[],
                          const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    if (me->ch_beg > me->ch_end)
    {
        DoDriverLog(devid, 0, "beg=%d > end=%d! Resetting to [0,%d]",
                    me->ch_beg, me->ch_end, XCEAD20_CHAN_ADC_n_count-1);
        me->ch_beg = 0;
        me->ch_end = XCEAD20_CHAN_ADC_n_count-1;
    }
    
    if (me->fast  &&  me->ch_beg != me->ch_end)
    {
        DoDriverLog(devid, 0, "the \"fast\" mode is available only when single channel is measured, not [%d..%d]",
                    me->ch_beg, me->ch_end);
        me->fast = 0;
    }

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                xcead20_rst, xcead20_in,
                                XCEAD20_NUMCHANS * 2, XCEAD20_CHAN_REGS_base);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void xcead20_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_STOP;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, data);
  
    me->iface->disconnect(devid);
}

static void xcead20_rst(int   devid      __attribute__((unused)),
                        void *devptr,
                        int   is_a_reset __attribute__((unused)))
{
  privrec_t  *me     = (privrec_t *) devptr;
  uint8       data[8];

    if (!me->fast)
    {
        data[0] = DESC_MULTICHAN;
        data[1] = me->ch_beg;
        data[2] = me->ch_end;
        data[3] = me->timecode;
        data[4] = PackMode(me->magn_evn, me->magn_odd);
        data[5] = 0; // Label

        me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 6, data);
    }
    else
    {
        data[0] = DESC_OSCILL;
        data[1] = me->ch_beg | (me->magn_evn << 6);
        data[2] = me->timecode;
        data[3] = PackMode(0, 0);

        me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 4, data);
    }
}

static void xcead20_in(int devid, void *devptr __attribute__((unused)),
                       int desc, int datasize, uint8 *data)
{
  int         chan;
  int         magn_code;
  int32       code;
  int32       val;
  rflags_t    rflags;

    switch (desc)
    {
        case DESC_MULTICHAN:
        case DESC_OSCILL:
        case DESC_READONECHAN:
            if (datasize < 5) return;
            decode_attr(data[1], &chan, &magn_code);
            code   = xcead20_24_to_code(data[2] + (data[3] << 8) + (data[4] << 16));
            val    = xcead20_code24_to_val(code, magn_code);
            rflags = (code >= -0x3FFFFF && code <= 0x3FFFFF)? 0
                                                            : CXRF_OVERLOAD;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "CHAN=%-2d code=0x%08X magn=%d val=%duV",
                        chan, code, magn_code, val);
            if (chan < 0  ||  chan >= XCEAD20_CHAN_ADC_n_count) return;
            ReturnChanGroup(devid,
                            XCEAD20_CHAN_ADC_n_base + chan,
                            1, &val, &rflags);
            break;
    }
}

static void xcead20_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

        if (x >= XCEAD20_CHAN_REGS_base  &&  x <= XCEAD20_CHAN_REGS_last)
            me->iface->regs_rw(me->handle, action, x, values + n);
        /* else DO-NOTHING --
           no other write channels, and no need to call reads */
    }
}


/* Metric */

static CxsdMainInfoRec xcead20_main_info[] =
{
    {XCEAD20_MODE_CHAN_count + XCEAD20_CHAN_STD_WR_count,      1}, // Params and table control
    {XCEAD20_CHAN_STD_RD_count,                                0}, // 
    CANKOZ_REGS_MAIN_INFO,
    {XCEAD20_CHAN_REGS_RSVD_E-XCEAD20_CHAN_REGS_RSVD_B+1,      0},
    {KOZDEV_CHAN_ADC_n_maxcnt,                                 0}, // ADC_n
    {KOZDEV_CHAN_OUT_n_maxcnt*2,                               1}, // OUT_n, OUT_RATE_n
    {KOZDEV_CHAN_OUT_n_maxcnt+XCEAD20_CHAN_OUT_RESERVED_count, 0}, // OUT_CUR_n
};

DEFINE_DRIVER(xcead20, "eXtended CEAD20 support",
              NULL, NULL,
              sizeof(privrec_t), xcead20_params,
              2, 3,
              NULL, 0,
              countof(xcead20_main_info), xcead20_main_info,
              0, NULL,
              xcead20_init_d, xcead20_term_d,
              xcead20_rw_p, NULL);
