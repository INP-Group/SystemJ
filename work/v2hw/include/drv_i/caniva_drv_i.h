#ifndef __CANIVA_DRV_I_H
#define __CANIVA_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


enum
{
    CANIVA_CHAN_IMES_N_BASE  =  0, CANIVA_CHAN_IMES_N_COUNT = 16,
    CANIVA_CHAN_UMES_N_BASE  = 16, CANIVA_CHAN_UMES_N_COUNT = 16,
    CANIVA_CHAN_ILIM_N_BASE  = 32, CANIVA_CHAN_ILIM_N_COUNT = 16,
    CANIVA_CHAN_ULIM_N_BASE  = 48, CANIVA_CHAN_ULIM_N_COUNT = 16,
    CANIVA_CHAN_ILHW_N_BASE  = 64, CANIVA_CHAN_ILHW_N_COUNT = 16,
    CANIVA_CHAN_IALM_N_BASE  = 80, CANIVA_CHAN_IALM_N_COUNT = 16,
    CANIVA_CHAN_UALM_N_BASE  = 96, CANIVA_CHAN_UALM_N_COUNT = 16,

    CANIVA_NUMCHANS          = 112
};


#endif /* __CANIVA_DRV_I_H */
