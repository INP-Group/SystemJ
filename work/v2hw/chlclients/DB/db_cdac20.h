
#include "drv_i/cdac20_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t cdac20_grouping[] =
{

#define DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CDAC20_CHAN_WRITE), .minnorm=-10.0, .maxnorm=+10.0}

#define ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), NULL, NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CDAC20_CHAN_READ_N_BASE+n), .minnorm=-10.0, .maxnorm=+10.0}

    {
        &(elemnet_t){
            "vlotages", "DAC & ADC, V", NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("mes", "Measurements",
                                  "U, V", "Set\v0\v1\v2\v3\v4\vDAC\v0V\v10V",
                                  9, 1)
                    DAC_LINE(0),
                    ADC_LINE(0),
                    ADC_LINE(1),
                    ADC_LINE(2),
                    ADC_LINE(3),
                    ADC_LINE(4),
                    ADC_LINE(5),
                    ADC_LINE(6),
                    ADC_LINE(7),
                SUBELEM_END("noshadow,notitle,nohline,nocoltitles", NULL),
            },
            "nocoltitles,norowtitles"
        }, 1
    },

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CDAC20_CHAN_REGS_WR8_BASE+n)}
    
#define INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CDAC20_CHAN_REGS_RD8_BASE+n)}
    
    {
        &(elemnet_t){
            "ioregs", "I/O", "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR",
            ELEM_MULTICOL, 9*2, 9,
            (logchannet_t []){
                {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CDAC20_CHAN_REGS_WR1)},
                OUTB_LINE(7),
                OUTB_LINE(6),
                OUTB_LINE(5),
                OUTB_LINE(4),
                OUTB_LINE(3),
                OUTB_LINE(2),
                OUTB_LINE(1),
                OUTB_LINE(0),
                {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CDAC20_CHAN_REGS_RD1)},
                INPB_LINE(7),
                INPB_LINE(6),
                INPB_LINE(5),
                INPB_LINE(4),
                INPB_LINE(3),
                INPB_LINE(2),
                INPB_LINE(1),
                INPB_LINE(0),
            },
            ""
        }, 1
    },

    {NULL}
};


static physprops_t cdac20_physinfo[] =
{
    {CDAC20_CHAN_READ_N_BASE +  0, 1000000.0, 0, 0},
    {CDAC20_CHAN_READ_N_BASE +  1, 1000000.0, 0, 0},
    {CDAC20_CHAN_READ_N_BASE +  2, 1000000.0, 0, 0},
    {CDAC20_CHAN_READ_N_BASE +  3, 1000000.0, 0, 0},
    {CDAC20_CHAN_READ_N_BASE +  4, 1000000.0, 0, 0},
    {CDAC20_CHAN_READ_N_BASE +  5, 1000000.0, 0, 0},
    {CDAC20_CHAN_READ_N_BASE +  6, 1000000.0, 0, 0},
    {CDAC20_CHAN_READ_N_BASE +  7, 1000000.0, 0, 0},

    {CDAC20_CHAN_WRITE, 1000000.0, 0, 305},
};
