#include "fastadcclient.h"
#include "common_elem_macros.h"

#define PHYSINFO_ARRAY_NAME sukhpanel_physinfo
#include "pi_linac1_10.h"


#define ADC_LINE(n, l)                                          \
    GLOBELEM_START("adc-"__CX_STRINGIZE(n), l, 3, 1 | (2<<24))  \
        {"comm0", "Ch0", NULL, NULL,                            \
         "#T0\v1\v2\v3\vOff", NULL, LOGT_WRITE1, LOGD_CHOICEBS, \
         LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(COMM_BASE + (n)*2 + 0)}, \
        {"comm1", "Ch1", NULL, NULL,                            \
         "#T0\v1\v2\v3\vOff", NULL, LOGT_WRITE1, LOGD_CHOICEBS, \
         LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(COMM_BASE + (n)*2 + 1)}, \
        {"adc", NULL, NULL, NULL,                               \
            " width=700 save_button"                            \
            " subsys=\"sukh-adc200-"__CX_STRINGIZE(n)"\"",      \
         NULL, LOGT_READ, LOGD_NADC200, LOGK_DIRECT,            \
         /*!!! A hack: we use 'color' field to pass integer */  \
         NADC200_BIGC_BASE + (n),                               \
         PHYS_ID(NADC200_CHAN_BASE + (n*1) * NADC200_CHANS_PER_DEV) \
        },                                                      \
    GLOBELEM_END("nocoltitles,norowtitles,one", GU_FLAG_FROMNEWLINE | GU_FLAG_FILLHORZ | (n==1? GU_FLAG_FILLVERT:0))

#define C0642_SET(n) \
    {"set"__CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C0642_BASE+C0642_CHAN_OUT_n_base+n), .minnorm=-10.0, .maxnorm=+10.0}

static groupunit_t sukhpanel_grouping[] =
{
    GLOBELEM_START("s-m", "Settings, measurements", 3, 3)
        {"ch0", "Mes", "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C0609_BASE+C0609_CHAN_ADC_n_base +0)},
        VSEP_L(),
        SUBELEM_START("dac", "C0642", 16, 8)
            C0642_SET(0),
            C0642_SET(1),
            C0642_SET(2),
            C0642_SET(3),
            C0642_SET(4),
            C0642_SET(5),
            C0642_SET(6),
            C0642_SET(7),
            C0642_SET(8),
            C0642_SET(9),
            C0642_SET(10),
            C0642_SET(11),
            C0642_SET(12),
            C0642_SET(13),
            C0642_SET(14),
            C0642_SET(15),
        SUBELEM_END("noshadow,notitle,nocoltitles", NULL)
    GLOBELEM_END("nocoltitles,norowtitles", 0),

    ADC_LINE(0, "One"),
    ADC_LINE(1, "Two"),

    {NULL}
};
