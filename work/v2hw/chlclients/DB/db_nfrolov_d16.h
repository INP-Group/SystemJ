#include "common_elem_macros.h"

#include "drv_i/nfrolov_d16_drv_i.h"

enum
{
    D16_BASE  = 0 + NFROLOV_D16_NUMCHANS*0
};

#define EMPTY_CELL() {NULL, " ",  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP}

#define D16OUT_LINE(base, l) \
    {"a"  #l, "A",  NULL, "%5.0f",  "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_A_BASE   + l)}, \
    {"b"  #l, "B",  NULL, "%3.0f",  "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_B_BASE   + l)}, \
    {"t"  #l, "=",  "ns", "%10.1f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_V_BASE   + l)}, \
    {"off"#l, "-",  NULL, NULL,     NULL,        NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_OFF_BASE + l)}

static groupunit_t nfrolov_d16_grouping[] =
{
    {
        &(elemnet_t){
            NULL, NULL,
            NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP, .placement="horz=right"},
            },
            "notitle,noshadow,nocoltitles,norowtitles"
        }, GU_FLAG_FROMNEWLINE | GU_FLAG_FILLHORZ
    },

    {
        &(elemnet_t){
            "d16_1", "ä16-1",
            "", "",
            ELEM_MULTICOL, 3, 1,
            (logchannet_t []){
                SUBELEM_START("ctl", "Controls", 5, 5)
                    SUBELEM_START("clk", "Clocking", 3, 1)
                        {"fclk",    "Fclk",   NULL, NULL,    "#L\v#-#TFin\vQuartz 25MHz\vImpact~25MHz", NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_FCLK_SEL), .placement="horz=right"},
                        {"fin_qns", "Fin (ns)", NULL, "%7.1f", NULL,                        NULL, LOGT_WRITE1,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_F_IN_NS)},
                        {"kclk",    "Kclk",   NULL, NULL,    "#L/\v#T1\v2\v4\v8",         NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_KCLK_N),   .placement="horz=right"},
                    SUBELEM_END("noshadow,notitle,nocoltitles", NULL),
                    VSEP_L(),
                    SUBELEM_START("starts", "Starts", 3, 1)
                        {"start",   "Start",   NULL, NULL,    "#TSTART\vCAMAC Y1\v50Hz", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_START_SEL), .placement="horz=right"},
                        {"kstart",  "/Kstart", NULL, "%3.0f", "withlabel",             NULL, LOGT_WRITE1,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_KSTART),    .placement="horz=right"},
                        {"mode",    "Mode",   NULL, NULL,    "#TContinuous\vOneshot",   NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_MODE),       .placement="horz=right"},
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                    VSEP_L(),
                    SUBELEM_START("Run", "Operation", 3, 1)
                        {"tyk",     "\nTyk!",  NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_DO_SHOT),                                     .placement="horz=right"},
                        EMPTY_CELL(),
                        {"disable", "-All",   NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_ONOFF,    LOGK_DIRECT, LOGC_VIC,    PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_ALLOFF), NULL, NULL, 0.0, 0.1, 0.0, 0.0, 1.1, .placement="horz=right"},
                    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
                SEP_L(),
                SUBELEM_START_CRN("outputs", "Outputs", NULL, "1\v2\v3\v4", 4*4, 4)
                    D16OUT_LINE(D16_BASE, 0),
                    D16OUT_LINE(D16_BASE, 1),
                    D16OUT_LINE(D16_BASE, 2),
                    D16OUT_LINE(D16_BASE, 3),
                SUBELEM_END("noshadow,notitle,nocoltitles", NULL),
            },
            "nocoltitles,norowtitles"
        }
    },

    {
        &(elemnet_t){
            "d16_1", "ä16-1",
            "", "",
            ELEM_MULTICOL, 5, 1,
            (logchannet_t []){
                SUBELEM_START("run", "Run", 2, 2)
                    {"tyk",     "\nTyk!", NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_DO_SHOT),                                     .placement="horz=right"},
                    {"disable", "-All",   NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_ONOFF,    LOGK_DIRECT, LOGC_VIC,    PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_ALLOFF), NULL, NULL, 0.0, 0.1, 0.0, 0.0, 1.0, .placement="horz=right"},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
                SUBELEM_START_CRN("outputs", "Outputs", NULL, "1\v2\v3\v4", 4*4, 4)
                    D16OUT_LINE(D16_BASE, 0),
                    D16OUT_LINE(D16_BASE, 1),
                    D16OUT_LINE(D16_BASE, 2),
                    D16OUT_LINE(D16_BASE, 3),
                SUBELEM_END("noshadow,notitle,nocoltitles", NULL),
                SEP_L(),
                SUBELEM_START("clocks1", "Clocking", 3, 3)
                    {"kstart",  "Kstart", NULL, "%3.0f", "withlabel",             NULL, LOGT_WRITE1,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_KSTART)},
                    {"kclk",    "Kclk",   NULL, NULL,    "#T1\v2\v4\v8",          NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_KCLK_N)},
                    {"fclk",    "Fclk",   NULL, NULL,    "#TFin\vQuartz\vImpact", NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_FCLK_SEL)},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                SUBELEM_START("clocks2", "Clocking", 2, 2)
                    {"start",   "Start",  NULL, NULL,    "#TSTART\vCAMAC Y1\v50Hz", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_START_SEL)},
                    {"mode",    "Mode",   NULL, NULL,    "#TContinuous\vOneshot",   NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(D16_BASE + NFROLOV_D16_CHAN_MODE)},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            },
            "nocoltitles,norowtitles"
        }
    },

    {NULL}
};

static physprops_t nfrolov_d16_physinfo[] =
{
    {D16_BASE + NFROLOV_D16_CHAN_V_BASE + 0, 4, 0, 0},
    {D16_BASE + NFROLOV_D16_CHAN_V_BASE + 1, 4, 0, 0},
    {D16_BASE + NFROLOV_D16_CHAN_V_BASE + 2, 4, 0, 0},
    {D16_BASE + NFROLOV_D16_CHAN_V_BASE + 3, 4, 0, 0},
};
