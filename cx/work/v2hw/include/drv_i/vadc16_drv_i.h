#ifndef __VADC16_DRV_I_H
#define __VADC16_DRV_I_H


// w50,r24,r26
enum
{
    /*** Config channels ********************************************/
    VADC16_CONFIG_CHAN_BASE     = 0,
    VADC16_CONFIG_CHAN_COUNT    = 20,

    VADC16_CHAN_ADC_BEG         = VADC16_CONFIG_CHAN_BASE + 2,
    VADC16_CHAN_ADC_END         = VADC16_CONFIG_CHAN_BASE + 3,
    VADC16_CHAN_ADC_TIMECODE    = VADC16_CONFIG_CHAN_BASE + 4,

    /*** Regular channels *******************************************/
    VADC16_CHAN_ADC_n_BASE      = 50, VADC16_CHAN_ADC_n_COUNT = 24,

    VADC16_NUMCHANS             = 100
};


#endif /* __VADC16_DRV_I_H */
