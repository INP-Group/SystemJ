#include "common_elem_macros.h"

#include "drv_i/xcac208_drv_i.h"


enum
{
    C208_BASE = 58
};


static groupunit_t ringcurmon_grouping[] =
{
    GLOBELEM_START("value", "Value", 2, 2)
        {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP},
        {"current", "I", "V", "%8.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C208_BASE + XCAC208_CHAN_ADC_n_BASE + 0), .minnorm=-10.0, .maxnorm=+10.0},
        {"c2",      "I", "V", "%8.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C208_BASE + XCAC208_CHAN_ADC_n_BASE + 1), .minnorm=-10.0, .maxnorm=+10.0},
    GLOBELEM_END("notitle,nocoltitles,norowtitles", 0),
    GLOBELEM_START("histplot", "History plot", 1, 1)
        {"plot", "HistPlot", NULL, NULL, "width=700,height=290,add=.current,add=.c2,fixed", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
    GLOBELEM_END("noshadow,notitle,nocoltitles,norowtitles,one", 
                 GU_FLAG_FROMNEWLINE | GU_FLAG_FILLHORZ | GU_FLAG_FILLVERT),

    {NULL}
};

static physprops_t ringcurmon_physinfo[] =
{
    {C208_BASE + XCAC208_CHAN_ADC_n_BASE + 0, 1000000, 0, 0},
    {C208_BASE + XCAC208_CHAN_ADC_n_BASE + 1, 1000000, 0, 0},
};
