#include "cxsd_driver.h"
#include "pzframe_drv.h"

#include "drv_i/nadc502_drv_i.h"


enum
{
    A_DATA1    = 0,
    A_DATA2    = 1,
    A_ADRPTR   = 0,
};

enum
{
    S_RUN          = 1 << 0,
    S_REC          = 1 << 1,
//    S_Fn           = 1 << 2,
    S_TIMING_SHIFT =      2,
    S_TMODE_SHIFT  =      3,
    S_STM          = 1 << 4,
    S_BSY          = 1 << 5,
    S_OV           = 1 << 6,
    S_RSRVD8       = 1 << 7,
    S_SHIFT1_SHIFT =      8,
    S_RANGE1_SHIFT =     10,
    S_SHIFT2_SHIFT =     12,
    S_RANGE2_SHIFT =     14,
};

typedef struct
{
    pzframe_drv_t    pz;
    int              devid;
    int              N_DEV;

    NADC502_DATUM_T  retdata[NADC502_MAX_NUMPTS*NADC502_NUM_LINES];

    int              cword;
} nadc502_privrec_t;

#define PDR2ME(pdr) ((nadc502_privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

static psp_paramdescr_t nadc502_params[] =
{
    PSP_P_FLAG("calcstats", nadc502_privrec_t, pz.nxt_args[NADC502_PARAM_CALC_STATS], 1, 0),
    PSP_P_END()
};

static int32 saved_data_signature[] =
{
    0x00FFFFFF,
    0x00000000,
    0x00123456,
    0x00654321,
};

static void WriteMem(nadc502_privrec_t *me,
                     int byte_offset, int32 *data, int count)
{
  int    w;
  int    n;
    
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 24, &w); // Disable LAM
    w = 0;
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &w); // Drop RUN
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 10, &w); // Drop LAM

    if (byte_offset < 0) byte_offset = 65534 * 4 - byte_offset;
    for (n = 0;  n < count;  n++)
    {
        // 1. Write low-12 to chan0
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        w = data[n] & 0x00000FFF;
        DO_NAF(CAMAC_REF, me->N_DEV, 0,        16, &w);
        // 2. Write high-12 to chan1
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        w = (data[n] >> 12) & 0x00000FFF;
        DO_NAF(CAMAC_REF, me->N_DEV, 1,        16, &w);
    }
}

static void ReadMem (nadc502_privrec_t *me,
                     int byte_offset, int32 *data, int count)
{
  int    w;
  int    n;

    bzero(data, sizeof(*data) * count);

    DO_NAF(CAMAC_REF, me->N_DEV, 0, 24, &w); // Disable LAM
    w = 0;
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &w); // Drop RUN
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 10, &w); // Drop LAM

    if (byte_offset < 0) byte_offset = 65535 * 4 - byte_offset;
    for (n = 0;  n < count;  n++)
    {
        // 1. Read low-12 from chan0
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        DO_NAF(CAMAC_REF, me->N_DEV, 0,        0,  &w);
        data[n]  = w & 0x00000FFF;
        // 2. Read high-12 from chan1
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        DO_NAF(CAMAC_REF, me->N_DEV, 1,        0,  &w);
        data[n] |= (w & 0x00000FFF) << 12;
    }
}


static int32 ValidateParam(pzframe_drv_t *pdr, int n, int v)
{
    if      (n == NADC502_PARAM_TIMING)
        v &= 1;
    else if (n == NADC502_PARAM_TMODE)
        v &= 1;
    else if (n == NADC502_PARAM_RANGE1  ||  n == NADC502_PARAM_RANGE2)
        v &= 3;
    else if (n == NADC502_PARAM_SHIFT1  ||  n == NADC502_PARAM_SHIFT2)
    {
        if (v != NADC502_SHIFT_NONE  &&
            v != NADC502_SHIFT_NEG4  &&
            v != NADC502_SHIFT_POS4)
        v = NADC502_SHIFT_NONE;
    }
    else if (n == NADC502_PARAM_PTSOFS)
    {
        if (v < 0)                    v = 0;
        if (v > NADC502_MAX_NUMPTS-1) v = NADC502_MAX_NUMPTS-1;
        
    }
    else if (n == NADC502_PARAM_NUMPTS)
    {
        if (v < 1)                    v = 1;
        if (v > NADC502_MAX_NUMPTS)   v = NADC502_MAX_NUMPTS;
    }

    return v;
}


static int   InitParams(pzframe_drv_t *pdr)
{
  nadc502_privrec_t *me = PDR2ME(pdr);

  int    n;
  int32  saved_data[NADC502_NUM_PARAMS + countof(saved_data_signature)];
    
    me->pz.nxt_args[NADC502_PARAM_NUMPTS] = 1024;
    me->pz.nxt_args[NADC502_PARAM_TIMING] = NADC502_TIMING_INT;
    me->pz.nxt_args[NADC502_PARAM_TMODE]  = NADC502_TMODE_HF;
    me->pz.nxt_args[NADC502_PARAM_RANGE1] = NADC502_RANGE_8192;
    me->pz.nxt_args[NADC502_PARAM_RANGE2] = NADC502_RANGE_8192;
    me->pz.nxt_args[NADC502_PARAM_SHIFT1] = NADC502_SHIFT_NONE;
    me->pz.nxt_args[NADC502_PARAM_SHIFT2] = NADC502_SHIFT_NONE;
    
    me->pz.retdataunits = NADC502_DATAUNITS;

#if 0
    /* Try to restore previous settings */
    ReadMem(devid, me, 0, saved_data, countof(saved_data));
    if (memcmp(saved_data + NADC502_NUM_PARAMS, saved_data_signature, sizeof(saved_data_signature)) == 0)
    {
        memcpy(me->pz.nxt_args,
               saved_data,
               sizeof(me->pz.nxt_args[0]) * NADC502_NUM_PARAMS);
        for (n = 0;  n < NADC502_NUM_PARAMS;  n++)
            me->nxt_args[n] = ValidateParam(pdr, n, me->pz.nxt_args[n]);
    }
#endif

    return DEVSTATE_OPERATING;
}


static int  StartMeasurements(pzframe_drv_t *pdr)
{
  nadc502_privrec_t *me = PDR2ME(pdr);

  int                cword;
  int                junk;
  int                w;

  static int param2shiftcode[3] = {0, 1, 2};
    
    /* Additional check of PTSOFS against NUMPTS */
    if (me->pz.cur_args[NADC502_PARAM_PTSOFS] > NADC502_MAX_NUMPTS - me->pz.cur_args[NADC502_PARAM_NUMPTS])
        me->pz.cur_args[NADC502_PARAM_PTSOFS] = NADC502_MAX_NUMPTS - me->pz.cur_args[NADC502_PARAM_NUMPTS];

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[NADC502_PARAM_NUMPTS] *
                                         NADC502_NUM_LINES     *
                                         me->pz.retdataunits;

#if 0
    /* Store current parameters */
    WriteMem(devid, me,
             0,
             me->pz.cur_args, NADC502_NUM_PARAMS);
    WriteMem(devid, me,
             sizeof(me->pz.cur_args[0]) * NADC502_NUM_PARAMS,
             saved_data_signature, countof(saved_data_signature));
#endif

    /* Program the device */
    /* a. Drop LAM first... */
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 10, &junk);
    /* b. Prepare status/control word */
    cword = me->cword =
        /* common params */
        S_RUN * 0 | S_REC * 0 |
        (me->pz.cur_args[NADC502_PARAM_TIMING] << S_TIMING_SHIFT) |
        (me->pz.cur_args[NADC502_PARAM_TMODE]  << S_TMODE_SHIFT ) |
        S_STM * 1 |
        /* chan1 */
        (param2shiftcode[me->pz.cur_args[NADC502_PARAM_SHIFT1]] << S_SHIFT1_SHIFT) |
        (me->pz.cur_args[NADC502_PARAM_RANGE1] << S_RANGE1_SHIFT) |
        /* chan2 */
        (param2shiftcode[me->pz.cur_args[NADC502_PARAM_SHIFT2]] << S_SHIFT2_SHIFT) |
        (me->pz.cur_args[NADC502_PARAM_RANGE2] << S_RANGE2_SHIFT);

    ////DoDriverLog(devid, DRIVERLOG_DEBUG, "%s(), CWORD=0x%x", __FUNCTION__, cword);

    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // 1st write -- to clear RUN bit (?)
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // 2nd write -- to set all other bits
    /* c. Set "measurement size" */
    w = me->pz.cur_args[NADC502_PARAM_NUMPTS] + me->pz.cur_args[NADC502_PARAM_PTSOFS];
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 17, &w);
    /* d. Enable start */
    cword |= S_RUN;
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // Set RUN
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 26, &junk);  // Enable LAM
    
    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  nadc502_privrec_t *me = PDR2ME(pdr);

  int                cword = me->cword;
  int                junk;

    cword &=~ S_RUN;
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // Drop RUN
    cword &=~ S_STM; me->cword = cword;
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // Drop STM to allow A(0)F(25)
    cword |=  S_RUN;
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // Restore RUN
    
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 25, &junk);  // Trigger!

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  nadc502_privrec_t *me = PDR2ME(pdr);

  int                cword = me->cword &~ S_RUN;
  int                junk;

    DO_NAF(CAMAC_REF, me->N_DEV, 0, 24, &junk);  // Disable LAM
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // Drop RUN
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 10, &junk);  // Drop LAM
    
    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);

    return PZFRAME_R_READY;
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  nadc502_privrec_t *me = PDR2ME(pdr);

  int                cword = me->cword &~ S_RUN;
  int                junk;
  int                w;
  
  int                N_DEV = me->N_DEV;
  int                nc;
  NADC502_DATUM_T   *dp;
  int                n;
  int                base0;
  int                quant;
  enum              {COUNT_MAX = 100};
  int                count;
  int                x;
  int                rdata[COUNT_MAX];
  int                v;
  int32              min_v, max_v, sum_v;

  static int param2base0[3] = {2048, 1024, 3072};
  static int param2quant[4] = {39072, 19536, 9768, 4884};
    
    /* Stop the device */
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 24, &junk);  // Disable LAM
    DO_NAF(CAMAC_REF, me->N_DEV, 1, 17, &cword); // Drop RUN

    for (nc = 0;  nc < NADC502_NUM_LINES;  nc++)
    {
        w = me->pz.cur_args[NADC502_PARAM_NUMPTS];
        DO_NAF(CAMAC_REF, N_DEV, 0, 17, &w);  // Set read address

        base0 = param2base0[me->pz.cur_args[NADC502_PARAM_SHIFT1 + nc]];
        quant = param2quant[me->pz.cur_args[NADC502_PARAM_RANGE1 + nc]];

        dp = me->retdata + nc * me->pz.cur_args[NADC502_PARAM_NUMPTS];

        if (me->pz.cur_args[NADC502_PARAM_CALC_STATS] == 0)
        {
            for (n = me->pz.cur_args[NADC502_PARAM_NUMPTS];  n > 0;  n -= count)
            {
                count = n;
                if (count > COUNT_MAX) count = COUNT_MAX;
    
                DO_NAFB(CAMAC_REF, N_DEV, 0 + nc, 0, rdata, count);
                for (x = 0;  x < count;  x++)
                {
                    *dp++ = quant * ((rdata[x] & 0x0FFF) - base0) / 10;
                }
            }

            me->pz.cur_args[NADC502_PARAM_MIN1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_MIN1 + nc] =
            me->pz.cur_args[NADC502_PARAM_MAX1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_MAX1 + nc] =
            me->pz.cur_args[NADC502_PARAM_AVG1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_AVG1 + nc] =
            me->pz.cur_args[NADC502_PARAM_INT1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_INT1 + nc] = 0;
        }
        else
        {
            min_v = +99999; max_v = -99999; sum_v = 0;
            for (n = me->pz.cur_args[NADC502_PARAM_NUMPTS];  n > 0;  n -= count)
            {
                count = n;
                if (count > COUNT_MAX) count = COUNT_MAX;
    
                DO_NAFB(CAMAC_REF, N_DEV, 0 + nc, 0, rdata, count);
                for (x = 0;  x < count;  x++)
                {
                    v = (rdata[x] & 0x0FFF) - base0;
                    *dp++ = quant * v / 10;
                    if (min_v > v) min_v = v;
                    if (max_v < v) max_v = v;
                    sum_v += v;
                }
            }

            me->pz.cur_args[NADC502_PARAM_MIN1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_MIN1 + nc] = quant * min_v / 10;

            me->pz.cur_args[NADC502_PARAM_MAX1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_MAX1 + nc] = quant * max_v / 10;

            me->pz.cur_args[NADC502_PARAM_AVG1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_AVG1 + nc] = quant * (sum_v / me->pz.cur_args[NADC502_PARAM_NUMPTS]) / 10;

            me->pz.cur_args[NADC502_PARAM_INT1 + nc] =
            me->pz.nxt_args[NADC502_PARAM_INT1 + nc] = sum_v;
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////

enum
{
    PARAM_SHOT     = NADC502_PARAM_SHOT,
    PARAM_ISTART   = NADC502_PARAM_ISTART,
    PARAM_WAITTIME = NADC502_PARAM_WAITTIME,
    PARAM_STOP     = NADC502_PARAM_STOP,
    PARAM_ELAPSED  = NADC502_PARAM_ELAPSED,

    NUM_PARAMS     = NADC502_NUM_PARAMS,
};

#define FASTADC_NAME nadc502
#include "camac_fastadc_common.h"
