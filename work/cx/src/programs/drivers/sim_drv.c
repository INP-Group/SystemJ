#include "cxsd_driver.h"


typedef struct
{
    size_t  units;      // Dataunits -- 1, 2, 4
    int     length;     // Length of data (in dataunits)
    int     period;     // Period of simulated saw
    int     magn;       // Magnitude of saw (max-min!)
    int     zero;       // Zero shift
    int     msecs;      // >0 -- period between consecutive ReturnBigc()s
    int     shift;      // What to add to precession
    int     nparams;    // # of parameters
    int32  *params;
    void   *data;
    int     precession;
} privrec_t;

static psp_paramdescr_t sim_params[] =
{
    PSP_P_FLAG("b",       privrec_t, units,     1,  1),
    PSP_P_FLAG("s",       privrec_t, units,     2,  0),
    PSP_P_FLAG("i",       privrec_t, units,     4,  0),

    PSP_P_INT ("len",     privrec_t, length,    10,  1, 1000000),
    PSP_P_INT ("period",  privrec_t, period,    10,  1, 1000000),
    PSP_P_INT ("magn",    privrec_t, magn,      100, 0, 0),
    PSP_P_INT ("zero",    privrec_t, zero,      0,   0, 0),

    PSP_P_INT ("msecs",   privrec_t, msecs,     0,   0, 86400*1000),  // Max=1day
    PSP_P_INT ("shift",   privrec_t, shift,     1,   0, 0),
    
    PSP_P_INT ("nparams", privrec_t, nparams,   0,   0, 100),         // 100=cx_proto.h::CX_MAX_BIGC_PARAMS

    PSP_P_END()
};

static void sim_return_data(int devid, privrec_t *me)
{
  int8           *p8  = me->data;
  int16          *p16 = me->data;
  int32          *p32 = me->data;

  int             x;
  int             phase;
  int             v;
  
    for (x = 0;  x < me->length;  x++)
    {
        phase = (x + me->precession) % me->period;
        v = abs(me->magn * 2 * phase / me->period - me->magn);
        
        if      (me->units == 2) *p16++ = v;
        else if (me->units == 4) *p32++ = v;
        else                     *p8++  = v;
    }
    me->precession += me->shift;

    ReturnBigc(devid, 0,
               me->params, me->nparams,
               me->data, me->length * me->units, me->units, 0);
}

static void sim_heartbeat(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr)
{
  privrec_t      *me = (privrec_t *)devptr;
  
    sim_return_data  (devid, me);
    sl_enq_tout_after(devid, me, me->msecs*1000, sim_heartbeat, NULL);
}

static int sim_init_d(int devid, void *devptr, 
                      int businfocount, int businfo[],
                      const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;

    /* Allocate buffers */
    if ((me->nparams > 0  &&
         (me->params = malloc(me->nparams * sizeof(me->params[0]))) == NULL)
        ||
        ((me->data   = malloc(me->length  * me->units))             == NULL))
    {
        DoDriverLog(devid, 0, "unable to allocate buffers");
        return -CXRF_DRV_PROBL;
    }
    if (me->nparams > 0) bzero(me->params, me->nparams * sizeof(me->params[0]));

    /* Initiate heartbeat, if required */
    if (me->msecs > 0)
        sl_enq_tout_after(devid, me, me->msecs*1000, sim_heartbeat, NULL);
    
    return DEVSTATE_OPERATING;
}

static void sim_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;
  
    safe_free(me->params);
    safe_free(me->data);
}

static void sim_bigc_p(int devid, void *devptr, int chan __attribute__((unused)), int32 *args, int nargs, void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  privrec_t      *me = (privrec_t *)devptr;
  int             n;
  
    for (n = 0; n < nargs;  n++)
        if (n < me->nparams) me->params[n] = args[n];
    
    if (me->msecs <= 0) sim_return_data(devid, me);
}


DEFINE_DRIVER(sim, "Big-channels simulator",
              NULL, NULL,
              sizeof(privrec_t), sim_params,
              0, 0,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              sim_init_d, sim_term_d,
              StdSimulated_rw_p, sim_bigc_p);
