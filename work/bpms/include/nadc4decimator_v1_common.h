#ifndef __NADC4_DECIMATOR_V1_COMMON_H
#define __NADC4_DECIMATOR_V1_COMMON_H

#include "drv_i/nadc4decimator_v1_drv_i.h"
#include "log_func.h"

#if CM5307_SL_DBODY
#define FAKE_DEVID 1
#endif

enum
{
    MIN_DECFACT_CODE = 0,
    MAX_DECFACT_CODE = 15,

    MIN_DECFACT_VAL  = 64*(MIN_DECFACT_CODE+1),
    MAX_DECFACT_VAL  = 64*(MAX_DECFACT_CODE+1),


    MIN_DECDUTY_CODE = 0,
    MAX_DECDUTY_CODE = 15,

    MIN_DECDUTY_VAL  = 16*(MIN_DECDUTY_CODE+1),
    MAX_DECDUTY_VAL  = 16*(MAX_DECDUTY_CODE+1),
};

/* must be updated coherently with values in nadc4decimator_v1_drv_i.h */
static const char* dec_fact_c[] = {"none", "arbitrary"};
static const char* dec_duty_c[] = {"none", "arbitrary"};

static const char* istart_c[]   = {"NOT internal", "internal"};

/* Check suitability of Decimation Factor */
static inline int DF_SanityCheck(int val)
{
    if ( val <= DF_0            ) return DF_0;
//    if ( val < MIN_DECFACT_CODE ) return MIN_DECFACT_CODE;
    if ( val > MAX_DECFACT_CODE ) return MAX_DECFACT_CODE;

    return val;
}

/* Check suitability of Decimation Scale */
static inline int DD_SanityCheck(int val)
{
    if ( val < MIN_DECDUTY_CODE ) return MIN_DECDUTY_CODE;
    if ( val > MAX_DECDUTY_CODE ) return MAX_DECDUTY_CODE;

    return val;
}

static inline void ValuesSanityCheck(int *df, int *dd)
{
    int v1, v2;

    v1 = DF_SanityCheck(*df);
    v2 = DD_SanityCheck(*dd);

    /* check if the decimation factor > decimation scale */
#if 0
    if   ( 64*(v1+1) <= 16*(v2+1) )
    {
        *df = *dd = 0;
    }
    else
#endif
    {
        *df = v1;
        *dd = v2;
    }
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
 *     0-3 bit -> df_code;
 *     4-7 bit -> dd_code;
 */
static inline int code2DF(const int code)
{
    return ( ( code >> 4 ) & 0xF );
}

static inline int code2DD(const int code)
{
    return ( ( code >> 0 ) & 0xF );
}

static inline int val2code(const int *df, const int *dd)
{
    return (
            (( DF_SanityCheck(*df) << 4 ) & 0xF0)
            |
            (( DD_SanityCheck(*dd) << 0 ) & 0x0F)
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

    switch (cmd)
    {
        case DF_0: /* without decimation at all */
            f = 25;
            break;
        default:   /* decimation */
            f = 24;
            break;
    }

    MDoDriverLog(FAKE_DEVID, 0, "%s: f=%d -> (%s), %s %s",
                 __FUNCTION__, f, istart_c[istart], cmd == DF_0 ? "without" : "with", "DECIMATION");
    return f;
}

static int val2mult(int *df, int *dd)
{
    int mult = 1;

    if ( df == NULL || dd == NULL || *df == DF_0 ) return mult;

    mult = (64*(*df+1))/(16*(*dd+1));

    return mult;
}

static inline int WriteValues(int devid, int N,
                              int *df, int *dd,
                              rflags_t *rflags)
{
    int      i, code, tmp_code = 0, code_check = 0, junk, qx;
    rflags_t l_rflags = 0;

    if ( df == NULL || dd == NULL || rflags == NULL ) return 0;

    /* Code and write data */
    MDoDriverLog(devid, 0, "%s: %d AAAAAAAAAAAAAAAAAA: %d | %d, 0x%x", __FUNCTION__, N, *df, *dd, tmp_code);
    code = val2code(df, dd);
AGAIN:
    tmp_code = code;
    MDoDriverLog(devid, 0, "%s: %d AAAAAAAAAAAAAAAAAA: %d | %d, 0x%x", __FUNCTION__, N, *df, *dd, tmp_code);
#if 1
    /* Write code */

    for (i = 0; i < 8; i++)
    {
        if ( (tmp_code & 0x1) == 0 )
        {
            l_rflags  |= status2rflags((qx = DO_NAF(CAMAC_REF, N, 0, 28, &junk)));
            code_check = code_check | ( 0 << i );
        }
        else
        {
            l_rflags  |= status2rflags((qx = DO_NAF(CAMAC_REF, N, 0, 29, &junk)));
            code_check = code_check | ( 1 << i );
        }

//        enum {CAMAC_X = 1, CAMAC_Q = 2};

        if ( !(qx & CAMAC_X) )
        {
            MDoDriverLog(devid, 0, "%s: Error: bit = %d, qx = %d", __FUNCTION__, i, qx);
            goto AGAIN;
        }

        /* they was "code >> 1" originally, changed 07.12.2010 */
        tmp_code = tmp_code >> 1;
    }
#endif

    if ( rflags ) *rflags = l_rflags;
    MDoDriverLog(devid, 0, "%s: writed code = 0x%x, check = 0x%x, rflags = 0x%x", __FUNCTION__, code, code_check, l_rflags);
    return 1;
}

static inline int ReadValues(int devid, int N,
                             int *df, int *dd,
                             rflags_t *rflags)
{
    MDoDriverLog(devid, 0, "%s: read DECIMATION code is UNSUPPORTED feature", __FUNCTION__);

    if ( df == NULL || dd == NULL || rflags == NULL ) return 0;

    *rflags = CXRF_UNSUPPORTED;

    return 0;
}

#endif /* __NADC4_DECIMATOR_V1_COMMON_H */
