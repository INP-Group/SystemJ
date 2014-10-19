#ifndef __DOORILKS_DRV_I_H
#define __DOORILKS_DRV_I_H


// w50,r50
enum
{
    // Prerequisites
    DOORILKS_CHAN_ILK_count    = 4,
    
    // Regular channels
    DOORILKS_CHAN_WR_base      = 0,
      DOORILKS_CHAN_WR_count   = 50,

    DOORILKS_CHAN_MODE           = DOORILKS_CHAN_WR_base + 0,
    DOORILKS_CHAN_USED_base      = DOORILKS_CHAN_WR_base + 1,
    // ...+DOORILKS_CHAN_ILK_count-1

    DOORILKS_CHAN_RD_base      = 50,
      DOORILKS_CHAN_RD_count   = 50,

    DOORILKS_CHAN_SUM_STATE      = DOORILKS_CHAN_RD_base + 0,
    DOORILKS_CHAN_ILK_base       = DOORILKS_CHAN_RD_base + 1,
    // ...+DOORILKS_CHAN_ILK_count-1

    DOORILKS_NUMCHANS          =   100,
};


#endif /* __DOORILKS_DRV_I_H */
