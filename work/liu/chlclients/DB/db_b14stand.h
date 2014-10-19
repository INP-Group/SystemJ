
#include "drv_i/dl200me_drv_i.h"
#include "drv_i/panov_ubs_drv_i.h"

#include "common_elem_macros.h"


enum
{
    T16_FAST_BASE = 0,
    T16_SLOW_BASE = T16_FAST_BASE + DL200ME_NUMCHANS,
    UBS1_BASE     = T16_SLOW_BASE + DL200ME_NUMCHANS,
    UBS2_BASE     = UBS1_BASE + PANOV_UBS_NUMCHANS * 1,
    UBS3_BASE     = UBS1_BASE + PANOV_UBS_NUMCHANS * 2,
    UBS4_BASE     = UBS1_BASE + PANOV_UBS_NUMCHANS * 3,
};

enum
{
    REG_T16_FAST_AUTO = 80,
    REG_T16_FAST_SECS = 81,
    REG_T16_FAST_PREV = 82,

    REG_T16_SLOW_AUTO = 90,
    REG_T16_SLOW_SECS = 91,
    REG_T16_SLOW_PREV = 92,
};


static groupunit_t b14stand_grouping[] =
{
#define UBS_COUNTERS(id, base) \
    GLOBELEM_START(id"_counters", NULL, 2, 2) \
        {"powerons",  "Poweron",  NULL, "%-9.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + PANOV_UBS_CHAN_POWERON_CTR)},  \
        {"watchdogs", "Watchdog", NULL, "%-9.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + PANOV_UBS_CHAN_WATCHDOG_CTR)}, \
    GLOBELEM_END("notitle,noshadow,nocoltitles,norowtitles", 0)

#define  DL200ME_BASE     T16_SLOW_BASE
#define  DL200ME_IDENT    "dl200me_s"
#define  DL200ME_LABEL    "Slow DL200ME"
#define  DL200ME_EDITABLE_LABELS
#define  DL200ME_REG_AUTO REG_T16_SLOW_AUTO
#define  DL200ME_REG_SECS REG_T16_SLOW_SECS
#define  DL200ME_REG_PREV REG_T16_SLOW_PREV
#define  FROMNEWLINE
#include "elem_dl200me_slow.h"
,
#define  UBS_BASE     UBS1_BASE
#define  UBS_IDENT    "ubs1"
#define  UBS_LABEL    "Modulateur ID=4"
#define  FROMNEWLINE
#include "elem_ubs.h"
,UBS_COUNTERS("ubs1", UBS1_BASE)
,
#define  UBS_BASE     UBS2_BASE
#define  UBS_IDENT    "ubs2"
#define  UBS_LABEL    "Modulateur ID=2"
#include "elem_ubs.h"
,UBS_COUNTERS("ubs2", UBS2_BASE)
,
    {NULL}
};


static physprops_t b14stand_physinfo[] =
{
#define  UBS_BASE     UBS1_BASE
#include "pi_ubs.h"
    
#define  UBS_BASE     UBS2_BASE
#include "pi_ubs.h"
    
#define  UBS_BASE     UBS3_BASE
#include "pi_ubs.h"
    
#define  UBS_BASE     UBS4_BASE
#include "pi_ubs.h"
    
#define  DL200ME_BASE     T16_FAST_BASE
#include "pi_dl200me_fast.h"

#define  DL200ME_BASE     T16_SLOW_BASE
#include "pi_dl200me_slow.h"
};
