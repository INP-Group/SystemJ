#include "pzframe_drv.h"

#include "drv_i/s2v_drv_i.h"


enum
{
    RM_TRGR  = 0,
    RM_DATA  = 1,
    RM_COUNT = 2
};

typedef struct
{
    pzframe_drv_t         pz;

    int                   use_trigger;
    int                   was_trigger;
    inserver_refmetric_t  rms[RM_COUNT];

    rflags_t              rflags;
    int                   cur_rcvd;
    S2V_DATUM_T           retdata[S2V_MAX_NUMPTS*S2V_NUM_LINES];
} s2v_privrec_t;

#define PDR2ME(pdr) ((s2v_privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

//////////////////////////////////////////////////////////////////////

static int32 ValidateParam(pzframe_drv_t *pdr, int n, int32 v)
{
    if (n == S2V_PARAM_NUMPTS)
    {
        if (v < 1)                v = 1;
        if (v > S2V_MAX_NUMPTS)   v = S2V_MAX_NUMPTS;
    }

    return v;
}

static void  InitParams(pzframe_drv_t *pdr)
{   
  s2v_privrec_t *me = PDR2ME(pdr);

    me->pz.nxt_args[S2V_PARAM_NUMPTS] = 1000;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  s2v_privrec_t *me = PDR2ME(pdr);

    me->pz.retdataunits = sizeof(me->retdata[0]);
    me->pz.retdatasize  = me->pz.cur_args[S2V_PARAM_NUMPTS] * sizeof(me->retdata[0]);

    me->was_trigger = !(me->use_trigger);
    me->rflags      = 0;
    me->cur_rcvd    = 0;

    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  s2v_privrec_t *me = PDR2ME(pdr);

    me->was_trigger = 1;

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  s2v_privrec_t *me = PDR2ME(pdr);

    return PZFRAME_R_READY;
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  s2v_privrec_t *me = PDR2ME(pdr);
  int            unread = me->pz.cur_args[S2V_PARAM_NUMPTS] - me->cur_rcvd;

    if (unread > 0)
        bzero(me->retdata + me->cur_rcvd, sizeof(me->retdata[0]) * unread);

    return 0;
}

//////////////////////////////////////////////////////////////////////

static void trgr_evproc(int                 devid, void *devptr,
                        inserver_chanref_t  ref,
                        int                 reason,
                        void               *privptr)
{
  s2v_privrec_t *me = devptr;

    me->was_trigger = 1;
}

static void data_evproc(int                 devid, void *devptr,
                        inserver_chanref_t  ref,
                        int                 reason,
                        void               *privptr)
{
  s2v_privrec_t *me = devptr;
  int32          val;
  rflags_t       rflags;

    if (!me->was_trigger  ||  !me->pz.measuring_now) return;

    inserver_get_cval(devid, me->rms[RM_DATA].chan_ref,
                      &val, &rflags, NULL);
    me->rflags |= rflags;
    me->retdata[me->cur_rcvd++] = val;

    if (me->cur_rcvd >= me->pz.cur_args[S2V_PARAM_NUMPTS])
        pzframe_drv_drdy_p(&(me->pz), 1, me->rflags);
}

//////////////////////////////////////////////////////////////////////

static int s2v_init_d(int devid, void *devptr, 
                      int businfocount, int businfo[],
                      const char *auxinfo)
{
  s2v_privrec_t   *me = (s2v_privrec_t *) devptr;
  const char      *p  = auxinfo;

    /* Parse auxinfo */
    if (auxinfo == NULL)
    {
        DoDriverLog(devid, 0, "%s(): empty auxinfo", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    if (*p == '-')
    {
        p = strchr(p, '/');
        if (p == NULL)
        {
            DoDriverLog(devid, 0, "%s(): '/' expected after '-'", __FUNCTION__);
            return -CXRF_CFG_PROBL;
        }
    }
    else
    {
        if (inserver_parse_d_c_ref(devid, "trigger", &p, me->rms+RM_TRGR) < 0)
            return -CXRF_CFG_PROBL;
        me->use_trigger = 1;
    }
    if (*p != '/')
    {
        DoDriverLog(devid, 0, "%s(): '/' expected after trigger spec", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    p++;
    if (inserver_parse_d_c_ref(devid, "source", &p, me->rms+RM_DATA) < 0)
        return -CXRF_CFG_PROBL;

    /* Set watch */
    me->rms[0      ].m_count = RM_COUNT;
    me->rms[RM_TRGR].data_cb = trgr_evproc;
    me->rms[RM_DATA].data_cb = data_evproc;
    inserver_new_watch_list(devid, me->rms, 2*1000000);

    /* Init pzframe */
    pzframe_drv_init(&(me->pz), devid,
                     S2V_NUM_PARAMS,
                     S2V_PARAM_SHOT, S2V_PARAM_ISTART,
                     S2V_PARAM_WAITTIME, S2V_PARAM_STOP,
                     S2V_PARAM_ELAPSED,
                     ValidateParam,
                     StartMeasurements, TrggrMeasurements,
                     AbortMeasurements, ReadMeasurements);
    pzframe_set_buf_p(&(me->pz), me->retdata);

    InitParams(&(me->pz));

    return DEVSTATE_OPERATING;
}

static void s2v_term_d(int devid, void *devptr)
{
  s2v_privrec_t *me = (s2v_privrec_t *) devptr;

    pzframe_drv_term(&(me->pz));
}

static void s2v_rw_p  (int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  s2v_privrec_t *me = (s2v_privrec_t *)devptr;

    pzframe_drv_rw_p(&(me->pz), firstchan, count, values, action);
}

static void s2v_bigc_p(int devid, void *devptr,
                       int chan __attribute__((unused)),
                       int32 *args, int nargs,
                       void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  s2v_privrec_t *me = (s2v_privrec_t *)devptr;

    pzframe_drv_bigc_p(&(me->pz), args, nargs);
}


static CxsdMainInfoRec s2v_main_info[] =
{
    {60, 1},
    {40, 0},
};

static CxsdBigcInfoRec s2v_bigc_info[] =
{
    {1, 
        S2V_NUM_PARAMS, 0,                                          0,
        S2V_NUM_PARAMS, S2V_MAX_NUMPTS*S2V_NUM_LINES*S2V_DATAUNITS, S2V_DATAUNITS},
};

DEFINE_DRIVER(s2v,  "Scalar-to-vector data converter (fastadc-compatible)",
              NULL, NULL,
              sizeof(s2v_privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(s2v_main_info), s2v_main_info,
              countof(s2v_bigc_info), s2v_bigc_info,
              s2v_init_d, s2v_term_d,
              s2v_rw_p, s2v_bigc_p);
