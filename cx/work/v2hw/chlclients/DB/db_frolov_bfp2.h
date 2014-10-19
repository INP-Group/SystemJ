
#include "drv_i/frolov_bfp2_drv_i.h"
#include "common_elem_macros.h"


#define BFP2NUM_LINE(id, label, cn) \
    {id, label, NULL, "%5.0f", "nounits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .maxnorm=65535}
    
groupunit_t frolov_bfp2_grouping[] =
{
    GLOBELEM_START("frolov_bfp2", "BFP2", 11, 1)
        RGLED_LINE ("ubs",  "\nБлок. УБС", FROLOV_BFP2_CHAN_STAT_UBS),
        RGSWCH_LINE("work", "Работа",    "Выкл", "Вкл",
                    FROLOV_BFP2_CHAN_STAT_WKALWD,
                    FROLOV_BFP2_CHAN_WRK_DISABLE,
                    FROLOV_BFP2_CHAN_WRK_ENABLE),
        RGSWCH_LINE("modl", "Модуляция", "Выкл", "Вкл",
                    FROLOV_BFP2_CHAN_STAT_MOD,
                    FROLOV_BFP2_CHAN_MOD_DISABLE,
                    FROLOV_BFP2_CHAN_MOD_ENABLE),
        {"ktime", "Ktime", NULL, NULL, "#L\v#T2.5 ms\v5 ms\v10 ms\v20 ms", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BFP2_CHAN_KTIME)},
        BFP2NUM_LINE("max",  "Tmax, nn", FROLOV_BFP2_CHAN_KMAX),
        BFP2NUM_LINE("min",  "Tmin, nn", FROLOV_BFP2_CHAN_KMIN),
        {"mode", "Mode", NULL, NULL, "#L\v#TCont\vLoop", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BFP2_CHAN_LOOPMODE)},
        {"prds", "# periods", NULL, "%5.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BFP2_CHAN_NUMPERIODS), .maxnorm=65535},
        {"left", "# left",    NULL, "%5.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FROLOV_BFP2_CHAN_STAT_CURPRD), .maxnorm=65535},
        BUTT_CELL("start", "Start", FROLOV_BFP2_CHAN_LOOP_START),
        BUTT_CELL("stop",  "Stop",  FROLOV_BFP2_CHAN_LOOP_STOP),
    GLOBELEM_END("notitle,noshadow,nocoltitles", 1),

    {NULL}
};

physprops_t frolov_bfp2_physinfo[] =
{
};
