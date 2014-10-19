#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/cgti_drv_i.h"


/* CGTI specifics */

enum
{
    DEVTYPE   = 26, /* CGTI is 26 */
};

enum
{
    DESC_WRITE_DELAY_n_BASE   = 0x00,
    DESC_READ_DELAY_n_BASE    = 0x10,
    DESC_WRITE_SKPSHFT_n_BASE = 0x20,
    DESC_READ_SKPSHFT_n_BASE  = 0x30,

    DESC_WRITE_MASK           = 0xF0,
    DESC_WRITE_CONFIG         = 0xF1,
    DESC_WRITE_MODE           = 0xF2,
    DESC_WRITE_BASEPERIOD     = 0xF3,
    DESC_WRITE_SYNC_PKT_DELAY = 0xF4,
    DESC_SYNC_PKT             = DESC_WRITE_SYNC_PKT_DELAY,

    DESC_START_PC             = 0xF7,

    DESC_READ_BASEPERIOD      = 0xFB,
    DESC_READ_SYNC_PKT_DELAY  = 0xFC,
    DESC_READ_CONFIG          = 0xFD,
};


/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
    
} privrec_t;

static psp_paramdescr_t cgti_params[] =
{
    PSP_P_END()
};


static void cgti_rst(int devid, void *devptr, int is_a_reset);
static void cgti_in(int devid, void *devptr,
                    int desc, int datasize, uint8 *data);

static int cgti_init_d(int devid, void *devptr, 
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
                                cgti_rst, cgti_in,
                                CGTI_NUMCHANS * 2, -1);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void cgti_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void cgti_rst(int   devid    __attribute__((unused)),
                     void *devptr,
                     int   is_a_reset __attribute__((unused)))
{
  privrec_t  *me     = (privrec_t *) devptr;
  uint8       data[8];
  
}

static void cgti_in(int devid, void *devptr __attribute__((unused)),
                    int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         chan;
  int         magn_code;
  int32       code;
  int32       val;
  rflags_t    rflags;

    switch (desc)
    {
    }
}

static void cgti_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;
  int32       value;
  rflags_t    rflags;

  int         code;
  uint8       data[8];
  canqelem_t  item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) value = values[n];

        if      ()
        {
        }
        else if ()
        {
        }
    }
}


/* Metric */

static CxsdMainInfoRec cgti_main_info[] =
{
};

DEFINE_DRIVER(cgti, NULL,
              NULL, NULL,
              sizeof(privrec_t), cgti_params,
              2, 3,
              NULL, 0,
              countof(cgti_main_info), cgti_main_info,
              0, NULL,
              cgti_init_d, cgti_term_d,
              cgti_rw_p, NULL);
