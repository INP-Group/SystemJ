#define S_C 0

#define C_N(line,col) (S_C + ((line)/16)*112 + (col)*16 + ((line) % 16))

#define CFML(line) (excmd_t [])                                \
    {                                                          \
        CMD_RETSEP_I(0),                                       \
        /* return 0 if limit<=0 */                             \
        CMD_DUP_I(1.0), CMD_PUSH_I(0.0),                       \
        CMD_GETP_I(C_N(line,2)), CMD_CASE,                     \
        CMD_TEST, CMD_BREAK_I(0),                              \
        /* return (value/limit)*100 */                         \
        CMD_GETP_I(C_N(line,0)),                               \
        CMD_GETP_I(C_N(line,2)),                               \
        CMD_DIV,                                               \
        CMD_MUL_I(100.0),                                      \
        CMD_RET                                                \
    }

#define VAC_LINE(line, label, tip) \
    {"Imes"   #line, label,        "",  "%6.1f", NULL, tip,  LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_N(line,0)),          \
        CFML(line), NULL, 0.0, 90.0, 0.0, 0.0, 95.0, .mindisp=0, .maxdisp=1100},                                                       \
    {"Umes"   #line, "U���" #line,        "",  "%4.1f", NULL, NULL,  LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_N(line,1)), .mindisp = 0, .maxdisp = 26}, \
    {"Ilim"   #line, "������ I" #line,    "",  "%3.0f", NULL, NULL,  LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_N(line,2)), .minnorm = 0, .maxnorm = 9999}, \
    {"Ulim"   #line, "������ U" #line,    "",  "%3.1f", NULL, NULL,  LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_N(line,3))}, \
/*    {"IlimHW" #line, "������HW" #line,    "",  NULL,    NULL, NULL,  LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_N(line,4))},*/ \
    {"Ialm"   #line, NULL,                "",  NULL,    NULL, NULL,  LOGT_READ,   LOGD_ALARM,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_N(line,5))}, \
    {"Ualm"   #line, NULL,                "",  NULL,    NULL, NULL,  LOGT_READ,   LOGD_ALARM,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_N(line,6))}

static logchannet_t linvac_chans [] =
{
    VAC_LINE(0,  "GUN",     "1D1 - ����������� �����"),
    VAC_LINE(1,  "BUN1",    "1D2 - ���������������� ��������� - 1"),
    VAC_LINE(2,  "BUN2",    "1D3 - ���������������� ��������� - 2"),
    VAC_LINE(3,  "�����.",  "1D4 - ��������������"),

    VAC_LINE(4,  "WG1_2",   "2D1 - ����� 2 (�� �����) 1-�� ������������ ������ (3-� ��-�)"),
    VAC_LINE(5,  "SLED1",   "2D2 - ���������� �������� ������� ������������ ������"),
    VAC_LINE(6,  "WG1_3",   "2D3 - ����� 3 (� SLED) 1-�� ������������ ������ (3-� ��-�)"),
    VAC_LINE(7,  "WG1_4",   "2D4 - ����� 4 (� ������.) 1-�� ������������ ������ (3-� ��-�)"),

    VAC_LINE(8,  "S1_in",   "3D1 - ���� ������ ������"),
    VAC_LINE(9,  "S1_out",  "3D2 - ����� ������ ������"),
    VAC_LINE(10, "LOS1",    "3D3 - �������� ������ ������"),
    VAC_LINE(11, "S2_in",   "3D4 - ���� ������ ������"),

    VAC_LINE(12, "S2_out",  "4D1 - ����� ������ ������"),
    VAC_LINE(13, "LOS2",    "4D2 - �������� ������ ������"),
    VAC_LINE(14, "SPECT",   "4D3 - �����������"),
    VAC_LINE(15, "S3_in",   "4D4 - ���� ������� ������"),

    VAC_LINE(16, "S3_out",  "5D1 - ����� ������� ������"),
    VAC_LINE(17, "LOS3",    "5D2 - "),
    VAC_LINE(18, "WG2_2",   "5D3 - ����� 2 (�� �����) 2-�� ������������ ������ (2-� ��-�)"),
    VAC_LINE(19, "WG2_3",   "5D4 - ����� 3 (�� SLED) 2-�� ������������ ������ (2-� ��-�)"),

    VAC_LINE(20, "SLED2",   "6D1 - ���������� �������� ������� ������������ ������"),
    VAC_LINE(21, "WG2_4",   "6D2 - ����� 4 (����� SLED) 2-�� ������������ ������ (2-� ��-�)"),
    VAC_LINE(22, "WG2_5",   "6D3 - ����� 5 (����� SLED) 2-�� ������������ ������ (2-� ��-�)"),
    VAC_LINE(23, "WG2_6",   "6D4 - ����� 6 (� ������ 4) 2-�� ������������ ������ (2-� ��-�)"),

    VAC_LINE(24, "WG2_7",   "7D1 - ����� 7 (� ������ 5) 2-�� ������������ ������ (2-� ��-�)"),
    VAC_LINE(25, "WG2_8",   "7D2 - ����� 8 (� ������ 6) 2-�� ������������ ������ (2-� ��-�)"),
    VAC_LINE(26, "S4_in",   "7D3 - ���� ��������� ������"),
    VAC_LINE(27, "S4_out",  "7D4 - ����� ��������� ������"),

    VAC_LINE(28, "LOS4",    "8D1 - �������� ��������� ������"),
    VAC_LINE(29, "S5_in",   "8D2 - ���� ����� ������"),
    VAC_LINE(30, "S5_out",  "8D3 - ����� ����� ������"),
    VAC_LINE(31, "LOS5",    "8D4 - �������� ����� ������"),

    VAC_LINE(32, "TURN-1",  "9D1 - �������-1"),
    VAC_LINE(33, "TURN-2",  "9D2 - �������-2"),
    VAC_LINE(34, "-",       "9D3 - "),
    VAC_LINE(35, "pCnv",    "9D4 - ����������� ������"),

    VAC_LINE(36, "S6_in",   "10D1 - ���� ������ ������"),
    VAC_LINE(37, "S6_out",  "10D2 - ����� ������ ������"),
    VAC_LINE(38, "LOS6",    "10D3 - �������� ������ ������"),
    VAC_LINE(39, "WG3_2",   "10D4 - ����� 2 (�� �����) 3-�� ������������ ������ (1-� ��-�)"),

    VAC_LINE(40, "SLED3",   "11D1 - ���������� �������� �������� ������������ ������"),
    VAC_LINE(41, "WG3_3",   "11D2 - ����� 3 (17) 3-�� ������������ ������ (1-� ��-�)"),
    VAC_LINE(42, "WG3_4",   "11D3 - ����� 4 (18) 3-�� ������������ ������ (1-� ��-�)"),
    VAC_LINE(43, "WG3_5",   "11D4 - ����� 5 (19) 3-�� ������������ ������ (1-� ��-�)"),

    VAC_LINE(44, "WG3_6",   "12D1 - ����� 2 (20) 3-�� ������������ ������ (1-� ��-�)"),
    VAC_LINE(45, "WG3_7",   "12D2 - ����� 2 (21) 3-�� ������������ ������ (1-� ��-�)"),
    VAC_LINE(46, "S7_in",   "12D3 - ���� ������� ������"),
    VAC_LINE(47, "S7_out",  "12D4 - ����� ������� ������"),

    VAC_LINE(48, "LOS7",    "13D1 - �������� ������� ������"),
    VAC_LINE(49, "S8_in",   "13D2 - ���� ������� ������"),
    VAC_LINE(50, "S8_out",  "13D3 - ����� ������� ������"),
    VAC_LINE(51, "LOS8",    "13D4 - �������� ������� ������"),

    VAC_LINE(52, "S9_in",   "14D1 - ���� ������� ������"),
    VAC_LINE(53, "S9_out",  "14D2 - ����� ������� ������"),
    VAC_LINE(54, "LOS9",    "14D3 - �������� ������� ������"),
    VAC_LINE(55, "S10_in",  "14D4 - ���� ������� ������"),

    VAC_LINE(56, "S10_out", "15D1 - ����� ������� ������"),
    VAC_LINE(57, "LOS10",   "15D2 - �������� ������� ������"),
    VAC_LINE(58, "WG4_2",   "15D3 - ����� 2 (�� �����) 4-�� ������������ ������ (4-� ��-�)"),
    VAC_LINE(59, "SLED4",   "15D4 - ���������� �������� ���������� ������������ ������"),

    VAC_LINE(60, "WG4_3",   "16D1 - ����� 2 (24) 4-�� ������������ ������ (4-� ��-�)"),
    VAC_LINE(61, "WG4_4",   "16D2 - ����� 2 (25) 4-�� ������������ ������ (4-� ��-�)"),
    VAC_LINE(62, "WG4_5",   "16D3 - ����� 2 (26) 4-�� ������������ ������ (4-� ��-�)"),
    VAC_LINE(63, "-",       "16D4 - "),

    VAC_LINE(64, "S11_in",  "1D1 - ���� ������������ ������"),
    VAC_LINE(65, "S11_out", "1D2 - ����� ������������ ������"),
    VAC_LINE(66, "LOS11",   "1D3 - �������� ������������ ������"),
    VAC_LINE(67, "S12_in",  "1D4 - ���� ����������� ������"),

    VAC_LINE(68, "S12_out", "2D1 - ����� ����������� ������"),
    VAC_LINE(69, "LOS12",   "2D2 - �������� ����������� ������"),
    VAC_LINE(70, "S13_in",  "2D3 - ���� ����������� ������"),
    VAC_LINE(71, "S13_out", "2D4 - ����� ����������� ������"),

    VAC_LINE(72, "LOS13",   "3D1 - �������� ����������� ������"),
    VAC_LINE(73, "S14_in",  "3D2 - ���� ������������� ������"),
    VAC_LINE(74, "S14_out", "3D3 - ����� ������������� ������"),
    VAC_LINE(75, "LOS14",   "3D4 - �������� ������������� ������"),

    VAC_LINE(76, "PP",      "4D1 - "),
    VAC_LINE(77, "-",       "4D2 - "),
    VAC_LINE(78, "-",       "4D3 - "),
    VAC_LINE(79, "-",       "4D4 - "),

    VAC_LINE(80, "5D1",     "5D1 - "),
    VAC_LINE(81, "5D2",     "5D2 - "),
    VAC_LINE(82, "5D3",     "5D3 - "),
    VAC_LINE(83, "5D4",     "5D4 - "),

    VAC_LINE(84, "6D1",     "6D1 - "),
    VAC_LINE(85, "6D2",     "6D2 - "),
    VAC_LINE(86, "6D3",     "6D3 - "),
    VAC_LINE(87, "6D4",     "6D4 - "),

    VAC_LINE(88, "7D1",     "7D1 - "),
    VAC_LINE(89, "7D2",     "7D2 - "),
    VAC_LINE(90, "7D3",     "7D3 - "),
    VAC_LINE(91, "7D4",     "7D4 - "),

    VAC_LINE(92, "8D1",     "8D1 - "),
    VAC_LINE(93, "8D2",     "8D2 - "),
    VAC_LINE(94, "8D3",     "8D3 - "),
    VAC_LINE(95, "8D4",     "8D4 - "),
};
static elemnet_t linvac_elem = {
    "VacMatrix", "������",
    "I, �A\f���������� ����\v"
        "U, kV\f���������� ����������\v"
        "Il, �A\f������� �� ����\v"
        "Ul, kV\f������� �� ����������\v"
        "I!\vU!",
    "?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v"
        "?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v"
        "?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v"
        "?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v"
        "?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v"
        "?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v?\f?\v",
    ELEM_MULTICOL, 16*6*(4+2), 6 + 32*65536, linvac_chans, "noshadow,notitle"
};

static groupunit_t linvac_grouping[] =
{
    {&linvac_elem},
    {NULL}
};

#define  CHAN_LINE(NUM)\
{ C_N((NUM),0), 1.0/0.1, 0 },\
{ C_N((NUM),1), 1.0/0.1, 0 },\
{ C_N((NUM),2), 1.0/0.1, 0 },\
{ C_N((NUM),3), 1.0/0.1, 0 },\
{ C_N((NUM),4), 1, 0 },\
{ C_N((NUM),5), 1, 0 },\
{ C_N((NUM),6), 1, 0 }

static physprops_t linvac_physinfo[] =
{
CHAN_LINE(0),
CHAN_LINE(1),
CHAN_LINE(2),
CHAN_LINE(3),
CHAN_LINE(4),
CHAN_LINE(5),
CHAN_LINE(6),
CHAN_LINE(7),
CHAN_LINE(8),
CHAN_LINE(9),
CHAN_LINE(10),
CHAN_LINE(11),
CHAN_LINE(12),
CHAN_LINE(13),
CHAN_LINE(14),
CHAN_LINE(15),
CHAN_LINE(16),
CHAN_LINE(17),
CHAN_LINE(18),
CHAN_LINE(19),
CHAN_LINE(20),
CHAN_LINE(21),
CHAN_LINE(22),
CHAN_LINE(23),
CHAN_LINE(24),
CHAN_LINE(25),
CHAN_LINE(26),
CHAN_LINE(27),
CHAN_LINE(28),
CHAN_LINE(29),
CHAN_LINE(30),
CHAN_LINE(31),
CHAN_LINE(32),
CHAN_LINE(33),
CHAN_LINE(34),
CHAN_LINE(35),
CHAN_LINE(36),
CHAN_LINE(37),
CHAN_LINE(38),
CHAN_LINE(39),
CHAN_LINE(40),
CHAN_LINE(41),
CHAN_LINE(42),
CHAN_LINE(43),
CHAN_LINE(44),
CHAN_LINE(45),
CHAN_LINE(46),
CHAN_LINE(47),
CHAN_LINE(48),
CHAN_LINE(49),
CHAN_LINE(50),
CHAN_LINE(51),
CHAN_LINE(52),
CHAN_LINE(53),
CHAN_LINE(54),
CHAN_LINE(55),
CHAN_LINE(56),
CHAN_LINE(57),
CHAN_LINE(58),
CHAN_LINE(59),
CHAN_LINE(60),
CHAN_LINE(61),
CHAN_LINE(62),
CHAN_LINE(63),
CHAN_LINE(64),
CHAN_LINE(65),
CHAN_LINE(66),
CHAN_LINE(67),
CHAN_LINE(68),
CHAN_LINE(69),
CHAN_LINE(70),
CHAN_LINE(71),
CHAN_LINE(72),
CHAN_LINE(73),
CHAN_LINE(74),
CHAN_LINE(75),
CHAN_LINE(76),
CHAN_LINE(77),
CHAN_LINE(78),
CHAN_LINE(79),
CHAN_LINE(80),
CHAN_LINE(81),
CHAN_LINE(82),
CHAN_LINE(83),
CHAN_LINE(84),
CHAN_LINE(85),
CHAN_LINE(86),
CHAN_LINE(87),
CHAN_LINE(88),
CHAN_LINE(89),
CHAN_LINE(90),
CHAN_LINE(91),
CHAN_LINE(92),
CHAN_LINE(93),
CHAN_LINE(94),
CHAN_LINE(95),
};


#undef S_C
#undef VAC_LINE
#undef  CHAN_LINE

