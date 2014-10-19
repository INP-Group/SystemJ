
#include "fastadcclient.h"
#include "drv_i/v0308_drv_i.h"
#include "common_elem_macros.h"


enum
{
    VVI_CHAN_BASE = 0,
    FA2_CHAN_BASE = 200,
    FA2_BIGC_BASE = 0,
};

#define VVI_CHAN(nd, nl, base) \
    (VVI_CHAN_BASE + V0308_NUMCHANS *(nd)) + V0308_CHAN_##base##_base + nl

static groupunit_t blm_grouping[] =
{
#define V0308_LINE(nd, nl) \
    {"s"#nd"_"#nl, NULL,    NULL, NULL,    "#T-\v+", NULL, LOGT_READ,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VVI_CHAN(nd, nl, PLRTY))}, \
    {"U"#nd"_"#nl, "Uset", "V",   "%4.0f", NULL,     NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VVI_CHAN(nd, nl, VOLTS))}, \
    RGSWCH_LINE("-sw"#nd"_"#nl, NULL, "0", "1",   \
                VVI_CHAN(nd, nl, IS_ON ), \
                VVI_CHAN(nd, nl, SW_OFF), \
                VVI_CHAN(nd, nl, SW_ON )), \
    REDLED_LINE ("uovl"#nd"_"#nl, NULL,    VVI_CHAN(nd, nl, OVL_U)), \
    BLUELED_LINE("iovl"#nd"_"#nl, NULL,    VVI_CHAN(nd, nl, OVL_I)), \
    BUTT_CELL   ("-rst"#nd"_"#nl, "Reset", VVI_CHAN(nd, nl, RESET))

    {
        &(elemnet_t){
            ":", NULL,
            NULL, NULL,
            ELEM_CANVAS, 3, 1,
            (logchannet_t []){
        {"adc0", NULL, NULL, NULL,
            " width=700 save_button"
            " subsys=\"adc200-blm-a\"",
         NULL, LOGT_READ, LOGD_NADC200, LOGK_DIRECT,
         /*!!! A hack: we use 'color' field to pass integer */
         FA2_BIGC_BASE + 0,
         PHYS_ID(FA2_CHAN_BASE + 100*0),
         .placement="left=0,right=0,top=0,bottom=0@vvi"
        },

        SUBELEM_START_CRN("vvi", "V0308",
                          "-/+\fPolarity (-/+)\vU, V\v \vU!\vI!", "0\v1",
                          6 * 4, 6 + 2*65536)
            V0308_LINE(0, 0),
            V0308_LINE(0, 1),
            V0308_LINE(1, 0),
            V0308_LINE(1, 1),
        SUBELEM_END("notitle,noshadow", "left=0,bottom=0@adc1"),
    
        {"adc1", NULL, NULL, NULL,
            " width=700 save_button"
            " subsys=\"adc200-blm-b\"",
         NULL, LOGT_READ, LOGD_NADC200, LOGK_DIRECT,
         /*!!! A hack: we use 'color' field to pass integer */
         FA2_BIGC_BASE + 1,
         PHYS_ID(FA2_CHAN_BASE + 100*1),
         .placement="left=0,right=0,bottom=0"
        },

    GLOBELEM_END("notitle,noshadow",
                 GU_FLAG_FILLHORZ | GU_FLAG_FILLVERT),

    {NULL}
};


static physprops_t blm_physinfo[] =
{
};
