
#include "drv_i/xceac124_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t xceac124_grouping[] =
{

#define XCEAC124_DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_OUT_n_BASE      + n), .minnorm=-10.0, .maxnorm=+10.0}, \
    {"dacspd" __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_OUT_RATE_n_BASE + n), .minnorm=-20.0, .maxnorm=+20.0}, \
    {"daccur" __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_OUT_CUR_n_BASE  + n), .minnorm=-10.0, .maxnorm=+10.0}

    GLOBELEM_START("dac", "DAC", 2, 1)
        SUBELEM_START_CRN("set", "Settings",
                          "Set, V\vMaxSpd, V/s\vCur, V", "0\v1\v2\v3",
                          4*3, 3)
            XCEAC124_DAC_LINE(0),
            XCEAC124_DAC_LINE(1),
            XCEAC124_DAC_LINE(2),
            XCEAC124_DAC_LINE(3),
        SUBELEM_END("noshadow,notitle,nohline", NULL),
        SUBELEM_START("mode", "Mode", 2, 2)
            {"dac_state", "Mode:", NULL, NULL, "#+#TNorm\vTable", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_OUT_MODE), .placement="horz=right"},
            //{"dac_table", "Table...", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},

#define DO_BUTTON(l, cn) \
    {NULL, l, NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}
                
            SUBWIN_START("dac_table", "DAC table mode control", 3, 3)
                EMPTY_CELL(),
                {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP, LOGK_NOP, .placement="vert=fill"},
                SUBELEM_START(NULL, NULL, 6, 1)
                    DO_BUTTON("Drop",     XCEAC124_CHAN_DO_TAB_DROP),
                    DO_BUTTON("Activate", XCEAC124_CHAN_DO_TAB_ACTIVATE),
                    DO_BUTTON("Start",    XCEAC124_CHAN_DO_TAB_START),
                    DO_BUTTON("Stop",     XCEAC124_CHAN_DO_TAB_STOP),
                    DO_BUTTON("Pause",    XCEAC124_CHAN_DO_TAB_PAUSE),
                    DO_BUTTON("Resume",   XCEAC124_CHAN_DO_TAB_RESUME),
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            SUBWIN_END("Table...", "noshadow,notitle,nocoltitles", NULL),
        
        SUBELEM_END("noshadow,notitle,nohline,nocoltitles,norowtitles", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 1),

#define XCEAC124_ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), NULL, NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_ADC_n_BASE+n), .minnorm=-10.0, .maxnorm=+10.0}

    GLOBELEM_START("adc", "ADC", 3, 1)
        SUBELEM_START_CRN("mes", "Measurements",
                          "U, V", "0\v1\v2\v3\v4\v5\v6\v7\v8\v9\v10\v11\vT\vPWR\v10V\v0V",
                          16, 1 + 4*65536)
            XCEAC124_ADC_LINE(0),
            XCEAC124_ADC_LINE(1),
            XCEAC124_ADC_LINE(2),
            XCEAC124_ADC_LINE(3),
            XCEAC124_ADC_LINE(4),
            XCEAC124_ADC_LINE(5),
            XCEAC124_ADC_LINE(6),
            XCEAC124_ADC_LINE(7),
            XCEAC124_ADC_LINE(8),
            XCEAC124_ADC_LINE(9),
            XCEAC124_ADC_LINE(10),
            XCEAC124_ADC_LINE(11),
            XCEAC124_ADC_LINE(12),
            XCEAC124_ADC_LINE(13),
            XCEAC124_ADC_LINE(14),
            XCEAC124_ADC_LINE(15),
        SUBELEM_END("noshadow,notitle,nohline", NULL),
        SUBELEM_START("mode", "Mode", 4, 4)
            {"adc_state", "Mode:",     NULL, NULL, "#+#TNorm\vOscill\vPlot\vT-back", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_ADC_MODE), .placement="horz=right"},
            {"adc_osc",   "Oscill...", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
            {"adc_plot",  "Plot...",   NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
            {"adc_tback", "T-back...", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
        SUBELEM_END("noshadow,notitle,nohline,nocoltitles,norowtitles", NULL),
        SUBELEM_START("params", "Parameters", 4, 4)
            {"adc_time",  "",   NULL, NULL,    "1ms\v2ms\v5ms\v10ms\v20ms\v40ms\v80ms\v160ms", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_ADC_TIMECODE)},
            {"adc_magn",  "",   NULL, NULL,    "x1\vx10\v\nx100\v\nx1000",                     NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_ADC_GAIN)},
            {"beg",       "Ch", NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_ADC_BEG), .minnorm=0, .maxnorm=XCEAC124_CHAN_ADC_n_COUNT-1},
            {"end",       "-",  NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_ADC_END), .minnorm=0, .maxnorm=XCEAC124_CHAN_ADC_n_COUNT-1},
        SUBELEM_END("noshadow,notitle,nohline,nocoltitles,norowtitles", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 1),

#define XCEAC124_OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_REGS_WR8_BASE+n)}
    
#define XCEAC124_INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_REGS_RD8_BASE+n)}

    GLOBELEM_START_CRN("ioregs", "I/O",
                       "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR",
                       9*2, 9)
        {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_REGS_WR1)},
        XCEAC124_OUTB_LINE(7),
        XCEAC124_OUTB_LINE(6),
        XCEAC124_OUTB_LINE(5),
        XCEAC124_OUTB_LINE(4),
        XCEAC124_OUTB_LINE(3),
        XCEAC124_OUTB_LINE(2),
        XCEAC124_OUTB_LINE(1),
        XCEAC124_OUTB_LINE(0),
        {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_REGS_RD1)},
        XCEAC124_INPB_LINE(7),
        XCEAC124_INPB_LINE(6),
        XCEAC124_INPB_LINE(5),
        XCEAC124_INPB_LINE(4),
        XCEAC124_INPB_LINE(3),
        XCEAC124_INPB_LINE(2),
        XCEAC124_INPB_LINE(1),
        XCEAC124_INPB_LINE(0),
    GLOBELEM_END("", 1),

    GLOBELEM_START("control", "Modes", 1, 1)
        {"reset", "Reset!", NULL, NULL, "bg=red", NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCEAC124_CHAN_DO_RESET)},
    GLOBELEM_END("noshadow,notitle,nocoltitles,norowtitles", 0),

    {NULL}
};


static physprops_t xceac124_physinfo[] =
{
    {XCEAC124_CHAN_ADC_n_BASE       +  0, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  1, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  2, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  3, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  4, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  5, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  6, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  7, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  8, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       +  9, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       + 10, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       + 11, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       + 12, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       + 13, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       + 14, 1000000.0, 0, 0},
    {XCEAC124_CHAN_ADC_n_BASE       + 15, 1000000.0, 0, 0},

    {XCEAC124_CHAN_OUT_n_BASE       + 0, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_n_BASE       + 1, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_n_BASE       + 2, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_n_BASE       + 3, 1000000.0, 0, 305},

    {XCEAC124_CHAN_OUT_RATE_n_BASE  + 0, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_RATE_n_BASE  + 1, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_RATE_n_BASE  + 2, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_RATE_n_BASE  + 3, 1000000.0, 0, 305},

    {XCEAC124_CHAN_OUT_CUR_n_BASE   + 0, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_CUR_n_BASE   + 1, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_CUR_n_BASE   + 2, 1000000.0, 0, 305},
    {XCEAC124_CHAN_OUT_CUR_n_BASE   + 3, 1000000.0, 0, 305},
};
