#ifndef __BPMD_CFG_DRV_I_H
#define __BPMD_CFG_DRV_I_H

enum
{
    /* writable channels */
    BPMD_CFG_CHAN_WR_base          = 0,
        BPMD_CFG_CHAN_WR_count     = 30,

    /* operation type */
    BPMD_CFG_CHAN_REGIME_base      = BPMD_CFG_CHAN_WR_base + 0,
        BPMD_CFG_CHAN_REGIME_count = 8,
    BPMD_CFG_CHAN_REGIME_AUTOMAT   = BPMD_CFG_CHAN_REGIME_base + 0,
    BPMD_CFG_CHAN_REGIME_ELCTRUN   = BPMD_CFG_CHAN_REGIME_base + 1,
    BPMD_CFG_CHAN_REGIME_ELCTINJ   = BPMD_CFG_CHAN_REGIME_base + 2,
    BPMD_CFG_CHAN_REGIME_ELCTDLS   = BPMD_CFG_CHAN_REGIME_base + 3,
    BPMD_CFG_CHAN_REGIME_PSTRRUN   = BPMD_CFG_CHAN_REGIME_base + 4,
    BPMD_CFG_CHAN_REGIME_PSTRINJ   = BPMD_CFG_CHAN_REGIME_base + 5,
    BPMD_CFG_CHAN_REGIME_PSTRDLS   = BPMD_CFG_CHAN_REGIME_base + 6,
    BPMD_CFG_CHAN_REGIME_FREERUN   = BPMD_CFG_CHAN_REGIME_base + 7,

    /* ... */
    BPMD_CFG_CHAN_REGIME_RESET     = BPMD_CFG_CHAN_WR_base + 8,
    BPMD_CFG_CHAN_FORCE_RELOAD     = BPMD_CFG_CHAN_WR_base + 9,
    
    /* parameters for operation */
    BPMD_CFG_CHAN_NUMPTS           = BPMD_CFG_CHAN_WR_base + 10,
    BPMD_CFG_CHAN_PTSOFS           = BPMD_CFG_CHAN_WR_base + 11,

    BPMD_CFG_CHAN_DELAY            = BPMD_CFG_CHAN_WR_base + 12,
    BPMD_CFG_CHAN_ATTEN            = BPMD_CFG_CHAN_WR_base + 13,
    BPMD_CFG_CHAN_DECMTN           = BPMD_CFG_CHAN_WR_base + 14,

    BPMD_CFG_CHAN_DELAY_LINE1      = BPMD_CFG_CHAN_WR_base + 15,
    BPMD_CFG_CHAN_DELAY_LINE2      = BPMD_CFG_CHAN_WR_base + 16,
    BPMD_CFG_CHAN_DELAY_LINE3      = BPMD_CFG_CHAN_WR_base + 17,
    BPMD_CFG_CHAN_DELAY_LINE4      = BPMD_CFG_CHAN_WR_base + 18,

    BPMD_CFG_CHAN_ZERO_LINE1       = BPMD_CFG_CHAN_WR_base + 19,
    BPMD_CFG_CHAN_ZERO_LINE2       = BPMD_CFG_CHAN_WR_base + 20,
    BPMD_CFG_CHAN_ZERO_LINE3       = BPMD_CFG_CHAN_WR_base + 21,
    BPMD_CFG_CHAN_ZERO_LINE4       = BPMD_CFG_CHAN_WR_base + 22,

    /* ... */
    BPMD_CFG_CHAN_REGIME_MSAVE     = BPMD_CFG_CHAN_WR_base + 23,

    /* readable cgannels */
    BPMD_CFG_CHAN_RD_base          = 30,
        BPMD_CFG_CHAN_RD_count     = 20,

    BPMD_CFG_CHAN_BPMD_IS_ON       = BPMD_CFG_CHAN_RD_base + 0,
    BPMD_CFG_CHAN_BPMD_STATE       = BPMD_CFG_CHAN_RD_base + 1,
    
    BPMD_CFG_CHAN_COUNT            = 50,
};

#endif // __BPMD_CFG_DRV_I_H
