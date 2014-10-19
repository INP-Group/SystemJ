
#include "drv_i/vdac20_drv_i.h"
#include "common_elem_macros.h"

static groupunit_t vdac20_grouping[] =
{

#define DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VDAC20_CHAN_OUT_n_BASE      + n), .minnorm=-10.0, .maxnorm=+10.0}

    {
        &(elemnet_t){
            "dac", "DAC", NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("set", "Settings",
                                  "Set, V\vMaxSpd, V/s\vCur, V", "0\v1\v2\v3",
                                  1*1, 1)
                    DAC_LINE(0),
                SUBELEM_END("noshadow,notitle,nohline,norowtitles", NULL),
            },
            "nocoltitles,norowtitles"
        }, 1
    },

#define ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), NULL, NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VDAC20_CHAN_ADC_n_BASE+n), .minnorm=-10.0, .maxnorm=+10.0}

    {
        &(elemnet_t){
            "vlotages", "ADC, V", NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("mes", "Measurements",
                                  "U, V", "0\v1\v2\v3\v4\vDAC\v0V\v10V",
                                  8, 1)
                    ADC_LINE(0),
                    ADC_LINE(1),
                    ADC_LINE(2),
                    ADC_LINE(3),
                    ADC_LINE(4),
                    ADC_LINE(5),
                    ADC_LINE(6),
                    ADC_LINE(7),
                SUBELEM_END("noshadow,notitle,nohline,nocoltitles", NULL),
            },
            "nocoltitles,norowtitles"
        }, 1
    },

    {
        &(elemnet_t){
            "control", "Control", NULL, NULL,
            ELEM_MULTICOL, 3, 1,
            (logchannet_t []){
                {"calibrate", "Clbr DAC",   NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VDAC20_CHAN_DO_CALIBRATE)},
                {"dc_mode",   "Dig. corr.", NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_ONOFF,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VDAC20_CHAN_DIGCORR_MODE)},
                {"dc_val",    "",           NULL, "%7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VDAC20_CHAN_DIGCORR_V)},
            },
            "nocoltitles,norowtitles"
        }, 0
    },

    {NULL}
};


static physprops_t vdac20_physinfo[] =
{
    {VDAC20_CHAN_ADC_n_BASE +  0, 1000000.0, 0, 0},
    {VDAC20_CHAN_ADC_n_BASE +  1, 1000000.0, 0, 0},
    {VDAC20_CHAN_ADC_n_BASE +  2, 1000000.0, 0, 0},
    {VDAC20_CHAN_ADC_n_BASE +  3, 1000000.0, 0, 0},
    {VDAC20_CHAN_ADC_n_BASE +  4, 1000000.0, 0, 0},
    {VDAC20_CHAN_ADC_n_BASE +  5, 1000000.0, 0, 0},
    {VDAC20_CHAN_ADC_n_BASE +  6, 1000000.0, 0, 0},
    {VDAC20_CHAN_ADC_n_BASE +  7, 1000000.0, 0, 0},

    {VDAC20_CHAN_OUT_n_BASE + 0, 1000000.0, 0, 305},
};
