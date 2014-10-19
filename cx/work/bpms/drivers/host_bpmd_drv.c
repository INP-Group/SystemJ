#include "cxsd_driver.h"
#include "pzframe_drv.h"

#include <limits.h>
#include <math.h>

#include "drv_i/bpmd_drv_i.h"


#include "coord_calc.h"


enum
{
    SIM_EXTERNAL_PERIOD = 2 * 1000 * 1000, // 2s
    SIM_INTERNAL_PERIOD =       20 * 1000, // 10ms

    MAX_PTS    = 32768,

    ZERO_SHIFT = 0,
};

typedef struct
{
    // regular part as for pzframe_drv example
    pzframe_drv_t   pz;
    int             devid;
    BPMD_DATUM_T    retdata[BPMD_MAX_NUMPTS * BPMD_NUM_LINES];

    // auxilary part
    int             irq_tid;

    BPMD_DATUM_T    sgnls[BPMD_NUM_LINES][MAX_PTS];

    int             iz1;   // individual zerro shift for line #1
    int             iz2;   // individual zerro shift for line #2
    int             iz3;   // individual zerro shift for line #3
    int             iz4;   // individual zerro shift for line #4
    int             id1;   // individual delay       for line #1
    int             id2;   // individual delay       for line #2
    int             id3;   // individual delay       for line #3
    int             id4;   // individual delay       for line #4
    int             mgd;   // magic delay

    coord_calc_dscr_t   s2c;
    int                 s2c_type;
    scale_t             scale;
} privrec_t;

static psp_paramdescr_t bpmd_params[] =
{
    PSP_P_INT ("iz1", privrec_t, iz1, 0, -8192, 8191 ),
    PSP_P_INT ("iz2", privrec_t, iz2, 0, -8192, 8191 ),
    PSP_P_INT ("iz3", privrec_t, iz3, 0, -8192, 8191 ),
    PSP_P_INT ("iz4", privrec_t, iz4, 0, -8192, 8191 ),

    PSP_P_INT ("id1", privrec_t, id1, 0,  0,    1023 ),
    PSP_P_INT ("id2", privrec_t, id2, 0,  0,    1023 ),
    PSP_P_INT ("id3", privrec_t, id3, 0,  0,    1023 ),
    PSP_P_INT ("id4", privrec_t, id4, 0,  0,    1023 ),

    PSP_P_INT ("mgd", privrec_t, mgd, 0,  0,    7    ),

    PSP_P_FLAG  ("bep", privrec_t, s2c_type, TYPE_BEP, -1),
    PSP_P_FLAG  ("v2k", privrec_t, s2c_type, TYPE_V2K, -1),
    PSP_P_FLAG  ("v5r", privrec_t, s2c_type, TYPE_V5R, -1),

    PSP_P_PLUGIN("scale", privrec_t, scale, scale_plugin_parser, "host_txzi:"),

    PSP_P_END()
};

#define PDR2ME(pdr) ((privrec_t     *)pdr) //!!! Should better sufbtract offsetof(pz)
#define ME2PDR(pdr) ((pzframe_drv_t *)me )

//////////////////////////////////////////////////////////////////////

static rflags_t  zero_rflag = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

static int  clculateButtonsSignal(float x, float z, float Ib)
{
    return 0;
}

static void generateBeamSignal(privrec_t *me, int kiked_beam)
{
  pzframe_drv_t *pdr = ME2PDR(me);    

    // ==========
    float ksi = 3.3;               // linear part of chromaticity
    float mu  = 2.5 * 1.e-6;       // dnu/da2 - cubic nonlinearity
    float Z   = 15;                // kick amplitude
    float As  = 0;                 // formfactor due to chromaticity
    float A   = 0;                 // formfactor due to nonlinearity

    float x0     = +1.8;           // mm - beam displacement;
    float sigmaX = 3.0 * 1.e-1;    // mm - beam size
    float nuX    = 0.092;

    float z0     = -1.2;           // mm - beam displacement;
    float sigmaZ = 1.0 * 1.e-1;    // mm - beam size
    float nuZ    = 0.123;

    float sigmaE = 5 * 1e-4; // rms energy spread
    float nuS    = 0.003;    // normalised synchrotron frequency

    float Qp = 4 * M_PI * mu;
    float Qg = Z * Qp;

    float frac, alpha;

    int   N = 0, N0 = 80, it;

    float Ib = 3.0; // mA
    float ax = 14;  // mm
    float bz = 14;  // mm

    float eps, x, z;
    float h, v, s1, s2, s3, s4, s;
    float se, didsg, a;
    float bcx, bcz;

    kiked_beam = kiked_beam > 0;

    a = me->s2c.a(me->scale.a) / me->s2c.a(pdr->cur_args[BPMD_PARAM_ATTEN]);

    didsg = me->scale.i / me->scale.s
            *
            me->s2c.a(me->scale.a) / me->s2c.a(pdr->cur_args[BPMD_PARAM_ATTEN]);

    se = Ib / didsg;

    for (it = 0; it < MAX_PTS; ++it)
    {
        N = it - N0;

        alpha = 2 * sigmaE * ksi / nuS * sin( M_PI * nuS * N );
        As    = expf( -1.0 * powf( alpha, 2 ) /  2 );

        frac  = 1 / ( 1 + powf( Qp * N, 2 ));
        A     = frac * expf( -1.0 / 2 * powf( Qg * N , 2 ) * frac );

        eps = +1.5 * sigmaX * ((drand48() - 0.5) * 2) * 0.2;
        x = x0 + eps + ( N < 0 ? 0 : ( sigmaX * Z * As * A * cos( 2 * M_PI * nuX * it ) ) ) * kiked_beam;

        eps = -0.5 * sigmaZ * ((drand48() - 0.5) * 2) * 0.2;
        z = z0 + eps + ( N < 0 ? 0 : ( sigmaZ * Z * As * A * cos( 2 * M_PI * nuZ * it ) ) ) * kiked_beam;

	h = (x - x0 * 0) / ax;
	v = (z - z0 * 0) / bz;
#if 0
        /* Rotated pick-up */
	bcx = ax * cos(0 * M_PI / 4);
        bcz = bz * sin(0 * M_PI / 4);

	s1 = se / 4
	     *
	     sqrtf( powf(bcx, 2) + powf(bcz, 2) ) / sqrtf( powf(bcx - x, 2) + powf(bcz - z, 2) );

	s2 = (1+v) / (1+h) * (se * (1+h) - 2 * s1) / 2
        s3 = s1 * (1-h) / (1+h)
	s4 = s2 * (1-v) / (1+v);
#else
        /* Normal pick-up */
	bcx = ax * cos(1 * M_PI / 4);
        bcz = bz * sin(1 * M_PI / 4);

	s1 = se / 4
	     *
	     sqrtf( powf(bcx, 2) + powf(bcz, 2) ) / sqrtf( powf(bcx - x, 2) + powf(bcz - z, 2) );

	s4 = -s1 + se * (1 + h) / 2;
        s2 =  s4 + se * (v - h) / 2;
        s3 =  s1 - se * (h + v) / 2;
#endif
	s  = s1 + s2 + s3 + s4;

	me->sgnls[1-1][it] = (int)(rintf(1e+3 * s1) * 1e-3) + pdr->cur_args[BPMD_PARAM_ZERO0] * 0;
	me->sgnls[2-1][it] = (int)(rintf(1e+3 * s2) * 1e-3) + pdr->cur_args[BPMD_PARAM_ZERO1] * 0;
	me->sgnls[3-1][it] = (int)(rintf(1e+3 * s3) * 1e-3) + pdr->cur_args[BPMD_PARAM_ZERO2] * 0;
	me->sgnls[4-1][it] = (int)(rintf(1e+3 * s4) * 1e-3) + pdr->cur_args[BPMD_PARAM_ZERO3] * 0;
#if 0
	if ( it < 50 )
	    fprintf(stderr, "%d) %d %d %d %d, %f %f, %f %f | %f %f %f %f\n",
		    it,
		    me->sgnls[1-1][it], me->sgnls[2-1][it], me->sgnls[3-1][it], me->sgnls[4-1][it],
                    x, z, h, v,
                    a, didsg, se, s
		   );
#endif
    }
    // ==========
}

static void irq_heartbeat(int devid __attribute__((unused)), void *devptr,
                            int tid __attribute__((unused)), void *privptr __attribute__((unused)))
{
  privrec_t *me = (privrec_t *) devptr;

    int do_return = 1;
    me->irq_tid = -1;
    pzframe_drv_drdy_p(&(me->pz), do_return, zero_rflag * unsp_rflags);
}

//////////////////////////////////////////////////////////////////////

static inline rflags_t bpmd_code2uv(BPMD_DATUM_T code, int32 zero_shift, BPMD_DATUM_T *r_p)
{
    *r_p = code - ZERO_SHIFT - zero_shift;
    return 0;
}

static int32 ValidateParam(pzframe_drv_t *pdr __attribute__((unused)), int n, int v)
{
    if      (n == BPMD_PARAM_PTSOFS)
    {
        if      (v < 0)                 return 0;
        else if (v > BPMD_MAX_NUMPTS-1) return BPMD_MAX_NUMPTS-1;
        else                            return v;
    }
    else if (n == BPMD_PARAM_NUMPTS)
    {
        if      (v < 1)                 return 1;
        else if (v > BPMD_MAX_NUMPTS)   return BPMD_MAX_NUMPTS;
        else                            return v;
    }
    else if (n == BPMD_PARAM_DELAY)
    {
	if      (v < 0)                 return 0;
	else if (v > 100*1000)          return 100*1000;
        else                            return v;
    }
    else if (n == BPMD_PARAM_ATTEN)
    {
        if      (v < 0)                 return 0;
        else if (v > 15)                return 15;
        else                            return v;
    }
    else if (n == BPMD_PARAM_DECMTN)
    {
        if      (v < 1   )              return 1;
        else if (v > 2048)              return 2048;
        else                            return v;
    }
    else if (n >= BPMD_PARAM_ZERO0  &&  n <= BPMD_PARAM_ZERO3)
    {
        if (v < -8192)                  return -8192;
        if (v > +8191)                  return +8191;
        else                            return v;
    }
    else return v;
}

static void  InitParams(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

    pdr->retdataunits = BPMD_DATAUNITS;

    pdr->nxt_args[BPMD_PARAM_NUMPTS] = 1024;
#if 1
    pdr->nxt_args[BPMD_PARAM_ZERO0]  = me->iz1;
    pdr->nxt_args[BPMD_PARAM_ZERO1]  = me->iz2;
    pdr->nxt_args[BPMD_PARAM_ZERO2]  = me->iz3;
    pdr->nxt_args[BPMD_PARAM_ZERO3]  = me->iz4;
#endif
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

    /* Additional check of PTSOFS against NUMPTS */
    if (me->pz.cur_args[BPMD_PARAM_PTSOFS] > BPMD_MAX_NUMPTS - me->pz.cur_args[BPMD_PARAM_NUMPTS])
        me->pz.cur_args[BPMD_PARAM_PTSOFS] = BPMD_MAX_NUMPTS - me->pz.cur_args[BPMD_PARAM_NUMPTS];

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[BPMD_PARAM_NUMPTS] *
                                         BPMD_NUM_LINES     *
                                         BPMD_DATAUNITS;

    if (me->irq_tid >= 0)
    {
        sl_deq_tout(me->irq_tid);
        me->irq_tid = -1;
    }

    /* Start */
    generateBeamSignal(me, 1);

    me->irq_tid = sl_enq_tout_after(me->devid, me, SIM_EXTERNAL_PERIOD, irq_heartbeat, 0);
    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

    if (me->irq_tid >= 0)
    {
        sl_deq_tout(me->irq_tid);
        me->irq_tid = -1;
    }

    generateBeamSignal(me, 0);

    me->irq_tid = sl_enq_tout_after(me->devid, me, SIM_INTERNAL_PERIOD, irq_heartbeat, 0);
    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

    if (me->irq_tid >= 0)
    {
        sl_deq_tout(me->irq_tid);
        me->irq_tid = -1;
    }

    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);
    return PZFRAME_R_READY;
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

  BPMD_DATUM_T  *dp;
  int            nc;
  int32          zero_shift;
  int            n;
  BPMD_DATUM_T   v;
  BPMD_DATUM_T   vd;
  int32          min_v, max_v, sum_v;

#if 0
    for ( nc = 0; it < BPMD_NUM_LINES; ++nc )
    {
        memcpy(                 me->retdata + nc  * me->pz.cur_args[BPMD_PARAM_NUMPTS],
               (BPMD_DATUM_T *)(me->sgnls   + nc) + me->pz.cur_args[BPMD_PARAM_PTSOFS], me->pz.retdatasize);
    }
#endif

    dp = me->retdata;

    for (nc = 0;  nc < BPMD_NUM_LINES;  nc++)
    {
        zero_shift = pdr->cur_args[BPMD_PARAM_ZERO0 + nc];

        if (pdr->cur_args[BPMD_PARAM_CALC_STATS] == 0)
        {
            for (n = pdr->cur_args[BPMD_PARAM_NUMPTS];  n > 0;  n -= 1)
            {
                v = me->sgnls[nc][ pdr->cur_args[BPMD_PARAM_NUMPTS] - (n - pdr->cur_args[BPMD_PARAM_PTSOFS]) ];
                bpmd_code2uv(v, zero_shift, dp);
                dp++;
            }

            pdr->cur_args[BPMD_PARAM_MIN0 + nc] =
            pdr->nxt_args[BPMD_PARAM_MIN0 + nc] =
            pdr->cur_args[BPMD_PARAM_MAX0 + nc] =
            pdr->nxt_args[BPMD_PARAM_MAX0 + nc] =
            pdr->cur_args[BPMD_PARAM_AVG0 + nc] =
            pdr->nxt_args[BPMD_PARAM_AVG0 + nc] =
            pdr->cur_args[BPMD_PARAM_INT0 + nc] =
            pdr->nxt_args[BPMD_PARAM_INT0 + nc] = 0;
        }
        else
        {
            min_v = INT_MAX; max_v = INT_MIN; sum_v = 0;
            for (n = pdr->cur_args[BPMD_PARAM_NUMPTS];  n > 0;  n -= 1)
            {
                v = me->sgnls[nc][ pdr->cur_args[BPMD_PARAM_NUMPTS] - (n - pdr->cur_args[BPMD_PARAM_PTSOFS]) ];
                bpmd_code2uv(v, zero_shift, dp);
                dp++;

                if (min_v > v) min_v = v;
                if (max_v < v) max_v = v;
                sum_v += v;
            }

            bpmd_code2uv(min_v, zero_shift, &vd);
            pdr->cur_args[BPMD_PARAM_MIN0 + nc] =
            pdr->nxt_args[BPMD_PARAM_MIN0 + nc] = vd;

            bpmd_code2uv(max_v, zero_shift, &vd);
            pdr->cur_args[BPMD_PARAM_MAX0 + nc] =
            pdr->nxt_args[BPMD_PARAM_MAX0 + nc] = vd;

            bpmd_code2uv(sum_v / pdr->cur_args[BPMD_PARAM_NUMPTS], zero_shift, &vd);
            pdr->cur_args[BPMD_PARAM_AVG0 + nc] =
            pdr->nxt_args[BPMD_PARAM_AVG0 + nc] = vd;

            vd = sum_v - (ZERO_SHIFT + zero_shift) * pdr->cur_args[BPMD_PARAM_NUMPTS];
            pdr->cur_args[BPMD_PARAM_INT0 + nc] =
            pdr->nxt_args[BPMD_PARAM_INT0 + nc] = vd;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

enum
{
    PARAM_SHOT     = BPMD_PARAM_SHOT,
    PARAM_STOP     = BPMD_PARAM_STOP,
    PARAM_ISTART   = BPMD_PARAM_ISTART,
    PARAM_WAITTIME = BPMD_PARAM_WAITTIME,
    PARAM_ELAPSED  = BPMD_PARAM_ELAPSED,
    
    NUM_PARAMS     = BPMD_NUM_PARAMS,
};

static int  init_b(int devid, void *devptr, 
                   int businfocount __attribute__((unused)), int businfo[] __attribute__((unused)),
                   const char *auxinfo __attribute__((unused)))
{
  privrec_t   *me = (privrec_t *)devptr;

    // fprintf(stderr, "%s:%d:%s() I'm here!\n", __FILE__, __LINE__, __FUNCTION__);
    me->devid  = devid;

    if ( get_s2c(me->s2c_type, &me->s2c) < 0 )
    {
        DoDriverLog(devid, DRIVERLOG_CRIT,
                    "type specification (bep/v2k/v5r) is mandatory");
        return -CXRF_CFG_PROBL;
    }

    pzframe_drv_init(&(me->pz), devid,
                     NUM_PARAMS,
                     PARAM_SHOT, PARAM_ISTART, PARAM_WAITTIME, PARAM_STOP, PARAM_ELAPSED,
                     ValidateParam,
                     StartMeasurements, TrggrMeasurements,
                     AbortMeasurements, ReadMeasurements);
    pzframe_set_buf_p(&(me->pz), me->retdata);

    InitParams(&(me->pz));

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
    pzframe_drv_rw_p(&(me->pz), firstchan, count, values, action);
}

static void bigc_p(int devid __attribute__((unused)), void *devptr,
                   int chan __attribute__((unused)),
                   int32 *args, int nargs,
                   void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  privrec_t   *me = (privrec_t *)devptr;
    pzframe_drv_bigc_p(&(me->pz), args, nargs);
}

DEFINE_DRIVER(host_bpmd, "PC driver for simulation of TBT beam signals from BPM",
              NULL, NULL,
              sizeof(privrec_t), bpmd_params,
              0, 20, /*!!! CXSD_DB_MAX_BUSINFOCOUNT */
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_b, term_b, rdwr_p, bigc_p);
