#include "common_elem_macros.h"

#include "drv_i/senkov_vip_drv_i.h"

enum
{
    VIP_BASE      = 0,
};

static groupunit_t senkov_vip_grouping[] =
{

#define VIP_STAT_LINE(id, short_l, long_l, cn) \
    {id, short_l, NULL, NULL, "shape=circle", long_l, LOGT_READ, LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define VIP_SET_LINE(id, label, units, dpyfmt, cn, max_alwd) \
    {id, label, units, dpyfmt, "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .placement="", .minnorm=0, .maxnorm=max_alwd}

#define VIP_MES_LINE(id, label, units, dpyfmt, cn) \
    {id, label, units, dpyfmt, NULL,        NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .placement=""}

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
                SUBELEM_START("mes", "Измерения", 12, 1)
                    VIP_MES_LINE("vip_uout",      "Uвых ВИП",          "kV", "%5.2f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_UOUT),
                    VIP_MES_LINE("vip_iout_full", "Iвых ВИП (полн.)",  "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_FULL),
                    VIP_MES_LINE("vip_iout_prec", "Iвых ВИП (<100мА)", "mA", "%6.2f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_PREC),
                    VIP_MES_LINE("transf_iin",    "Iвх ВВ трансф.",    "A",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_TRANSF_IIN),
                    VIP_MES_LINE("vip_3furect",   "Uвыпр 3ф-сети",     "V",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_3FURECT),
                    VIP_MES_LINE("vip_3fi",       "Iпотр 3ф-сети",     "A",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_3FI),
                    VIP_MES_LINE("vip_t_trrad",   "T транзисторов",    "╟C", "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_T_TRRAD),
                    VIP_MES_LINE("tank_t",        "T внутри ВВ бака",  "╟C", "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_TANK_T),
                    VIP_MES_LINE("reserved9",     "резерв",            "",   "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_RESERVED9),
                    VIP_MES_LINE("vip_iout_auto", "Iвых ВИП (авто)",   "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AUTO),
                    VIP_MES_LINE("vip_iout_avg",  "Iвых ВИП (усред.)", "mA", "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AVG),
                    VIP_MES_LINE("brkdn_count",   "Число пробоев",     "",   "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_CUR_BRKDN_COUNT),
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
            "notitle,noshadow,nocoltitles,norowtitles"
        }
    },

    {NULL}
};

static physprops_t senkov_vip_physinfo[] =
{
    {VIP_BASE + SENKOV_VIP_CHAN_SET_DAC_UOUT,                10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_SET_VIP_IOUT_PROT,           10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_SET_BRKDN_COUNT,              1, -1, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_UOUT,               100,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_FULL,           10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AUTO,           10,  0, 0},
    {VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AVG,            10,  0, 0},
};
