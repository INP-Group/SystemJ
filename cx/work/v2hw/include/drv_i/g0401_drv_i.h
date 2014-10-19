#ifndef __G0401_DRV_I_H
#define __G0401_DRV_I_H


enum
{
    G0401_CHAN_16BIT_N_BASE = 0,
    G0401_CHAN_QUANT_N_BASE = 8,
    G0401_CHAN_RSRVD_N_BASE = 16, // Reserved for "mask"
    G0401_CHAN_RSRVD8       = 24, // Reserved for "mask8"
    G0401_CHAN_PROGSTART    = 25,
    G0401_CHAN_SCALE        = 26,
    G0401_CHAN_BRKON7       = 27,

    G0401_NUMCHANS          = 28
};


#endif /* __G0401_DRV_I_H */
