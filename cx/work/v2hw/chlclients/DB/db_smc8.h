#include "drv_i/smc8_drv_i.h"
#include "common_elem_macros.h"


#define ONE_LINE(n) \
    {"steps"    #n, #n,     NULL, "%8.0f", NULL,                   NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_NUM_STEPS_base    + n)}, \
    {"go"       #n, "GO",   NULL, NULL,    NULL,                   NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_GO_base           + n)}, \
    {"going"    #n, NULL,   NULL, NULL,    "shape=circle",         NULL, LOGT_READ,   LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_GOING_base        + n)}, \
    {"stop"     #n, "Stop", NULL, NULL,    NULL,                   NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_STOP_base         + n)}, \
    {"km"       #n, NULL,   NULL, NULL,    "shape=circle",         NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_KM_base           + n)}, \
    {"kp"       #n, NULL,   NULL, NULL,    "shape=circle",         NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_KP_base           + n)}, \
    {"state"    #n, NULL,   NULL, NULL,    "#T"SMC8_STATE_MSTRING, NULL, LOGT_READ,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_STATE_base        + n)}, \
    {"start_frq"#n, "[",    NULL, "%5.0f", "withlabel",            NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_START_FREQ_base   + n)}, \
    {"final_frq"#n, "-",    "]",  "%5.0f", "withlabel,withunits",  NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_FINAL_FREQ_base   + n)}, \
    {"accel"    #n, NULL,   NULL, "%5.0f", NULL,                   NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_ACCELERATION_base + n)}, \
    {"counter"  #n, NULL,   NULL, "%8.0f", NULL,                   NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_COUNTER_base      + n)}, \
    {"rst_ctr"  #n, "=0",   NULL, NULL,    NULL,                   NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_RST_COUNTER_base  + n)}, \
    {"out_md"   #n, NULL,   NULL, NULL,    "#TS+/S-\vS/Dir",       NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_OUT_MD_base       + n)}, \
    {"maskm"    #n, NULL,   NULL, NULL,    "#TIgn\vStop",          NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_MASKM_base        + n)}, \
    {"maskp"    #n, NULL,   NULL, NULL,    "#TIgn\vStop",          NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_MASKP_base        + n)}, \
    {"typem"    #n, NULL,   NULL, NULL,    "#TNorm=Cls\vNorm=Opn", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_TYPEM_base        + n)}, \
    {"typep"    #n, NULL,   NULL, NULL,    "#TNorm=Cls\vNorm=Opn", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_CHAN_TYPEP_base        + n)}

static groupunit_t smc8_grouping[] =
{
    GLOBELEM_START_CRN("all", NULL, "# steps\v \v...\v \v-\v+\vState\vStartFrq, /s\vFinalFrq, /s\vAccel, /s^2\vCounter\v \vOut Mode\vM-\vM+\vT-\vT+", NULL, SMC8_NUMLINES*17, 17)
        ONE_LINE(0),
        ONE_LINE(1),
        ONE_LINE(2),
        ONE_LINE(3),
        ONE_LINE(4),
        ONE_LINE(5),
        ONE_LINE(6),
        ONE_LINE(7),
    GLOBELEM_END("noshadow,notitle", 0),

    {NULL}
};

static physprops_t smc8_physinfo[] =
{
};
