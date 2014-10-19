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

static groupunit_t pursen_grouping[] =
{
    GLOBELEM_START("beam", "Пучок", 3, 1)
        SUBELEM_START_CRN("ebc", "Пучок", NULL, "?\v?\v?\v", 7 + 1, 2 + (1 << 24))
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

#define DAC_ADC_LINE(n) \
    {"dac" __CX_STRINGIZE(n), "DAC" __CX_STRINGIZE(n), NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_DAC_N_BASE+n)}, \
    {"adc" __CX_STRINGIZE(n), "ADC" __CX_STRINGIZE(n), NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+n)}
            SUBWINV4_START("service", "Служебные каналы контроллера EBC", 1)
                SUBELEM_START(":", NULL, 2, 2)
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

static physinfodb_rec_t pursen_physinfo_database[] =
{
    {"rack0:44", rack0_44_physinfo, countof(rack0_44_physinfo)},
    {"rack0:45", rack0_45_physinfo, countof(rack0_45_physinfo)},

    {NULL, NULL, 0}
};
