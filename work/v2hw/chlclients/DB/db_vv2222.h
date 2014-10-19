
#include "drv_i/vv2222_drv_i.h"
#include "common_elem_macros.h"


#define VV2222_LINE(n) \
    {"dac"__CX_STRINGIZE(n), "DAC"__CX_STRINGIZE(n), NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VV2222_CHAN_OUT_n_BASE    + n)}, \
    {"adc"__CX_STRINGIZE(n), "ADC"__CX_STRINGIZE(n), NULL, "%7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VV2222_CHAN_ADC_n_BASE    + n)}, \
    {"or" __CX_STRINGIZE(n), NULL,                   NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VV2222_CHAN_REGS_WR8_BASE + n)}, \
    {"ir" __CX_STRINGIZE(n), NULL,                   NULL, NULL,    NULL, NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VV2222_CHAN_REGS_RD8_BASE + n)}

static groupunit_t vv2222_grouping[] =
{
    GLOBELEM_START_CRN("all", NULL, "DAC, V\vADC, V\v", "0\v1\v2\v3", 4*4, 4)
        VV2222_LINE(0),
        VV2222_LINE(1),
        VV2222_LINE(2),
        VV2222_LINE(3),
    GLOBELEM_END("noshadow,notitle", 0),

    {NULL}
};

static physprops_t vv2222_physinfo[] =
{
    {VV2222_CHAN_ADC_n_BASE +  0, 1000000.0, 0, 0},
    {VV2222_CHAN_ADC_n_BASE +  1, 1000000.0, 0, 0},
    {VV2222_CHAN_ADC_n_BASE +  2, 1000000.0, 0, 0},
    {VV2222_CHAN_ADC_n_BASE +  3, 1000000.0, 0, 0},

    {VV2222_CHAN_OUT_n_BASE +  0, 1000000.0, 0, 305},
    {VV2222_CHAN_OUT_n_BASE +  1, 1000000.0, 0, 305},
    {VV2222_CHAN_OUT_n_BASE +  2, 1000000.0, 0, 305},
    {VV2222_CHAN_OUT_n_BASE +  3, 1000000.0, 0, 305},
};
