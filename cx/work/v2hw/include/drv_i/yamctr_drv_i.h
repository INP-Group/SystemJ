#ifndef __YAMCTR_DRV_I_H
#define __YAMCTR_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


// r20,w30
enum
{
    YAMCTR_CHAN_CUR_COORD        = 0,
    YAMCTR_CHAN_CUR_V            = 1,
    YAMCTR_CHAN_CUR_I            = 2,

    YAMCTR_CHAN_ILK_base         = 10, YAMCTR_CHAN_ILK_count = 8,

    YAMCTR_CHAN_LKM_base         = 20, YAMCTR_CHAN_LKM_count = 8,

    YAMCTR_CHAN_SET_V            = 30,
    YAMCTR_CHAN_SET_I_MAX        = 31,
    YAMCTR_CHAN_SET_V_PROP_COEFF = 32,
    YAMCTR_CHAN_SET_V_TIME_COEFF = 33,
    YAMCTR_CHAN_SET_I_PROP_COEFF = 34,
    YAMCTR_CHAN_SET_I_TIME_COEFF = 35,

    YAMCTR_CHAN_GO               = 40,
    YAMCTR_CHAN_STOP             = 41,

    YAMCTR_NUMCHANS              = 50,
};


#endif /* __YAMCTR_DRV_I_H */
