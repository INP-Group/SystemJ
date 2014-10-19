#include "drv_i/tvac320_drv_i.h"

#include "common_elem_macros.h"


static groupunit_t tvac320_grouping[] =
{

#define  TVAC320_BASE     0
#define  TVAC320_IDENT    "tvac320"
#define  TVAC320_LABEL    "TVAC320"

    {
        &(elemnet_t){
            TVAC320_IDENT, TVAC320_LABEL,
            "", "",
            ELEM_MULTICOL, 5, 1,
            (logchannet_t []){
                SUBELEM_START("ctl", NULL, 3, 3)
                    RGSWCH_LINE("tvr", NULL, "Выкл", "Вкл", (TVAC320_BASE) + TVAC320_CHAN_STAT_base, (TVAC320_BASE) + TVAC320_CHAN_TVR_OFF, (TVAC320_BASE) + TVAC320_CHAN_TVR_ON),
                    HLABEL_CELL(TVAC320_LABEL),
                    {NULL, "Сбр.блк", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]){
                            CMD_RET_I(0)
                        },
                        (excmd_t[]){
                            CMD_QRYVAL, CMD_SETP_I((TVAC320_BASE) + TVAC320_CHAN_RST_ILK),
                            CMD_RET
                        },
                        .placement="horz=right"
                    },
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
                SEP_L(),
                SUBELEM_START("vals", NULL, 4, 4)
                    {"set", NULL, "V",  "%5.1f", "withunits",           NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((TVAC320_BASE) + TVAC320_CHAN_SET_U),     .minnorm=0, .maxnorm=300},
                    {"mes", NULL, NULL, "%5.1f", NULL,                  NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((TVAC320_BASE) + TVAC320_CHAN_MES_U)},
                    {"min", "[",  NULL, "%5.1f", "withlabel",           NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((TVAC320_BASE) + TVAC320_CHAN_SET_U_MIN), .minnorm=0, .maxnorm=300},
                    {"max", "-",  "]",  "%5.1f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((TVAC320_BASE) + TVAC320_CHAN_SET_U_MAX), .minnorm=0, .maxnorm=300},
                SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                SEP_L(),
                SUBELEM_START("bits", NULL, 3, 3)
                    SUBELEM_START("stat", "Состояние", 8, 1)
#define TVAC320_STAT_LINE(n, l) \
    {"s"__CX_STRINGIZE(n), l, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_BLUELED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((TVAC320_BASE) + TVAC320_CHAN_STAT_base + n)}
                        TVAC320_STAT_LINE(0, "0 ЗУ вкл"),
                        TVAC320_STAT_LINE(1, "1 ТРН вкл"),
                        TVAC320_STAT_LINE(2, "2 стаб. фазы ТРН"),
                        TVAC320_STAT_LINE(3, "3 стаб. U ТРН"),
                        TVAC320_STAT_LINE(4, "4 разряд накоп."),
                        TVAC320_STAT_LINE(5, "5"),
                        TVAC320_STAT_LINE(6, "6"),
                        TVAC320_STAT_LINE(7, "7 ЗУ неисправно"),
                    SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),
                    VSEP_L(),
                    SUBELEM_START("ilks", "Блокировки", 16, 2)
#define TVAC320_ILK_LINE(n, l) \
    {"m"__CX_STRINGIZE(n), l,    NULL, NULL, "",             NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((TVAC320_BASE) + TVAC320_CHAN_LKM_base + n)}, \
    {"l"__CX_STRINGIZE(n), NULL, NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID((TVAC320_BASE) + TVAC320_CHAN_ILK_base + n)}
                        TVAC320_ILK_LINE(0, "0 превышение Iзар"),
                        TVAC320_ILK_LINE(1, "1 превышение Uраб"),
                        TVAC320_ILK_LINE(2, "2 полн.разряд накоп."),
                        TVAC320_ILK_LINE(3, "3"),
                        TVAC320_ILK_LINE(4, "4"),
                        TVAC320_ILK_LINE(5, "5"),
                        TVAC320_ILK_LINE(6, "6"),
                        TVAC320_ILK_LINE(7, "7 ЗУ неисправно"),
                    SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
            },
            "notitle,nocoltitles,norowtitles"
        }
    }

,
    {NULL}
};

static physprops_t tvac320_physinfo[] =
{
#define  TVAC320_BASE     0
#include "pi_tvac320.h"
};
