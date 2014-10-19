#include "drv_i/pisoenc600_drv_i.h"

#include "common_elem_macros.h"


static groupunit_t pisoenc600_grouping[] =
{
    {
        &(elemnet_t){
            "hwaddr", "HW-addr",
            NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"hwaddr", "HW-addr", NULL, "%3.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_HWADDR)},
            },
            "notitle,noshadow,nocoltitles,norowtitles"
        }, 0
    },

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_OUTRBx_base+n)}

    {
        &(elemnet_t){
            "outr", "Output reg",
            "8b\v7\v6\v5\v4\v3\v2\v1\v0", NULL,
            ELEM_MULTICOL, 9-2*1, 9,
            (logchannet_t []){
                {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_OUTR8B)},
                OUTB_LINE(7),
                OUTB_LINE(6),
                OUTB_LINE(5),
                OUTB_LINE(4),
                OUTB_LINE(3),
                OUTB_LINE(2),
                OUTB_LINE(1),
                OUTB_LINE(0),
            },
            "norowtitles"
        }, 0
    },

#define PISOENC600_LINE(l) \
    {"ctr"  __CX_STRINGIZE(l), __CX_STRINGIZE(l), "",   "%9.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_COUNTER_base      + l-1)}, \
    {"mode" __CX_STRINGIZE(l), NULL,              NULL, NULL,    "#TQadrant\vCW/CCW\vPulse/Dir\vUNUSED", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_MODE_base + l-1)}, \
    {"-z"   __CX_STRINGIZE(l), "0",               NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_DO_RST_base       + l-1)}, \
    {"c"    __CX_STRINGIZE(l), "",                NULL, NULL,    NULL, NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_C_base            + l-1)}, \
    {"hr"   __CX_STRINGIZE(l), "",                NULL, NULL,    NULL, NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_HR_base           + l-1)}, \
    {"acr"  __CX_STRINGIZE(l), "",                NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_ALLOW_CR_RST_base + l-1)}, \
    {"ahr"  __CX_STRINGIZE(l), "",                NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PISOENC600_CHAN_ALLOW_HR_RST_base + l-1)} \

    {
        &(elemnet_t){
            "ctl", "6 axis",
            "Counter\vMode\v0\vC\vHR\vC\n>0\vHR\n>0", "1\v2\v3\v4\v5\v6",
            ELEM_MULTICOL, 6*7, 7,
            (logchannet_t []){
                PISOENC600_LINE(1),
                PISOENC600_LINE(2),
                PISOENC600_LINE(3),
                PISOENC600_LINE(4),
                PISOENC600_LINE(5),
                PISOENC600_LINE(6),
            },
            ""
        }, 1
    },

    
    {NULL}
};

static physprops_t pisoenc600_physinfo[] =
{
};
