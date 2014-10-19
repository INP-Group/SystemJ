
#include "drv_i/cac208_drv_i.h"
#include "common_elem_macros.h"


static groupunit_t cac208_grouping[] =
{

#define DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), NULL, NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CAC208_CHAN_WRITE_N_BASE      + n), .minnorm=-10.0, .maxnorm=+10.0}

    {
        &(elemnet_t){
            "dac", "DAC", NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("set", "Settings",
                                  "Set, V", "0\v1\v2\v3\v4\v5\v6\v7",
                                  8*1, 1)
                    DAC_LINE(0),
                    DAC_LINE(1),
                    DAC_LINE(2),
                    DAC_LINE(3),
                    DAC_LINE(4),
                    DAC_LINE(5),
                    DAC_LINE(6),
                    DAC_LINE(7),
                SUBELEM_END("noshadow,notitle,nohline", NULL),
            },
            "nocoltitles,norowtitles"
        }, 1
    },

#define ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), NULL, NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CAC208_CHAN_READ_N_BASE+n), .minnorm=-10.0, .maxnorm=+10.0}

    {
        &(elemnet_t){
            "adc", "ADC", NULL, NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                SUBELEM_START_CRN("mes", "Measurements",
                                  "U, V", "0\v1\v2\v3\v4\v5\v6\v7\v8\v9\v10\v11\v12\v13\v14\v15\v16\v17\v18\v19\v10V\v0V\vT\vPWR",
                                  24, 1 + 8*65536)
                    ADC_LINE(0),
                    ADC_LINE(1),
                    ADC_LINE(2),
                    ADC_LINE(3),
                    ADC_LINE(4),
                    ADC_LINE(5),
                    ADC_LINE(6),
                    ADC_LINE(7),
                    ADC_LINE(8),
                    ADC_LINE(9),
                    ADC_LINE(10),
                    ADC_LINE(11),
                    ADC_LINE(12),
                    ADC_LINE(13),
                    ADC_LINE(14),
                    ADC_LINE(15),
                    ADC_LINE(16),
                    ADC_LINE(17),
                    ADC_LINE(18),
                    ADC_LINE(19),
                    ADC_LINE(20),
                    ADC_LINE(21),
                    ADC_LINE(22),
                    ADC_LINE(23),
                SUBELEM_END("noshadow,notitle,nohline", NULL),
            },
            "nocoltitles,norowtitles"
        }, 1
    },

#define OUTB_LINE(n) \
    {"outb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CAC208_CHAN_REGS_WR8_BASE+n)}
    
#define INPB_LINE(n) \
    {"inpb" __CX_STRINGIZE(n), "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CAC208_CHAN_REGS_RD8_BASE+n)}
    
    {
        &(elemnet_t){
            "ioregs", "I/O", "8b\v7\v6\v5\v4\v3\v2\v1\v0", "OutR\vInpR",
            ELEM_MULTICOL, 9*2, 9,
            (logchannet_t []){
                {"out8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CAC208_CHAN_REGS_WR1)},
                OUTB_LINE(7),
                OUTB_LINE(6),
                OUTB_LINE(5),
                OUTB_LINE(4),
                OUTB_LINE(3),
                OUTB_LINE(2),
                OUTB_LINE(1),
                OUTB_LINE(0),
                {"inp8", "8b", NULL, "%3.0f", NULL, NULL, LOGT_READ,   LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(CAC208_CHAN_REGS_RD1)},
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


static physprops_t cac208_physinfo[] =
{
    {CAC208_CHAN_READ_N_BASE +  0, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  1, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  2, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  3, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  4, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  5, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  6, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  7, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  8, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE +  9, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 10, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 11, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 12, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 13, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 14, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 15, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 16, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 17, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 18, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 19, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 20, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 21, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 22, 1000000.0, 0, 0},
    {CAC208_CHAN_READ_N_BASE + 23, 1000000.0, 0, 0},

    {CAC208_CHAN_WRITE_N_BASE     + 0, 1000000.0, 0, 305},
    {CAC208_CHAN_WRITE_N_BASE     + 1, 1000000.0, 0, 305},
    {CAC208_CHAN_WRITE_N_BASE     + 2, 1000000.0, 0, 305},
    {CAC208_CHAN_WRITE_N_BASE     + 3, 1000000.0, 0, 305},
    {CAC208_CHAN_WRITE_N_BASE     + 4, 1000000.0, 0, 305},
    {CAC208_CHAN_WRITE_N_BASE     + 5, 1000000.0, 0, 305},
    {CAC208_CHAN_WRITE_N_BASE     + 6, 1000000.0, 0, 305},
    {CAC208_CHAN_WRITE_N_BASE     + 7, 1000000.0, 0, 305},
};
