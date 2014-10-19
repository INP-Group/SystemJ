#ifndef __XCEAC124_DRV_I_H
#define __XCEAC124_DRV_I_H


#include "drv_i/cankoz_common_drv_i.h"


// w20,w8,r8,w1,r1,r12,r16,w8,r4
enum
{
    /*** Config channels ********************************************/
    XCEAC124_CONFIG_CHAN_BASE     = 0,  /* This is crucial for driver code! */
    XCEAC124_CONFIG_CHAN_COUNT    = 20,

    XCEAC124_CHAN_DO_RESET        = XCEAC124_CONFIG_CHAN_BASE + 0,
    XCEAC124_CHAN_ADC_MODE        = XCEAC124_CONFIG_CHAN_BASE + 1,
    XCEAC124_CHAN_ADC_BEG         = XCEAC124_CONFIG_CHAN_BASE + 2,
    XCEAC124_CHAN_ADC_END         = XCEAC124_CONFIG_CHAN_BASE + 3,
    XCEAC124_CHAN_ADC_TIMECODE    = XCEAC124_CONFIG_CHAN_BASE + 4,
    XCEAC124_CHAN_ADC_GAIN        = XCEAC124_CONFIG_CHAN_BASE + 5,
    XCEAC124_CHAN_RESERVED6       = XCEAC124_CONFIG_CHAN_BASE + 6,
    XCEAC124_CHAN_RESERVED7       = XCEAC124_CONFIG_CHAN_BASE + 7,
    XCEAC124_CHAN_RESERVED8       = XCEAC124_CONFIG_CHAN_BASE + 8,
    XCEAC124_CHAN_OUT_MODE        = XCEAC124_CONFIG_CHAN_BASE + 9,

    XCEAC124_CHAN_DO_TAB_DROP     = XCEAC124_CONFIG_CHAN_BASE + 10,
    XCEAC124_CHAN_DO_TAB_ACTIVATE = XCEAC124_CONFIG_CHAN_BASE + 11,
    XCEAC124_CHAN_DO_TAB_START    = XCEAC124_CONFIG_CHAN_BASE + 12,
    XCEAC124_CHAN_DO_TAB_STOP     = XCEAC124_CONFIG_CHAN_BASE + 13,
    XCEAC124_CHAN_DO_TAB_PAUSE    = XCEAC124_CONFIG_CHAN_BASE + 14,
    XCEAC124_CHAN_DO_TAB_RESUME   = XCEAC124_CONFIG_CHAN_BASE + 15,
      XCEAC124_CHAN_DO_TAB_BASE = XCEAC124_CHAN_DO_TAB_DROP,
      XCEAC124_CHAN_DO_TAB_CNT  = XCEAC124_CHAN_DO_TAB_RESUME - XCEAC124_CHAN_DO_TAB_BASE + 1,

    XCEAC124_CHAN_DO_CALIBRATE    = XCEAC124_CONFIG_CHAN_BASE + 16,
    XCEAC124_CHAN_DIGCORR_MODE    = XCEAC124_CONFIG_CHAN_BASE + 17,
    XCEAC124_CHAN_DIGCORR_V       = XCEAC124_CONFIG_CHAN_BASE + 18,
    XCEAC124_CHAN_RESERVED19      = XCEAC124_CONFIG_CHAN_BASE + 19,

    /*** I/O registers **********************************************/
    XCEAC124_CHAN_REGS_BASE       = 20,
    XCEAC124_CHAN_REGS_WR8_BASE   = XCEAC124_CHAN_REGS_BASE + CANKOZ_REGS_WR8_BOFS,
    XCEAC124_CHAN_REGS_RD8_BASE   = XCEAC124_CHAN_REGS_BASE + CANKOZ_REGS_RD8_BOFS,
    XCEAC124_CHAN_REGS_WR1        = XCEAC124_CHAN_REGS_BASE + CANKOZ_REGS_WR1_OFS,
    XCEAC124_CHAN_REGS_RD1        = XCEAC124_CHAN_REGS_BASE + CANKOZ_REGS_RD1_OFS,
    XCEAC124_CHAN_REGS_LAST       = XCEAC124_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS -1,

    XCEAC124_CHAN_REGS_RSVD_B     = XCEAC124_CHAN_REGS_BASE + CANKOZ_REGS_NUMCHANS,
    XCEAC124_CHAN_REGS_RSVD_E     = 49,

    /* Regular ADC/DAC channels *************************************/
    XCEAC124_CHAN_ADC_n_BASE      = 50, XCEAC124_CHAN_ADC_n_COUNT = 16,
    XCEAC124_CHAN_OUT_n_BASE      = 66, XCEAC124_CHAN_OUT_n_COUNT = 4,
    XCEAC124_CHAN_OUT_RATE_n_BASE = 70,
    XCEAC124_CHAN_OUT_CUR_n_BASE  = 74,

    XCEAC124_NUMCHANS             = XCEAC124_CHAN_OUT_CUR_n_BASE + XCEAC124_CHAN_OUT_n_COUNT
};

enum
{
    XCEAC124_CHAN_ADC_T    = XCEAC124_CHAN_ADC_n_BASE + XCEAC124_CHAN_ADC_n_COUNT - 4,
    XCEAC124_CHAN_ADC_PWR  = XCEAC124_CHAN_ADC_n_BASE + XCEAC124_CHAN_ADC_n_COUNT - 3,
    XCEAC124_CHAN_ADC_P10V = XCEAC124_CHAN_ADC_n_BASE + XCEAC124_CHAN_ADC_n_COUNT - 2,
    XCEAC124_CHAN_ADC_0V   = XCEAC124_CHAN_ADC_n_BASE + XCEAC124_CHAN_ADC_n_COUNT - 1,
};

// 8*1+58i/1+58i,1*1+261i/1+261i
enum
{
    XCEAC124_BIGC_ITAB_n_BASE = 0, // Individual per-channel tables
    XCEAC124_BIGC_TAB_WHOLE   = 4, // Whole-channels table -- =XCEAC124_BIGC_ITABLE_n_BASE+XCEAC124_CHAN_OUT_n_COUNT

    XCEAC124_NUM_BIGCS          = 5
};

enum
{
    XCEAC124_ADC_MODE_NORM   = 0,
    XCEAC124_ADC_MODE_OSCILL = 1,
    XCEAC124_ADC_MODE_PLOT   = 2,
    XCEAC124_ADC_MODE_TBACK  = 3,
};

enum
{
    XCEAC124_OUT_MODE_NORM   = 0,
    XCEAC124_OUT_MODE_TABLE  = 1,
};

enum
{
    XCEAC124_VAL_DISABLE_TABLE_CHAN = 20*1000*1000,  // 20V or higher -- disable channel

};


#endif /* __XCEAC124_DRV_I_H */