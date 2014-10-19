#ifndef __CEAC124_DRV_I_H
#define __CEAC124_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


enum
{
    CEAC124_CHAN_READ_N_BASE   = 0,   CEAC124_CHAN_READ_N_COUNT  = 16,
    CEAC124_CHAN_WRITE_N_BASE  = 16,  CEAC124_CHAN_WRITE_N_COUNT = 4,
    CEAC124_CHAN_REGS_BASE     = 20,
    
    CEAC124_CHAN_REGS_WR8_BASE = CEAC124_CHAN_REGS_BASE + CANKOZ_REGS_WR8_BOFS,
    CEAC124_CHAN_REGS_RD8_BASE = CEAC124_CHAN_REGS_BASE + CANKOZ_REGS_RD8_BOFS,
    CEAC124_CHAN_REGS_WR1      = CEAC124_CHAN_REGS_BASE + CANKOZ_REGS_WR1_OFS,
    CEAC124_CHAN_REGS_RD1      = CEAC124_CHAN_REGS_BASE + CANKOZ_REGS_RD1_OFS,
    CEAC124_CHAN_REGS_LAST     = CEAC124_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS -1,

    CEAC124_NUMCHANS           = CEAC124_CHAN_REGS_LAST + 1
};

enum
{
    CEAC124_CHAN_READ_T    = CEAC124_CHAN_READ_N_BASE + CEAC124_CHAN_READ_N_COUNT - 4,
    CEAC124_CHAN_READ_PWR  = CEAC124_CHAN_READ_N_BASE + CEAC124_CHAN_READ_N_COUNT - 3,
    CEAC124_CHAN_READ_P10V = CEAC124_CHAN_READ_N_BASE + CEAC124_CHAN_READ_N_COUNT - 2,
    CEAC124_CHAN_READ_0V   = CEAC124_CHAN_READ_N_BASE + CEAC124_CHAN_READ_N_COUNT - 1,
};


#endif /* __CEAC124_DRV_I_H */
