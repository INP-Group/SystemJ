#include "common_elem_macros.h"

#include "drv_i/xcac208_drv_i.h"
#include "drv_i/xceac124_drv_i.h"
#include "drv_i/senkov_vip_drv_i.h"
#include "drv_i/weld02_drv_i.h"
#include "drv_i/dds300_drv_i.h"
#include "drv_i/frolov_bfp_drv_i.h"
#include "drv_i/nkshd485_drv_i.h"

#include "weldclient.h"


enum
{
    VIP_BASE        = 0,
    WELD02_BASE     = 50,
    XCAC208_1_BASE  = 68,
    XCEAC124_1_BASE = 166,
    XCEAC124_2_BASE = 244,
    BFP_BASE        = 322,
    DDS300_BASE     = 332,
};

enum
{
    C_I_ACX_S = XCEAC124_2_BASE + XCEAC124_CHAN_OUT_n_BASE + 2,
    C_I_ACX_M = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 2*2+0,
    C_I_ACX_V = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 2*2+1,

    C_I_ACY_S = XCEAC124_2_BASE + XCEAC124_CHAN_OUT_n_BASE + 3,
    C_I_ACY_M = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 3*2+0,
    C_I_ACY_V = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 3*2+1,

    C_I_FOC_S = XCEAC124_1_BASE + XCEAC124_CHAN_OUT_n_BASE + 1,
    C_I_FOC_M = XCEAC124_1_BASE + XCEAC124_CHAN_ADC_n_BASE + 1*2+0,
    C_I_FOC_V = XCEAC124_1_BASE + XCEAC124_CHAN_ADC_n_BASE + 1*2+1,

    C_I_C1X_S = XCEAC124_1_BASE + XCEAC124_CHAN_OUT_n_BASE + 2,
    C_I_C1X_M = XCEAC124_1_BASE + XCEAC124_CHAN_ADC_n_BASE + 2*2+0,
    C_I_C1X_V = XCEAC124_1_BASE + XCEAC124_CHAN_ADC_n_BASE + 2*2+1,

    C_I_C1Y_S = XCEAC124_1_BASE + XCEAC124_CHAN_OUT_n_BASE + 3,
    C_I_C1Y_M = XCEAC124_1_BASE + XCEAC124_CHAN_ADC_n_BASE + 3*2+0,
    C_I_C1Y_V = XCEAC124_1_BASE + XCEAC124_CHAN_ADC_n_BASE + 3*2+1,

    C_I_C2X_S = XCEAC124_2_BASE + XCEAC124_CHAN_OUT_n_BASE + 0,
    C_I_C2X_M = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 0*2+0,
    C_I_C2X_V = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 0*2+1,

    C_I_C2Y_S = XCEAC124_2_BASE + XCEAC124_CHAN_OUT_n_BASE + 1,
    C_I_C2Y_M = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 1*2+0,
    C_I_C2Y_V = XCEAC124_2_BASE + XCEAC124_CHAN_ADC_n_BASE + 1*2+1,
};
    

static groupunit_t verona_grouping[] =
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

#define MAGSYS_LINE(id, label, cn_s, cn_m, cn_v, min_alwd, max_alwd) \
    {id "_s", label, NULL, "% 7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_s), .minnorm=min_alwd, .maxnorm=max_alwd, .incdec_step=0.01}, \
    {id "_m", NULL,  NULL, "% 7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn_m)}, \
    {id "_v", NULL,  NULL, "% 7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_DIM,    PHYS_ID(cn_v)}

    {
        &(elemnet_t){
            "magsys", "Магнитная система",
            NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("is", "Магнитная оптика, A", "Iуст, A\vIизм, A\vUизм, V", NULL, 21, 3)
                    MAGSYS_LINE("i_acx",  "I анодн. корр. X", C_I_ACX_S, C_I_ACX_M, C_I_ACX_V, -1.5, 1.5),
                    MAGSYS_LINE("i_acy",  "I анодн. корр. Y", C_I_ACY_S, C_I_ACY_M, C_I_ACY_V, -1.5, 1.5),
                    MAGSYS_LINE("i_foc",  "I фок. катушки",   C_I_FOC_S, C_I_FOC_M, C_I_FOC_V,  0.0, 1.5),
                    MAGSYS_LINE("i_c2x",  "I корректора 2 X", C_I_C2X_S, C_I_C2X_M, C_I_C2X_V, -1.5, 1.5),
                    MAGSYS_LINE("i_c2y",  "I корректора 2 Y", C_I_C2Y_S, C_I_C2Y_M, C_I_C2Y_V, -1.5, 1.5),
                    MAGSYS_LINE("i_dc1x", "I корректора 1 X", C_I_C1X_S, C_I_C1X_M, C_I_C1X_V, -1.5, 1.5),
                    MAGSYS_LINE("i_dc1y", "I корректора 1 Y", C_I_C1Y_S, C_I_C1Y_M, C_I_C1Y_V, -1.5, 1.5),
                SUBELEM_END("notitle,nohline,nofold,noshadow", "horz=left"),
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

#define VAC_MES_FLA()                                             \
    (excmd_t []){                                                 \
        CMD_GETP_I(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 0), \
        CMD_MUL_I(0.8968),                                        \
        CMD_EXP,                                                  \
        CMD_MUL_I(5.0/100000.0),                                  \
        CMD_RET                                                   \
    }
// vacuum(volts)=10^(1.667*volts-d), where d=11.33mbar,9.33Pa,11.46torr
#define VAC_MES_FLA()                                             \
    (excmd_t []){                                                 \
        CMD_PUSH_I(10),                                           \
        CMD_GETP_I(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 0), \
        CMD_MUL_I(1.667), CMD_SUB_I(9.33), CMD_PWR,               \
        CMD_RET                                                   \
    }
#define ALARM_LAMP(id, label, cn) \
    {id, label, NULL, NULL, "shape=circle", "", LOGT_READ, LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}
    {
        &(elemnet_t){
            "alarms", "Алармы и блокировки",
            "", "",
            ELEM_MULTICOL, 12, 2,
            (logchannet_t []){
                ALARM_LAMP("", "Запрет ВИПу",       XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 6),
                ALARM_LAMP("", "Вакууметр",         XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 7),
                ALARM_LAMP("", "Токооседание",      XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 3),
                {"", "", "mA", "%6.3f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 1)},
                ALARM_LAMP("", "Вакуум",            XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 1),
                {"vacuum", "P", "Pa", "%5.0e", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    VAC_MES_FLA(), .mindisp=1e-10, .maxdisp=1e0},
                EMPTY_CELL(),
                {"linear", "=", "Pa", "%5.3f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    VAC_MES_FLA(), .mindisp=1e-10, .maxdisp=1e0},
                {"-tmnctl", "ТМН", NULL,  NULL, "#T0\v1", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_1_BASE + XCAC208_CHAN_REGS_WR8_BASE + 1)},
                {"tmnrps",  NULL,  "rps", "%4.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 5)},
                {"Reset", "Сброс",   NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t[]){
                        /* Is R_RESET_TIME!=0? */
                        CMD_PUSH_I(0.0), CMD_SETLCLREGDEFVAL_I(R_RESET_TIME),
                        CMD_PUSH_I(1.0), CMD_PUSH_I(1.0), CMD_PUSH_I(0.0), CMD_GETLCLREG_I(R_RESET_TIME), CMD_CASE,
                        CMD_TEST, CMD_BREAK_I(0.0),
                        
                        CMD_PUSH_I(0.0), CMD_PUSH_I(0.0), CMD_PUSH_I(1.0),
                        CMD_GETLCLREG_I(R_RESET_TIME), CMD_ADD_I(1.0), CMD_GETTIME, CMD_SUB, //
                        CMD_CASE,
                        CMD_TEST, CMD_BREAK_I(CX_VALUE_LIT_MASK),
                        
                        CMD_PUSH_I(0), CMD_ALLOW_W, CMD_PRGLY, CMD_SETP_I(XCAC208_1_BASE + XCAC208_CHAN_REGS_WR8_BASE + 0),
                        CMD_PUSH_I(0), CMD_SETLCLREG_I(R_RESET_TIME),
                        CMD_RET_I(0.0)
                    },
                    (excmd_t[]){
                        CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                        CMD_PUSH_I(1), CMD_PRGLY, CMD_SETP_I(XCAC208_1_BASE + XCAC208_CHAN_REGS_WR8_BASE + 0),
                        
                        CMD_GETTIME, CMD_SETLCLREG_I(R_RESET_TIME), CMD_REFRESH_I(1.0),
                        CMD_RET
                    },
                    //.placement="horz=right"
                },

                {"sec-emis", "Втор.эмис.", "uA", "%4.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 3)}
            },
            "nocoltitles,norowtitles"
        }, 0
    },

    {
        &(elemnet_t){
            "histplot", "Histplot",
            "", "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"histplot", "HistPlot", NULL, NULL, "width=700,height=290,add=.vacuum,add=.linear,add=.vip_uout,add=.i_heat_m,add=.i_cat_top_m,add=.i_cat_bot_m,add=.i_electr_m,add=.u_heat", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
                //{"histplot", "HistPlot", NULL, NULL, "width=700,height=290,plot1=.vacuum,plot2=.vip_uout,plot3=.i_heat_m,plot4=.i_cat_top_m,plot5=.i_cat_bot_m,plot6=.i_electr_m,plot7=.u_heat,plot8=.linear", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
            },
            "notitle,nocoltitles,norowtitles"
        }, 1
    },


    {NULL}
};

static physprops_t verona_physinfo[] =
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

    {XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 0,  1000000,        0, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 1,  1000000.0/2,    0, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 3, 10000000.0/1000, 0, 0}, // 10V==1mA
    {XCAC208_1_BASE + XCAC208_CHAN_ADC_n_BASE + 5, 10000000.0/1500, 0, 0}, // 10V==1500rps

    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 0, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 1, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 2, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 3, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 4, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 5, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 6, -1, -1, 0},
    {XCAC208_1_BASE + XCAC208_CHAN_REGS_RD8_BASE + 7, -1, -1, 0},
    
    {C_I_ACX_S,  1000000*(10/1.5), 0, 305},
    {C_I_ACY_S,  1000000*(10/1.5), 0, 305},
    {C_I_FOC_S,  1000000*(10/1.5), 0, 305},
    {C_I_C1X_S,  1000000*(10/1.5), 0, 305},
    {C_I_C1Y_S,  1000000*(10/1.5), 0, 305},
    {C_I_C2X_S,  1000000*(10/1.5), 0, 305},
    {C_I_C2Y_S,  1000000*(10/1.5), 0, 305},

    {C_I_FOC_M,  1000000*(2.0/1.0), 0, 0},
    {C_I_C1X_M,  1000000*(2.0/1.0), 0, 0},
    {C_I_C1Y_M,  1000000*(2.0/1.0), 0, 0},
    {C_I_C2X_M,  1000000*(2.0/1.0), 0, 0},
    {C_I_C2Y_M,  1000000*(2.0/1.0), 0, 0},
    {C_I_ACX_M,  1000000*(2.0/1.0), 0, 0},
    {C_I_ACY_M,  1000000*(2.0/1.0), 0, 0},

    {C_I_FOC_V,  1000000*(2.7/0.86), 0, 0},
    {C_I_C1X_V,  1000000*(2.7/0.86), 0, 0},
    {C_I_C1Y_V,  1000000*(2.7/0.86), 0, 0},
    {C_I_C2X_V,  1000000*(2.7/0.86), 0, 0},
    {C_I_C2Y_V,  1000000*(2.7/0.86), 0, 0},
    {C_I_ACX_V,  1000000*(2.7/0.86), 0, 0},
    {C_I_ACY_V,  1000000*(2.7/0.86), 0, 0},

    {BFP_BASE + FROLOV_BFP_CHAN_KMAX, 1/2.5, 0, -1},
    {BFP_BASE + FROLOV_BFP_CHAN_KMIN, 1/2.5, 0, -1},
    
    {DDS300_BASE + DDS300_CHAN_AMP1, (0xFFF/600.0), 0, 0},
    {DDS300_BASE + DDS300_CHAN_AMP2, (0xFFF/600.0), 0, 0},
    {DDS300_BASE + DDS300_CHAN_PHS1, 1,             0, 0},
    {DDS300_BASE + DDS300_CHAN_PHS2, 1,             0, 0},
    {DDS300_BASE + DDS300_CHAN_FRQ1, 1,             0, 0},
    {DDS300_BASE + DDS300_CHAN_FRQ2, 1,             0, 0},
};
