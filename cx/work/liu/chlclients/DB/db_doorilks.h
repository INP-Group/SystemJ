#include "drv_i/doorilks_drv_i.h"
#include "common_elem_macros.h"

static groupunit_t doorilks_grouping[] =
{
    GLOBELEM_START(NULL, NULL, 2*5, 5)
        {"mode", "Mode#", NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_MODE)},
        {"use0", "0",     NULL, NULL,    "",          NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_USED_base + 0)},
        {"use1", "1",     NULL, NULL,    "",          NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_USED_base + 1)},
        {"use2", "2",     NULL, NULL,    "",          NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_USED_base + 2)},
        {"use3", "3",     NULL, NULL,    "",          NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_USED_base + 3)},
        {"",     " OK ",  NULL, NULL,    "panel,offcol=red,color=green", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_SUM_STATE)},
        {"ilk0", " 0 ",   NULL, NULL,    "panel,offcol=red,color=green", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_ILK_base + 0)},
        {"ilk1", " 1 ",   NULL, NULL,    "panel,offcol=red,color=green", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_ILK_base + 1)},
        {"ilk2", " 2 ",   NULL, NULL,    "panel,offcol=red,color=green", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_ILK_base + 2)},
        {"ilk3", " 3 ",   NULL, NULL,    "panel,offcol=red,color=green", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DOORILKS_CHAN_ILK_base + 3)},
    GLOBELEM_END("noshadow,notitle,nocoltitles,norowtitles", 0),

    {NULL}
};

static physprops_t doorilks_physinfo[] =
{
};
