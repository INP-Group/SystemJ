
#include "drv_i/v0628_drv_i.h"
#include "common_elem_macros.h"


#define V0628_OUT(n) \
    {"b"__CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, NULL, NULL, NULL, \
     LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0628_CHAN_WR_N_BASE + n)}

#define V0628_INP(n) \
    {"b"__CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, NULL, NULL, NULL, \
     LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0628_CHAN_RD_N_BASE + n)}

static groupunit_t v0628_grouping[] =
{
    GLOBELEM_START("out", "V0628 output register",
                   8 * 2 + 1, 8 + (1 << 24))
        {"word24", "24-bit", NULL, "%8.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0628_CHAN_FAKE_WR1)},
        V0628_OUT(7),
        V0628_OUT(6),
        V0628_OUT(5),
        V0628_OUT(4),
        V0628_OUT(3),
        V0628_OUT(2),
        V0628_OUT(1),
        V0628_OUT(0),
        V0628_OUT(15),
        V0628_OUT(14),
        V0628_OUT(13),
        V0628_OUT(12),
        V0628_OUT(11),
        V0628_OUT(10),
        V0628_OUT(9),
        V0628_OUT(8),
    GLOBELEM_END("noshadow,nocoltitles,norowtitles", 0),

    GLOBELEM_START("inp", "V0628 input register",
                   8 * 4 + 1, 8 + (1 << 24))
        {"word24", "32-bit", NULL, "%10.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0628_CHAN_RD1)},
        V0628_INP(7),
        V0628_INP(6),
        V0628_INP(5),
        V0628_INP(4),
        V0628_INP(3),
        V0628_INP(2),
        V0628_INP(1),
        V0628_INP(0),
        V0628_INP(15),
        V0628_INP(14),
        V0628_INP(13),
        V0628_INP(12),
        V0628_INP(11),
        V0628_INP(10),
        V0628_INP(9),
        V0628_INP(8),
        V0628_INP(23),
        V0628_INP(22),
        V0628_INP(21),
        V0628_INP(20),
        V0628_INP(19),
        V0628_INP(18),
        V0628_INP(17),
        V0628_INP(16),
        V0628_INP(31),
        V0628_INP(30),
        V0628_INP(29),
        V0628_INP(28),
        V0628_INP(27),
        V0628_INP(26),
        V0628_INP(25),
        V0628_INP(24),
    GLOBELEM_END("noshadow,nocoltitles,norowtitles", GU_FLAG_FROMNEWLINE),

    {NULL}
};


static physprops_t v0628_physinfo[] =
{
};
