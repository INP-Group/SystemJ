#ifndef __NADC502_DRV_I_H
#define __NADC502_DRV_I_H


#include "misc_types.h"


enum
{
    /* 1. writable parameters, range = [0, 50)                    */
    /* a. [00-10), standard controls, common for all drivers      */
    NADC502_PARAM_SHOT       = 0,
    NADC502_PARAM_STOP       ,
    NADC502_PARAM_ISTART     ,
    NADC502_PARAM_WAITTIME   ,
    NADC502_PARAM_CALC_STATS , 
    
    /* b. [10-20), fastadc-specific controls, common for adc's    */
    NADC502_PARAM_PTSOFS     = 10,
    NADC502_PARAM_NUMPTS     ,

    /* c. [20-30), device-specific controls, such as timings,     */
    /*             frqdivision i.e. common for all lines          */
    NADC502_PARAM_TIMING     = 20,
    NADC502_PARAM_TMODE      ,

    /* d. [30-40), auxilary controls ...                          */
    NADC502_PARAM_AUXFAKE    = 30, /* fake: just for range mark   */


    /* e. [40-60), per-line configuration, i.e. individual delay, */
    /*             individual zero, individual gain measurements  */
    /*             range, zero shift and so on                    */
    NADC502_PARAM_RANGE1     = 40,
    NADC502_PARAM_RANGE2     ,
    NADC502_PARAM_SHIFT1     ,
    NADC502_PARAM_SHIFT2     ,





    /* 2. readable parameters, range = [60, 100)                  */
    /* a. only statistics avaliable at the moment                 */
    NADC502_PARAM_MIN1       = 60,
    NADC502_PARAM_MIN2       = 61,
    NADC502_PARAM_MAX1       = 62,
    NADC502_PARAM_MAX2       = 63,
    NADC502_PARAM_AVG1       = 64,
    NADC502_PARAM_AVG2       = 65,
    NADC502_PARAM_INT1       = 66,
    NADC502_PARAM_INT2       = 67,

    NADC502_PARAM_STATUS     = 98,
    NADC502_PARAM_ELAPSED    = 99,

    NADC502_NUM_PARAMS       = 100,
};


/* Timings */
enum
{
    NADC502_TIMING_INT = 0,
    NADC502_TIMING_EXT = 1,
};

enum
{
    NADC502_TMODE_HF = 0,
    NADC502_TMODE_LF = 1,
};

/* Ranges */
enum
{
    NADC502_RANGE_8192  = 0,
    NADC502_RANGE_4096  = 1,
    NADC502_RANGE_2048  = 2,
    NADC502_RANGE_1024  = 3,
};

enum
{
    NADC502_SHIFT_NONE = 0,
    NADC502_SHIFT_NEG4 = 1,
    NADC502_SHIFT_POS4 = 2,
};

/* General device specs */
typedef int32 NADC502_DATUM_T;
enum
{
    NADC502_MAX_NUMPTS = 65534*0+32767, // "6553*4*" -- because 65535=0xFFFF, which is forbidden for write into addr-ctr
    NADC502_NUM_LINES  = 2,
    NADC502_DATAUNITS  = sizeof(NADC502_DATUM_T),
};
enum
{
    NADC502_MIN_VALUE= -12002918, // ((    0 - 3072)*39072)/10
    NADC502_MAX_VALUE= +11999011, // ((0xFFF - 1024)*39072)/10
};


#endif /* __NADC502_DRV_I_H */
