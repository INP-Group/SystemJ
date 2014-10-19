
#include "drv_i/v0308_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t v0308_grouping[] =
{
#define V0308_LINE(n) \
    {"s"    #n, NULL,    NULL, NULL,    "#T-\v+", NULL, LOGT_READ,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0308_CHAN_PLRTY_base + n)}, \
    {"U"    #n, "Uset", "V",   "%4.0f", NULL,     NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0308_CHAN_VOLTS_base + n)}, \
    RGSWCH_LINE("-sw" #n, NULL, "0", "1",   \
                V0308_CHAN_IS_ON_base  + n, \
                V0308_CHAN_SW_OFF_base + n, \
                V0308_CHAN_SW_ON_base  + n), \
    REDLED_LINE ("uovl" #n, NULL, V0308_CHAN_OVL_U_base + n), \
    BLUELED_LINE("iovl" #n, NULL, V0308_CHAN_OVL_I_base + n), \
    BUTT_CELL   ("-rst" #n, "Reset", V0308_CHAN_RESET_base)

    GLOBELEM_START_CRN("dev", "V0308",
                       "-/+\fPolarity (-/+)\vU, V\v \vU!\vI!", "0\v1",
                       6 * 2, 6)
        V0308_LINE(0),
        V0308_LINE(1),
    GLOBELEM_END("notitle,noshadow", 0),

    {NULL}
};


static physprops_t v0308_physinfo[] =
{
};
