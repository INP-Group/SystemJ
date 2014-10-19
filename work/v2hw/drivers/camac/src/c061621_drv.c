#include "cxsd_driver.h"
#include "pzframe_drv.h"

#include "drv_i/c061621_drv_i.h"


enum
{
    DEVTYPE_ADC101S  = 001,
    DEVTYPE_ADC102S  = 002,
    DEVTYPE_STROBE_S = 003,
    DEVTYPE_ADC850S  = 004,
    DEVTYPE_ADC710S  = 005,
    DEVTYPE_ADC101SK = 011,
    DEVTYPE_ADC850SK = 014,
};
static const char *devnames[] =
{
    [DEVTYPE_ADC101S]  = "ADC-101S",
    [DEVTYPE_ADC102S]  = "ADC-102S",
    [DEVTYPE_STROBE_S] = "STROBE-S",
    [DEVTYPE_ADC850S]  = "ADC-850S",
    [DEVTYPE_ADC710S]  = "ADC-710S",
    [DEVTYPE_ADC101SK] = "ADC-101SK",
    [DEVTYPE_ADC850SK] = "ADC-850SK",
};

enum
{
    A_MEMORY = 0,
    A_STATUS = 1,
    A_MEMADR = 2,
    A_LIMITS = 3,
    A_AUXDAT = 4,
    A_PRGRUN = 5,
    A_DOTICK = 6,
    A_COMREG = 7,  // K-suffixed devices
    A_INDREG = 15, // 710S/710SK only?
};

enum
{
    S_CPU_RAM = 1 << 0,
    S_DIS_LAM = 1 << 1,
    S_SNG_RUN = 1 << 2,
    S_UNUSED3 = 1 << 3,
    S_AUTOPLT = 1 << 4,
};

static int tick_codes[] =
{
    // Digit 1: 0:x1, 1:x2, 2:x4, 3:x5
    // Digit 2:
    //   0:0.1ns
    //   1:  1ns
    //   2: 10ns
    //   2:100ns
    //   4:  1us
    //   5: 10us
    //   6:100us
    //   7:  1ms

    032, // 50  ns
    003, // 100 ns
    013, // 200 ns
    023, // 400 ns
    033, // 500 ns
    004, // 1   us
    014, // 2   us
    024, // 4   us
    034, // 5   us
    005, // 10  us
    015, // 20  us
    025, // 40  us
    035, // 50  us
    006, // 100 us
    016, // 200 us
    026, // 400 us
    036, // 500 us
    007, // 1   ms
    017, // 2   ms
    027, // CPU
    037, // Ext
};

typedef struct
{
    pzframe_drv_t    pz;
    int              devid;
    int              N_DEV;

    C061621_DATUM_T  retdata [C061621_MAX_NUMPTS*C061621_NUM_LINES];

    int              memsize;
    int              mintick;
    int              k_type;
    //int              cword; !!!???
} c061621_privrec_t;

#define PDR2ME(pdr) ((c061621_privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

static psp_paramdescr_t c061621_params[] =
{
    PSP_P_FLAG("calcstats", c061621_privrec_t, pz.nxt_args[C061621_PARAM_CALC_STATS], 1, 0),
    PSP_P_END()
};

static void SetToPassive(c061621_privrec_t *me)
{
  int         w;
  int         junk;

    w = S_CPU_RAM | S_DIS_LAM;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS, 16, &w);
    DO_NAF(CAMAC_REF, me->N_DEV,        0, 10, &junk); // Drom LAM
}

static int32 ValidateParam(pzframe_drv_t *pdr, int n, int v)
{
  c061621_privrec_t *me = PDR2ME(pdr);

    if      (n == C061621_PARAM_TIMING)
    {
        if (v < me->mintick)             v = me->mintick;
        if (v > countof(tick_codes) - 1) v = countof(tick_codes) - 1;
    }
    else if (n == C061621_PARAM_RANGE)
    {
        if (v < 0) v = 0;
        if (v > 7) v = 7;
        if (me->k_type  &&  v < 4) v = 4; // K-type devices have limited range
    }
    else if (n == C061621_PARAM_PTSOFS)
    {
        if (v < 0)               v = 0;
        if (v > me->memsize - 1) v = me->memsize - 1;
    }
    else if (n == C061621_PARAM_NUMPTS)
    {
        if (v < 1)               v = 1;
        if (v > me->memsize)     v = me->memsize;
    }

    return v;
}

static int   InitParams(pzframe_drv_t *pdr)
{
  c061621_privrec_t *me = PDR2ME(pdr);

  rflags_t           rflags;
  int                w;
  int                devtype;
  const char        *devname;

    rflags = status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_AUXDAT, 1, &w));
    if (rflags != 0)
    {
        DoDriverLog(me->devid, 0,
                    "reading devtype, NAF(%d,%d,%d): X=%d,Q=%d, aborting",
                    me->N_DEV, A_AUXDAT, 1,
                    rflags & CXRF_CAMAC_NO_X? 0 : 1,
                    rflags & CXRF_CAMAC_NO_Q? 0 : 1);
        return -rflags;
    }

    devtype = w & 017;
    devname = NULL;
    if (devtype < countof(devnames)) devname = devnames[devtype];
    if (devname == NULL) devname = "???";

    SetToPassive(me);

    switch (devtype)
    {
        case DEVTYPE_ADC101S:
            me->memsize = 4096;
            me->mintick = 5;
            DoDriverLog(me->devid, 0, "%s, memsize=%d",
                        devname, me->memsize);
            break;

        case DEVTYPE_ADC101SK:
            me->memsize = 4096;
            me->mintick = 5;
            DoDriverLog(me->devid, 0, "%s, using as %s with memsize=%d",
                        devname, devnames[devtype & 07], me->memsize);
            break;

        case DEVTYPE_ADC850S:
            me->memsize = 1024;
            me->mintick = 0;
            DoDriverLog(me->devid, 0, "%s, memsize=%d",
                        devname, me->memsize);
            break;

        case DEVTYPE_ADC850SK:
            me->memsize = 4096;
            me->mintick = 0;
            DoDriverLog(me->devid, 0, "%s, using as %s with memsize=%d",
                        devname, devnames[devtype & 07], me->memsize);
            break;

        default:
            DoDriverLog(me->devid, 0, "unsupported devtype=%o:%s", devtype, devname);
            return -CXRF_WRONG_DEV;
    }
    me->k_type = (devtype & 010) != 0;

    pdr->nxt_args[C061621_PARAM_NUMPTS] = me->memsize;
    pdr->nxt_args[C061621_PARAM_TIMING] = me->mintick;

    return DEVSTATE_OPERATING;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  c061621_privrec_t *me = PDR2ME(pdr);

  int  w;
  int  range_l;
  int  range_k;

    /* Additional check of PTSOFS against NUMPTS */
    if (me->pz.cur_args[C061621_PARAM_PTSOFS] > me->memsize - me->pz.cur_args[C061621_PARAM_NUMPTS])
        me->pz.cur_args[C061621_PARAM_PTSOFS] = me->memsize - me->pz.cur_args[C061621_PARAM_NUMPTS];

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[C061621_PARAM_NUMPTS] *
                                         C061621_NUM_LINES     *
                                         C061621_DATAUNITS;

    if (me->k_type)
    {
        range_l = 4;
        range_k = me->pz.cur_args[C061621_PARAM_RANGE] - 4;
    }
    else
    {
        range_l = me->pz.cur_args[C061621_PARAM_RANGE];
        range_k = 0;
    }

    /* Program the device */
    SetToPassive(me);
    w = (range_l << 6) | tick_codes[me->pz.cur_args[C061621_PARAM_TIMING]];
    DO_NAF(CAMAC_REF, me->N_DEV, A_LIMITS, 16, &w);
    w = (range_k * 0xAA) + (0 << 8); // Force chan=0
    if (me->k_type) DO_NAF(CAMAC_REF, me->N_DEV, A_COMREG, 16, &w);
    w = S_SNG_RUN;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS, 16, &w);

    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  c061621_privrec_t *me = PDR2ME(pdr);

  int  junk;
    
    DO_NAF(CAMAC_REF, me->N_DEV, A_PRGRUN, 16, &junk);

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  c061621_privrec_t *me = PDR2ME(pdr);

    SetToPassive(me);

    return PZFRAME_R_READY;
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  c061621_privrec_t *me = PDR2ME(pdr);

  int              w;
  int              base0;
  int              quant;
  C061621_DATUM_T *dp;
  int              n;
  enum            {COUNT_MAX = 100};
  int              count;
  int              x;
  int              rdata[COUNT_MAX];
  int              v;
  int32            min_v, max_v, sum_v;

  static int param2quant[8] = { 390,   780, 1560,   3120,
                               6250, 12500, 25000, 50000};

    w = me->pz.cur_args[C061621_PARAM_PTSOFS];
    DO_NAF(CAMAC_REF, me->N_DEV, A_MEMADR, 16, &w); // Set read address

    quant = param2quant[me->pz.cur_args[C061621_PARAM_RANGE]];
    base0 = 4095 /* ==2047.5*2 */ * quant / 2;

    dp = me->retdata;

    if (me->pz.cur_args[C061621_PARAM_CALC_STATS] == 0  ||  1)
    {
        for (n = me->pz.cur_args[C061621_PARAM_NUMPTS];  n > 0;  n -= count)
        {
            count = n;
            if (count > COUNT_MAX) count = COUNT_MAX;
            
            DO_NAFB(CAMAC_REF, me->N_DEV, A_MEMORY, 0, rdata, count);
            for (x = 0;  x < count;  x++)
            {
                *dp++ = quant * (rdata[x] & 0x0FFF) - base0;
            }
        }

        me->pz.cur_args[C061621_PARAM_MIN] =
        me->pz.nxt_args[C061621_PARAM_MIN] =
        me->pz.cur_args[C061621_PARAM_MAX] =
        me->pz.nxt_args[C061621_PARAM_MAX] =
        me->pz.cur_args[C061621_PARAM_AVG] =
        me->pz.nxt_args[C061621_PARAM_AVG] =
        me->pz.cur_args[C061621_PARAM_INT] =
        me->pz.nxt_args[C061621_PARAM_INT] = 0;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////

enum
{
    PARAM_SHOT     = C061621_PARAM_SHOT,
    PARAM_ISTART   = C061621_PARAM_ISTART,
    PARAM_WAITTIME = C061621_PARAM_WAITTIME,
    PARAM_STOP     = C061621_PARAM_STOP,
    PARAM_ELAPSED  = C061621_PARAM_ELAPSED,

    NUM_PARAMS     = C061621_NUM_PARAMS,
};

#define FASTADC_NAME c061621
#include "camac_fastadc_common.h"
