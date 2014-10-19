#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/candac16_drv_i.h"


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
};


static inline int32 candac16_to_val(uint16 kozak16)
{
    return scale32via64((int32)kozak16 - 0x8000, 10000000, 0x8000);
}

static inline int32 val_to_candac16(int32 val)
{
    return scale32via64(val, 0x8000, 10000000) + 0x8000;
}

/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
} privrec_t;


static void candac16_in(int devid, void *devptr,
                        int desc, int datasize, uint8 *data);

static int candac16_init_d(int devid, void *devptr, 
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
                                NULL, candac16_in,
                                CANDAC16_NUMCHANS * 2, CANDAC16_CHAN_REGS_BASE);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void candac16_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}


static void candac16_in(int devid, void *devptr,
                        int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         chan;
  int32       val;
  rflags_t    rflags;

    switch (desc)
    {
        case DESC_READ_n_BASE ... DESC_READ_n_BASE + 0xF:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 3) return;

            chan   = desc - DESC_READ_n_BASE;
            val    = candac16_to_val(data[1] + data[2]*256);
            rflags = 0;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "%s(#%d:=%d)", __FUNCTION__, chan, val);
            ReturnChanGroup(devid,
                            CANDAC16_CHAN_WRITE_N_BASE + chan,
                            1, &val, &rflags);
            break;
    }
}

static void candac16_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;

  int         code;
  uint8       data[8];
  canqelem_t  item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

        if (x >= CANDAC16_CHAN_REGS_BASE  &&  x <= CANDAC16_CHAN_REGS_LAST)
            me->iface->regs_rw(me->handle, action, x, values + n);
        else
        {
            if (action == DRVA_WRITE)
            {
                /* Convert volts to code, preventing integer overflow/slip */
                code = values[n];
                if (code >   9999999) code =   9999999;
                if (code < -10000305) code = -10000305;
                code = val_to_candac16(code);
                DoDriverLog(devid, 0 | DRIVERLOG_C_DATACONV, "%s: w=%d => [%d]:=0x%04X", __FUNCTION__, values[n], x, code);

                data[0] = DESC_WRITE_n_BASE + (x - CANDAC16_CHAN_WRITE_N_BASE);
                data[1] = code & 0xFF;
                data[2] = (code >> 8) & 0xFF;
                data[3] = 0;
                data[4] = 0;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      5, data);
            }

            /* Perform read anyway */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_READ_n_BASE + (x - CANDAC16_CHAN_WRITE_N_BASE);
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
        }
    }
}


/* Metric */

static CxsdMainInfoRec candac16_main_info[] =
{
    {CANDAC16_CHAN_WRITE_N_COUNT, 1},
    CANKOZ_REGS_MAIN_INFO
};

DEFINE_DRIVER(candac16, NULL,
              NULL, NULL,
              sizeof(privrec_t), NULL,
              2, 3,
              NULL, 0,
              countof(candac16_main_info), candac16_main_info,
              0, NULL,
              candac16_init_d, candac16_term_d,
              candac16_rw_p, NULL);
