#ifndef __OTTCAM_DRV_I_H
#define __OTTCAM_DRV_I_H


#include "misc_types.h"


enum
{
    OTTCAM_PARAM_SHOT       = 0,
    OTTCAM_PARAM_ISTART     = 1,
    OTTCAM_PARAM_WAITTIME   = 2,
    OTTCAM_PARAM_STOP       = 3,

    OTTCAM_PARAM_X          = 4,
    OTTCAM_PARAM_Y          = 5,
    OTTCAM_PARAM_W          = 6,
    OTTCAM_PARAM_H          = 7,
    OTTCAM_PARAM_K          = 8,
    OTTCAM_PARAM_T          = 9,
    OTTCAM_PARAM_SYNC       = 10,
    OTTCAM_PARAM_RRQ_MSECS  = 11,

    OTTCAM_PARAM_MISS       = 12,

    OTTCAM_PARAM_ELAPSED    = 19,
    OTTCAM_NUM_PARAMS       = 20
};


/* General device specs */
typedef int16 OTTCAM_DATUM_T;
enum
{
    OTTCAM_MAX_W      = 753,
    OTTCAM_MAX_H      = 480,
    OTTCAM_DATAUNITS  = sizeof(OTTCAM_DATUM_T),

    OTTCAM_SRCMAXVAL = 1022,
    OTTCAM_BPP       = 10,
};

#endif /* __OTTCAM_DRV_I_H */
