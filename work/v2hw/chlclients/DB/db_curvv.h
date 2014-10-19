
#include "drv_i/curvv_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t curvv_grouping[] =
{

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_REGS_WR8_BASE+n)}

#define INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_REGS_RD8_BASE+n)}

#define PWRB_LINE(n) \
    {"pwrb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_PWR_Bn_BASE+n)}

#define TTLB_LINE(n) \
    {"ttlb" __CX_STRINGIZE(n), "", NULL, NULL, "", "#"__CX_STRINGIZE(n), LOGT_READ,   LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_TTL_Bn_BASE+n)}

    {
        &(elemnet_t){
            "ioregs", "I/O", "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR\vPwrR\vTTL",
            ELEM_MULTICOL, 9*6, 9,
            (logchannet_t []){
                {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_REGS_WR1)},
                OUTB_LINE(7),
                OUTB_LINE(6),
                OUTB_LINE(5),
                OUTB_LINE(4),
                OUTB_LINE(3),
                OUTB_LINE(2),
                OUTB_LINE(1),
                OUTB_LINE(0),
                {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_REGS_RD1)},
                INPB_LINE(7),
                INPB_LINE(6),
                INPB_LINE(5),
                INPB_LINE(4),
                INPB_LINE(3),
                INPB_LINE(2),
                INPB_LINE(1),
                INPB_LINE(0),
                {"pwr8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_PWR_8B)},
                PWRB_LINE(7),
                PWRB_LINE(6),
                PWRB_LINE(5),
                PWRB_LINE(4),
                PWRB_LINE(3),
                PWRB_LINE(2),
                PWRB_LINE(1),
                PWRB_LINE(0),
                {"ttl24", "24b", NULL, "%8.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CURVV_CHAN_TTL_24B)},
                TTLB_LINE(7),
                TTLB_LINE(6),
                TTLB_LINE(5),
                TTLB_LINE(4),
                TTLB_LINE(3),
                TTLB_LINE(2),
                TTLB_LINE(1),
                TTLB_LINE(0),
                EMPTY_CELL(),
                TTLB_LINE(15),
                TTLB_LINE(14),
                TTLB_LINE(13),
                TTLB_LINE(12),
                TTLB_LINE(11),
                TTLB_LINE(10),
                TTLB_LINE(9),
                TTLB_LINE(8),
                EMPTY_CELL(),
                TTLB_LINE(23),
                TTLB_LINE(22),
                TTLB_LINE(21),
                TTLB_LINE(20),
                TTLB_LINE(19),
                TTLB_LINE(18),
                TTLB_LINE(17),
                TTLB_LINE(16),
            },
            ""
        }, 1
    },

    {NULL}
};


static physprops_t curvv_physinfo[] =
{
};
