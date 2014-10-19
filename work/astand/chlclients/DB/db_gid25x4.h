#include "drv_i/gid25x4_drv_i.h"

#include "common_elem_macros.h"


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

static groupunit_t gid25x4_grouping[] =
{

    
#define MAIN_LINE(l, n) \
    {"-onoff"l, "", NULL, NULL, "#-#TВыкл\f\flit=red\vВкл\f\flit=green", \
        NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_ONOFF_base+n)}, \
    {"err"l, "Ошибка",     NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_ERROR_base+n), .minyelw=0, .maxyelw=0.5}, \
    {"ena"l,    "",        NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_ENABLE_base+n), .placement="horz=center"}, \
    {"U"l"set", "U"l"уст", NULL, "%7.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_USET_base  +n), .minnorm=-1000, .maxnorm=+1000}, \
    {"U"l"mes", "U"l"изм", NULL, "%7.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_UMES_base  +n), .mindisp=-1000, .maxdisp=+1000}, \
    {"U"l"uvh", "U"l"увх", NULL, "%7.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_UUVH_base  +(n)*2), .mindisp=-1000, .maxdisp=+1000}, \
    {"I"l"mes", "I"l"изм", NULL, "%7.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((n)<2? GID25X4_CHAN_IMES01_base+(n) : GID25X4_CHAN_IMES23_base+(n)-2)}, \
    {"D"l"beg", "З"l"зап", NULL, "%6.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_DBEG_base  +n), .minnorm=0, .maxnorm=6553.5},     \
    {"stp"l,    "",        NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_EN_STP_base+n)}, \
    {"D"l"end", "З"l"стп", NULL, "%6.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_DEND_base  +n), .minnorm=0, .maxnorm=6553.5}

    GLOBELEM_START_CRN("matrix", "Уставки/измерения",
                       "Заряд\v \vЗапуск\vUуст, V\vUизм, V\vUувх, V\vIизм, A\vГВИ: старт\v \vстоп, us", "1\v2\v3\v4",
                       10*4 + 1, 10 + (1 << 24))

        SUBWINV4_START(NULL, "CEAC124", 1)
            SUBELEM_START(":", NULL, 3, 3)

#define XCEAC124_DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_OUT_n_BASE      + n)}, \
    {"dacspd" __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_OUT_RATE_n_BASE + n)}, \
    {"daccur" __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_OUT_CUR_n_BASE  + n)}
#define XCEAC124_ADC_LINE(n) \
    {"adc"    __CX_STRINGIZE(n), NULL, NULL, "%9.6f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_ADC_n_BASE+n)}


                SUBELEM_START_CRN("set", "DAC",
                                  "Set, :-( !V\vMaxSpd, V/s\vCur, V", "0\v1\v2\v3",
                                  4*3, 3)
                    XCEAC124_DAC_LINE(0),
                    XCEAC124_DAC_LINE(1),
                    XCEAC124_DAC_LINE(2),
                    XCEAC124_DAC_LINE(3),
                SUBELEM_END("", NULL),

                SUBELEM_START_CRN("mes", "ADC",
                                  "U, :-( !V", "0\v1\v2\v3\v4\v5\v6\v7\v8\v9\v10\v11\vT\vPWR\v10V\v0V",
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
                SUBELEM_END("", NULL),
        
                SUBELEM_START_CRN("params", "Parameters", " ", NULL, 4, 1)
                    {"adc_time",  "",   NULL, NULL,    "1ms\v2ms\v5ms\v10ms\v20ms\v40ms\v80ms\v160ms", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_ADC_TIMECODE)},
                    {"adc_magn",  "",   NULL, NULL,    "x1\vx10\v\nx100\v\nx1000",                     NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_ADC_GAIN)},
                    {"beg",       "Ch", NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_ADC_BEG), .minnorm=0, .maxnorm=XCEAC124_CHAN_ADC_n_COUNT-1},
                    {"end",       "-",  NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_XCEAC124_base + XCEAC124_CHAN_ADC_END), .minnorm=0, .maxnorm=XCEAC124_CHAN_ADC_n_COUNT-1},
                SUBELEM_END("norowtitles", NULL),

            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL)
        SUBWIN_END("CEAC124...", "compact", NULL),

        MAIN_LINE("1", 0),
        MAIN_LINE("2", 1),
        MAIN_LINE("3", 2),
        MAIN_LINE("4", 3),
    GLOBELEM_END("", 1),

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

    GLOBELEM_START("triggers", "Запуски", 6 + 1, 2 + (1 << 24))

        SUBWINV4_START(NULL, "CGVI8", 1)
            SUBELEM_START(":", NULL, 1, 1)

#define CGVI_LINE(n) \
    {"e"   #n, "",   NULL, "%1.0f",  NULL,                  NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_CGVI8_base + CGVI8_CHAN_MASK1_N_BASE+n)}, \
    {"c"   #n, #n,   "q",  "%5.0f",  "withunits",           NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_CGVI8_base + CGVI8_CHAN_16BIT_N_BASE+n), .minnorm=0, .maxnorm=65535}, \
    {"hns" #n, "=",  "us", "%11.1f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_CGVI8_base + CGVI8_CHAN_QUANT_N_BASE+n), .incdec_step=0.1, .minnorm=0, .maxnorm=214700000}

                SUBELEM_START("gvi", "CGVI8", 3, 1)
                    SUBELEM_START_CRN("set", "Settings",
                                      " \v16-bit code\vTime",
                                      "?\v?\v?\v?\v?\v?\v?\v?\v",
                                      8*3, 3)
                        CGVI_LINE(0),
                        CGVI_LINE(1),
                        CGVI_LINE(2),
                        CGVI_LINE(3),
                        CGVI_LINE(4),
                        CGVI_LINE(5),
                        CGVI_LINE(6),
                        CGVI_LINE(7),
                    SUBELEM_END("notitle,noshadow,norowtitles", NULL),
             
                    SEP_L(),
             
                    SUBELEM_START("ctl", "Control", 3, 2)
                        {"base",  "Base byte",   NULL, "%3.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_CGVI8_base + CGVI8_CHAN_BASEBYTE), .minnorm=0, .maxnorm=255},
                        {"scale", "q=", NULL, "%1.0f",
                            "#T100 ns\v200 ns\v400 ns\v800 ns\v1.60 us\v3.20 us\v6.40 us\v12.8 us\v25.6 us\v51.2 us\v102 us\v205 us\v410 us\v819 us\v1.64 ms\v3.28 ms",
                        NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_CGVI8_base + CGVI8_CHAN_PRESCALER)},
                        {"Start", "ProgStart",  NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_CGVI8_base + CGVI8_CHAN_PROGSTART), .placement="horz=right"},
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
                SUBELEM_END("nocoltitles,norowtitles", NULL),
             
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL)
        SUBWIN_END("CGVI8...", "compact", NULL),

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
                CMD_ALLOW_W, CMD_SETP_I(GID25X4_CHAN_PROGSTART),
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
        {"-once", "Стук!", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GID25X4_CHAN_PROGSTART), .placement="horz=center"},
    GLOBELEM_END("nocoltitles,norowtitles", 0),

    GLOBELEM_START("histplot", "Histplot", 1, 1)
        {"histplot", "HistPlot", NULL, NULL, "width=700,height=300,plot1=.U1mes,plot2=.U1uvh,plot3=.U1set", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
    GLOBELEM_END("notitle,nocoltitles,norowtitles", 1),

    {NULL}
};

static physprops_t gid25x4_physinfo[] =
{
    {GID25X4_CHAN_UMES_base +   0,    1000000.0/100, 0, 0},
    {GID25X4_CHAN_UMES_base +   1,    1000000.0/100, 0, 0},
    {GID25X4_CHAN_UMES_base +   2,    1000000.0/100, 0, 0},
    {GID25X4_CHAN_UMES_base +   3,    1000000.0/100, 0, 0},

    {GID25X4_CHAN_UUVH_base +  (0)*2, 1000000.0/100, 0, 0},
    {GID25X4_CHAN_UUVH_base +  (1)*2, 1000000.0/100, 0, 0},
    {GID25X4_CHAN_UUVH_base +  (2)*2, 1000000.0/100, 0, 0},
    {GID25X4_CHAN_UUVH_base +  (3)*2, 1000000.0/100, 0, 0},
    {GID25X4_CHAN_USET_base +   0,    1000000.0/100, 0, 305},
    {GID25X4_CHAN_USET_base +   1,    1000000.0/100, 0, 305},
    {GID25X4_CHAN_USET_base +   2,    1000000.0/100, 0, 305},
    {GID25X4_CHAN_USET_base +   3,    1000000.0/100, 0, 305},

    /* Note: quant-channels are in 100ns quants; we need us; 1us/100ns=10 */
    {GID25X4_CHAN_DBEG_base +   0,    10,            0, 0},
    {GID25X4_CHAN_DBEG_base +   1,    10,            0, 0},
    {GID25X4_CHAN_DBEG_base +   2,    10,            0, 0},
    {GID25X4_CHAN_DBEG_base +   3,    10,            0, 0},
    {GID25X4_CHAN_DEND_base +   0,    10,            0, 0},
    {GID25X4_CHAN_DEND_base +   1,    10,            0, 0},
    {GID25X4_CHAN_DEND_base +   2,    10,            0, 0},
    {GID25X4_CHAN_DEND_base +   3,    10,            0, 0},

    /* 278353/370.0A */
    {GID25X4_CHAN_IMES01_base + 0,    278353/370.,   0, 0},
    {GID25X4_CHAN_IMES01_base + 1,    278353/370.,   0, 0},
    {GID25X4_CHAN_IMES23_base + 0,    278353/370.,   0, 0},
    {GID25X4_CHAN_IMES23_base + 1,    278353/370.,   0, 0},

    /* XCEAC124 source */
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
