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
                 " foldctls width=700 height=200"                      \
                 " save_button subsys=\"bpm-adc-"__CX_STRINGIZE(n)"\"" \
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

static groupunit_t liubpms_grouping[] =
{
    ADC200ME_LINE(0, "Сумма X; Сумма Y",
                  " descrA=СумX  descrB=СумY"
                  " "
                  " "),
    ADC200ME_LINE(1, "Разность X; Разность Y",
                  " descrA=РазнX descrB=РазнY"
                  " "
                  " "),
    ADC200ME_LINE(2, "Трансф. тока 1; Шарик",
                  " descrA=ТрI1  descrB=Шарик"
//                  " coeffA=0.540 coeffB=435.7 coeffA=701.7 coeffB=626" //435.7=233*1.87 = 233*(1k||1k63+540)/(k||1k63)
                  " coeffA=0.472 coeffB=2.796"
                  " unitsA=kA    unitsB=MV"),
    ADC200ME_LINE(3, "Трансф. тока 2; Цил. Фарадея",
                  " descrA=ТрI2  descrB=Ц.Фар"
                  " coeffA=0.433 coeffB=0.583"
                  " unitsA=kA    unitsB=kA"),

    {NULL}
};

static physprops_t liubpms_physinfo[] =
{
};
