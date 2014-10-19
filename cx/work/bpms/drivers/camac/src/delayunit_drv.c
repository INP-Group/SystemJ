#include "cxsd_driver.h"

#include "drv_i/delayunit_drv_i.h"
#include "delayunit_io.h"


enum
{
    FAC_TYPE_UNKNOWN = 0,
    FAC_TYPE_V2K     = 1,
    FAC_TYPE_VEPP5   = 2,
};

typedef struct
{
    int               N;
    int               ftype;
    delayunit_dscr_t  dd;
} delayunit_privrec_t;

static psp_paramdescr_t delayunit_params[] =
{
    PSP_P_FLAG("v2k",   delayunit_privrec_t, ftype, FAC_TYPE_V2K,   0),
    PSP_P_FLAG("vepp5", delayunit_privrec_t, ftype, FAC_TYPE_VEPP5, 0),
    PSP_P_END()
};


static int delayunit_init_d(int devid, void *devptr,
                            int businfocount, int *businfo,
                            const char *auxinfo)
{
  delayunit_privrec_t *me = (delayunit_privrec_t*)devptr;
  
    me->N = businfo[0];
    if      (me->ftype == FAC_TYPE_V2K)   me->dd = v2k_dd;
    else if (me->ftype == FAC_TYPE_VEPP5) me->dd = vepp5_dd;
    else
    {
        DoDriverLog(devid, DRIVERLOG_CRIT,
                    "type specivication (v2k/vepp5) is mandatory");
        return -CXRF_CFG_PROBL;
    }

    return DEVSTATE_OPERATING;
}

static void delayunit_rw_p(int devid, void *devptr,
                           int first, int count, int32 *values, int action)
{
  delayunit_privrec_t *me = (delayunit_privrec_t*)devptr;

  int                  n;
  int                  x;
  
  int32                val;
  rflags_t             rflags;
  
    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if (x >= DELAYUNIT_CHAN_0  &&  x <= DELAYUNIT_CHAN_3)
            delayunit_do_io(&(me->dd),
                            CAMAC_REF,
                            me->N, x - DELAYUNIT_CHAN_0, action,
                            &val, &rflags,
                            NULL);
        else
        {
            val    = 0;
            rflags = CXRF_UNSUPPORTED;
        }
        ReturnChanGroup(devid, x, 1, &val, &rflags);
    }
}

static CxsdMainInfoRec delayunit_main_info[] =
{
    {DELAYUNIT_NUMCHANS, 1},
};

DEFINE_DRIVER(delayunit, "V2K/VEPP5 ring BPM's delayunit",
              NULL, NULL,
              sizeof(delayunit_privrec_t), delayunit_params,
              1, 1,
              NULL, 0,
              countof(delayunit_main_info), delayunit_main_info,
              -1, NULL,
              delayunit_init_d, NULL, delayunit_rw_p, NULL);
