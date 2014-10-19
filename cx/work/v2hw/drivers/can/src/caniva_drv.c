#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/caniva_drv_i.h"


/* CANIVA specifics */

enum
{
    DEVTYPE   = 17, /* CANIVA is 17 */
};

static inline int chan2cset(int chan)
{
    return chan & 0x0F;
}

static void CalcAlarms(int32 *curvals, int cset)
{
    curvals[CANIVA_CHAN_IALM_N_BASE + cset] =
        (curvals[CANIVA_CHAN_ILIM_N_BASE + cset] == 0)? 0
        : (curvals[CANIVA_CHAN_IMES_N_BASE + cset] > curvals[CANIVA_CHAN_ILIM_N_BASE + cset]);
    
    curvals[CANIVA_CHAN_UALM_N_BASE + cset] =
        (curvals[CANIVA_CHAN_ULIM_N_BASE + cset] == 0)? 0
        : (curvals[CANIVA_CHAN_UMES_N_BASE + cset] < curvals[CANIVA_CHAN_ULIM_N_BASE + cset]);
}

enum
{
    DESC_VAL_n_BASE   = 0x00,
    DESC_READ_n_BASE  = 0x10,
    DESC_WRITEDEVSTAT = 0xFD,
};


/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

    int32                curvals[CANIVA_NUMCHANS];
} privrec_t;


static void caniva_rst(int devid, void *devptr, int is_a_reset);
static void caniva_in(int devid, void *devptr,
                      int desc, int datasize, uint8 *data);

static int caniva_init_d(int devid, void *devptr, 
                         int businfocount, int businfo[],
                         const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                caniva_rst, caniva_in,
                                10, -1);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void caniva_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_WRITEDEVSTAT;
    data[1] = data[2] = 0;
    data[3] = 0;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 4, data);
  
    me->iface->disconnect(devid);
}

static void caniva_rst(int   devid    __attribute__((unused)),
                       void *devptr,
                       int   is_a_reset __attribute__((unused)))
{
  privrec_t  *me     = (privrec_t *) devptr;
  uint8       data[8];
  
    data[0] = DESC_WRITEDEVSTAT;
    data[1] = data[2] = 0xFF;
    data[3] = 1;

    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 4, data);
}

static void caniva_in(int devid, void *devptr __attribute__((unused)),
                      int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         cset;
  rflags_t    empty_rflags = 0;

  int32           cns         [4];
  int32           vls         [4];
  static rflags_t empt4_rflags[4] = {0, 0, 0, 0};
  
    switch (desc)
    {
        case DESC_VAL_n_BASE  ... DESC_VAL_n_BASE  + 0xF:
        case DESC_READ_n_BASE ... DESC_READ_n_BASE + 0xF:
            if (datasize < 4) return;
            cset = desc & 0x0F;
            me->curvals[CANIVA_CHAN_IMES_N_BASE + cset] = (data[1] * 256 + data[2]) * 2 / 3; /* That f@#king CANIVA uses 300ms integration period, so that all measurements are 1.5 times bigger than they should be... :-( */
            me->curvals[CANIVA_CHAN_UMES_N_BASE + cset] = data[3] * 2 / 3;
            CalcAlarms(me->curvals, cset);
#if 0
            cns[0] = CANIVA_CHAN_IMES_N_BASE + cset;  vls[0] = me->curvals[CANIVA_CHAN_IMES_N_BASE + cset];
            cns[1] = CANIVA_CHAN_UMES_N_BASE + cset;  vls[1] = me->curvals[CANIVA_CHAN_UMES_N_BASE + cset];
            cns[2] = CANIVA_CHAN_IALM_N_BASE + cset;  vls[2] = me->curvals[CANIVA_CHAN_IALM_N_BASE + cset];
            cns[3] = CANIVA_CHAN_UALM_N_BASE + cset;  vls[3] = me->curvals[CANIVA_CHAN_UALM_N_BASE + cset];
            ReturnChanSet(devid, cns, 4, vls, empt4_rflags);
#else
            ReturnChanGroup(devid, CANIVA_CHAN_IMES_N_BASE + cset, 1, me->curvals + CANIVA_CHAN_IMES_N_BASE + cset, &empty_rflags);
            ReturnChanGroup(devid, CANIVA_CHAN_UMES_N_BASE + cset, 1, me->curvals + CANIVA_CHAN_UMES_N_BASE + cset, &empty_rflags);
            ReturnChanGroup(devid, CANIVA_CHAN_IALM_N_BASE + cset, 1, me->curvals + CANIVA_CHAN_IALM_N_BASE + cset, &empty_rflags);
            ReturnChanGroup(devid, CANIVA_CHAN_UALM_N_BASE + cset, 1, me->curvals + CANIVA_CHAN_UALM_N_BASE + cset, &empty_rflags);
#endif
            break;
    }
}

static void caniva_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;
  rflags_t    empty_rflags = 0;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

        /* "Set limit"? */
        if (action == DRVA_WRITE  &&
            (x >= CANIVA_CHAN_ILIM_N_BASE  &&
             x <= CANIVA_CHAN_ULIM_N_BASE+CANIVA_CHAN_ULIM_N_COUNT-1))
        {
            me->curvals[x] = values[n];
            CalcAlarms(me->curvals, chan2cset(x));
        }

        /* Any artificial channel -- return it immediately */
        if (x >= CANIVA_CHAN_ILIM_N_BASE  &&
            x <= CANIVA_CHAN_UALM_N_BASE+CANIVA_CHAN_UALM_N_COUNT-1)
            ReturnChanGroup(devid, x, 1, me->curvals + x, &empty_rflags);
    }
}


/* Metric */

static CxsdMainInfoRec caniva_main_info[] =
{
    {CANIVA_CHAN_IMES_N_COUNT, 0},
    {CANIVA_CHAN_UMES_N_COUNT, 0},
    {CANIVA_CHAN_ILIM_N_COUNT, 1},
    {CANIVA_CHAN_ULIM_N_COUNT, 1},
    {CANIVA_CHAN_ILHW_N_COUNT, 0},
    {CANIVA_CHAN_IALM_N_COUNT, 0},
    {CANIVA_CHAN_UALM_N_COUNT, 0},
};

DEFINE_DRIVER(caniva, "Gudkov/Tararyshkin CAN-IVA",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              2, 3,
              NULL, 0,
              countof(caniva_main_info), caniva_main_info,
              0, NULL,
              caniva_init_d, caniva_term_d,
              caniva_rw_p, NULL);
