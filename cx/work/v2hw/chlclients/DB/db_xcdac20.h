
#include "drv_i/xcdac20_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t xcdac20_grouping[] =
{

#define DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_OUT_n_base      + n), .minnorm=-10.0, .maxnorm=+10.0}, \
    {"dacspd" __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_OUT_RATE_n_base + n), .minnorm=-20.0, .maxnorm=+20.0}, \
    {"daccur" __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_OUT_CUR_n_base  + n), .minnorm=-10.0, .maxnorm=+10.0}

    {
        &(elemnet_t){
            "dac", "DAC", NULL, NULL,
            ELEM_MULTICOL, 2, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("set", "Settings",
                                  "Set, V\vMaxSpd, V/s\vCur, V", "0\v1\v2\v3",
                                  1*3, 3)
                    DAC_LINE(0),
                SUBELEM_END("noshadow,notitle,nohline,norowtitles", NULL),
                SUBELEM_START("mode", "Mode", 2, 2)
                    {"dac_state", "Mode:", NULL, NULL, "#+#TNorm\vTable", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_OUT_MODE), .placement="horz=right"},
                    //{"dac_table", "Table...", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},

#define DO_BUTTON(l, cn) \
    {NULL, l, NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}
                
                    SUBWIN_START("dac_table", "DAC table mode control", 3, 3)
                        EMPTY_CELL(),
                        {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP, LOGK_NOP, .placement="vert=fill"},
                        SUBELEM_START(NULL, NULL, 6, 1)
                            DO_BUTTON("Drop",     XCDAC20_CHAN_DO_TAB_DROP),
                            DO_BUTTON("Activate", XCDAC20_CHAN_DO_TAB_ACTIVATE),
                            DO_BUTTON("Start",    XCDAC20_CHAN_DO_TAB_START),
                            DO_BUTTON("Stop",     XCDAC20_CHAN_DO_TAB_STOP),
                            DO_BUTTON("Pause",    XCDAC20_CHAN_DO_TAB_PAUSE),
                            DO_BUTTON("Resume",   XCDAC20_CHAN_DO_TAB_RESUME),
                        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                    SUBWIN_END("Table...", "noshadow,notitle,nocoltitles", NULL),
                
                SUBELEM_END("noshadow,notitle,nohline,nocoltitles,norowtitles", NULL),
            },
            "nocoltitles,norowtitles"
        }, 1
    },

#define ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), NULL, NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_ADC_n_base+n), .minnorm=-10.0, .maxnorm=+10.0}

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
            ELEM_MULTICOL, 13, 1,
            (logchannet_t []){
                SUBELEM_START("ver", NULL, 2, 2)
                    {"hw", "HW", NULL, "%-2.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_HW_VER)},
                    {"sw", "SW", NULL, "%-2.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_SW_VER)},
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
                {"adc_time",  "",   NULL, NULL, "1ms\v2ms\v5ms\v10ms\v20ms\v40ms\v80ms\v160ms", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_ADC_TIMECODE), .placement="horz=right"},
                //{"adc_magn",  "",     NULL, NULL, "x1\vx10\v\nx100\v\nx1000",                     NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_ADC_GAIN), .placement="horz=right"},
                {"beg",       "Ch", NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_ADC_BEG), .minnorm=0, .maxnorm=XCDAC20_CHAN_ADC_n_count-1, .placement="horz=right"},
                {"end",       "-",  NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_ADC_END), .minnorm=0, .maxnorm=XCDAC20_CHAN_ADC_n_count-1, .placement="horz=right"},
                SEP_L(),
                {"calb", "Clbr DAC",   NULL, NULL, NULL,   NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_DO_CALB_DAC)},
                {"auto_calb", "Auto", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_AUTOCALB_ONOFF)},
                {"calb_period", "@",         "s",  "%6.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_AUTOCALB_SECS)},
                SEP_L(),
                {"dc_mode",   "Dig. corr.", NULL, NULL, NULL,   NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_DIGCORR_MODE)},
                {"dc_val",    "",   NULL, "%8.0f", NULL,   NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_DIGCORR_FACTOR)},
                SEP_L(),
                {"reset",     "Reset!",     NULL, NULL, "bg=red", NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_DO_RESET), .placement="horz=center"},
            },
            "nocoltitles,norowtitles"
        }, 0
    },

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_REGS_WR8_base+n)}
    
#define INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_REGS_RD8_base+n)}

    GLOBELEM_START_CRN("ioregs", "I/O",
                       "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR",
                       9*2, 9)
        {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_REGS_WR1)},
        OUTB_LINE(7),
        OUTB_LINE(6),
        OUTB_LINE(5),
        OUTB_LINE(4),
        OUTB_LINE(3),
        OUTB_LINE(2),
        OUTB_LINE(1),
        OUTB_LINE(0),
        {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCDAC20_CHAN_REGS_RD1)},
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


static physprops_t xcdac20_physinfo[] =
{
    {XCDAC20_CHAN_ADC_n_base +  0, 1000000.0, 0, 0},
    {XCDAC20_CHAN_ADC_n_base +  1, 1000000.0, 0, 0},
    {XCDAC20_CHAN_ADC_n_base +  2, 1000000.0, 0, 0},
    {XCDAC20_CHAN_ADC_n_base +  3, 1000000.0, 0, 0},
    {XCDAC20_CHAN_ADC_n_base +  4, 1000000.0, 0, 0},
    {XCDAC20_CHAN_ADC_n_base +  5, 1000000.0, 0, 0},
    {XCDAC20_CHAN_ADC_n_base +  6, 1000000.0, 0, 0},
    {XCDAC20_CHAN_ADC_n_base +  7, 1000000.0, 0, 0},

    {XCDAC20_CHAN_OUT_n_base + 0, 1000000.0, 0, 305},
    {XCDAC20_CHAN_OUT_RATE_n_base + 0, 1000000.0, 0, 305},
    {XCDAC20_CHAN_OUT_CUR_n_base + 0, 1000000.0, 0, 305},
};
