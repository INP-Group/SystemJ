#ifndef __DELAYUNIT_DRV_I_H
#define __DELAYUNIT_DRV_I_H

enum
{
    DELAYUNIT_CHAN_0 = 0,
    DELAYUNIT_CHAN_1,
    DELAYUNIT_CHAN_2,
    DELAYUNIT_CHAN_3,

    DELAYUNIT_NUMCHANS,
};

enum
{
    /* VEPP2K */
    DELAYUNIT_V2_DIGITAL_QUANT =  5000,     // 5ns    = 5000ps
    DELAYUNIT_V2_ANALOG_QUANT  =   250,     // 0.25ns = 250ps

    DELAYUNIT_V2_DP_BITS       = 8,
    DELAYUNIT_V2_AP_BITS       = 5,
    DELAYUNIT_V2_DP_MAX        = 255,            // == (1<<DP_BITS)-1
    DELAYUNIT_V2_AP_MAX        = 31, /*!!! 16?*/ // == (1<<AP_BITS)-1

    DELAYUNIT_V2_MIN_VAL       = 0,
    DELAYUNIT_V2_MAX_VAL       =            // VEPP2000 period == 81357 ps
         16 * DELAYUNIT_V2_DIGITAL_QUANT +
          7 * DELAYUNIT_V2_ANALOG_QUANT,    // == 16 * 5000 + 7 * 250

    /* VEPP5 */
    DELAYUNIT_V5_DIGITAL_QUANT = 41800,     // 41.8ns = 41800ps
    DELAYUNIT_V5_ANALOG_QUANT  =   200,     //  0.2ns =   200ps

    DELAYUNIT_V5_DP_BITS       = 1,
    DELAYUNIT_V5_AP_BITS       = 8,
    DELAYUNIT_V5_DP_MAX        = 1,         // == (1<<DP_BITS)-1
    DELAYUNIT_V5_AP_MAX        = 255,       // == (1<<AP_BITS)-1

    DELAYUNIT_V5_MIN_VAL       = 0,
    DELAYUNIT_V5_MAX_VAL       =
          1 * DELAYUNIT_V5_DIGITAL_QUANT +
        255 * DELAYUNIT_V5_ANALOG_QUANT     // == 1 * 41800 + 255 * 200
};

#endif /* __DELAYUNIT_DRV_I_H */
