
#include "drv_i/xcandac16_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t xcandac16_grouping[] =
{
#define DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_OUT_n_base      + n), .minnorm=-10.0, .maxnorm=+10.0}, \
    {"dacspd" __CX_STRINGIZE(n), NULL,              NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_OUT_RATE_n_base + n), .minnorm=-20.0, .maxnorm=+20.0}, \
    {"daccur" __CX_STRINGIZE(n), NULL,              NULL, "%7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_OUT_CUR_n_base  + n), .minnorm=-10.0, .maxnorm=+10.0}

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_REGS_WR8_base+n)}
    
#define INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_REGS_RD8_base+n)}

    
    GLOBELEM_START_CRN("adc", "DAC", "Set, V\vMaxSpd, V/s\vCur, V", NULL, 2 + XCANDAC16_CHAN_OUT_n_count*3, 3 + (2 << 24))
        {"dac_state", "Mode:", NULL, NULL, "#+#TNorm\vTable", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_OUT_MODE)},
        SUBWIN_START("dac_table", "DAC table mode control", 3, 3)
            EMPTY_CELL(),
            {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP, LOGK_NOP, .placement="vert=fill"},
            SUBELEM_START(NULL, NULL, 6, 1)
                BUTT_CELL(NULL, "Drop",     XCANDAC16_CHAN_DO_TAB_DROP),
                BUTT_CELL(NULL, "Activate", XCANDAC16_CHAN_DO_TAB_ACTIVATE),
                BUTT_CELL(NULL, "Start",    XCANDAC16_CHAN_DO_TAB_START),
                BUTT_CELL(NULL, "Stop",     XCANDAC16_CHAN_DO_TAB_STOP),
                BUTT_CELL(NULL, "Pause",    XCANDAC16_CHAN_DO_TAB_PAUSE),
                BUTT_CELL(NULL, "Resume",   XCANDAC16_CHAN_DO_TAB_RESUME),
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
        SUBWIN_END("Table...", "noshadow,notitle,nocoltitles", NULL),

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
    GLOBELEM_END("", GU_FLAG_FROMNEWLINE),

    GLOBELEM_START_CRN("ioregs", "I/O",
                       "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR",
                       9*2, 9)
        {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_REGS_WR1)},
        OUTB_LINE(7),
        OUTB_LINE(6),
        OUTB_LINE(5),
        OUTB_LINE(4),
        OUTB_LINE(3),
        OUTB_LINE(2),
        OUTB_LINE(1),
        OUTB_LINE(0),
        {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCANDAC16_CHAN_REGS_RD1)},
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


static physprops_t xcandac16_physinfo[] =
{
    {XCANDAC16_CHAN_OUT_n_base +  0, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  1, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  2, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  3, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  4, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  5, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  6, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  7, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  8, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base +  9, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base + 10, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base + 11, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base + 12, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base + 13, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base + 14, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_n_base + 15, 1000000.0, 0, 305},

    {XCANDAC16_CHAN_OUT_RATE_n_base +  0, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  1, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  2, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  3, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  4, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  5, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  6, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  7, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  8, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base +  9, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base + 10, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base + 11, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base + 12, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base + 13, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base + 14, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_RATE_n_base + 15, 1000000.0, 0, 305},

    {XCANDAC16_CHAN_OUT_CUR_n_base +  0, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  1, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  2, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  3, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  4, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  5, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  6, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  7, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  8, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base +  9, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base + 10, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base + 11, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base + 12, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base + 13, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base + 14, 1000000.0, 0, 305},
    {XCANDAC16_CHAN_OUT_CUR_n_base + 15, 1000000.0, 0, 305},
};
