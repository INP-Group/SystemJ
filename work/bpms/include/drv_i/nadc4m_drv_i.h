#ifndef __NADC4_DRV_I_H
#define __NADC4_DRV_I_H


#include "misc_types.h"


enum
{
    /* 1. writable parameters, range = [0, 50)                    */
    /* a. [00-10), standard controls, common for all drivers      */
    NADC4_PARAM_SHOT       = 0,
    NADC4_PARAM_STOP       ,
    NADC4_PARAM_ISTART     ,
    NADC4_PARAM_WAITTIME   ,
    NADC4_PARAM_CALC_STATS ,

    /* b. [10-20), fastadc-specific controls, common for adc's    */
    NADC4_PARAM_PTSOFS     = 10,
    NADC4_PARAM_NUMPTS     ,

    /* c. [20-30), device-specific controls, such as timings,     */
    /*             frqdivision i.e. common for all lines          */
    NADC4_PARAM_TIMING     = 20, /* fake: just for range mark     */


    /* d. [30-40), auxilary controls ...                          */
    NADC4_PARAM_DELAY      = 30,
    NADC4_PARAM_ATTEN      ,
    NADC4_PARAM_DECMTN     ,
    /* e. [40-60), per-line configuration, i.e. individual delay, */
    /*             individual zero, individual gain measurements  */
    /*             range, zero shift and so on                    */
    NADC4_PARAM_RANGE0     = 40,
    NADC4_PARAM_RANGE1     ,
    NADC4_PARAM_RANGE2     ,
    NADC4_PARAM_RANGE3     ,
    NADC4_PARAM_ZERO0      = 44,
    NADC4_PARAM_ZERO1      ,
    NADC4_PARAM_ZERO2      ,
    NADC4_PARAM_ZERO3      ,

    /* 2. readable parameters, range = [60, 100)                  */
    /* a. only statistics avaliable at the moment                 */
    NADC4_PARAM_MIN0       = 60,
    NADC4_PARAM_MAX0       = 64,
    NADC4_PARAM_AVG0       = 68,
    NADC4_PARAM_INT0       = 72,

    NADC4_PARAM_STATUS     = 98,
    NADC4_PARAM_ELAPSED    = 99,

    NADC4_NUM_PARAMS       = 100,
};


/* General device specs */
typedef int32 NADC4_DATUM_T;
enum
{
    NADC4_JUNK_MSMTS = 6,
    NADC4_MAX_NUMPTS = 8192 - NADC4_JUNK_MSMTS, // 32K words per channel, minus 6 words of junk
    NADC4_NUM_LINES  = 4,
    NADC4_DATAUNITS  = sizeof(NADC4_DATUM_T),
};
enum
{
    NADC4_MIN_VALUE = -2048,
    NADC4_MAX_VALUE = +2047,
};


#endif /* __NADC4_DRV_I_H */
