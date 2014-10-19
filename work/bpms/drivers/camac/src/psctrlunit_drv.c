#include "cxsd_driver.h"

#include "drv_i/psctrlunit_drv_i.h"
#include "psctrlunit_io.h"


enum
{
    FAC_TYPE_UNKNOWN = 0,
    FAC_TYPE_V2K     = 1,
    FAC_TYPE_VEPP5   = 2,
};

typedef struct
{
    int                N;
    int                ftype;
    psctrlunit_dscr_t  pd;
} psctrlunit_privrec_t;

static psp_paramdescr_t psctrlunit_params[] =
{
    PSP_P_FLAG("v2k",   psctrlunit_privrec_t, ftype, FAC_TYPE_V2K,   0),
    PSP_P_FLAG("vepp5", psctrlunit_privrec_t, ftype, FAC_TYPE_VEPP5, 0),
    PSP_P_END()
};


static int psctrlunit_init_d(int devid, void *devptr,
                             int businfocount, int *businfo,
                             const char *auxinfo)
{
  psctrlunit_privrec_t *me = (psctrlunit_privrec_t*)devptr;
  
    me->N = businfo[0];
    if      (me->ftype == FAC_TYPE_V2K)   me->pd = v2k_pd;
    else if (me->ftype == FAC_TYPE_VEPP5) me->pd = vepp5_pd;
    else
    {
        DoDriverLog(devid, DRIVERLOG_CRIT,
                    "type specivication (v2k/vepp5) is mandatory");
        return -CXRF_CFG_PROBL;
    }

    return DEVSTATE_OPERATING;
}

static void psctrlunit_rw_p(int devid, void *devptr,
                            int first, int count, int32 *values, int action)
{
  psctrlunit_privrec_t *me = (psctrlunit_privrec_t*)devptr;

  int                   n;
  int                   x;
  
  int32                 val;
  rflags_t              rflags;
  
    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if (x >= PSCTRLUNIT_CHAN_GAIN0  &&  
            x <  PSCTRLUNIT_CHAN_GAIN0 + PSCTRLUNIT_NUM_GAIN_CHANS)
            psctrlunit_do_io(&(me->pd),
                             CAMAC_REF,
                             me->N, x - PSCTRLUNIT_CHAN_GAIN0, action,
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

static CxsdMainInfoRec psctrlunit_main_info[] =
{
    {PSCTRLUNIT_NUMCHANS, 1},
};

DEFINE_DRIVER(psctrlunit, "V2K/VEPP5 ring BPM's psctrlunit",
              NULL, NULL,
              sizeof(psctrlunit_privrec_t), psctrlunit_params,
              1, 1,
              NULL, 0,
              countof(psctrlunit_main_info), psctrlunit_main_info,
              -1, NULL,
              psctrlunit_init_d, NULL, psctrlunit_rw_p, NULL);
