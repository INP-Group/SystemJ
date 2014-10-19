#ifndef __FROLOV_BFP16_DRV_I_H
#define __FROLOV_BFP16_DRV_I_H


// w20,r10
enum
{
    FROLOV_BFP16_CHAN_KMAX        = 0,
    FROLOV_BFP16_CHAN_KMIN        = 1,
    FROLOV_BFP16_CHAN_KTIME       = 2,

    FROLOV_BFP16_CHAN_LOOPMODE    = 8,
    FROLOV_BFP16_CHAN_NUMPERIODS  = 9,

    /* Note: command channels are in order of their As */
    FROLOV_BFP16_CHAN_MOD_ENABLE  = 10,
    FROLOV_BFP16_CHAN_MOD_DISABLE = 11,
    FROLOV_BFP16_CHAN_RESET_CTRS  = 12,
    FROLOV_BFP16_CHAN_WRK_ENABLE  = 13,
    FROLOV_BFP16_CHAN_WRK_DISABLE = 14,
    FROLOV_BFP16_CHAN_LOOP_START  = 15,
    FROLOV_BFP16_CHAN_LOOP_STOP   = 16,

    FROLOV_BFP16_CHAN_STAT_UBS    = 20,
    FROLOV_BFP16_CHAN_STAT_WKALWD = 21,
    FROLOV_BFP16_CHAN_STAT_MOD    = 22,
    FROLOV_BFP16_CHAN_STAT_LOOP   = 23,

    FROLOV_BFP16_CHAN_STAT_CURPRD = 24,

    FROLOV_BFP16_CHAN_COUNT       = 30
};


#endif /* __FROLOV_BFP16_DRV_I_H */