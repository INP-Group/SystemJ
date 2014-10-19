#ifndef __BPMD_DRV_I_H
#define __BPMD_DRV_I_H


#include "misc_types.h"


enum
{
    /* 1. writable parameters, range = [0, 50)                    */
    /* a. [00-10), standard controls, common for all drivers      */
    BPMD_PARAM_SHOT       = 0,
    BPMD_PARAM_STOP       ,
    BPMD_PARAM_ISTART     ,
    BPMD_PARAM_WAITTIME   ,
    BPMD_PARAM_CALC_STATS ,

    /* b. [10-20), fastadc-specific controls, common for adc's    */
    BPMD_PARAM_PTSOFS     = 10,
    BPMD_PARAM_NUMPTS     ,

    /* c. [20-30), device-specific controls, such as timings,     */
    /*             frqdivision i.e. common for all lines          */
    BPMD_PARAM_TIMING     = 20, /* fake: just for range mark      */


    /* d. [30-40), auxilary controls ...                          */
    BPMD_PARAM_DELAY      = 30,
    BPMD_PARAM_ATTEN      ,
    BPMD_PARAM_DECMTN     ,

    /* e. [40-60), per-line configuration, i.e. individual delay, */
    /*             individual zero, individual gain measurements  */
    /*             range, zero shift and so on                    */
    BPMD_PARAM_RANGE0     = 40,
    BPMD_PARAM_RANGE1     ,
    BPMD_PARAM_RANGE2     ,
    BPMD_PARAM_RANGE3     ,

    BPMD_PARAM_ZERO0      = 44,
    BPMD_PARAM_ZERO1      ,
    BPMD_PARAM_ZERO2      ,
    BPMD_PARAM_ZERO3      ,

    BPMD_PARAM_DLAY0      = 48,
    BPMD_PARAM_DLAY1      ,
    BPMD_PARAM_DLAY2      ,
    BPMD_PARAM_DLAY3      ,

    BPMD_PARAM_MGDLY      = 52,

    /* 2. readable parameters, range = [60, 100)                  */
    /* a. only statistics avaliable at the moment                 */
    BPMD_PARAM_MIN0       = 60,
    BPMD_PARAM_MIN1       ,
    BPMD_PARAM_MIN2       ,
    BPMD_PARAM_MIN3       ,

    BPMD_PARAM_MAX0       = 64,
    BPMD_PARAM_MAX1       ,
    BPMD_PARAM_MAX2       ,
    BPMD_PARAM_MAX3       ,

    BPMD_PARAM_AVG0       = 68,
    BPMD_PARAM_AVG1       ,
    BPMD_PARAM_AVG2       ,
    BPMD_PARAM_AVG3       ,

    BPMD_PARAM_INT0       = 72,
    BPMD_PARAM_INT1       ,
    BPMD_PARAM_INT2       ,
    BPMD_PARAM_INT3       ,

    BPMD_PARAM_STATUS     = 98,
    BPMD_PARAM_ELAPSED    = 99,

    BPMD_NUM_PARAMS       = 100,
};

/* General device specs */
typedef int32 BPMD_DATUM_T;
enum
{
    BPMD_JUNK_MSMTS = 3,
    BPMD_MAX_NUMPTS = 8192 - BPMD_JUNK_MSMTS,
    BPMD_NUM_LINES  = 4,
    BPMD_DATAUNITS  = sizeof(BPMD_DATUM_T),
};

enum
{
    BPMD_MIN_VALUE = -8192,
    BPMD_MAX_VALUE = +8191,
};

#endif /* __BPMD_DRV_I_H */
