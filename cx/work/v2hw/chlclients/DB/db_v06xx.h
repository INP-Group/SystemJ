
#include "drv_i/v06xx_drv_i.h"
#include "common_elem_macros.h"


#define V06XX_BIT(n) \
    {"b"__CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, NULL, NULL, NULL, \
     LOGT_WRITE1, LOGD_BLUELED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V06XX_CHAN_WR_N_BASE + n)}

static groupunit_t v06xx_grouping[] =
{
    GLOBELEM_START("dev", "V06xx individual bits",
                   8 * 3 + 1, 8 + (1 << 24))
        {"word24", "24-bit", NULL, "%8.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V06XX_CHAN_WR1)},
        V06XX_BIT(7),
        V06XX_BIT(6),
        V06XX_BIT(5),
        V06XX_BIT(4),
        V06XX_BIT(3),
        V06XX_BIT(2),
        V06XX_BIT(1),
        V06XX_BIT(0),
        V06XX_BIT(15),
        V06XX_BIT(14),
        V06XX_BIT(13),
        V06XX_BIT(12),
        V06XX_BIT(11),
        V06XX_BIT(10),
        V06XX_BIT(9),
        V06XX_BIT(8),
        V06XX_BIT(23),
        V06XX_BIT(22),
        V06XX_BIT(21),
        V06XX_BIT(20),
        V06XX_BIT(19),
        V06XX_BIT(18),
        V06XX_BIT(17),
        V06XX_BIT(16),
    GLOBELEM_END("noshadow,nocoltitles,norowtitles", 0),

    {NULL}
};


static physprops_t v06xx_physinfo[] =
{
};
