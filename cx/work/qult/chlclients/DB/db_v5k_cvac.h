#include "common_elem_macros.h"

#include "drv_i/xceac124_drv_i.h"

enum
{
    XCEACS_BASE = 0,
};

#define C_IMES(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n) * 2 + 0 + XCEAC124_CHAN_ADC_n_BASE)
#define C_UMES(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n) * 2 + 1 + XCEAC124_CHAN_ADC_n_BASE)
#define C_U5_7(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n)         + XCEAC124_CHAN_OUT_n_BASE)
#define C_ISON(f, n) (XCEACS_BASE + (f) * XCEAC124_NUMCHANS + (n)         + XCEAC124_CHAN_REGS_WR8_BASE)

#define LINE_ID(p, f, n) p "-" __CX_STRINGIZE(f) "-" __CX_STRINGIZE(n)

#define NEWVAC_LINE(f, n, l, p, t) \
    {LINE_ID("KV57", f, n), l,    NULL, NULL,    "#L\v#T5\v7", t"\nНасос: "p, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(C_U5_7(f, n)),  \
        (excmd_t[]){                                     \
            CMD_PUSH_I(0), CMD_PUSH_I(0), CMD_PUSH_I(1), \
            CMD_GETP_I(C_U5_7(f, n)),                    \
            CMD_CASE,                                    \
            CMD_RET                                      \
        },                                               \
        (excmd_t[]){                                            \
            CMD_QRYVAL, CMD_MUL_I(7), CMD_SETP_I(C_U5_7(f, n)), \
            CMD_RET                                             \
        },                                                      \
    }, \
    {LINE_ID("IsOn", f, n), NULL, NULL, NULL,    NULL,     NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_ISON(f, n))}, \
    {LINE_ID("Imes", f, n), NULL, NULL, "%6.3f", NULL,     NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_IMES(f, n))}, \
    {LINE_ID("Umes", f, n), NULL, NULL, "%6.3f", NULL,     NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C_UMES(f, n))}

static groupunit_t v5k_cvac_grouping[] =
{
    {
        &(elemnet_t){
            "new", "New",
            "5/7kV\v \vI, uA\vU, kV", NULL,
            ELEM_MULTICOL, 4*1*1, 4 + 32*65536,
            (logchannet_t []){
                NEWVAC_LINE( 0, 0, "КK",     "???",       "Приёмник пучка"),
            },
            "noshadow,notitle"
        }, 0

    },

    GLOBELEM_START("histplot", "History plot", 1, 1)
        {"plot", "HistPlot", NULL, NULL, "width=700,height=290,add=.Imes-0-0,add=.Umes-0-0", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
    GLOBELEM_END("noshadow,notitle,nocoltitles,norowtitles,one", 
                 GU_FLAG_FROMNEWLINE | GU_FLAG_FILLHORZ | GU_FLAG_FILLVERT),

    {NULL},
};

#define PHYS_LINE(f,n)                          \
    {C_IMES(f,n),    1000 /* 1mA/V */, 0, 0},   \
    {C_UMES(f,n), 1000000 /* 1kV/V */, 0, 0},   \
    {C_U5_7(f,n), 1000000 /* */,       0, 305}, \
    {C_ISON(f,n),       1,             0, 0}

#define XCEAC124_PHYS_INFO(f) \
    PHYS_LINE(f, 0),         \
    PHYS_LINE(f, 1),         \
    PHYS_LINE(f, 2),         \
    PHYS_LINE(f, 3)

static physprops_t v5k_cvac_physinfo[] =
{
    XCEAC124_PHYS_INFO(0),
};
