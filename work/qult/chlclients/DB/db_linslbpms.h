#include "common_elem_macros.h"

#include "drv_i/slbpm_drv_i.h"


#define ONE_BASE(n) (0 + SLBPM_NUMCHANS*(n))
#define BIG_BASE(n) (0 + SLBPM_NUMBIGCS*(n))

#define ONE_BIGC(n, id, s, b) \
    {"b"#n"b"id, NULL, NULL, "%5.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, BIG_BASE(n) + b, PHYS_ID(ONE_BASE(n) + s) }
#define ONE_LINE(n, l) \
    HLABEL_CELL(l),    \
    {"b"#n"plrty",   NULL, NULL, NULL,    "#T+\v-", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_EMINUS)}, \
    {"b"#n"ampl",    NULL, NULL, NULL,    "#T=\v*", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_AMPLON)}, \
    {"b"#n"delayA",  NULL, NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_DELAYA), .minnorm=0, .maxnorm=10.23}, \
    {"b"#n"delayX1", NULL, NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_DELAY0), .minnorm=0, .maxnorm=10.23}, \
    {"b"#n"delayX2", NULL, NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_DELAY1), .minnorm=0, .maxnorm=10.23}, \
    {"b"#n"delayY1", NULL, NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_DELAY2), .minnorm=0, .maxnorm=10.23}, \
    {"b"#n"delayY2", NULL, NULL, "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_DELAY3), .minnorm=0, .maxnorm=10.23}, \
    {"b"#n"border",  NULL, NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_BORDER)}, \
    {"b"#n"mmxX1",   NULL, NULL, "%5.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_MMX0)},   \
    {"b"#n"mmxX2",   NULL, NULL, "%5.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_MMX1)},   \
    {"b"#n"mmxY1",   NULL, NULL, "%5.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_MMX2)},   \
    {"b"#n"mmxY2",   NULL, NULL, "%5.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_MMX3)},   \
    {"b"#n"nav",     NULL, NULL, "%2.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ONE_BASE(n) + SLBPM_CHAN_NAV)},    \
    HLABEL_CELL("   "),                                     \
    ONE_BIGC(n, "mmxX1", SLBPM_CHAN_MMX0, SLBPM_BIGC_BUF0), \
    ONE_BIGC(n, "mmxX2", SLBPM_CHAN_MMX1, SLBPM_BIGC_BUF1), \
    ONE_BIGC(n, "mmxY1", SLBPM_CHAN_MMX2, SLBPM_BIGC_BUF2), \
    ONE_BIGC(n, "mmxY2", SLBPM_CHAN_MMX3, SLBPM_BIGC_BUF3)


static groupunit_t linslbpms_grouping[] =
{
    GLOBELEM_START_CRN("ctl", NULL,
                       " \v+/-\vAmpl\vDlyAll, ns\vDlyX1, ns\vDlyX2, ns\vDlyY1, ns\vDlyY2, ns\vBorder\vX1\vX2\vY1\vY2\vNav",
                       NULL, 14*19, 19)
        ONE_LINE(0,  "1"),
        ONE_LINE(1,  "2"),
        ONE_LINE(2,  "3"),
        ONE_LINE(3,  "4"),
        ONE_LINE(4,  "5"),
        ONE_LINE(5,  "6:Tech"),
        ONE_LINE(6,  "7:Trip"),
        ONE_LINE(7,  "8"),
        ONE_LINE(8,  "9"),
        ONE_LINE(9,  "10"),
        ONE_LINE(10, "11"),
        ONE_LINE(11, "12"),
        ONE_LINE(12, "13"),
        ONE_LINE(13, "14"),
    GLOBELEM_END("noshadow,notitle,norowtitles", 0),

    {NULL}
};

#define ONE_PHYSINFO(n)             \
    {ONE_BASE(n) + SLBPM_CHAN_DELAYA, 100, 0, 0}, \
    {ONE_BASE(n) + SLBPM_CHAN_DELAY0, 100, 0, 0}, \
    {ONE_BASE(n) + SLBPM_CHAN_DELAY1, 100, 0, 0}, \
    {ONE_BASE(n) + SLBPM_CHAN_DELAY2, 100, 0, 0}, \
    {ONE_BASE(n) + SLBPM_CHAN_DELAY3, 100, 0, 0}

static physprops_t linslbpms_physinfo[] =
{
    ONE_PHYSINFO(0),
    ONE_PHYSINFO(1),
    ONE_PHYSINFO(2),
    ONE_PHYSINFO(3),
    ONE_PHYSINFO(4),
    ONE_PHYSINFO(5),
    ONE_PHYSINFO(6),
    ONE_PHYSINFO(7),
    ONE_PHYSINFO(8),
    ONE_PHYSINFO(9),
    ONE_PHYSINFO(10),
    ONE_PHYSINFO(11),
    ONE_PHYSINFO(12),
};
