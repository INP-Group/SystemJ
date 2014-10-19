#include "common_elem_macros.h"

#include "drv_i/u0632_drv_i.h"
#include "ippclient.h"

#define PHYSINFO_ARRAY_NAME linac1_31_physinfo
#include "pi_linac1_31.h"

#define C_SENSOR1   1
#define REG_SENSOR1 1
#define C_SENSOR2   2
#define REG_SENSOR2 2
#define C_SENSOR3   3
#define REG_SENSOR3 3
#define C_SENSOR4   4
#define REG_SENSOR4 4
#define C_SENSOR5   11
#define REG_SENSOR5 5
#define C_SENSOR6   5
#define REG_SENSOR6 6
#define C_SENSOR7   8
#define REG_SENSOR7 7

enum
{
    LINIPP_WAIT_TIME   = 40, // 40s to wait while stepping motor moves
};

#define GET_TIME_LEFT(reg) \
    CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(reg),             /* Init reg if required */      \
    CMD_PUSH_I(0), CMD_PUSH_I(0),                          /* Zeroes for <0 and ==0 */     \
    CMD_GETLCLREG_I(reg),                                  /* Time when wait should end */ \
    CMD_GETTIME, CMD_SUB, CMD_DUP,                         /* Real value and selector */   \
    CMD_CASE                                               /* =0 if curtime>=endtime */

#define BOOLEANIZE(GET_VAL_CMDS, TRUE_VAL)                     \
    CMD_PUSH_I(TRUE_VAL), CMD_PUSH_I(0), CMD_PUSH_I(TRUE_VAL), \
    GET_VAL_CMDS, CMD_CASE

#define GET_DISABLE(reg)                                              \
    CMD_PUSH_I(0), CMD_PUSH_I(0), CMD_PUSH_I(CX_VALUE_DISABLED_MASK), \
    GET_TIME_LEFT(reg), CMD_CASE


#define IPP_ELEM(in_chan, reg, ndev, box, nl, title) \
    GLOBELEM_START("Box" #ndev "-" #box, "#" #ndev "." #box ": " title, \
                   2, 1) \
        SUBELEM_START("ctl", NULL, 4, 4) \
            {                                                 \
             "in",                                            \
             in_chan >= 0? "(*)"/*""*/               : " ",         \
             NULL, NULL,                                      \
             in_chan >= 0? "panel color=green" : NULL,        \
             in_chan >= 0? "Управление вдвинутостью сеточного датчика" : NULL, \
             in_chan >= 0? LOGT_WRITE1         : LOGT_READ,   \
             in_chan >= 0? LOGD_ONOFF          : LOGD_HLABEL, \
             in_chan >= 0? LOGK_CALCED         : LOGK_NOP,    \
             in_chan >= 0? LOGC_VIC            : LOGC_NORMAL, \
             PHYS_ID(-1),                                     \
             in_chan >= 0? (excmd_t[]){                       \
                  BOOLEANIZE(CMD_GETP_BYNAME_I("linac1:1!" __CX_STRINGIZE(in_chan)), 1), \
                  CMD_ADD_I(1), CMD_MOD_I(2),                 \
                  GET_DISABLE(reg),                           \
                  CMD_ADD, CMD_RET                            \
             }  : NULL,                                       \
             in_chan >= 0? (excmd_t[]){                       \
                  CMD_QRYVAL,                                 \
                  CMD_ADD_I(1), CMD_MOD_I(2),                 \
                  CMD_SETP_BYNAME_I("linac1:1!" __CX_STRINGIZE(in_chan)), \
                  CMD_GETTIME, CMD_ADD_I(LINIPP_WAIT_TIME),   \
                  CMD_SETLCLREG_I(reg),                       \
                  CMD_RET                                     \
             }  : NULL,                                       \
            0.9, 1.1, 0.0, 0.0, 1.0}, \
            {"sens",  "S",  NULL, NULL, "#+\v#T400.00e3\v100.00e3\v25.00e3\v6.25e3", "Чувствительность", \
             LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(U0632_NUMCHANS*ndev+U0632_CHAN_M_BASE+box)}, \
            {"delay", "T",  NULL, NULL, "#+\v#F32f,0.1088,0.0,%4.2fms", "Задержка перед измерением", \
             LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(U0632_NUMCHANS*ndev+U0632_CHAN_P_BASE+box)}, \
            {"magn", NULL, NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_IPP_MAGN, LOGK_NOP, LOGC_NORMAL, PHYS_ID(-1)}, \
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL), \
        SUBELEM_START("graph", NULL, 1, 1) \
            {"g-" #ndev "-" #box, NULL, NULL, NULL, "x=:..ctl.magn", NULL, LOGT_READ, LOGD_IPP_XY, LOGK_DIRECT, LOGC_NORMAL, \
             PHYS_ID(U0632_NUMCHANS * (ndev) +    \
                     U0632_CHAN_INTDATA_BASE +    \
                     U0632_CHANS_PER_UNIT * (box) \
                    )                             \
            }, \
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),                      \
    GLOBELEM_END("nohline,nocoltitles,norowtitles,fold_h", nl)

#define ESP_ELEM(in_chan, reg, ndev, box, nl, title) \
    GLOBELEM_START("Box" #ndev "-" #box, "#" #ndev "." #box ": " title, \
                   3, 1 + (1 << 24)) \
        SUBELEM_START("I", "I", 1, 1) \
            {"I", "I:", "A", "%-6.1f", "withlabel", "Ток спектрометра", LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(B_V1ADC+ 6*5)}, \
        SUBELEM_END("defserver=linac1:31 noshadow,notitle,nocoltitles,norowtitles", NULL), \
        SUBELEM_START("ctl", NULL, 4, 4) \
            {                                                 \
             "in",                                            \
             in_chan >= 0? "(*)"/*""*/               : " ",         \
             NULL, NULL,                                      \
             in_chan >= 0? "panel color=green" : NULL,        \
             in_chan >= 0? "Управление вдвинутостью сеточного датчика" : NULL, \
             in_chan >= 0? LOGT_WRITE1         : LOGT_READ,   \
             in_chan >= 0? LOGD_ONOFF          : LOGD_HLABEL, \
             in_chan >= 0? LOGK_CALCED         : LOGK_NOP,    \
             in_chan >= 0? LOGC_VIC            : LOGC_NORMAL, \
             PHYS_ID(-1),                                     \
             in_chan >= 0? (excmd_t[]){                       \
                  BOOLEANIZE(CMD_GETP_BYNAME_I("linac1:1!" __CX_STRINGIZE(in_chan)), 1), \
                  CMD_ADD_I(1), CMD_MOD_I(2),                 \
                  GET_DISABLE(reg),                           \
                  CMD_ADD, CMD_RET                            \
             }  : NULL,                                       \
             in_chan >= 0? (excmd_t[]){                       \
                  CMD_QRYVAL,                                 \
                  CMD_ADD_I(1), CMD_MOD_I(2),                 \
                  CMD_SETP_BYNAME_I("linac1:1!" __CX_STRINGIZE(in_chan)), \
                  CMD_GETTIME, CMD_ADD_I(LINIPP_WAIT_TIME),   \
                  CMD_SETLCLREG_I(reg),                       \
                  CMD_RET                                     \
             }  : NULL,                                       \
            0.9, 1.1, 0.0, 0.0, 1.0}, \
            {"sens",  "S",  NULL, NULL, "#+\v#T2000.00e3\v500.00e3\v125.00e3\v31.25e3", "Чувствительность", \
             LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(U0632_NUMCHANS*ndev+U0632_CHAN_M_BASE+box)}, \
            {"delay", "T",  NULL, NULL, "#+\v#F32f,0.1088,0.0,%4.2fms", "Задержка перед измерением", \
             LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(U0632_NUMCHANS*ndev+U0632_CHAN_P_BASE+box)}, \
            {"magn", NULL, NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_IPP_MAGN, LOGK_NOP, LOGC_NORMAL, PHYS_ID(-1)}, \
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL), \
        SUBELEM_START("graph", NULL, 1, 1) \
            {"g-" #ndev "-" #box, NULL, NULL, NULL, "i=:..I.I r=:..ctl.sens x=:..ctl.magn", NULL, LOGT_READ, LOGD_IPP_ESPECTR, LOGK_DIRECT, LOGC_NORMAL, \
             PHYS_ID(U0632_NUMCHANS * (ndev) +    \
                     U0632_CHAN_INTDATA_BASE +    \
                     U0632_CHANS_PER_UNIT * (box) \
                    )                             \
            }, \
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),                      \
    GLOBELEM_END("nohline,nocoltitles,norowtitles,fold_h", nl)


static groupunit_t linipp_grouping[] =
{
    IPP_ELEM(C_SENSOR1, REG_SENSOR1, 1, 1,  1, "Перед спектрометром"),
    ESP_ELEM(-1,        -1,          1, 14, 0, "180╟-Спектрометр"),
    IPP_ELEM(C_SENSOR2, REG_SENSOR2, 1, 8,  0, "После пучкового датчика"),
    IPP_ELEM(C_SENSOR3, REG_SENSOR3, 0, 4,  1, "Датчик 3 (после 5-й секции)"),
    IPP_ELEM(C_SENSOR4, REG_SENSOR4, 0, 7,  0, "Датчик 4 (середина техпромежутка #1)"),
    IPP_ELEM(C_SENSOR5, REG_SENSOR5, 0, 3,  0, "Датчик 5 (середина техпромежутка #2)"),
    IPP_ELEM(C_SENSOR6, REG_SENSOR6, 0, 0,  1, "Датчик 6 (перед поворотом)"),
    IPP_ELEM(-1, -1, 1, 13, 0, "Разводящий магнит"),
    IPP_ELEM(-1, -1, 1, 2,  0, "ИПП во 2-м зале (в перепуске)"),
    {NULL}
};

static physprops_t linac1_57_physinfo[] =
{
};

static physinfodb_rec_t linipp_physinfo_database[] =
{
    {"linac1:57", linac1_57_physinfo, countof(linac1_57_physinfo)},
    {"linac1:31", linac1_31_physinfo, countof(linac1_31_physinfo)},
    {NULL, NULL, 0}
};
