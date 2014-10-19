#include "drv_i/frolov_bl700_drv_i.h"

#include "common_elem_macros.h"


static groupunit_t frolov_bl700_grouping[] =
{

#define FROLOV_BL700_MINMAX_LINE(n) \
    {"min"__CX_STRINGIZE(n), NULL, NULL, NULL, "shape=circle", NULL, LOGT_READ, LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BL700_CHAN_MIN_ILK_n_BASE + n)}, \
    {"max"__CX_STRINGIZE(n), NULL, NULL, NULL, "shape=circle", NULL, LOGT_READ, LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BL700_CHAN_MAX_ILK_n_BASE + n)}

    {
        &(elemnet_t){
            NULL, NULL,
            NULL, NULL,
            ELEM_MULTICOL, 2, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("mins_maxs", NULL, "M\ni\nn\vM\na\nx", "0\v1\v2\v3\v4\v5\v6\v7", 16, 2)
                    FROLOV_BL700_MINMAX_LINE(0),
                    FROLOV_BL700_MINMAX_LINE(1),
                    FROLOV_BL700_MINMAX_LINE(2),
                    FROLOV_BL700_MINMAX_LINE(3),
                    FROLOV_BL700_MINMAX_LINE(4),
                    FROLOV_BL700_MINMAX_LINE(5),
                    FROLOV_BL700_MINMAX_LINE(6),
                    FROLOV_BL700_MINMAX_LINE(7),
                SUBELEM_END("notitle,noshadow", NULL),
                SUBELEM_START("ctl", NULL, 3, 1)
                    {"out",  "Ilk",   NULL, NULL, "shape=circle", "Effective interlock state", LOGT_READ,   LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BL700_CHAN_OUT_ILK_STATE)},
                    {"ext",  "Ext",   NULL, NULL, "shape=circle", "External interlock state",  LOGT_READ,   LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BL700_CHAN_EXT_ILK_STATE)},
                    {"-rst", "Reset", NULL, NULL, NULL,           "Reset interlocks",          LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BL700_CHAN_RESET_ILKS)},
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
            },
            "notitle,noshadow,nocoltitles,norowtitles"
        }, 0
    },

    {NULL}
};


static physprops_t frolov_bl700_physinfo[] =
{
    /* All channels have 1-1 mapping */
};
