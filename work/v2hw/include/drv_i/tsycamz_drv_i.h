#ifndef __TSYCAMZ_DRV_I_H
#define __TSYCAMZ_DRV_I_H


#include "misc_types.h"


enum
{
    TSYCAMZ_PARAM_SHOT       = 0,
    TSYCAMZ_PARAM_ISTART     = 1,
    TSYCAMZ_PARAM_WAITTIME   = 2,
    TSYCAMZ_PARAM_STOP       = 3,

    TSYCAMZ_PARAM_X          = 4,
    TSYCAMZ_PARAM_Y          = 5,
    TSYCAMZ_PARAM_W          = 6,
    TSYCAMZ_PARAM_H          = 7,
    TSYCAMZ_PARAM_K          = 8,
    TSYCAMZ_PARAM_T          = 9,
    TSYCAMZ_PARAM_SYNC       = 10,
    TSYCAMZ_PARAM_RRQ_MSECS  = 11,

    TSYCAMZ_PARAM_MISS       = 12,

    TSYCAMZ_PARAM_ELAPSED    = 19,
    TSYCAMZ_NUM_PARAMS       = 20
};


/* General device specs */
typedef int16 TSYCAMZ_DATUM_T;
enum
{
    TSYCAMZ_MAX_W      = 660,
    TSYCAMZ_MAX_H      = 500,
    TSYCAMZ_DATAUNITS  = sizeof(TSYCAMZ_DATUM_T),

    TSYCAMZ_SRCMAXVAL = 1023,
    TSYCAMZ_BPP       = 10,
};

#endif /* __TSYCAMZ_DRV_I_H */
