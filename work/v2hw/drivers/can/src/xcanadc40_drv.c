#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/xcanadc40_drv_i.h"


/* CANADC40 specifics */

enum
{
    DEVTYPE   = 2, /* CANADC40 is 2 */
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

static inline int32 canadc40_24_to_code(int32 kozak24)
{
    if ((kozak24 &  0x800000) != 0)
        kozak24 -= 0x1000000;

    return kozak24;
}

static int32 canadc40_code24_to_val(int32 code, int magn_code)
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

static psp_paramdescr_t xcanadc40_params[] =
{
    PSP_P_INT ("beg",  privrec_t, ch_beg,   0,                            0, XCANADC40_CHAN_ADC_n_count-1),
    PSP_P_INT ("end",  privrec_t, ch_end,   XCANADC40_CHAN_ADC_n_count-1, 0, XCANADC40_CHAN_ADC_n_count-1),
    PSP_P_INT ("time", privrec_t, timecode, 4,                            0, 7),
    PSP_P_INT ("magn", privrec_t, magn_evn, 0,                            0, 1),
    PSP_P_FLAG("fast", privrec_t, fast,     1, 0),
    PSP_P_END()
};


static void xcanadc40_rst(int devid, void *devptr, int is_a_reset);
static void xcanadc40_in(int devid, void *devptr,
                         int desc, int datasize, uint8 *data);

static int xcanadc40_init_d(int devid, void *devptr,
                            int businfocount, int businfo[],
                            const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    me->magn_odd = me->magn_evn;

    if (me->ch_beg > me->ch_end)
    {
        DoDriverLog(devid, 0, "beg=%d > end=%d! Resetting to [0,%d]",
                    me->ch_beg, me->ch_end, XCANADC40_CHAN_ADC_n_count-1);
        me->ch_beg = 0;
        me->ch_end = XCANADC40_CHAN_ADC_n_count-1;
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
                                xcanadc40_rst, xcanadc40_in,
                                XCANADC40_NUMCHANS * 2, XCANADC40_CHAN_REGS_base);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void xcanadc40_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_STOP;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, data);
  
    me->iface->disconnect(devid);
}

static void xcanadc40_rst(int   devid    __attribute__((unused)),
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

static void xcanadc40_in(int devid, void *devptr __attribute__((unused)),
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
            code   = canadc40_24_to_code(data[2] + (data[3] << 8) + (data[4] << 16));
            val    = canadc40_code24_to_val(code, magn_code);
            rflags = (code >= -0x3FFFFF && code <= 0x3FFFFF)? 0
                                                            : CXRF_OVERLOAD;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "CHAN=%-2d code=0x%08X magn=%d val=%duV",
                        chan, code, magn_code, val);
            if (chan < 0  ||  chan >= XCANADC40_CHAN_ADC_n_count) return;
            ReturnChanGroup(devid,
                            XCANADC40_CHAN_ADC_n_base + chan,
                            1, &val, &rflags);
            break;
    }
}

static void xcanadc40_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

        if (x >= XCANADC40_CHAN_REGS_base  &&  x <= XCANADC40_CHAN_REGS_last)
            me->iface->regs_rw(me->handle, action, x, values + n);
        /* else DO-NOTHING --
           no other write channels, and no need to call reads */
    }
}


/* Metric */

#if 0
static CxsdMainInfoRec xcanadc40_main_info[] =
{
    {XCANADC40_CHAN_ADC_n_count, 0},
    CANKOZ_REGS_MAIN_INFO
};
#endif

DEFINE_DRIVER(xcanadc40, NULL,
              NULL, NULL,
              sizeof(privrec_t), xcanadc40_params,
              2, 3,
              NULL, 0,
              /*countof(xcanadc40_main_info), xcanadc40_main_info,*/-1, NULL,
              0, NULL,
              xcanadc40_init_d, xcanadc40_term_d,
              xcanadc40_rw_p, NULL);
