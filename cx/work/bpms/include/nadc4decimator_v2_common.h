#ifndef __NADC4_DECIMATOR_V2_COMMON_H
#define __NADC4_DECIMATOR_V2_COMMON_H

#include "drv_i/nadc4decimator_v2_drv_i.h"
#include "log_func.h"

#if CM5307_SL_DBODY
#define FAKE_DEVID 1
#endif

/* must be updated coherently with values in nadc4decimator_v2_drv_i.h */
static const char* dec_fact_c[] = {"none", "/3",  "/5",  "/9",  "/17", "/33"};
static const char* dec_duty_c[] = {"x1",   "x2",  "x3",  "x4",  "x5",  "x6",
                                   "x7",   "x8",  "x9",  "x10", "x11", "x12",
                                   "x13",  "x14", "x15", "x16"};

static const char* istart_c[]   = {"NOT internal", "internal"};

/* Check suitability of Decimation Factor */
static inline int DF_SanityCheck(int val)
{
    if ( val <  DF_0  ) return DF_0;
    if ( val >  DF_33 ) return DF_33;

    return val;
}

/* Check suitability of Decimation Scale */
static inline int DD_SanityCheck(int val)
{
    if ( val < DD_1  ) return DD_1;
    if ( val > DD_16 ) return DD_16;

    return val;
}

static inline void ValuesSanityCheck(int *df, int *dd)
{
    *df = DF_SanityCheck(*df);
    *dd = DD_SanityCheck(*dd);

    return;
}

/*
 * this 2 functions returns pure code. real values of Decimation Factor
 * and its Decimation Duty we keep in mind, according to conformation:
 *
 * decimation factor = 1/(1 + 2^(df_code + 1));
 * decimation duty   = 1/(1 +    dd_code     );
 *
 * full_code has 12 bits:
 *     0-2 bit -> df_code;
 *     6-9 bit -> dd_code;
 */
static inline int code2DF(const int code)
{
    return ( ( code >> 0 ) & 0x7 );
}

static inline int code2DD(const int code)
{
    return ( ( code >> 6 ) & 0xF );
}

static inline int val2code(const int *df, const int *dd)
{
    return (
            (( DF_SanityCheck(*df) & 0x7 ) << 0 )
            |
            (( DD_SanityCheck(*dd) & 0xF ) << 6 )
           );
}

static inline int code2val(const int code, int *df, int *dd)
{
    if ( df != NULL && dd != NULL )
    {
        *df = code2DF(code);
        *dd = code2DD(code);
        return 1;
    }
    return 0;
}

static int cmd2f(const int istart, const int cmd)
{
    int f;

    if      (istart == 0)
    {
        /* Start Device by external signal */
        switch (cmd)
        {
            case DF_0:
                f = 24;
                break;
            default:
                f = 25;
                break;
        }
    }
    else if (istart == 1)
    {
        /* Start Device in internal mode   */
        switch (cmd)
        {
            case DF_0:
                f = 30;
                break;
            default:
                f = 31;
                break;
        }
    }
    else
    {
        /* Default */
        f = 10;
    }

    MDoDriverLog(FAKE_DEVID, 0, "%s: f=%d -> (%s), %s %s",
                 __FUNCTION__, f, istart_c[istart], cmd == DF_0 ? "without" : "   with", "DECIMATION");
    return f;
}

static inline int chan2subadr(const int chan)
{
  int subadr;

    switch (chan)
    {
        case 0 ... 3:
            subadr = 1 << chan;
            break;
        default:
            subadr = 0xf;
            break;
    }

    return subadr;
}

static inline float val2mult(const int *df, const int *dd)
{
    int mult = 1, df_mult, dd_mult;
    //    wait_mksek = 2700 * val2mul(&me->val_cache[DEC_FACT_CH], &me->val_cache[DEC_DUTY_CH]);
    if ( df == NULL || dd == NULL ) return mult;

    switch ( *df )
    {
        default:
        case DF_0:  df_mult = 1;  break;
        case DF_3:  df_mult = 3;  break;
        case DF_5:  df_mult = 5;  break;
        case DF_9:  df_mult = 9;  break;
        case DF_17: df_mult = 17; break;
        case DF_33: df_mult = 33; break;
    }

    dd_mult = *dd + 1;

    mult = df_mult * dd_mult;

    return mult;
}

static inline int WriteValues(int devid, int N,
                              int *df, int *dd,
                              rflags_t *rflags)
{
    int code;
    rflags_t l_rflags;

    if ( df == NULL || dd == NULL ) return 0;

    /* Code and write data */
    MDoDriverLog(devid, 0, "%s: AAAAAAAAAAAAAAAAAA: %d", __FUNCTION__, *df);
    code = val2code(df, dd);
    MDoDriverLog(devid, 0, "%s: AAAAAAAAAAAAAAAAAA: %d", __FUNCTION__, *df);

    /* Write code */
    l_rflags = status2rflags(DO_NAF(CAMAC_REF, N, 0, 16, &code));
    if ( rflags != NULL ) *rflags = l_rflags;

    MDoDriverLog(devid, 0, "%s: writed code = 0x%x", __FUNCTION__, code);
    return 1;
}

static inline int ReadValues(int devid, int N,
                             int *df, int *dd,
                             rflags_t *rflags)
{
    int code;
    rflags_t l_rflags;

    if ( df == NULL || dd == NULL ) return 0;

    /* Read code  */
    l_rflags = status2rflags(DO_NAF(CAMAC_REF, N, 0,  0, &code));
    if ( rflags != NULL ) *rflags = l_rflags;

    /* Decode and return data */
    code2val(code, df, dd);

    MDoDriverLog(devid, 0, "%s: read code = 0x%x", __FUNCTION__, code);
    return 1;
}

#if 0
static inline int WriteValuesS(int devid, void *privptr)
{
    privrec_t *me = (privrec_t*)privptr;

    return WriteValues(devid, me->N_DEV,
                       &(me->val_cache[DEC_FACT_CH]), &(me->val_cache[DEC_FACT_CH]),
                       NULL);
}

static inline int ReadValuesS(int devid, void *privptr)
{
    privrec_t *me = (privrec_t*)privptr;

    return ReadValues(devid, me->N_DEV,
                      &(me->val_cache[DEC_FACT_CH]), &(me->val_cache[DEC_FACT_CH]),
                      NULL);
}
#endif

static inline int ProgramStart(int devid, int N,
                               int chan, int istart, int cmd,
                               rflags_t *rflags)
{
    int subadr, f, junk;
    rflags_t l_rflags;

    subadr = chan2subadr(chan);
    f      = cmd2f(istart, cmd);

    /* Real Decimator start */
    l_rflags = status2rflags(DO_NAF(CAMAC_REF, N, subadr, f, &junk));
    if ( rflags != NULL ) *rflags = l_rflags;

    MDoDriverLog(devid, 0, "%s: Program Start: ch =%d, f=%d", __FUNCTION__, chan, f);
    return 1;
}

static inline int ProgramStop(int devid, int N,
                              rflags_t *rflags)
{
    int junk;
    rflags_t l_rflags;

    /* Real Decimator stop */
    l_rflags = status2rflags(DO_NAF(CAMAC_REF, N, A_ANY, 10, &junk));
    if ( rflags != NULL ) *rflags = l_rflags;

    MDoDriverLog(devid, 0, "%s: Program Stop:", __FUNCTION__);
    return 1;
}

#endif /* __NADC4_DECIMATOR_V2_COMMON_H */
