#ifndef __CANPCSC8_DRV_I_H
#define __CANPCSC8_DRV_I_H


enum
{
    CANPCSC8_NUMLINES = 8,
    CANPCSC8_CHAN_STATUS_BASE = 0,
    CANPCSC8_CHAN_LATCH_BASE  = 8,
    CANPCSC8_CHAN_1ST_BASE    = 16,
    CANPCSC8_CHAN_WR_BASE     = 24,
    CANPCSC8_CHAN_STATUS1     = 32,
    CANPCSC8_CHAN_LATCH1      = 33,
    CANPCSC8_CHAN_1ST1        = 34,
    CANPCSC8_CHAN_WR1         = 35,
    CANPCSC8_NUMCHANS         = 36
};


#endif /* __CANPCSC8_DRV_I_H */