#include "drv_i/xceac124_drv_i.h"

enum
{
    XCEACS_BASE = 896,
};

#define C_IMES(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n) * 2 + 0 + XCEAC124_CHAN_ADC_n_BASE)
#define C_UMES(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n) * 2 + 1 + XCEAC124_CHAN_ADC_n_BASE)
#define C_U5_7(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n)         + XCEAC124_CHAN_OUT_n_BASE)
#define C_ISON(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n)         + XCEAC124_CHAN_REGS_WR8_BASE)

#define LINE_ID(p, f, n) p "-" __CX_STRINGIZE(f) "-" __CX_STRINGIZE(n)

#define NEWVAC_LINE(f, n, l, p, t) \
    {LINE_ID("KV57", f, n), l,    NULL, NULL,    "#L\v#T5\v7", t"\n�����: "p, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(C_U5_7(f, n)),  \
        (excmd_t[]){                                     \
            CMD_PUSH_I(0), CMD_PUSH_I(0), CMD_PUSH_I(1), \
            CMD_GETP_I(C_U5_7(f, n)),                    \
            CMD_CASE,                                    \
            CMD_RET                                      \
        },                                               \
        (excmd_t[]){                                            \
            CMD_QRYVAL, CMD_MUL_I(7), CMD_SETP_I(C_U5_7(f, n)), \
            CMD_RET                                             \
        },                                                      \
    }, \
    {LINE_ID("IsOn", f, n), NULL, NULL, NULL,    NULL,     NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_ISON(f, n))}, \
    {LINE_ID("Imes", f, n), NULL, NULL, "%6.3f", NULL,     NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_IMES(f, n))}, \
    {LINE_ID("Umes", f, n), NULL, NULL, "%6.3f", NULL,     NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_UMES(f, n))}

static groupunit_t newvac_grouping[] =
{
    {
        &(elemnet_t){
            "new", "New",
            "5/7kV\v \vI, uA\vU, kV", NULL,
            ELEM_MULTICOL, 4*4*12, 4 + 32*65536,
            (logchannet_t []){
                NEWVAC_LINE( 0, 0, "�1",     "???",       "��ɣ���� �����"),
                NEWVAC_LINE( 0, 1, "�2",     "����",      "������-2"),
                NEWVAC_LINE( 0, 2, "�3",     "����",      "������-3"),
                NEWVAC_LINE( 0, 3, "�4",     "����",      "������-4"),

                NEWVAC_LINE( 1, 0, "�5",     "����",      "������-5"),
                NEWVAC_LINE( 1, 1, "�6",     "����",      "������-6"),
                NEWVAC_LINE( 1, 2, "�7",     "����",      "������-7"),
                NEWVAC_LINE( 1, 3, "�8",     "���-0,025", "������-8"),

                NEWVAC_LINE( 2, 0, "�9",     "����",      "������-9"),
                NEWVAC_LINE( 2, 1, "�10",    "����",      "������-10"),
                NEWVAC_LINE( 2, 2, "�11",    "����",      "������-11"),
                NEWVAC_LINE( 2, 3, "�12",    "����",      "������-12"),

                NEWVAC_LINE( 3, 0, "�13",    "����",      "������-13"),
                NEWVAC_LINE( 3, 1, "�14",    "����",      "������-14"),
                NEWVAC_LINE( 3, 2, "4D3",    "���-0,025", "����� ��������� ������� � ��-����� (���-�������)"),
                NEWVAC_LINE( 3, 3, "4D4",    "���-0,025", "����� ��������� ������� � �������������� (���-�������)"),

                NEWVAC_LINE( 4, 0, "�1",     "���-0,025", "����������� ����� ����������-���������� 1"),
                NEWVAC_LINE( 4, 1, "�2",     "���-0,025", "����������� ����� ����������-���������� 2"),
                NEWVAC_LINE( 4, 2, "�3",     "����",      "����������� ����� ����������-���������� 3"),
                NEWVAC_LINE( 4, 3, "�4",     "����",      "����������� ����� ����������-���������� 4"),

                NEWVAC_LINE( 5, 0, "�5",     "���-0,025", ""),
                NEWVAC_LINE( 5, 1, "������", "",      ""),
                NEWVAC_LINE( 5, 2, "�5",     "",      ""),
                NEWVAC_LINE( 5, 3, "�6",     "",      ""),

                NEWVAC_LINE( 6, 0, "�1",     "���-0,025", "����������� ����� ����������-���������� 1"),
                NEWVAC_LINE( 6, 1, "�2",     "���-0,025", "����������� ����� ����������-���������� 2"),
                NEWVAC_LINE( 6, 2, "�3",     "����",      "����������� ����� ����������-���������� 3"),
                NEWVAC_LINE( 6, 3, "�4",     "����",      "����������� ����� ����������-���������� 4"),

                NEWVAC_LINE( 7, 0, "", "", ""),
                NEWVAC_LINE( 7, 1, "", "", ""),
                NEWVAC_LINE( 7, 2, "", "", ""),
                NEWVAC_LINE( 7, 3, "", "", ""),

                NEWVAC_LINE( 8, 0, "", "", ""),
                NEWVAC_LINE( 8, 1, "", "", ""),
                NEWVAC_LINE( 8, 2, "", "", ""),
                NEWVAC_LINE( 8, 3, "", "", ""),

                NEWVAC_LINE( 9, 0, "", "", ""),
                NEWVAC_LINE( 9, 1, "", "", ""),
                NEWVAC_LINE( 9, 2, "", "", ""),
                NEWVAC_LINE( 9, 3, "", "", ""),

                NEWVAC_LINE(10, 0, "", "", ""),
                NEWVAC_LINE(10, 1, "", "", ""),
                NEWVAC_LINE(10, 2, "", "", ""),
                NEWVAC_LINE(10, 3, "", "", ""),

                NEWVAC_LINE(11, 0, "", "", ""),
                NEWVAC_LINE(11, 1, "", "", ""),
                NEWVAC_LINE(11, 2, "", "", ""),
                NEWVAC_LINE(11, 3, "", "", ""),

            },
            //"transposed"
        }, 0

    },

    {NULL},
};

#define PHYS_LINE(f,n)                          \
    {C_IMES(f,n),    1000 /* 1mA/V */, 0, 0},   \
    {C_UMES(f,n), 1000000 /* 1kV/V */, 0, 0},   \
    {C_U5_7(f,n), 1000000 /* */,       0, 305}, \
    {C_ISON(f,n),       1,             0, 0}

#define XCEAC124_PHYS_INFO(f) \
    PHYS_LINE(f, 0),         \
    PHYS_LINE(f, 1),         \
    PHYS_LINE(f, 2),         \
    PHYS_LINE(f, 3)

static physprops_t newvac_physinfo[] =
{
    XCEAC124_PHYS_INFO(0),
    XCEAC124_PHYS_INFO(1),
    XCEAC124_PHYS_INFO(2),
    XCEAC124_PHYS_INFO(3),
    XCEAC124_PHYS_INFO(4),
    XCEAC124_PHYS_INFO(5),
    XCEAC124_PHYS_INFO(6),
    XCEAC124_PHYS_INFO(7),
    XCEAC124_PHYS_INFO(8),
    XCEAC124_PHYS_INFO(9),
    XCEAC124_PHYS_INFO(10),
    XCEAC124_PHYS_INFO(11),
};
