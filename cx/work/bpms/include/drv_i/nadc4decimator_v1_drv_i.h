#ifndef __NADC4DECIMATOR_V1_DRV_I_H
#define __NADC4DECIMATOR_V1_DRV_I_H

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

    /* Another values are alowed to be arbitrary, but >= 0 */
};




enum /* decimation duty   */
{
    __JUNK__ = 0,
    /* Another values are alowed to be arbitrary, but >= 0 */
};















#endif /* __NADC4DECIMATOR_V1_DRV_I_H */
