
#include "drv_i/vadc16_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t vadc16_grouping[] =
{

#define ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), NULL, NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VADC16_CHAN_ADC_n_BASE+n), .minnorm=-10.0, .maxnorm=+10.0}

    {
        &(elemnet_t){
            "adc", "ADC, V", NULL, NULL,
            ELEM_MULTICOL, 2, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("mes", "Measurements",
                                  "U, V", "0\v1\v2\v3\v4\v5\v6\v7\v8\v9\v10\v11\v12\v13\v14\v15\v10V\vGND\vT\vG19\vG20\vG21\vG22\vG23",
                                  24, 1 + 8*65536)
                    ADC_LINE(0),
                    ADC_LINE(1),
                    ADC_LINE(2),
                    ADC_LINE(3),
                    ADC_LINE(4),
                    ADC_LINE(5),
                    ADC_LINE(6),
                    ADC_LINE(7),
                    ADC_LINE(8),
                    ADC_LINE(9),
                    ADC_LINE(10),
                    ADC_LINE(11),
                    ADC_LINE(12),
                    ADC_LINE(13),
                    ADC_LINE(14),
                    ADC_LINE(15),
                    ADC_LINE(16),
                    ADC_LINE(17),
                    ADC_LINE(18),
                    ADC_LINE(19),
                    ADC_LINE(20),
                    ADC_LINE(21),
                    ADC_LINE(22),
                    ADC_LINE(23),
                SUBELEM_END("noshadow,notitle,nohline,nocoltitles", NULL),
                SUBELEM_START("params", "Parameters", 3, 3)
                    {"adc_time",  "", NULL, NULL, "1ms\v2ms\v5ms\v10ms\v20ms\v40ms\v80ms\v160ms", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VADC16_CHAN_ADC_TIMECODE)},
                    {"beg",       "Ch", NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VADC16_CHAN_ADC_BEG), .minnorm=0, .maxnorm=VADC16_CHAN_ADC_n_COUNT-1},
                    {"end",       "-",  NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VADC16_CHAN_ADC_END), .minnorm=0, .maxnorm=VADC16_CHAN_ADC_n_COUNT-1},
                SUBELEM_END("noshadow,notitle,nohline,nocoltitles,norowtitles", NULL),
            },
            "nocoltitles,norowtitles"
        }, 1
    },

    {NULL}
};


static physprops_t vadc16_physinfo[] =
{
    {VADC16_CHAN_ADC_n_BASE +  0, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  1, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  2, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  3, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  4, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  5, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  6, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  7, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  8, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE +  9, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 10, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 11, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 12, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 13, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 14, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 15, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 16, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 17, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 18, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 19, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 20, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 21, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 22, 1000000.0, 0, 0},
    {VADC16_CHAN_ADC_n_BASE + 23, 1000000.0, 0, 0},
};
