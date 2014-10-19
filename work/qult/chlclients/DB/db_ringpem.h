#include "drv_i/v06xx_drv_i.h"

#define B_RINGPEM 0

#define C_RINGPEM_MOVE1 (B_RINGPEM + V06XX_CHAN_WR_N_BASE + 1)
#define C_RINGPEM_MOVE2 (B_RINGPEM + V06XX_CHAN_WR_N_BASE + 0)

enum
{
    REG_RINGPEM_RMN100MS = 1,   // Current time ReMaiNing to run
    REG_RINGPEM_DISABLE  = 2,   // :=CX_VALUE_DISABLED_MASK when running
    REG_RINGPEM_CURFLTR  = 3,   // Current filter #, or 1000000 if unknown
};

enum
{
    V_RINBPEM_NFLTRS  = 6,
    V_RINGPEM_UNKFLTR = 1000000,
    V_RINGPEM_ONETIME = (1)*10, // Time to move by one
    V_RINGPEM_BIGTIME = (10)*10, // Time to move to some end
};

#define RINGPEM_STOP_CMD                                     \
    CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_RINGPEM_RMN100MS),    \
    CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_RINGPEM_DISABLE),     \
    CMD_PUSH_I(0), CMD_ALLOW_W, CMD_SETP_I(C_RINGPEM_MOVE1), \
    CMD_PUSH_I(0), CMD_ALLOW_W, CMD_SETP_I(C_RINGPEM_MOVE2)

#define RINGPEM_MOVE_CMD(left, ttm)                                          \
    CMD_QRYVAL,    CMD_SETP_I(left? C_RINGPEM_MOVE1 : C_RINGPEM_MOVE2),      \
    CMD_PUSH_I(0), CMD_SETP_I(left? C_RINGPEM_MOVE2 : C_RINGPEM_MOVE1),      \
    CMD_PUSH_I(ttm), CMD_SETLCLREG_I(REG_RINGPEM_RMN100MS),                  \
    CMD_PUSH_I(CX_VALUE_DISABLED_MASK), CMD_SETLCLREG_I(REG_RINGPEM_DISABLE)

static groupunit_t ringpem_grouping[] =
{
#if 1
    {
        &(elemnet_t){
            "-", "Езда",
            "", "\v?\v?\v?\v?",
            ELEM_SINGLECOL, 5, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP, .placement="horz=right"},
                {"pos",   "Фильтр #", NULL, NULL, "#L\v#T???\v1\v2\v3\v4\v5\v6", "", LOGT_READ, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        // Initialize registers
                        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_RINGPEM_RMN100MS),
                        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_RINGPEM_DISABLE),
                        CMD_PUSH_I(V_RINGPEM_UNKFLTR), CMD_SETLCLREGDEFVAL_I(REG_RINGPEM_CURFLTR),

                        // Manage RMN100MS
                        /// Is it >0?
                        CMD_GETLCLREG_I(REG_RINGPEM_RMN100MS), CMD_PUSH_I(0),
                        CMD_IF_LE_TEST, CMD_GOTO_I("skip_if_rmn_zero"),
                        /// Decrement by 1
                        CMD_GETLCLREG_I(REG_RINGPEM_RMN100MS), CMD_SUB_I(1),
                        CMD_SETLCLREG_I(REG_RINGPEM_RMN100MS),
                        /// Is it still >0, or should we perform STOP?
                        CMD_GETLCLREG_I(REG_RINGPEM_RMN100MS), CMD_PUSH_I(0),
                        CMD_IF_GT_TEST, CMD_GOTO_I("skip_if_rmn_zero"),
                        RINGPEM_STOP_CMD,
                        CMD_LABEL_I("skip_if_rmn_zero"),
                        
                        // Obtain current filter #
                        CMD_GETLCLREG_I(REG_RINGPEM_CURFLTR), CMD_ADD_I(1),
                        CMD_SETREG_I(0),

                        // Isn't it too little?
                        CMD_GETREG_I(0), CMD_PUSH_I(0),
                        CMD_IF_GE_TEST, CMD_GOTO_I("skip_if_pos"),
                        CMD_PUSH_I(0), CMD_SETREG_I(0),
                        CMD_LABEL_I("skip_if_pos"),

                        // Isn't it too big?
                        CMD_GETREG_I(0), CMD_PUSH_I(V_RINBPEM_NFLTRS),
                        CMD_IF_LE_TEST, CMD_GOTO_I("skip_if_sml"),
                        CMD_PUSH_I(0), CMD_SETREG_I(0),
                        CMD_LABEL_I("skip_if_sml"),

                        CMD_GETREG_I(0), CMD_RET
                    }},
                {"-", "1 шаг:", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "",
                        "",
                        NULL,
                        ELEM_MULTICOL, 2, 2,
                        (logchannet_t []){
                            {"to_l", NULL, NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_ARROW_LT, LOGK_CALCED, LOGC_HILITED, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_RINGPEM_MOVE1),
                                    CMD_GETLCLREG_I(REG_RINGPEM_DISABLE),
                                    CMD_ADD,
                                    CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    RINGPEM_MOVE_CMD(1, V_RINGPEM_ONETIME),

                                    CMD_GETLCLREG_I(REG_RINGPEM_CURFLTR), CMD_PUSH_I(0),
                                    CMD_IF_LE_TEST, CMD_GOTO_I("skip_step"),
                                    CMD_GETLCLREG_I(REG_RINGPEM_CURFLTR), CMD_SUB_I(1),
                                    CMD_SETLCLREG_I(REG_RINGPEM_CURFLTR),
                                    CMD_LABEL_I("skip_step"),

                                    CMD_RET
                                }},
                            {"to_r", NULL, NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_ARROW_RT, LOGK_CALCED, LOGC_HILITED, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_RINGPEM_MOVE2),
                                    CMD_GETLCLREG_I(REG_RINGPEM_DISABLE),
                                    CMD_ADD,
                                    CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    RINGPEM_MOVE_CMD(0, V_RINGPEM_ONETIME),

                                    CMD_GETLCLREG_I(REG_RINGPEM_CURFLTR), CMD_PUSH_I(V_RINBPEM_NFLTRS-1),
                                    CMD_IF_GE_TEST, CMD_GOTO_I("skip_step"),
                                    CMD_GETLCLREG_I(REG_RINGPEM_CURFLTR), CMD_ADD_I(1),
                                    CMD_SETLCLREG_I(REG_RINGPEM_CURFLTR),
                                    CMD_LABEL_I("skip_step"),

                                    CMD_RET
                                }},
                        },
                        "noshadow,notitle,nocoltitles,norowtitles"
                    }
                },
                {"-", "До упора:", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "",
                        "",
                        NULL,
                        ELEM_MULTICOL, 3, 3,
                        (logchannet_t []){
                            {"to_l", NULL, NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_ARROW_LT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_RINGPEM_MOVE1),
                                    CMD_GETLCLREG_I(REG_RINGPEM_DISABLE),
                                    CMD_ADD,
                                    CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    RINGPEM_MOVE_CMD(1, V_RINGPEM_BIGTIME),
                                    CMD_PUSH_I(0),                  CMD_SETLCLREG_I(REG_RINGPEM_CURFLTR),
                                    CMD_RET
                                }},
                            {"to_r", NULL, NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_ARROW_RT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETP_I(C_RINGPEM_MOVE2),
                                    CMD_GETLCLREG_I(REG_RINGPEM_DISABLE),
                                    CMD_ADD,
                                    CMD_RET
                                },
                                (excmd_t []){
                                    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                    RINGPEM_MOVE_CMD(0, V_RINGPEM_BIGTIME),
                                    CMD_PUSH_I(V_RINBPEM_NFLTRS-1), CMD_SETLCLREG_I(REG_RINGPEM_CURFLTR),
                                    CMD_RET
                                }},
                            {"stop", "STOP", NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){CMD_RET_I(0)},
                            (excmd_t []){
                                CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                RINGPEM_STOP_CMD,
                                CMD_PUSH_I(V_RINGPEM_UNKFLTR), CMD_SETLCLREG_I(REG_RINGPEM_CURFLTR),
                                CMD_RET
                            }},
                        },
                        "noshadow,notitle,nocoltitles,norowtitles"
                    }
                },
                {"left", "Осталось", "с", "%4.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_GETLCLREG_I(REG_RINGPEM_RMN100MS), CMD_DIV_I(10.0), CMD_RET
                    }},
            },
            "notitle,noshadow,nocoltitles"
        }, 1
    },
#else
    {
        &(elemnet_t){
            "-", "Езда",
            "", "",
            ELEM_MULTICOL, 2, 2,
            (logchannet_t []){
                {"to_l", NULL, NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_ARROW_LT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_RINGPEM_RMN100MS),
                        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_RINGPEM_DISABLE),
                        
                        CMD_GETP_I(C_RINGPEM_MOVE1),
                        CMD_GETLCLREG_I(REG_RINGPEM_DISABLE),
                        CMD_ADD,
                        CMD_RET
                    },
                    (excmd_t []){
                        CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                        //CMD_QRYVAL,    CMD_SETP_I(C_RINGPEM_MOVE1),
                        //CMD_PUSH_I(0), CMD_SETP_I(C_RINGPEM_MOVE2),
                        //CMD_PUSH_I(2),
                        //CMD_SETLCLREG_I(REG_RST100MS),
                        CMD_RET
                    }},
                {"to_r", NULL, NULL, NULL, NULL, "", LOGT_WRITE1, LOGD_ARROW_RT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_GETP_I(C_RINGPEM_MOVE2),
                        CMD_GETLCLREG_I(REG_RINGPEM_DISABLE),
                        CMD_ADD,
                        CMD_RET
                    },
                    (excmd_t []){
                        CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                        //CMD_QRYVAL,    CMD_SETP_I(C_RINGPEM_MOVE2),
                        //CMD_PUSH_I(0), CMD_SETP_I(C_RINGPEM_MOVE1),
                        //CMD_PUSH_I(2),
                        //CMD_SETLCLREG_I(REG_RST100MS),
                        CMD_RET
                    }},
            },
            "noshadow,nocoltitles,norowtitles"
        }, 1
    },
#endif
    
    {NULL}
};



static physprops_t ringpem_physinfo[] =
{
};
