#ifndef __NADC4DECIMATOR_V2_DRV_I_H
#define __NADC4DECIMATOR_V2_DRV_I_H

/* Channels */
enum
{
    DEC_FACT_CH,        /* Decimation factor,
                           or frequency division factor for ADC  */
    DEC_DUTY_CH,        /* Decimation scale
                           or duty factor                        */
    CALIBR_CH,          /* =1 -> Turn ON  calibration signal
                           =0 -> Turn OFF calibration signal     */

    SHOT_CH,            /* =1 -> Single-shot / start from PC     */
    STOP_CH,            /* =1 -> Stop device at all and drop LAM */

    DECIMATOR_NUMC,
};

/* Values */
enum /* decimation factor */
{
    DF_0   = -1,        /* Fake value.
                           Meaning -> No decimation at all       */
    DF_3   =  0,
    DF_5   =  1,
    DF_9   =  2,
    DF_17  =  3,
    DF_33  =  4,
};

enum /* decimation duty   */
{
    DD_1   = 0,
    DD_2   = 1,
    DD_3   = 2,
    DD_4   = 3,
    DD_5   = 4,
    DD_6   = 5,
    DD_7   = 6,
    DD_8   = 7,
    DD_9   = 8,
    DD_10  = 9,
    DD_11  = 10,
    DD_12  = 11,
    DD_13  = 12,
    DD_14  = 13,
    DD_15  = 14,
    DD_16  = 15,
};

#endif /* __NADC4DECIMATOR_V2_DRV_I_H */
