#include "drv_i/yamkshd8_drv_i.h"
#include "common_elem_macros.h"


#define ONE_LINE(n) \
    {"steps"  #n, NULL,   NULL, "%8.0f", NULL,                  NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_NUM_STEPS_base    + n)}, \
    {"go"     #n, "GO",   NULL, NULL,    NULL,                  NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_GO_base           + n)}, \
    {"going"  #n, NULL,   NULL, NULL,    NULL,                  NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_GOING_base        + n)}, \
    {"stop"   #n, "Stop", NULL, NULL,    NULL,                  NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_STOP_base         + n)}, \
    {"min_vel"#n, "[",    NULL, "%5.0f", "withlabel",           NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_MIN_VELOCITY_base + n)}, \
    {"max_vel"#n, "-",    "]",  "%5.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_MAX_VELOCITY_base + n)}, \
    {"accel"  #n, NULL,   NULL, "%5.0f", NULL,                  NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_ACCELERATION_base + n)}, \
    {"t_accel"#n, NULL,   NULL, "%5.0f", NULL,                  NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_T_ACCEL_base      + n)}, \
    {"counter"#n, NULL,   NULL, "%8.0f", NULL,                  NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_COUNTER_base      + n)}, \
    {"rst_ctr"#n, "Rst",  NULL, NULL,    NULL,                  NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(YAMKSHD8_CHAN_RST_COUNTER_base  + n)}

static groupunit_t yamkshd8_grouping[] =
{
    GLOBELEM_START_CRN("all", NULL, "# steps\v \v...\v \vMinSpd\vMaxSpd\vAccel, /s^2\vTaccel, ms\vCounter\vRst", "0\v1\v2\v3\v4\v5\v6\v7", 8*10, 10)
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

static physprops_t yamkshd8_physinfo[] =
{
};
