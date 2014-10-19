#include "drv_i/dl200me_drv_i.h"
#include "drv_i/adc200me_drv_i.h"
#include "drv_i/adc812me_drv_i.h"
#include "drv_i/panov_ubs_drv_i.h"
#include "drv_i/tvac320_drv_i.h"
#include "drv_i/senkov_ebc_drv_i.h"
#include "drv_i/nkshd485_drv_i.h"
#include "drv_i/xcac208_drv_i.h"
#include "drv_i/xceac124_drv_i.h"
#include "drv_i/canadc40_drv_i.h"
#include "drv_i/cgvi8_drv_i.h"
#include "drv_i/curvv_drv_i.h"
#include "drv_i/doorilks_drv_i.h"

#include "liuclient.h"

#include "common_elem_macros.h"

#define DEFSERVER_RACK0 "rack0:44"
#include "devmap_rack0_44.h"
#include "devmap_rackX_43.h"
#include "devmap_rack0_45.h"

enum
{
    R_RESET_TIME = 900,
    R_CHK2       = 901,
    R_LIM2       = 902,

    R_HEAT_T     = 910,
    R_HEAT_W     = 911,
    R_HEAT_P     = 912,
    
    R_TYK_DIS    = 920,
};

enum
{
    LIUBPMS_BIGC_BASE = 0,
    LIUBPMS_CHAN_BASE = 0,

    R0_812_BIGC_BASE  = 4,
    R0_812_CHAN_BASE  = 80,
};

// Note: it is the same as in elem_ubs.h, except that it uses (1-c_stat) -- i.e., 0:on,1:off
#define BEAM_TIME_LINE(filename, c_stat, r_time, r_wass, r_prvt)      \
    {"wktime", NULL, "m", "%8.0f", NULL, "Total on-time", LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t []){                                                  \
            /*CMD_RET_I(12345.67),*/                                       \
            CMD_PUSH_I(-1), CMD_SETLCLREGDEFVAL_I(r_wass),             \
            CMD_GETLCLREG_I(r_wass), CMD_PUSH_I(0),                    \
            CMD_IF_GE_TEST, CMD_GOTO_I("already_loaded"),              \
            CMD_PUSH_I(0), CMD_LOADVAL_I(filename),                    \
            CMD_SETLCLREG_I(r_time),                                   \
            CMD_PUSH_I(0), CMD_SETLCLREG_I(r_wass),                    \
            CMD_PUSH_I(0), CMD_SETLCLREG_I(r_prvt),                    \
            CMD_LABEL_I("already_loaded"),                             \
                                                                       \
            /* Is heat on? */                                          \
            CMD_PUSH_I(1), CMD_GETP_I(c_stat),  CMD_SUB, CMD_PUSH_I(0.9), \
            CMD_IF_LT_TEST, CMD_GOTO_I("ret"),                         \
            /* Get current time */                                     \
            CMD_GETTIME, CMD_SETREG_I(0),                              \
                                                                       \
            /* Was heat previously "on"? */                            \
            CMD_GETLCLREG_I(r_wass), CMD_PUSH_I(0.0),                  \
            CMD_IF_GT_TEST, CMD_GOTO_I("was_on"),                      \
            /* No?  Set current moment as start */                     \
            CMD_GETREG_I(0), CMD_SETLCLREG_I(r_prvt),                  \
            CMD_LABEL_I("was_on"),                                     \
                                                                       \
            /* Add elapsed time to a sum... */                         \
            CMD_GETREG_I(0), CMD_GETLCLREG_I(r_prvt), CMD_SUB,         \
            CMD_GETLCLREG_I(r_time), CMD_ADD, CMD_SETLCLREG_I(r_time), \
            /* ...save sum... */                                       \
            CMD_GETLCLREG_I(r_time), CMD_SAVEVAL_I(filename),          \
            /* ...and promote "previous" time to current */            \
            CMD_GETREG_I(0), CMD_SETLCLREG_I(r_prvt),                  \
                                                                       \
            CMD_LABEL_I("ret"),                                        \
            CMD_PUSH_I(1), CMD_GETP_I(c_stat), CMD_SUB, CMD_SETLCLREG_I(r_wass), \
            CMD_GETLCLREG_I(r_time), CMD_DIV_I(60), CMD_RET            \
        }                                                              \
    }

static groupunit_t liu_grouping[] =
{
    GLOBELEM_START("beam", "Пучок", 3, 1)
        SUBELEM_START("ebc", "Пучок", 7, 2 + (1 << 24))
            BEAM_TIME_LINE("beam_heattime.val", BEAM_C_WORK, R_HEAT_T, R_HEAT_W, R_HEAT_P),
            {"Uset",  "Uвых",    NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_DAC_N_BASE+0), .minnorm=0, .maxnorm=60.0, .mindisp=0, .maxdisp=60.0},
            {"Umes",  "UвыхИзм", NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+1), .mindisp=0, .maxdisp=60.0},
            {"Iheat", "Iнакала", NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+0), .mindisp=0, .maxdisp=60.0},
            {"state", "",        NULL, NULL,
                "#-#T"
                    "0000\v0001\v0010\v0011\v"
                    "0100\v0101\v0110\v0111\v"
                    "1000\v1001\v1010\v1011\v"
                    "1100\vOK\f\flit=green\vERR\f\flit=red\vWAIT\f\flit=yellow",
             NULL, LOGT_READ*1|LOGT_WRITE1*0, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                (excmd_t[]){
                    CMD_GETP_I(EBC_BASE+SENKOV_EBC_CHAN_REGS_RD1),
                    CMD_MOD_I(16),
                    CMD_SETREG_I(0),

                    CMD_GETREG_I(0), CMD_PUSH_I(13),
                    CMD_IF_GE_TEST, CMD_GOTO_I("value_sane"),
                    CMD_WEIRD_I(1),
                    CMD_LABEL_I("value_sane"),
                    
                    CMD_GETREG_I(0), CMD_RET
                },
                (excmd_t[]){CMD_RET_I(0)},
            },
            {"onoff", "",        NULL, NULL,
                "#!#-#TВыкл\f\flit=red\vВкл\f\flit=green",
                NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                (excmd_t[]){
                    CMD_PUSH_I(1),
                    CMD_GETP_I(BEAM_C_WORK),
                    CMD_SUB,
                    CMD_RET
                },
                (excmd_t[]){
                    CMD_PUSH_I(1),
                    CMD_QRYVAL,
                    CMD_SUB,
                    CMD_SETP_I(BEAM_C_WORK),
                    CMD_RET
                },
            },
            {"Reset", "Сброс",   NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                (excmd_t[]){
                    /* Is R_RESET_TIME!=0? */
                    CMD_PUSH_I(0.0), CMD_SETLCLREGDEFVAL_I(R_RESET_TIME),
                    CMD_PUSH_I(1.0), CMD_PUSH_I(1.0), CMD_PUSH_I(0.0), CMD_GETLCLREG_I(R_RESET_TIME), CMD_CASE,
                    CMD_TEST, CMD_BREAK_I(0.0),
                    
                    CMD_PUSH_I(0.0), CMD_PUSH_I(0.0), CMD_PUSH_I(1.0),
                    CMD_GETLCLREG_I(R_RESET_TIME), CMD_ADD_I(4.0), CMD_GETTIME, CMD_SUB, //
                    CMD_CASE,
                    CMD_TEST, CMD_BREAK_I(CX_VALUE_DISABLED_MASK),
                    
                    CMD_PUSH_I(0), CMD_ALLOW_W, CMD_PRGLY, CMD_SETP_I(BEAM_C_RESET),
                    CMD_PUSH_I(0), CMD_SETLCLREG_I(R_RESET_TIME),
                    CMD_RET_I(0.0)
                },
                (excmd_t[]){
                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                    CMD_PUSH_I(1), CMD_PRGLY, CMD_SETP_I(BEAM_C_RESET),
                    
                    CMD_GETTIME, CMD_SETLCLREG_I(R_RESET_TIME), CMD_REFRESH_I(1.0),
                    CMD_RET
                },
                .placement="horz=right"
            },
        SUBELEM_END("noshadow", NULL),
        SEP_L(),
        SUBELEM_START("vacuum", "Вакуум", 3, 3)
            {"mes0", "Вакуум", "V",  "%6.3f", "withunits", NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VAC124_BASE + XCEAC124_CHAN_ADC_n_BASE + 0), .mindisp=     0.0, .maxdisp=+10.0},
            {"mes1", "БП1",    "uA", "%6.0f", "withunits", NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VAC124_BASE + XCEAC124_CHAN_ADC_n_BASE + 1), .mindisp=-10000.0, .maxdisp=+10000.0},
            {"mes2", "БП2",    "uA", "%6.0f", "withunits", NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VAC124_BASE + XCEAC124_CHAN_ADC_n_BASE + 2), .mindisp=-10000.0, .maxdisp=+10000.0},
        SUBELEM_END("noshadow,notitle,norowtitles", NULL),
    GLOBELEM_END("defserver=rack0:45 notitle,nocoltitles,norowtitles", 0)
    ,
    GLOBELEM_START("histplot", "Histplot", 1, 1)
        {"histplot", "HistPlot", NULL, NULL, "width=800,height=150,plot1=beam.ebc.Uset,plot2=beam.ebc.Umes,plot3=beam.ebc.Iheat,plot4=beam.vacuum.mes0,plot5=beam.vacuum.mes1,plot6=beam.vacuum.mes2,fixed", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
    GLOBELEM_END("notitle,nocoltitles,norowtitles", 0)
    ,

    GLOBELEM_START("tokarev", "Tokarev's", 2, 2)

#define TOKAREV_LINE(l, n) \
    {"-onoff"l, "", NULL, NULL, "#-#TВыкл\f\flit=red\vВкл\f\flit=green", \
        NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_REGS_WR8_BASE+n)}, \
    {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_REGS_RD8_BASE+n)}, \
    {"IPS"l"set", "IPS"l"set", NULL, "%6.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_OUT_n_BASE+n), .minnorm=0, .maxnorm=1000}, \
    {"IPS"l"uvh", "IPS"l"УВХ", NULL, "%6.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_ADC_n_BASE+(n)*3+0)}, \
    {"IPS"l"dac", "IPS"l"ЦАП", NULL, "%6.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_ADC_n_BASE+(n)*3+1)}, \
    {"IPS"l"cap", "IPS"l"ёмк", NULL, "%6.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C124_BASE+XCEAC124_CHAN_ADC_n_BASE+(n)*3+2)}, \
    {"IPS"l"ena", "",          NULL, NULL,    NULL, "Разрешение импульса каналу ГВИ", LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GVI_BASE +CGVI8_CHAN_MASK1_N_BASE +0+n)}, \
    {"IPS"l"beg", "IPS"l"зап", NULL, "%6.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GVI_BASE +CGVI8_CHAN_16BIT_N_BASE +0+n), .minnorm=0, .maxnorm=65535}

        SUBELEM_START_CRN("tokarev", "Tokarev's", " \v ?\vУст, V\vУВХ, V\vЦАП, V\vЁмк, V\v \vGVI, q", "IPS1\vIPS2\vIPS3\vIPS4", 8*4, 8)
            TOKAREV_LINE("1", 3),
            TOKAREV_LINE("2", 2),
            TOKAREV_LINE("3", 1),
            TOKAREV_LINE("4", 0),
        SUBELEM_END("noshadow,notitle", NULL),
        SUBELEM_START("gvi_ctl", "Управление\nГВИ", 2, 1)
            {"scale", "q=", NULL, "%1.0f",
                "#T100ns\v200ns\v400ns\v800ns\v1.60us\v3.20us\v6.40us\v12.8us\v25.6us\v51.2us\v102us\v205us\v410us\v819us\v1.64ms\v3.28ms",
                NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GVI_BASE+CGVI8_CHAN_PRESCALER)},
            {"Start", "Программный\nзапуск",  NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GVI_BASE+CGVI8_CHAN_PROGSTART)},
        SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),
    GLOBELEM_END("defserver=rack0:45 titleatleft,nocoltitles,norowtitles", GU_FLAG_FROMNEWLINE)
    ,
    GLOBELEM_START("belikov", "Корректоры\n(Яминов)", 3*6, 3 + 3*65536)

#define BELIKOV_LINE(l, n) \
    {"Iset"l,  "Iуст"l, NULL, "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_OUT_n_BASE + 0 + (n)), .minnorm=-2, .maxnorm=+2, .incdec_step=0.1},     \
    {"Imes"l,  "Iизм"l, NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+0)}, \
    {"Uload"l, "Uнгр"l, NULL, "%6.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+1)}

        BELIKOV_LINE("1X", 0),
        BELIKOV_LINE("2X", 2),
        BELIKOV_LINE("3X", 4),
        BELIKOV_LINE("1Y", 1),
        BELIKOV_LINE("2Y", 3),
        BELIKOV_LINE("3Y", 5),

    GLOBELEM_END("defserver=rack0:45 titleatleft", 0)
    ,
    GLOBELEM_START("ignition", "Зажигание", 7, 1)
        SUBELEM_START("chargers", "Зарядные устройства", 2, 1)
#define TVAC320_MAX  300
#define TVAC320_STAT_LINE(base, n, l) \
    {"s"__CX_STRINGIZE(n), l,    NULL, NULL, NULL,           NULL, LOGT_READ,   LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + TVAC320_CHAN_STAT_base + n)}
#define TVAC320_ILK_LINE( base, n, l) \
    {"m"__CX_STRINGIZE(n), l,    NULL, NULL, "",             NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + TVAC320_CHAN_LKM_base + n)}, \
    {"l"__CX_STRINGIZE(n), NULL, NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + TVAC320_CHAN_ILK_base + n)}
#define CHARGER_LINE(base, bel_n, l)                                                                              \
    SUBELEM_START(l, l, 4, 4)                                                                                     \
        RGSWCH_LINE("-tvr", NULL, "Выкл", "Вкл",                                                                  \
                    (base) + TVAC320_CHAN_STAT_base + 0,                                                          \
                    (base) + TVAC320_CHAN_TVR_OFF,                                                                \
                    (base) + TVAC320_CHAN_TVR_ON),                                                                \
        {"set", NULL, "V", "%5.1f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL,          \
         PHYS_ID((base) + TVAC320_CHAN_SET_U), .minnorm=0, .maxnorm=TVAC320_MAX},                                 \
        {"mes", NULL, "V", "%6.2f", NULL,        NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL,          \
         PHYS_ID((base) + TVAC320_CHAN_MES_U)},                                                                   \
        SUBWINV4_START("service", "Зарядное "l": служебные каналы", 1)                                            \
            SUBELEM_START("content", NULL, 5, 1)                                                                  \
                SUBELEM_START("ctl", NULL, 4, 4)                                                                      \
                    HLABEL_CELL("Диапазон"),                                                                          \
                    {"min", NULL, NULL, "%5.1f", NULL,        NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, \
                     PHYS_ID((base) + TVAC320_CHAN_SET_U_MIN), .minnorm=0, .maxnorm=TVAC320_MAX},                     \
                    {"max", "-",  NULL, "%5.1f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, \
                     PHYS_ID((base) + TVAC320_CHAN_SET_U_MAX), .minnorm=0, .maxnorm=TVAC320_MAX},                     \
                    {NULL, "Сбр.блк", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON,     LOGK_DIRECT, LOGC_NORMAL, \
                     PHYS_ID((base) + TVAC320_CHAN_RST_ILK), .placement="horz=right"},                                \
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),                                        \
                SEP_L(),                                                                                              \
                SUBELEM_START("bits", NULL, 3, 3)                                                                     \
                    SUBELEM_START("stat", "Состояние", 8, 1)                                                          \
                        TVAC320_STAT_LINE((base), 0, "0 ЗУ вкл"),                                                     \
                        TVAC320_STAT_LINE((base), 1, "1 ТРН вкл"),                                                    \
                        TVAC320_STAT_LINE((base), 2, "2 стаб. фазы ТРН"),                                             \
                        TVAC320_STAT_LINE((base), 3, "3 стаб. U ТРН"),                                                \
                        TVAC320_STAT_LINE((base), 4, "4 разряд накоп."),                                              \
                        TVAC320_STAT_LINE((base), 5, "5"),                                                            \
                        TVAC320_STAT_LINE((base), 6, "6"),                                                            \
                        TVAC320_STAT_LINE((base), 7, "7 ЗУ неисправно"),                                              \
                    SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                            \
                    VSEP_L(),                                                                                         \
                    SUBELEM_START("ilks", "Блокировки", 16, 2)                                                        \
                        TVAC320_ILK_LINE((base), 0, "0 превышение Iзар"),                                             \
                        TVAC320_ILK_LINE((base), 1, "1 превышение Uраб"),                                             \
                        TVAC320_ILK_LINE((base), 2, "2 полн.разряд накоп."),                                          \
                        TVAC320_ILK_LINE((base), 3, "3"),                                                             \
                        TVAC320_ILK_LINE((base), 4, "4"),                                                             \
                        TVAC320_ILK_LINE((base), 5, "5"),                                                             \
                        TVAC320_ILK_LINE((base), 6, "6"),                                                             \
                        TVAC320_ILK_LINE((base), 7, "7 ЗУ неисправно"),                                               \
                    SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                            \
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                        \
                SEP_L(),                                                                                              \
                SUBELEM_START("degauss", "Размагничивание", 3, 3)                                                     \
                    {"Iset",  "Iуст", "A", "%6.3f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELCHRG_BASE + XCAC208_CHAN_OUT_n_BASE + 0 + (bel_n)), .minnorm=-25, .maxnorm=+25, .incdec_step=0.1},     \
                    {"Imes",  "Iизм", "A", "%6.3f", "withlabel,withunits", NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELCHRG_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (bel_n)*2+0)}, \
                    {"Uload", "Uнгр", "V", "%6.3f", "withlabel,withunits", NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELCHRG_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (bel_n)*2+1)}  \
                SUBELEM_END("noshadow,nocoltitles,norowtitles", "horz=center"),                                       \
            SUBELEM_END("notitle,nocoltitles,norowtitles", NULL),                                        \
        SUBWIN_END("...", "nocoltitles,norowtitles", NULL),                                                       \
    SUBELEM_END("notitle,nocoltitles,norowtitles,noshadow", NULL)

            CHARGER_LINE((PACHKOV_BASE)+TVAC320_NUMCHANS*0, 0, "1"),
            CHARGER_LINE((PACHKOV_BASE)+TVAC320_NUMCHANS*1, 1, "2"),
        SUBELEM_END("noshadow,nocoltitles", NULL),

        SEP_L(),
        SUBELEM_START("keys", "Ключи", 1, 1)
            {"key", "ПУЛЬТ", NULL, NULL,
             " dev=/dev/ttyUSB0"
             "  on_on='set dl200me_s.ctl.starts.-lkcont.lock=0'"
             " on_off='set dl200me_s.ctl.starts.-lkcont.lock=1'"
             ,
             NULL, LOGT_READ, LOGD_LIU_KEY_OFFON, LOGK_NOP}
        SUBELEM_END("noshadow,nocoltitles", NULL),

        SEP_L(),
        SUBELEM_START("-tbgi", "ТБГИ", 5, 5)
            {"offon", NULL, NULL, NULL, "#!#L\v#TВыкл\f\flit=red\vВкл\f\flit=green", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                (excmd_t[]){
                    CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_TBGI_ON_TIME),

                    CMD_GETP_I(PANOV_ADC40_BASE + CANADC40_CHAN_REGS_WR8_BASE + 0),
                    CMD_RET
                },
                (excmd_t[]){
                    CMD_QRYVAL,
                    CMD_SETP_I(PANOV_ADC40_BASE + CANADC40_CHAN_REGS_WR8_BASE + 0),

                    CMD_QRYVAL, CMD_PUSH_I(0.9),
                    CMD_IF_LT_TEST, CMD_GOTO_I("reset"),
                    // Check if previous values is the same -- then do nothing
                    CMD_GETP_I(PANOV_ADC40_BASE + CANADC40_CHAN_REGS_WR8_BASE + 0),
                    CMD_PUSH_I(0.9),
                    CMD_IF_GT_TEST, CMD_BREAK,
                    // Okay, let's start...
                    CMD_GETTIME, CMD_SETLCLREG_I(REG_TBGI_ON_TIME),
                    CMD_REFRESH_I(1),
                    
                    CMD_BREAK,

                    CMD_LABEL_I("reset"),
                    CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TBGI_ON_TIME),

                    CMD_RET
                },
            },
            {"time", NULL, "s", "%3.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                (excmd_t[]){
                    CMD_PUSH_I(0), CMD_SETREG_I(0),
                    CMD_GETLCLREG_I(REG_TBGI_ON_TIME), CMD_PUSH_I(1),
                    CMD_IF_LT_TEST, CMD_GOTO_I("ret"),

                    // Calc "time left"
                    CMD_GETLCLREG_I(REG_TBGI_ON_TIME), CMD_ADD_I(60*3),
                    CMD_GETTIME,
                    CMD_SUB, CMD_SETREG_I(0),

                    // Had the time expired?
                    CMD_GETREG_I(0), CMD_PUSH_I(0),
                    CMD_IF_GT_TEST, CMD_GOTO_I("ret"),
                    // Yes...
                    CMD_PUSH_I(0), CMD_SETREG_I(0),
                    CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TBGI_ON_TIME),
                    CMD_PUSH_I(0), CMD_ALLOW_W,
                    CMD_SETP_I(PANOV_ADC40_BASE + CANADC40_CHAN_REGS_WR8_BASE + 0),

                    CMD_LABEL_I("ret"),
                    CMD_GETREG_I(0), CMD_RET
                },
            },
            RGLED_LINE("status", NULL, PANOV_ADC40_BASE + CANADC40_CHAN_REGS_RD8_BASE + 0),
            {"c1", "ХБЗ1", "V", "%5.2f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PANOV_ADC40_BASE + CANADC40_CHAN_READ_N_BASE + 0)},
            {"c2", "ХБЗ2", "V", "%5.2f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PANOV_ADC40_BASE + CANADC40_CHAN_READ_N_BASE + 1)},
        SUBELEM_END("noshadow,norowtitles,nocoltitles", 0),

        SEP_L(),
        SUBELEM_START("doors", "Дверные блокировки", 8, 8)
#define DOOR_ILOCK(n) \
    {"l"__CX_STRINGIZE(n), " "__CX_STRINGIZE(n)" ", NULL, NULL,       \
     "panel,offcol=red,color=green", \
     NULL, LOGT_READ, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(/*VAC124_BASE + XCEAC124_CHAN_REGS_RD8_BASE*/DOORILKS_BASE + DOORILKS_CHAN_ILK_base + n)}
            {"mode", NULL, NULL, NULL,
                "#TТест\f\flit=red\vРабота\f\flit=green",
                NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL,
                PHYS_ID(DOORILKS_BASE + DOORILKS_CHAN_MODE)},
            HLABEL_CELL(": "),
            DOOR_ILOCK(0),
            DOOR_ILOCK(1),
            DOOR_ILOCK(2),
            DOOR_ILOCK(3),
            HLABEL_CELL("=>"),
            {"l_all", " OK ", NULL, NULL,
             "panel,offcol=red,color=green",
             NULL, LOGT_READ, LOGD_GREENLED, LOGK_CALCED, LOGC_NORMAL,
             PHYS_ID(-1),
             (excmd_t[]){
                 /* Get status */
#if 1
                 CMD_GETP_I(DOORILKS_BASE + DOORILKS_CHAN_SUM_STATE),
#else // Old, BOOL-based direct calc code
                 CMD_GETP_I(VAC124_BASE + XCEAC124_CHAN_REGS_RD8_BASE + 0),
                 CMD_BOOLEANIZE,
                 CMD_GETP_I(VAC124_BASE + XCEAC124_CHAN_REGS_RD8_BASE + 1),
                 CMD_BOOLEANIZE,
                 CMD_BOOL_AND,
                 CMD_GETP_I(VAC124_BASE + XCEAC124_CHAN_REGS_RD8_BASE + 2),
                 CMD_BOOLEANIZE,
                 CMD_BOOL_AND,
                 CMD_GETP_I(VAC124_BASE + XCEAC124_CHAN_REGS_RD8_BASE + 3),
                 CMD_BOOLEANIZE,
                 CMD_BOOL_AND,
#endif
                 CMD_SETREG_I(0),

                 /* Set "disabled state" */
                 CMD_PUSH_I(1), CMD_GETREG_I(0), CMD_SUB, CMD_MUL_I(2),
                 CMD_SETLCLREG_I(R_TYK_DIS),

                 /* Switch-off "auto-shot" if interlock */
                 CMD_PUSH_I(0), // Value for writing
                 CMD_PUSH_I(1), CMD_GETREG_I(0), CMD_SUB, CMD_TEST,
                 CMD_ALLOW_W, CMD_SETP_BYNAME_I("rack0:44!86"),

                 /* Return */
                 CMD_GETREG_I(0),
                 CMD_RET,
             },
            },
        SUBELEM_END("defserver=rack0:45 noshadow,nocoltitles,norowtitles", "horz=center"),
    GLOBELEM_END("notitle,nocoltitles,norowtitles", GU_FLAG_FROMNEWLINE)
    ,
    {
        &(elemnet_t){
            "common", "Управление",
            "", "",
            ELEM_MULTICOL, 5, 1,
            (logchannet_t [])
            {
                SUBELEM_START("dl200", "DL200", 4, 1)
                    SCENARIO_CELL("Инициализир.", "",
                        " set dl200me_s.ctl.starts.mode=3"
                        " set dl200me_s.lines.i{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}=1"
                        ""
                        " set modulators.rack{1,2,3,4,5,6,7,8}.dl200.dl200me_f.ctl.starts.mode=0"
                        " set modulators.rack{1,2,3,4,5,6,7,8}.dl200.dl200me_f.ctl.status.o-all=0"
                        " set modulators.rack{1,2,3,4,5,6,7,8}.dl200.dl200me_f.lines.i{3,4,5,6,7,8}=0"
                        " set modulators.rack{1,2,3,4,5,6,7,8}.dl200.dl200me_f.lines.i{1,2,9,10,11,12,13,14,15,16}=1"
                        " set modulators.rack{1,2,3,4,5,6,7,8}.dl200.dl200me_f.lines.b{3,4,5,6,7,8}=0"
                        " set modulators.rack{1,2,3,4,5,6,7,8}.dl200.dl200me_f.lines.b{1,2,9,10,11,12,13,14,15,16}=1"
                    ),
                    {"new_shift", "Нов.", "ns", "%6.0f", "withlabel", "Новое, ЖЕЛАЕМОЕ значение сдвига", LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]){
                            CMD_PUSH_I(10000), CMD_SETLCLREGDEFVAL_I(REG_ALLDL200_NEWSHIFT),
                            /* That's just for updatability */
                            CMD_GETP_I(DL200ME_SLOW_BASE+DL200ME_CHAN_SERIAL), CMD_POP,

                            CMD_GETLCLREG_I(REG_ALLDL200_NEWSHIFT), CMD_RET,
                        },
                        (excmd_t[]){
                            CMD_QRYVAL, CMD_SETLCLREG_I(REG_ALLDL200_NEWSHIFT),
                            CMD_RET,
                        },
                        .minnorm=0, .maxnorm=20000, .incdec_step=5
                    },
#define SHIFT_ONE_2(r, l) \
    CMD_GETP_BYNAME_I("rack"r":43!"l),  \
    CMD_GETREG_I(0), CMD_ADD, \
    CMD_SETP_BYNAME_I("rack"r":43!"l)
#define SHIFT_2S_IN_RACK(r) \
    SHIFT_ONE_2(r, "8"),      \
    SHIFT_ONE_2(r, "9"),      \
    SHIFT_ONE_2(r, "10"),     \
    SHIFT_ONE_2(r, "11"),     \
    SHIFT_ONE_2(r, "12"),     \
    SHIFT_ONE_2(r, "13"),     \
    SHIFT_ONE_2(r, "15")
                    {"move", "v Сдвинуть v", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]){
                            CMD_RET_I(0),
                        },
                        (excmd_t[]){
                            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,

                            /* Calculate difference, store in r0 */
                            CMD_GETLCLREG_I(REG_ALLDL200_NEWSHIFT), 
                            CMD_GETLCLREG_I(REG_ALLDL200_CURSHIFT),
                            CMD_SUB,
                            CMD_SETREG_I(0),

                            SHIFT_2S_IN_RACK("1"),
                            SHIFT_2S_IN_RACK("2"),
                            SHIFT_2S_IN_RACK("3"),
                            SHIFT_2S_IN_RACK("4"),
                            SHIFT_2S_IN_RACK("5"),
                            SHIFT_2S_IN_RACK("6"),
                            SHIFT_2S_IN_RACK("7"),
                            SHIFT_2S_IN_RACK("8"),

                            /* Store "new" as "current" */
                            CMD_GETLCLREG_I(REG_ALLDL200_NEWSHIFT), 
                            CMD_SETLCLREG_I(REG_ALLDL200_CURSHIFT),

                            CMD_RET,
                        },
                    },
                    {"cur_shift", "Тек.", "ns", "%6.0f", "withlabel", "Текущее значение сдвига", LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]){
                            CMD_PUSH_I(10000), CMD_SETLCLREGDEFVAL_I(REG_ALLDL200_CURSHIFT),
                            /* That's just for updatability */
                            CMD_GETP_I(DL200ME_SLOW_BASE+DL200ME_CHAN_SERIAL), CMD_POP,

                            CMD_GETLCLREG_I(REG_ALLDL200_CURSHIFT), CMD_RET,
                        },
                        (excmd_t[]){
                            CMD_QRYVAL, CMD_SETLCLREG_I(REG_ALLDL200_CURSHIFT),
                            CMD_RET,
                        },
                        .minnorm=0, .maxnorm=20000, .incdec_step=5
                    },
                SUBELEM_END("titleatleft,noshadow,nocoltitles,norowtitles", NULL),

                SEP_L(),

#define SEND_CHAN_VAL(spec, v) \
    CMD_PUSH_I(v), CMD_ALLOW_W, CMD_SETP_BYNAME_I(spec)/*CMD_DEBUGP_I(spec)*/

#define SEND_TO_RACK(r, suff, v) \
    SEND_CHAN_VAL("rack"r":43!130"suff, v), \
    SEND_CHAN_VAL("rack"r":43!132"suff, v), \
    SEND_CHAN_VAL("rack"r":43!134"suff, v), \
    SEND_CHAN_VAL("rack"r":43!136"suff, v), \
    SEND_CHAN_VAL("rack"r":43!138"suff, v), \
    SEND_CHAN_VAL("rack"r":43!140"suff, v)

#define SEND_TO_ALL(suff, v)    \
    SEND_TO_RACK("1", suff, v), \
    SEND_TO_RACK("2", suff, v), \
    SEND_TO_RACK("3", suff, v), \
    SEND_TO_RACK("4", suff, v), \
    SEND_TO_RACK("5", suff, v), \
    SEND_TO_RACK("6", suff, v), \
    SEND_TO_RACK("7", suff, v), \
    SEND_TO_RACK("8", suff, v)

                SUBELEM_START("adc200", "ADC200", 2, 1)
                    {NULL, "Калибровать", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]){
                            CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_CALB_ADC200ME),
                            /* That's just for updatability */
                            CMD_GETP_I(DL200ME_SLOW_BASE+DL200ME_CHAN_SERIAL), CMD_POP,

                            /* Is calibration sequence activated? */
                            CMD_GETLCLREG_I(REG_CALB_ADC200ME), CMD_PUSH_I(0.9), 
                            CMD_IF_LT_TEST, CMD_GOTO_I("ret"),

                            /* Yes. Which stage is now? */
                            CMD_GETLCLREG_I(REG_CALB_ADC200ME), CMD_PUSH_I(1.1), 
                            CMD_IF_GT_TEST, CMD_GOTO_I("run_stop"),
                            /* INTERMEDIATE stage: just promote counter upon update */
                            CMD_DEBUGP_I("clb:=2"),
                            CMD_PUSH_I(2), CMD_SETLCLREG_I(REG_CALB_ADC200ME),
                            CMD_GOTO_I("ret"),

                            /* FINAL stage: send STOP/DISABLED */
                            CMD_LABEL_I("run_stop"),
                            CMD_DEBUGP_I("clb:=0"),
                            CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_CALB_ADC200ME),
                            SEND_TO_ALL("3", CX_VALUE_COMMAND|CX_VALUE_DISABLED_MASK),

                            CMD_LABEL_I("ret"),
                            CMD_RET_I(0),
                        },
                        (excmd_t[]){
                            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                            CMD_DEBUGP_I("clb:=1"),
                            CMD_PUSH_I(1), CMD_SETLCLREG_I(REG_CALB_ADC200ME),
                            SEND_TO_ALL("9", CX_VALUE_COMMAND),
                            CMD_RET
                        },
                        .placement="horz=fill"
                    },

#define SNDR_CHAN_VAL(spec, rn) \
    CMD_GETLCLREG_I(rn), CMD_SETP_BYNAME_I(spec)/*CMD_DEBUGP_I(spec)*/

#define SNDR_TO_RACK(r, suff, rn) \
    SNDR_CHAN_VAL("rack"r":43!130"suff, rn), \
    SNDR_CHAN_VAL("rack"r":43!132"suff, rn), \
    SNDR_CHAN_VAL("rack"r":43!134"suff, rn), \
    SNDR_CHAN_VAL("rack"r":43!136"suff, rn), \
    SNDR_CHAN_VAL("rack"r":43!138"suff, rn), \
    SNDR_CHAN_VAL("rack"r":43!140"suff, rn)

#define SNDR_TO_ALL(suff, rn)    \
    SNDR_TO_RACK("1", suff, rn), \
    SNDR_TO_RACK("2", suff, rn), \
    SNDR_TO_RACK("3", suff, rn), \
    SNDR_TO_RACK("4", suff, rn), \
    SNDR_TO_RACK("5", suff, rn), \
    SNDR_TO_RACK("6", suff, rn), \
    SNDR_TO_RACK("7", suff, rn), \
    SNDR_TO_RACK("8", suff, rn)

                    SUBELEM_START("adc200numpts", NULL, 2, 2)
                        {"numpts", NULL, NULL, "%5.0f", NULL,
                         "Количество точек для измерения, сколько уставить во ВСЕ ADC200",
                         LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                         (excmd_t[]){
                             /* That's just for updatability */
                             CMD_GETP_I(DL200ME_SLOW_BASE+DL200ME_CHAN_SERIAL), CMD_POP,
                             CMD_PUSH_I(3000), CMD_SETLCLREGDEFVAL_I(REG_ADC200ME_NUMPTS),
                             CMD_GETLCLREG_I(REG_ADC200ME_NUMPTS), CMD_RET
                         },
                         (excmd_t[]){
                             CMD_QRYVAL, CMD_SETLCLREG_I(REG_ADC200ME_NUMPTS),
                             CMD_RET
                         },
                         .minnorm=1, .maxnorm=10000
                        },
                        {"-set", ">все", NULL, NULL, NULL,
                         "Уставить указанное слева число точек для измерения во ВСЕ ADC200",
                         LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                         (excmd_t[]){
                             /* That's just for updatability */
                             CMD_GETP_I(DL200ME_SLOW_BASE+DL200ME_CHAN_SERIAL), CMD_POP,
                             CMD_RET_I(0),
                         },
                         (excmd_t[]){
                             SNDR_TO_ALL("5", REG_ADC200ME_NUMPTS),
                             //SEND_TO_ALL("3", CX_VALUE_COMMAND|CX_VALUE_DISABLED_MASK),
                             CMD_RET,
                         },
                        },
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),

                SUBELEM_END("titleatleft,noshadow,nocoltitles,norowtitles", NULL),

                SEP_L(),

#define MOD1_CD(m)   "modulators.rack{1,2,3,4,5,6,7,8}.top-cd"".cont-ubs"m".subwin-ubs.mod"
#define MOD1_ABEF(m) "modulators.rack{1,2,3,4,5,6,7,8}.bot-abef.cont-ubs"m".subwin-ubs.mod"

#define MODS_CD   "modulators.rack{1,2,3,4,5,6,7,8}.top-cd"".cont-ubs{C,D}"  ".subwin-ubs.mod"
#define MODS_ABEF "modulators.rack{1,2,3,4,5,6,7,8}.bot-abef.cont-ubs{A,B,E,F}.subwin-ubs.mod"

#define SCENARIO_RST() \
    " set "MOD1_ABEF("A")".ctl.-rst_ilks=1" \
    " set "MOD1_ABEF("B")".ctl.-rst_ilks=1" \
    " set "MOD1_CD  ("C")".ctl.-rst_ilks=1" \
    " set "MOD1_CD  ("D")".ctl.-rst_ilks=1" \
    " set "MOD1_ABEF("E")".ctl.-rst_ilks=1" \
    " set "MOD1_ABEF("F")".ctl.-rst_ilks=1"

#define SCENARIO_UBS(val, delay)                                                           \
    " set "MOD1_ABEF("A")".ctl.power="__CX_STRINGIZE(val)" sleep "__CX_STRINGIZE(delay)" " \
    " set "MOD1_ABEF("B")".ctl.power="__CX_STRINGIZE(val)" sleep "__CX_STRINGIZE(delay)" " \
    " set "MOD1_CD  ("C")".ctl.power="__CX_STRINGIZE(val)" sleep "__CX_STRINGIZE(delay)" " \
    " set "MOD1_CD  ("D")".ctl.power="__CX_STRINGIZE(val)" sleep "__CX_STRINGIZE(delay)" " \
    " set "MOD1_ABEF("E")".ctl.power="__CX_STRINGIZE(val)" sleep "__CX_STRINGIZE(delay)" " \
    " set "MOD1_ABEF("F")".ctl.power="__CX_STRINGIZE(val)

#define SCENARIO_STn(n, val) \
    " set "MODS_CD  ".sam.heat_"__CX_STRINGIZE(n)"="__CX_STRINGIZE(val)" " \
    " set "MODS_ABEF".sam.heat_"__CX_STRINGIZE(n)"="__CX_STRINGIZE(val)

#define SCENARIO_DGS(   val) \
    " set "MODS_CD  ".sam.udgs_n"                "="__CX_STRINGIZE(val)" " \
    " set "MODS_ABEF".sam.udgs_n"                "="__CX_STRINGIZE(val)

                SUBELEM_START_CRN("-mods", "Мод-ры", NULL, "УБС\vБЗ2\vБЗ1\vБРМ", 4*2 + 1, 2 | (1 << 24))
                    SCENARIO_CELL("R",    "", SCENARIO_RST()),
                    SCENARIO_CELL("Выкл", "", SCENARIO_UBS(0, 0)),
                    SCENARIO_CELL("Вкл",  "", SCENARIO_UBS(1, 2)),
                    SCENARIO_CELL("Выкл", "", SCENARIO_STn(2, 0)),
                    SCENARIO_CELL("Вкл",  "", SCENARIO_STn(2, 1)),
                    SCENARIO_CELL("Выкл", "", SCENARIO_STn(1, 0)),
                    SCENARIO_CELL("Вкл",  "", SCENARIO_STn(1, 1)),
                    SCENARIO_CELL("Выкл", "", SCENARIO_DGS(   0)),
                    SCENARIO_CELL("Вкл",  "", SCENARIO_DGS(   1)),
                SUBELEM_END("titleatleft,nocoltitles,noshadow", NULL),
            },
            "nocoltitles,norowtitles"
        }
    }
    ,
    {
        &(elemnet_t){
            "modulators", "Все модуляторы",
            "", "",
            ELEM_MULTICOL, 8, 8,
            (logchannet_t [])
            {

                #define RACK_N 1
                #define RACK_ADC812_0_SPECIFIC_PARAMS "inverse1A inverse2A"
                #define RACK_ADC812_1_SPECIFIC_PARAMS "inverse0A inverse1A inverse2A"
                #include "liu_elem_rack.h"
                ,
                #define RACK_N 2
                #include "liu_elem_rack.h"
                ,
                #define RACK_N 3
                #include "liu_elem_rack.h"
                ,
                #define RACK_N 4
                #include "liu_elem_rack.h"
                ,
                #define RACK_N 5
                #define RACK_ADC812_1_SPECIFIC_PARAMS "inverse0A"
                #include "liu_elem_rack.h"
                ,
                #define RACK_N 6
                #include "liu_elem_rack.h"
                ,
                #define RACK_N 7
                #include "liu_elem_rack.h"
                ,
                #define RACK_N 8
                #include "liu_elem_rack.h"

            },
            "notitle,noshadow,nocoltitles,norowtitles, hspc=10,vspc=10"
        }
        ,GU_FLAG_FROMNEWLINE*0
    }
    ,
    {
        &(elemnet_t){
            "target", "Мишенный узел",
            NULL, NULL,
            ELEM_MULTICOL, 7, 1,
            (logchannet_t []){
                SUBELEM_START("kshd485", "КШД485 (колесо)", 3, 1 | (1 << 24))
#define NKSHD485_BASE  NKSHD485_BASE
#define NKSHD485_NAME  "WHEEL"
#define NKSHD485_IDENT "wheel"
                        #include "elem_subwin_nkshd485.h"
                        ,

#if 0
target.kshd485.main.config.config.curr.work=5
target.kshd485.main.config.config.curr.hold=1
target.kshd485.main.config.config.hold_delay=30
target.kshd485.main.config.config.cfg_byte=61
target.kshd485.main.config.config.vel.min=100
target.kshd485.main.config.config.vel.max=500
target.kshd485.main.config.config.accel=1000
#endif

#define COLORLED_LINE(name, comnt, dtype, n) \
    {name, NULL, NULL, NULL, NULL, comnt, LOGT_READ, dtype, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + n)}
#define GO_STEPS(n, l)                                                                      \
    {"-"l, l, NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
     (excmd_t[]){                                                                           \
         CMD_GETP_I(NKSHD485_BASE + NKSHD485_CHAN_GO_N_STEPS), CMD_RET                      \
     },                                                                                     \
     (excmd_t[]){                                                                           \
         CMD_PUSH_I(n), CMD_SETP_I(NKSHD485_BASE + NKSHD485_CHAN_GO_N_STEPS), CMD_RET       \
     },                                                                                     \
    }

                    SUBELEM_START("flags", "Flags", 8 , 8)
                        COLORLED_LINE("ready", "Ready",       LOGD_GREENLED, 0),
                        COLORLED_LINE("going", "Going",       LOGD_GREENLED, 1),
                        COLORLED_LINE("k1",    "K-",          LOGD_REDLED,   2),
                        COLORLED_LINE("k2",    "K+",          LOGD_REDLED,   3),
                        COLORLED_LINE("k3",    "Sensor",      LOGD_REDLED,   4),
                        COLORLED_LINE("s_b5",  "Prec. speed", LOGD_AMBERLED, 5),
                        COLORLED_LINE("s_b6",  "K acc",       LOGD_AMBERLED, 6),
                        COLORLED_LINE("s_b7",  "BC",          LOGD_AMBERLED, 7),
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", "horz=right"),
                    SUBELEM_START("movement", "Movement", 4, 4)
                        GO_STEPS(5000, "5000>"),
                        {"-stop", "STOP", "", NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_STOP)},
                        GO_STEPS(-30,  "<30"),
                        GO_STEPS(+30,  "30>"),
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", ""),
                SUBELEM_END("noshadow,nohline,nocoltitles,norowtitles", NULL),
                SEP_L(),
                SUBELEM_START("collim", "Коллиматор", 2, 1)
                    SUBELEM_START(NULL, NULL, 2, 2)
                        {NULL, "Закрыть", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                         (excmd_t[]){CMD_RET_I(0)},
                         (excmd_t[]){CMD_PUSH_I(1), CMD_SETP_I(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_WR8_BASE+0), CMD_RET},
                        },
                        {NULL, "Открыть", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                         (excmd_t[]){CMD_RET_I(0)},
                         (excmd_t[]){CMD_PUSH_I(0), CMD_SETP_I(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_WR8_BASE+0), CMD_RET},
                        },
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                    {NULL, NULL, NULL, NULL, "#TКу-ку...\f\flit=red\vЗакрыто\f\flit=amber\vОткрыто\f\flit=green\vЕдем\f\flit=yellow", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                     (excmd_t[]){
                         CMD_GETP_I(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_RD8_BASE+0),
                         CMD_GETP_I(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_RD8_BASE+1), CMD_MUL_I(2),
                         CMD_ADD,
                         CMD_RET
                     },
                     .placement="horz=center"
                    },
                SUBELEM_END("noshadow,nohline,nocoltitles,norowtitles", NULL),
                SEP_L(),
                SUBELEM_START("faradey", "Цилиндр Фарадея", 2, 1)
                    SUBELEM_START(NULL, NULL, 2, 2)
                        {NULL, "Ехать", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL,
                         PHYS_ID(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_WR8_BASE+1)},
                        {NULL, "Стоп",  NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                         (excmd_t[]){CMD_RET_I(0)},
                         (excmd_t[]){CMD_PUSH_I(0), CMD_SETP_I(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_WR8_BASE+1), CMD_RET},
                        },
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=center"),
                    {NULL, NULL, NULL, NULL, "#TСтоим\vЕдем\vПолзём\vХбз", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                     (excmd_t[]){
                         CMD_GETP_I(PANOV_CAMSEL_BASE+CURVV_CHAN_REGS_RD8_BASE), CMD_POP,
                         CMD_GETP_I(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_RD8_BASE+2),
                         CMD_GETP_I(PANOV_FROLOV_BASE+CURVV_CHAN_REGS_RD8_BASE+3), CMD_MUL_I(2),
                         CMD_ADD,
                         CMD_RET
                     },
                     .placement="horz=center"
                    }
                SUBELEM_END("noshadow,nohline,nocoltitles,norowtitles", NULL),
                SEP_L(),
                SUBELEM_START("camera", "Камеры", 1, 1)
                  {"sel", "", NULL, NULL, "#T 1 \v 2 ", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PANOV_CAMSEL_BASE + CURVV_CHAN_PWR_Bn_BASE + 0)},
                SUBELEM_END("noshadow,nohline,nocoltitles,norowtitles", "horz=center"),
            },
            "defserver=rack0:45 nocoltitles,norowtitles,titleatleft"
        }, 0
    }
    ,
    GLOBELEM_START("bpms", "BPMы", 3, 1)
        SUBWINV4_START(":", "Rack0 ADC812me", 1)
            {"r0adc812", "r0-adc812me", NULL, NULL,
             " cmpr=2",
             NULL, LOGT_READ, LOGD_LIU_ADC812ME_ONE, LOGK_DIRECT,
             R0_812_BIGC_BASE /*!!! A hack: we use 'color' field to pass integer (bigc_n) */,
             PHYS_ID(R0_812_CHAN_BASE)},
        SUBWIN_END("0-812", "one noshadow,notitle,nocoltitles,norowtitles resizable,size=large,bold,compact,noclsbtn", NULL),

#define LIUBPM_LINE(n, l, plc, adc_opts)                       \
    SUBELEM_START("adc200me-"__CX_STRINGIZE(n), l, 1, 1)       \
        {"adc", "bpm"__CX_STRINGIZE(n), NULL, NULL,            \
         " foldctls width=700 height=200"                      \
         " save_button subsys=\"bpm-adc-"__CX_STRINGIZE(n)"\"" \
         " "adc_opts,                                          \
         NULL, LOGT_READ, LOGD_LIU_ADC200ME_ONE, LOGK_DIRECT,  \
         /*!!! A hack: we use 'color' field to pass integer */ \
         LIUBPMS_BIGC_BASE + (n),                              \
         PHYS_ID(LIUBPMS_CHAN_BASE + (n) * ADC200ME_NUM_PARAMS), \
        }                                                      \
    SUBELEM_END("titleatleft,nocoltitles,norowtitles,nofold,one", plc)

        SUBWINV4_START(":", "ЛИУ: BPMы", 1)

//            ROWCOL_START(":", NULL, 4, 1)
            CANVAS_START(":", NULL, 4)
                LIUBPM_LINE(0, "Сумма X; Сумма Y",             "left=0,right=0,top=0",
                            " descrA=СумX  descrB=СумY"
                            " "
                            " "),
                LIUBPM_LINE(1, "Разность X; Разность Y",       "left=0,right=0,top=0@adc200me-0",
                            " descrA=РазнX descrB=РазнY"
                            " "
                            " "),
                LIUBPM_LINE(2, "Трансф. тока 1; Шарик",        "left=0,right=0,top=0@adc200me-1",
                            " descrA=ТрI1  descrB=Шарик"
          //                " coeffA=0.540 coeffB=435.7 coeffA=701.7 coeffB=626" //435.7=233*1.87 = 233*(1k||1k63+540)/(k||1k63)
                            " coeffA=0.472 coeffB=2.796"
                            " unitsA=kA    unitsB=MV"),
                LIUBPM_LINE(3, "Трансф. тока 2; Цил. Фарадея", "left=0,right=0,top=0@adc200me-2,bottom=0",
                            " descrA=ТрI2  descrB=Ц.Фар"
                            " coeffA=0.433 coeffB=0.583"
                            " unitsA=kA    unitsB=kA"),
            SUBELEM_END("notitle,noshadow", "")
//            SUBELEM_END("notitle,noshadow,vertical", "")
        SUBWIN_END(" \n BPMы \n", "resizable,size=large,bold,compact,noclsbtn", NULL),

        {"beamprof", "Beam profile", NULL, NULL,
         "sumsrc=bpms.adc200me-0.adc difsrc=bpms.adc200me-1.adc",
         NULL, LOGT_READ, LOGD_LIU_BEAMPROF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)/* That's just for updatability */},
    GLOBELEM_END("defserver=rack0:46 notitle,noshadow,nocoltitles,norowtitles", 0)
    ,
    #define  DEFSERVER        DEFSERVER_RACK0
    #define  DL200ME_BASE     DL200ME_SLOW_BASE
    #define  DL200ME_IDENT    "dl200me_s"
    #define  DL200ME_LABEL    "Основной DL200ME"
    #define  DL200ME_S_EXT_PFX "\n"
    #define  DL200ME_S_EXT_SFX "\f\flit=red"
    #define  DL200ME_L_1  "Заряд 1"
    #define  DL200ME_L_2  "Заряд 2"
    #define  DL200ME_L_3  "Старт линз"
    #define  DL200ME_L_4  "Размагн.1"
    #define  DL200ME_L_5  "Откл. катода"
    #define  DL200ME_L_6  "Поджиг дуги"
    #define  DL200ME_L_7  "ТБГИ"
    #define  DL200ME_L_8  "Старт секций"
    #define  DL200ME_L_9  "Триггер 1"
    #define  DL200ME_L_10 "Триггер 2"
    #define  DL200ME_L_11 "Размагн.2"
    #define  DL200ME_L_12 "Пуск 0-812"
    #define  DL200ME_L_13 "Резерв 1"
    #define  DL200ME_L_14 "Резерв 2"
    #define  DL200ME_L_15 "Резерв 3"
    #define  DL200ME_L_16 "Резерв 4"
    #define  DL200ME_EDITABLE_LABELS
    #define  DL200ME_REG_AUTO REG_T16_SLOW_AUTO
    #define  DL200ME_REG_SECS REG_T16_SLOW_SECS
    #define  DL200ME_REG_PREV REG_T16_SLOW_PREV
    #define  DL200ME_REG_CNTR REG_T16_SLOW_CNTR
    #define  DL200ME_FILE_CNTR "reg_liucc_dl200me_s_counter.val"

#define INIT_HEAT(N, M, x, goodval) \
    CMD_PUSH_I(0), CMD_LOADVAL_I(UBS_HEAT_FILE_NAME(N, __CX_STRINGIZE(M), x)),                    \
    CMD_SETLCLREG_I(r_time),                                   \
    CMD_PUSH_I(0), CMD_SETLCLREG_I(r_wass),                    \
    CMD_PUSH_I(0), CMD_SETLCLREG_I(r_prvt)

#define INIT_UBS(N, M, gv2, gv1) \
    INIT_HEAT(N, M, 2, gv2),     \
    INIT_HEAT(N, M, 1, gv1)
    
#define DL200ME_COUNT_CHAN                                                    \
    {"count",  NULL, "x",  "%6.0f",  NULL,         "",         LOGT_READ,   LOGD_TEXT,     LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){                                                          \
            CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_T16_SLOW_HTGR),          \
            CMD_GETLCLREG_I(REG_T16_SLOW_HTGR), CMD_PUSH_I(0),                \
            CMD_IF_GT_TEST, CMD_GOTO_I("heats_inited"),                       \
                                                                              \
            CMD_PUSH_I(1.91), CMD_SETLCLREG_I(REG_R11_IH2N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R11_IH1N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R12_IH2N), \
            CMD_PUSH_I(1.75), CMD_SETLCLREG_I(REG_R12_IH1N), \
            CMD_PUSH_I(1.79), CMD_SETLCLREG_I(REG_R13_IH2N), \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R13_IH1N), \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R14_IH2N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R14_IH1N), \
            CMD_PUSH_I(1.77), CMD_SETLCLREG_I(REG_R15_IH2N), \
            CMD_PUSH_I(1.75), CMD_SETLCLREG_I(REG_R15_IH1N), \
            CMD_PUSH_I(1.66), CMD_SETLCLREG_I(REG_R16_IH2N), \
            CMD_PUSH_I(1.67), CMD_SETLCLREG_I(REG_R16_IH1N), \
                                                             \
            CMD_PUSH_I(1.72), CMD_SETLCLREG_I(REG_R21_IH2N), \
            CMD_PUSH_I(1.77), CMD_SETLCLREG_I(REG_R21_IH1N), \
            CMD_PUSH_I(1.72), CMD_SETLCLREG_I(REG_R22_IH2N), \
            CMD_PUSH_I(1.62), CMD_SETLCLREG_I(REG_R22_IH1N), \
            CMD_PUSH_I(1.65), CMD_SETLCLREG_I(REG_R23_IH2N), \
            CMD_PUSH_I(1.78), CMD_SETLCLREG_I(REG_R23_IH1N), \
            CMD_PUSH_I(1.87), CMD_SETLCLREG_I(REG_R24_IH2N), \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R24_IH1N), \
            CMD_PUSH_I(1.68), CMD_SETLCLREG_I(REG_R25_IH2N), \
            CMD_PUSH_I(2.00), CMD_SETLCLREG_I(REG_R25_IH1N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R26_IH2N), \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R26_IH1N), \
                                                             \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R31_IH2N), \
            CMD_PUSH_I(1.62), CMD_SETLCLREG_I(REG_R31_IH1N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R32_IH2N), \
            CMD_PUSH_I(1.63), CMD_SETLCLREG_I(REG_R32_IH1N), \
            CMD_PUSH_I(1.72), CMD_SETLCLREG_I(REG_R33_IH2N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R33_IH1N), \
            CMD_PUSH_I(1.76), CMD_SETLCLREG_I(REG_R34_IH2N), \
            CMD_PUSH_I(1.93), CMD_SETLCLREG_I(REG_R34_IH1N), \
            CMD_PUSH_I(1.63), CMD_SETLCLREG_I(REG_R35_IH2N), \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R35_IH1N), \
            CMD_PUSH_I(1.74), CMD_SETLCLREG_I(REG_R36_IH2N), \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R36_IH1N), \
                                                             \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R41_IH2N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R41_IH1N), \
            CMD_PUSH_I(2.01), CMD_SETLCLREG_I(REG_R42_IH2N), \
            CMD_PUSH_I(1.99), CMD_SETLCLREG_I(REG_R42_IH1N), \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R43_IH2N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R43_IH1N), \
            CMD_PUSH_I(1.79), CMD_SETLCLREG_I(REG_R44_IH2N), \
            CMD_PUSH_I(1.75), CMD_SETLCLREG_I(REG_R44_IH1N), \
            CMD_PUSH_I(1.75), CMD_SETLCLREG_I(REG_R45_IH2N), \
            CMD_PUSH_I(1.72), CMD_SETLCLREG_I(REG_R45_IH1N), \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R46_IH2N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R46_IH1N), \
                                                             \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R51_IH2N), \
            CMD_PUSH_I(1.75), CMD_SETLCLREG_I(REG_R51_IH1N), \
            CMD_PUSH_I(1.68), CMD_SETLCLREG_I(REG_R52_IH2N), \
            CMD_PUSH_I(1.77), CMD_SETLCLREG_I(REG_R52_IH1N), \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R53_IH2N), \
            CMD_PUSH_I(1.79), CMD_SETLCLREG_I(REG_R53_IH1N), \
            CMD_PUSH_I(1.76), CMD_SETLCLREG_I(REG_R54_IH2N), \
            CMD_PUSH_I(1.68), CMD_SETLCLREG_I(REG_R54_IH1N), \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R55_IH2N), \
            CMD_PUSH_I(1.91), CMD_SETLCLREG_I(REG_R55_IH1N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R56_IH2N), \
            CMD_PUSH_I(1.79), CMD_SETLCLREG_I(REG_R56_IH1N), \
                                                             \
            CMD_PUSH_I(1.78), CMD_SETLCLREG_I(REG_R61_IH2N), \
            CMD_PUSH_I(1.99), CMD_SETLCLREG_I(REG_R61_IH1N), \
            CMD_PUSH_I(1.80), CMD_SETLCLREG_I(REG_R62_IH2N), \
            CMD_PUSH_I(1.72), CMD_SETLCLREG_I(REG_R62_IH1N), \
            CMD_PUSH_I(1.91), CMD_SETLCLREG_I(REG_R63_IH2N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R63_IH1N), \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R64_IH2N), \
            CMD_PUSH_I(1.65), CMD_SETLCLREG_I(REG_R64_IH1N), \
            CMD_PUSH_I(1.78), CMD_SETLCLREG_I(REG_R65_IH2N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R65_IH1N), \
            CMD_PUSH_I(1.87), CMD_SETLCLREG_I(REG_R66_IH2N), \
            CMD_PUSH_I(1.95), CMD_SETLCLREG_I(REG_R66_IH1N), \
                                                             \
            CMD_PUSH_I(1.65), CMD_SETLCLREG_I(REG_R71_IH2N), \
            CMD_PUSH_I(1.66), CMD_SETLCLREG_I(REG_R71_IH1N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R72_IH2N), \
            CMD_PUSH_I(1.93), CMD_SETLCLREG_I(REG_R72_IH1N), \
            CMD_PUSH_I(1.65), CMD_SETLCLREG_I(REG_R73_IH2N), \
            CMD_PUSH_I(1.68), CMD_SETLCLREG_I(REG_R73_IH1N), \
            CMD_PUSH_I(1.74), CMD_SETLCLREG_I(REG_R74_IH2N), \
            CMD_PUSH_I(1.69), CMD_SETLCLREG_I(REG_R74_IH1N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R75_IH2N), \
            CMD_PUSH_I(1.66), CMD_SETLCLREG_I(REG_R75_IH1N), \
            CMD_PUSH_I(1.94), CMD_SETLCLREG_I(REG_R76_IH2N), \
            CMD_PUSH_I(1.76), CMD_SETLCLREG_I(REG_R76_IH1N), \
                                                             \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R81_IH2N), \
            CMD_PUSH_I(1.71), CMD_SETLCLREG_I(REG_R81_IH1N), \
            CMD_PUSH_I(1.73), CMD_SETLCLREG_I(REG_R82_IH2N), \
            CMD_PUSH_I(1.76), CMD_SETLCLREG_I(REG_R82_IH1N), \
            CMD_PUSH_I(1.74), CMD_SETLCLREG_I(REG_R83_IH2N), \
            CMD_PUSH_I(1.68), CMD_SETLCLREG_I(REG_R83_IH1N), \
            CMD_PUSH_I(1.92), CMD_SETLCLREG_I(REG_R84_IH2N), \
            CMD_PUSH_I(1.85), CMD_SETLCLREG_I(REG_R84_IH1N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R85_IH2N), \
            CMD_PUSH_I(1.95), CMD_SETLCLREG_I(REG_R85_IH1N), \
            CMD_PUSH_I(1.70), CMD_SETLCLREG_I(REG_R86_IH2N), \
            CMD_PUSH_I(1.89), CMD_SETLCLREG_I(REG_R86_IH1N), \
                                                             \
            CMD_LABEL_I("heats_inited"),                     \
                                                                              \
            CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_T16_SLOW_WPRV),          \
            CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_T16_SLOW_CGRD),          \
            CMD_GETLCLREG_I(REG_T16_SLOW_CGRD), CMD_PUSH_I(0),                \
            CMD_IF_GT_TEST, CMD_GOTO_I("end_load_slow_cntr"),                 \
                                                                              \
            /* guard:=1, load value from file */                              \
            CMD_PUSH_I(1), CMD_SETLCLREG_I(REG_T16_SLOW_CGRD),                \
            CMD_PUSH_I(0), CMD_LOADVAL_I(DL200ME_FILE_CNTR),                  \
            CMD_SETLCLREG_I(REG_T16_SLOW_CNTR),                               \
            CMD_LABEL_I("end_load_slow_cntr"),                                \
                                                                              \
            CMD_LABEL_I("check"),                                             \
            /* was_shot==0? */                                                \
            CMD_GETP_I(DL200ME_BASE + DL200ME_CHAN_WAS_SHOT), CMD_PUSH_I(0),  \
            CMD_IF_GT_TEST, CMD_GOTO_I("end_check"),                          \
            /* ...and prev_was_shot==1? */                                    \
            CMD_GETLCLREG_I(REG_T16_SLOW_WPRV), CMD_PUSH_I(0.9),              \
            CMD_IF_LT_TEST, CMD_GOTO_I("end_check"),                          \
            /* Yes!  That was a new shot! Increment and remember */           \
            CMD_GETLCLREG_I(REG_T16_SLOW_CNTR), CMD_ADD_I(1),                 \
            CMD_SETLCLREG_I(REG_T16_SLOW_CNTR),                               \
            CMD_GETLCLREG_I(REG_T16_SLOW_CNTR), CMD_SAVEVAL_I(DL200ME_FILE_CNTR), \
            /* ...and switch TBGI off */                                      \
            CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TBGI_ON_TIME),                 \
            CMD_PUSH_I(0), CMD_ALLOW_W,                                       \
            CMD_SETP_I(PANOV_ADC40_BASE + CANADC40_CHAN_REGS_WR8_BASE + 0),   \
            CMD_LABEL_I("end_check"),                                         \
                                                                              \
            CMD_LABEL_I("weirdness"),                                         \
            CMD_PUSH_I(0),     \
            ADD_ONE_DELAY(0),  \
            ADD_ONE_DELAY(1),  \
            ADD_ONE_DELAY(2),  \
            ADD_ONE_DELAY(3),  \
            ADD_ONE_DELAY(4),  \
            ADD_ONE_DELAY(5),  \
            ADD_ONE_DELAY(6),  \
            ADD_ONE_DELAY(7),  \
            ADD_ONE_DELAY(8),  \
            ADD_ONE_DELAY(9),  \
            ADD_ONE_DELAY(10), \
            ADD_ONE_DELAY(11), \
            ADD_ONE_DELAY(12), \
            ADD_ONE_DELAY(13), \
            ADD_ONE_DELAY(14), \
            ADD_ONE_DELAY(15), \
            CMD_DUP,           \
            CMD_PUSH_I(0.1), CMD_IF_GT_TEST, CMD_GOTO_I("end_weirdness"), \
            CMD_WEIRD_I(1.0),  \
            CMD_LABEL_I("end_weirdness"), \
                                                                              \
            CMD_LABEL_I("ret"),                                               \
            CMD_GETP_I(DL200ME_BASE + DL200ME_CHAN_WAS_SHOT), CMD_SETLCLREG_I(REG_T16_SLOW_WPRV), \
            CMD_GETLCLREG_I(REG_T16_SLOW_CNTR), CMD_RET                       \
        }                                                                     \
    }

#define DL200ME_TYK_AUX_READ_CODE \
    CMD_GETLCLREG_I(R_TYK_DIS), CMD_ADD
#define DL200ME_AUTO_AUX_READ_CODE DL200ME_TYK_AUX_READ_CODE

#define DL200ME_ATTITLE_CHAN \
    SUBWINV4_START("-dl200me-slow-set-ext", "Ужас-ужас - поставить EXT", 1) \
        SUBELEM_START(NULL, NULL, 1, 1)                                     \
            {"-", "Осторожно! Опасно!\nВключить режим \"Ext\"\nу DL200ME/SLOW", NULL, NULL, "size=huge,bg=red", NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
                (excmd_t[]){CMD_GETP_I(DL200ME_SLOW_BASE+DL200ME_CHAN_RUN_MODE), CMD_POP, CMD_RET_I(0)},                                  \
                (excmd_t[]){CMD_PUSH_I(DL200ME_RUN_MODE_SLOW_EXT), CMD_SETP_I(DL200ME_SLOW_BASE+DL200ME_CHAN_RUN_MODE), CMD_RET}, \
            },                                                              \
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),      \
    SUBWIN_END("...", "", "")
//#undef DL200ME_ATTITLE_CHAN

    #define  FROMNEWLINE
    #include "elem_dl200me_slow.h"
    ,
    {
        &(elemnet_t){
            "adc", "ADC200MEs",
            "", "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t [])
            {
#define LIST_OF_ADC200MES \
    " modulators.rack{1,2,3,4,5,6,7,8}.top-cd"".cont-ubs{C,D}"    ".subwin-adc.adc" \
    " modulators.rack{1,2,3,4,5,6,7,8}.bot-abef.cont-ubs{A,B,E,F}"".subwin-adc.adc"
#define LIST_OF_SAVABLES \
    " adc200me:bpms.adc200me-{0,1,2,3}.adc" \
    " adc812me:bpms.r0adc812" \
    " adc812me:modulators.rack{1,2,3,4,5,6,7,8}.812s.adc812me-{0,1}"
                {"adc", "", NULL, NULL, 
                 " magn1=4 magn2=4 magn3=4 magn4=4"
                 " magn5=4 magn6=4 magn7=4 magn8=4"
                 " width=800,height=220 sources='" LIST_OF_ADC200MES "'"
                 " ia_src=bpms.adc200me-2.adc"
                 " savables='" LIST_OF_SAVABLES "'"
                 , NULL, LOGT_READ, LOGD_LIU_ADC200ME_ANY, LOGK_NOP}
            },
            "nocoltitles,norowtitles"
        }
        ,GU_FLAG_FROMNEWLINE*0
    }
    ,
    
    {NULL}
};


//////////////////////////////////////////////////////////////////////

static physprops_t rack0_44_physinfo[] =
{
#include "pi_rack0_44.h"
};

static physprops_t rack0_45_physinfo[] =
{
#include "pi_rack0_45.h"
};

static physprops_t rack1_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack2_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack3_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack4_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack5_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack6_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack7_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack8_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physinfodb_rec_t liu_physinfo_database[] =
{
    {"rack0:44", rack0_44_physinfo, countof(rack0_44_physinfo)},
    {"rack0:45", rack0_45_physinfo, countof(rack0_45_physinfo)},

    {"rack1:43", rack1_43_physinfo, countof(rack1_43_physinfo)},
    {"rack2:43", rack2_43_physinfo, countof(rack2_43_physinfo)},
    {"rack3:43", rack3_43_physinfo, countof(rack3_43_physinfo)},
    {"rack4:43", rack4_43_physinfo, countof(rack4_43_physinfo)},
    {"rack5:43", rack5_43_physinfo, countof(rack5_43_physinfo)},
    {"rack6:43", rack6_43_physinfo, countof(rack6_43_physinfo)},
    {"rack7:43", rack7_43_physinfo, countof(rack7_43_physinfo)},
    {"rack8:43", rack8_43_physinfo, countof(rack8_43_physinfo)},

    {NULL, NULL, 0}
};
