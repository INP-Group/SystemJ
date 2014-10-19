#ifndef __CURVV_DRV_I_H
#define __CURVV_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


// w20,w8,r8,w1,r1,r12,w8,r24,w24,w1,r1,w1,r1
enum
{
    /*** Config channels ********************************************/
    CURVV_CONFIG_CHAN_BASE     = 0,  /* This is crucial for driver code! */
    CURVV_CONFIG_CHAN_COUNT    = 20,

    /*** I/O registers **********************************************/
    CURVV_CHAN_REGS_BASE       = 20,
    CURVV_CHAN_REGS_WR8_BASE   = CURVV_CHAN_REGS_BASE + CANKOZ_REGS_WR8_BOFS,
    CURVV_CHAN_REGS_RD8_BASE   = CURVV_CHAN_REGS_BASE + CANKOZ_REGS_RD8_BOFS,
    CURVV_CHAN_REGS_WR1        = CURVV_CHAN_REGS_BASE + CANKOZ_REGS_WR1_OFS,
    CURVV_CHAN_REGS_RD1        = CURVV_CHAN_REGS_BASE + CANKOZ_REGS_RD1_OFS,
    CURVV_CHAN_REGS_LAST       = CURVV_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS -1,

    CURVV_CHAN_REGS_RSVD_B     = CURVV_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS,
    CURVV_CHAN_REGS_RSVD_E     = 49,

    /* Powerful/opticalless I/O registers */
    CURVV_CHAN_PWR_TTL_BASE    = 50, CURVV_CHAN_PWR_TTL_COUNT = 60,

    CURVV_CHAN_PWR_Bn_BASE     = CURVV_CHAN_PWR_TTL_BASE + 0,  CURVV_CHAN_PWR_Bn_COUNT = 8,
    CURVV_CHAN_TTL_Bn_BASE     = CURVV_CHAN_PWR_TTL_BASE + 8,  CURVV_CHAN_TTL_Bn_COUNT = 24,
    CURVV_CHAN_TTL_IEn_BASE    = CURVV_CHAN_PWR_TTL_BASE + 32,
    CURVV_CHAN_PWR_8B          = CURVV_CHAN_PWR_TTL_BASE + 56,
    CURVV_CHAN_TTL_24B         = CURVV_CHAN_PWR_TTL_BASE + 57,
    CURVV_CHAN_TTL_IE24B       = CURVV_CHAN_PWR_TTL_BASE + 58,
    CURVV_CHAN_TTL_PWR_RSRVD59 = CURVV_CHAN_PWR_TTL_BASE + 59,
    
    CURVV_NUMCHANS             = CURVV_CHAN_PWR_TTL_BASE + CURVV_CHAN_PWR_TTL_COUNT
};


#endif /* __CURVV_DRV_I_H */
