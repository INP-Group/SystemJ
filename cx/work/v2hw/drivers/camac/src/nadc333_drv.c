#include "cxsd_driver.h"
#include "pzframe_drv.h"

#include "drv_i/nadc333_drv_i.h"


enum
{
    A_ANY  = 0,
    
    A_DATA = 0,
    A_ADDR = 8,
    A_STAT = 9,
    A_LIMS = 10
};


typedef struct
{
    pzframe_drv_t    pz;
    int              devid;
    int              N_DEV;

    NADC333_DATUM_T  retdata[NADC333_MAX_NUMPTS];

    int              nchans;
    int              curscales[4];
} nadc333_privrec_t;

#define PDR2ME(pdr) ((nadc333_privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

static psp_paramdescr_t nadc333_params[] =
{
    PSP_P_END()
};


static inline rflags_t code2uv(uint16 code, int scale, NADC333_DATUM_T *r_p)
{
    /* Overflow? */
    if ((code & (1 << 12)) != 0)
    {
        *r_p = (code & (1 << 11)) != 0? +100000000 : -100000000;
        return CXRF_OVERLOAD;
    }

    *r_p = ((int)(code & 0x0FFF) - 2048) * scale;
    
    return 0;
}

static int32 ValidateParam(pzframe_drv_t *pdr, int n, int v)
{
    if      (n == NADC333_PARAM_TIMING)
        v &= 7;
    else if (n >= NADC333_PARAM_RANGE0  &&  n <= NADC333_PARAM_RANGE3)
        v = (v >= NADC333_R_8192  &&  v <= NADC333_R_1024)? v : NADC333_R_OFF;
    else if (n == NADC333_PARAM_PTSOFS)
    {
        if (v < 0)                    v = 0;
        if (v > NADC333_MAX_NUMPTS-1) v = NADC333_MAX_NUMPTS-1;
    }
    else if (n == NADC333_PARAM_NUMPTS)
    {
        if (v < 1)                    v = 1;
        if (v > NADC333_MAX_NUMPTS)   v = NADC333_MAX_NUMPTS;
    }

    return v;
}

static int   InitParams(pzframe_drv_t *pdr)
{
    pdr->nxt_args[NADC333_PARAM_NUMPTS] = 1024;
    pdr->nxt_args[NADC333_PARAM_TIMING] = NADC333_T_500NS;
    pdr->nxt_args[NADC333_PARAM_RANGE0] = NADC333_R_8192;
    pdr->nxt_args[NADC333_PARAM_RANGE1] = NADC333_R_8192;
    pdr->nxt_args[NADC333_PARAM_RANGE2] = NADC333_R_8192;
    pdr->nxt_args[NADC333_PARAM_RANGE3] = NADC333_R_8192;

    pdr->retdataunits = NADC333_DATAUNITS;

    return DEVSTATE_OPERATING;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  nadc333_privrec_t *me = PDR2ME(pdr);

  int                nc;
  int                nchans;
  int                rcode;

  int                regv_status;
  int                regv_limits;

  int                w;
  int                junk;

  static int scale_factors[4] =
  {
      8192000/2048,
      4096000/2048,
      2048000/2048,
      1024000/2048
  };

    /* Calculate # of channels and prepare scale factors */
    for (nc = 0, nchans = 0, regv_status = 0, regv_limits = (me->pz.cur_args[NADC333_PARAM_TIMING] << 8);
         nc < NADC333_NUM_LINES;
         nc++)
    {
        rcode = me->pz.cur_args[NADC333_PARAM_RANGE0 + nc];
        if (rcode >= NADC333_R_8192  &&  rcode <= NADC333_R_1024)
        {
            regv_status |= (1 << (nc + 4));
            regv_limits |= (rcode << (nc*2));
            me->curscales[nchans] = scale_factors[rcode];
            nchans++;
        }
    }
    me->nchans = nchans;
    DoDriverLog(me->devid, DRIVERLOG_DEBUG, "scales={%d,%d,%d,%d} regv_status=%08x regv_limits=%08x", me->curscales[0], me->curscales[1], me->curscales[2], me->curscales[3], regv_status, regv_limits);

    /* No channels?! */
    if (nchans == 0)
    {
        me->pz.retdatasize = 0;
        return PZFRAME_R_READY;
    }

    /* Check NUMPTS */
    if (me->pz.cur_args[NADC333_PARAM_NUMPTS] > NADC333_MAX_NUMPTS / nchans)
        me->pz.cur_args[NADC333_PARAM_NUMPTS] = NADC333_MAX_NUMPTS / nchans;

    /* Check PTSOFS */
    if (me->pz.cur_args[NADC333_PARAM_PTSOFS] > NADC333_MAX_NUMPTS / nchans - me->pz.cur_args[NADC333_PARAM_NUMPTS])
        me->pz.cur_args[NADC333_PARAM_PTSOFS] = NADC333_MAX_NUMPTS / nchans - me->pz.cur_args[NADC333_PARAM_NUMPTS];

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[NADC333_PARAM_NUMPTS] * nchans * NADC333_DATAUNITS;

    /* Program the device */
    /* a. Set parameters */
    regv_status |= 1;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &regv_status);  // Set status with HLT=1
    DO_NAF(CAMAC_REF, me->N_DEV, A_LIMS, 16, &regv_limits);  // Set limits
    w = (me->pz.cur_args[NADC333_PARAM_NUMPTS] +
         me->pz.cur_args[NADC333_PARAM_PTSOFS])
        * nchans;
    DO_NAF(CAMAC_REF, me->N_DEV, A_ADDR, 16, &w);            // Set address counter
    DO_NAF(CAMAC_REF, me->N_DEV, 0,      10, &junk);         // Drop LAM
    DO_NAF(CAMAC_REF, me->N_DEV, A_ANY,  26, &junk);         // Enable LAM
    /* b. Start */
    regv_status &=~ 1;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &regv_status);  // Set status with HLT=0

    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  nadc333_privrec_t *me = PDR2ME(pdr);

  int                junk;
    
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 25, &junk);

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  nadc333_privrec_t *me = PDR2ME(pdr);

  int                w;

    w = 0; DO_NAF(CAMAC_REF, me->N_DEV, A_ANY,  24, &w);     // Disable LAM
    w = 1; DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);     // Stop (HLT:=1)
    w = 0; DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 10, &w);     // Reset LAM
    
    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);

    return PZFRAME_R_READY;
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  nadc333_privrec_t *me = PDR2ME(pdr);

  int                numpts;
  int                N_DEV = me->N_DEV;
  int                w;

  rflags_t           rfv;
  int                qq;
  NADC333_DATUM_T   *p0, *p1, *p2, *p3;
  int                s0,  s1,  s2,  s3;

  enum              {COUNT_MAX = 108}; // ==0 on %2, %3, %4
  int                count;
  int                x;
  int                rdata[COUNT_MAX];
  
    w = 0; DO_NAF(CAMAC_REF, me->N_DEV, A_ANY,  24, &w);     // Disable LAM
    w = 1; DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 16, &w);     // Stop (HLT:=1)
    w = 0; DO_NAF(CAMAC_REF, me->N_DEV, A_STAT, 10, &w);     // Reset LAM

    numpts = me->pz.cur_args[NADC333_PARAM_NUMPTS];
    w = numpts * me->nchans;
    DO_NAF(CAMAC_REF, me->N_DEV, A_ADDR, 16, &w);            // Set address counter
    rfv = 0;
    
    s0 = me->curscales[0];
    s1 = me->curscales[1];
    s2 = me->curscales[2];
    s3 = me->curscales[3];

    switch (me->nchans)
    {
        case 1:
            p0 = me->retdata + numpts * 0;
            for (qq = 0; numpts > 0;  numpts -= count /* / 1 */)
            {
                count = numpts /* * 1 */;
                if (count > COUNT_MAX) count = COUNT_MAX;

                qq |= ~(DO_NAFB(CAMAC_REF, N_DEV, A_DATA, 0, rdata, count));
                for (x = 0;  x < count;  p0++)
                {
                    rfv |= code2uv(rdata[x++], s0, p0);
                }
            }
            break;

        case 2:
            p0 = me->retdata + numpts * 0;
            p1 = me->retdata + numpts * 1;
            for (qq = 0; numpts > 0;  numpts -= count / 2)
            {
                count = numpts * 2;
                if (count > COUNT_MAX) count = COUNT_MAX;

                qq |= ~(DO_NAFB(CAMAC_REF, N_DEV, A_DATA, 0, rdata, count));
                for (x = 0;  x < count;  p0++, p1++)
                {
                    rfv |= code2uv(rdata[x++], s0, p0);
                    rfv |= code2uv(rdata[x++], s1, p1);
                }
            }
            break;

        case 3:
            p0 = me->retdata + numpts * 0;
            p1 = me->retdata + numpts * 1;
            p2 = me->retdata + numpts * 2;
            for (qq = 0; numpts > 0;  numpts -= count / 3)
            {
                count = numpts * 3;
                if (count > COUNT_MAX) count = COUNT_MAX;

                qq |= ~(DO_NAFB(CAMAC_REF, N_DEV, A_DATA, 0, rdata, count));
                for (x = 0;  x < count;  p0++, p1++, p2++)
                {
                    rfv |= code2uv(rdata[x++], s0, p0);
                    rfv |= code2uv(rdata[x++], s1, p1);
                    rfv |= code2uv(rdata[x++], s2, p2);
                }
            }
            break;

        case 4:
            p0 = me->retdata + numpts * 0; 
            p1 = me->retdata + numpts * 1; 
            p2 = me->retdata + numpts * 2; 
            p3 = me->retdata + numpts * 3; 
            for (qq = 0; numpts > 0;  numpts -= count / 4)
            {
                count = numpts * 4;
                if (count > COUNT_MAX) count = COUNT_MAX;

                qq |= ~(DO_NAFB(CAMAC_REF, N_DEV, A_DATA, 0, rdata, count));
                for (x = 0;  x < count;  p0++, p1++, p2++, p3++)
                {
                    rfv |= code2uv(rdata[x++], s0, p0);
                    rfv |= code2uv(rdata[x++], s1, p1);
                    rfv |= code2uv(rdata[x++], s2, p2);
                    rfv |= code2uv(rdata[x++], s3, p3);
                }
            }
            break;

    }

    return rfv;
}


//////////////////////////////////////////////////////////////////////

enum
{
    PARAM_SHOT     = NADC333_PARAM_SHOT,
    PARAM_ISTART   = NADC333_PARAM_ISTART,
    PARAM_WAITTIME = NADC333_PARAM_WAITTIME,
    PARAM_STOP     = NADC333_PARAM_STOP,
    PARAM_ELAPSED  = NADC333_PARAM_ELAPSED,

    NUM_PARAMS     = NADC333_NUM_PARAMS,
};

#define FASTADC_NAME nadc333
#include "camac_fastadc_common.h"
