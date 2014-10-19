#include "common_elem_macros.h"

#include "drv_i/senkov_ebc_drv_i.h"

enum
{
    EBC_BASE      = 0,

    C_WORK  = EBC_BASE+SENKOV_EBC_CHAN_REGS_WR8_BASE+0,
    C_RESET = EBC_BASE+SENKOV_EBC_CHAN_REGS_WR8_BASE+1,

    R_RESET_TIME = 0,
};

static groupunit_t senkov_ebc_grouping[] =
{
    {
        &(elemnet_t){
            "ctl", "Controls", NULL, NULL,
            ELEM_MULTICOL, 3, 3,
            (logchannet_t []){

                SUBELEM_START_CRN("ctl", "Controls", NULL, "Выходное напряжение\vТок накала", 6, 3)
                    {"Uset",  "UвыхУст", NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_DAC_N_BASE+0), .mindisp=0, .maxdisp=60.0},
                    {"Umes",  "UвыхИзм", NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+1), .mindisp=0, .maxdisp=60.0},
                    {"onoff", "",        NULL, NULL,
                        "#-#TВыкл\f\flit=red\vВкл\f\flit=green",
                        NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]){
                            CMD_PUSH_I(1),
                            CMD_GETP_I(C_WORK),
                            CMD_SUB,
                            CMD_RET
                        },
                        (excmd_t[]){
                            CMD_PUSH_I(1),
                            CMD_QRYVAL,
                            CMD_SUB,
                            CMD_SETP_I(C_WORK),
                            CMD_RET
                        },
                    },
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
                            
                            CMD_PUSH_I(0), CMD_ALLOW_W, CMD_PRGLY, CMD_SETP_I(C_RESET),
                            CMD_PUSH_I(0), CMD_SETLCLREG_I(R_RESET_TIME),
                            CMD_RET_I(0.0)
                        },
                        (excmd_t[]){
                            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                            CMD_PUSH_I(1), CMD_PRGLY, CMD_SETP_I(C_RESET),
                            
                            CMD_GETTIME, CMD_SETLCLREG_I(R_RESET_TIME), CMD_REFRESH_I(1.0),
                            CMD_RET
                        },
                    },
                SUBELEM_END("noshadow,notitle,nocoltitles", "horz=left"),
                
#define DAC_ADC_LINE(n) \
    {"dac" __CX_STRINGIZE(n), "DAC" __CX_STRINGIZE(n), NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_DAC_N_BASE+n)}, \
    {"adc" __CX_STRINGIZE(n), "ADC" __CX_STRINGIZE(n), NULL, "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+n)}
                SUBWINV4_START("service", "Служебные каналы контроллера EBC", 1)
                    SUBELEM_START("content", NULL, 2, 2)
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
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                SUBWIN_END("Служебные\nканалы...", "noshadow,notitle,nocoltitles,norowtitles", "horz=right"),

                SUBELEM_START(NULL, NULL, 1, 1)
                    {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=right"),
                
            },
            "noshadow,notitle,nocoltitles,norowtitles"
        }, 1 | GU_FLAG_FILLHORZ*1
    },

    {
        &(elemnet_t){
            "histplot", "Histplot",
            "", "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"histplot", "HistPlot", NULL, NULL, "width=800,height=600,plot1=.Uset,plot2=.Umes,plot3=.Iheat,fixed", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
            },
            "notitle,nocoltitles,norowtitles"
        }, 1*1
    },

    {NULL}
};

static physprops_t senkov_ebc_physinfo[] =
{
    {EBC_BASE+SENKOV_EBC_CHAN_DAC_N_BASE+0, 5000/100.0},
    {EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+0, 5000/50.0},
    {EBC_BASE+SENKOV_EBC_CHAN_ADC_N_BASE+1, 5000/100.0},
};
