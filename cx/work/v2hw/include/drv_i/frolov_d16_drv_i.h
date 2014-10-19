#ifndef __FROLOV_D16_DRV_I_H
#define __FROLOV_D16_DRV_I_H


enum
{
    FROLOV_D16_NUMOUTPUTS     = 4,
    
    FROLOV_D16_CHAN_A_BASE    = 0,
    FROLOV_D16_CHAN_B_BASE    = 4,
    FROLOV_D16_CHAN_OFF_BASE  = 8,
    FROLOV_D16_CHAN_ALLOFF    = 12,
    FROLOV_D16_CHAN_DO_SHOT   = 13,

    FROLOV_D16_CHAN_KSTART    = 14,
    FROLOV_D16_CHAN_KCLK_N    = 15,
    FROLOV_D16_CHAN_FCLK_SEL  = 16,
    FROLOV_D16_CHAN_START_SEL = 17,
    FROLOV_D16_CHAN_MODE      = 18,
    FROLOV_D16_CHAN_UNUSED2   = 19,

    FROLOV_D16_NUMCHANS       = 20
};

enum
{
    FROLOV_D16_V_KCLK_1          = 0,
    FROLOV_D16_V_KCLK_2          = 1,
    FROLOV_D16_V_KCLK_4          = 2,
    FROLOV_D16_V_KCLK_8          = 3,

    FROLOV_D16_V_FCLK_FIN        = 0,
    FROLOV_D16_V_FCLK_QUARTZ     = 1,
    FROLOV_D16_V_FCLK_IMPACT     = 2,

    FROLOV_D16_V_START_START     = 0,
    FROLOV_D16_V_START_Y1        = 1,
    FROLOV_D16_V_START_50HZ      = 2,

    FROLOV_D16_V_MODE_CONTINUOUS = 0,
    FROLOV_D16_V_MODE_ONESHOT    = 1,
};


#endif /* __FROLOV_D16_DRV_I_H */