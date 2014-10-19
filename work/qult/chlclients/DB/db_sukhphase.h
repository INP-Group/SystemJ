#include "fastadcclient.h"
#include "sukhclient.h"
#include "common_elem_macros.h"

#define PHYSINFO_ARRAY_NAME sukhphase_physinfo
#include "pi_linac1_10.h"


static groupunit_t sukhphase_grouping[] =
{

#define COMM_SEL(n)                                              \
    {"comm"__CX_STRINGIZE(n), "Ch"__CX_STRINGIZE(n), NULL, NULL, \
        "#T0\v1\v2\v3\vOff", NULL, LOGT_WRITE1, LOGD_CHOICEBS,   \
        LOGK_CALCED, LOGC_NORMAL, PHYS_ID(COMM_BASE + (n)*2 + 0),\
        (excmd_t[]){                                             \
            CMD_GETP_I(COMM_BASE + (n)*2 + 0),                   \
            CMD_RETSEP,                                          \
            CMD_GETP_I(COMM_BASE + (n)*2 + 0),                   \
            CMD_GETP_I(COMM_BASE + (n)*2 + 1),                   \
            CMD_SUB, CMD_WEIRD,                                  \
            CMD_RET_I(0),                                        \
        },                                                       \
        (excmd_t[]){                                             \
            CMD_QRYVAL, CMD_SETP_I(COMM_BASE + (n)*2 + 0),       \
            CMD_QRYVAL, CMD_SETP_I(COMM_BASE + (n)*2 + 1),       \
            CMD_RET                                              \
        }                                                        \
    }

#define C0642_SET(n) \
    {"set"__CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C0642_BASE+C0642_CHAN_OUT_n_base+n), .minnorm=-10.0, .maxnorm=+10.0}

#define ADC_LINE(n, l, plc)                                 \
    {"adc_"__CX_STRINGIZE(n), NULL, NULL, NULL,             \
        " width=700 save_button"                            \
        " subsys=\"sukh-adc200-"__CX_STRINGIZE(n)"\"",      \
     NULL, LOGT_READ, LOGD_NADC200, LOGK_DIRECT,            \
     /*!!! A hack: we use 'color' field to pass integer */  \
     NADC200_BIGC_BASE + (n),                               \
     PHYS_ID(NADC200_CHAN_BASE + (n*1) * NADC200_CHANS_PER_DEV),\
     .placement=plc                                         \
    }

    GLOBELEM_START("adc200s", "ADC200s", 1, 1)
        SUBWINV4_START(":", "ADC200s", 1)
            CANVAS_START(":", "ADCs", 2)
                ADC_LINE(0, "One", "left=0,right=0,top=0"),
                ADC_LINE(1, "Two", "left=0,right=0,top=8@adc_0,bottom=0"),
            SUBELEM_END("noshadow,notitle", ""),
        SUBWIN_END  (". . .", "one,noshadow,notitle,nocoltitles,norowtitles size=small,logc=important,resizable,compact,noclsbtn", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 0),

    GLOBELEM_START("s-m", "Settings, measurements", 5, 3 + (2<<24))
        COMM_SEL(0),
        COMM_SEL(1),
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

    GLOBELEM_START("View", "Calculated data", 1, 1)
        {"adc", NULL, NULL, NULL,
         " width=700 save_button src1=.adc_0 src2=.adc_1",
         NULL, LOGT_READ, LOGD_SUKHVIEW, LOGK_NOP,
        },
    GLOBELEM_END("nocoltitles,norowtitles", GU_FLAG_FROMNEWLINE),

    {NULL}
};
