#include "cxsd_driver.h"
#include "pzframe_drv.h"

#include "drv_i/nadc4_drv_i.h"


enum
{
    A_ANY  = 0,

    A_ADDR = 0,
    A_STAT = 1,

    STAT_NONE  = 0,       // No bits -- stop
    STAT_AINC  = 1 << 0,  // Enable auto-increment on reads
    STAT_START = 1 << 1,  // Start measurements
    STAT_EXT   = 1 << 2,  // When =1, start on external pulse (START), =0 -- start immediately
    
    ADDR_EOM   = 32767,   // End-Of-Memory
};


typedef struct
{
    pzframe_drv_t    pz;
    int              devid;
    int              N_DEV;

    NADC4_DATUM_T    retdata[NADC4_MAX_NUMPTS*NADC4_NUM_LINES];

#ifdef NADC4_EXT_PRIVREC
    NADC4_EXT_PRIVREC
#endif
} nadc4_privrec_t;

#define PDR2ME(pdr) ((nadc4_privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

static psp_paramdescr_t nadc4_params[] =
{
    PSP_P_FLAG("calcstats", nadc4_privrec_t, pz.nxt_args[NADC4_PARAM_CALC_STATS], 1, 0),

    PSP_P_INT ("zero0", nadc4_privrec_t, pz.nxt_args[NADC4_PARAM_ZERO0], 0, -2048, +2047),
    PSP_P_INT ("zero1", nadc4_privrec_t, pz.nxt_args[NADC4_PARAM_ZERO1], 0, -2048, +2047),
    PSP_P_INT ("zero2", nadc4_privrec_t, pz.nxt_args[NADC4_PARAM_ZERO2], 0, -2048, +2047),
    PSP_P_INT ("zero3", nadc4_privrec_t, pz.nxt_args[NADC4_PARAM_ZERO3], 0, -2048, +2047),
#ifdef NADC4_EXT_PARAMS
    NADC4_EXT_PARAMS
#endif
    PSP_P_END()
};


static inline rflags_t nadc4_code2uv(uint16 code, int32 zero_shift,
                                     NADC4_DATUM_T *r_p)
{
    *r_p = code - 2048 - zero_shift; /*!!!*/
    return 0;
}

static int32 ValidateParam(pzframe_drv_t *pdr, int n, int v)
{
    if      (n == NADC4_PARAM_PTSOFS)
    {
        if (v < 0)                  v = 0;
        if (v > NADC4_MAX_NUMPTS-1) v = NADC4_MAX_NUMPTS-1;
    }
    else if (n == NADC4_PARAM_NUMPTS)
    {
        if (v < 1)                  v = 1;
        if (v > NADC4_MAX_NUMPTS)   v = NADC4_MAX_NUMPTS;
    }
    else if (n >= NADC4_PARAM_ZERO0  &&  n <= NADC4_PARAM_ZERO3)
    {
        if (v < -2048) v = -2048;
        if (v > +2047) v = +2047;
    }
#ifdef NADC4_EXT_VALIDATE_PARAM
    NADC4_EXT_VALIDATE_PARAM
#endif

    return v;
}

static int   InitParams(pzframe_drv_t *pdr)
{
    pdr->nxt_args[NADC4_PARAM_NUMPTS] = 1024;

    pdr->retdataunits = NADC4_DATAUNITS;

#ifdef NADC4_EXT_INIT_PARAMS
    NADC4_EXT_INIT_PARAMS
#endif

    return DEVSTATE_OPERATING;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  nadc4_privrec_t *me = PDR2ME(pdr);

  int              w;
    
    /* Additional check of PTSOFS against NUMPTS */
    if (me->pz.cur_args[NADC4_PARAM_PTSOFS] > NADC4_MAX_NUMPTS - me->pz.cur_args[NADC4_PARAM_NUMPTS])
        me->pz.cur_args[NADC4_PARAM_PTSOFS] = NADC4_MAX_NUMPTS - me->pz.cur_args[NADC4_PARAM_NUMPTS];

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[NADC4_PARAM_NUMPTS] *
                                         NADC4_NUM_LINES     *
                                         NADC4_DATAUNITS;
    
    /* Program the device */
    /* a. Prepare... */
    w = STAT_NONE;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);  // Stop device
    DO_NAF(CAMAC_REF, me->N_DEV, 0,      0,  &w);  // Dummy read -- reset LAM
    /* b. Set parameters */
    w = ADDR_EOM -
        me->pz.cur_args[NADC4_PARAM_NUMPTS] -
        me->pz.cur_args[NADC4_PARAM_PTSOFS] -
        NADC4_JUNK_MSMTS; // 6 junk measurements
    DO_NAF(CAMAC_REF, me->N_DEV, A_ADDR, 16, &w);  // Set addr
#ifdef NADC4_EXT_START
    NADC4_EXT_START
#endif
    /* c. Allow start */
    w = STAT_START | STAT_EXT*1;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);

    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  nadc4_privrec_t *me = PDR2ME(pdr);

  int              w;

    w = STAT_START | STAT_EXT*0;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);  // Set EXT=0, thus causing immediate prog-start

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  nadc4_privrec_t *me = PDR2ME(pdr);

  int              w;

    w = STAT_NONE;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);  // Stop device
    DO_NAF(CAMAC_REF, me->N_DEV, 0,      0,  &w);  // Dummy read -- reset LAM
  
    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);

    return PZFRAME_R_READY;
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  nadc4_privrec_t *me = PDR2ME(pdr);

  int              w;
  int              N_DEV = me->N_DEV;
  NADC4_DATUM_T   *dp;
  int              nc;
  int32            zero_shift;
  int              n;
  enum            {COUNT_MAX = 100};
  int              count;
  int              x;
  int              rdata[COUNT_MAX];
  int              v;
  int              cum_v;
  NADC4_DATUM_T    vd;
  int32            min_v, max_v, sum_v;
  
    /* Stop the device and set autoincrement-on-read */
    w = STAT_NONE;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);  // Stop device (in fact, does nothing, since ADC4 just freezes until LAM reset)
    DO_NAF(CAMAC_REF, me->N_DEV, 0,      0,  &w);  // Dummy read -- reset LAM
    w = STAT_AINC;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);  // Set autoincrement mode

    cum_v = 0;
    dp = me->retdata;
    for (nc = 0;  nc < NADC4_NUM_LINES;  nc++)
    {
        /* Set read address */
        w = ADDR_EOM -
            me->pz.cur_args[NADC4_PARAM_NUMPTS];
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADDR, 16, &w);

        zero_shift = me->pz.cur_args[NADC4_PARAM_ZERO0 + nc];

        if (me->pz.cur_args[NADC4_PARAM_CALC_STATS] == 0)
        {
            for (n = me->pz.cur_args[NADC4_PARAM_NUMPTS];  n > 0;  n -= count)
            {
                count = n;
                if (count > COUNT_MAX) count = COUNT_MAX;

                DO_NAFB(CAMAC_REF, N_DEV, nc, 0, rdata, count);
                for (x = 0;  x < count;  x++)
                {
                    v = rdata[x] & 0x0FFF;
                    cum_v |= (v - 1) | (v + 1);
                    nadc4_code2uv(v, zero_shift, dp);
                    dp++;
                }
            }

            me->pz.cur_args[NADC4_PARAM_MIN0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_MIN0 + nc] =
            me->pz.cur_args[NADC4_PARAM_MAX0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_MAX0 + nc] =
            me->pz.cur_args[NADC4_PARAM_AVG0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_AVG0 + nc] =
            me->pz.cur_args[NADC4_PARAM_INT0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_INT0 + nc] = 0;
        }
        else
        {
            min_v = 0x0FFF; max_v = 0; sum_v = 0;
            for (n = me->pz.cur_args[NADC4_PARAM_NUMPTS];  n > 0;  n -= count)
            {
                count = n;
                if (count > COUNT_MAX) count = COUNT_MAX;
    
                DO_NAFB(CAMAC_REF, N_DEV, nc, 0, rdata, count);
                for (x = 0;  x < count;  x++)
                {
                    v = rdata[x] & 0x0FFF;
                    cum_v |= (v - 1) | (v + 1);
                    nadc4_code2uv(v, zero_shift, dp);
                    dp++;
                    if (min_v > v) min_v = v;
                    if (max_v < v) max_v = v;
                    sum_v += v;
                }
            }

            nadc4_code2uv(min_v, zero_shift, &vd);
            me->pz.cur_args[NADC4_PARAM_MIN0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_MIN0 + nc] = vd;

            nadc4_code2uv(max_v, zero_shift, &vd);
            me->pz.cur_args[NADC4_PARAM_MAX0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_MAX0 + nc] = vd;

            nadc4_code2uv(sum_v / me->pz.cur_args[NADC4_PARAM_NUMPTS], zero_shift, &vd);
            me->pz.cur_args[NADC4_PARAM_AVG0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_AVG0 + nc] = vd;

            vd = sum_v - (2048 + zero_shift) * me->pz.cur_args[NADC4_PARAM_NUMPTS];
            me->pz.cur_args[NADC4_PARAM_INT0 + nc] =
            me->pz.nxt_args[NADC4_PARAM_INT0 + nc] = vd;
        }
    }

    me->pz.cur_args[NADC4_PARAM_STATUS] =
        (cum_v < 0  ||  (cum_v & 0x1000) != 0)? CXRF_OVERLOAD : 0;

    return 0;
}


//////////////////////////////////////////////////////////////////////

enum
{
    PARAM_SHOT     = NADC4_PARAM_SHOT,
    PARAM_ISTART   = NADC4_PARAM_ISTART,
    PARAM_WAITTIME = NADC4_PARAM_WAITTIME,
    PARAM_STOP     = NADC4_PARAM_STOP,
    PARAM_ELAPSED  = NADC4_PARAM_ELAPSED,

    NUM_PARAMS     = NADC4_NUM_PARAMS,
};

#define FASTADC_NAME nadc4
#include "camac_fastadc_common.h"
