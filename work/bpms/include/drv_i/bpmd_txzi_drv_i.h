#ifndef __BPMD_TXZI_DRV_I_H
#define __BPMD_TXZI_DRV_I_H


#include "misc_types.h"


enum
{
    /* writable channels */
    BPMD_TXZI_WR_base           = 0,
        BPMD_TXZI_WR_count      = 60,

    BPMD_TXZI_CHAN_SHOT         = BPMD_TXZI_WR_base + 0,
    BPMD_TXZI_CHAN_STOP         = BPMD_TXZI_WR_base + 1,
    BPMD_TXZI_CHAN_ISTART       = BPMD_TXZI_WR_base + 2,
    BPMD_TXZI_CHAN_WAITTIME     = BPMD_TXZI_WR_base + 3,
    BPMD_TXZI_CHAN_CALC_STATS   = BPMD_TXZI_WR_base + 4,

    BPMD_TXZI_CHAN_NUMPTS       = BPMD_TXZI_WR_base + 10,
    BPMD_TXZI_CHAN_PTSOFS       = BPMD_TXZI_WR_base + 11,

    BPMD_TXZI_CHAN_TIMINGS      = BPMD_TXZI_WR_base + 20,

    BPMD_TXZI_CHAN_DELAY        = BPMD_TXZI_WR_base + 30,
    BPMD_TXZI_CHAN_ATTEN        = BPMD_TXZI_WR_base + 31,
    BPMD_TXZI_CHAN_DECMTN       = BPMD_TXZI_WR_base + 32,
    BPMD_TXZI_CHAN_SIG2INT      = BPMD_TXZI_WR_base + 33,

    /* readable channels */
    BPMD_TXZI_RD_base           = 60,
        BPMD_TXZI_RD_count      = 40,

    BPMD_TXZI_CHAN_MIN0         = BPMD_TXZI_RD_base + 0,
    BPMD_TXZI_CHAN_MIN1         = BPMD_TXZI_RD_base + 1,
    BPMD_TXZI_CHAN_MIN2         = BPMD_TXZI_RD_base + 2,
    BPMD_TXZI_CHAN_MIN3         = BPMD_TXZI_RD_base + 3,
    
    BPMD_TXZI_CHAN_MAX0         = BPMD_TXZI_RD_base + 4,
    BPMD_TXZI_CHAN_MAX1         = BPMD_TXZI_RD_base + 5,
    BPMD_TXZI_CHAN_MAX2         = BPMD_TXZI_RD_base + 6,
    BPMD_TXZI_CHAN_MAX3         = BPMD_TXZI_RD_base + 7,

    BPMD_TXZI_CHAN_AVG0         = BPMD_TXZI_RD_base + 8,
    BPMD_TXZI_CHAN_AVG1         = BPMD_TXZI_RD_base + 9,
    BPMD_TXZI_CHAN_AVG2         = BPMD_TXZI_RD_base + 10,
    BPMD_TXZI_CHAN_AVG3         = BPMD_TXZI_RD_base + 11,

    BPMD_TXZI_CHAN_INT0         = BPMD_TXZI_RD_base + 12,
    BPMD_TXZI_CHAN_INT1         = BPMD_TXZI_RD_base + 13,
    BPMD_TXZI_CHAN_INT2         = BPMD_TXZI_RD_base + 14,
    BPMD_TXZI_CHAN_INT3         = BPMD_TXZI_RD_base + 15,

    /* own statistiscs */
    BPMD_TXZI_CHAN_BPMID        = BPMD_TXZI_RD_base + 20,

    BPMD_TXZI_CHAN_XMEAN        = BPMD_TXZI_RD_base + 21,
    BPMD_TXZI_CHAN_XSTDDEV      = BPMD_TXZI_RD_base + 22,
    BPMD_TXZI_CHAN_ZMEAN        = BPMD_TXZI_RD_base + 23,
    BPMD_TXZI_CHAN_ZSTDDEV      = BPMD_TXZI_RD_base + 24,
    BPMD_TXZI_CHAN_IMEAN        = BPMD_TXZI_RD_base + 25,
    BPMD_TXZI_CHAN_ISTDDEV      = BPMD_TXZI_RD_base + 26,

    BPMD_TXZI_CHAN_STATUS       = BPMD_TXZI_RD_base + 38,
    BPMD_TXZI_CHAN_ELAPSED      = BPMD_TXZI_RD_base + 39,

    BPMD_TXZI_CHAN_COUNT        = 100
};

/* General device specs */
typedef float BPMD_TXZI_DATUM_T;
enum
{
    BPMD_TXZI_JUNK_MSMTS = 0,
    BPMD_TXZI_MAX_NUMPTS = 8192 - BPMD_TXZI_JUNK_MSMTS,
    BPMD_TXZI_NUM_LINES  = 4,
    BPMD_TXZI_DATAUNITS  = sizeof(BPMD_TXZI_DATUM_T),
};

#endif /* __BPMD_TXZI_DRV_I_H */

