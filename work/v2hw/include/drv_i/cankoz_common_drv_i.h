#ifndef __CANKOZ_COMMON_DRV_I_H
#define __CANKOZ_COMMON_DRV_I_H


enum
{
    CANKOZ_REGS_WR8_BOFS = 0,   // Individual (channel-per-bit) outreg's bits
    CANKOZ_REGS_RD8_BOFS = 8,   // Individual (channel-per-bit) inreg's bits
    CANKOZ_REGS_WR1_OFS  = 16,  // Whole, 8-bit outreg channel
    CANKOZ_REGS_RD1_OFS  = 17,  // Whole, 8-bit inreg channel
    CANKOZ_REGS_NUMCHANS = 18   // # of regs-block channels
};


#endif /* __CANKOZ_COMMON_DRV_I_H */
