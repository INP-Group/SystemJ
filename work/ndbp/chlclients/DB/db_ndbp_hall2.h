#include "ndbp_elemlist.h"
#include "drv_i/nfrolov_d16_drv_i.h"
#include "drv_i/gzi11_drv_i.h"
#include "drv_i/tsycamv_drv_i.h"

#include "common_elem_macros.h"


enum
{
    DAC16_BASE   = 0,
    NFD16_1_BASE = 16,
    CAMC_BASE    = 46,
    NFD16_2_BASE = 256,
};


#define DAC1_LINE(n, l, min, max) \
    {"dac_"__CX_STRINGIZE(n)"_set", l, NULL, "%5.3f", NULL, "", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DAC16_BASE+n), .minnorm=min, .maxnorm=max, .incdec_step=0.1} \

#define D16OUT_LINE(base, l) \
    {"a"  #l, "A",  NULL, "%5.0f",  "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_A_BASE   + l)}, \
    {"b"  #l, "B",  NULL, "%3.0f",  "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_B_BASE   + l)}, \
    {"t"  #l, "=",  "ns", "%10.1f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_V_BASE   + l)}, \
    {"off"#l, "-",  NULL, NULL,     NULL,        NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(base + NFROLOV_D16_CHAN_OFF_BASE + l)}

#define JUST_CHAN(id, cn) \
    {id, NULL, NULL, NULL, "withlabel compact", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

static groupunit_t ndbp_hall2_grouping[] =
{
    {
        &(elemnet_t){
            "image", "Image",
            NULL, NULL,
            ELEM_IMAGE, 2, 0,
            (logchannet_t []){
                JUST_CHAN("K", CAMC_BASE + TSYCAMV_PARAM_K),
                JUST_CHAN("T", CAMC_BASE + TSYCAMV_PARAM_T),
            },
            " bigc=1 port1=8008 port2=8009"
                " chanlist="
        }, 0
    },

    {
        &(elemnet_t){
            "dac1616", "DAC, V",
            "",
            "?\v?\v?\v?\v?\v?\v?\v?\v?\v?\v?\v?\v?\v?\v?\v?",
            ELEM_SINGLECOL, 16, 1,
            (logchannet_t []){
                DAC1_LINE(0,  "0",  -6.5, +6.5),
                DAC1_LINE(1,  "1",  -6.5, +6.5),
                DAC1_LINE(2,  "2",  -6.5, +6.5),
                DAC1_LINE(3,  "3",  -6.5, +6.5),
                DAC1_LINE(4,  "4",  -6.5, +6.5),
                DAC1_LINE(5,  "5",  -6.5, +6.5),
                DAC1_LINE(6,  "6",  -6.5, +6.5),
                DAC1_LINE(7,  "7",  -6.5, +6.5),
                DAC1_LINE(8,  "8",  -6.5, +6.5),
                DAC1_LINE(9,  "9",  -6.5, +6.5),
                DAC1_LINE(10, "10", -6.5, +6.5),
                DAC1_LINE(11, "11", -6.5, +6.5),
                DAC1_LINE(12, "12", -6.5, +6.5),
                DAC1_LINE(13, "13", -6.5, +6.5),
                DAC1_LINE(14, "14", -6.5, +6.5),
                DAC1_LINE(15, "15", -6.5, +6.5),
            },
            "nocoltitles"
        }, 0
    },

    GLOBELEM_START(":", "D16s", 2, 1)
        SUBELEM_START("d16_1", "ä16-1", 3, 1)
            SUBELEM_START("ctl", "Controls", 5, 5)
                SUBELEM_START("clk", "Clocking", 3, 1)
                    {"fclk",    "Fclk",   NULL, NULL,    "#L\v#TFin\vQuartz\vImpact", NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_FCLK_SEL), .placement="horz=right"},
                    {"fin_qns", "Fin (ns)", NULL, "%7.1f", NULL,                      NULL, LOGT_WRITE1,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_F_IN_NS)},
                    {"kclk",    "Kclk",   NULL, NULL,    "#L/\v#T1\v2\v4\v8",         NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_KCLK_N),   .placement="horz=right"},
                SUBELEM_END("noshadow,notitle,nocoltitles", NULL),
                VSEP_L(),
                SUBELEM_START("starts", "Starts", 3, 1)
                    {"start",   "Start",   NULL, NULL,    "#TSTART\vCAMAC Y1\v50Hz", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_START_SEL), .placement="horz=right"},
                    {"kstart",  "/Kstart", NULL, "%3.0f", "withlabel",             NULL, LOGT_WRITE1,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_KSTART),    .placement="horz=right"},
                    {"mode",    "Mode",   NULL, NULL,    "#TContinuous\vOneshot",   NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL,  PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_MODE),      .placement="horz=right"},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                VSEP_L(),
                SUBELEM_START("Run", "Operation", 3, 1)
                    {"tyk",     "\nTyk!",  NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_DO_SHOT),                                     .placement="horz=right"},
                    EMPTY_CELL(),
                    {"disable", "-All",   NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_ONOFF,    LOGK_DIRECT, LOGC_VIC,     PHYS_ID(NFD16_1_BASE + NFROLOV_D16_CHAN_ALLOFF), NULL, NULL, 0.0, 0.1, 0.0, 0.0, 1.1, .placement="horz=right"},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
            SEP_L(),
            SUBELEM_START_CRN("outputs", "Outputs", NULL, "1\v2\v3\v4", 4*4, 4)
                D16OUT_LINE(NFD16_1_BASE, 0),
                D16OUT_LINE(NFD16_1_BASE, 1),
                D16OUT_LINE(NFD16_1_BASE, 2),
                D16OUT_LINE(NFD16_1_BASE, 3),
            SUBELEM_END("noshadow,notitle,nocoltitles", NULL),
        SUBELEM_END("nocoltitles,norowtitles", NULL),
        SUBELEM_START("d16_2", "ä16-2", 3, 1)
            SUBELEM_START("ctl", "Controls", 5, 5)
                SUBELEM_START("clk", "Clocking", 3, 1)
                    {"fclk",    "Fclk",   NULL, NULL,    "#L\v#TFin\vQuartz\vImpact", NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL,   PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_FCLK_SEL), .placement="horz=right"},
                    {"fin_qns", "Fin (ns)", NULL, "%7.1f", NULL,                        NULL, LOGT_WRITE1,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_F_IN_NS)},
                    {"kclk",    "Kclk",   NULL, NULL,    "#L/\v#T1\v2\v4\v8",         NULL, LOGT_WRITE1,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL,   PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_KCLK_N),   .placement="horz=right"},
                SUBELEM_END("noshadow,notitle,nocoltitles", NULL),
                VSEP_L(),
                SUBELEM_START("starts", "Starts", 3, 1)
                    {"start",   "Start",   NULL, NULL,    "#TSTART\vCAMAC Y1\v50Hz", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_START_SEL), .placement="horz=right"},
                    {"kstart",  "/Kstart", NULL, "%3.0f", "withlabel",             NULL, LOGT_WRITE1,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_KSTART),    .placement="horz=right"},
                    {"mode",    "Mode",   NULL, NULL,    "#TContinuous\vOneshot",   NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL,  PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_MODE),      .placement="horz=right"},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                VSEP_L(),
                SUBELEM_START("Run", "Operation", 3, 1)
                    {"tyk",     "\nTyk!",  NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_DO_SHOT),                                     .placement="horz=right"},
                    EMPTY_CELL(),
                    {"disable", "-All",   NULL, NULL,    NULL,                    NULL, LOGT_WRITE1,   LOGD_ONOFF,    LOGK_DIRECT, LOGC_VIC,     PHYS_ID(NFD16_2_BASE + NFROLOV_D16_CHAN_ALLOFF), NULL, NULL, 0.0, 0.1, 0.0, 0.0, 1.1, .placement="horz=right"},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
            SEP_L(),
            SUBELEM_START_CRN("outputs", "Outputs", NULL, "1\v2\v3\v4", 4*4, 4)
                D16OUT_LINE(NFD16_2_BASE, 0),
                D16OUT_LINE(NFD16_2_BASE, 1),
                D16OUT_LINE(NFD16_2_BASE, 2),
                D16OUT_LINE(NFD16_2_BASE, 3),
            SUBELEM_END("noshadow,notitle,nocoltitles", NULL),
        SUBELEM_END("nocoltitles,norowtitles", NULL),
    GLOBELEM_END("noshadow,notitle,nocoltitles,norowtitles", 0),

    {
        &(elemnet_t){
            "adc200_1", "ADC200 #1",
            NULL, NULL,
            ELEM_NADC200, 0, 0,
            NULL,
            "bigc=3"
        }, 1
    },
    {
        &(elemnet_t){
            "adc200_1", "ADC200 #2",
            NULL, NULL,
            ELEM_NADC200, 0, 0,
            NULL,
            "bigc=4"
        }, 0
    },
    {NULL}
};

static physprops_t ndbp_hall2_physinfo[] =
{
    {DAC16_BASE +  0,   1000000, 0, 200},
    {DAC16_BASE +  1,   1000000, 0, 200},
    {DAC16_BASE +  2,   1000000, 0, 200},
    {DAC16_BASE +  3,   1000000, 0, 200},
    {DAC16_BASE +  4,   1000000, 0, 200},
    {DAC16_BASE +  5,   1000000, 0, 200},
    {DAC16_BASE +  6,   1000000, 0, 200},
    {DAC16_BASE +  7,   1000000, 0, 200},
    {DAC16_BASE +  8,   1000000, 0, 200},
    {DAC16_BASE +  9,   1000000, 0, 200},
    {DAC16_BASE + 10,   1000000, 0, 200},
    {DAC16_BASE + 11,   1000000, 0, 200},
    {DAC16_BASE + 12,   1000000, 0, 200},
    {DAC16_BASE + 13,   1000000, 0, 200},
    {DAC16_BASE + 14,   1000000, 0, 200},
    {DAC16_BASE + 15,   1000000, 0, 200},
};
