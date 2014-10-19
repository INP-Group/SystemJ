#ifndef __OTTCAMV_DRV_I_H
#define __OTTCAMV_DRV_I_H


enum
{
    OTTCAMV_PARAM_K             = 0,
    OTTCAMV_PARAM_T             = 1,
    OTTCAMV_PARAM_SYNC          = 2,
    OTTCAMV_PARAM_MISS          = 3,
    OTTCAMV_PARAM_W             = 4,
    OTTCAMV_PARAM_H             = 5,
    OTTCAMV_PARAM_WAITTIME      = 6, // in ms, NOT us
    OTTCAMV_PARAM_WAITTIME_ACKD = 7, // in ms, NOT us
    OTTCAMV_PARAM_STOP          = 8,

    OTTCAMV_NUM_PARAMS          = 9,
};

enum
{
    OTTCAMV_BIGC_RAW   = 0,
    OTTCAMV_BIGC_16BIT = 1,
    OTTCAMV_BIGC_32BIT = 2,
    
    OTTCAMV_NUM_BIGCS  = 3
};

enum
{
    OTTCAMV_NUMCHANS = OTTCAMV_NUM_PARAMS
};

/* General device specs */
typedef uint16 OTTCAMV_CH1_DATUM_T;
typedef uint32 OTTCAMV_CH2_DATUM_T;
enum
{
    OTTCAMV_WIDTH         = 753,
    OTTCAMV_HEIGHT        = 480,
    OTTCAMV_BPP           = 10,
    OTTCAMV_CH1_DATAUNITS = sizeof(OTTCAMV_CH1_DATUM_T),
    OTTCAMV_CH2_DATAUNITS = sizeof(OTTCAMV_CH2_DATUM_T),

    OTTCAMV_SRCMAXVAL     = 1022,
};


#endif /* __OTTCAMV_DRV_I_H */
