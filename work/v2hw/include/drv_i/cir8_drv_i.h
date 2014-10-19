#ifndef __CIR8_DRV_I_H
#define __CIR8_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


// w20,w8,r8,w1,r1,r12
enum
{
    /*** Config channels ********************************************/
    CIR8_CONFIG_CHAN_BASE     = 0,  /* This is crucial for driver code! */
    CIR8_CONFIG_CHAN_COUNT    = 20,

    /*** I/O registers **********************************************/
    CIR8_CHAN_REGS_BASE       = 20,
    CIR8_CHAN_REGS_WR8_BASE   = CIR8_CHAN_REGS_BASE + CANKOZ_REGS_WR8_BOFS,
    CIR8_CHAN_REGS_RD8_BASE   = CIR8_CHAN_REGS_BASE + CANKOZ_REGS_RD8_BOFS,
    CIR8_CHAN_REGS_WR1        = CIR8_CHAN_REGS_BASE + CANKOZ_REGS_WR1_OFS,
    CIR8_CHAN_REGS_RD1        = CIR8_CHAN_REGS_BASE + CANKOZ_REGS_RD1_OFS,
    CIR8_CHAN_REGS_LAST       = CIR8_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS -1,

    CIR8_CHAN_,

    CIR8_CHAN_REGS_RSVD_B     = CIR8_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS,
    CIR8_CHAN_REGS_RSVD_E     = 49,

    /* Powerful/opticalless I/O registers */
    
    CIR8_NUMCHANS             = ???
};


#endif /* __CIR8_DRV_I_H */
