#ifndef __OTTCAMX_DRV_I_H
#define __OTTCAMX_DRV_I_H


#include "misc_types.h"


enum
{
    OTTCAMX_PARAM_SHOT       = 0,
    OTTCAMX_PARAM_ISTART     = 1,
    OTTCAMX_PARAM_WAITTIME   = 2,
    OTTCAMX_PARAM_STOP       = 3,

    OTTCAMX_PARAM_X          = 4,
    OTTCAMX_PARAM_Y          = 5,
    OTTCAMX_PARAM_W          = 6,
    OTTCAMX_PARAM_H          = 7,
    OTTCAMX_PARAM_K          = 8,
    OTTCAMX_PARAM_T          = 9,
    OTTCAMX_PARAM_SYNC       = 10,
    OTTCAMX_PARAM_RRQ_MSECS  = 11,

    OTTCAMX_PARAM_MISS       = 12,

    OTTCAMX_PARAM_ELAPSED    = 19,
    OTTCAMX_NUM_PARAMS       = 20
};


/* General device specs */
typedef int16 OTTCAMX_DATUM_T;
enum
{
    OTTCAMX_MAX_W      = 753,
    OTTCAMX_MAX_H      = 480,
    OTTCAMX_DATAUNITS  = sizeof(OTTCAMX_DATUM_T),

    OTTCAMX_SRCMAXVAL = 1022,
    OTTCAMX_BPP       = 10,
};

#endif /* __OTTCAMX_DRV_I_H */
