#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/k5045bbs_drv_i.h"


enum
{
    DEVTYPE = 0x21
};



enum
{
    DESC_HIGH_OFF     = 0x00,
    DESC_HIGH_ENABLE  = 0x01,
    DESC_RST_ILKS     = 0x02,
    DESC_GET_DATA     = 0x03,
    DESC_SUBSCRIBE    = 0x04,
    DESC_UNSUBSCRIBE  = 0x05,
    DESC_SUBSCRIPTION = 0xAA,
};


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

} privrec_t;

static psp_paramdescr_t k5045bbs_params[] =
{
    PSP_P_END()
};


static void k5045bbs_rst(int devid, void *devptr, int is_a_reset);
static void k5045bbs_in(int devid, void *devptr,
                        int desc, int datasize, uint8 *data);

static int k5045bbs_init_d(int devid, void *devptr, 
                           int businfocount, int businfo[],
                           const char *auxinfo)
{
  privrec_t *me      = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                k5045bbs_rst, k5045bbs_in,
                                K5045BBS_NUMCHANS * 2,
                                -1);
    if (me->handle < 0) return me->handle;
    
    return DEVSTATE_OPERATING;
}

static void k5045bbs_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void k5045bbs_rst(int   devid    __attribute__((unused)),
                         void *devptr,
                         int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;

    if (is_a_reset)
    {
    }
}

static void k5045bbs_in(int devid, void *devptr __attribute__((unused)),
                        int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int32        val;     // Value
  int          n;

    switch (desc)
    {
        default:;
    }
}

static void k5045bbs_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int          n;       // channel N in values[] (loop ctl var)
  int          x;       // channel indeX
  int32        val;     // Value
  uint8        data[8];
  int          nc;

  canqelem_t   item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x == 0)
        {
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec k5045bbs_main_info[] =
{
};

DEFINE_DRIVER(k5045bbs, "Chupyra's BBS for K5045 klystron",
              NULL, NULL,
              sizeof(privrec_t), k5045bbs_params,
              2, 3,
              NULL, 0,
              countof(k5045bbs_main_info), k5045bbs_main_info,
              0, NULL,
              k5045bbs_init_d, k5045bbs_term_d,
              k5045bbs_rw_p, NULL);
