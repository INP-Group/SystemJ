#define SUBSYS_BASENAME v5adc200_kls3
#define SUBSYS_ADC200_N 4
#define SUBSYS_SELECTOR                      \
    SUBELEM_START("selector", NULL, 3, 3)          \
        {"ch0", "Ch0", NULL, NULL,           \
         "#T�����\f\fcolor=blue\v1� ���\v1� ����\v3� ����", \
         NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, \
         PHYS_ID(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0) \
        },                                   \
        VSEP_L(), \
        {"ch1", "Ch1", NULL, NULL,           \
         "#T3� ���\f\fcolor=red\v2� ����\v2� ���\v2� ���", \
         NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, \
         PHYS_ID(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1) \
        },                                   \
        EMPTY_CELL(), \
    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", "left=0,bottom=0")
#include "v5adc200_common.h"
