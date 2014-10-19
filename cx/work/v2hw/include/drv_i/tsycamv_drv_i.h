#ifndef __TSYCAMV_DRV_I_H
#define __TSYCAMV_DRV_I_H


enum
{
    TSYCAMV_PARAM_K             = 0,
    TSYCAMV_PARAM_T             = 1,
    TSYCAMV_PARAM_SYNC          = 2,
    TSYCAMV_PARAM_MISS          = 3,
    TSYCAMV_PARAM_W             = 4,
    TSYCAMV_PARAM_H             = 5,
    TSYCAMV_PARAM_WAITTIME      = 6, // in ms, NOT us
    TSYCAMV_PARAM_WAITTIME_ACKD = 7, // in ms, NOT us
    TSYCAMV_PARAM_STOP          = 8,

    TSYCAMV_NUM_PARAMS          = 9,
};

enum
{
    TSYCAMV_BIGC_RAW   = 0,
    TSYCAMV_BIGC_16BIT = 1,
    TSYCAMV_BIGC_32BIT = 2,
    
    TSYCAMV_NUM_BIGCS  = 3
};

enum
{
    TSYCAMV_NUMCHANS = TSYCAMV_NUM_PARAMS
};


#endif /* __TSYCAMV_DRV_I_H */
