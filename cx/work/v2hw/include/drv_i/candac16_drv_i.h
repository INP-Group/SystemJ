#ifndef __CANDAC16_DRV_I_H
#define __CANDAC16_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


enum
{
    CANDAC16_CHAN_WRITE_N_BASE  = 0,  CANDAC16_CHAN_WRITE_N_COUNT = 16,
    CANDAC16_CHAN_REGS_BASE     = 16,
    
    CANDAC16_CHAN_REGS_WR8_BASE = CANDAC16_CHAN_REGS_BASE + CANKOZ_REGS_WR8_BOFS,
    CANDAC16_CHAN_REGS_RD8_BASE = CANDAC16_CHAN_REGS_BASE + CANKOZ_REGS_RD8_BOFS,
    CANDAC16_CHAN_REGS_WR1      = CANDAC16_CHAN_REGS_BASE + CANKOZ_REGS_WR1_OFS,
    CANDAC16_CHAN_REGS_RD1      = CANDAC16_CHAN_REGS_BASE + CANKOZ_REGS_RD1_OFS,
    CANDAC16_CHAN_REGS_LAST     = CANDAC16_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS -1,

    CANDAC16_NUMCHANS           = CANDAC16_CHAN_REGS_LAST + 1
};


#endif /* __CANDAC16_DRV_I_H */