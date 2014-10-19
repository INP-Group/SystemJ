
#include "drv_i/sukh_comm_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t sukh_comm_grouping[] =
{

#define COMM_LINE(n) \
    {"comm" __CX_STRINGIZE(n), "Chan" __CX_STRINGIZE(n), NULL, NULL, "#T0\v1\v2\v3\vOff", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SUKH_COMM_CHAN_COMM_n_base+n), .minnorm=-10.0, .maxnorm=+10.0}

    GLOBELEM_START("comm", "Comm", 2, 2)
        COMM_LINE(0),
        COMM_LINE(1),
    GLOBELEM_END("notitle,noshadow,nocoltitles,norowtitles", 0),

    {NULL}
};


static physprops_t sukh_comm_physinfo[] =
{
};
