#include "drv_i/c0601_drv_i.h"

#include "common_elem_macros.h"


static groupunit_t c0601_grouping[] =
{

#define C0601_LINE(n) \
    {"t"__CX_STRINGIZE(n), NULL, NULL, "%3.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C0601_CHAN_T_n_BASE + n)}

    {
        &(elemnet_t){
            NULL, NULL,
            NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("matrix", NULL, "T, °C", "0\v1\v2\v3\v4\v5\v6\v7\v8\v9\v10\v11\v12\v13\v14\v15", 16, 1)
                    C0601_LINE(0),
                    C0601_LINE(1),
                    C0601_LINE(2),
                    C0601_LINE(3),
                    C0601_LINE(4),
                    C0601_LINE(5),
                    C0601_LINE(6),
                    C0601_LINE(7),
                    C0601_LINE(8),
                    C0601_LINE(9),
                    C0601_LINE(10),
                    C0601_LINE(11),
                    C0601_LINE(12),
                    C0601_LINE(13),
                    C0601_LINE(14),
                    C0601_LINE(15),
                SUBELEM_END("notitle,noshadow", NULL),
            },
            "notitle,noshadow,nocoltitles,norowtitles"
        }, 0
    },


    {NULL}
};


static physprops_t c0601_physinfo[] =
{
    /* All channels have 1-1 mapping */
};
