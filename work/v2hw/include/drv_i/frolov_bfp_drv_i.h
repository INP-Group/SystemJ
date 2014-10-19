#ifndef __FROLOV_BFP_DRV_I_H
#define __FROLOV_BFP_DRV_I_H


// w6,r4
enum
{
    FROLOV_BFP_CHAN_KMAX        = 0,
    FROLOV_BFP_CHAN_KMIN        = 1,
    FROLOV_BFP_CHAN_WRK_ENABLE  = 2,
    FROLOV_BFP_CHAN_WRK_DISABLE = 3,
    FROLOV_BFP_CHAN_MOD_ENABLE  = 4,
    FROLOV_BFP_CHAN_MOD_DISABLE = 5,
    FROLOV_BFP_CHAN_STAT_UBS    = 6,
    FROLOV_BFP_CHAN_STAT_WKALWD = 7,
    FROLOV_BFP_CHAN_STAT_WRK    = 8,
    FROLOV_BFP_CHAN_STAT_MOD    = 9,

    FROLOV_BFP_CHAN_COUNT       = 10
};


#endif /* __FROLOV_BFP_DRV_I_H */