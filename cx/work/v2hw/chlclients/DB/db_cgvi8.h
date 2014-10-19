
#include "drv_i/cgvi8_drv_i.h"
#include "common_elem_macros.h"


#define CGVI_LINE(n) \
    {"e"   #n, "",   NULL, "%1.0f",  NULL,                  NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_MASK1_N_BASE+n)}, \
    {"c"   #n, #n,   "q",  "%5.0f",  "withunits",           NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_16BIT_N_BASE+n), .minnorm=0, .maxnorm=65535}, \
    {"hns" #n, "=",  "us", "%11.1f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_QUANT_N_BASE+n), .incdec_step=0.1, .minnorm=0, .maxnorm=214700000}

groupunit_t cgvi8_grouping[] =
{
    GLOBELEM_START("gvi", "CGVI8", 3, 1)
        SUBELEM_START_CRN("set", "Settings",
                          " \v16-bit code\vTime",
                          "?\v?\v?\v?\v?\v?\v?\v?\v",
                          8*3, 3)
            CGVI_LINE(0),
            CGVI_LINE(1),
            CGVI_LINE(2),
            CGVI_LINE(3),
            CGVI_LINE(4),
            CGVI_LINE(5),
            CGVI_LINE(6),
            CGVI_LINE(7),
        SUBELEM_END("notitle,noshadow,norowtitles", NULL),

        SEP_L(),

        SUBELEM_START("ctl", "Control", 3, 2)
            {"base",  "Base byte",   NULL, "%3.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_BASEBYTE), .minnorm=0, .maxnorm=255},
            {"scale", "q=", NULL, "%1.0f",
                "#T100 ns\v200 ns\v400 ns\v800 ns\v1.60 us\v3.20 us\v6.40 us\v12.8 us\v25.6 us\v51.2 us\v102 us\v205 us\v410 us\v819 us\v1.64 ms\v3.28 ms",
            NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_PRESCALER)},
            {"Start", "ProgStart",  NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_PROGSTART), .placement="horz=right"},
        SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 0),
    
#define CGVI8_OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_REGS_WR8_BASE+n)}
    
#define CGVI8_INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_REGS_RD8_BASE+n)}
    
    GLOBELEM_START_CRN("ioregs", "I/O",
                       "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR",
                       9*2, 9)
        {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_REGS_WR1)},
        CGVI8_OUTB_LINE(7),
        CGVI8_OUTB_LINE(6),
        CGVI8_OUTB_LINE(5),
        CGVI8_OUTB_LINE(4),
        CGVI8_OUTB_LINE(3),
        CGVI8_OUTB_LINE(2),
        CGVI8_OUTB_LINE(1),
        CGVI8_OUTB_LINE(0),
        {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CGVI8_CHAN_REGS_RD1)},
        CGVI8_INPB_LINE(7),
        CGVI8_INPB_LINE(6),
        CGVI8_INPB_LINE(5),
        CGVI8_INPB_LINE(4),
        CGVI8_INPB_LINE(3),
        CGVI8_INPB_LINE(2),
        CGVI8_INPB_LINE(1),
        CGVI8_INPB_LINE(0),
    GLOBELEM_END("", 1),

    {NULL}
};

physprops_t cgvi8_physinfo[] =
{
    {CGVI8_CHAN_QUANT_N_BASE+0, 10},
    {CGVI8_CHAN_QUANT_N_BASE+1, 10},
    {CGVI8_CHAN_QUANT_N_BASE+2, 10},
    {CGVI8_CHAN_QUANT_N_BASE+3, 10},
    {CGVI8_CHAN_QUANT_N_BASE+4, 10},
    {CGVI8_CHAN_QUANT_N_BASE+5, 10},
    {CGVI8_CHAN_QUANT_N_BASE+6, 10},
    {CGVI8_CHAN_QUANT_N_BASE+7, 10},
};
