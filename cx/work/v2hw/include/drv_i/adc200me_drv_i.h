#ifndef __ADC200ME_DRV_I_H
#define __ADC200ME_DRV_I_H


#include "misc_types.h"


enum
{
    ADC200ME_PARAM_SHOT         = 0,
    ADC200ME_PARAM_ISTART       = 1,
    ADC200ME_PARAM_WAITTIME     = 2,
    ADC200ME_PARAM_STOP         = 3,
    ADC200ME_PARAM_PTSOFS       = 4,
    ADC200ME_PARAM_NUMPTS       = 5,
    
    ADC200ME_PARAM_TIMING       = 6,
    ADC200ME_PARAM_RANGE1       = 7,
    ADC200ME_PARAM_RANGE2       = 8,
    ADC200ME_PARAM_CALIBRATE    = 9,
    ADC200ME_PARAM_FGT_CLB      = 10,
    ADC200ME_PARAM_VISIBLE_CLB  = 11,

    ADC200ME_PARAM_CLB_OFS1     = 12,
    ADC200ME_PARAM_CLB_OFS2     = 13,
    ADC200ME_PARAM_CLB_COR1     = 14,
    ADC200ME_PARAM_CLB_COR2     = 15,

    ADC200ME_PARAM_ELAPSED      = 17,
    ADC200ME_PARAM_CLB_STATE    = 18,
    ADC200ME_PARAM_SERIAL       = 19,

    ADC200ME_NUM_PARAMS         = 20
};


/* Timings */
enum
{
    ADC200ME_T_200MHZ = 0,
    ADC200ME_T_EXT    = 1,
};

/**/
enum
{
    ADC200ME_R_4V  = 0,
    ADC200ME_R_2V  = 1,
    ADC200ME_R_1V  = 2,
    ADC200ME_R_05V = 3,
};

/*  */
enum
{
    ADC200ME_CLB_COR_R = 3900
};


/* General device specs */
typedef int32 ADC200ME_DATUM_T;
enum
{
    ADC200ME_MAX_NUMPTS = 1048575,
    ADC200ME_NUM_LINES  = 2,
    ADC200ME_DATAUNITS  = sizeof(ADC200ME_DATUM_T),
};
enum
{
    ADC200ME_MIN_VALUE = -4096000, // (    0-2048)*2000
    ADC200ME_MAX_VALUE = +4094000, // (0xFFF-2048)*2000
};


#endif /* __ADC200ME_DRV_I_H */
