#include "common_elem_macros.h"
#include "pzframeclient.h"

#include "drv_i/u0632_drv_i.h"


#define IPP_CELL(ndev, box, x) \
    {"l"__CX_STRINGIZE(x), NULL, NULL, "%5.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, \
     PHYS_ID(U0632_NUMCHANS * (ndev)      + \
             U0632_CHAN_INTDATA_BASE      + \
             U0632_CHANS_PER_UNIT * (box) + \
             x)                             \
    }

#define THE_BOX(ndev, box, nl)   \
    GLOBELEM_START("Box" #ndev "-" #box, "#" #ndev "." #box, 2+32 + 1, 1 + 8*65536 + (2 << 24)) \
        {"sens",  "S",  NULL, NULL, "#+\v#T400.00e3\v100.00e3\v25.00e3\v6.25e3", "Чувствительность", \
         LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(U0632_NUMCHANS*(ndev)+U0632_CHAN_M_BASE+(box))}, \
        {"delay", "T",  NULL, NULL, "#+\v#F32f,0.1088,0.0,%4.2fms", "Задержка перед измерением", \
         LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(U0632_NUMCHANS*(ndev)+U0632_CHAN_P_BASE+(box))}, \
        IPP_CELL(ndev, box, 0),  \
        IPP_CELL(ndev, box, 1),  \
        IPP_CELL(ndev, box, 2),  \
        IPP_CELL(ndev, box, 3),  \
        IPP_CELL(ndev, box, 4),  \
        IPP_CELL(ndev, box, 5),  \
        IPP_CELL(ndev, box, 6),  \
        IPP_CELL(ndev, box, 7),  \
        IPP_CELL(ndev, box, 8),  \
        IPP_CELL(ndev, box, 9),  \
        IPP_CELL(ndev, box, 10), \
        IPP_CELL(ndev, box, 11), \
        IPP_CELL(ndev, box, 12), \
        IPP_CELL(ndev, box, 13), \
        IPP_CELL(ndev, box, 14), \
        IPP_CELL(ndev, box, 15), \
        IPP_CELL(ndev, box, 16), \
        IPP_CELL(ndev, box, 17), \
        IPP_CELL(ndev, box, 18), \
        IPP_CELL(ndev, box, 19), \
        IPP_CELL(ndev, box, 20), \
        IPP_CELL(ndev, box, 21), \
        IPP_CELL(ndev, box, 22), \
        IPP_CELL(ndev, box, 23), \
        IPP_CELL(ndev, box, 24), \
        IPP_CELL(ndev, box, 25), \
        IPP_CELL(ndev, box, 26), \
        IPP_CELL(ndev, box, 27), \
        IPP_CELL(ndev, box, 28), \
        IPP_CELL(ndev, box, 29), \
        IPP_CELL(ndev, box, 30), \
        IPP_CELL(ndev, box, 31), \
        {"b", NULL, NULL, NULL, "", \
         NULL, LOGT_READ, LOGD_U0632, LOGK_DIRECT, \
         /*!!! A hack: we use 'color' field to pass integer */ \
         U0632_NUM_BIGCS * (ndev) + (box), PHYS_ID(0)}, \
    GLOBELEM_END("nocoltitles,norowtitles,fold_h", nl)


static groupunit_t ylinipppanel_grouping[] =
{
    THE_BOX(1, 1,  1),
    THE_BOX(1, 14, 0),
    THE_BOX(1, 8,  0),
    THE_BOX(0, 4,  1),
    THE_BOX(0, 7,  0),
    THE_BOX(0, 3,  0),
    THE_BOX(0, 0,  1),
    THE_BOX(1, 13, 0),
    THE_BOX(1, 2,  0),
    {NULL}
};

static physprops_t ylinipppanel_physinfo[] =
{
};
