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
            "vip", "�������������� �������� �������",
            "", "",
            ELEM_MULTICOL, 4, 4,
            (logchannet_t []){
                SUBELEM_START("ctl", "����������", 8, 1)
                    SUBELEM_START("setts", "�������", 2, 1)
                        VIP_SET_LINE("u_high", "�������", "kV", "%4.1f", VIP_BASE + SENKOV_VIP_CHAN_SET_DAC_UOUT, 60.0),
                        VIP_SET_LINE("k_corr", "K ����.", "",   "%4.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_DAC_CORR, 255),
                    SUBELEM_END("notitle,noshadow,nocoltitles", ""),
                    LED_LINE("pwr",  "3�-���� ������",    VIP_BASE + SENKOV_VIP_CHAN_MODE_POWER_ON),
                    LED_LINE("act",  "��������� ���.",    VIP_BASE + SENKOV_VIP_CHAN_MODE_ACTUATOR_ON),
                    SUBELEM_START("onoff", "���/����", 2, 1)
                        RGSWCH_LINE("vip", "���",      "����", "���",
                                    VIP_BASE + SENKOV_VIP_CHAN_MODE_VIP_ON,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_VIP_OFF,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_VIP_ON),
                        RGSWCH_LINE("hvl", "�������", "����", "���",
                                    VIP_BASE + SENKOV_VIP_CHAN_MODE_OPERATE,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_HVL_OFF,
                                    VIP_BASE + SENKOV_VIP_CHAN_CMD_HVL_ON),
                    SUBELEM_END("notitle,noshadow,nocoltitles", ""),
                    LED_LINE("manu", "������ ����������", VIP_BASE + SENKOV_VIP_CHAN_MODE_MANUAL),
                    LED_LINE("lock", "����������",        VIP_BASE + SENKOV_VIP_CHAN_MODE_VIP_BLOCKED),
                    EMPTY_CELL(),
                    SUBWIN_START("service", "��������� ������ ����������� ���", 5, 1)
                        VIP_SET_LINE("reserved3"    , "��������� �����",                  "",   "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_SET_RESERVED3,     255),
                        VIP_SET_LINE("uso_coeff"    , "�����. �������� ���",              "",   "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_USO_COEFF,     255),
                        VIP_SET_LINE("vip_invrt_frq", "k ������� ������ ��������� ���",   "",   "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_VIP_INVRT_FRQ, 1000),
                        VIP_SET_LINE("vip_iout_prot", "������ �� ��������� ���� ���",     "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_SET_VIP_IOUT_PROT, 1200.0),
                        VIP_SET_LINE("brkdn_count",   "����� �������/1� ��� ����. �� 1�", "",   "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_SET_BRKDN_COUNT,   21),
                    SUBWIN_END("���������...", "noshadow,notitle,nocoltitles", "horz=center"),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles,norowtitles", ""),
                {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP, LOGK_NOP, .placement="vert=fill"},
                SUBELEM_START("mes", "���������", 12, 1)
                    VIP_MES_LINE("vip_uout",      "U��� ���",          "kV", "%5.2f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_UOUT),
                    VIP_MES_LINE("vip_iout_full", "I��� ��� (����.)",  "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_FULL),
                    VIP_MES_LINE("vip_iout_prec", "I��� ��� (<100��)", "mA", "%6.2f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_PREC),
                    VIP_MES_LINE("transf_iin",    "I�� �� ������.",    "A",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_TRANSF_IIN),
                    VIP_MES_LINE("vip_3furect",   "U���� 3�-����",     "V",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_3FURECT),
                    VIP_MES_LINE("vip_3fi",       "I���� 3�-����",     "A",  "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_3FI),
                    VIP_MES_LINE("vip_t_trrad",   "T ������������",    "�C", "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_T_TRRAD),
                    VIP_MES_LINE("tank_t",        "T ������ �� ����",  "�C", "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_TANK_T),
                    VIP_MES_LINE("reserved9",     "������",            "",   "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_RESERVED9),
                    VIP_MES_LINE("vip_iout_auto", "I��� ��� (����)",   "mA", "%6.1f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AUTO),
                    VIP_MES_LINE("vip_iout_avg",  "I��� ��� (�����.)", "mA", "%6.0f", VIP_BASE + SENKOV_VIP_CHAN_MES_VIP_IOUT_AVG),
                    VIP_MES_LINE("brkdn_count",   "����� �������",     "",   "%3.0f", VIP_BASE + SENKOV_VIP_CHAN_CUR_BRKDN_COUNT),
                SUBELEM_END("nohline,nofold,noshadow,nocoltitles", ""),
                SUBELEM_START("stat", "?", 13, 1)
                    VIP_STAT_LINE("vip_uout_gt_70kv"  , "U���>70",   "", VIP_BASE + SENKOV_VIP_CHAN_STAT_VIP_UOUT_GT_70kV),
                    VIP_STAT_LINE("vip_iout_gt_1100ma", "I���>1100", "", VIP_BASE + SENKOV_VIP_CHAN_STAT_VIP_IOUT_GT_1100mA),
                    VIP_STAT_LINE("feedback_break"    , "����� ��",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_FEEDBACK_BREAK),
                    VIP_STAT_LINE("transf_iin_gt_300a", "I����>300", "", VIP_BASE + SENKOV_VIP_CHAN_STAT_TRANSF_IIN_GT_300A),
                    VIP_STAT_LINE("phase_break"       , "���.����",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_PHASE_BREAK),
                    VIP_STAT_LINE("vip_iin_gt_250a"   , "I��>250",   "", VIP_BASE + SENKOV_VIP_CHAN_STAT_VIP_IIN_GT_250A),
                    VIP_STAT_LINE("ttemp_gt_45c"      , "T�����>45", "", VIP_BASE + SENKOV_VIP_CHAN_STAT_TTEMP_GT_45C),
                    VIP_STAT_LINE("tank_t_gt_60c"     , "T����>60",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_TANK_T_GT_60C),
                    VIP_STAT_LINE("no_water"          , "��� ����",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_NO_WATER),
                    VIP_STAT_LINE("ext_block1_ubs"    , "�1 ���",    "", VIP_BASE + SENKOV_VIP_CHAN_STAT_EXT_BLOCK1_UBS),
                    VIP_STAT_LINE("ext_block2_ubs"    , "�2 ���",    "", VIP_BASE + SENKOV_VIP_CHAN_STAT_EXT_BLOCK2_UBS),
                    VIP_STAT_LINE("constant_brkdn"    , "��������",  "", VIP_BASE + SENKOV_VIP_CHAN_STAT_CONSTANT_BRKDN),
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
