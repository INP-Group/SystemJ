#ifndef __VSDC2_DRV_I_H
#define __VSDC2_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"

// w20,w8,r8,w1,r1,r12,w10,r10
enum
{
    /*** Config channels ********************************************/
    VSDC2_CONFIG_CHAN_BASE     = 0,  /* This is crucial for driver code! */
    VSDC2_CONFIG_CHAN_COUNT    = 20,

    /*** I/O registers **********************************************/
    VSDC2_CHAN_REGS_BASE       = 20,

    /* Regular channels         *************************************/
    VSDC2_CHAN_INT0            = 50,
    VSDC2_CHAN_INT1            = 51,

    VSDC2_NUMCHANS             = 70
};



#endif /* __VSDC2_DRV_I_H */
