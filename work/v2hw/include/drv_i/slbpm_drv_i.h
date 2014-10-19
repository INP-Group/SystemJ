#ifndef __SLBPM_DRV_I_H
#define __SLBPM_DRV_I_H


// auxinfo: ip_address/hostname

// w50,r50
enum
{
    SLBPM_CONFIG_CHAN_base    = 0,
      SLBPM_CONFIG_CHAN_count = 50,

    SLBPM_CHAN_DELAYA = SLBPM_CONFIG_CHAN_base + 9,   // Common delay, 1bit=10ps
    SLBPM_CHAN_DELAY0 = SLBPM_CONFIG_CHAN_base + 10,  // Individual delays 0-3
    SLBPM_CHAN_DELAY1 = SLBPM_CONFIG_CHAN_base + 11,
    SLBPM_CHAN_DELAY2 = SLBPM_CONFIG_CHAN_base + 12,
    SLBPM_CHAN_DELAY3 = SLBPM_CONFIG_CHAN_base + 13,
    SLBPM_CHAN_EMINUS = SLBPM_CONFIG_CHAN_base + 14,
    SLBPM_CHAN_AMPLON = SLBPM_CONFIG_CHAN_base + 15,
    SLBPM_CHAN_NAV    = SLBPM_CONFIG_CHAN_base + 16,
    SLBPM_CHAN_BORDER = SLBPM_CONFIG_CHAN_base + 17,
    SLBPM_CHAN_SHOT   = SLBPM_CONFIG_CHAN_base + 18,
    SLBPM_CHAN_CALB_0 = SLBPM_CONFIG_CHAN_base + 19,

    SLBPM_READ_CHAN_base      = 50,
      SLBPM_READ_CHAN_count   = 50,

    SLBPM_CHAN_MMX0   = SLBPM_READ_CHAN_base + 0,
    SLBPM_CHAN_MMX1   = SLBPM_READ_CHAN_base + 1,
    SLBPM_CHAN_MMX2   = SLBPM_READ_CHAN_base + 2,
    SLBPM_CHAN_MMX3   = SLBPM_READ_CHAN_base + 3,
#if 0
    SLBPM_CHAN_ZERO0  = SLBPM_READ_CHAN_base + 4,
    SLBPM_CHAN_ZERO1  = SLBPM_READ_CHAN_base + 5,
    SLBPM_CHAN_ZERO2  = SLBPM_READ_CHAN_base + 6,
    SLBPM_CHAN_ZERO3  = SLBPM_READ_CHAN_base + 7,
#endif

    SLBPM_NUMCHANS = 100,
};

// 4*0+0i/0+1i,1*0+0i/0+4i,5*0+0i/0+0i,4*1+0i/1+32i,1*1+0i/1+128i,4*1+0i/1+64i,1*1+0i/1+256i
enum
{
    // Mins/maxs as big-channels
    SLBPM_BIGC_MMX0 =  0,
    SLBPM_BIGC_MMX1 =  1,
    SLBPM_BIGC_MMX2 =  2,
    SLBPM_BIGC_MMX3 =  3,
    SLBPM_BIGC_MMXA =  4,

    // Stub (5pcs)

    // Per-cycle oscillograms
    // params[0]=32
    SLBPM_BIGC_OSC0 = 10,
    SLBPM_BIGC_OSC1 = 11,
    SLBPM_BIGC_OSC2 = 12,
    SLBPM_BIGC_OSC3 = 13,
    SLBPM_BIGC_OSCA = 14,

    // Per-(Nav+1) buffers
    // params[0]=(Nav+1)
    SLBPM_BIGC_BUF0 = 15,
    SLBPM_BIGC_BUF1 = 16,
    SLBPM_BIGC_BUF2 = 17,
    SLBPM_BIGC_BUF3 = 18,
    SLBPM_BIGC_BUFA = 19,

    SLBPM_NUMBIGCS = 20,
};


#endif /* __SLBPM_DRV_I_H */
