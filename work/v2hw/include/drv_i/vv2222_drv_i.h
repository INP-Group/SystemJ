#ifndef __VV2222_DRV_I_H
#define __VV2222_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


// w20,w8,r8,w1,r1,r12,r24,w16,r8
enum
{
    /*** Config channels ********************************************/
    VV2222_CONFIG_CHAN_BASE     = 0,  /* This is crucial for driver code! */
    VV2222_CONFIG_CHAN_COUNT    = 20,

    /*** I/O registers **********************************************/
    VV2222_CHAN_REGS_BASE       = 20,
    VV2222_CHAN_REGS_WR8_BASE   = VV2222_CHAN_REGS_BASE + CANKOZ_REGS_WR8_BOFS,
    VV2222_CHAN_REGS_RD8_BASE   = VV2222_CHAN_REGS_BASE + CANKOZ_REGS_RD8_BOFS,
    VV2222_CHAN_REGS_WR1        = VV2222_CHAN_REGS_BASE + CANKOZ_REGS_WR1_OFS,
    VV2222_CHAN_REGS_RD1        = VV2222_CHAN_REGS_BASE + CANKOZ_REGS_RD1_OFS,
    VV2222_CHAN_REGS_LAST       = VV2222_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS -1,

    VV2222_CHAN_REGS_RSVD_B     = VV2222_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS,
    VV2222_CHAN_REGS_RSVD_E     = 49,

    /* Regular ADC/DAC channels *************************************/
    VV2222_CHAN_ADC_n_BASE      = 50, VV2222_CHAN_ADC_n_COUNT = 4,
    VV2222_CHAN_OUT_n_BASE      = 74, VV2222_CHAN_OUT_n_COUNT = 4,
    VV2222_CHAN_OUT_RATE_n_BASE = 82,
    VV2222_CHAN_OUT_CUR_n_BASE  = 90,

    VV2222_NUMCHANS             = VV2222_CHAN_OUT_CUR_n_BASE + VV2222_CHAN_OUT_n_COUNT
};


#endif /* __VV2222_DRV_I_H */
