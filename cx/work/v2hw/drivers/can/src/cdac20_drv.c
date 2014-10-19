#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/cdac20_drv_i.h"


/* CDAC20 specifics */

enum
{
    DEVTYPE   = 3, /* CDAC20 is 3 */
};

enum
{
    DESC_STOP         = 0x00,
    DESC_MULTICHAN    = 0x01,
    DESC_OSCILL       = 0x02,
    DESC_READONECHAN  = 0x03,
    DESC_READRING     = 0x04,
    DESC_WRITEDAC     = 0x05,
    DESC_READDAC      = 0x06,
    
    MODE_BIT_LOOP = 1 << 4,
    MODE_BIT_SEND = 1 << 5,

    MODE_BYTE = MODE_BIT_LOOP | MODE_BIT_SEND,
};


static inline int32 cdac20_24_to_code(int32 kozak24)
{
    if ((kozak24 &  0x800000) != 0)
        kozak24 -= 0x1000000;

    return kozak24;
}

static inline int32 cdac20_code24_to_val(int32 code)
{
    return scale32via64(code, 10000000, 0x3fffff);
}

static inline int32 cdac20_daccode_to_val(uint32 kozak32)
{
    return scale32via64((int32)kozak32 - 0x800000, 10000000, 0x800000);
}

static inline int32 cdac20_val_to_daccode(int32 val)
{
    return scale32via64(val, 0x800000, 10000000) + 0x800000;
}


typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
    
    int                  ch_beg;
    int                  ch_end;
    int                  timecode;
    int                  fast;
} privrec_t;

static psp_paramdescr_t cdac20_params[] =
{
    PSP_P_INT ("beg",  privrec_t, ch_beg,   0,                          0, CDAC20_CHAN_READ_N_COUNT-1),
    PSP_P_INT ("end",  privrec_t, ch_end,   CDAC20_CHAN_READ_N_COUNT-1, 0, CDAC20_CHAN_READ_N_COUNT-1),
    PSP_P_INT ("time", privrec_t, timecode, 4,                          0, 7),
    PSP_P_FLAG("fast", privrec_t, fast,     1, 0),
    PSP_P_END()
};



static void cdac20_rst(int devid, void *devptr, int is_a_reset);
static void cdac20_in(int devid, void *devptr,
                      int desc, int datasize, uint8 *data);

static int cdac20_init_d(int devid, void *devptr, 
                         int businfocount, int businfo[],
                         const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    if (me->ch_beg > me->ch_end)
    {
        DoDriverLog(devid, 0, "beg=%d > end=%d! Resetting to [0,%d]",
                    me->ch_beg, me->ch_end, CDAC20_CHAN_READ_N_COUNT-1);
        me->ch_beg = 0;
        me->ch_end = CDAC20_CHAN_READ_N_COUNT-1;
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
                                cdac20_rst, cdac20_in,
                                CDAC20_NUMCHANS * 2, CDAC20_CHAN_REGS_BASE);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void cdac20_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_STOP;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, data);
  
    me->iface->disconnect(devid);
}

static void cdac20_rst(int   devid    __attribute__((unused)),
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
        data[4] = MODE_BYTE;
        data[5] = 0; // Label

        me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 6, data);
    }
    else
    {
        data[0] = DESC_OSCILL;
        data[1] = me->ch_beg;
        data[2] = me->timecode;
        data[3] = MODE_BYTE;

        me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 4, data);
    }
}

static void cdac20_in(int devid, void *devptr __attribute__((unused)),
                      int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         chan;
  int32       code;
  int32       val;
  rflags_t    rflags;

    switch (desc)
    {
        case DESC_MULTICHAN:
        case DESC_OSCILL:
        case DESC_READONECHAN:
            if (datasize < 5) return;
            chan = data[1] & 7;
            code   = cdac20_24_to_code(data[2] + (data[3] << 8) + (data[4] << 16));
            val    = cdac20_code24_to_val(code);
            rflags = (code >= -0x3FFFFF && code <= 0x3FFFFF)? 0
                                                            : CXRF_OVERLOAD;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "in: RCHAN=%-2d code=0x%08X val=%duV",
                        chan, code, val);
            ReturnChanGroup(devid,
                            CDAC20_CHAN_READ_N_BASE + chan,
                            1, &val, &rflags);
            break;

        case DESC_READDAC:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 4) return;

            code   = data[1] + (data[2] << 8) + (data[3] << 16);
            val    = cdac20_daccode_to_val(code);
            rflags = 0;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "in: WCHAN code=0x%04X val=%duv",
                        code, val);
            ReturnChanGroup(devid,
                            CDAC20_CHAN_WRITE,
                            1, &val, &rflags);
            break;
    }
}

static void cdac20_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;

  int         code;
  uint8       data[8];
  canqelem_t  item;

  int32       val;
  rflags_t    rflags;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

        if      (x >= CDAC20_CHAN_REGS_BASE  &&  x <= CDAC20_CHAN_REGS_LAST)
            me->iface->regs_rw(me->handle, action, x, values + n);
        else if (x == CDAC20_CHAN_WRITE)
        {
            if (action == DRVA_WRITE)
            {
                /* Convert volts to code, preventing integer overflow/slip */
                code = values[n];
                if (code >   9999999) code =   9999999;
                if (code < -10000000) code = -10000000;
                code = cdac20_val_to_daccode(code);
                DoDriverLog(devid, 0 | DRIVERLOG_C_DATACONV, "%s: w=%d => [%d]:=0x%04X", __FUNCTION__, values[n], x, code);

                data[0] = DESC_WRITEDAC;
                data[1] = code         & 0xFF;
                data[2] = (code >> 8)  & 0xFF;
                data[3] = (code >> 16) & 0xFF;
                data[4] = 0;
                data[5] = 0;
                data[6] = 0;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      7, data);
            }

            /* Perform read anyway */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_READDAC;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
        }
        else if (x == CDAC20_CHAN_DUMMY)
        {
            val    = 0;
            rflags = CXRF_UNSUPPORTED;
            ReturnChanGroup(devid, x, 1, &val, &rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec cdac20_main_info[] =
{
    {CDAC20_CHAN_READ_N_COUNT,   0},
    {1,                          1},  // CDAC20_CHAN_WRITE
    {1,                          1},  // CDAC20_CHAN_DUMMY
    CANKOZ_REGS_MAIN_INFO
};

DEFINE_DRIVER(cdac20, NULL,
              NULL, NULL,
              sizeof(privrec_t), cdac20_params,
              2, 3,
              NULL, 0,
              countof(cdac20_main_info), cdac20_main_info,
              0, NULL,
              cdac20_init_d, cdac20_term_d,
              cdac20_rw_p, NULL);
