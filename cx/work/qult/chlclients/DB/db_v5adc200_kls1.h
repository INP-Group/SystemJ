#define SUBSYS_BASENAME v5adc200_kls1
#define SUBSYS_ADC200_N 0
#define SUBSYS_SELECTOR                      \
    SUBELEM_START("selector", NULL, 3, 3)          \
        {"ch0", "Ch0", NULL, NULL,           \
         "#T7с пад\f\fcolor=blue\v8с пад\v9с пад\v10с пад", \
         NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, \
         PHYS_ID(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0) \
        },                                   \
        VSEP_L(), \
        {"ch1", "Ch1", NULL, NULL,           \
         "#T7с нагр\f\fcolor=red\v8с нагр\v9с нагр\v10с нагр", \
         NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, \
         PHYS_ID(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1) \
        },                                   \
        EMPTY_CELL(), \
    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", "left=0,bottom=0")
#include "v5adc200_common.h"
