#include "drv_i/xceac124_drv_i.h"
#include "drv_i/cgvi8_drv_i.h"
#include "drv_i/vsdc2_drv_i.h"

#include "common_elem_macros.h"


enum
{
    C124_BASE   = 0,
    CGVI_BASE   = C124_BASE + XCEAC124_NUMCHANS,
    BII0_BASE   = CGVI_BASE + CGVI8_NUMCHANS,
    BII1_BASE   = BII0_BASE + VSDC2_NUMCHANS,
    TGRD_BASE   = BII1_BASE + VSDC2_NUMCHANS,
};

enum
{
    REG_START_MODE = 90,
    REG_START_SECS = 91,
    REG_START_PREV = 92,
};

enum
{
    MODE_XTRN = 0,
    MODE_AUTO = 1,
    MODE_ONCE = 2,
};

static groupunit_t rstand_grouping[] =
{

    
#define MAIN_LINE(l, n) \
    {"-onoff"l, "", NULL, NULL, "#-#TВыкл\f\flit=red\vВкл\f\flit=green", \
        NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_REGS_WR8_BASE+n)}, \
    {"err"l, "Ошибка",     NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_REGS_RD8_BASE+n), .minyelw=0, .maxyelw=0.5}, \
    {"ena"l,    "",        NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI_BASE+CGVI8_CHAN_MASK1_N_BASE +4+n), .placement="horz=center"}, \
    {"U"l"set", "U"l"уст", NULL, "%7.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_OUT_n_BASE+n), .minnorm=-1000, .maxnorm=+1000}, \
    {"U"l"mes", "U"l"изм", NULL, "%7.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(/*C124_BASE+XCEAC124_CHAN_ADC_n_BASE+(n)*2+0*/TGRD_BASE+n), .mindisp=-1000, .maxdisp=+1000}, \
    {"U"l"uvh", "U"l"увх", NULL, "%7.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_ADC_n_BASE+(n)*2+1), .mindisp=-1000, .maxdisp=+1000}, \
    {"I"l"mes", "I"l"изм", NULL, "%7.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((n)<2? BII0_BASE+VSDC2_CHAN_INT0+(n) : BII1_BASE+VSDC2_CHAN_INT0+(n)-2)}, \
    {"D"l"beg", "З"l"зап", NULL, "%6.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI_BASE+CGVI8_CHAN_QUANT_N_BASE +4+n), .minnorm=0, .maxnorm=6553.5},     \
    {"stp"l,    "",        NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI_BASE+CGVI8_CHAN_MASK1_N_BASE +0+n)}, \
    {"D"l"end", "З"l"стп", NULL, "%6.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI_BASE+CGVI8_CHAN_QUANT_N_BASE +0+n), .minnorm=0, .maxnorm=6553.5}

    {
        &(elemnet_t){
            "matrix", "Уставки/измерения",
            "Заряд\v \vЗапуск\vUуст, V\vUизм, V\vUувх, V\vIизм, A\vГВИ: старт\v \vстоп, us", "1\v2\v3\v4",
            ELEM_MULTICOL, 10*4, 10,
            (logchannet_t []){
                MAIN_LINE("1", 0),
                MAIN_LINE("2", 1),
                MAIN_LINE("3", 2),
                MAIN_LINE("4", 3),
            },
            ""
        }, 1*1
    },

#define START_MODE_FLAS(v)                                       \
    (excmd_t[]){                                                 \
        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_START_MODE),    \
                                                                 \
        CMD_GETLCLREG_I(REG_START_MODE), CMD_SUB_I(v), CMD_ABS,  \
        CMD_PUSH_I(0.001), CMD_IF_LT_TEST, CMD_BREAK_I(1.0),     \
        CMD_RET_I(0),                                            \
    },                                                           \
    (excmd_t[]){                                                 \
        CMD_PUSH_I(v), CMD_SETLCLREG_I(REG_START_MODE),          \
        CMD_REFRESH_I(1),                                        \
        CMD_RET                                                  \
    }
    
    {
        &(elemnet_t){
            "triggers", "Запуски",
            "", "",
            ELEM_MULTICOL, 6, 2,
            (logchannet_t []){
                {"-start-ext", " Внешние ", NULL, NULL, "panel,color=orange", NULL, LOGT_WRITE1, LOGD_RADIOBTN, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    START_MODE_FLAS(MODE_XTRN), .placement="horz=fill"},
                EMPTY_CELL(),
                {"-start-prg", " От ЭВМ ", NULL, NULL, "panel,color=green", NULL, LOGT_WRITE1, LOGD_RADIOBTN, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    START_MODE_FLAS(MODE_AUTO), .placement="horz=fill"},
                {"period", "Раз в", "s", "%4.1f", "withlabel,withunits", NULL,
                    LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t[]){
                        // Set default values
                        CMD_PUSH_I(10), CMD_SETLCLREGDEFVAL_I(REG_START_SECS),
                        CMD_PUSH_I(0),  CMD_SETLCLREGDEFVAL_I(REG_START_PREV),

                        // Is mode ~= MODE_AUTO?
                        CMD_GETLCLREG_I(REG_START_MODE), CMD_SUB_I(MODE_AUTO), CMD_ABS,
                        CMD_PUSH_I(0.001), CMD_IF_GE_TEST, CMD_GOTO_I("ret"),

                        // Is period sane?
                        CMD_GETLCLREG_I(REG_START_SECS), CMD_PUSH_I(0.5),
                        CMD_IF_LT_TEST, CMD_GOTO_I("ret"),

                        // Enough time from previous pulse?
                        CMD_GETTIME,
                        CMD_GETLCLREG_I(REG_START_PREV),
                        CMD_GETLCLREG_I(REG_START_SECS),
                        CMD_ADD,
                        CMD_IF_LT_TEST, CMD_GOTO_I("ret"),

                        // Yeah, we MUST do pulse!
                        CMD_GETTIME, CMD_SETLCLREG_I(REG_START_PREV),
                        CMD_PUSH_I(CX_VALUE_COMMAND),
                        CMD_ALLOW_W, CMD_SETP_I(CGVI_BASE + CGVI8_CHAN_PROGSTART),
                        //CMD_DEBUGP_I("zzz!"),
                        
                        CMD_LABEL_I("ret"),
                        CMD_GETLCLREG_I(REG_START_SECS), CMD_RET,
                    },
                    (excmd_t[]){
                        CMD_QRYVAL, CMD_SETLCLREG_I(REG_START_SECS),
                        CMD_RET
                    },
                .minnorm=0.5, .maxnorm=3600, .incdec_step = 0.5},
                {"-start-one", " Однократно ", NULL, NULL, "panel,color=white", NULL, LOGT_WRITE1, LOGD_RADIOBTN, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    START_MODE_FLAS(MODE_ONCE)},
                {"-once", "Стук!", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI_BASE + CGVI8_CHAN_PROGSTART), .placement="horz=center"},
            },
            "nocoltitles,norowtitles"
        }, 1*0
    },

    {
        &(elemnet_t){
            "histplot", "Histplot",
            "", "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"histplot", "HistPlot", NULL, NULL, "width=700,height=300,plot1=.U1mes,plot2=.U1uvh,plot3=.U1set", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
            },
            "notitle,nocoltitles,norowtitles"
        }, 1*1
    },

#if 0
    {
        &(elemnet_t){
            "ceac124", "CEAC124",
            "", "Время инт.\vДиапазон",
            ELEM_MULTICOL, 2, 1,
            (logchannet_t []){
                {"adc_time",  "", NULL, NULL, "1ms\v2ms\v5ms\v10ms\v20ms\v40ms\v80ms\v160ms", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE + XCEAC124_CHAN_ADC_TIMECODE)},
                {"adc_magn",  "",     NULL, NULL, "x1 (\xB1""10V)\vx10 (\xB1""1V)\v\nx100 (\xB1""0.1V)\v\nx1000 (\xB1""0.01V)",                     NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE + XCEAC124_CHAN_ADC_GAIN)},
            },
            "nocoltitles"
        }, 1*0
    },
#endif
    
    {NULL}
};

static physprops_t rstand_physinfo[] =
{
    {TGRD_BASE +                             0, 1000000.0/100, 0, 0},
    {TGRD_BASE +                             1, 1000000.0/100, 0, 0},
    {TGRD_BASE +                             2, 1000000.0/100, 0, 0},
    {TGRD_BASE +                             3, 1000000.0/100, 0, 0},

    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  0, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  1, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  2, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  3, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  4, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  5, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  6, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  7, 1000000.0/100, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  8, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE +  9, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE + 10, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE + 11, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE + 12, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE + 13, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE + 14, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_ADC_n_BASE + 15, 1000000.0, 0, 0},
    {C124_BASE + XCEAC124_CHAN_OUT_n_BASE +  0, 1000000.0/100, 0, 305},
    {C124_BASE + XCEAC124_CHAN_OUT_n_BASE +  1, 1000000.0/100, 0, 305},
    {C124_BASE + XCEAC124_CHAN_OUT_n_BASE +  2, 1000000.0/100, 0, 305},
    {C124_BASE + XCEAC124_CHAN_OUT_n_BASE +  3, 1000000.0/100, 0, 305},

    /* Note: quant-channels are in 100ns quants; we need us; 1us/100ns=10 */
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+0, 10},
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+1, 10},
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+2, 10},
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+3, 10},
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+4, 10},
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+5, 10},
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+6, 10},
    {CGVI_BASE + CGVI8_CHAN_QUANT_N_BASE+7, 10},

    /* 278353/370.0A */
    {BII0_BASE + VSDC2_CHAN_INT0, 278353/370., 0, 0},
    {BII0_BASE + VSDC2_CHAN_INT1, 278353/370., 0, 0},
    {BII1_BASE + VSDC2_CHAN_INT0, 278353/370., 0, 0},
    {BII1_BASE + VSDC2_CHAN_INT1, 278353/370., 0, 0},
};
