#include "drv_i/slbpm_drv_i.h"
#include "common_elem_macros.h"


#define SLBPM_LINE(n) \
    {"delay"#n, "Delay "  #n, "ns", "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_DELAY0 + n)}, \
    {"mmx"  #n, "Min/Max "#n, "",   "%5.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_MMX0   + n)}//, \
//    {"zero" #n, "Zero "   #n, "",   "%5.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_ZERO0  + n)}

groupunit_t slbpm_grouping[] =
{
#if 1
    GLOBELEM_START_CRN("ctl", NULL, NULL, " \vCommon delay\vPolarity\vAmplification\vBorder\vNav", 6, 1)
        EMPTY_CELL(),
        {"delayA", "Common Delay", "ns", "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_DELAYA)},
        {"plrty",  "", NULL, NULL, "#Te+\ve-", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_EMINUS)},
        {"ampl",   "", NULL, NULL, "#T=\v*",   "Amplification", LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_AMPLON)},
        {"border", "", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_BORDER)},
        {"nav",    "", NULL, "%2.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_NAV)},
    GLOBELEM_END("noshadow,notitle,nocoltitles", 0),
    GLOBELEM_START_CRN("data", NULL, "Delay, ns\vRead", "0\v1\v2\v3", 4*2, 2)
        SLBPM_LINE(0),
        SLBPM_LINE(1),
        SLBPM_LINE(2),
        SLBPM_LINE(3),
    GLOBELEM_END("noshadow,notitle", 0),
#else
    GLOBELEM_START_CRN("slbpm", NULL, NULL, " \v \v0\v1\v2\v3", 6*3, 3)
        {"delayA",   "Common Delay", "ns", "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_DELAYA)},
        {"plrty",   "", NULL, NULL, "#Te+\ve-", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_EMINUS)},
        {"ampl",    "", NULL, NULL, "#T=\v*",   "Amplification", LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_AMPLON)},

        HLABEL_CELL("Delay, ns"),
        {"border", "[0-", ")", "%3.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SLBPM_CHAN_BORDER)},
        BUTT_CELL("-calb0", "Calb 0", SLBPM_CHAN_CALB_0),

        SLBPM_LINE(0),
        SLBPM_LINE(1),
        SLBPM_LINE(2),
        SLBPM_LINE(3),
    GLOBELEM_END("noshadow,notitle,nocoltitles", 0),
#endif

    {NULL}
};

physprops_t slbpm_physinfo[] =
{
    {SLBPM_CHAN_DELAYA, 100, 0, 0},
    {SLBPM_CHAN_DELAY0, 100, 0, 0},
    {SLBPM_CHAN_DELAY1, 100, 0, 0},
    {SLBPM_CHAN_DELAY2, 100, 0, 0},
    {SLBPM_CHAN_DELAY3, 100, 0, 0},
};
