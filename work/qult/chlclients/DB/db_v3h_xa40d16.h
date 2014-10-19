#include "drv_i/v3h_xa40d16_drv_i.h"

#include "common_elem_macros.h"


static groupunit_t v3h_xa40d16_grouping[] =
{
    GLOBELEM_START("ctl", "Control", 7, 5 + (2<<24))
        {"v1k_state", "I_S",      NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_V3H_STATE)},
        {"rst",       "R",        NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_RESET_STATE)},
        {"dac",    "Set, A",      NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_OUT),      .minnorm=  0.0, .maxnorm=+1800.0},
        {"dacspd", "MaxSpd, A/s", NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_OUT_RATE), .minnorm=-20.0*250, .maxnorm=+20.0*250},
        {"daccur", "Cur, A",      NULL, "%7.1f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_OUT_CUR)},
        {"opr",    NULL,          NULL, NULL,    NULL, NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_OPR)},
        RGSWCH_LINE("onoff", NULL, "Off", "On", V3H_XA40D16_CHAN_IS_ON, V3H_XA40D16_CHAN_SWITCH_OFF, V3H_XA40D16_CHAN_SWITCH_ON),
    GLOBELEM_END("norowtitles", 0),
    GLOBELEM_START("mes", "Measurements", 5+1, 1 + (1<<24))
        REDLED_LINE("ilk", "Interlock",    V3H_XA40D16_CHAN_ILK),
        {"dms", "DAC control",                 "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_DMS)},
        {"sts", "Stabilisation shunt (DCCT2)", "V", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_STS)},
        {"mes", "Measurement shunt (DCCT1)",   "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_MES)},
        {"u1",  "U1",                          "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_MU1)},
        {"u2",  "U2",                          "A", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_MU2)},
    GLOBELEM_END("nocoltitles", GU_FLAG_FROMNEWLINE),
#if 0
    GLOBELEM_START("ilk", "Interlocks", 2, 1)
        {"rst", "Reset", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V3H_XA40D16_CHAN_RST_ILKS)},
        SUBELEM_START(":", NULL, 8, 2)

#define ILK_LINE(id, l, c_suf) \
            REDLED_LINE(id,   l,    __CX_CONCATENATE(V3H_XA40D16_CHAN_ILK_,   c_suf)), \
            REDLED_LINE(NULL, NULL, __CX_CONCATENATE(V3H_XA40D16_CHAN_C_ILK_, c_suf))

            ILK_LINE("overheat", "Overheating source",  OVERHEAT),
            ILK_LINE("emergsht", "Emergency shutdown",  EMERGSHT),
            ILK_LINE("currprot", "Current protection",  CURRPROT),
            ILK_LINE("loadovrh", "Load overheat/water", LOADOVRH),
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 0),
#endif

    {NULL}
};


static physprops_t v3h_xa40d16_physinfo[] =
{
    {V3H_XA40D16_CHAN_DMS, 1000000.0/125, 0, 0},
    {V3H_XA40D16_CHAN_STS, 1000000.0/125, 0, 0},
    {V3H_XA40D16_CHAN_MES, 1000000.0/125, 0, 0},
    {V3H_XA40D16_CHAN_MU1, 1000000.0/125, 0, 0},
    {V3H_XA40D16_CHAN_MU2, 1000000.0/125, 0, 0},

    {V3H_XA40D16_CHAN_OUT,          1000000.0/125, 0, 305},
    {V3H_XA40D16_CHAN_OUT_RATE,     1000000.0/125, 0, 305},
    {V3H_XA40D16_CHAN_OUT_CUR,      1000000.0/125, 0, 305},
};
