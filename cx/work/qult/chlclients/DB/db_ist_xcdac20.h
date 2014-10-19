#include "drv_i/ist_xcdac20_drv_i.h"

#include "common_elem_macros.h"


static groupunit_t ist_xcdac20_grouping[] =
{
    GLOBELEM_START("ctl", "Control", 7, 5 + (2<<24))
        {"ist_state", "I_S",      NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_IST_STATE)},
        {"rst",       "R",        NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_RESET_STATE)},
        {"dac",    "Set, A",      NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_OUT),      .minnorm=  0.0, .maxnorm=+1800.0},
        {"dacspd", "MaxSpd, A/s", NULL, "%7.1f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_OUT_RATE), .minnorm=-20.0*250, .maxnorm=+20.0*250},
        {"daccur", "Cur, A",      NULL, "%7.1f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_OUT_CUR)},
        {"opr",    NULL,          NULL, NULL,    NULL, NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_OPR)},
        RGSWCH_LINE("onoff", NULL, "Off", "On", IST_XCDAC20_CHAN_IS_ON, IST_XCDAC20_CHAN_SWITCH_OFF, IST_XCDAC20_CHAN_SWITCH_ON),
    GLOBELEM_END("norowtitles", 0),
    GLOBELEM_START("mes", "Measurements", 8, 1)
        {"dcct1",  "DCCT1",   "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_DCCT1)},
        {"dcct2",  "DCCT2",   "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_DCCT2)},
        {"dacmes", "DAC",     "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_DAC)},
        {"u2",     "U2",      "V", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_U2)},
        {"adc4",   "ADC4",    "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_ADC4)},
        {"adcdac", "DAC@ADC", "A", "%9.1f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_ADC_DAC)},
        {"0v",     "0V",      "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_ADC_0V)},
        {"10v",    "10V",     "V", "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_ADC_P10V)},
    GLOBELEM_END("nocoltitles", GU_FLAG_FROMNEWLINE),
    GLOBELEM_START("ilk", "Interlocks", 16, 2)
        {"rst", "Reset", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_RST_ILKS)},
        {"rci", "R",     NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_RESET_C_ILKS)},
#define ILK_LINE(id, l, c_suf) \
        REDLED_LINE(id,   l,    __CX_CONCATENATE(IST_XCDAC20_CHAN_ILK_,   c_suf)), \
        REDLED_LINE(NULL, NULL, __CX_CONCATENATE(IST_XCDAC20_CHAN_C_ILK_, c_suf))

        ILK_LINE("out",     "Out Prot", OUT_PROT),
        ILK_LINE("water",   "Water",    WATER),
        ILK_LINE("imax",    "Imax",     IMAX),
        ILK_LINE("umax",    "Umax",     UMAX),
        ILK_LINE("battery", "Battery",  BATTERY),
        ILK_LINE("phase",   "Phase",    PHASE),
        ILK_LINE("temp",    "Tempr",    TEMP),
    GLOBELEM_END("nocoltitles,norowtitles", 0),
    GLOBELEM_START("clb", "Calibrate & Dig.corr", 6, 6)
        {"calb", "Clbr DAC",   NULL, NULL, NULL,   NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_DO_CALB_DAC)},
        {"auto_calb", "Auto", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_AUTOCALB_ONOFF)},
        {"calb_period", "@",         "s",  "%6.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_AUTOCALB_SECS)},
        VSEP_L(),
        {"dc_mode",   "Dig. corr.", NULL, NULL, NULL,   NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_DIGCORR_MODE)},
        {"dc_val",    "",   NULL, "%8.0f", NULL,   NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IST_XCDAC20_CHAN_DIGCORR_FACTOR)},
    GLOBELEM_END("nocoltitles,norowtitles", GU_FLAG_FROMNEWLINE),

    {NULL}
};


static physprops_t ist_xcdac20_physinfo[] =
{
    {IST_XCDAC20_CHAN_DCCT1,    1000000.0/250, 0, 0},
    {IST_XCDAC20_CHAN_DCCT2,    1000000.0/250, 0, 0},
    {IST_XCDAC20_CHAN_DAC,      1000000.0/250, 0, 0},
    {IST_XCDAC20_CHAN_U2,       1000000.0/250, 0, 0},
    {IST_XCDAC20_CHAN_ADC4,     1000000.0, 0, 0},
    {IST_XCDAC20_CHAN_ADC_DAC,  1000000.0/250, 0, 0},
    {IST_XCDAC20_CHAN_ADC_0V,   1000000.0, 0, 0},
    {IST_XCDAC20_CHAN_ADC_P10V, 1000000.0, 0, 0},

    {IST_XCDAC20_CHAN_OUT,      1000000.0/250, 0, 305},
    {IST_XCDAC20_CHAN_OUT_RATE, 1000000.0/250, 0, 305},
    {IST_XCDAC20_CHAN_OUT_CUR,  1000000.0/250, 0, 305},
};
