#include "misc_macros.h"
#include "pzframe_drv.h"


#ifndef FASTADC_NAME
  #error The "FASTADC_NAME" is undefined
#endif


#define FASTADC_PRIVREC_T __CX_CONCATENATE(FASTADC_NAME,_privrec_t)
#define FASTADC_PARAMS    __CX_CONCATENATE(FASTADC_NAME,_params)
#define FASTADC_LAM_CB    __CX_CONCATENATE(FASTADC_NAME,_lam_cb)
#define FASTADC_INIT_D    __CX_CONCATENATE(FASTADC_NAME,_init_d)
#define FASTADC_TERM_D    __CX_CONCATENATE(FASTADC_NAME,_term_d)
#define FASTADC_RW_P      __CX_CONCATENATE(FASTADC_NAME,_rw_p)
#define FASTADC_BIGC_P    __CX_CONCATENATE(FASTADC_NAME,_bigc_p)


static void FASTADC_LAM_CB(int devid, void *devptr)
{
  FASTADC_PRIVREC_T   *me = (FASTADC_PRIVREC_T *)devptr;

    pzframe_drv_drdy_p(&(me->pz), 1, 0);
}

static int  FASTADC_INIT_D(int devid, void *devptr, 
                           int businfocount, int businfo[],
                           const char *auxinfo __attribute__((unused)))
{
  FASTADC_PRIVREC_T   *me = (FASTADC_PRIVREC_T *)devptr;
  const char          *errstr;

    me->devid  = devid;
    me->N_DEV  = businfo[0];

    pzframe_drv_init(&(me->pz), devid,
                     NUM_PARAMS,
                     PARAM_SHOT, PARAM_ISTART, PARAM_WAITTIME, PARAM_STOP, PARAM_ELAPSED,
                     ValidateParam,
                     StartMeasurements, TrggrMeasurements,
                     AbortMeasurements, ReadMeasurements);
    pzframe_set_buf_p(&(me->pz), me->retdata);

    if ((errstr = WATCH_FOR_LAM(devid, devptr, me->N_DEV, FASTADC_LAM_CB)) != NULL)
    {
        DoDriverLog(devid, DRIVERLOG_ERRNO, "WATCH_FOR_LAM(): %s", errstr);
        return -CXRF_DRV_PROBL;
    }

    return InitParams(&(me->pz));
}

static void FASTADC_TERM_D(int devid, void *devptr)
{
  FASTADC_PRIVREC_T   *me = (FASTADC_PRIVREC_T *)devptr;

    pzframe_drv_term(&(me->pz));
}

static void FASTADC_RW_P  (int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  FASTADC_PRIVREC_T   *me = (FASTADC_PRIVREC_T *)devptr;

    pzframe_drv_rw_p(&(me->pz), firstchan, count, values, action);
}

static void FASTADC_BIGC_P(int devid, void *devptr,
                           int chan __attribute__((unused)),
                           int32 *args, int nargs,
                           void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  FASTADC_PRIVREC_T   *me = (FASTADC_PRIVREC_T *)devptr;

    pzframe_drv_bigc_p(&(me->pz), args, nargs);
}

DEFINE_DRIVER(FASTADC_NAME, __CX_STRINGIZE(FASTADC_NAME) " fast-ADC",
              NULL, NULL,
              sizeof(FASTADC_PRIVREC_T), FASTADC_PARAMS,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              FASTADC_INIT_D, FASTADC_TERM_D,
              FASTADC_RW_P, FASTADC_BIGC_P);
