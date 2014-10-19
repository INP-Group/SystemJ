#include "drv_i/cdac20_drv_i.h"
#include "tantalclient.h"


enum
{
    B_TANTAL = 0,

    C_TANTAL_ISET = B_TANTAL + CDAC20_CHAN_WRITE,

    C_TANTAL_MES1 = B_TANTAL + CDAC20_CHAN_READ_N_BASE + 0,
    C_TANTAL_MES2 = B_TANTAL + CDAC20_CHAN_READ_N_BASE + 1,
    C_TANTAL_MES3 = B_TANTAL + CDAC20_CHAN_READ_N_BASE + 2,
    C_TANTAL_MES4 = B_TANTAL + CDAC20_CHAN_READ_N_BASE + 3,
    C_TANTAL_MES5 = B_TANTAL + CDAC20_CHAN_READ_N_BASE + 4,
};

#define TANTAL_LCLREG_REF(rn, defval, rfrsh)           \
    PHYS_ID(-1),                                       \
    (excmd_t []){                                      \
        CMD_PUSH_I(defval), CMD_SETLCLREGDEFVAL_I(rn), \
        CMD_GETLCLREG_I(rn), CMD_RET                   \
    },                                                 \
    (excmd_t []){                                      \
        CMD_QRYVAL, CMD_SETLCLREG_I(rn),               \
        CMD_REFRESH_I(rfrsh), CMD_RET                  \
    }


static groupunit_t tantal_grouping[] =
{
    {
        &(elemnet_t){
            "main", "Application controls",
            NULL,
            NULL,
            ELEM_MULTICOL, 4, 2,
            (logchannet_t []){
                {"-", "Mode", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "Mode",
                        "?\v?\v?\v?\v?\v \v ",
                        NULL,
                        ELEM_MULTICOL, 1*0, 1,
                        (logchannet_t []){
                            {"mode", "Режим", NULL, NULL, "#C#TАвто\vРучн", "", LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                                (excmd_t []){
                                    CMD_GETLCLREG_I(REG_TANTAL_MANUAL), CMD_RET,
                                },
                                (excmd_t []){
                                    CMD_QRYVAL, CMD_SETLCLREG_I(REG_TANTAL_MANUAL),
                                    CMD_REFRESH_I(1), CMD_RET
                                }, .placement="horz=right"},
                        },
                        "noshadow,notitle,norowtitles"
                    },
                .placement="horz=right"},
                {"-", "Mode", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "-", "Mode",
                        "?\v?\v?\v?\v \v ",
                        NULL,
                        ELEM_MULTICOL, 10, 5,
                        (logchannet_t []){
                            {"from",    "От (A)",    "A", TANTAL_I_FMT, NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_FROM_I,    20,  0),  .minnorm=0, .maxnorm=100},
                            {"to",      "До (A)",    "A", TANTAL_I_FMT, NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_TO_I,      30,  0),  .minnorm=0, .maxnorm=100},
                            {"nsteps",  "Шагов",     "",  "%4.0f",      NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_NSTEPS,    10,  0),  .minnorm=1, .maxnorm=TANTAL_MAXPLOTSIZE},
                            {"time",    "сек/шаг",   "s", "%4.0f",      NULL, NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_STEP_TIME, 2,   0), .minnorm=1, .maxnorm=9999},
                            {"go",      "Пуск!",     "",  NULL,         NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_C_GO,      0,  1)},
                            {"-", "",   NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP},
                            {"-", "",   NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP},
                            {"curstep", "Шаг",      "",  "%-4.0f",      NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_CUR_STEP,  0, 0)},
                            {"curtime", "Тек.Сек.", "s", "%-4.0f",      NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_CUR_TIME,  0, 0)},
                            {"stop",    "STOP",      "",  NULL,         NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, TANTAL_LCLREG_REF(REG_TANTAL_C_STOP,    0, 1)},
                        },
                        "noshadow,notitle,norowtitles"
                    }
                },
                {"vals", "Числа", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "vals", "Числа",
                        NULL,
                        "?\v?\v?\v?\v?\v?",
                        ELEM_MULTICOL, 6, 1,
                        (logchannet_t []){
                            {"iset",  "Iset (A)",  "A", TANTAL_I_FMT, NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_TANTAL_ISET), .minnorm=0, .maxnorm=100},
                            {"mes1", "I/шунт",     "A", TANTAL_I_FMT, NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_TANTAL_MES1)},
                            {"mes2", "I нагрузки", "A", TANTAL_I_FMT, NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_TANTAL_MES2)},
                            {"mes3", "=ЦАП?",      "V", TANTAL_I_FMT, NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_TANTAL_MES3)},
                            {"mes4", "U вых 1",    "V", TANTAL_I_FMT, NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_TANTAL_MES4)},
                            {"mes5", "U вых 2",    "V", TANTAL_I_FMT, NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_TANTAL_MES5)},
                        },
                        "noshadow,notitle,nocoltitles"
                    }
                },
                {"plots", "Графики", NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "plots", "Графики",
                        NULL,
                        NULL,
                        ELEM_MULTICOL, 1, 1,
                        (logchannet_t []){
                            {"plots", "Place for plots...", NULL, NULL, "color=white width=600,height=400 border=1 textpos=0c0m fontsize=huge fontmod=bolditalic", NULL, LOGT_READ, LOGD_TANTAL_PLOT, LOGK_NOP},
                        },
                        "noshadow,notitle,nocoltitles,norowtitles"
                    }
                },
            },
            "noshadow,notitle,nocoltitles,norowtitles,fold_h"
        }, 1
    },
    
    
    {NULL}
};



static physprops_t tantal_physinfo[] =
{
    {C_TANTAL_ISET, (10000000/10.0)*(8.0/300), 0.0, 10},
    {C_TANTAL_MES1, (10000000/10.0)*(1.0/10),  0.0, 0},
    {C_TANTAL_MES2, (10000000/10.0)*(8.0/300), 0.0, 0},
    {C_TANTAL_MES3, (10000000/10.0)*1, 0.0, 0},
    {C_TANTAL_MES4, (10000000/10.0)*(1.0/2), 0.0, 0},
    {C_TANTAL_MES5, (10000000/10.0)*(1.0/2), 0.0, 0},
};
