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
    {LINE_ID("KV57", f, n), l,    NULL, NULL,    "#L\v#T5\v7", t"\nНасос: "p, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(C_U5_7(f, n)),  \
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
                NEWVAC_LINE( 0, 0, "К1",     "???",       "Приёмник пучка"),
                NEWVAC_LINE( 0, 1, "К2",     "ПВИГ",      "Кольцо-2"),
                NEWVAC_LINE( 0, 2, "К3",     "ПВИГ",      "Кольцо-3"),
                NEWVAC_LINE( 0, 3, "К4",     "ПВИГ",      "Кольцо-4"),

                NEWVAC_LINE( 1, 0, "К5",     "ПВИГ",      "Кольцо-5"),
                NEWVAC_LINE( 1, 1, "К6",     "ПВИГ",      "Кольцо-6"),
                NEWVAC_LINE( 1, 2, "К7",     "ПВИГ",      "Кольцо-7"),
                NEWVAC_LINE( 1, 3, "К8",     "НМД-0,025", "Кольцо-8"),

                NEWVAC_LINE( 2, 0, "К9",     "ПВИГ",      "Кольцо-9"),
                NEWVAC_LINE( 2, 1, "К10",    "ПВИГ",      "Кольцо-10"),
                NEWVAC_LINE( 2, 2, "К11",    "ПВИГ",      "Кольцо-11"),
                NEWVAC_LINE( 2, 3, "К12",    "ПВИГ",      "Кольцо-12"),

                NEWVAC_LINE( 3, 0, "К13",    "ПВИГ",      "Кольцо-13"),
                NEWVAC_LINE( 3, 1, "К14",    "ПВИГ",      "Кольцо-14"),
                NEWVAC_LINE( 3, 2, "4D3",    "НМД-0,025", "Стенд пучкового датчика в ВЧ-блоке (экс-Малютин)"),
                NEWVAC_LINE( 3, 3, "4D4",    "НМД-0,025", "Стенд пучкового датчика в радиопультовой (экс-Малютин)"),

                NEWVAC_LINE( 4, 0, "Э1",     "НМД-0,025", "Электронная часть накопителя-охладителя 1"),
                NEWVAC_LINE( 4, 1, "Э2",     "НМД-0,025", "Электронная часть накопителя-охладителя 2"),
                NEWVAC_LINE( 4, 2, "Э3",     "ПВИГ",      "Электронная часть накопителя-охладителя 3"),
                NEWVAC_LINE( 4, 3, "Э4",     "ПВИГ",      "Электронная часть накопителя-охладителя 4"),

                NEWVAC_LINE( 5, 0, "Э5",     "НМД-0,025", ""),
                NEWVAC_LINE( 5, 1, "резерв", "",      ""),
                NEWVAC_LINE( 5, 2, "П5",     "",      ""),
                NEWVAC_LINE( 5, 3, "П6",     "",      ""),

                NEWVAC_LINE( 6, 0, "П1",     "НМД-0,025", "Позитронная часть накопителя-охладителя 1"),
                NEWVAC_LINE( 6, 1, "П2",     "НМД-0,025", "Позитронная часть накопителя-охладителя 2"),
                NEWVAC_LINE( 6, 2, "П3",     "ПВИГ",      "Позитронная часть накопителя-охладителя 3"),
                NEWVAC_LINE( 6, 3, "П4",     "ПВИГ",      "Позитронная часть накопителя-охладителя 4"),

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
