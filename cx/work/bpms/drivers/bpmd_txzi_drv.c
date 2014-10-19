#include "cxsd_driver.h"
#include "pzframe_drv.h"

#include <limits.h>
#include <math.h>

#include "drv_i/bpmd_drv_i.h"
#include "drv_i/bpmd_txzi_drv_i.h"

#include "coord_calc.h"
#include "moving_statistics.h"

enum
{
    BIND_HEARTBEAT_PERIOD = 2 * 1000 * 1000  // 2s
};

enum { T_VAL, X_VAL, Z_VAL, I_VAL };

typedef struct
{
    // regular part as for pzframe_drv example
    pzframe_drv_t       pz;
    int                 devid;

    BPMD_TXZI_DATUM_T   retdata[BPMD_TXZI_MAX_NUMPTS * BPMD_TXZI_NUM_LINES];

    // auxilary part
    inserver_refmetric_t rfm;
    int                 bpm_id;
    int                 clb_id;
    float               iscale;
    float               rotang;

    tag_t               tag;
    rflags_t            rflags;
    int32               meas_params[CX_MAX_BIGC_PARAMS];
    BPMD_DATUM_T        meas_data[BPMD_MAX_NUMPTS * BPMD_NUM_LINES];

    coord_calc_dscr_t   s2c;
    int                 s2c_type;
    scale_t             scale;

    float               t_data[BPMD_MAX_NUMPTS];
    float               x_data[BPMD_MAX_NUMPTS];
    float               z_data[BPMD_MAX_NUMPTS];
    float               i_data[BPMD_MAX_NUMPTS];

    float               v_min   [BPMD_NUM_LINES];
    float               v_max   [BPMD_NUM_LINES];
    float               v_mean  [BPMD_NUM_LINES];
    float               v_stddev[BPMD_NUM_LINES];
} privrec_t;

static psp_paramdescr_t bpmd_txzi_params[] =
{
    PSP_P_PLUGIN("src", privrec_t, rfm, inserver_d_c_ref_plugin_parser, NULL),

    PSP_P_FLAG  ("bep", privrec_t, s2c_type, TYPE_BEP, -1),
    PSP_P_FLAG  ("v2k", privrec_t, s2c_type, TYPE_V2K, -1),
    PSP_P_FLAG  ("v5r", privrec_t, s2c_type, TYPE_V5R, -1),

    PSP_P_INT   ("bpm_id", privrec_t, bpm_id, -1, 0, 100),
    PSP_P_INT   ("clb_id", privrec_t, clb_id, -1, 0, 100),

    PSP_P_REAL  ("iscale", privrec_t, iscale, 1e-3, 0,   1),
    PSP_P_REAL  ("rotang", privrec_t, rotang, 0,    0, 360),

    PSP_P_PLUGIN("scale",  privrec_t, scale, scale_plugin_parser, "bpmd_txzi:"),

    PSP_P_END()
};

#define PDR2ME(pdr) ((privrec_t     *)pdr) //!!! Should better sufbtract offsetof(pz)
#define ME2PDR(pdr) ((pzframe_drv_t *)me )

//////////////////////////////////////////////////////////////////////

static int32 ValidateParam(pzframe_drv_t *pdr, int n, int v)
{
  privrec_t *me = PDR2ME(pdr);
    /* just to make gcc happy */
    me = me;
    n  = n;
    return v;
}

static void  InitParams(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

    me->pz.retdataunits = BPMD_TXZI_DATAUNITS;

    me->pz.cur_args[BPMD_TXZI_CHAN_SIG2INT] =
    me->pz.nxt_args[BPMD_TXZI_CHAN_SIG2INT] = me->iscale * 1.e+6;

    return;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
#if 1
    me = me; /* just to make gcc happy */
#else
    static int firsttime = 0;
    int cachectl = (firsttime ? CX_CACHECTL_SNIFF : CX_CACHECTL_FROMCACHE);
    inserver_req_bigc(me->devid, me->trgr[0].chanref, NULL, 0, NULL, 0, 0, cachectl);
    firsttime = 1;
#endif
    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
    me = me; /* just to make gcc happy */
    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
    me = me; /* just to make gcc happy */
    return PZFRAME_R_READY;
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

  int    it    = 0;
  int    count = 0;
  float  scale = 0;
  size_t bytes = 0;

    /* All data already obtained before this function is called */

    /* Stages 1-3 are done in rfm_bigc_evproc()                 */

    me->pz.retdataunits = BPMD_TXZI_DATAUNITS;
    me->pz.retdatasize  = BPMD_TXZI_DATAUNITS * me->meas_params[BPMD_PARAM_NUMPTS] *
                          BPMD_TXZI_NUM_LINES;

    /* 4. Calculation */
    if (1)
    {
        float *t_p, *x_p, *z_p, *i_p, didsg;

        count = me->meas_params[BPMD_PARAM_NUMPTS];

	didsg = me->scale.i / me->scale.s
	        *
	        me->s2c.a(me->scale.a) / me->s2c.a(me->meas_params[BPMD_PARAM_ATTEN]);

        /* 1. calculate t,x,z,i from s1,s2,s3,s4 */
        for (
             it = 0,       t_p = me->t_data, x_p = me->x_data, z_p = me->z_data, i_p = me->i_data;
             it < count;
             ++it,       ++t_p,            ++x_p,            ++z_p,            ++i_p
            )
        {
            float s1, s2, s3, s4;

            s1 = 1.0 * (*(me->meas_data + 0 * count + it));
            s2 = 1.0 * (*(me->meas_data + 1 * count + it));
            s3 = 1.0 * (*(me->meas_data + 2 * count + it));
            s4 = 1.0 * (*(me->meas_data + 3 * count + it));

            *x_p = me->s2c.x(me->clb_id, s1, s2, s3, s4);
            *z_p = me->s2c.z(me->clb_id, s1, s2, s3, s4);
            *i_p = me->s2c.i(me->clb_id, s1, s2, s3, s4);

            *t_p =     1 + it * me->meas_params[BPMD_PARAM_DECMTN];
	    *i_p = fabs(*i_p) * didsg;

	    // fprintf(stderr, "t = %f, x = %f, z = %f, i = %f\n", *t_p, *x_p, *z_p, *i_p);
        }

        /* 2. calculate some statistics */
        for (it = 0; it < BPMD_NUM_LINES; ++it)
        {
            switch(it)
            {
                case T_VAL:
                    me->v_min   [it] = min_value   (me->t_data, count);
                    me->v_max   [it] = max_value   (me->t_data, count);
                    me->v_mean  [it] = mean_value  (me->t_data, count);
                    me->v_stddev[it] = stddev_value(me->t_data, count);
                    break;
                case X_VAL:
                    me->v_min   [it] = min_value   (me->x_data, count);
                    me->v_max   [it] = max_value   (me->x_data, count);
                    me->v_mean  [it] = mean_value  (me->x_data, count);
                    me->v_stddev[it] = stddev_value(me->x_data, count);
                    break;
                case Z_VAL:
                    me->v_min   [it] = min_value   (me->z_data, count);
                    me->v_max   [it] = max_value   (me->z_data, count);
                    me->v_mean  [it] = mean_value  (me->z_data, count);
                    me->v_stddev[it] = stddev_value(me->z_data, count);
                    break;
                case I_VAL:
                    me->v_min   [it] = min_value   (me->i_data, count);
                    me->v_max   [it] = max_value   (me->i_data, count);
                    me->v_mean  [it] = mean_value  (me->i_data, count);
                    me->v_stddev[it] = stddev_value(me->i_data, count);
                    break;
            }
        }

        /* 3. return data */
        me->pz.cur_args[BPMD_TXZI_CHAN_SHOT    ] = me->meas_params[BPMD_PARAM_SHOT    ];
        me->pz.cur_args[BPMD_TXZI_CHAN_STOP    ] = me->meas_params[BPMD_PARAM_STOP    ];
        me->pz.cur_args[BPMD_TXZI_CHAN_ISTART  ] = me->meas_params[BPMD_PARAM_ISTART  ];
        me->pz.cur_args[BPMD_TXZI_CHAN_DELAY   ] = me->meas_params[BPMD_PARAM_DELAY   ];
        me->pz.cur_args[BPMD_TXZI_CHAN_ATTEN   ] = me->meas_params[BPMD_PARAM_ATTEN   ];
        me->pz.cur_args[BPMD_TXZI_CHAN_NUMPTS  ] = me->meas_params[BPMD_PARAM_NUMPTS  ];
        me->pz.cur_args[BPMD_TXZI_CHAN_PTSOFS  ] = me->meas_params[BPMD_PARAM_PTSOFS  ];
        me->pz.cur_args[BPMD_TXZI_CHAN_DECMTN  ] = me->meas_params[BPMD_PARAM_DECMTN  ];
        me->pz.cur_args[BPMD_TXZI_CHAN_WAITTIME] = me->meas_params[BPMD_PARAM_WAITTIME];

	scale = 1.e+5;
        for (it = 0; it < BPMD_NUM_LINES; ++it)
        {
            switch(it)
            {
                case T_VAL:
                    me->pz.cur_args[BPMD_TXZI_CHAN_MIN0 + it] = scale * me->v_min   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_MAX0 + it] = scale * me->v_max   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_AVG0 + it] = scale * me->v_mean  [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_INT0 + it] = 0;
                    break;
                case X_VAL:
                    me->pz.cur_args[BPMD_TXZI_CHAN_MIN0 + it] = scale * me->v_min   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_MAX0 + it] = scale * me->v_max   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_AVG0 + it] = scale * me->v_mean  [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_INT0 + it] = 0;
                    break;
                case Z_VAL:
                    me->pz.cur_args[BPMD_TXZI_CHAN_MIN0 + it] = scale * me->v_min   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_MAX0 + it] = scale * me->v_max   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_AVG0 + it] = scale * me->v_mean  [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_INT0 + it] = 0;
                    break;
                case I_VAL:
                    me->pz.cur_args[BPMD_TXZI_CHAN_MIN0 + it] = scale * me->v_min   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_MAX0 + it] = scale * me->v_max   [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_AVG0 + it] = scale * me->v_mean  [it];
                    me->pz.cur_args[BPMD_TXZI_CHAN_INT0 + it] = 0;
                    break;
            }
        }

        me->pz.cur_args[BPMD_TXZI_CHAN_BPMID   ] =         me->bpm_id;
        me->pz.cur_args[BPMD_TXZI_CHAN_XMEAN   ] = scale * me->v_mean  [X_VAL];
        me->pz.cur_args[BPMD_TXZI_CHAN_XSTDDEV ] = scale * me->v_stddev[X_VAL];
        me->pz.cur_args[BPMD_TXZI_CHAN_ZMEAN   ] = scale * me->v_mean  [Z_VAL];
        me->pz.cur_args[BPMD_TXZI_CHAN_ZSTDDEV ] = scale * me->v_stddev[Z_VAL];
        me->pz.cur_args[BPMD_TXZI_CHAN_IMEAN   ] = scale * me->v_mean  [I_VAL];
        me->pz.cur_args[BPMD_TXZI_CHAN_ISTDDEV ] = scale * me->v_stddev[I_VAL];

        bytes = count * sizeof(float);
        memcpy(me->retdata + 0 * count,  me->t_data,  bytes);
        memcpy(me->retdata + 1 * count,  me->x_data,  bytes);
        memcpy(me->retdata + 2 * count,  me->z_data,  bytes);
        memcpy(me->retdata + 3 * count,  me->i_data,  bytes);
    }
#if 0
    fprintf(stderr, "%s: pic% d: x_mean = % f, z_mean = % f, i_mean = % f\n",
            __FUNCTION__, me->bpm_id, me->v_mean[X_VAL], me->v_mean[Z_VAL], me->v_mean[I_VAL]);
#endif
    return 0;
}

//////////////////////////////////////////////////////////////////////

static void rfm_bigc_evproc(int                 devid, void *devptr,
                            inserver_chanref_t  ref,
                            int                 reason __attribute__((unused)),
                            void               *privptr)
{
  privrec_t      *me  = (privrec_t *)devptr;

  pzframe_drv_t  *pdr       = ME2PDR(me);
  int32           do_return = 1;

  int             r = 0;
  size_t          retdataunits;

  inserver_refmetric_t *m_p = privptr;

    /* NB!!!!!: 'reason' value is 0 for a moment */

    if ( m_p != NULL)
	DoDriverLog(devid, 0 * DRIVERLOG_DEBUG | DRIVERLOG_C_ENTRYPOINT,
		    "%s(%s.%d->%d.%d)", __FUNCTION__,
		    m_p->targ_name, m_p->chan_n, m_p->targ_ref, m_p->chan_ref);
    else return;

    /* Get data */
    /* 1. Read ststistics */
    r = inserver_get_bigc_stats (devid, ref, &me->tag, &me->rflags);

    /* 2. Read parameters */
    r = inserver_get_bigc_params(devid, ref, 0, BPMD_NUM_PARAMS, me->meas_params);

    /* 3. Read data */
    r = inserver_get_bigc_data  (devid, ref, 0,
				 BPMD_MAX_NUMPTS * BPMD_NUM_LINES * BPMD_DATAUNITS,
				 me->meas_data, &retdataunits);
    r = r; /* just to make gcc happy */

    //fprintf(stderr, "%s) %d\n", __FUNCTION__, me->meas_params[BPMD_PARAM_ATTEN]);

    /*!!! Any checks?  Result, retdataunits? */

    /* this is a HACK!!! */
    pdr->measuring_now = 1;
    pdr->tid = -1;

    pzframe_drv_drdy_p(pdr, do_return, me->rflags);
}

//////////////////////////////////////////////////////////////////////

enum
{
    PARAM_SHOT     = BPMD_TXZI_CHAN_SHOT,
    PARAM_ISTART   = BPMD_TXZI_CHAN_ISTART,
    PARAM_WAITTIME = BPMD_TXZI_CHAN_WAITTIME,
    PARAM_STOP     = BPMD_TXZI_CHAN_STOP,
    PARAM_ELAPSED  = BPMD_TXZI_CHAN_ELAPSED,

    NUM_PARAMS     = BPMD_TXZI_CHAN_COUNT,
};

static int  init_b(int devid, void *devptr, 
                   int businfocount __attribute__((unused)), int businfo[] __attribute__((unused)),
                   const char *auxinfo __attribute__((unused)))
{
  privrec_t   *me = (privrec_t *)devptr;

    me->devid  = devid;

    /* I. Parse auxinfo */
    /* 0. Is there anything at all? */
#if 0
    if (auxinfo == NULL || auxinfo[0] == '\0')
    {
        DoDriverLog(devid, DRIVERLOG_CRIT, "%s: auxinfo is empty...", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    p = auxinfo;
#endif

    DoDriverLog(devid, 0 * DRIVERLOG_INFO,
		"targ_name = %s, bpm_id = %d, clb_id = %d, iscale = %f, s2c_type = %d",
		me->rfm.targ_name, me->bpm_id, me->clb_id, me->iscale, me->s2c_type);

    if ( get_s2c(me->s2c_type, &me->s2c) < 0 )
    {
        DoDriverLog(devid, DRIVERLOG_CRIT,
                    "type specification (bep/v2k/v5r) is mandatory");
        return -CXRF_CFG_PROBL;
    }

    /* II. PzFrame library initialization */
    pzframe_drv_init(&(me->pz), devid,
                     NUM_PARAMS,
                     PARAM_SHOT, PARAM_ISTART, PARAM_WAITTIME, PARAM_STOP, PARAM_ELAPSED,
                     ValidateParam,
                     StartMeasurements, TrggrMeasurements,
                     AbortMeasurements, ReadMeasurements);
    pzframe_set_buf_p(&(me->pz), me->retdata);

    InitParams(&(me->pz));

    /* III. Inserver Bind mechanism initialization */
    /* Register evproc */
    me->rfm.chan_is_bigc = 1;
    me->rfm.data_cb      = rfm_bigc_evproc;
    me->rfm.privptr      = &me->rfm;

    inserver_new_watch_list(devid, &(me->rfm), BIND_HEARTBEAT_PERIOD);

    return DEVSTATE_OPERATING;
}

static void term_b(int devid __attribute__((unused)), void *devptr)
{
  privrec_t   *me = (privrec_t *)devptr;
    pzframe_drv_term(&(me->pz));
}

static void rdwr_p(int devid __attribute__((unused)), void *devptr,
                   int firstchan, int count, int32 *values, int action)
{
  privrec_t   *me = (privrec_t *)devptr;
    return;
    pzframe_drv_rw_p(&(me->pz), firstchan, count, values, action);
}

static void bigc_p(int devid __attribute__((unused)), void *devptr,
                   int chan __attribute__((unused)),
                   int32 *args, int nargs,
                   void *data __attribute__((unused)), size_t datasize __attribute__((unused)),
                   size_t dataunits __attribute__((unused)))
{
  privrec_t   *me = (privrec_t *)devptr;
    return;
    pzframe_drv_bigc_p(&(me->pz), args, nargs);
}

/* 
    Note Bene!!!
    
    DEFINE_DRIVER(
        name, comment,                     \
        init_mod, term_mod,                \
        privrecsize, paramtable,           \
        min_businfo_n, max_businfo_n,      \
        layer, layerver,                   \
        main_nsegs, main_info,             \
        bigc_nsegs, bigc_info,             \
        init_dev, term_dev, rw_p, bigc_p   \
    )
*/

DEFINE_DRIVER(bpmd_txzi, "PC driver for calculation of beam X,Z coord and Intensity",
              NULL, NULL,
              sizeof(privrec_t), bpmd_txzi_params,
              0, 20, /*!!! CXSD_DB_MAX_BUSINFOCOUNT */
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_b, term_b, rdwr_p, bigc_p);
