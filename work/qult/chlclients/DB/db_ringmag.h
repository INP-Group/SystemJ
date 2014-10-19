#include "common_elem_macros.h"

#define PHYSINFO_ARRAY_NAME ringmag_physinfo
#include "pi_ring1_32.h"


#define RINGMAG_IST_LINE(n, id, label, tip, max, step) \
    MAGX_IST_XCDAC20_LINE(id, label, tip, B_IST_C20(n), B_IST_IST(n), max, step, 0, 0)
#define RINGMAG_REV_LINE(n, id, label, tip, max, step) \
    MAGX_IST_XCDAC20_LINE(id, label, tip, B_IST_C20(n), B_IST_IST(n), max, step, 0, 1)
#define RINGMAG_X1K_LINE(n, id, label, tip, max, step) \
    MAGX_X1K_XCDAC20_LINE(id, label, tip, B_IST_C20(n), B_IST_IST(n), max, step)
#define RINGMAG_X3H_LINE(n, id, label, tip, max) \
    MAGX_XCH300_LINE     (id, label, tip, B_VCH400_ADC, B_VCH400_DAC, B_VCH400_V3H + V3H_XA40D16_NUMCHANS*(n), n, max)

#define RINGMAG_SECTION(label) \
    EMPTY_CELL(),       \
    HLABEL_CELL(label), \
    EMPTY_CELL(),       \
    EMPTY_CELL(),       \
    EMPTY_CELL(),       \
    EMPTY_CELL()


static groupunit_t ringmag_grouping[] =
{
#if 1
    GLOBELEM_START_CRN("ringmag", "Магнитная система",
                       "Задано, A\vИзм., A\vd/dT\v\v", NULL,
                       32*6, 6)
        RINGMAG_SECTION("Впуск-П"),
        RINGMAG_X1K_LINE(0,  m_1l0,  "1L0",      "ИП2: В-1000",   1000, 1),
        RINGMAG_X1K_LINE(1,  m_1l1,  "1L1",      "ИП2: В-1000",   1000, 1),
        RINGMAG_X1K_LINE(2,  m_1l2,  "1L2, 1L6", "ИП2: В-1000",   1000, 1),
        RINGMAG_X1K_LINE(3,  m_1l3,  "1L3, 1L5", "ИП2: В-1000",   1000, 1),
        RINGMAG_X1K_LINE(4,  m_1l4,  "1L4, 1L8", "ИП2: В-1000",   1000, 1),
        RINGMAG_IST_LINE(12, m_1l7,  "1l7, 1L9", "ИП1: И-В-1000", 1000, 1),
        RINGMAG_X3H_LINE(0,  m_1l10, "1L10",     "ИП1: ВЧ-400",   1000.0),
        RINGMAG_X3H_LINE(1,  m_1l11, "1L11",     "ИП1: ВЧ-400",   1000.0),
        RINGMAG_SECTION("Впуск-Э"),
        RINGMAG_IST_LINE(16, m_1m5,  "1M5-1M7",  "ИП1: ИСТ",      1000, 1),
        RINGMAG_X3H_LINE(2,  m_2l1,  "2L1",      "ИП1: ВЧ-400",   1000.0),
        RINGMAG_X3H_LINE(3,  m_2l2,  "2L2",      "ИП1: ВЧ-400",   1000.0),
        RINGMAG_X3H_LINE(4,  m_2l3,  "2L3",      "ИП1: ВЧ-400",   1000.0),
        RINGMAG_X3H_LINE(5,  m_2l4,  "2L4",      "ИП1: ВЧ-400",   1000.0),
        RINGMAG_X3H_LINE(6,  m_2l5,  "2L5",      "ИП1: ВЧ-400",   1000.0),
        RINGMAG_SECTION("Кольцо"),
        RINGMAG_IST_LINE(15, m_rm1, "RM1-RM8",   "ИП1: ИСТ",      1000, 1),
        RINGMAG_IST_LINE(11, m_sm1, "SM1, SM2",  "ИП1: И-В-1000", 1000, 1),
        RINGMAG_IST_LINE(10, m_d1,  "D1",        "ИП1: И-В-1000", 1000, 1), // ?
        RINGMAG_IST_LINE(21, m_f1,  "F1, F2",    "ИП1: ИСТ",      1000, 1), // ?
        RINGMAG_IST_LINE(19, m_f4,  "F4",        "ИП1: ИСТ",      1000, 1), // ?
        RINGMAG_IST_LINE(18, m_d2,  "D2",        "ИП1: ИСТ",      1000, 1), // ?
        RINGMAG_IST_LINE(17, m_d3,  "D3",        "ИП1: ИСТ",      1000, 1), // ?
        RINGMAG_IST_LINE(20, m_f3,  "F3",        "ИП1: ИСТ",      1000, 1), // ?
        RINGMAG_SECTION("Выпуск"),
        RINGMAG_IST_LINE(5,  m_3m4, "3M4, 3M5",  "ИП1: И-В-1000", 1000, 1),
        RINGMAG_IST_LINE(6,  m_4m4, "4M4, 4M5",  "ИП1: И-В-1000", 1000, 1),
        RINGMAG_IST_LINE(7,  m_3m1, "3M1, 3M3",  "ИП1: И-В-1000", 1000, 1),
        RINGMAG_IST_LINE(8,  m_4m1, "4M1, 4M3",  "ИП1: И-В-1000", 1000, 1),
        RINGMAG_REV_LINE(23, m_r13, "R#13",      "ИП1: ИСТ",      1000, 1), // ?
        RINGMAG_REV_LINE(24, m_r14, "R#14",      "ИП1: ИСТ",      1000, 1), // ?
    GLOBELEM_END("", 0),
#else
    GLOBELEM_START_CRN("vch400", "Магнитная система: VCh400",
                       "Задано, A\vИзм., A\vd/dT\v\v", NULL,
                       8*6, 6)
        RINGMAG_X3H_LINE(0, l1l10, "1L10", "ИП1", 1000.0),
        RINGMAG_X3H_LINE(1, l1l11, "1L11", "ИП1", 1000.0),
        RINGMAG_X3H_LINE(2, l2l1,  "2L1",  "ИП1", 1000.0),
        RINGMAG_X3H_LINE(3, l2l2,  "2L2",  "ИП1", 1000.0),
        RINGMAG_X3H_LINE(4, l2l3,  "2L3",  "ИП1", 1000.0),
        RINGMAG_X3H_LINE(5, l2l4,  "2L4",  "ИП1", 1000.0),
        RINGMAG_X3H_LINE(6, l2l5,  "2L5",  "ИП1", 1000.0),
        RINGMAG_X3H_LINE(7, rsrv,  "rsrv", "ИП1", 1000.0),
    GLOBELEM_END("", 0),

    GLOBELEM_START_CRN("ip2", "Магнитная система: IP-2",
                       "Задано, A\vИзм., A\vd/dT\v\v", NULL,
                       5*6, 6)
        RINGMAG_X1K_LINE(0, ip2_1l0, "ip2_1l0", "ИП2", 1000, 1),
        RINGMAG_X1K_LINE(1, ip2_1l1, "ip2_1l1", "ИП2", 1000, 1),
        RINGMAG_X1K_LINE(2, ip2_1l2, "ip2_1l2", "ИП2", 1000, 1),
        RINGMAG_X1K_LINE(3, ip2_1l3, "ip2_1l3", "ИП2", 1000, 1),
        RINGMAG_X1K_LINE(4, ip2_1l4, "ip2_1l4", "ИП2", 1000, 1),
    GLOBELEM_END("", GU_FLAG_FROMNEWLINE),

    GLOBELEM_START_CRN("ip1_v1000", "Магнитная система: IP-1/v1000",
                       "Задано, A\vИзм., A\vd/dT\v\v", NULL,
                       10*6, 6)
        RINGMAG_IST_LINE(5,  ip1_3m4, "ip1_3m4", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(6,  ip1_4m4, "ip1_4m4", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(7,  ip1_3m1, "ip1_3m1", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(8,  ip1_4m1, "ip1_4m1", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(9,  ip1_r14, "ip1_r14", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(10, ip1_d1,  "ip1_d1",  "ИП1", 1000, 1),
        RINGMAG_IST_LINE(11, ip1_sm1, "ip1_sm1", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(12, ip1_1l7, "ip1_1l7", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(13, ip1_r8,  "ip1_r8",  "ИП1", 1000, 1),
        RINGMAG_IST_LINE(14, ip1_r9,  "ip1_r9",  "ИП1", 1000, 1),
    GLOBELEM_END("", GU_FLAG_FROMNEWLINE),

    GLOBELEM_START_CRN("ip1_ists", "Магнитная система: IP-1/ISTs",
                       "Задано, A\vИзм., A\vd/dT\v\v", NULL,
                       10*6, 6)
        RINGMAG_IST_LINE(15, ip1_rm1, "ip1_rm1", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(16, ip1_1m5, "ip1_1m5", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(17, ip1_d3,  "ip1_d3",  "ИП1", 1000, 1),
        RINGMAG_IST_LINE(18, ip1_d2,  "ip1_d2",  "ИП1", 1000, 1),
        RINGMAG_IST_LINE(19, ip1_f4,  "ip1_f4",  "ИП1", 1000, 1),
        RINGMAG_IST_LINE(20, ip1_f3,  "ip1_f3",  "ИП1", 1000, 1),
        RINGMAG_IST_LINE(21, ip1_f1,  "ip1_f1",  "ИП1", 1000, 1),
        RINGMAG_IST_LINE(22, ip1_i12, "ip1_i12", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(23, ip1_i13, "ip1_i13", "ИП1", 1000, 1),
        RINGMAG_IST_LINE(24, ip1_i14, "ip1_i14", "ИП1", 1000, 1),
    GLOBELEM_END("", GU_FLAG_FROMNEWLINE),
#endif

    {NULL}
};
