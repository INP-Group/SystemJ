#ifndef __SENKOV_EBC_DRV_I_H
#define __SENKOV_EBC_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


// r5,w5,w8,r8,w1,r1
enum
{
    SENKOV_EBC_CHAN_ADC_N_BASE = 0, SENKOV_EBC_CHAN_ADC_N_COUNT = 5,
    SENKOV_EBC_CHAN_DAC_N_BASE = 5, SENKOV_EBC_CHAN_DAC_N_COUNT = 5,
    SENKOV_EBC_CHAN_REGS_BASE  = 10,
    
    SENKOV_EBC_CHAN_REGS_WR8_BASE = SENKOV_EBC_CHAN_REGS_BASE + CANKOZ_REGS_WR8_BOFS,
    SENKOV_EBC_CHAN_REGS_RD8_BASE = SENKOV_EBC_CHAN_REGS_BASE + CANKOZ_REGS_RD8_BOFS,
    SENKOV_EBC_CHAN_REGS_WR1      = SENKOV_EBC_CHAN_REGS_BASE + CANKOZ_REGS_WR1_OFS,
    SENKOV_EBC_CHAN_REGS_RD1      = SENKOV_EBC_CHAN_REGS_BASE + CANKOZ_REGS_RD1_OFS,
    SENKOV_EBC_CHAN_REGS_LAST     = SENKOV_EBC_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS -1,

    SENKOV_EBC_NUMCHANS           = SENKOV_EBC_CHAN_REGS_LAST + 1
};

enum
{
    SENKOV_EBC_CHAN_ADC0 = SENKOV_EBC_CHAN_ADC_N_BASE + 0,
    SENKOV_EBC_CHAN_DAC0 = SENKOV_EBC_CHAN_DAC_N_BASE + 0,
};


#endif /* __SENKOV_EBC_DRV_I_H */