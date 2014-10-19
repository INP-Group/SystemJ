#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/cpks8_drv_i.h"


/* CPKS8 specifics */

enum
{
    DEVTYPE   = 7, /* CPKS is 7 */
};

enum
{
    DESC_WRITE_n_BASE  = 0,
    DESC_READ_n_BASE   = 0x10,
};

/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
} privrec_t;


static void cpks8_in(int devid, void *devptr,
                     int desc, int datasize, uint8 *data);

static int cpks8_init_d(int devid, void *devptr, 
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
                                NULL, cpks8_in,
                                CPKS8_NUMCHANS * 2, -1);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void cpks8_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}


static void cpks8_in(int devid, void *devptr,
                     int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int32       val;
  rflags_t    rflags;

    switch (desc)
    {
        case DESC_READ_n_BASE ... DESC_READ_n_BASE + CPKS8_CHAN_WRITE_N_COUNT-1:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 3) return;

            val    = data[1] + data[2]*256;
            rflags = 0;
            ReturnChanGroup(devid, desc - DESC_READ_n_BASE, 1, &val, &rflags);
            break;
    }
}

static void cpks8_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
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

        if (action == DRVA_WRITE)
        {
            code = values[n];
            
            data[0] = DESC_WRITE_n_BASE + x;
            data[1] = code & 0xFF;
            data[2] = (code >> 8) & 0xFF;
            me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                  3, data);
        }
        
        /* Perform read anyway */
        item.prio     = CANKOZ_PRIO_UNICAST;
        item.datasize = 1;
        item.data[0]  = DESC_READ_n_BASE + x;
        me->iface->q_enqueue(me->handle,
                             &item, QFE_IF_NONEORFIRST);
    }
}


/* Metric */

static CxsdMainInfoRec cpks8_main_info[] =
{
    {CPKS8_CHAN_WRITE_N_COUNT, 1}
};

DEFINE_DRIVER(cpks8, NULL,
              NULL, NULL,
              sizeof(privrec_t), NULL,
              2, 3,
              NULL, 0,
              countof(cpks8_main_info), cpks8_main_info,
              0, NULL,
              cpks8_init_d, cpks8_term_d,
              cpks8_rw_p, NULL);
