#ifndef __S2V_DRV_I_H
#define __S2V_DRV_I_H


#include "misc_types.h"


// w60,r40 1*100+0i/100+100000i
enum
{
    /* 1. writable parameters, range = [0, 50)                    */
    /* a. [00-10), standard controls, common for all drivers      */
    S2V_PARAM_SHOT       = 0,
    S2V_PARAM_STOP       ,
    S2V_PARAM_ISTART     ,
    S2V_PARAM_WAITTIME   ,
    S2V_PARAM_CALC_STATS , 
    
    /* b. [10-20), fastadc-specific controls, common for adc's    */
    S2V_PARAM_PTSOFS     = 10,
    S2V_PARAM_NUMPTS     ,

    /* 2. readable parameters, range = [60, 100)                  */
    /* a. only statistics avaliable at the moment                 */
    S2V_PARAM_MIN        = 60,
    S2V_PARAM_MAX        = 61,
    S2V_PARAM_AVG        = 62,
    S2V_PARAM_INT        = 63,

    S2V_PARAM_STATUS     = 98,
    S2V_PARAM_ELAPSED    = 99,

    S2V_NUM_PARAMS       = 100,
};

/* General device specs */
typedef int32 S2V_DATUM_T;
enum
{
    S2V_MAX_NUMPTS = 100000,
    S2V_NUM_LINES  = 1,
    S2V_DATAUNITS  = sizeof(S2V_DATUM_T),
};
enum
{
    S2V_MIN_VALUE= -10000000,
    S2V_MAX_VALUE= +10000000,
};


#endif /* __S2V_DRV_I_H */
