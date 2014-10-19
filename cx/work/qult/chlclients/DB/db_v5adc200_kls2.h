#define SUBSYS_BASENAME v5adc200_kls2
#define SUBSYS_ADC200_N 1
#define SUBSYS_SELECTOR                      \
    SUBELEM_START("selector", NULL, 3, 3)          \
        {"ch0", "Ch0", NULL, NULL,           \
         "#T4с нагр\f\fcolor=blue\v5с нагр\v6с нагр\v6с отр", \
         NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, \
         PHYS_ID(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0) \
        },                                   \
        VSEP_L(), \
        {"ch1", "Ch1", NULL, NULL,           \
         "#T4с пад\f\fcolor=red\v5с пад\v6с пад\v6с отр", \
         NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, \
         PHYS_ID(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1) \
        },                                   \
        EMPTY_CELL(), \
    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", "left=0,bottom=0")
#include "v5adc200_common.h"
