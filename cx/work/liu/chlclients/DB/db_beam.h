#include "drv_i/dl200me_drv_i.h"
#include "drv_i/senkov_ebc_drv_i.h"
#include "drv_i/xcac208_drv_i.h"
#include "drv_i/xceac124_drv_i.h"
#include "drv_i/cgvi8_drv_i.h"
#include "drv_i/nkshd485_drv_i.h"
#include "drv_i/curvv_drv_i.h"
#include "drv_i/tvac320_drv_i.h"
#include "drv_i/canadc40_drv_i.h"

#include "common_elem_macros.h"

#include "devmap_rack0_45.h"

#include "devmap_rack0_44.h"

enum
{
    R_RESET_TIME = 900,
    R_CHK2       = 901,
    R_LIM2       = 902,

    R_HEAT_T     = 910,
    R_HEAT_W     = 911,
    R_HEAT_P     = 912,
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

static groupunit_t beam_grouping[] =
{
    {
        &(elemnet_t){
            "ctl", "Controls", NULL, NULL,
            ELEM_MULTICOL, 3, 3,
            (logchannet_t []){

                SUBELEM_START_CRN("ctl", "Controls", NULL, "Выходное напряжение\vТок накала", 6, 4)
                    {"Uset",  "UвыхУст", NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_DAC_N_BASE+0), .minnorm=0, .maxnorm=60.0, .mindisp=0, .maxdisp=60.0},
                    {"Umes",  "UвыхИзм", NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+1), .mindisp=0, .maxdisp=60.0},
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
                    BEAM_TIME_LINE("beam_heattime.val", BEAM_C_WORK, R_HEAT_T, R_HEAT_W, R_HEAT_P),
                    {"Iheat", "Iheat",   NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+0), .mindisp=0, .maxdisp=60.0},
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
                    },
                SUBELEM_END("noshadow,notitle,nocoltitles", "horz=left"),

                HLABEL_CELL("          "),

#define DAC_ADC_LINE(n) \
    {"dac" __CX_STRINGIZE(n), "DAC" __CX_STRINGIZE(n), NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_DAC_N_BASE+n)}, \
    {"adc" __CX_STRINGIZE(n), "ADC" __CX_STRINGIZE(n), NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+n)}
                SUBWINV4_START("service", "Служебные каналы контроллера EBC", 1)
                    SUBELEM_START(NULL, NULL, 2, 2)
                        SUBELEM_START_CRN("dacadc", "DAC and ADC", "DAC, V\vADC, V", "0\v1\v2\v3\v4", 5*2,2)
                            DAC_ADC_LINE(0),
                            DAC_ADC_LINE(1),
                            DAC_ADC_LINE(2),
                            DAC_ADC_LINE(3),
                            DAC_ADC_LINE(4),
                        SUBELEM_END(NULL, NULL),

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_REGS_WR8_BASE+n)}
#define INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_REGS_RD8_BASE+n)}
                        SUBELEM_START_CRN("ioregs", "I/O", "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR", 9*2, 9)
                            {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_REGS_WR1)},
                            OUTB_LINE(7),
                            OUTB_LINE(6),
                            OUTB_LINE(5),
                            OUTB_LINE(4),
                            OUTB_LINE(3),
                            OUTB_LINE(2),
                            OUTB_LINE(1),
                            OUTB_LINE(0),
                            {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_REGS_RD1)},
                            INPB_LINE(7),
                            INPB_LINE(6),
                            INPB_LINE(5),
                            INPB_LINE(4),
                            INPB_LINE(3),
                            INPB_LINE(2),
                            INPB_LINE(1),
                            INPB_LINE(0),
                        SUBELEM_END(NULL, NULL),
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL)
                SUBWIN_END("Служ.\nканалы...", "noshadow,notitle,nocoltitles,norowtitles,size=small", "horz=right"),

                SUBELEM_START(NULL, NULL, 1, 1)
                    {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=right"),
                
            },
            "noshadow,notitle,nocoltitles,norowtitles"
        }, GU_FLAG_FROMNEWLINE | GU_FLAG_FILLHORZ*0
    },

    {
        &(elemnet_t){
            "vacuum", "Вакуум",
            NULL, NULL,
            ELEM_MULTICOL, 3, 3,
            (logchannet_t []){
                {"mes0", "Вакуум", "V",   "%6.3f", "withunits", NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VAC124_BASE + XCEAC124_CHAN_ADC_n_BASE + 0), .mindisp=     0.0, .maxdisp=+10.0},
                {"mes1", "БП1",    "мкA", "%6.0f", "withunits", NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VAC124_BASE + XCEAC124_CHAN_ADC_n_BASE + 1), .mindisp=-10000.0, .maxdisp=+10000.0},
                {"mes2", "БП2",    "мкA", "%6.0f", "withunits", NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(VAC124_BASE + XCEAC124_CHAN_ADC_n_BASE + 2), .mindisp=-10000.0, .maxdisp=+10000.0},
                {"chk2", "<",    NULL,  NULL,    NULL,        "Следить за значением\nи выключать накал при превышении", LOGT_WRITE1, LOGD_AMBERLED, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t[]){
                        CMD_PUSH_I(1), CMD_SETLCLREGDEFVAL_I(R_CHK2),
                        CMD_GETLCLREG_I(R_CHK2), CMD_RET
                    },
                    (excmd_t[]){
                        CMD_QRYVAL, CMD_SETLCLREG_I(R_CHK2),
                        CMD_RET
                    },
                },
                {"lim2", "Предел", "мкA", "%6.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t[]){
                        CMD_PUSH_I(9000), CMD_SETLCLREGDEFVAL_I(R_LIM2),
                        /* Is checking on? */
                        CMD_GETLCLREG_I(R_CHK2), CMD_PUSH_I(0.9),
                        CMD_IF_LT_TEST, CMD_GOTO_I("ret"),
                        /* And if value exceeds the limit? */
                        CMD_GETP_I(VAC124_BASE + XCEAC124_CHAN_ADC_n_BASE + 2), 
                        CMD_GETLCLREG_I(R_LIM2),
                        CMD_IF_LT_TEST, CMD_GOTO_I("ret"),
                        /* Yes, switch heat off */
                        CMD_PUSH_I(1), CMD_ALLOW_W, CMD_PRGLY, CMD_SETP_I(BEAM_C_WORK),

                        CMD_LABEL_I("ret"),
                        CMD_GETLCLREG_I(R_LIM2), CMD_RET
                    },
                    (excmd_t[]){
                        CMD_QRYVAL, CMD_SETLCLREG_I(R_LIM2),
                        CMD_RET
                    },
                },
            },
            "notitle,norowtitles"
        }, 0
    },

    {
        &(elemnet_t){
            "chargers", "За-\nряд-\nные",
            NULL, NULL,
            ELEM_MULTICOL, 2, 1,
            (logchannet_t [])
            {

#define TVAC320_MAX  300
#define TVAC320_STAT_LINE(base, n, l) \
    {"s"__CX_STRINGIZE(n), l,    NULL, NULL, NULL,           NULL, LOGT_READ,   LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + TVAC320_CHAN_STAT_base + n)}
#define TVAC320_ILK_LINE( base, n, l) \
    {"m"__CX_STRINGIZE(n), l,    NULL, NULL, "",             NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + TVAC320_CHAN_LKM_base + n)}, \
    {"l"__CX_STRINGIZE(n), NULL, NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + TVAC320_CHAN_ILK_base + n)}
#define CHARGER_LINE(base, bel_n, l)                                                                              \
    SUBELEM_START(l, l, 4, 4)                                                                                     \
        RGSWCH_LINE("-tvr", NULL, "Выкл", "Вкл",                                                                   \
                    (base) + TVAC320_CHAN_STAT_base + 0,                                                           \
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
            },
            "defserver=rack0:44 titleatleft nocoltitles"
        }
        ,0
    },

#if 0
#define DOVZH_LINE(l, n) \
    {"Iset"l,  "Iуст"l, NULL, "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELCHRG_BASE + XCAC208_CHAN_OUT_n_BASE + 0 + (n)), .minnorm=-25, .maxnorm=+25, .incdec_step=0.1},     \
    {"Icoil"l, "Iкат"l, NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELCHRG_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+0)}, \
    {"Uload"l, "Uнгр"l, NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELCHRG_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+1)}
    
    {
        &(elemnet_t){
            "belikov", "Belikov's",
            "", "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("belikov", "Belikov's", "Iуст, A\vIкат, A\vUнгр, V", "2\v1", 3*2, 3)
                    DOVZH_LINE("2", 0),
                    DOVZH_LINE("1", 1),
                SUBELEM_END("noshadow,notitle", NULL),
            },
            "nocoltitles,norowtitles"
        }, 0
    },
#endif

    {
        &(elemnet_t){
            "histplot", "Histplot",
            "", "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"histplot", "HistPlot", NULL, NULL, "width=800,height=300,plot1=.Uset,plot2=.Umes,plot3=.Iheat,plot4=vacuum.mes0,plot5=vacuum.mes1,plot6=vacuum.mes2,fixed", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
            },
            "notitle,nocoltitles,norowtitles"
        }, GU_FLAG_FROMNEWLINE*1
    },

    {
        &(elemnet_t){
            "target", "Мишенный узел",
            NULL, NULL,
            ELEM_MULTICOL, 7, 1,
            (logchannet_t []){
                SUBELEM_START("kshd485", "КШД485 (колесо)", 2, 1)

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
#define BUT_LINE(name, title, n) \
    {name, title, "", NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(n)}

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
                    SUBELEM_START("main", NULL, 4, 2)
                        {"numsteps", NULL,   NULL, "%6.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_NUM_STEPS)},
                        {"-go",      "GO",   NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_GO)},
                        SUBWINV4_START("config", "Config", 1)
                            SUBELEM_START("content", NULL, 3, 1)
                                SUBELEM_START("info", "Info", 2, 2)
                                    {"devver", "Dev.ver.", NULL, "%4.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_DEV_VERSION)},
                                    {"devsrl", "serial#",  NULL, "%3.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_DEV_SERIAL)},
                                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                                SUBELEM_START("config", "Config", 5, 1)
                                    SUBELEM_START("curr", "Current", 2, 2)
                                        {"work", "Work", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_WORK_CURRENT)},
                                        {"hold", "Hold", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_HOLD_CURRENT)},
                                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                                    {"hold_delay", "Hold dly", "1/30s", "%3.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_HOLD_DELAY), .minnorm=0, .maxnorm=255, .placement="horz=right"},
                                    {"cfg_byte", "Cfg byte", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BYTE)},
                                    SUBELEM_START("vel", "Velocity", 2, 2)
                                        {"min", "[",   "/s",   "%5.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_MIN_VELOCITY), .minnorm=32, .maxnorm=12000},
                                        {"max", "-",   "]/s",   "%5.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_MAX_VELOCITY), .minnorm=32, .maxnorm=12000},
                                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                                    {"accel",   "Accel", "/s^2", "%5.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_ACCELERATION), .minnorm=32, .maxnorm=65535, .placement="horz=center"},
                                SUBELEM_END("noshadow,nocoltitles,fold_h", NULL),
                                SUBELEM_START("operate", "Operation", 2, 1)
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
                                    SUBELEM_START("steps", "Steps", 6, 3)
                                        {"numsteps", NULL, NULL, "%6.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_NUM_STEPS)},
                                        BUT_LINE("go",         "GO",          NKSHD485_BASE + NKSHD485_CHAN_GO),
                                        BUT_LINE("go_unaccel", "GO unaccel.", NKSHD485_BASE + NKSHD485_CHAN_GO_WO_ACCEL),
                                        EMPTY_CELL(),
                                        BUT_LINE("stop",       "STOP",             NKSHD485_BASE + NKSHD485_CHAN_STOP),
                                        BUT_LINE("switchoff",  "Switch OFF",       NKSHD485_BASE + NKSHD485_CHAN_SWITCHOFF),
                                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                                SUBELEM_END("noshadow,nocoltitles", NULL),
                            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                        SUBWIN_END("Config...", "noshadow,notitle,nocoltitles,norowtitles", NULL),
                        {"-stop",    "STOP", NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_STOP)},
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
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
                  {"sel", "", NULL, NULL, "#T 1 \v 2 ", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PANOV_CAMSEL_BASE + CURVV_CHAN_REGS_WR8_BASE + 0)},
                SUBELEM_END("noshadow,nohline,nocoltitles,norowtitles", "horz=center"),
            },
            "nocoltitles,norowtitles"
        }, 0
    },

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

#define NOTHING    {"IPS"l"end", "IPS"l"стп", NULL, "%6.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(GVI_BASE +CGVI8_CHAN_16BIT_N_BASE +0/*4?*/+n), .minnorm=0, .maxnorm=65535}

    {
        &(elemnet_t){
            "tokarev", "Tokarev's",
            "", "",
            ELEM_MULTICOL, 2, 2,
            (logchannet_t []){
                //VLABEL_CELL("Tokarev's (линзы)"),
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
            },
            "titleatleft,nocoltitles,norowtitles"
        }, GU_FLAG_FROMNEWLINE*1
    },

#if 1
#define BELIKOV_LINE(l, n) \
    {"Iset"l,  "Iуст"l, NULL, "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_OUT_n_BASE + 0 + (n)), .minnorm=-2, .maxnorm=+2, .incdec_step=0.1},     \
    {"Imes"l,  "Iизм"l, NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+0)}, \
    {"Uload"l, "Uнгр"l, NULL, "%6.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+1)}

    {
        &(elemnet_t){
            "belikov", "Корректоры\n(Яминов)",
            "Iуст, A\vIизм, A\vU, V", "1X\v2X\v3X\v1Y\v2Y\v3Y",
            ELEM_MULTICOL, 3*6, 3 + 3*65536,
            (logchannet_t []){
                BELIKOV_LINE("1X", 0),
                BELIKOV_LINE("2X", 2),
                BELIKOV_LINE("3X", 4),
                BELIKOV_LINE("1Y", 1),
                BELIKOV_LINE("2Y", 3),
                BELIKOV_LINE("3Y", 5),
            },
            "titleatleft"
        },
    },
#else
#define DOVZH_LINE(l, n) \
    {"Iset"l,  "Iуст"l, NULL, "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_OUT_n_BASE + 0 + (n)), .minnorm=-25, .maxnorm=+25, .incdec_step=0.1},     \
    {"Icoil"l, "Iкат"l, NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+0)}, \
    {"Uload"l, "Uнгр"l, NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BELLENS_BASE + XCAC208_CHAN_ADC_n_BASE + 0 + (n)*2+1)}
    
    {
        &(elemnet_t){
            "belikov", "Belikov's",
            "", "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("belikov", "Belikov's", "Iуст, A\vIкат, A\vUнгр, V", "2\v1", 3*2, 3)
                    DOVZH_LINE("2", 0),
                    DOVZH_LINE("1", 1),
                SUBELEM_END("noshadow,notitle", NULL),
            },
            "nocoltitles,norowtitles"
        }, 0
    },
#endif
    
    {NULL}
};

static physprops_t rack0_45_physinfo[] =
{
#include "pi_rack0_45.h"
};

static physprops_t rack0_44_physinfo[] =
{
#include "pi_rack0_44.h"
};

static physinfodb_rec_t beam_physinfo_database[] =
{
    {"rack0:44", rack0_44_physinfo, countof(rack0_44_physinfo)},
    {"rack0:45", rack0_45_physinfo, countof(rack0_45_physinfo)},
    {NULL},
};
