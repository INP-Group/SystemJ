#ifndef __MAGX_MACROS_H
#define __MAGX_MACROS_H


#include "drv_i/canadc40_drv_i.h"
#include "drv_i/candac16_drv_i.h"
#include "drv_i/xcdac20_drv_i.h"
#include "drv_i/ist_xcdac20_drv_i.h"
#include "drv_i/v1k_xcdac20_drv_i.h"
#include "drv_i/v3h_xa40d16_drv_i.h"
#include "drv_i/xcac208_drv_i.h"
#include "drv_i/xcanadc40_drv_i.h"
#include "drv_i/xcandac16_drv_i.h"


/*** General macros *************************************************/

#define MAGX_C100(c1, c2) (excmd_t []) \
    {                                  \
        CMD_RETSEP_I(0),               \
        CMD_GETP_I(c1),                \
        CMD_PUSH_I(0), CMD_IF_EQ_TEST, \
        CMD_BREAK_I(0),                \
        CMD_GETP_I(c1),                \
        CMD_GETP_I(c2),                \
        CMD_DIV,                       \
        CMD_SUB_I(1.0),                \
        CMD_ABS,                       \
        CMD_MUL_I(100.0),              \
        CMD_RET                        \
    }

#define MAGX_C100_SW(c_s, c1, c2) (excmd_t []) \
    {                                  \
        CMD_RETSEP_I(0),               \
        CMD_GETP_I(c_s),               \
        CMD_PUSH_I(0), CMD_IF_EQ_TEST, \
        CMD_BREAK_I(0),                \
        CMD_GETP_I(c1),                \
        CMD_PUSH_I(0), CMD_IF_EQ_TEST, \
        CMD_BREAK_I(0),                \
        CMD_GETP_I(c1),                \
        CMD_GETP_I(c2),                \
        CMD_DIV,                       \
        CMD_SUB_I(1.0),                \
        CMD_ABS,                       \
        CMD_MUL_I(100.0),              \
        CMD_RET                        \
    }

#define MAGX_CFWEIRD100(c_s, c1, c2) (excmd_t []) \
    {                                  \
        CMD_RETSEP_I(0),               \
        CMD_GETP_I(c_s),               \
        CMD_PUSH_I(0), CMD_IF_EQ_TEST, \
        CMD_BREAK_I(0),                \
        CMD_GETP_I(c1),                \
        CMD_GETP_I(c2),                \
        CMD_DIV,                       \
        CMD_SUB_I(1.0),                \
        CMD_ABS,                       \
        CMD_MUL_I(100.0),              \
        CMD_DUP,                       \
        CMD_DIV_I(1.0), /* 1% */       \
        CMD_TRUNC,                     \
        CMD_WEIRD,                     \
        CMD_RET                        \
    }

#define MAGX_RNGREDLED_LINE(id, label, cn) \
    {id,  label, NULL, NULL, "shape=circle",            NULL, LOGT_READ, LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .minyelw=-0.1, .maxyelw=+0.1}


/*** XCANADC40+XCANDAC16-based VCH300 *******************************/

#define MAGX_XCH300_COEFF (1000000.0*(8.0/300))
#define MAGX_XCH300_ONE_PHYSLINES(v3h_base)                               \
    {(v3h_base) + V3H_XA40D16_CHAN_DMS,      MAGX_XCH300_COEFF , 0, 0},   \
    {(v3h_base) + V3H_XA40D16_CHAN_STS,      MAGX_XCH300_COEFF , 0, 0},   \
    {(v3h_base) + V3H_XA40D16_CHAN_MES,      MAGX_XCH300_COEFF , 0, 0},   \
    {(v3h_base) + V3H_XA40D16_CHAN_MU1,      MAGX_XCH300_COEFF , 0, 0},   \
    {(v3h_base) + V3H_XA40D16_CHAN_MU2,      MAGX_XCH300_COEFF , 0, 0},   \
                                                                          \
    {(v3h_base) + V3H_XA40D16_CHAN_OUT,      MAGX_XCH300_COEFF , 0, 305}, \
    {(v3h_base) + V3H_XA40D16_CHAN_OUT_RATE, MAGX_XCH300_COEFF , 0, 305}, \
    {(v3h_base) + V3H_XA40D16_CHAN_OUT_CUR,  MAGX_XCH300_COEFF , 0, 305}

#define MAGX_XCH300_PHYSLINES(x40_base, x16_base, v3h_base)  \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  0, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  1, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  2, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  3, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  4, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  5, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  6, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  7, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  8, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base +  9, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base + 10, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base + 11, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base + 12, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base + 13, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base + 14, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_n_base + 15, 1000000.0, 0, 305}, \
                                                                      \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  0, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  1, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  2, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  3, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  4, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  5, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  6, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  7, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  8, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base +  9, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base + 10, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base + 11, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base + 12, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base + 13, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base + 14, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base + 15, 1000000.0, 0, 305}, \
                                                                           \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  0, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  1, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  2, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  3, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  4, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  5, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  6, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  7, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  8, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base +  9, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base + 10, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base + 11, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base + 12, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base + 13, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base + 14, 1000000.0, 0, 305}, \
    {(x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base + 15, 1000000.0, 0, 305}, \
                                                                          \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  0, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  1, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  2, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  3, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  4, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  5, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  6, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  7, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  8, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base +  9, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 10, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 11, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 12, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 13, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 14, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 15, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 16, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 17, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 18, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 19, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 20, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 21, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 22, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 23, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 24, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 25, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 26, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 27, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 28, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 29, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 30, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 31, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 32, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 33, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 34, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 35, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 36, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 37, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 38, 1000000.0, 0}, \
    {(x40_base) + XCANADC40_CHAN_ADC_n_base + 39, 1000000.0, 0}, \
                                                                      \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 0), \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 1), \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 2), \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 3), \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 4), \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 5), \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 6), \
    MAGX_XCH300_ONE_PHYSLINES((v3h_base) + V3H_XA40D16_NUMCHANS * 7)

// Note: v3n is related to x40/x16 ONLY, v3h_base should point to v3h_xa40d16 directly
#define MAGX_XCH300_LINE(id, label, tip, x40_base, x16_base, v3h_base, v3n, max) \
    {#id "_set", label,     NULL, "%6.1f", "groupable", tip,  LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_OUT), .minnorm=0, .maxnorm=max}, \
    {#id "_mes", label"_m", NULL, "%6.1f", NULL,        NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_MES), \
     MAGX_C100_SW((v3h_base) + V3H_XA40D16_CHAN_OPR,  \
                  (v3h_base) + V3H_XA40D16_CHAN_OUT,  \
                  (v3h_base) + V3H_XA40D16_CHAN_MES), \
     NULL, 0.0, 1.0, 0.0, 0.0, 2.0, 1.0, 0.0, max},                                   \
    {#id "_dvn", NULL,      NULL, "%6.3f", NULL,       NULL, LOGT_MINMAX, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_MES)},                     \
    {#id "_opr", NULL,      NULL, NULL, "shape=circle",NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_OPR)},                     \
    RGSWCH_LINE("-"__CX_STRINGIZE(id)"_swc", NULL, "0", "1", (v3h_base) + V3H_XA40D16_CHAN_OPR, (v3h_base) + V3H_XA40D16_CHAN_SWITCH_OFF, (v3h_base) + V3H_XA40D16_CHAN_SWITCH_ON), \
    SUBWINV4_START("-"__CX_STRINGIZE(id)"_ctl", label ": extended controls", 1)                                                                                                     \
        SUBELEM_START(":", NULL, 3, 3)                                                                                                                                              \
            SUBELEM_START("v3h", "VCh300", 3 + 4, 1 + (4<<24))                                                                                                                      \
                {"opr",       NULL,          NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_OPR)},   \
                RGSWCH_LINE("onoff", NULL, "Off", "On", (v3h_base) + V3H_XA40D16_CHAN_IS_ON, (v3h_base) + V3H_XA40D16_CHAN_SWITCH_OFF, (v3h_base) + V3H_XA40D16_CHAN_SWITCH_ON), \
                {"v3h_state", "V_S",         NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_V3H_STATE)},      \
                {"rst",       "R",           NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_RESET_STATE)},    \
                SUBELEM_START("ctl", "Control", 3, 3)                                                                                                                               \
                    {"dac",       "Set, A",      NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_OUT),      .minnorm=  0.0, .maxnorm=max},       \
                    {"dacspd",    "MaxSpd, A/s", NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_OUT_RATE), .mindisp=-2500*2, .maxdisp=+2500*2}, \
                    {"daccur",    "Cur, A",      NULL, "%7.1f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_OUT_CUR),  .mindisp=-2500,   .maxdisp=+2500},   \
                SUBELEM_END("notitle,noshadow,norowtitles", NULL),                                                                                                                               \
                SEP_L(),                                                                                                                                                                         \
                SUBELEM_START(":", NULL, 3, 3)                                                                                                                                                   \
                    SUBELEM_START("mes", "Measurements", 5, 1)                                                                                                                                   \
                        {"dms", "DAC control",         "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_DMS)}, \
                        {"sts", "Stab. shunt (DCCT2)", "V", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_STS)}, \
                        {"mes", "Meas. shunt (DCCT1)", "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_MES)}, \
                        {"u1",  "U1",                  "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_MU1)}, \
                        {"u2",  "U2",                  "A", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v3h_base) + V3H_XA40D16_CHAN_MU2)}, \
                    SUBELEM_END("noshadow,nocoltitles", NULL),                                                                                                                                   \
                    VSEP_L(),                                                                                                                                                                    \
                    MAGX_RNGREDLED_LINE("ilk", "Ilk",  (v3h_base) + V3H_XA40D16_CHAN_ILK),                                                                                                       \
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                                                                   \
            SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                                                                               \
            VSEP_L(),                                                                                                                                                                            \
            SUBELEM_START("c40d16", "XCANADC40+XCANDAC16", 3, 1)                                                                                                                                 \
                SUBELEM_START("dac", "DAC", 3, 3)                                                                                                                                                \
                    {"dac",       "Set, V",      NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(((x16_base) + XCANDAC16_CHAN_OUT_n_base      + (v3n))), .mindisp=-10.0, .maxdisp=+10.0}, \
                    {"dacspd",    "MaxSpd, V/s", NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(((x16_base) + XCANDAC16_CHAN_OUT_RATE_n_base + (v3n))), .mindisp=-20.0, .maxdisp=+20.0}, \
                    {"daccur",    "Cur, V",      NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(((x16_base) + XCANDAC16_CHAN_OUT_CUR_n_base  + (v3n))), .mindisp=-10.0, .maxdisp=+10.0}, \
                SUBELEM_END("notitle,noshadow,norowtitles", NULL),                                                                                                                               \
                SEP_L(),                                                                                                                                                                         \
                SUBELEM_START(":", NULL, 3, 3)                                                                                                                                                   \
                    SUBELEM_START("mes", "ADC, V", 5, 1)                                                                                                                                         \
                        {"adc0", "+0",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((x40_base) + XCANADC40_CHAN_ADC_n_base + (v3n)*5 + 0), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc1", "+1",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((x40_base) + XCANADC40_CHAN_ADC_n_base + (v3n)*5 + 1), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc2", "+2",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((x40_base) + XCANADC40_CHAN_ADC_n_base + (v3n)*5 + 2), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc3", "+3",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((x40_base) + XCANADC40_CHAN_ADC_n_base + (v3n)*5 + 3), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc4", "+4",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((x40_base) + XCANADC40_CHAN_ADC_n_base + (v3n)*5 + 4), .mindisp=-10.0, .maxdisp=+10.0},    \
                    SUBELEM_END("noshadow,nocoltitles", NULL),                                                                                                                                   \
                    VSEP_L(),                                                                                                                                                                    \
                    SUBELEM_START_CRN("io", "I/O", "DAC\vADC", NULL, 2*2, 2)                                                    \
                        LED_LINE    ("i0", NULL, ((x16_base) + XCANDAC16_CHAN_REGS_RD8_base + (v3n))),                                                \
                        LED_LINE    ("i1", NULL, ((x40_base) + XCANADC40_CHAN_REGS_RD8_base + (v3n))),                                                \
                        BLUELED_LINE("o0", NULL, ((x16_base) + XCANDAC16_CHAN_REGS_WR8_base + (v3n))),                                                \
                        BLUELED_LINE("o1", NULL, ((x40_base) + XCANADC40_CHAN_REGS_WR8_base + (v3n))),                                                \
                    SUBELEM_END("noshadow,norowtitles,transposed", NULL),                                                                     \
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                \
            SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                            \
        SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                        \
    SUBWIN_END("...", "", NULL)


/*** XCDAC20-based V1K **********************************************/

#define MAGX_X1K_XCDAC20_PHYSLINES(c20_base, v1k_base, coeff)             \
    /* XCDAC20 -- all as "read" */                                        \
    {(c20_base) + XCDAC20_CHAN_OUT_n_base      + 0, 1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_OUT_RATE_n_base + 0, 1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_OUT_CUR_n_base  + 0, 1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 0,      1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 1,      1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 2,      1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 3,      1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 4,      1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 5,      1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 6,      1000000., 0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 7,      1000000., 0, 0},   \
    /* V1K_XCDAC20 -- measurements */                               \
    {(v1k_base) + V1K_XCDAC20_CHAN_CURRENT1M,       coeff,    0, 0},   \
    {(v1k_base) + V1K_XCDAC20_CHAN_VOLTAGE2M,       1000000., 0, 0},   \
    {(v1k_base) + V1K_XCDAC20_CHAN_CURRENT2M,       coeff,    0, 0},   \
    {(v1k_base) + V1K_XCDAC20_CHAN_OUTVOLTAGE1M,    1000000., 0, 0},   \
    {(v1k_base) + V1K_XCDAC20_CHAN_OUTVOLTAGE2M,    1000000., 0, 0},   \
    {(v1k_base) + V1K_XCDAC20_CHAN_ADC_DAC,         coeff,    0, 0},   \
    {(v1k_base) + V1K_XCDAC20_CHAN_ADC_0V,          1000000., 0, 0},   \
    {(v1k_base) + V1K_XCDAC20_CHAN_ADC_P10V,        1000000., 0, 0},   \
    /* V1K_XCDAC20 -- settings */                                   \
    {(v1k_base) + V1K_XCDAC20_CHAN_OUT,             coeff,    0, 305}, \
    {(v1k_base) + V1K_XCDAC20_CHAN_OUT_RATE,        coeff,    0, 305}, \
    {(v1k_base) + V1K_XCDAC20_CHAN_OUT_CUR,         coeff,    0, 305}

#define MAGX_X1K_XCDAC20_ILK_LINE(id, l, v1k_base, c_suf)                                           \
            MAGX_RNGREDLED_LINE(id,     l,    (v1k_base) + __CX_CONCATENATE(V1K_XCDAC20_CHAN_ILK_,   c_suf)), \
            AMBERLED_LINE      (id"_c", NULL, (v1k_base) + __CX_CONCATENATE(V1K_XCDAC20_CHAN_C_ILK_, c_suf))
#define MAGX_X1K_XCDAC20_LINE(id, label, tip, c20_base, v1k_base, max, step) \
    {#id "_set", label,     NULL, "%6.1f", "groupable", tip,  LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OUT), .minnorm=  0.0, .maxnorm=max}, \
    {#id "_mes", label"_m", NULL, "%6.1f", NULL,        NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_CURRENT1M),                        \
     MAGX_C100_SW((v1k_base) + V1K_XCDAC20_CHAN_OPR, (v1k_base) + V1K_XCDAC20_CHAN_OUT, (v1k_base) + V1K_XCDAC20_CHAN_CURRENT1M),                                                   \
     NULL, 0.0, 1.0, 0.0, 0.0, 2.0, 1.0, 0.0, max},                                                                                                                                 \
    {#id "_dvn", NULL,      NULL, "%6.3f", NULL,        NULL, LOGT_MINMAX, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_CURRENT1M)},              \
    {#id "_opr", NULL,      NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OPR)},                    \
    RGSWCH_LINE("-"__CX_STRINGIZE(id)"_swc", NULL, "0", "1", (v1k_base) + V1K_XCDAC20_CHAN_OPR, (v1k_base) + V1K_XCDAC20_CHAN_SWITCH_OFF, (v1k_base) + V1K_XCDAC20_CHAN_SWITCH_ON), \
    SUBWINV4_START("-"__CX_STRINGIZE(id)"_ctl", label ": extended controls", 1)                                                                                                     \
        SUBELEM_START(":", NULL, 3, 3)                                                                                                                                              \
            SUBELEM_START("v1k", "V1000", 3 + 4, 1 + (4<<24))                                                                                                                       \
                {"opr",       NULL,          NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OPR)},   \
                RGSWCH_LINE("onoff", NULL, "Off", "On", (v1k_base) + V1K_XCDAC20_CHAN_IS_ON, (v1k_base) + V1K_XCDAC20_CHAN_SWITCH_OFF, (v1k_base) + V1K_XCDAC20_CHAN_SWITCH_ON),    \
                {"v1k_state", "V_S",         NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_V1K_STATE)},      \
                {"rst",       "R",           NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_RESET_STATE)},    \
                SUBELEM_START("ctl", "Control", 3, 3)                                                                                                                               \
                    {"dac",       "Set, A",      NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OUT),      .minnorm=  0.0, .maxnorm=max},       \
                    {"dacspd",    "MaxSpd, A/s", NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OUT_RATE), .mindisp=-2500*2, .maxdisp=+2500*2}, \
                    {"daccur",    "Cur, A",      NULL, "%7.1f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OUT_CUR),  .mindisp=-2500,   .maxdisp=+2500},   \
                SUBELEM_END("notitle,noshadow,norowtitles", NULL),                                                                                                                               \
                SEP_L(),                                                                                                                                                                         \
                SUBELEM_START(":", NULL, 3, 3)                                                                                                                                                   \
                    SUBELEM_START("mes", "Measurements", 8, 1)                                                                                                                                   \
                        {"current1m",     "Current1M",    "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_CURRENT1M)},           \
                        {"voltage2m",     "Voltage2M",    "V", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_VOLTAGE2M)},           \
                        {"current2m",     "Current2M",    "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_CURRENT2M)},           \
                        {"outvoltage1m",  "OutVoltage1M", "V", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OUTVOLTAGE1M)},        \
                        {"outvoltage2m",  "OutVoltage2M", "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_OUTVOLTAGE2M)},        \
                        {"adcdac",        "DAC@ADC",      "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_ADC_DAC)},             \
                        {"0v",            "0V",           "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_ADC_0V)},              \
                        {"10v",           "10V",          "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_ADC_P10V)},            \
                    SUBELEM_END("noshadow,nocoltitles", NULL),                                                                                                                                   \
                    VSEP_L(),                                                                                                                                                                    \
                    SUBELEM_START("ilk", "Interlocks", 5*2, 2)                                                                                                                                   \
                        MAGX_X1K_XCDAC20_ILK_LINE("overheat", "Overheating source",  (v1k_base), OVERHEAT),                                                                                      \
                        MAGX_X1K_XCDAC20_ILK_LINE("emergsht", "Emergency shutdown",  (v1k_base), EMERGSHT),                                                                                      \
                        MAGX_X1K_XCDAC20_ILK_LINE("currprot", "Current protection",  (v1k_base), CURRPROT),                                                                                      \
                        MAGX_X1K_XCDAC20_ILK_LINE("loadovrh", "Load overheat/water", (v1k_base), LOADOVRH),                                                                                      \
                        {"rst", "Reset", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_RST_ILKS)},                           \
                        {"rci", "R",     NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((v1k_base) + V1K_XCDAC20_CHAN_RESET_C_ILKS)},                       \
                    SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                                                                       \
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                                                                   \
            SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                                                                               \
            VSEP_L(),                                                                                                                                                                            \
            SUBELEM_START("xcdac20", "X-CDAC20", 3, 1)                                                                                                                                           \
                SUBELEM_START("dac", "DAC", 3, 3)                                                                                                                                                \
                    {"dac",       "Set, V",      NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_OUT_n_base      + 0), .mindisp=-10.0, .maxdisp=+10.0}, \
                    {"dacspd",    "MaxSpd, V/s", NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_OUT_RATE_n_base + 0), .mindisp=-20.0, .maxdisp=+20.0}, \
                    {"daccur",    "Cur, V",      NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_OUT_CUR_n_base  + 0), .mindisp=-10.0, .maxdisp=+10.0}, \
                SUBELEM_END("notitle,noshadow,norowtitles", NULL),                                                                                                                               \
                SEP_L(),                                                                                                                                                                         \
                SUBELEM_START(":", NULL, 3, 3)                                                                                                                                                   \
                    SUBELEM_START("mes", "ADC, V", 8, 1)                                                                                                                                         \
                        {"adc0", "0",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 0), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc1", "1",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 1), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc2", "2",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 2), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc3", "3",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 3), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc4", "4",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 4), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc5", "DAC", NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 5), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc6", "0V",  NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 6), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc7", "10V", NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 7), .mindisp=-10.0, .maxdisp=+10.0},    \
                    SUBELEM_END("noshadow,nocoltitles", NULL),                                                                                                                                   \
                    VSEP_L(),                                                                                                                                                                    \
                    SUBELEM_START_CRN("io", "I/O", "0\v1\v2\v3\v4\v5\v6\v7", NULL, 2*8, 8)                                                    \
                        LED_LINE    ("i0", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 0),                                                \
                        LED_LINE    ("i1", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 1),                                                \
                        LED_LINE    ("i2", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 2),                                                \
                        LED_LINE    ("i3", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 3),                                                \
                        LED_LINE    ("i4", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 4),                                                \
                        LED_LINE    ("i5", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 5),                                                \
                        LED_LINE    ("i6", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 6),                                                \
                        LED_LINE    ("i7", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 7),                                                \
                        BLUELED_LINE("o0", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 0),                                                \
                        BLUELED_LINE("o1", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 1),                                                \
                        BLUELED_LINE("o2", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 2),                                                \
                        BLUELED_LINE("o3", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 3),                                                \
                        BLUELED_LINE("o4", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 4),                                                \
                        BLUELED_LINE("o5", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 5),                                                \
                        BLUELED_LINE("o6", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 6),                                                \
                        BLUELED_LINE("o7", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 7),                                                \
                    SUBELEM_END("noshadow,norowtitles,transposed", NULL),                                                                     \
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                \
            SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                            \
        SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                        \
    SUBWIN_END("...", "", NULL)


/*** XCDAC20-based IST **********************************************/

#define MAGX_IST_XCDAC20_PHYSLINES(c20_base, ist_base, coeff, d2m)        \
    /* XCDAC20 -- all as "read" */                                        \
    {(c20_base) + XCDAC20_CHAN_OUT_n_base      + 0, 1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_OUT_RATE_n_base + 0, 1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_OUT_CUR_n_base  + 0, 1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 0,      1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 1,      1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 2,      1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 3,      1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 4,      1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 5,      1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 6,      1000000.,    0, 0},   \
    {(c20_base) + XCDAC20_CHAN_ADC_n_base + 7,      1000000.,    0, 0},   \
    /* IST_XCDAC20 -- measurements */                                  \
    {(ist_base) + IST_XCDAC20_CHAN_DCCT1,           coeff,       0, 0},   \
    {(ist_base) + IST_XCDAC20_CHAN_DCCT2,           coeff*(d2m), 0, 0},   \
    {(ist_base) + IST_XCDAC20_CHAN_DAC,             coeff,       0, 0},   \
    {(ist_base) + IST_XCDAC20_CHAN_U2,              1000000.,    0, 0},   \
    {(ist_base) + IST_XCDAC20_CHAN_ADC4,            1000000.,    0, 0},   \
    {(ist_base) + IST_XCDAC20_CHAN_ADC_DAC,         coeff,       0, 0},   \
    {(ist_base) + IST_XCDAC20_CHAN_ADC_0V,          1000000.,    0, 0},   \
    {(ist_base) + IST_XCDAC20_CHAN_ADC_P10V,        1000000.,    0, 0},   \
    /* IST_XCDAC20 -- settings */                                      \
    {(ist_base) + IST_XCDAC20_CHAN_OUT,             coeff,       0, 305}, \
    {(ist_base) + IST_XCDAC20_CHAN_OUT_RATE,        coeff,       0, 305}, \
    {(ist_base) + IST_XCDAC20_CHAN_OUT_CUR,         coeff,       0, 305}

#define MAGX_IST_XCDAC20_ILK_LINE(id, l, ist_base, c_suf)                                           \
            MAGX_RNGREDLED_LINE(id,     l,   (ist_base) + __CX_CONCATENATE(IST_XCDAC20_CHAN_ILK_,   c_suf)), \
            /*REDLED_LINE(id,   l,    (ist_base) + __CX_CONCATENATE(IST_XCDAC20_CHAN_ILK_,   c_suf)),*/ \
            AMBERLED_LINE      (id"_c", NULL,(ist_base) + __CX_CONCATENATE(IST_XCDAC20_CHAN_C_ILK_, c_suf))
#define MAGX_IST_XCDAC20_LINE(id, label, tip, c20_base, ist_base, max, step, weird_dcct2, reversable) \
    {#id "_set", label,     NULL, "%6.1f", "groupable", tip,  LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_OUT), .minnorm=(reversable)?-max:0.0, .maxnorm=max}, \
    {#id "_mes", label"_m", NULL, "%6.1f", NULL,        NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DCCT1),                           \
     MAGX_C100_SW((ist_base) + IST_XCDAC20_CHAN_OPR, (ist_base) + IST_XCDAC20_CHAN_OUT, (ist_base) + IST_XCDAC20_CHAN_DCCT1),                                                                                            \
     NULL, 0.0, 1.0, 0.0, 0.0, 2.0, 1.0, (reversable)?-max:0.0, max},                                                                                                                       \
    {#id "_dvn", NULL,      NULL, "%6.3f", NULL,        NULL, LOGT_MINMAX, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DCCT1)},                          \
    {#id "_opr", NULL,      NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_OPR),                             \
     (excmd_t[]){                                                 \
         CMD_RETSEP_I(0),                                         \
         CMD_GETP_I((c20_base) + XCDAC20_CHAN_REGS_WR8_base + 1), \
         CMD_GETP_I((ist_base) + IST_XCDAC20_CHAN_OPR),           \
         CMD_SUB, CMD_DUP, CMD_WEIRD,                             \
         CMD_RET,                                                 \
     }                                                            \
    },                                                            \
    RGSWCH_LINE("-"__CX_STRINGIZE(id)"_swc", NULL, "0", "1", (ist_base) + IST_XCDAC20_CHAN_OPR, (ist_base) + IST_XCDAC20_CHAN_SWITCH_OFF, (ist_base) + IST_XCDAC20_CHAN_SWITCH_ON), \
    SUBWINV4_START("-"__CX_STRINGIZE(id)"_ctl", label ": extended controls", 1)                                                                                                     \
        SUBELEM_START(":", NULL, 4, 3)                                                                                                                                              \
            SUBELEM_START("ist", "IST", 3 + 4, 1 + (4<<24))                                                                                                                         \
                {"opr",       NULL,          NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_OPR)},   \
                RGSWCH_LINE("onoff", NULL, "Off", "On", (ist_base) + IST_XCDAC20_CHAN_IS_ON, (ist_base) + IST_XCDAC20_CHAN_SWITCH_OFF, (ist_base) + IST_XCDAC20_CHAN_SWITCH_ON),    \
                {"ist_state", "I_S",         NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_IST_STATE)},      \
                {"rst",       "R",           NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_RESET_STATE)},    \
                SUBELEM_START("ctl", "Control", 3, 3)                                                                                                                               \
                    {"dac",       "Set, A",      NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_OUT),      .minnorm=(reversable)?-max:0.0, .maxnorm=max},       \
                    {"dacspd",    "MaxSpd, A/s", NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_OUT_RATE), .mindisp=-2500*2, .maxdisp=+2500*2}, \
                    {"daccur",    "Cur, A",      NULL, "%7.1f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_OUT_CUR),  .mindisp=-2500,   .maxdisp=+2500},   \
                SUBELEM_END("notitle,noshadow,norowtitles", NULL),                                                                                                                               \
                SEP_L(),                                                                                                                                                                         \
                SUBELEM_START(":", NULL, 3, 3)                                                                                                                                                   \
                    SUBELEM_START("mes", "Measurements", 8, 1)                                                                                                                                   \
                        {"dcct1",  "DCCT1",   "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DCCT1), .mindisp=-2500, .maxdisp=+2500},    \
                        {"dcct2",  "DCCT2",   "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DCCT2),                                     \
                         !weird_dcct2? NULL :                                                                                                                                                    \
                         MAGX_CFWEIRD100((ist_base) + IST_XCDAC20_CHAN_OUT_CUR,                                                                                                                  \
                                    (ist_base) + IST_XCDAC20_CHAN_DCCT1,                                                                                                                         \
                                    (ist_base) + IST_XCDAC20_CHAN_DCCT2),                                                                                                                        \
                         .mindisp=-2500, .maxdisp=+2500},                                                                                                                                        \
                        {"dacmes", "DAC",     "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DAC),      .mindisp=-2500, .maxdisp=+2500}, \
                        {"u2",     "U2",      "V", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_U2),       .mindisp=-2500, .maxdisp=+2500}, \
                        {"adc4",   "ADC4",    "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_ADC4),     .mindisp=-10,   .maxdisp=+10},   \
                        {"adcdac", "DAC@ADC", "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_ADC_DAC),  .mindisp=-2500, .maxdisp=+2500}, \
                        {"0v",     "0V",      "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_ADC_0V),   .mindisp=-10,   .maxdisp=+10},   \
                        {"10v",    "10V",     "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_ADC_P10V), .mindisp=-10,   .maxdisp=+10},   \
                    SUBELEM_END("noshadow,nocoltitles", NULL),                                                                                                                                   \
                    VSEP_L(),                                                                                                                                                                    \
                    SUBELEM_START("ilk", "Interlocks", 16, 2)                                                                                                                                    \
                        {"rst", "Reset", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_RST_ILKS)},                           \
                        {"rci", "R",     NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_RESET_C_ILKS)},                       \
                        MAGX_IST_XCDAC20_ILK_LINE("out",     "Out Prot", (ist_base), OUT_PROT),                                                                                                  \
                        MAGX_IST_XCDAC20_ILK_LINE("water",   "Water",    (ist_base), WATER),                                                                                                     \
                        MAGX_IST_XCDAC20_ILK_LINE("imax",    "Imax",     (ist_base), IMAX),                                                                                                      \
                        MAGX_IST_XCDAC20_ILK_LINE("umax",    "Umax",     (ist_base), UMAX),                                                                                                      \
                        MAGX_IST_XCDAC20_ILK_LINE("battery", "Battery",  (ist_base), BATTERY),                                                                                                   \
                        MAGX_IST_XCDAC20_ILK_LINE("phase",   "Phase",    (ist_base), PHASE),                                                                                                     \
                        MAGX_IST_XCDAC20_ILK_LINE("temp",    "Tempr",    (ist_base), TEMP),                                                                                                      \
                    SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                                                                       \
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                                                                   \
            SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                                                                               \
            VSEP_L(),                                                                                                                                                                            \
            SUBELEM_START("xcdac20", "X-CDAC20", 3, 1)                                                                                                                                           \
                SUBELEM_START("dac", "DAC", 3, 3)                                                                                                                                                \
                    {"dac",       "Set, V",      NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_OUT_n_base      + 0), .mindisp=-10.0, .maxdisp=+10.0}, \
                    {"dacspd",    "MaxSpd, V/s", NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_OUT_RATE_n_base + 0), .mindisp=-20.0, .maxdisp=+20.0}, \
                    {"daccur",    "Cur, V",      NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_OUT_CUR_n_base  + 0), .mindisp=-10.0, .maxdisp=+10.0}, \
                SUBELEM_END("notitle,noshadow,norowtitles", NULL),                                                                                                                               \
                SEP_L(),                                                                                                                                                                         \
                SUBELEM_START(":", NULL, 3, 3)                                                                                                                                                   \
                    SUBELEM_START("mes", "ADC, V", 8, 1)                                                                                                                                         \
                        {"adc0", "0",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 0), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc1", "1",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 1), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc2", "2",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 2), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc3", "3",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 3), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc4", "4",   NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 4), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc5", "DAC", NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 5), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc6", "0V",  NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 6), .mindisp=-10.0, .maxdisp=+10.0},    \
                        {"adc7", "10V", NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((c20_base) + XCDAC20_CHAN_ADC_n_base + 7), .mindisp=-10.0, .maxdisp=+10.0},    \
                    SUBELEM_END("noshadow,nocoltitles", NULL),                                                                                                                                   \
                    VSEP_L(),                                                                                                                                                                    \
                    SUBELEM_START_CRN("io", "I/O", "0\v1\v2\v3\v4\v5\v6\v7", NULL, 2*8, 8)                                                    \
                        LED_LINE    ("i0", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 0),                                                \
                        LED_LINE    ("i1", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 1),                                                \
                        LED_LINE    ("i2", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 2),                                                \
                        LED_LINE    ("i3", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 3),                                                \
                        LED_LINE    ("i4", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 4),                                                \
                        LED_LINE    ("i5", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 5),                                                \
                        LED_LINE    ("i6", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 6),                                                \
                        LED_LINE    ("i7", NULL, (c20_base) + XCDAC20_CHAN_REGS_RD8_base + 7),                                                \
                        BLUELED_LINE("o0", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 0),                                                \
                        BLUELED_LINE("o1", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 1),                                                \
                        BLUELED_LINE("o2", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 2),                                                \
                        BLUELED_LINE("o3", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 3),                                                \
                        BLUELED_LINE("o4", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 4),                                                \
                        BLUELED_LINE("o5", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 5),                                                \
                        BLUELED_LINE("o6", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 6),                                                \
                        BLUELED_LINE("o7", NULL, (c20_base) + XCDAC20_CHAN_REGS_WR8_base + 7),                                                \
                    SUBELEM_END("noshadow,norowtitles,transposed", NULL),                                                                     \
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                \
            SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),                                                                            \
            SUBELEM_START("-srvc", "clb_corr", 5, 5)                                                                                             \
                {"calb",    "Clbr DAC",   NULL, NULL,     NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DO_CALB_DAC)}, \
                {"dc_mode", "Dig. corr.", NULL, NULL,     NULL, NULL, LOGT_WRITE1, LOGD_ONOFF,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DIGCORR_MODE)}, \
                {"dc_val",  "",           NULL, "%-8.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_DIGCORR_FACTOR)}, \
                HLABEL_CELL(" "),                                                                                                                                                 \
                {"sign",    "S:",         NULL, "%+2.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((ist_base) + IST_XCDAC20_CHAN_CUR_POLARITY)}, \
            SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                    \
        SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),                                                                        \
    SUBWIN_END("...", "", NULL)


/*** XCANADC40+XCANDAC16-based magnetic correction ******************/

#define MAGX_XOR4016_NUMCHANS (XCANADC40_NUMCHANS + XCANDAC16_NUMCHANS)
#define MAGX_XOR4016_n2W(x4016_base, n) ((x4016_base) + ((n)/16)*MAGX_XOR4016_NUMCHANS + ((n)%16)*1 + XCANDAC16_CHAN_OUT_n_base + XCANADC40_NUMCHANS)
#define MAGX_XOR4016_n2I(x4016_base, n) ((x4016_base) + ((n)/16)*MAGX_XOR4016_NUMCHANS + ((n)%16)*2 + XCANADC40_CHAN_ADC_n_base + 0)
#define MAGX_XOR4016_n2U(x4016_base, n) ((x4016_base) + ((n)/16)*MAGX_XOR4016_NUMCHANS + ((n)%16)*2 + XCANADC40_CHAN_ADC_n_base + 1)

#define MAGX_XOR4016_PHYSLINES(x4016_base, n)                     \
    {MAGX_XOR4016_n2W((x4016_base), n), 5.4/10.0*1000, 0.0, 305}, \
    {MAGX_XOR4016_n2I((x4016_base), n), 5.4/10.0*1000, 0.0, 305}, \
    {MAGX_XOR4016_n2U((x4016_base), n), 1000000.0/3.7, 0.0, 305}

#define MAGX_XOR4016_LINE(x4016_base, n, id, label)                 \
    {#id"_set", label,     "", "%5.0f", "groupable", NULL,          \
     LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL,              \
     PHYS_ID(MAGX_XOR4016_n2W((x4016_base), n)),                    \
     .minnorm=-10000, .maxnorm=+10000, .incdec_step=100.0},         \
    {#id"_mes", label"_m", "", "%5.0f", NULL,        NULL,          \
     LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL,              \
     PHYS_ID(MAGX_XOR4016_n2I((x4016_base), n)),                    \
      MAGX_C100(MAGX_XOR4016_n2W((x4016_base), n),MAGX_XOR4016_n2I((x4016_base), n)), \
      NULL, 0.0, 5.0, 0.0, 0.0, 7.0},                               \
    {#id"_U",   label"_u", "", "%6.2f", NULL,        NULL,          \
     LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_DIM,                 \
     PHYS_ID(MAGX_XOR4016_n2U((x4016_base), n)), .mindisp=-50.0, .maxdisp=+50.0}



/*** XCAC208-based magnetic correction ******************************/

#define MAGX_COR208_PHYSLINES(dev_base, n) \
    {(dev_base) + XCAC208_CHAN_OUT_n_BASE + n,         5.4/10.0*1000, 0.0, 305}, \
    {(dev_base) + XCAC208_CHAN_ADC_n_BASE + n * 2 + 0, 5.4/10.0*1000, 0.0, 305}, \
    {(dev_base) + XCAC208_CHAN_ADC_n_BASE + n * 2 + 1, 1000000.0/3.7, 0.0, 305}

#define MAGX_COR208_LINE(base, n, id, l) \
    {__CX_STRINGIZE(id)"_set", l,     NULL, "%5.0f", "groupable", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + XCAC208_CHAN_OUT_n_BASE + n)},        \
    {__CX_STRINGIZE(id)"_mes", l"_m", NULL, "%5.0f", "", NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((base) + XCAC208_CHAN_ADC_n_BASE + n * 2 + 0), \
     (excmd_t[]){ \
         CMD_RETSEP_I(0), \
         CMD_RET_I(0), \
     }, \
    }, \
    {__CX_STRINGIZE(id)"_u",   l"_u", NULL, "%6.2f", "", NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_DIM,    PHYS_ID((base) + XCAC208_CHAN_ADC_n_BASE + n * 2 + 1), \
     .minyelw=-1.0, .maxyelw=+24.0}



#endif /* __MAGX_MACROS_H */
