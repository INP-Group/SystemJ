#include "drv_i/xcac208_drv_i.h"

#include "common_elem_macros.h"


#define A_LINE(n) \
    {NULL, NULL, NULL, "%5.0f", "", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_CHAN_OUT_n_BASE + n)},     \
    {NULL, NULL, NULL, "%5.0f", "", NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_CHAN_ADC_n_BASE + n * 2 + 0)}, \
    {NULL, NULL, NULL, "%6.2f", "", NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_CHAN_ADC_n_BASE + n * 2 + 1)}

static groupunit_t z0fed208_grouping[] =
{
    GLOBELEM_START_CRN(NULL, NULL, "Iset\vImes\vUmes", "0\v1\v2\v3\v4\v5", 6*3, 3)
        A_LINE(0),
        A_LINE(1),
        A_LINE(2),
        A_LINE(3),
        A_LINE(4),
        A_LINE(5),
    GLOBELEM_END("noshadow,notitle", 0),

    {NULL}
};


#define A_PHYS(n) \
    {XCAC208_CHAN_OUT_n_BASE + n,         5.4/10.0*1000, 0.0, 305}, \
    {XCAC208_CHAN_ADC_n_BASE + n * 2 + 0, 5.4/10.0*1000, 0.0, 305}, \
    {XCAC208_CHAN_ADC_n_BASE + n * 2 + 1, 1000000.0/3.7, 0.0, 305}

static physprops_t z0fed208_physinfo[] =
{
    A_PHYS(0),
    A_PHYS(1),
    A_PHYS(2),
    A_PHYS(3),
    A_PHYS(4),
    A_PHYS(5),
};
