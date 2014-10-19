#include "drv_i/v06xx_drv_i.h"
#include "drv_i/frolov_bl700_drv_i.h"

#include "common_elem_macros.h"

enum
{
    PKS_BASE = 319,
    URR_BASE = 0,
    ADC_BASE = 225,
    TRM_BASE = 25,
    BL7_BASE = 289
};


#define RINGRF_LINE(n) \
    {"i"__CX_STRINGIZE(n)"set", "Катушка "__CX_STRINGIZE(n), NULL, "%6.3f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PKS_BASE +  1 + n-1)}, \
    {"a"__CX_STRINGIZE(n)"min", NULL,                        NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BL7_BASE + FROLOV_BL700_CHAN_MIN_ILK_n_BASE + n-1)}, \
    {"i"__CX_STRINGIZE(n)"mes", NULL,                        NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ADC_BASE +  0 + n-1)}, \
    {"a"__CX_STRINGIZE(n)"max", NULL,                        NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_REDLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(BL7_BASE + FROLOV_BL700_CHAN_MAX_ILK_n_BASE + n-1)}, \
    {"u"__CX_STRINGIZE(n)"mes", NULL,                        NULL, "%6.3f", NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ADC_BASE + 16 + n-1)}

#define RINGRF_KLS(id, l, u_u, u_i, c_u, c_i) \
    {"u_"id, "U"l, u_u, "%6.3f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ADC_BASE + c_u), .maxdisp=20000}, \
    {"i_"id, "I"l, u_i, "%6.3f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ADC_BASE + c_i), .maxdisp=20000}

#define RINGRF_MES(id, l, u, c) \
    {id, l, u, "%6.3f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ADC_BASE + c), .maxdisp=20000}

#define RINGRF_TRM(id, l, u, c) \
    {id, l, u, "%6.3f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(TRM_BASE + c), .maxdisp=20000}


static groupunit_t ringrf_grouping[] =
{

    GLOBELEM_START_CRN("coils", "Катушки соленоида", "Iуст, ??\v<\fПренижение минимума\vIизм, ??\v>\fПревышение максимума\vUизм, ??", NULL, 5*5, 5)
        RINGRF_LINE(1),
        RINGRF_LINE(2),
        RINGRF_LINE(3),
        RINGRF_LINE(4),
        RINGRF_LINE(5),
    GLOBELEM_END("", 0),

    GLOBELEM_START("kls_pwr", "Питание клистрона", 2, 1)
        SUBELEM_START_CRN("", "", NULL, "U, I накала\vU, I выпрямителя\vU, I коллектора", 6, 2)
            RINGRF_KLS("heat", "накала",      "V",  "A",   0, 34),
            RINGRF_KLS("rect", "выпрямителя", "kV", "kA", 36, 33),
            RINGRF_KLS("coll", "коллектора",  "V",  "A",   0, 32),
        SUBELEM_END("notitle,noshadow,nocoltitles", NULL),
        SUBELEM_START("", "", 3, 1)
            RINGRF_MES("settling",  "Токооседание",     "A",  45),
            RINGRF_MES("ivac_rznr", "Iвак. резонатора", "╣A", 60),
            RINGRF_MES("ivac_coll", "Iвак. коллектора", "╣A", 61),
        SUBELEM_END("notitle,noshadow,nocoltitles", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 0),

    GLOBELEM_START("klst", "ВЧ клистрона", 6, 1)
        RINGRF_MES("", "Pвых",    "kW",  0),
        RINGRF_MES("", "Pотр",    "kW",  0),
        RINGRF_MES("", "S",       "",    0),
        RINGRF_TRM("", "T",       "╟C",  0),
        RINGRF_MES("", "di/ik",   "%",   0),
        RINGRF_MES("", "Pпуч",    "kW",  0),
    GLOBELEM_END("nocoltitles", 0),

    GLOBELEM_START("circ", "ВЧ циркулятора", 6, 1)
        RINGRF_MES("", "Pпад",    "kW",  0),
        RINGRF_MES("", "Pотр",    "kW",  0),
        RINGRF_MES("", "S",       "",    0),
        RINGRF_TRM("", "T",       "╟C",  1),
        RINGRF_MES("", "dTсц",    "╟C",  0),
        RINGRF_MES("", "Pвч",     "kW",  0),
    GLOBELEM_END("nocoltitles", 0),

    GLOBELEM_START("wave", "ВЧ волновода", 6, 1)
        RINGRF_MES("", "Pпад",    "kW",  0),
        RINGRF_MES("", "Pотр",    "kW",  0),
        RINGRF_MES("", "S",       "",    0),
        RINGRF_TRM("", "TцН",     "╟C",  2),
        RINGRF_MES("", "dTцН",    "╟C",  0),
        RINGRF_MES("", "Частота", "MHz", 0),
    GLOBELEM_END("nocoltitles", 0),

    GLOBELEM_START("load", "ВЧ нагрузки", 6, 1)
        RINGRF_MES("", "Pпад",    "kW",  0),
        RINGRF_MES("", "Pотр",    "kW",  0),
        RINGRF_MES("", "S",       "",    0),
        RINGRF_TRM("", "T",       "╟C",  3),
        RINGRF_MES("", "dTЭ",     "╟C",  0),
        RINGRF_MES("", "КПД",     "%",   0),
    GLOBELEM_END("nocoltitles", 0),

    GLOBELEM_START("rznr", "ВЧ резонатора", 6, 1)
        RINGRF_MES("", "Pпад",    "kW",  0),
        RINGRF_MES("", "Pотр",    "kW",  0),
        RINGRF_MES("", "S",       "",    0),
        RINGRF_TRM("", "T",       "╟C",  4),
        RINGRF_MES("", "dTВ",     "╟C",  0),
        EMPTY_CELL(),
    GLOBELEM_END("nocoltitles", 0),

    GLOBELEM_START("histplot", "Histplot", 1, 1)
        {"histplot", "HistPlot", NULL, NULL,
            "width=700,height=290"
                ",add=.i1set,add=.i2set,add=.i3set,add=.i4set,add=.i5set"
                ",add=.i_coll,add=.i_rect,add=.i_heat,add=.u_rect,add=.settling,add=.ivac_rznr,add=.ivac_coll",
         NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
    GLOBELEM_END("notitle,nocoltitles,norowtitles", 1),

    {NULL}
};


static physprops_t ringrf_physinfo[] =
{
};
