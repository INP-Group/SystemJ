
#include "drv_i/c0642_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t c0642_grouping[] =
{

#define DAC_LINE(n) \
    {"dac" __CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%9.6f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C0642_CHAN_OUT_n_base+n), .minnorm=-10.0, .maxnorm=+10.0}

    GLOBELEM_START_CRN("dac", "DAC", "U, V", NULL, 16, 1 + 8*65536)
        DAC_LINE(0),
        DAC_LINE(1),
        DAC_LINE(2),
        DAC_LINE(3),
        DAC_LINE(4),
        DAC_LINE(5),
        DAC_LINE(6),
        DAC_LINE(7),
        DAC_LINE(8),
        DAC_LINE(9),
        DAC_LINE(10),
        DAC_LINE(11),
        DAC_LINE(12),
        DAC_LINE(13),
        DAC_LINE(14),
        DAC_LINE(15),
    GLOBELEM_END("notitle,noshadow", 0),

    {NULL}
};


static physprops_t c0642_physinfo[] =
{
    {C0642_CHAN_OUT_n_base +  0, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  1, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  2, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  3, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  4, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  5, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  6, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  7, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  8, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base +  9, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base + 10, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base + 11, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base + 12, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base + 13, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base + 14, 1000000.0, 0, 305},
    {C0642_CHAN_OUT_n_base + 15, 1000000.0, 0, 305},
};
