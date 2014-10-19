
#include "drv_i/xcanadc40_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t xcanadc40_grouping[] =
{

#define ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANADC40_CHAN_ADC_n_base+n), .minnorm=-10.0, .maxnorm=+10.0}

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANADC40_CHAN_REGS_WR8_base+n)}
    
#define INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANADC40_CHAN_REGS_RD8_base+n)}
    
    GLOBELEM_START_CRN("adc", "ADC",
                       "U, V", NULL,
                       40, 1 + 8*65536)
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
        ADC_LINE(24),
        ADC_LINE(25),
        ADC_LINE(26),
        ADC_LINE(27),
        ADC_LINE(28),
        ADC_LINE(29),
        ADC_LINE(30),
        ADC_LINE(31),
        ADC_LINE(32),
        ADC_LINE(33),
        ADC_LINE(34),
        ADC_LINE(35),
        ADC_LINE(36),
        ADC_LINE(37),
        ADC_LINE(38),
        ADC_LINE(39),
    GLOBELEM_END("", GU_FLAG_FROMNEWLINE),

    GLOBELEM_START_CRN("ioregs", "I/O",
                       "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR",
                       9*2, 9)
        {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANADC40_CHAN_REGS_WR1)},
        OUTB_LINE(7),
        OUTB_LINE(6),
        OUTB_LINE(5),
        OUTB_LINE(4),
        OUTB_LINE(3),
        OUTB_LINE(2),
        OUTB_LINE(1),
        OUTB_LINE(0),
        {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANADC40_CHAN_REGS_RD1)},
        INPB_LINE(7),
        INPB_LINE(6),
        INPB_LINE(5),
        INPB_LINE(4),
        INPB_LINE(3),
        INPB_LINE(2),
        INPB_LINE(1),
        INPB_LINE(0),
    GLOBELEM_END("", GU_FLAG_FROMNEWLINE),

    {NULL}
};


static physprops_t xcanadc40_physinfo[] =
{
    {XCANADC40_CHAN_ADC_n_base +  0, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  1, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  2, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  3, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  4, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  5, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  6, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  7, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  8, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base +  9, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 10, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 11, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 12, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 13, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 14, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 15, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 16, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 17, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 18, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 19, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 20, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 21, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 22, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 23, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 24, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 25, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 26, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 27, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 28, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 29, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 30, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 31, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 32, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 33, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 34, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 35, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 36, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 37, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 38, 1000000.0, 0, 0},
    {XCANADC40_CHAN_ADC_n_base + 39, 1000000.0, 0, 0},
};
