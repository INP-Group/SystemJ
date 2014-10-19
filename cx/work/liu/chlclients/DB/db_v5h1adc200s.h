#include "liuclient.h"
#include "drv_i/adc200me_drv_i.h"

#include "common_elem_macros.h"


enum
{
    LIUBPMS_BIGC_BASE = 0,
    LIUBPMS_CHAN_BASE = 0,
};


#define ADC200ME_LINE(n, l, adc_opts)                                  \
    {                                                                  \
        &(elemnet_t){                                                  \
            "adc200me-"__CX_STRINGIZE(n), l,                           \
            NULL, NULL,                                                \
            ELEM_MULTICOL, 1, 1,                                       \
            (logchannet_t [])                                          \
            {                                                          \
                {"adc", l, NULL, NULL,                                 \
                 "width=700 height=200"                                \
                 " maxfrq=10 save_button subsys=\"adc-"__CX_STRINGIZE(n)"\""     \
                 " "adc_opts,                                          \
                 NULL, LOGT_READ, LOGD_LIU_ADC200ME_ONE, LOGK_DIRECT,  \
                 /*!!! A hack: we use 'color' field to pass integer */ \
                 LIUBPMS_BIGC_BASE + (n),                              \
                 PHYS_ID(LIUBPMS_CHAN_BASE + (n) * ADC200ME_NUM_PARAMS),\
                 /*.placement="horz=fill"*/                            \
                }                                                      \
            },                                                         \
            "titleatleft,nocoltitles,norowtitles,fold_h,one"           \
        },                                                             \
        GU_FLAG_FROMNEWLINE | GU_FLAG_FILLHORZ | (n==3? GU_FLAG_FILLVERT:0)                        \
    }

static groupunit_t v5h1adc200s_grouping[] =
{
    ADC200ME_LINE(0, "ADC #1",
                  " descrA=Chan1-1  descrB=Chan1-2"
                  " "
                  " "),
    ADC200ME_LINE(1, "ADC #2",
                  " descrA=Chan2-1 descrB=Chan2-2"
                  " "
                  " "),
    ADC200ME_LINE(2, "ADC #3",
                  " descrA=Chan3-1  descrB=Chan3-2"
                  //" coeffA=0.472 coeffB=2.796"
                  " "),
    {NULL}
};

static physprops_t v5h1adc200s_physinfo[] =
{
};
