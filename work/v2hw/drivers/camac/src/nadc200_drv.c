#include "cxsd_driver.h"
#include "pzframe_drv.h"

#include "drv_i/nadc200_drv_i.h"


enum
{
    A_DATA1    = 0,
    A_DATA2    = 1,
    A_ADRPTR   = 0,
    A_BLKSTART = 1,
    A_BLKSIZE  = 2,
    A_STATUS   = 3,
    A_ZERO1    = 4,
    A_ZERO2    = 5,
};

enum
{
    S_RUN      = 1,
    S_DL       = 1 << 1,
    S_TMOD     = 1 << 2,
    S_LOOP     = 1 << 3,
    S_BLK      = 1 << 4,
    S_HLT      = 1 << 5,
    S_FDIV_SHIFT   = 6,
    S_FRQ200   = 1 << 8,
    S_FRQ195   = 1 << 9,
    S_FRQXTRN  = 1 << 10,
    S_SCALE1_SHIFT = 12,
    S_SCALE2_SHIFT = 14,
};

typedef struct
{
    pzframe_drv_t    pz;
    int              devid;
    int              N_DEV;

    NADC200_DATUM_T  retdata [NADC200_MAX_NUMPTS*NADC200_NUM_LINES];

    int              cword;
} nadc200_privrec_t;

#define PDR2ME(pdr) ((nadc200_privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

static psp_paramdescr_t nadc200_params[] =
{
    PSP_P_FLAG("calcstats", nadc200_privrec_t, pz.nxt_args[NADC200_PARAM_CALC_STATS], 1, 0),
    PSP_P_END()
};

static int32 saved_data_signature[] =
{
    0xFFFFFFFF,
    0x00000000,
    0x12345678,
    0x87654321,
};

static void WriteMem(nadc200_privrec_t *me,
                     int byte_offset, int32 *data, int count)
{
  int    w;
  int    n;
    
    DO_NAF(CAMAC_REF, me->N_DEV, 0,          10, &w);  // Drop LAM *first*
    w = S_RUN * 0 | S_FRQ200 * 1 | S_DL * 1;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS,   17, &w);  // Set clocks and no-run

    if (byte_offset < 0) byte_offset = 65535 * 4 + byte_offset;
    for (n = 0;  n < count;  n++)
    {
        // 1. Write low-16 to chan0
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        w = data[n] & 0x0000FFFF;
        DO_NAF(CAMAC_REF, me->N_DEV, 0,        16, &w);
        // 2. Write high-16 to chan1
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        w = (data[n] >> 16) & 0x0000FFFF;
        DO_NAF(CAMAC_REF, me->N_DEV, 1,        16, &w);
    }
}

static void ReadMem (nadc200_privrec_t *me,
                     int byte_offset, int32 *data, int count)
{
  int    w;
  int    n;

    bzero(data, sizeof(*data) * count);

    DO_NAF(CAMAC_REF, me->N_DEV, 0,          10, &w);  // Drop LAM *first*
    w = S_RUN * 0 | S_FRQ200 * 1 | S_DL * 1;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS,   17, &w);  // Set clocks and no-run

    if (byte_offset < 0) byte_offset = 65535 * 4 + byte_offset;
    for (n = 0;  n < count;  n++)
    {
        // 1. Read low-16 from chan0
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        DO_NAF(CAMAC_REF, me->N_DEV, 0,        0,  &w);
        data[n]  = w & 0x0000FFFF;
        // 2. Read high-16 from chan1
        w = n + byte_offset / 4;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w); // Set addr
        DO_NAF(CAMAC_REF, me->N_DEV, 1,        0,  &w);
        data[n] |= (w & 0x0000FFFF) << 16;
    }
}


static int32 ValidateParam(pzframe_drv_t *pdr, int n, int32 v)
{
    if      (n == NADC200_PARAM_TIMING)
    {
        if (v < NADC200_T_200MHZ) v = NADC200_T_200MHZ;
        if (v > NADC200_T_TIMER)  v = NADC200_T_TIMER;
    }
    else if (n == NADC200_PARAM_FRQDIV)
        v &= 3;
    else if (n == NADC200_PARAM_RANGE1  ||  n == NADC200_PARAM_RANGE2)
        v &= 3;
    else if (n == NADC200_PARAM_ZERO1   ||  n == NADC200_PARAM_ZERO2)
    {
        if (v < 0)   v = 0;
        if (v > 255) v = 255;
    }
    else if (n == NADC200_PARAM_PTSOFS)
    {
        v &=~ 1;
        if (v < 0)                    v = 0;
        if (v > NADC200_MAX_NUMPTS-2) v = NADC200_MAX_NUMPTS-2;
    }
    else if (n == NADC200_PARAM_NUMPTS)
    {
        v = (v + 1) &~ 1;
        if (v < 2)                    v = 2;
        if (v > NADC200_MAX_NUMPTS)   v = NADC200_MAX_NUMPTS;
    }

    return v;
}

static int   InitParams(pzframe_drv_t *pdr)
{
  nadc200_privrec_t *me = PDR2ME(pdr);

  int                n;
  int32              saved_data[NADC200_NUM_PARAMS + countof(saved_data_signature)];
    
    me->pz.nxt_args[NADC200_PARAM_NUMPTS] = 1024;
    me->pz.nxt_args[NADC200_PARAM_TIMING] = NADC200_T_200MHZ;
    me->pz.nxt_args[NADC200_PARAM_FRQDIV] = NADC200_FRQD_5NS;
    me->pz.nxt_args[NADC200_PARAM_RANGE1] = NADC200_R_2048;
    me->pz.nxt_args[NADC200_PARAM_RANGE2] = NADC200_R_2048;
    me->pz.nxt_args[NADC200_PARAM_ZERO1]  = 128;
    me->pz.nxt_args[NADC200_PARAM_ZERO2]  = 128;

    me->pz.retdataunits = NADC200_DATAUNITS;

    /* Try to restore previous settings */
    ReadMem(me, -sizeof(saved_data), saved_data, countof(saved_data));
#if 0
    for (n = 0;  n < countof(saved_data_signature); n++)
         DoDriverLog(me->devid, 0, "\t [%d]: 0x%08x, 0x%08x", n, saved_data[n], saved_data_signature[n]);
#endif
    if (memcmp(saved_data, saved_data_signature, sizeof(saved_data_signature)) == 0)
    {
        memcpy(me->pz.nxt_args,
               saved_data + countof(saved_data_signature),
               sizeof(me->pz.nxt_args[0]) * NADC200_NUM_PARAMS);
        for (n = 0;  n < NADC200_NUM_PARAMS;  n++)
            me->pz.nxt_args[n] = ValidateParam(pdr, n, me->pz.nxt_args[n]);
    }

    return DEVSTATE_OPERATING;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  nadc200_privrec_t *me = PDR2ME(pdr);

  int                statword;
  int                w;
  int                junk;
  
  static int timing2frq[] = {S_FRQ200, S_FRQ195, S_FRQXTRN};
    
    /* Additional check of PTSOFS against NUMPTS */
    if (me->pz.cur_args[NADC200_PARAM_PTSOFS] > NADC200_MAX_NUMPTS - me->pz.cur_args[NADC200_PARAM_NUMPTS])
        me->pz.cur_args[NADC200_PARAM_PTSOFS] = NADC200_MAX_NUMPTS - me->pz.cur_args[NADC200_PARAM_NUMPTS];

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[NADC200_PARAM_NUMPTS] *
                                         NADC200_NUM_LINES     *
                                         NADC200_DATAUNITS;
    
    /* Store current parameters */
    WriteMem(me,
             -sizeof(me->pz.cur_args[0]) * NADC200_NUM_PARAMS,
             me->pz.cur_args, NADC200_NUM_PARAMS);
    WriteMem(me,
             -sizeof(me->pz.cur_args[0]) * NADC200_NUM_PARAMS - sizeof(saved_data_signature),
             saved_data_signature, countof(saved_data_signature));

    /* Program the device */
    /* a. Prepare... */
    DO_NAF(CAMAC_REF, me->N_DEV, 0,          10, &junk);  // Drop LAM *first*
    statword = S_RUN * 0 | S_DL * 0 | S_TMOD * 1 | S_LOOP * 0 | S_BLK * 0 |
               S_HLT * 0 |
               (me->pz.cur_args[NADC200_PARAM_FRQDIV] << S_FDIV_SHIFT) |
               timing2frq[me->pz.cur_args[NADC200_PARAM_TIMING]] |
               (me->pz.cur_args[NADC200_PARAM_RANGE1] << S_SCALE1_SHIFT) |
               (me->pz.cur_args[NADC200_PARAM_RANGE2] << S_SCALE2_SHIFT);
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS,   17, &statword);
    /* b. Set parameters */
    w = 0;
    DO_NAF(CAMAC_REF, me->N_DEV, A_BLKSTART, 17, &w); // "Block start counter" := 0
    w = (me->pz.cur_args[NADC200_PARAM_NUMPTS] + 1 + me->pz.cur_args[NADC200_PARAM_PTSOFS]) / 2;
    //w = 2; /////
    DO_NAF(CAMAC_REF, me->N_DEV, A_BLKSIZE,  17, &w); // "Block size" := number + offset
    w = me->pz.cur_args[NADC200_PARAM_ZERO1];
    DO_NAF(CAMAC_REF, me->N_DEV, A_ZERO1,    17, &w); // Set chan1 zero
    w = me->pz.cur_args[NADC200_PARAM_ZERO2];
    DO_NAF(CAMAC_REF, me->N_DEV, A_ZERO2,    17, &w); // Set chan2 zero
    /* c. Drop LAM */
    DO_NAF(CAMAC_REF, me->N_DEV, 0,          10, &junk);
    /* d. Start */
    statword &=~ S_HLT;
    statword |=  S_RUN;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS,   17, &statword);

    me->cword = statword;

    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  nadc200_privrec_t *me = PDR2ME(pdr);

  int                junk;
    
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 25, &junk);

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  nadc200_privrec_t *me = PDR2ME(pdr);

  int                w;
  int                junk;
  
    w = (me->cword &~ S_RUN) | S_HLT;
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS, 17, &w);
    DO_NAF(CAMAC_REF, me->N_DEV, 0,        10, &junk);

    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);

    return PZFRAME_R_READY;
}


static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  nadc200_privrec_t *me = PDR2ME(pdr);

  int                numduplets;
  int                N_DEV = me->N_DEV;
  int                w;
  int                junk;
  
  int                nc;
  NADC200_DATUM_T   *dp;
  int                n;
  enum              {COUNT_MAX = 100};
  int                count;
  int                x;
  int                rdata[COUNT_MAX];
  int                v;
  int32              min_v, max_v, sum_v;

    /* Stop the device */
    w = (me->cword &~ S_RUN) | S_DL | S_HLT*0;
    DoDriverLog(me->devid, DRIVERLOG_DEBUG, "START cword=%x, READ cword=%x", me->cword, w);
    DO_NAF(CAMAC_REF, me->N_DEV, A_STATUS, 17, &w);
    DO_NAF(CAMAC_REF, me->N_DEV, 0,        10, &junk);

    /**/
    #if 0
    memset(me->retdata, 0x44, 8192); /////
            w = 0;
            DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w);
        for (n = 0; n < 6553;  n++)
        {
            w = n;
            //DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w);
            w = 0x55AA;//(n & 255)*0;// + 0xFFFF - ((n & 255)*256);
            DO_NAF(CAMAC_REF, me->N_DEV, 0, 16, &w);
        }
        DO_NAF(CAMAC_REF, me->N_DEV, 0, 1, &w);
        DoDriverLog(devid, 0, "ADDR_CTR=%d", w);
        w = 0;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w);
        w = 0x1234;
        DO_NAF(CAMAC_REF, me->N_DEV, 0, 16, &w);
        w = 0;
        DO_NAF(CAMAC_REF, me->N_DEV, A_ADRPTR, 17, &w);
        DO_NAF(CAMAC_REF, me->N_DEV, 0, 0,  &w);
        DoDriverLog(devid, 0, "rd=%x", w);
    #endif
    numduplets = (me->pz.cur_args[NADC200_PARAM_NUMPTS] + 1) / 2;
    for (nc = 0;  nc < NADC200_NUM_LINES;  nc++)
    {
        dp = me->retdata + nc * me->pz.cur_args[NADC200_PARAM_NUMPTS];
        w = me->pz.cur_args[NADC200_PARAM_PTSOFS] / 2;
        DO_NAF(CAMAC_REF, N_DEV, A_ADRPTR, 17, &w);  // Set read address

        if (me->pz.cur_args[NADC200_PARAM_CALC_STATS] == 0)
        {
            for (n = numduplets;  n > 0;  n -= count)
            {
                count = n;
                if (count > COUNT_MAX) count = COUNT_MAX;
    
                DO_NAFB(CAMAC_REF, N_DEV, A_DATA1 + nc, 0, rdata, count);
                for (x = 0;  x < count;  x++)
                {
                    w = rdata[x];
                    *dp++ =  w       & 0xFF;
                    *dp++ = (w >> 8) & 0xFF;
                }
            }

            me->pz.cur_args[NADC200_PARAM_MIN1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_MIN1 + nc] =
            me->pz.cur_args[NADC200_PARAM_MAX1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_MAX1 + nc] =
            me->pz.cur_args[NADC200_PARAM_AVG1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_AVG1 + nc] =
            me->pz.cur_args[NADC200_PARAM_INT1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_INT1 + nc] = 0;
        }
        else
        {
            min_v = NADC200_MAX_VALUE; max_v = NADC200_MIN_VALUE; sum_v = 0;
            for (n = numduplets;  n > 0;  n -= count)
            {
                count = n;
                if (count > COUNT_MAX) count = COUNT_MAX;
    
                DO_NAFB(CAMAC_REF, N_DEV, A_DATA1 + nc, 0, rdata, count);
                for (x = 0;  x < count;  x++)
                {
                    v =  rdata[x]       & 0xFF;
                    *dp++ = v;
                    if (min_v > v) min_v = v;
                    if (max_v < v) max_v = v;
                    sum_v += v;

                    v = (rdata[x] >> 8) & 0xFF;
                    *dp++ = v;
                    if (min_v > v) min_v = v;
                    if (max_v < v) max_v = v;
                    sum_v += v;
                }
            }

            me->pz.cur_args[NADC200_PARAM_MIN1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_MIN1 + nc] = min_v;

            me->pz.cur_args[NADC200_PARAM_MAX1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_MAX1 + nc] = max_v;

            me->pz.cur_args[NADC200_PARAM_AVG1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_AVG1 + nc] = sum_v / me->pz.cur_args[NADC200_PARAM_NUMPTS];

            me->pz.cur_args[NADC200_PARAM_INT1 + nc] =
            me->pz.nxt_args[NADC200_PARAM_INT1 + nc] = sum_v;
        }
    }
    ////memset(me->retdata, 0x44, 10); /////

    return 0;
}


enum
{
    PARAM_SHOT     = NADC200_PARAM_SHOT,
    PARAM_ISTART   = NADC200_PARAM_ISTART,
    PARAM_WAITTIME = NADC200_PARAM_WAITTIME,
    PARAM_STOP     = NADC200_PARAM_STOP,
    PARAM_ELAPSED  = NADC200_PARAM_ELAPSED,

    NUM_PARAMS     = NADC200_NUM_PARAMS,
};

#define FASTADC_NAME nadc200
#include "camac_fastadc_common.h"
