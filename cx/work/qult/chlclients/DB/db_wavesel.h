#include "drv_i/l0403_drv_i.h"

enum
{
    WAVESEL_L0403_N1_BASE = 25,
    WAVESEL_L0403_N2_BASE = 31,
    WAVESEL_L0403_N3_BASE = 37,
};

#define A_LINE(chan, v, title, col) \
    {NULL, title, NULL, NULL, "color="col, NULL, LOGT_WRITE1, LOGD_RADIOBTN, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
     (excmd_t []){CMD_PUSH_I(0), CMD_PUSH_I(CX_VALUE_LIT_MASK), CMD_PUSH_I(0), \
                  CMD_GETP_I(chan), CMD_SUB_I(v), CMD_CASE, \
                  CMD_RET}, \
     (excmd_t []){CMD_PUSH_I(v), CMD_SETP_I(chan), CMD_RET}       \
    }

#define CHAN0_LINE(chan, v, title) A_LINE(chan, v, title, "blue")

#define CHAN1_LINE(chan, v, title) A_LINE(chan, v, title, "red")

#define COMM_LINE(chan, v, title)  A_LINE(chan, v, title, "red")

static groupunit_t wavesel_grouping[] =
{
    {
        &(elemnet_t){
            "-leds", "LEDS",
            "", "",
            ELEM_SINGLECOL, 1, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP, .placement="horz=left"}
            },
            "nocoltitles,norowtitles,notitle,noshadow"
        }, 1
    },
    {
        &(elemnet_t){
            "-ChanA0", "����� A-0",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 0, "4� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 1, "5� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 2, "6� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 3, "6� ����������"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {
        &(elemnet_t){
            "-ChanA1", "����� A-1",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 0, "4� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 1, "5� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 2, "6� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 3, "5� ����������"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
#if 1
    {
        &(elemnet_t){
            "-ChanB0", "����� B-0",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 0, "�����"),
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 1, "1� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 2, "1� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 3, "3� ��������"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {
        &(elemnet_t){
            "-ChanB1", "����� B-1",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 0, "3� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 1, "2� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 2, "2� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 3, "2� ����������"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
#else
    {
        &(elemnet_t){
            "-Comm", "����������",
            "", "",
            ELEM_SINGLECOL, 8, 1,
            (logchannet_t []){
                CHAN1_LINE(30, 0, "????????"),
                CHAN1_LINE(30, 1, "1� ��������"),
                CHAN1_LINE(30, 2, "1� ��������"),
                CHAN1_LINE(30, 3, "3� ��������"),
                CHAN1_LINE(30, 4, "3� ��������"),
                CHAN1_LINE(30, 5, "2� ��������"),
                CHAN1_LINE(30, 6, "2� ��������"),
                CHAN1_LINE(30, 7, "2� ����������"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
#endif
    {
        &(elemnet_t){
            "-ChanC0", "����� C-0",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 0, "7� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 1, "8� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 2, "9� ��������"),
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 3, "10� ��������"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {
        &(elemnet_t){
            "-ChanC1", "����� C-1",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 0, "7� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 1, "8� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 2, "9� ��������"),
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 3, "10� ��������"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {NULL}
};

static physprops_t wavesel_physinfo[] =
{
};
