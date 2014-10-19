#ifndef __IST_XCDAC20_DRV_I_H
#define __IST_XCDAC20_DRV_I_H


// w50,r50
enum
{
    IST_XCDAC20_CHAN_WR_base      = 0,
      IST_XCDAC20_CHAN_WR_count   = 50,

    IST_XCDAC20_CHAN_OUT            = IST_XCDAC20_CHAN_WR_base + 0,
    IST_XCDAC20_CHAN_OUT_RATE       = IST_XCDAC20_CHAN_WR_base + 1,
    IST_XCDAC20_CHAN_SWITCH_ON      = IST_XCDAC20_CHAN_WR_base + 2,
    IST_XCDAC20_CHAN_SWITCH_OFF     = IST_XCDAC20_CHAN_WR_base + 3,
    IST_XCDAC20_CHAN_RST_ILKS       = IST_XCDAC20_CHAN_WR_base + 4,
    IST_XCDAC20_CHAN_RESET_STATE    = IST_XCDAC20_CHAN_WR_base + 5,
    IST_XCDAC20_CHAN_DO_CALB_DAC    = IST_XCDAC20_CHAN_WR_base + 6,
    IST_XCDAC20_CHAN_AUTOCALB_ONOFF = IST_XCDAC20_CHAN_WR_base + 7,
    IST_XCDAC20_CHAN_AUTOCALB_SECS  = IST_XCDAC20_CHAN_WR_base + 8,
    IST_XCDAC20_CHAN_DIGCORR_MODE   = IST_XCDAC20_CHAN_WR_base + 9,
    IST_XCDAC20_CHAN_RESET_C_ILKS   = IST_XCDAC20_CHAN_WR_base + 10,

    
    IST_XCDAC20_CHAN_RD_base      = 50,
      IST_XCDAC20_CHAN_RD_count   = 50,

    IST_XCDAC20_CHAN_DCCT1          = IST_XCDAC20_CHAN_RD_base + 0,
    IST_XCDAC20_CHAN_DCCT2          = IST_XCDAC20_CHAN_RD_base + 1,
    IST_XCDAC20_CHAN_DAC            = IST_XCDAC20_CHAN_RD_base + 2,
    IST_XCDAC20_CHAN_U2             = IST_XCDAC20_CHAN_RD_base + 3,
    IST_XCDAC20_CHAN_ADC4           = IST_XCDAC20_CHAN_RD_base + 4,
    IST_XCDAC20_CHAN_ADC_DAC        = IST_XCDAC20_CHAN_RD_base + 5,
    IST_XCDAC20_CHAN_ADC_0V         = IST_XCDAC20_CHAN_RD_base + 6,
    IST_XCDAC20_CHAN_ADC_P10V       = IST_XCDAC20_CHAN_RD_base + 7,

    IST_XCDAC20_CHAN_OUT_CUR        = IST_XCDAC20_CHAN_RD_base + 8,

    IST_XCDAC20_CHAN_DIGCORR_FACTOR = IST_XCDAC20_CHAN_RD_base + 9,

    IST_XCDAC20_CHAN_OPR            = IST_XCDAC20_CHAN_RD_base + 10,
    
    IST_XCDAC20_CHAN_ILK_base     =   IST_XCDAC20_CHAN_RD_base + 11,
      IST_XCDAC20_CHAN_ILK_count  =   7,
    IST_XCDAC20_CHAN_ILK_OUT_PROT   = IST_XCDAC20_CHAN_ILK_base + 0,
    IST_XCDAC20_CHAN_ILK_WATER      = IST_XCDAC20_CHAN_ILK_base + 1,
    IST_XCDAC20_CHAN_ILK_IMAX       = IST_XCDAC20_CHAN_ILK_base + 2,
    IST_XCDAC20_CHAN_ILK_UMAX       = IST_XCDAC20_CHAN_ILK_base + 3,
    IST_XCDAC20_CHAN_ILK_BATTERY    = IST_XCDAC20_CHAN_ILK_base + 4,
    IST_XCDAC20_CHAN_ILK_PHASE      = IST_XCDAC20_CHAN_ILK_base + 5,
    IST_XCDAC20_CHAN_ILK_TEMP       = IST_XCDAC20_CHAN_ILK_base + 6,

    IST_XCDAC20_CHAN_IS_ON          = IST_XCDAC20_CHAN_RD_base + 18,

    IST_XCDAC20_CHAN_C_ILK_base   =   IST_XCDAC20_CHAN_RD_base + 19,
      IST_XCDAC20_CHAN_C_ILK_count=   7,
    IST_XCDAC20_CHAN_C_ILK_OUT_PROT = IST_XCDAC20_CHAN_C_ILK_base + 0,
    IST_XCDAC20_CHAN_C_ILK_WATER    = IST_XCDAC20_CHAN_C_ILK_base + 1,
    IST_XCDAC20_CHAN_C_ILK_IMAX     = IST_XCDAC20_CHAN_C_ILK_base + 2,
    IST_XCDAC20_CHAN_C_ILK_UMAX     = IST_XCDAC20_CHAN_C_ILK_base + 3,
    IST_XCDAC20_CHAN_C_ILK_BATTERY  = IST_XCDAC20_CHAN_C_ILK_base + 4,
    IST_XCDAC20_CHAN_C_ILK_PHASE    = IST_XCDAC20_CHAN_C_ILK_base + 5,
    IST_XCDAC20_CHAN_C_ILK_TEMP     = IST_XCDAC20_CHAN_C_ILK_base + 6,

    IST_XCDAC20_CHAN_CUR_POLARITY   = 98,
    IST_XCDAC20_CHAN_IST_STATE      = 99,

    IST_XCDAC20_NUMCHANS          =   100,
};


#endif /* __IST_XCDAC20_DRV_I_H */