#include "common_elem_macros.h"

#include "drv_i/xcac208_drv_i.h"
#include "drv_i/senkov_vip_drv_i.h"
#include "drv_i/weld02_drv_i.h"
#include "drv_i/dds300_drv_i.h"
#include "drv_i/frolov_bfp_drv_i.h"
#include "drv_i/smc8_drv_i.h"
#include "drv_i/nkshd485_drv_i.h"

#include "weldclient.h"


enum
{
    VIP_BASE      = 0,
    WELD02_BASE   = 50,
    XCAC208_1_BASE = 68,
    XCAC208_2_BASE = 166,
    BFP_BASE      = 264,
    DDS300_BASE   = 274,

    SMC8_A_BASE   = 284,
    SMC8_B_BASE   = 484,

    NKSHD485_X_BASE = 684,
    NKSHD485_Y_BASE = 784,
    NKSHD485_R_BASE = 884,
};

enum
{
    XCAC208_2_TABLE_ALL_BIGC = 17
};


enum
{
    C_I_ACX_S  = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 6, C_I_ACX_M  = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 12,
    C_I_ACY_S  = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 7, C_I_ACY_M  = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 13,
    C_I_LD_S   = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 0, C_I_LD_M   = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 0,
    C_I_LS_S   = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 1, C_I_LS_M   = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 1,
    C_I_DC1X_S = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 2, C_I_DC1X_M = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 4,
    C_I_DC1Y_S = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 3, C_I_DC1Y_M = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 5,
    C_I_C2X_S  = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 4, C_I_C2X_M  = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 8,
    C_I_C2Y_S  = XCAC208_2_BASE + XCAC208_CHAN_OUT_n_BASE + 5, C_I_C2Y_M  = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 9,

    C_I_LD_R   = XCAC208_2_BASE + XCAC208_CHAN_OUT_RATE_n_BASE + 0,

    C_U_FEED_M = XCAC208_2_BASE + XCAC208_CHAN_ADC_n_BASE + 16,
};

enum
{
    REG_COEFF_STEPPER_A = 301,
    REG_COEFF_STEPPER_B = 302,
    REG_COEFF_STEPPER_R = 303,
};

#define COEFF_A (1/1.9834e-4)
#define COEFF_B (1/(360/(960*10.5)))
#define COEFF_R (1/8.25e-3)
    

static groupunit_t weldcc_grouping[] =
{

#define VIP_STAT_LINE(id, short_l, long_l, cn) \
    {id, short_l, NULL, NULL, "shape=circle", long_l, LOGT_READ, LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define VIP_SET_LINE(id, label, units, dpyfmt, cn, max_alwd) \
    {id, label, units, dpyfmt, "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .placement="", .minnorm=0, .maxnorm=max_alwd}

#define VIP_MES_LINE(id, label, units, dpyfmt, cn, dmin, dmax) \
    {id, label, units, dpyfmt, NULL,        NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .placement="", .mindisp=dmin, .maxdisp=dmax}

    {
        &(elemnet_t){
            "vip", "Высоковольтный источник питания",
            "", "",
            ELEM_MULTICOL, 4, 4,
            (logchannet_t []){
                SUBELEM_START("ctl", "Управление", 8, 1)
                    SUBELEM_START("setts", "Уставки", 2, 1)
                        VIP_SET_LINE("u_high", "Высокое", "kV", "%4.1f", VIP_BASE + SENKOV_VIP_CHAN_SET_DAC_UOUT, 60.0),
                        VIP_SET_LINE("k_corr", "K корр.", "",   "%4.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_DAC_CORR, 255),
                    SUBELEM_END("notitle,noshadow,nocoltitles", ""),
                    LED_LINE("pwr",  "3ф-сеть подана",    VIP_BASE + SENKOV_VIP_CHAN_MODE_POWER_ON),
                    LED_LINE("act",  "Пускатель вкл.",    VIP_BASE + SENKOV_VIP_CHAN_MODE_ACTUATOR_ON),
                    SUBELEM_START("onoff", "Вкл/выкл", 2, 1)
                        RGSWCH_LINE("vip", "ВИП",      "Выкл", "Вкл",
                                    VIP_BASE + SENKOV_VIP_CHAN_MODE_VIP_ON,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_VIP_OFF,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_VIP_ON),
                        RGSWCH_LINE("hvl", "Высокое", "Выкл", "Вкл",
                                    VIP_BASE + SENKOV_VIP_CHAN_MODE_OPERATE,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_HVL_OFF,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_HVL_ON),
                    SUBELEM_END("notitle,noshadow,nocoltitles", ""),
                    LED_LINE("manu", "Ручное управление", VIP_BASE + SENKOV_VIP_CHAN_MODE_MANUAL),
                    LED_LINE("lock", "Блокировка",        VIP_BASE + SENKOV_VIP_CHAN_MODE_VIP_BLOCKED),
                    EMPTY_CELL(),
                    SUBWIN_START("service", "Служебные каналы контроллера ВИП", 5, 1)
                        VIP_SET_LINE("reserved3"    , "Резервный канал",                  "",   "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_SET_RESERVED3,     255),
                        VIP_SET_LINE("uso_coeff"    , "Коэфф. усиления УСО",              "",   "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_USO_COEFF,     255),
                        VIP_SET_LINE("vip_invrt_frq", "k частоты работы инвертора ВИП",   "",   "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_VIP_INVRT_FRQ, 1000),
                        VIP_SET_LINE("vip_iout_prot", "Защита по выходному току ВИП",     "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_SET_VIP_IOUT_PROT, 1200.0),
                        VIP_SET_LINE("brkdn_count",   "Число пробоев/1с для выкл. на 1с", "",   "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_BRKDN_COUNT,   21),
                    SUBWIN_END("Служебные...", "noshadow,notitle,nocoltitles", "horz=center"),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles,norowtitles", ""),
                {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP, LOGK_NOP, .placement="vert=fill"},
                SUBELEM_START("mes", "Измерения", 13, 1)
                    VIP_MES_LINE("vip_uout",      "Uвых ВИП",          "kV", "%5.2f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_UOUT,      0, 70),
                    VIP_MES_LINE("vip_iout_full", "Iвых ВИП (полн.)",  "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_FULL, 0, 0),
                    VIP_MES_LINE("vip_iout_prec", "Iвых ВИП (<100мА)", "mA", "%6.2f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_PREC, 0, 0),
                    VIP_MES_LINE("transf_iin",    "Iвх ВВ трансф.",    "A",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_TRANSF_IIN,    0, 0),
                    VIP_MES_LINE("vip_3furect",   "Uвыпр 3ф-сети",     "V",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_3FURECT,   0, 0),
                    VIP_MES_LINE("vip_3fi",       "Iпотр 3ф-сети",     "A",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_3FI,       0, 0),
                    VIP_MES_LINE("vip_t_trrad",   "T транзисторов",    "╟C", "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_T_TRRAD,   0, 0),
                    VIP_MES_LINE("tank_t",        "T внутри ВВ бака",  "╟C", "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_TANK_T,        0, 0),
                    VIP_MES_LINE("reserved9",     "резерв",            "",   "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_RESERVED9,     0, 0),
                    VIP_MES_LINE("vip_iout_auto", "Iвых ВИП (авто)",   "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AUTO, 0, 0),
                    VIP_MES_LINE("vip_iout_avg",  "Iвых ВИП (усред.)", "mA", "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AVG,  0, 0),
                    VIP_MES_LINE("brkdn_count",   "Число пробоев",     "",   "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_CUR_BRKDN_COUNT,   0, 0),
                    {"-watch", "^Следить^", NULL, NULL,
                        "#L\v#TНет\vДа\f\flit=green",
                        "Следить за [Числом Пробоев], и при новом пробое\n"
                        "восстанавливать уставку [I накала]",
                        LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]) {
                            CMD_PUSH_I(0),  CMD_SETLCLREGDEFVAL_I(REG_WATCH),
                            CMD_PUSH_I(-1), CMD_SETLCLREGDEFVAL_I(REG_WATCH_TIME),

                            // Are we watching?
                            CMD_GETLCLREG_I(REG_WATCH), CMD_PUSH_I(0.9),
                            CMD_IF_LT_TEST, CMD_GOTO_I("ret"),

                            // Is number-of-breakdowns <255?
                            CMD_GETP_I(VIP_BASE + SENKOV_VIP_CHAN_CUR_BRKDN_COUNT),
                            CMD_PUSH_I(255),
                            CMD_IF_LT_TEST, CMD_GOTO_I("lt255"),
                            // No, should switch off the feature...
                            CMD_PUSH_I(0), CMD_SETLCLREG_I(REG_WATCH),
                            CMD_GOTO_I("ret"),

                            // Yes, <255
                            CMD_LABEL_I("lt255"),
                            // Have more breakdowns occured?
                            CMD_GETP_I(VIP_BASE + SENKOV_VIP_CHAN_CUR_BRKDN_COUNT),
                            CMD_GETLCLREG_I(REG_WATCH_BDPREV),
                            CMD_IF_LE_TEST, CMD_GOTO_I("should_restore?"),
                            // O-o-ops... More...
                            // Save current count...
                            CMD_GETP_I(VIP_BASE + SENKOV_VIP_CHAN_CUR_BRKDN_COUNT),
                            CMD_SETLCLREG_I(REG_WATCH_BDPREV),
                            // ...and schedule restore now+2sec
                            CMD_GETTIME, CMD_ADD_I(2.0),
                            CMD_SETLCLREG_I(REG_WATCH_TIME),
                            CMD_GOTO_I("ret"),
                            
                            //
                            CMD_LABEL_I("should_restore?"),

                            // Is time-to-restore-at set?
                            CMD_GETLCLREG_I(REG_WATCH_TIME), CMD_PUSH_I(0),
                            CMD_IF_LT_TEST, CMD_GOTO_I("ret"),
                            // Is time in the past?
                            CMD_GETLCLREG_I(REG_WATCH_TIME), CMD_GETTIME,
                            CMD_IF_GT_TEST, CMD_GOTO_I("ret"),

                            // Yes! Let's do restore
                            CMD_DEBUGP_I("Doing a restore..."),
                            CMD_GETLCLREG_I(REG_SET_UH), CMD_ALLOW_W, CMD_SETP_I(WELD02_BASE + WELD02_CHAN_SET_UH),
                            CMD_GETLCLREG_I(REG_SET_UL), CMD_ALLOW_W, CMD_SETP_I(WELD02_BASE + WELD02_CHAN_SET_UL),
                            CMD_GETLCLREG_I(REG_SET_UN), CMD_ALLOW_W, CMD_SETP_I(WELD02_BASE + WELD02_CHAN_SET_UN),
                            CMD_PUSH_I(-1), CMD_SETLCLREG_I(REG_WATCH_TIME),
                            
                            CMD_LABEL_I("ret"), CMD_GETLCLREG_I(REG_WATCH), CMD_RET
                        },
                        (excmd_t[]) {
                            CMD_QRYVAL, CMD_SETLCLREG_I(REG_WATCH),

                            // Is it On or Off?  Off(==0)=>return
                            CMD_QRYVAL, CMD_PUSH_I(0.9),
                            CMD_IF_LT_TEST, CMD_GOTO_I("ret"),

                            // Okay, let's set control variables...
                            CMD_GETP_I(VIP_BASE + SENKOV_VIP_CHAN_CUR_BRKDN_COUNT),
                            CMD_SETLCLREG_I(REG_WATCH_BDPREV),
                            CMD_PUSH_I(-1), CMD_SETLCLREG_I(REG_WATCH_TIME),
                            // ...and remember current data
                            CMD_GETP_I(WELD02_BASE + WELD02_CHAN_SET_UH),
                            CMD_SETLCLREG_I(REG_SET_UH),
                            CMD_GETP_I(WELD02_BASE + WELD02_CHAN_SET_UL),
                            CMD_SETLCLREG_I(REG_SET_UL),
                            CMD_GETP_I(WELD02_BASE + WELD02_CHAN_SET_UN),
                            CMD_SETLCLREG_I(REG_SET_UN),
                            
                            CMD_LABEL_I("ret"), CMD_RET
                        },
                    },
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", ""),
                SUBELEM_START("stat", "?", 13, 1)
                    VIP_STAT_LINE("vip_uout_gt_70kv"  , "Uвых>70",   "", VIP_BASE + SENKOV_VIP_CHAN_STAT_VIP_UOUT_GT_70kV),
                    VIP_STAT_LINE("vip_iout_gt_1100ma", "Iвых>1100", "", VIP_BASE + SENKOV_VIP_CHAN_STAT_VIP_IOUT_GT_1100mA),
                    VIP_STAT_LINE("feedback_break"    , "Обрыв ОС",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_FEEDBACK_BREAK),
                    VIP_STAT_LINE("transf_iin_gt_300a", "Iтрнс>300", "", VIP_BASE + SENKOV_VIP_CHAN_STAT_TRANSF_IIN_GT_300A),
                    VIP_STAT_LINE("phase_break"       , "Обр.фазы",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_PHASE_BREAK),
                    VIP_STAT_LINE("vip_iin_gt_250a"   , "Iвх>250",   "", VIP_BASE + SENKOV_VIP_CHAN_STAT_VIP_IIN_GT_250A),
                    VIP_STAT_LINE("ttemp_gt_45c"      , "Tтранз>45", "", VIP_BASE + SENKOV_VIP_CHAN_STAT_TTEMP_GT_45C),
                    VIP_STAT_LINE("tank_t_gt_60c"     , "Tбака>60",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_TANK_T_GT_60C),
                    VIP_STAT_LINE("no_water"          , "Нет воды",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_NO_WATER),
                    VIP_STAT_LINE("ext_block1_ubs"    , "Б1 УБС",    "", VIP_BASE + SENKOV_VIP_CHAN_STAT_EXT_BLOCK1_UBS),
                    VIP_STAT_LINE("ext_block2_ubs"    , "Б2 УБС",    "", VIP_BASE + SENKOV_VIP_CHAN_STAT_EXT_BLOCK2_UBS),
                    VIP_STAT_LINE("constant_brkdn"    , "Постоянн",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_CONSTANT_BRKDN),
                    VIP_STAT_LINE("igbt_protection"   , "IGBT",      "", VIP_BASE + SENKOV_VIP_CHAN_STAT_IGBT_PROTECTION),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles,norowtitles", ""),
            },
            "nocoltitles,norowtitles"
        }
    },

#define ECTL_LINE(id, label, units, dpyfmt, cn_s, cn_m, reg_n, max_alwd, max_mes_disp) \
    {id "_s", label " уст.", units, dpyfmt, "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(cn_s), \
        (excmd_t []){ \
            CMD_GETP_I(cn_s), \
            CMD_DUP, CMD_SETLCLREGDEFVAL_I(reg_n), \
            CMD_RET, \
        }, \
        (excmd_t []){ \
            CMD_QRYVAL, \
            CMD_DUP, CMD_SETLCLREG_I(reg_n), \
            CMD_SETP_I(cn_s), \
            CMD_RET, \
        }, \
     .maxnorm=max_alwd}, \
    {id "_m", label " изм.",  NULL,  dpyfmt, NULL,       NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_m), .mindisp=0, .maxdisp=max_mes_disp}

#define ECTL_STAB_LINE(id, label, dpyfmt, cn_s, cn_m, max_mes_disp) \
    {id "_s", label " уст.", NULL,  dpyfmt, NULL,        NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_s)}, \
    {id "_m", label " изм.", NULL,  dpyfmt, NULL,        NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_m), .placement="horz=right", .mindisp = 0, .maxdisp=max_mes_disp}
    
#define ECTL_SEL_LINE(id, label, items, cn_s, cn_m) \
    {id "_s", label, NULL, NULL,    "#L\v#T" items, NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_s)}, \
    EMPTY_CELL() //{id "_m", NULL,  NULL, "%5.0f", NULL,           NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_m), .placement="horz=right"}
    
#define ECTL_MES_LINE(id, label, cn, dmax) \
    {id, label, NULL, "", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .placement="horz=right", .mindisp=0, .maxdisp=dmax}
    
    {
        &(elemnet_t){
            "gunctl", "Контроллер пушки",
            "", "",
            ELEM_MULTICOL, 3, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("ctl", "Управление", NULL, "U упр.эл. отп.\vU упр.эл. зап.\vI накала\vI упр.эл.", 8, 2)
                    ECTL_LINE("u_mesh_op", "U упр.эл. отп.", "V", "%6.0f", WELD02_BASE + WELD02_CHAN_SET_UH, WELD02_BASE + WELD02_CHAN_MES_UH, REG_SET_UH, 6000, 0),
                    ECTL_LINE("u_mesh_cl", "U упр.эл. зап.", "V", "%6.0f", WELD02_BASE + WELD02_CHAN_SET_UL, WELD02_BASE + WELD02_CHAN_MES_UL, REG_SET_UL, 6000, 0),
                    ECTL_LINE("i_heat",    "I накала",       "A", "%6.0f", WELD02_BASE + WELD02_CHAN_SET_UN, WELD02_BASE + WELD02_CHAN_MES_UN, REG_SET_UN, 85,   100),
#if 0
                    ECTL_LINE("u_electr",  "U электрода",    "V", "%6.0f", WELD02_BASE + WELD02_CHAN_SET_UP, WELD02_BASE + WELD02_CHAN_MES_UP,  250),
#else
                    //{"u_electr_s", "U электрода", "V",   "%6.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(WELD02_BASE + WELD02_CHAN_SET_UP), .maxnorm=250},
                    //{NULL, "\nI упр.эл.",  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP},
                    {"", "^restore^", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t []){
                            CMD_RET_I(0),
                        },
                        (excmd_t []){
                            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                            CMD_GETLCLREG_I(REG_SET_UH), CMD_SETP_I(WELD02_BASE + WELD02_CHAN_SET_UH),
                            CMD_GETLCLREG_I(REG_SET_UL), CMD_SETP_I(WELD02_BASE + WELD02_CHAN_SET_UL),
                            CMD_GETLCLREG_I(REG_SET_UN), CMD_SETP_I(WELD02_BASE + WELD02_CHAN_SET_UN),
                            CMD_RET,
                        },
                    },
                    {"i_electr_m", "I упр.эл. изм.",         "mkA", "%3.0f", NULL,        NULL, LOGT_READ,   LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t []){
                            CMD_GETP_I(WELD02_BASE + WELD02_CHAN_MES_UP),
                            CMD_MUL_I(1000), // Convert to millivolts
                            CMD_LAPPROX_FROM_I("weld_mes_up_mv2mka", 2*65536 + 1),
                            CMD_RET,
                        },
                        .mindisp = 0, .maxdisp = 300,
                    },
#endif
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", ""),

                SUBELEM_START_CRN("stab", "Режим стабилизации тока, mA", NULL, "I катода верх.\vI катода нижн.\v?\v?", 8, 2)
                    ECTL_STAB_LINE("i_cat_top", "I катода верх.", "%6.2f",           WELD02_BASE + WELD02_CHAN_SET_IH,   WELD02_BASE + WELD02_CHAN_MES_IH, 100),
                    ECTL_STAB_LINE("i_cat_bot", "I катода нижн.", "%6.2f",           WELD02_BASE + WELD02_CHAN_SET_IL,   WELD02_BASE + WELD02_CHAN_MES_IL, 100),
                    ECTL_SEL_LINE ("gain", "Усиление",     "НЕТ\vx10",               WELD02_BASE + WELD02_CHAN_SET_GAIN, WELD02_BASE + 0),
                    ECTL_SEL_LINE ("stab", "Стабилизация", "НЕТ\v1kHz\v300Hz\v30Hz", WELD02_BASE + WELD02_CHAN_SET_STAB, WELD02_BASE + 0),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", "horz=fill"),

                SUBELEM_START("mes", "Измерения контроллера пушки, V", 3, 1)
                    ECTL_MES_LINE("u_heat", "U накала",  WELD02_BASE + WELD02_CHAN_MES_U_HEAT, 2),
                    ECTL_MES_LINE("u_high", "U высокое", WELD02_BASE + WELD02_CHAN_MES_U_HIGH, 0),
                    // When Uhigh becomes less than 3500, set Ihigh:=0
                    ECTL_MES_LINE("u_feed", "U питания", WELD02_BASE + WELD02_CHAN_MES_U_POWR, 0),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", "horz=fill"),
            },
            "nocoltitles,norowtitles"
        }
    },

#define MAGSYS_LINE(id, label, cn_s, cn_m, min_alwd, max_alwd) \
    {id "_s", label, NULL, "% 7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_s), .minnorm=min_alwd, .maxnorm=max_alwd, .incdec_step=0.01}, \
    {id "_m", NULL,  NULL, "% 7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_m)}

    {
        &(elemnet_t){
            "magsys", "Магнитная система",
            NULL, NULL,
            ELEM_MULTICOL, 2-1, 1,
            (logchannet_t []){
                SUBELEM_START("is", "Магнитная оптика, A", 16+1, 2)
                    MAGSYS_LINE("i_acx",  "I анодн. корр. X", C_I_ACX_S,  C_I_ACX_M,  -1.5, 1.5),
                    MAGSYS_LINE("i_acy",  "I анодн. корр. Y", C_I_ACY_S,  C_I_ACY_M,  -1.5, 1.5),
                    MAGSYS_LINE("i_ld",   "I линзы динам.",   C_I_LD_S,   C_I_LD_M,    0.0, 1.5),
                    MAGSYS_LINE("i_ls",   "I линзы статич.",  C_I_LS_S,   C_I_LS_M,    0.0, 1.5),
                    MAGSYS_LINE("i_c2x",  "I корректора 2 X", C_I_C2X_S,  C_I_C2X_M,  -1.5, 1.5),
                    MAGSYS_LINE("i_c2y",  "I корректора 2 Y", C_I_C2Y_S,  C_I_C2Y_M,  -1.5, 1.5),
                    MAGSYS_LINE("i_dc1x", "I корректора 1 X", C_I_DC1X_S, C_I_DC1X_M, -1.5, 1.5),
                    MAGSYS_LINE("i_dc1y", "I корректора 1 Y", C_I_DC1Y_S, C_I_DC1Y_M, -1.5, 1.5),
                    {"i_ld_r", "I линзы дин.скор.", NULL, "% 7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_I_LD_R)},
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", "horz=left"),

                SUBELEM_START("catch", "Ионная ловушка, kV", 1, 1)
                    {"catch", "U источника питания", NULL, "%7.4f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_U_FEED_M), .placement="horz=right"},
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", "horz=fill"),
            },
            "nocoltitles,norowtitles"
        }, 0
    },

#define DDS300_LINE(id, label, units, dpyfmt, cn) \
    {id, label, units, dpyfmt, "nounits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .placement=""}

#define BFPNUM_LINE(id, label, cn) \
    {id, label, NULL, "%5.1f", "nounits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .maxnorm=255*2.5}

    {
        &(elemnet_t){
            "sync", "Синхронизация",
            "", "",
            ELEM_MULTICOL, 3, 3,
            (logchannet_t []){
                SUBELEM_START_CRN("dds300", "DDS300",
                                  "1\v2", "Ампл., mV\vФаза, ??\vЧаст., Hz",
                                  6, 2)
                    DDS300_LINE("ampl1", "Ампл. 1", "mV", "%5.1f", DDS300_BASE + DDS300_CHAN_AMP1),
                    DDS300_LINE("ampl2", "Ампл. 2", "mV", "%5.1f", DDS300_BASE + DDS300_CHAN_AMP2),
                    DDS300_LINE("phas1", "Фаза  1", "??", "%5.0f", DDS300_BASE + DDS300_CHAN_PHS1),
                    DDS300_LINE("phas2", "Фаза  2", "??", "%5.0f", DDS300_BASE + DDS300_CHAN_PHS2),
                    DDS300_LINE("freq1", "Част. 1", "Hz", "%5.0f", DDS300_BASE + DDS300_CHAN_FRQ1),
                    DDS300_LINE("freq2", "Част. 2", "Hz", "%5.0f", DDS300_BASE + DDS300_CHAN_FRQ2),
                SUBELEM_END("nofold,noshadow", ""),
                {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP, LOGK_NOP, .placement="vert=fill"},
                SUBELEM_START("bfp", "Модуляция", 2, 1)
                    SUBELEM_START("leds", "STATUS", 2, 1)
                        RGLED_LINE ("ubs",  "Блокировка УБС", BFP_BASE + FROLOV_BFP_CHAN_STAT_UBS),
                        RGLED_LINE ("alwd", "Тумблер РАБОТА", BFP_BASE + FROLOV_BFP_CHAN_STAT_WKALWD),
                    SUBELEM_END("notitle,nohline,nofold,noshadow,nocoltitles,norowtitles", ""),
                    SUBELEM_START("ctl", "CONTROL", 4, 1)
                        RGSWCH_LINE("work", "Работа",    "Выкл", "Вкл",
                                    BFP_BASE + FROLOV_BFP_CHAN_STAT_WRK,
                                    BFP_BASE + FROLOV_BFP_CHAN_WRK_DISABLE,
                                    BFP_BASE + FROLOV_BFP_CHAN_WRK_ENABLE),
                        RGSWCH_LINE("modl", "Модуляция", "Выкл", "Вкл",
                                    BFP_BASE + FROLOV_BFP_CHAN_STAT_MOD,
                                    BFP_BASE + FROLOV_BFP_CHAN_MOD_DISABLE,
                                    BFP_BASE + FROLOV_BFP_CHAN_MOD_ENABLE),
                        BFPNUM_LINE("max",  "Tmax, ms", BFP_BASE + FROLOV_BFP_CHAN_KMAX),
                        BFPNUM_LINE("min",  "Tmin, ms", BFP_BASE + FROLOV_BFP_CHAN_KMIN),
                    SUBELEM_END("notitle,nohline,nofold,noshadow,nocoltitles", ""),
                SUBELEM_END("nofold,noshadow,nocoltitles,norowtitles", ""),
            },
            "notitle,nocoltitles,norowtitles"
        }, 1
    },

#define ALARM_LAMP(id, label, cn) \
    {id, label, NULL, NULL, "shape=circle", "", LOGT_READ, LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}
    {
        &(elemnet_t){
            "alarms", "Алармы и блокировки",
            "", "",
            ELEM_MULTICOL, 10, 2,
            (logchannet_t []){
                ALARM_LAMP("", "Запрет ВИПу",       XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 6),
                ALARM_LAMP("", "Запрет эл.пучка",   XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 7),
                ALARM_LAMP("", "Токооседание",      XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 3),
                {"", "", "mA", "%6.3f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 1)},
                ALARM_LAMP("", "Вак.+в-метр",       XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 1),
                {"vacuum", "P", "Pa", "%5.0e", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 0),
                    (excmd_t []){
                        CMD_GETP_I(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 0),
                        CMD_MUL_I(0.8968),
                        CMD_EXP,
                        CMD_MUL_I(5.0/100000.0),
                        CMD_RET
                    },
                .mindisp=1e-10, .maxdisp=1e0},
                ALARM_LAMP("", "T трубы",           XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 4),
                ALARM_LAMP("", "T жидкости",        XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 5),
                ALARM_LAMP("", "Ур. жидкости",  XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 0),
                ALARM_LAMP("", "Расх. жидкости",    XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 2),
            },
            "nocoltitles,norowtitles"
        }, 0
    },

#define COLORLED_LINE(name, l, comnt, dtype, cn) \
    {name, "\n"l, NULL, NULL, NULL, comnt, LOGT_READ,   dtype, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}
#define BUT_LINE(name, title, n) \
    {name, title, "", NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(n)}

#define MOVEN485_START(id, label, units, base) \
    /*SUBELEM_START(id, label, 8, 8)*/ \
        {id "numsteps", label,  units, "%6.2f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NKSHD485_CHAN_NUM_STEPS)}, \
        BUT_LINE     (id "go",   "GO",   base + NKSHD485_CHAN_GO),   \
        BUT_LINE     (id "stop", "STOP", base + NKSHD485_CHAN_STOP), \
        COLORLED_LINE(id "k2",    "+", "K+",    LOGD_REDLED,   base + NKSHD485_CHAN_STATUS_KP), \
        COLORLED_LINE(id "k1",    "-", "K-",    LOGD_REDLED,   base + NKSHD485_CHAN_STATUS_KM), \
        COLORLED_LINE(id "going", "g", "Going", LOGD_GREENLED, base + NKSHD485_CHAN_STATUS_GOING), \
        COLORLED_LINE(id "ready", "R", "Ready", LOGD_BLUELED,  base + NKSHD485_CHAN_STATUS_READY), /*<- note: this comma IS necessary*/

#define MOVEN485_END() \
    /*SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL)*/

#define MOVESMC8_START(id, label, units, base) \
    /*SUBELEM_START(id, label, 8, 8)*/ \
        {id "numsteps", label,  units, "%6.2f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + SMC8_CHAN_NUM_STEPS_base)}, \
        BUT_LINE     (id "go",   "GO",   base + SMC8_CHAN_GO_base),   \
        BUT_LINE     (id "stop", "STOP", base + SMC8_CHAN_STOP_base), \
        COLORLED_LINE(id "k2",    "+", "K+",    LOGD_REDLED,   base + SMC8_CHAN_KP_base), \
        COLORLED_LINE(id "k1",    "-", "K-",    LOGD_REDLED,   base + SMC8_CHAN_KM_base), \
        COLORLED_LINE(id "going", "g", "Going", LOGD_GREENLED, base + SMC8_CHAN_GOING_base), \
        EMPTY_CELL(), /*<- note: this comma IS necessary*/

#define MOVESMC8_END() \
    /*SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL)*/

#if 1
    GLOBELEM_START("weld", "Электронно-лучевая сварка", 1 + 2, 1 + (2<<24))
        {NULL, "DO!",   NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_RET_I(0),
            },
            (excmd_t[]){
                CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                CMD_PUSH_I(1), CMD_SETP_I(SMC8_A_BASE     + SMC8_CHAN_GO_base),
                CMD_PUSH_I(1), CMD_SETP_I(SMC8_B_BASE     + SMC8_CHAN_GO_base),
                CMD_PUSH_I(1), CMD_SETP_I(NKSHD485_R_BASE + NKSHD485_CHAN_GO),
                CMD_RET,
            }
        },
        {NULL, "STOP!", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_RET_I(0),
            },
            (excmd_t[]){
                CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                CMD_PUSH_I(1), CMD_SETP_I(SMC8_A_BASE     + SMC8_CHAN_STOP_base),
                CMD_PUSH_I(1), CMD_SETP_I(SMC8_B_BASE     + SMC8_CHAN_STOP_base),
                CMD_PUSH_I(1), CMD_SETP_I(NKSHD485_R_BASE + NKSHD485_CHAN_STOP),
                CMD_RET,
            }
        },

        SUBELEM_START("movements", "Подвижки", 3*8, 8)

#define STEPPER_CONV(defcoeff, units, rn, cn)                         \
    {NULL, NULL, units, NULL, "withunits", NULL,                      \
     LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),   \
        (excmd_t[]){                                                  \
            CMD_PUSH_I(defcoeff), CMD_SETLCLREGDEFVAL_I(rn),          \
            CMD_GETP_I(cn), CMD_GETLCLREG_I(rn), CMD_DIV,             \
            CMD_RET                                                   \
        },                                                            \
        (excmd_t[]){                                                  \
            CMD_QRYVAL, CMD_GETLCLREG_I(rn), CMD_MUL, CMD_SETP_I(cn), \
            CMD_RET                                                   \
        }                                                             \
    },                                                                \
    {NULL, "*",  "=", NULL, "withlabel,withunits", NULL,              \
     LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),   \
        (excmd_t[]){                                                  \
            CMD_GETLCLREG_I(rn), CMD_RET                              \
        },                                                            \
        (excmd_t[]){                                                  \
            CMD_QRYVAL, CMD_SETLCLREG_I(rn),                          \
            CMD_RET                                                   \
        }                                                             \
    }

#define SMC8_BASE  SMC8_A_BASE
#define SMC8_NAME  "A"
#define SMC8_IDENT "a"
#define SMC8_COEFF COEFF_A
#define SMC8_PRC   "3"
///            STEPPER_CONV(5.0, "?1", REG_COEFF_STEPPER_A, SMC8_BASE + SMC8_CHAN_NUM_STEPS_base + 0),
            MOVESMC8_START(SMC8_IDENT, SMC8_NAME, "mm", SMC8_BASE)
            #include "elem_subwin_smc8.h"
            MOVESMC8_END(),

#define SMC8_BASE  SMC8_B_BASE
#define SMC8_NAME  "B"
#define SMC8_IDENT "b"
#define SMC8_COEFF COEFF_B
#define SMC8_PRC   "3"
///            STEPPER_CONV(5.0, "?2", REG_COEFF_STEPPER_A, SMC8_BASE + SMC8_CHAN_NUM_STEPS_base + 0),
            MOVESMC8_START(SMC8_IDENT, SMC8_NAME, "\xb0", SMC8_BASE)
            #include "elem_subwin_smc8.h"
            MOVESMC8_END(),

#define NKSHD485_BASE  NKSHD485_R_BASE
#define NKSHD485_NAME  "R"
#define NKSHD485_IDENT "r"
#define NKSHD485_COEFF COEFF_R
#define NKSHD485_PRC   "3"
///            STEPPER_CONV(5.0, "mm/s", REG_COEFF_STEPPER_A, NKSHD485_BASE + NKSHD485_CHAN_NUM_STEPS),
            MOVEN485_START(NKSHD485_IDENT, NKSHD485_NAME, "mm", NKSHD485_BASE)
            #include "elem_subwin_nkshd485.h"
            MOVEN485_END(),

#define NKSHD485_BASE  NKSHD485_X_BASE
#define NKSHD485_NAME  "X"
#define NKSHD485_IDENT "x"
            MOVEN485_START(NKSHD485_IDENT, NKSHD485_NAME, NULL, NKSHD485_BASE)
            #include "elem_subwin_nkshd485.h"
            MOVEN485_END(),

#define NKSHD485_BASE  NKSHD485_Y_BASE
#define NKSHD485_NAME  "Y"
#define NKSHD485_IDENT "y"
            MOVEN485_START(NKSHD485_IDENT, NKSHD485_NAME, NULL, NKSHD485_BASE)
            #include "elem_subwin_nkshd485.h"
            MOVEN485_END(),

        SUBELEM_END("noshadow,titleatleft,nocoltitles", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 0),
#endif

    GLOBELEM_START("weld", "Электронно-лучевая сварка", 1, 1)
      {NULL, NULL, NULL, NULL,
       "",
       NULL, LOGT_READ, LOGD_WELD_PROCESS, LOGK_DIRECT,
       17 /*!!!*/, PHYS_ID(XCAC208_2_BASE)
      }
    GLOBELEM_END("nocoltitles,norowtitles", 0),


#if 0
#define ELS_LINE(id, label, cn) \
    {id, label, NULL, "% 7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="horz=right"}

    {
        &(elemnet_t){
            "weld", "Электронно-лучевая сварка",
            "", "",
            ELEM_MULTICOL, 2, 1,
            (logchannet_t []){
                SUBELEM_START("i_gun", "Ток пушки", 4, 1)
                    ELS_LINE("t_asc",  "T подъёма",      0),
                    ELS_LINE("t_flat", "T постоянн.",    0),
                    ELS_LINE("t_desc", "T спуска",       0),
                    ELS_LINE("u_mesh", "U сетки запир.", 0),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", "horz=fill"),

                SUBELEM_START("scan", "Развертка пучка", 4, 1)
                    ELS_LINE("ic1xmax", "I корр. 1 X max", 0),
                    ELS_LINE("ic1xmin", "I корр. 1 X min", 0),
                    ELS_LINE("ic1ymax", "I корр. 1 Y max", 0),
                    ELS_LINE("ic1ymin", "I корр. 1 Y min", 0),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", "horz=fill"),
            },
            "nocoltitles,norowtitles"
        }, 1
    },
#endif


#if 0
#define REG_R_W(rn, defval)                            \
    (excmd_t []){                                      \
        CMD_PUSH_I(defval), CMD_SETLCLREGDEFVAL_I(rn), \
        CMD_GETLCLREG_I(rn), CMD_RET,                  \
    },                                                 \
    (excmd_t []){                                      \
        CMD_QRYVAL, CMD_SETLCLREG_I(rn),               \
        CMD_RET,                                       \
    }

#define TEXT_REG(rn, defval) \
    {NULL, NULL, NULL, "% 7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), REG_R_W(rn, defval)}


    {
        &(elemnet_t){
            "weld", "Техпроцесс",
            "", "",
            ELEM_MULTICOL, 3*1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN(NULL, "Параметры", "X, A\vY, A", "От\vДо\v\v\vНа", 5*2, 2)
                    TEXT_REG(REG_WELD_FR_X, 0),
                    TEXT_REG(REG_WELD_FR_Y, 0),
                    TEXT_REG(REG_WELD_TO_X, 0),
                    TEXT_REG(REG_WELD_TO_Y, 0),
                    HLABEL_CELL("Повторить"),
                    {NULL, NULL, "раз(а)", "%2.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), REG_R_W(REG_WELD_RPT, 1)},
                    HLABEL_CELL("Со сдвигом"),
                    EMPTY_CELL(),
                    TEXT_REG(REG_WELD_BY_X, 0),
                    TEXT_REG(REG_WELD_BY_Y, 0),
                SUBELEM_END("nohline,noshadow", NULL),

                SEP_L(),

                SUBELEM_START(NULL, "Процесс", 2, 2)
                    SUBWINV4_START(NULL, NULL, 0)
                    SUBWIN_END("...", "", NULL),

                    SUBELEM_START(NULL, NULL, 3, 3)
                        {NULL, "Пуск!",  NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), REG_R_W(REG_WELD_START, 0)},
                        {NULL, "Шаг #", NULL, "%-3.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), REG_R_W(REG_WELD_STEP_N, 0)},
                        {NULL, "Стоп",   NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), REG_R_W(REG_WELD_START, 2)},
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
                    {NULL, NULL, NULL, NULL,
                        "",
                        NULL,
                        LOGT_READ, LOGD_WELD_PROCESS, LOGK_DIRECT,
                        /*!!! A hack: we use 'color' field to pass integer */
                        XCAC208_2_TABLE_ALL_BIGC,
                        PHYS_ID(XCAC208_2_BASE)
                    },
                SUBELEM_END("nohline,noshadow", NULL),

                SUBELEM_START("", "", 2, 2)
                    {"start", "Start", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t []){
                            CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(REG_WELD_START),
                            CMD_GETLCLREG_I(REG_WELD_START), CMD_RET
                        },
                        (excmd_t []){
                            CMD_QRYVAL, CMD_SETLCLREG_I(REG_WELD_START), CMD_RET
                        },
                    },
                    
                    {"", "", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                        (void *)&(elemnet_t){
                            NULL, NULL,
                            NULL, NULL,
                            ELEM_WELD_DUMMY, 0, 0,
                            NULL,
                        }
                    }
                SUBELEM_END("notitle,nohline,nofold,noshadow,nocoltitles,norowtitles", NULL),
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
                {"histplot", "HistPlot", NULL, NULL, "width=700,height=290,plot1=.vacuum,plot2=.vip_uout,plot3=.i_heat_m,plot4=.i_cat_top_m,plot5=.i_cat_bot_m,plot6=.i_electr_m,plot7=.u_heat", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
            },
            "notitle,nocoltitles,norowtitles"
        }, 1
    },


    {NULL}
};

static physprops_t weldcc_physinfo[] =
{
    {VIP_BASE + SENKOV_VIP_CHAN_SET_DAC_UOUT,                10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_SET_VIP_IOUT_PROT,           10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_SET_BRKDN_COUNT,              1, -1, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_UOUT,               100,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_FULL,           10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AUTO,           10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AVG,            10,  0, 0},

    {WELD02_BASE + WELD02_CHAN_SET_UH,   4095.0/6000, 0, 0},
    {WELD02_BASE + WELD02_CHAN_SET_UL,   4095.0/6000, 0, 0},
    {WELD02_BASE + WELD02_CHAN_SET_UN,   4095.0/125,  0, 0},
    {WELD02_BASE + WELD02_CHAN_SET_UP,   4095.0/250/*!!!???*/,  0, 0},
    {WELD02_BASE + WELD02_CHAN_SET_IH,   10,  0, 0},
    {WELD02_BASE + WELD02_CHAN_SET_IL,   10,  0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_UH,   4095.0/6000, 0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_UL,   4095.0/6000, 0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_UN,   4095.0/125,  0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_UP,   4095.0/2.5,  0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_IH,   (40950.0/2500)*4,  0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_IL,   (40950.0/2500)*4,  0, 0},

    {WELD02_BASE + WELD02_CHAN_MES_U_HEAT, 4095.0/2.5, 0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_U_HIGH, 4095.0/6000, 0, 0},
    {WELD02_BASE + WELD02_CHAN_MES_U_POWR, 4095.0/50.0, 0, 0},

    {XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 0, 1000000,    0, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 1, 1000000.0/2, 0, 0},

    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 0, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 1, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 2, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 3, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 4, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 5, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 6, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 7, -1, -1, 0},
    
    {C_I_ACX_S,  1000000*(10/1.5), 0, 305},  // 0
    {C_I_ACY_S,  1000000*(10/1.5), 0, 305},  // 1
    {C_I_LD_S,   1000000*(10/1.5), 0, 305},  // 2
    {C_I_LS_S,   1000000*(10/1.5), 0, 305},  // 3
    {C_I_DC1X_S, 1000000*(10/1.5), 0, 305},  // 4
    {C_I_DC1Y_S, 1000000*(10/1.5), 0, 305},  // 5
    {C_I_C2X_S,  1000000*(10/1.5), 0, 305},  // 6
    {C_I_C2Y_S,  1000000*(10/1.5), 0, 305},  // 7

    {C_I_LD_M,   1000000*(2.0/1.0), 0, 0},  //  0
    {C_I_LS_M,   1000000*(2.0/1.0), 0, 0},  //  1
    {C_I_DC1X_M, 1000000*(2.0/1.0), 0, 0},  //  4
    {C_I_DC1Y_M, 1000000*(2.0/1.0), 0, 0},  //  5
    {C_I_C2X_M,  1000000*(2.0/1.0), 0, 0},  //  8
    {C_I_C2Y_M,  1000000*(2.0/1.0), 0, 0},  //  9
    {C_I_ACX_M,  1000000*(2.0/1.0), 0, 0},  // 12
    {C_I_ACY_M,  1000000*(2.0/1.0), 0, 0},  // 13
    {C_U_FEED_M, 1000000,           0, 0},  // 16

    {C_I_LD_R,   1000000*(10/1.5), 0, 305},
                                
    {BFP_BASE + FROLOV_BFP_CHAN_KMAX, 1/2.5, 0, -1},
    {BFP_BASE + FROLOV_BFP_CHAN_KMIN, 1/2.5, 0, -1},
    
    {DDS300_BASE + DDS300_CHAN_AMP1, (0xFFF/600.0), 0, 0},
    {DDS300_BASE + DDS300_CHAN_AMP2, (0xFFF/600.0), 0, 0},
    {DDS300_BASE + DDS300_CHAN_PHS1, 1,             0, 0},
    {DDS300_BASE + DDS300_CHAN_PHS2, 1,             0, 0},
    {DDS300_BASE + DDS300_CHAN_FRQ1, 1,             0, 0},
    {DDS300_BASE + DDS300_CHAN_FRQ2, 1,             0, 0},

    {SMC8_A_BASE + SMC8_CHAN_NUM_STEPS_base    + 0, COEFF_A, 0, 0},
    {SMC8_A_BASE + SMC8_CHAN_START_FREQ_base   + 0, COEFF_A, 0, 0},
    {SMC8_A_BASE + SMC8_CHAN_FINAL_FREQ_base   + 0, COEFF_A, 0, 0},
    {SMC8_A_BASE + SMC8_CHAN_ACCELERATION_base + 0, COEFF_A, 0, 0},
    {SMC8_A_BASE + SMC8_CHAN_COUNTER_base      + 0, COEFF_A, 0, 0},

    {SMC8_B_BASE + SMC8_CHAN_NUM_STEPS_base    + 0, COEFF_B, 0, 0},
    {SMC8_B_BASE + SMC8_CHAN_START_FREQ_base   + 0, COEFF_B, 0, 0},
    {SMC8_B_BASE + SMC8_CHAN_FINAL_FREQ_base   + 0, COEFF_B, 0, 0},
    {SMC8_B_BASE + SMC8_CHAN_ACCELERATION_base + 0, COEFF_B, 0, 0},
    {SMC8_B_BASE + SMC8_CHAN_COUNTER_base      + 0, COEFF_B, 0, 0},

    {NKSHD485_R_BASE + NKSHD485_CHAN_NUM_STEPS,    COEFF_R, 0, 0},
    {NKSHD485_R_BASE + NKSHD485_CHAN_MIN_VELOCITY, COEFF_R, 0, 0},
    {NKSHD485_R_BASE + NKSHD485_CHAN_MAX_VELOCITY, COEFF_R, 0, 0},
    {NKSHD485_R_BASE + NKSHD485_CHAN_ACCELERATION, COEFF_R, 0, 0},
};
