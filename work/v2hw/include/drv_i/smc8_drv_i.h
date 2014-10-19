#ifndef __SMC8_DRV_I_H
#define __SMC8_DRV_I_H


enum
{
    SMC8_NUMLINES = 8,
};


// r64,w136
enum
{
    SMC8_g_GOING        = 0,
    SMC8_g_STATE        = 1,
    SMC8_g_KM           = 2,
    SMC8_g_KP           = 3,
    SMC8_g_COUNTER      = 4,
    SMC8_g_5            = 5,
    SMC8_g_6            = 6,
    SMC8_g_7            = 7,
    SMC8_g_STOP         = 8,
    SMC8_g_GO           = 9,
    SMC8_g_GO_N_STEPS   = 10,
    SMC8_g_RST_COUNTER  = 11,
    SMC8_g_START_FREQ   = 12,
    SMC8_g_FINAL_FREQ   = 13,
    SMC8_g_ACCELERATION = 14,
    SMC8_g_NUM_STEPS    = 15,
    SMC8_g_OUT_MD       = 16, SMC8_g_CONTROL_FIRST = SMC8_g_OUT_MD, // Note: following gropus are in CONTROL_REG bits order
    SMC8_g_MASKM        = 17,
    SMC8_g_MASKP        = 18,
    SMC8_g_TYPEM        = 19,
    SMC8_g_TYPEP        = 20, SMC8_g_CONTROL_LAST  = SMC8_g_TYPEP,
    SMC8_g_21           = 21,
    SMC8_g_22           = 22,
    SMC8_g_23           = 23,
    SMC8_g_24           = 24,

    SMC8_CHAN_GOING_base        = SMC8_g_GOING        * SMC8_NUMLINES,
    SMC8_CHAN_STATE_base        = SMC8_g_STATE        * SMC8_NUMLINES,
    SMC8_CHAN_KM_base           = SMC8_g_KM           * SMC8_NUMLINES,
    SMC8_CHAN_KP_base           = SMC8_g_KP           * SMC8_NUMLINES,
    SMC8_CHAN_COUNTER_base      = SMC8_g_COUNTER      * SMC8_NUMLINES,
    SMC8_CHAN_STOP_base         = SMC8_g_STOP         * SMC8_NUMLINES,
    SMC8_CHAN_GO_base           = SMC8_g_GO           * SMC8_NUMLINES,
    SMC8_CHAN_GO_N_STEPS_base   = SMC8_g_GO_N_STEPS   * SMC8_NUMLINES,
    SMC8_CHAN_RST_COUNTER_base  = SMC8_g_RST_COUNTER  * SMC8_NUMLINES,
    SMC8_CHAN_START_FREQ_base   = SMC8_g_START_FREQ   * SMC8_NUMLINES,
    SMC8_CHAN_FINAL_FREQ_base   = SMC8_g_FINAL_FREQ   * SMC8_NUMLINES,
    SMC8_CHAN_ACCELERATION_base = SMC8_g_ACCELERATION * SMC8_NUMLINES,
    SMC8_CHAN_NUM_STEPS_base    = SMC8_g_NUM_STEPS    * SMC8_NUMLINES,
    SMC8_CHAN_OUT_MD_base       = SMC8_g_OUT_MD       * SMC8_NUMLINES,
    SMC8_CHAN_MASKM_base        = SMC8_g_MASKM        * SMC8_NUMLINES,
    SMC8_CHAN_MASKP_base        = SMC8_g_MASKP        * SMC8_NUMLINES,
    SMC8_CHAN_TYPEM_base        = SMC8_g_TYPEM        * SMC8_NUMLINES,
    SMC8_CHAN_TYPEP_base        = SMC8_g_TYPEP        * SMC8_NUMLINES,

    SMC8_NUMCHANS = 200,
};

enum
{
    SMC8_STATE_STILL       = 0,  // 0b0000
    SMC8_STATE_LIMIT       = 1,  // 0b0001
    SMC8_STATE_STOPPED     = 2,  // 0b0010
    SMC8_STATE_ERRPARAMS   = 2,  // 0b0011

    SMC8_STATE_STARTING    = 8,  // 0b1000
    SMC8_STATE_ACCEL       = 9,  // 0b1001
    SMC8_STATE_CONSTANT    = 10, // 0b1010
    SMC8_STATE_DECEL       = 11, // 0b1011
    SMC8_STATE_STOPPING    = 12, // 0b1100

    SMC8_STATE_MOVING_mask = 8
};
#define SMC8_STATE_MSTRING \
    "----\vLimitSw\vStopped\vErrPar\v?4?\v?5?\v?6?\v?7?\v" \
    "Start\vAccel\vGoing\vDecel\vStop\v?13?\v?14?\v?15?"


#endif /* __SMC8_DRV_I_H */
