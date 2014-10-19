#include "drv_i/canadc40_drv_i.h"

#define BOOLEANIZE(GET_VAL_CMDS, TRUE_VAL)                     \
    CMD_PUSH_I(TRUE_VAL), CMD_PUSH_I(0), CMD_PUSH_I(TRUE_VAL), \
    GET_VAL_CMDS, CMD_CASE

#define B_TURNMAG 0
#define C_TURNMAG_STAT1 (B_TURNMAG + CANADC40_CHAN_REGS_RD8_BASE + 6)
#define C_TURNMAG_STAT2 (B_TURNMAG + CANADC40_CHAN_REGS_RD8_BASE + 7)
#define C_TURNMAG_MOVE1 (B_TURNMAG + CANADC40_CHAN_REGS_WR8_BASE + 6)
#define C_TURNMAG_MOVE2 (B_TURNMAG + CANADC40_CHAN_REGS_WR8_BASE + 7)
#define C_TURNMAG_IMES  (B_TURNMAG + CANADC40_CHAN_READ_N_BASE   + 31)

enum
{
    REG_TURNMAG_NUM100MS = 0,   // Time to run (in 100ms quants) upon [<] and [>] press
    REG_TURNMAG_RMN100MS = 1,   // Current time ReMaiNing to run
    REG_TURNMAG_LIM_IMOT = 2,   // Imot limit
    REG_TURNMAG_CTR_IMOT = 3,   // For how long time Imot>Ilim (in 100ms quants)
    REG_TURNMAG_100_IMOT = 4,   // For how long time may Imot stay >Ilim (in 100ms quants)
};

static groupunit_t turnmag_grouping[] =
{
    {
        &(elemnet_t){
            "-", "",
            "", "?\v?\v?\v?\v?",
            ELEM_SINGLECOL, 5, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP, .placement="horz=right"},
                {"loc", "Положение:", NULL, NULL, "#L\v#+\v#T00?!\f\flit=blue\vВ кольцо\f\flit=green\vВ могильник\f\flit=red\vМежду\f\flit=yellow", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_GETP_I(C_TURNMAG_STAT1),
                        CMD_GETP_I(C_TURNMAG_STAT2), CMD_MUL_I(2), CMD_ADD,
                        CMD_RET
                    }, .placement="horz=right"},
                {"Imot", "Ток мотора:", "A", "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_PUSH_I(1.5), CMD_SETLCLREGDEFVAL_I(REG_TURNMAG_LIM_IMOT),
                        CMD_GETP_I(C_TURNMAG_IMES), CMD_SETREG_I(0),

                        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_TURNMAG_CTR_IMOT),

                        CMD_GETREG_I(0), CMD_ABS,
                        CMD_GETLCLREG_I(REG_TURNMAG_LIM_IMOT),
                        CMD_IF_LT_TEST, CMD_GOTO_I("zero_ctr"),
                        CMD_GETLCLREG_I(REG_TURNMAG_CTR_IMOT), CMD_ADD_I(1), CMD_SETLCLREG_I(REG_TURNMAG_CTR_IMOT),
                        CMD_GOTO_I("skip_inc"),

                        CMD_LABEL_I("zero_ctr"),
                        CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TURNMAG_CTR_IMOT),
                        
                        CMD_LABEL_I("skip_inc"),

                        CMD_PUSH_I(10), CMD_SETLCLREGDEFVAL_I(REG_TURNMAG_100_IMOT),
                        
                        CMD_GETLCLREG_I(REG_TURNMAG_CTR_IMOT),
                        CMD_GETLCLREG_I(REG_TURNMAG_100_IMOT),
                        CMD_IF_LT_TEST, CMD_GOTO_I("skip_stop"),
                        CMD_DEBUGP_I("Stop!!!"),
                        CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TURNMAG_CTR_IMOT),
                        CMD_PUSH_I(0), CMD_ALLOW_W, CMD_SETP_I(C_TURNMAG_MOVE1),
                        CMD_PUSH_I(0), CMD_ALLOW_W, CMD_SETP_I(C_TURNMAG_MOVE2),
                        CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),
                        CMD_LABEL_I("skip_stop"),
                        
/////////CMD_SUB, CMD_SETREG_I(1),

//                        CMD_GETLCLREG_I(REG_TURNMAG_CTR_IMOT), CMD_ADD_I(1),

//                        CMD_SETLCLREG_I(REG_TURNMAG_CTR_IMOT),
                        
                        CMD_GETREG_I(0), CMD_RETSEP,

                        CMD_GETP_I(C_TURNMAG_IMES),
                        CMD_GETLCLREG_I(REG_TURNMAG_LIM_IMOT), CMD_DIV,
                        CMD_MUL_I(100), CMD_RET
                    }, .minnorm=0, .maxnorm=70, .minyelw=0, .maxyelw=100, .placement="horz=right"},
                {"Ilim", "Предел (A):",     "A", "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_GETLCLREG_I(REG_TURNMAG_LIM_IMOT), CMD_RET
                    },
                    (excmd_t []){
                        CMD_QRYVAL, CMD_SETLCLREG_I(REG_TURNMAG_LIM_IMOT), CMD_RET
                    }, .placement="horz=right"},
                {"Tlim", "Пред. время:", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "",
                        "",
                        NULL,
                        ELEM_MULTICOL, 2, 2,
                        (logchannet_t []){
                            {"-",       NULL, "с",  "%3.1f", NULL,        "", LOGT_READ,   LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETLCLREG_I(REG_TURNMAG_CTR_IMOT),
                                    CMD_DIV_I(10.0), CMD_RET
                                }},
                            {"T100lim", "из", NULL, "%3.1f", "withlabel", "", LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETLCLREG_I(REG_TURNMAG_100_IMOT),
                                    CMD_DIV_I(10.0), CMD_RET
                                },
                                (excmd_t []){
                                    CMD_QRYVAL, CMD_MUL_I(10.0),
                                    CMD_SETLCLREG_I(REG_TURNMAG_100_IMOT),
                                    CMD_RET
                                }, .minnorm=0, .maxnorm=5.0},
                        },
                        "noshadow,notitle,nocoltitles,norowtitles"
                    }
                },
            },
            "noshadow,notitle"
        }, 1
    },

    {
        &(elemnet_t){
            "-", "Ехать",
            "", "?\v?\v?\v",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                {"-", "До упора:", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "",
                        "",
                        NULL,
                        ELEM_MULTICOL, 3, 3,
                        (logchannet_t []){
                            {"to_l", "Могил.", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_PUSH_I(0),
                                    CMD_PUSH_I(1), CMD_GETP_I(C_TURNMAG_STAT1), CMD_SUB,
                                    CMD_TEST, CMD_ALLOW_W, CMD_SETP_I(C_TURNMAG_MOVE1),
                                    
                                    CMD_GETP_I(C_TURNMAG_MOVE1), CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    CMD_QRYVAL,    CMD_SETP_I(C_TURNMAG_MOVE1),
                                    CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE2),
                                    CMD_RET
                                }},
                            {"to_r", "Кольцо", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_PUSH_I(0),
                                    CMD_PUSH_I(1), CMD_GETP_I(C_TURNMAG_STAT2), CMD_SUB,
                                    CMD_TEST, CMD_ALLOW_W, CMD_SETP_I(C_TURNMAG_MOVE2),
                                    
                                    CMD_GETP_I(C_TURNMAG_MOVE2), CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    CMD_QRYVAL,    CMD_SETP_I(C_TURNMAG_MOVE2),
                                    CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE1),
                                    CMD_RET
                                }},
                            {"stop", "STOP", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){CMD_RET_I(0)},
                            (excmd_t []){
                                CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE1),
                                CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE2),
                                CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),
                                CMD_RET
                            }},
                        },
                        "noshadow,notitle,nocoltitles,norowtitles"
                    }
                },
                {"-", "100 мс:", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "",
                        "",
                        NULL,
                        ELEM_MULTICOL, 2, 3,
                        (logchannet_t []){
                            {"to_l", "Могил.", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_HILITED, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_TURNMAG_MOVE1), CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    CMD_QRYVAL,    CMD_SETP_I(C_TURNMAG_MOVE1),
                                    CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE2),
                                    CMD_PUSH_I(2),
                                    CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),
                                    CMD_RET
                                }},
                            {"to_r", "Кольцо", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_HILITED, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_TURNMAG_MOVE2), CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    CMD_QRYVAL,    CMD_SETP_I(C_TURNMAG_MOVE2),
                                    CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE1),
                                    CMD_PUSH_I(2),
                                    CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),
                                    CMD_RET
                                }},
                        },
                        "noshadow,notitle,nocoltitles,norowtitles"
                    }
                },
                {"-", "N*100мс:", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "",
                        "",
                        NULL,
                        ELEM_MULTICOL, 3, 3,
                        (logchannet_t []){
                            {"to_l", "Могил.", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_IMPORTANT, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_TURNMAG_MOVE1), CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    CMD_QRYVAL,    CMD_SETP_I(C_TURNMAG_MOVE1),
                                    CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE2),
                                    CMD_GETLCLREG_I(REG_TURNMAG_NUM100MS), CMD_ADD_I(1),
                                    CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),
                                    CMD_RET
                                }},
                            {"to_r", "Кольцо", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_IMPORTANT, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_TURNMAG_MOVE2), CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    CMD_QRYVAL,    CMD_SETP_I(C_TURNMAG_MOVE2),
                                    CMD_PUSH_I(0), CMD_SETP_I(C_TURNMAG_MOVE1),
                                    CMD_GETLCLREG_I(REG_TURNMAG_NUM100MS), CMD_ADD_I(1),
                                    CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),
                                    CMD_RET
                                }},
                            {"-", "N=", NULL, "%3.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_PUSH_I(1.0), CMD_SETLCLREGDEFVAL_I(REG_TURNMAG_NUM100MS),
                                    CMD_GETLCLREG_I(REG_TURNMAG_NUM100MS), CMD_RET,
                                },
                                (excmd_t []){
                                    CMD_QRYVAL, CMD_SETLCLREG_I(REG_TURNMAG_NUM100MS), CMD_RET,
                                }},
                        },
                        "noshadow,notitle,nocoltitles,norowtitles"
                    }
                },
                {"-", "", "мс", "%5.0f", NULL, "", LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_TURNMAG_RMN100MS),

                        CMD_GETLCLREG_I(REG_TURNMAG_RMN100MS), CMD_PUSH_I(0),
                        CMD_IF_LE_TEST, CMD_GOTO_I("ret_val"),

                        CMD_GETLCLREG_I(REG_TURNMAG_RMN100MS), CMD_SUB_I(1),
                        CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),

                        CMD_GETLCLREG_I(REG_TURNMAG_RMN100MS), CMD_PUSH_I(0),
                        CMD_IF_GT_TEST, CMD_GOTO_I("ret_val"),
                        CMD_PUSH_I(0), CMD_ALLOW_W, CMD_SETP_I(C_TURNMAG_MOVE1),
                        CMD_PUSH_I(0), CMD_ALLOW_W, CMD_SETP_I(C_TURNMAG_MOVE2),
                        CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_TURNMAG_RMN100MS),
                        
                        CMD_LABEL_I("ret_val"),
                        CMD_GETLCLREG_I(REG_TURNMAG_RMN100MS), CMD_MUL_I(100), CMD_RET,
                    },
                .placement="horz=right"}
            },
            "noshadow"
        }, 1
    },

    {NULL}
};

static physprops_t turnmag_physinfo[] =
{
    {C_TURNMAG_IMES, 1000000.0, 0},
};
