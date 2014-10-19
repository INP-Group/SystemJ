#include "drv_i/canadc40_drv_i.h"

enum {MOTOR_I_C = CANADC40_CHAN_READ_N_BASE + 32};

enum
{
    RINGCAMSEL_WAIT_TIME    = 300, // 300s to wait while stepping motor moves
    RINGCAMSEL_REG_ENDWAIT  = 99,
    RINGCAMSEL_REG_ENDSTATE = 98,
    TRAILSTATE_BREAK = 0,
    TRAILSTATE_001   = 1,
    TRAILSTATE_010   = 2,
    TRAILSTATE_011   = 3,
    TRAILSTATE_IN    = 4,
    TRAILSTATE_101   = 5,
    TRAILSTATE_BETW  = 6,
    TRAILSTATE_OUT   = 7,
    TRAILSTATE_N_A   = -1
};


#define TRAIL_LCLREG(n, u) ((n)*2+(u))

#define ONE_TRAILER(u, n) \
    {NULL, NULL, NULL, NULL, "", NULL, LOGT_READ, u? LOGD_GREENLED:LOGD_ONOFF, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),\
     (excmd_t[]){CMD_GETLCLREG_I(TRAIL_LCLREG(n, u)), CMD_RET}}

#define TRAIL_LINE(u, s)   \
    ONE_TRAILER(u, (s)*8+0), \
    ONE_TRAILER(u, (s)*8+1), \
    ONE_TRAILER(u, (s)*8+2), \
    ONE_TRAILER(u, (s)*8+3), \
    ONE_TRAILER(u, (s)*8+4), \
    ONE_TRAILER(u, (s)*8+5), \
    ONE_TRAILER(u, (s)*8+6), \
    ONE_TRAILER(u, (s)*8+7)

#define LAB_TRAIL_LINE(u, s)   \
    {NULL, u? "^":"v", NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP}, \
    TRAIL_LINE(u, s)

enum {
    LREG_CAMSEL = 1,
    LREG_TRLSTT = 2,
    LREG_CMSLx2 = 3,
};
#define CAMSEL_R_F \
    /*CMD_PUSH_I(1.0), CMD_DUP_I(0),*/ /* Prepare values for CASE */       \
    CMD_GETP_I(CANADC40_CHAN_REGS_WR8_BASE+2), CMD_MUL_I(1),           \
    CMD_GETP_I(CANADC40_CHAN_REGS_WR8_BASE+3), CMD_MUL_I(2),  CMD_ADD, \
    CMD_GETP_I(CANADC40_CHAN_REGS_WR8_BASE+4), CMD_MUL_I(4),  CMD_ADD, \
    CMD_GETP_I(CANADC40_CHAN_REGS_WR8_BASE+0), CMD_MUL_I(8),  CMD_ADD, \
    CMD_GETP_I(CANADC40_CHAN_REGS_WR8_BASE+1), CMD_MUL_I(16), CMD_ADD, \
    CMD_SUB_I(8),    /* That is the result */                          \
    CMD_SETREG_I(LREG_CAMSEL), /* Store it */                          \
    CMD_PUSH_I(1.0), CMD_DUP_I(0), CMD_GETREG_I(LREG_CAMSEL),          \
    CMD_CASE, CMD_CALCERR, /* if(n<0) rflags|=CXRF_CALCERR */          \
    CMD_GETREG_I(LREG_CAMSEL)

#define CAMSEL_W_F \
    /*CMD_QRYVAL, CMD_SETP_I(CANADC40_CHAN_REGS_WR1),*/           \
    CMD_PUSH_I(0), CMD_SETP_I(CANADC40_CHAN_REGS_WR8_BASE+5), \
    CMD_QRYVAL, CMD_DIV_I(1),  CMD_MOD_I(2), CMD_TRUNC, CMD_SETP_I(CANADC40_CHAN_REGS_WR8_BASE+2), \
    CMD_QRYVAL, CMD_DIV_I(2),  CMD_MOD_I(2), CMD_TRUNC, CMD_SETP_I(CANADC40_CHAN_REGS_WR8_BASE+3), \
    CMD_QRYVAL, CMD_DIV_I(4),  CMD_MOD_I(2), CMD_TRUNC, CMD_SETP_I(CANADC40_CHAN_REGS_WR8_BASE+4), \
    CMD_QRYVAL, CMD_ADD_I(8), CMD_DIV_I(8),  CMD_MOD_I(2), CMD_TRUNC, CMD_SETP_I(CANADC40_CHAN_REGS_WR8_BASE+0), \
    CMD_QRYVAL, CMD_ADD_I(8), CMD_DIV_I(16), CMD_MOD_I(2), CMD_TRUNC, CMD_SETP_I(CANADC40_CHAN_REGS_WR8_BASE+1)

#define TRAILSTATE_R_F \
    /* Obtain current value of selector */                             \
    CAMSEL_R_F,                                                        \
    CMD_SETREG_I(LREG_CAMSEL),                                         \
    /* Check if it is negative */                                      \
    CMD_PUSH_I(1.0), CMD_DUP_I(0), CMD_GETREG_I(LREG_CAMSEL),          \
    CMD_CASE, CMD_TEST, CMD_BREAK_I(TRAILSTATE_N_A),                   \
    /* Calc an store value of selector *2 */                           \
    CMD_GETREG_I(LREG_CAMSEL),                                         \
    CMD_MUL_I(2), CMD_SETREG_I(LREG_CMSLx2),                           \
    /* Obtain trailers' state -- IN bits xxxx321x */                   \
    CMD_GETP_I(CANADC40_CHAN_REGS_RD1), CMD_DIV_I(2), CMD_MOD_I(8),    \
    CMD_SETREG_I(LREG_TRLSTT),                                         \
    /* Set individual checkboxes' registers */                         \
    CMD_PUSH_I(0), CMD_PUSH_I(1), CMD_PUSH_I(0),                       \
        CMD_GETREG_I(LREG_TRLSTT), CMD_SUB_I(4), CMD_CASE,             \
        CMD_GETREG_I(LREG_CMSLx2), CMD_ADD_I(1+OP_ARG_REG_TYPE_LCL),   \
        CMD_SETREG,                                                    \
    CMD_PUSH_I(0), CMD_PUSH_I(1), CMD_PUSH_I(0),                       \
        CMD_GETREG_I(LREG_TRLSTT), CMD_SUB_I(7), CMD_CASE,             \
        CMD_GETREG_I(LREG_CMSLx2), CMD_ADD_I(0+OP_ARG_REG_TYPE_LCL),   \
        CMD_SETREG,                                                    \
    /* "if (curstate==endstate) set reg_ENDWAIT:=0" */                 \
    CMD_PUSH_I(0) /* Prepare 0 to store in ENDWAIT reg */,             \
    CMD_PUSH_I(0), CMD_PUSH_I(1), CMD_PUSH_I(0),                       \
        CMD_GETREG_I(LREG_TRLSTT), CMD_GETLCLREG_I(RINGCAMSEL_REG_ENDSTATE), \
        CMD_SUB, CMD_CASE,                                             \
        CMD_TEST, CMD_SETLCLREG_I(RINGCAMSEL_REG_ENDWAIT),             \
    /* "if (curstate==endstate) set reg_ENDSTATE:=N/A" */              \
    CMD_PUSH_I(TRAILSTATE_N_A) /* Prepare 0 to store in ENDSTATE reg */,\
    CMD_PUSH_I(0), CMD_PUSH_I(1), CMD_PUSH_I(0),                       \
        CMD_GETREG_I(LREG_TRLSTT), CMD_GETLCLREG_I(RINGCAMSEL_REG_ENDSTATE), \
        CMD_SUB, CMD_CASE,                                             \
        CMD_TEST, CMD_SETREG_I(RINGCAMSEL_REG_ENDSTATE),               \
    /* The trailers' state value */                                    \
    CMD_GETREG_I(LREG_TRLSTT)

#define GET_TIME_LEFT(reg) \
    CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(reg),             /* Init reg if required */      \
    CMD_PUSH_I(0), CMD_PUSH_I(0),                          /* Zeroes for <0 and ==0 */     \
    CMD_GETLCLREG_I(reg),                                  /* Time when wait should end */ \
    CMD_GETTIME, CMD_SUB, CMD_DUP,                         /* Real value and selector */   \
    CMD_CASE                                               /* =0 if curtime>=endtime */

#define TIM_CHAN(reg) \
    {"-progress", NULL, "с", "%-3.0f", "thermometer,size=90", "", LOGT_READ, LOGD_HSLIDER, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t []){GET_TIME_LEFT(reg), CMD_RET}, \
        .mindisp=0, .maxdisp=RINGCAMSEL_WAIT_TIME}

static groupunit_t ringcamsel_grouping[] =
{
    {
        &(elemnet_t){
            "Control", "Управление",
            "", "\vКамера\vСостояние\vВведенность\vТок мотора\vВремя до...",
            ELEM_SINGLECOL, 6, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP, .placement="horz=right"},
                {"", NULL, NULL, NULL,
                    "#T"
                        "Электроны 1\f\flit=green\v"
                        "Электроны 2\f\flit=green\v"
                        "Электроны 3\f\flit=green\v"
                        "Электроны 4\f\flit=green\v"
                        "Электроны 5\f\flit=green\v"
                        "Электроны 6\f\flit=green\v"
                        "Электроны 7\f\flit=green\v"
                        "Электроны 8\f\flit=green\v"
                        "Кольцо 1\f\flit=yellow\v"
                        "Кольцо 2\f\flit=yellow\v"
                        "Кольцо 3\f\flit=yellow\v"
                        "Кольцо 4\f\flit=yellow\v"
                        "Кольцо 5\f\flit=yellow\v"
                        "Кольцо 6\f\flit=yellow\v"
                        "Кольцо 7\f\flit=yellow\v"
                        "Кольцо 8\f\flit=yellow\v"
                        "Позитроны 1\f\flit=red\v"
                        "Позитроны 2\f\flit=red\v"
                        "Позитроны 3\f\flit=red\v"
                        "Позитроны 4\f\flit=red\v"
                        "Позитроны 5\f\flit=red\v"
                        "Позитроны 6\f\flit=red\v"
                        "Позитроны 7\f\flit=red\v"
                        "Позитроны 8\f\flit=red",
                    NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_CALCED, LOGC_VIC, PHYS_ID(-1),
                    (excmd_t[]){CAMSEL_R_F, CMD_RETSEP, CAMSEL_R_F, CMD_RET},
                    (excmd_t[]){CAMSEL_W_F,
                                CMD_GETTIME, CMD_ADD_I(RINGCAMSEL_WAIT_TIME),
                                CMD_SETLCLREG_I(RINGCAMSEL_REG_ENDWAIT),
                                CMD_PUSH_I(TRAILSTATE_OUT),
                                CMD_SETLCLREG_I(RINGCAMSEL_REG_ENDSTATE),
                                CMD_RET_I(0)},
                    .minnorm=0, .maxnorm=7, .minyelw=0, .maxyelw=15},
                {"", NULL, NULL, NULL,
                    "#-#T"
                        "Обрыв кабеля\v"
                        "Хрень - 001\v"
                        "Хрень - 010\v"
                        "Хрень - 011\v"
                        "Верхний (вдвинуто)\v"
                        "Хрень - 101\v"
                        "Между\v"
                        "Нижний (выдв)",
                    NULL, LOGT_READ*1+LOGT_WRITE1*0, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t[]){TRAILSTATE_R_F, CMD_RET}},
                {"", "Введено", NULL, NULL, "color=green", NULL,
                    LOGT_WRITE1, LOGD_ONOFF, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t[]){CMD_GETP_I(CANADC40_CHAN_REGS_WR8_BASE+5), CMD_RET},
                    (excmd_t[]){CMD_QRYVAL, CMD_SETP_I(CANADC40_CHAN_REGS_WR8_BASE+5),
                                CMD_GETTIME, CMD_ADD_I(RINGCAMSEL_WAIT_TIME),
                                CMD_SETLCLREG_I(RINGCAMSEL_REG_ENDWAIT),

                                CMD_PUSH_I(TRAILSTATE_N_A),
                                CMD_PUSH_I(TRAILSTATE_OUT),
                                CMD_PUSH_I(TRAILSTATE_IN),
                                CMD_QRYVAL, CMD_CASE, CMD_SETLCLREG_I(RINGCAMSEL_REG_ENDSTATE),

                                CMD_RET}
                },
                {"", NULL, "A", "%5.3f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(MOTOR_I_C)},
                TIM_CHAN(RINGCAMSEL_REG_ENDWAIT),
            },
            "notitle,noshadow,nocoltitles"
        }, 1
    },
    {
        &(elemnet_t){
            "Selector", "Концевики",
            " \v1\v2\v3\v4\v5\v6\v7\v8", "Электроны\v \vКольцо\v \vПозитроны\v ",
            ELEM_MULTICOL, 6*9, 9,
            (logchannet_t []){
                LAB_TRAIL_LINE(1, 0),
                LAB_TRAIL_LINE(0, 0),
                LAB_TRAIL_LINE(1, 1),
                LAB_TRAIL_LINE(0, 1),
                LAB_TRAIL_LINE(1, 2),
                LAB_TRAIL_LINE(0, 2),
            },
            "noshadow,fold_h"
        }, 1
    },
    {NULL}
};

static physprops_t ringcamsel_physinfo[] =
{
    {MOTOR_I_C, 1000000.0, 0},
};
