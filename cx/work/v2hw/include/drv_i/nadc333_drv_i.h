#ifndef __NADC333_DRV_I_H
#define __NADC333_DRV_I_H


#include "misc_types.h"


enum
{
    /* 1. writable parameters, range = [0, 50)                    */
    /* a. [00-10), standard controls, common for all drivers      */
    NADC333_PARAM_SHOT       = 0,
    NADC333_PARAM_STOP       ,
    NADC333_PARAM_ISTART     ,
    NADC333_PARAM_WAITTIME   ,
    NADC333_PARAM_CALC_STATS ,

    /* b. [10-20), fastadc-specific controls, common for adc's    */
    NADC333_PARAM_PTSOFS     = 10,
    NADC333_PARAM_NUMPTS     ,

    /* c. [20-30), device-specific controls, such as timings,     */
    /*             frqdivision i.e. common for all lines          */
    NADC333_PARAM_TIMING     = 20,


    /* d. [30-40), auxilary controls ...                          */
    NADC333_PARAM_AUXFAKE    = 30, /* fake: just for range mark   */


    /* e. [40-60), per-line configuration, i.e. individual delay, */
    /*             individual zero, individual gain measurements  */
    /*             range, zero shift and so on                    */
    NADC333_PARAM_RANGE0     = 40,
    NADC333_PARAM_RANGE1     ,
    NADC333_PARAM_RANGE2     ,
    NADC333_PARAM_RANGE3     ,





    /* 2. readable parameters, range = [60, 100)                  */
    /* a. only statistics avaliable at the moment                 */
    NADC333_PARAM_STATUS     = 98,
    NADC333_PARAM_ELAPSED    = 99,

    NADC333_NUM_PARAMS       = 100,
};


/* Timings */
enum
{
    NADC333_T_500NS = 0,
    NADC333_T_1US   = 1,
    NADC333_T_2US   = 2,
    NADC333_T_4US   = 3,
    NADC333_T_8US   = 4,
    NADC333_T_16US  = 5,
    NADC333_T_32US  = 6,
    NADC333_T_EXT   = 7,
};

/* Ranges */
enum
{
    NADC333_R_8192  = 0,
    NADC333_R_4096  = 1,
    NADC333_R_2048  = 2,
    NADC333_R_1024  = 3,
    NADC333_R_OFF   = 4
};

/* General device specs */
typedef int32 NADC333_DATUM_T;
enum
{
    NADC333_MAX_NUMPTS = 65535,
    NADC333_NUM_LINES  = 4,
    NADC333_DATAUNITS  = sizeof(NADC333_DATUM_T),
};
enum
{
    NADC333_MIN_VALUE = -8192000, // (    0-2048) * (8192000/2048)
    NADC333_MAX_VALUE = +8188000, // (0xFFF-2048) * (8192000/2048)
};


#endif /* __NADC333_DRV_I_H */
