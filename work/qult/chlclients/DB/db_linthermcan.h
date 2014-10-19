#include "drv_i/xcandac16_drv_i.h"
#include "drv_i/xcanadc40_drv_i.h"

#include "common_elem_macros.h"


#define SUBSYSSIZE 10
#define N2X(N,START,BLKSIZE) (START + BLKSIZE * (N / SUBSYSSIZE) + (N % SUBSYSSIZE))
#define N2B(N,rw)            ( \
                              (((N) % SUBSYSSIZE) < 8)? \
                                  (DACS_BASE + XCANDAC16_NUMCHANS * ((N) / SUBSYSSIZE) + \
                                   (rw?XCANDAC16_CHAN_REGS_WR8_base:XCANDAC16_CHAN_REGS_RD8_base) + \
                                   + ((N) % SUBSYSSIZE)) \
                                  : \
                                  (ADCS_BASE + XCANADC40_NUMCHANS * ((N) / SUBSYSSIZE) + \
                                   (rw?XCANADC40_CHAN_REGS_WR8_base:XCANADC40_CHAN_REGS_RD8_base) + \
                                   + ((N) % SUBSYSSIZE) - 8) \
                             )

enum
{
    DACS_BASE = 0,
    ADCS_BASE = DACS_BASE + XCANDAC16_NUMCHANS * 2,

    POBX_BASE = 1200,
    T_IN_LIM  = POBX_BASE + 0,
};

#define LO_LIM 25
#define HI_LIM 40

enum
{
    R_RST_TIME_BASE = 100
};

#define N2IN_CN( n) N2X(n, ADCS_BASE+XCANADC40_CHAN_ADC_n_base + 0, XCANADC40_NUMCHANS)
#define N2SET_CN(n) N2X(n, DACS_BASE+XCANDAC16_CHAN_OUT_n_base + 0, XCANDAC16_NUMCHANS)
#define N2OUT_CN(n) N2X(n, ADCS_BASE+XCANADC40_CHAN_ADC_n_base +10, XCANADC40_NUMCHANS)
#define N2CRN_CN(n) N2X(n, ADCS_BASE+XCANADC40_CHAN_ADC_n_base +20, XCANADC40_NUMCHANS)
#define N2PWR_CN(n) N2X(n, ADCS_BASE+XCANADC40_CHAN_ADC_n_base +30, XCANADC40_NUMCHANS)
#define N2TRB_CN(n) N2B(n, 0)
#define N2RST_CN(n) N2B(n, 1)

#define THERMLINE(n, l) \
    {"in" #n, l,       "╟C",  "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(N2IN_CN (n)), .mindisp=+20, .maxdisp=+40}, \
    {"set"#n, l"_set", "╟C",  "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(N2SET_CN(n)), .mindisp=+20, .maxdisp=+40}, \
    {"out"#n, l"_out", "╟C",  "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(N2OUT_CN(n)), \
        (excmd_t[]){                                                   \
            CMD_RETSEP_I(0),                                           \
            CMD_GETP_I(N2OUT_CN(n)), CMD_GETP_I(N2SET_CN(n)), CMD_SUB, \
            CMD_RET                                                    \
        },                                                             \
        .minnorm=-1, .maxnorm=+1, .minyelw=-2, .maxyelw=+2, .mindisp=+20, .maxdisp=+40             \
    }, \
    {"crn"#n, l"_crn", "л/с", "%6.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(N2CRN_CN(n))}, \
    {"pwr"#n, l"_pwr", "%",   "%4.1f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(N2PWR_CN(n))}, \
    {"trb"#n, NULL,    NULL,  NULL,    NULL, NULL, LOGT_READ,   LOGD_ALARM,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(N2TRB_CN(n))}, \
    {"rst"#n, "Сброс", NULL,  NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(N2RST_CN(n)), \
        (excmd_t[]){ \
            CMD_PUSH_I(-1), CMD_SETLCLREGDEFVAL_I(R_RST_TIME_BASE + n), \
                                                                        \
            /* Is time-to-restore-at set? */                            \
            CMD_GETLCLREG_I(R_RST_TIME_BASE + n), CMD_PUSH_I(0),        \
            CMD_IF_LE_TEST, CMD_GOTO_I("ret"),                          \
            /* Is time still in the future? */                          \
            CMD_GETLCLREG_I(R_RST_TIME_BASE + n), CMD_GETTIME,          \
            CMD_IF_GT_TEST, CMD_GOTO_I("ret"),                          \
            /* Okay -- time to restore */                               \
            CMD_PUSH_I(0), CMD_ALLOW_W, CMD_SETP_I(N2RST_CN(n)),        \
            CMD_PUSH_I(-1), CMD_SETLCLREG_I(R_RST_TIME_BASE + n),       \
                                                                        \
            CMD_LABEL_I("ret"), CMD_RET_I(0),                           \
        },                                                              \
        (excmd_t[]){                                                    \
            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,                        \
            CMD_PUSH_I(1), CMD_SETP_I(N2RST_CN(n)),                     \
            CMD_GETTIME, CMD_ADD_I(0.5), CMD_SETLCLREG_I(R_RST_TIME_BASE + n),       \
            CMD_RET,                                                    \
        },                                                              \
    }

static groupunit_t linthermcan_grouping[] =
{
    GLOBELEM_START_CRN("ThermosM", "Термостабилизация",
                       "Вход\vЗадано, ╟C\vВыход\vРасход\vМощн.\v", NULL,
                       7*20 + 2, 7 + (2 << 24))

        {"-limit", "Tвход_max", "╟C", "%5.2f", NULL,
            "При передудонивании Tвход_cредн>Tвход_max бокр штеко трямкает",
            LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_DIM, PHYS_ID(T_IN_LIM)
        },
        SUBWINV4_START("-setlim", "Tвход_max", 1)
            {"-limit", "Tвход_max", NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(T_IN_LIM), .minnorm=LO_LIM, .maxnorm=HI_LIM}
        SUBWIN_END("...", "", ""),

        THERMLINE( 0, "ГРУП_П"),
        THERMLINE( 1, "SLED_1"),
        THERMLINE( 4, "SLED_2"),
        THERMLINE(13, "SLED_3"),
        THERMLINE(17, "SLED_4"),
        THERMLINE( 2, "SEC_01"),
        THERMLINE( 3, "SEC_02"),
        THERMLINE( 5, "SEC_03"),
        THERMLINE( 6, "SEC_04"),
        THERMLINE( 7, "SEC_05"),
        THERMLINE( 8, "SEC_06"),
        THERMLINE( 9, "SEC_07"),
        THERMLINE(10, "SEC_08"),
        THERMLINE(11, "SEC_09"),
        THERMLINE(12, "SEC_10"),
        THERMLINE(14, "SEC_11"),
        THERMLINE(15, "SEC_12"),
        THERMLINE(16, "SEC_13"),
        THERMLINE(18, "SEC_14"),
#if 1

#define CALC_IN_AVG() \
                CMD_GETP_I(N2IN_CN(0)),  CMD_GETP_I(N2IN_CN(1)),           CMD_ADD, \
                CMD_GETP_I(N2IN_CN(2)),  CMD_GETP_I(N2IN_CN(3)),  CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(4)),  CMD_GETP_I(N2IN_CN(5)),  CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(6)),  CMD_GETP_I(N2IN_CN(7)),  CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(8)),  CMD_GETP_I(N2IN_CN(9)),  CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(10)), CMD_GETP_I(N2IN_CN(11)), CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(12)), CMD_GETP_I(N2IN_CN(13)), CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(14)), CMD_GETP_I(N2IN_CN(15)), CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(16)), CMD_GETP_I(N2IN_CN(17)), CMD_ADD, CMD_ADD, \
                CMD_GETP_I(N2IN_CN(18)),                          CMD_ADD,          \
                CMD_DIV_I(19)

#define OVERHEAT_FLA() \
    CMD_GETP_I(T_IN_LIM), CMD_PUSH_I(0), CMD_IF_LE_TEST, CMD_BREAK_I(0.0), \
    CMD_GETP_I(T_IN_LIM), CMD_IF_GT_TEST, CMD_BREAK_I(1.0),              \
    CMD_RET_I(0.0)

        {"inA",  "AVG",    "╟C",  "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_CALCED, LOGC_IMPORTANT, PHYS_ID(-1),
            (excmd_t[]){
                CALC_IN_AVG(), CMD_RETSEP,
                CALC_IN_AVG(), OVERHEAT_FLA()
            }, .minyelw=0, .maxyelw=0.1, .mindisp=+20, .maxdisp=+40},
        {"setA", "setAVG", "╟C",  "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_GETP_I(N2SET_CN(0)),  CMD_GETP_I(N2SET_CN(1)),           CMD_ADD,
                CMD_GETP_I(N2SET_CN(2)),  CMD_GETP_I(N2SET_CN(3)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(4)),  CMD_GETP_I(N2SET_CN(5)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(6)),  CMD_GETP_I(N2SET_CN(7)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(8)),  CMD_GETP_I(N2SET_CN(9)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(10)), CMD_GETP_I(N2SET_CN(11)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(12)), CMD_GETP_I(N2SET_CN(13)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(14)), CMD_GETP_I(N2SET_CN(15)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(16)), CMD_GETP_I(N2SET_CN(17)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2SET_CN(18)),                           CMD_ADD,
                CMD_DIV_I(19), CMD_RET
            }, .mindisp=+20, .maxdisp=+40},
        {"outA", "outAVG", "╟C",  "%5.2f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_GETP_I(N2OUT_CN(0)),  CMD_GETP_I(N2OUT_CN(1)),           CMD_ADD,
                CMD_GETP_I(N2OUT_CN(2)),  CMD_GETP_I(N2OUT_CN(3)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(4)),  CMD_GETP_I(N2OUT_CN(5)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(6)),  CMD_GETP_I(N2OUT_CN(7)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(8)),  CMD_GETP_I(N2OUT_CN(9)),  CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(10)), CMD_GETP_I(N2OUT_CN(11)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(12)), CMD_GETP_I(N2OUT_CN(13)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(14)), CMD_GETP_I(N2OUT_CN(15)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(16)), CMD_GETP_I(N2OUT_CN(17)), CMD_ADD, CMD_ADD,
                CMD_GETP_I(N2OUT_CN(18)),                           CMD_ADD,
                CMD_DIV_I(19), CMD_RET
            }, .mindisp=+20, .maxdisp=+40},
        EMPTY_CELL(),
        EMPTY_CELL(),
        EMPTY_CELL(),
        {NULL, "<-", NULL, NULL, NULL, NULL, LOGT_READ, LOGD_ALARM, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CALC_IN_AVG(), OVERHEAT_FLA()
            }},
#else
        THERMLINE(19, "rsrv20"),
#endif
    GLOBELEM_END("", 1),

    {NULL}
};

#define THERM_PHYSLINE(n) \
    {N2IN_CN (n), +5000,                   -40, 0}, \
    {N2SET_CN(n), 100000,                  0.0, 305}, \
    {N2OUT_CN(n), +5000,                   -40, 0}, \
    {N2CRN_CN(n), -1/((1/1000000.0)*0.25), 0.0, 0}, \
    {N2PWR_CN(n), 1000,                    0.0, 0}

static physprops_t linthermcan_physinfo[] =
{
    THERM_PHYSLINE( 0),
    THERM_PHYSLINE( 1),
    THERM_PHYSLINE( 2),
    THERM_PHYSLINE( 3),
    THERM_PHYSLINE( 4),
    THERM_PHYSLINE( 5),
    THERM_PHYSLINE( 6),
    THERM_PHYSLINE( 7),
    THERM_PHYSLINE( 8),
    THERM_PHYSLINE( 9),
    THERM_PHYSLINE(10),
    THERM_PHYSLINE(11),
    THERM_PHYSLINE(12),
    THERM_PHYSLINE(13),
    THERM_PHYSLINE(14),
    THERM_PHYSLINE(15),
    THERM_PHYSLINE(16),
    THERM_PHYSLINE(17),
    THERM_PHYSLINE(18),
    THERM_PHYSLINE(19),

    {T_IN_LIM, 100000, 0, 0},
};
