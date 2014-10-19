
#include "drv_i/kshd485_drv_i.h"
#include "common_elem_macros.h"


enum
{
    KSHD_BASE = 0,
};

#define COLORLED_LINE(name, comnt, dtype, n) \
    {name, NULL, NULL, NULL, NULL, comnt, LOGT_READ, dtype, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_STATUS_BASE + n)}

#define BUT_LINE(name, title, n) \
    {name, title, "", NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(n)}

static groupunit_t kshd485_grouping[] =
{
    {
        &(elemnet_t){
            "Kshd", "ëûä-485",
            NULL, NULL,
            ELEM_MULTICOL, 3, 1,
            (logchannet_t []){
                SUBELEM_START("info", "Info", 2, 2)
                    {"devver", "Dev.ver.", NULL, "%4.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_DEV_VERSION)},
                    {"devsrl", "serial#",  NULL, "%3.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_DEV_SERIAL)},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                SUBELEM_START("config", "Config", 5, 1)
                    SUBELEM_START("curr", "Current", 2, 2)
                        {"work", "Work", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_WORK_CURRENT)},
                        {"hold", "Hold", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_HOLD_CURRENT)},
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                    {"hold_delay", "Hold dly", "1/30s", "%3.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_HOLD_DELAY), .minnorm=0, .maxnorm=255, .placement="horz=right"},
                    {"cfg_byte", "Cfg byte", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_CONFIG_BYTE)},
                    SUBELEM_START("vel", "Velocity", 2, 2)
                        {"min_vel", "[",   "/s",  "%5.0f", "withlabel",           NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_MIN_VELOCITY), .minnorm=32, .maxnorm=12000},
                        {"max_vel", "-",   "]/s", "%5.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_MAX_VELOCITY), .minnorm=32, .maxnorm=12000},
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                    {"accel",   "Accel", "/s^2", "%5.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_ACCELERATION), .minnorm=32, .maxnorm=65535, .placement="horz=center"},
                SUBELEM_END("noshadow,nocoltitles,fold_h", NULL),
                SUBELEM_START("operate", "Operation", 2, 1)
                    SUBELEM_START("flags", "Flags", 8 , 8)
                        COLORLED_LINE("ready", "Ready",       LOGD_GREENLED, 0),
                        COLORLED_LINE("going", "Going",       LOGD_GREENLED, 1),
                        COLORLED_LINE("k1",    "K-",          LOGD_REDLED,   2),
                        COLORLED_LINE("k2",    "K+",          LOGD_REDLED,   3),
                        COLORLED_LINE("k3",    "Sensor",      LOGD_REDLED,   4),
                        COLORLED_LINE("s_b5",  "Prec. speed", LOGD_AMBERLED, 5),
                        COLORLED_LINE("s_b6",  "K acc",       LOGD_AMBERLED, 6),
                        COLORLED_LINE("s_b7",  "BC",          LOGD_AMBERLED, 7),
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", "horz=right"),
                    SUBELEM_START("steps", "Steps", 6, 3)
                        {"numsteps", NULL, NULL, "%6.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KSHD_BASE + KSHD485_C_NUM_STEPS)},
                        BUT_LINE("go",         "GO",          KSHD_BASE + KSHD485_C_GO),
                        BUT_LINE("go_unaccel", "GO unaccel.", KSHD_BASE + KSHD485_C_GO_WO_ACCEL),
                        EMPTY_CELL(),
                        BUT_LINE("stop",       "STOP",             KSHD_BASE + KSHD485_C_STOP),
                        BUT_LINE("switchoff",  "Switch OFF",       KSHD_BASE + KSHD485_C_SWITCHOFF),
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                SUBELEM_END("noshadow,nocoltitles", NULL),
            },
            "noshadow,notitle,nocoltitles,norowtitles"
        }, 0
    },

    {NULL}
};

static physprops_t kshd485_physinfo[] =
{
};
