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
            "-ChanA0", "Канал A-0",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 0, "4с Нагрузка"),
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 1, "5с Нагрузка"),
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 2, "6с Нагрузка"),
                CHAN0_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT0, 3, "6с Отраженная"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {
        &(elemnet_t){
            "-ChanA1", "Канал A-1",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 0, "4с Падающая"),
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 1, "5с Падающая"),
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 2, "6с Падающая"),
                CHAN1_LINE(WAVESEL_L0403_N1_BASE+L0403_CHAN_OUT1, 3, "5с Отраженная"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
#if 1
    {
        &(elemnet_t){
            "-ChanB0", "Канал B-0",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 0, "Пучок"),
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 1, "1с Падающая"),
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 2, "1с Нагрузка"),
                CHAN0_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT0, 3, "3с Нагрузка"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {
        &(elemnet_t){
            "-ChanB1", "Канал B-1",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 0, "3с Падающая"),
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 1, "2с Нагрузка"),
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 2, "2с Падающая"),
                CHAN1_LINE(WAVESEL_L0403_N2_BASE+L0403_CHAN_OUT1, 3, "2с Отраженная"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
#else
    {
        &(elemnet_t){
            "-Comm", "Коммутатор",
            "", "",
            ELEM_SINGLECOL, 8, 1,
            (logchannet_t []){
                CHAN1_LINE(30, 0, "????????"),
                CHAN1_LINE(30, 1, "1с Падающая"),
                CHAN1_LINE(30, 2, "1с Нагрузка"),
                CHAN1_LINE(30, 3, "3с Нагрузка"),
                CHAN1_LINE(30, 4, "3с Падающая"),
                CHAN1_LINE(30, 5, "2с Нагрузка"),
                CHAN1_LINE(30, 6, "2с Падающая"),
                CHAN1_LINE(30, 7, "2с Отраженная"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
#endif
    {
        &(elemnet_t){
            "-ChanC0", "Канал C-0",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 0, "7с Падающая"),
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 1, "8с Падающая"),
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 2, "9с Падающая"),
                CHAN0_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT0, 3, "10с Падающая"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {
        &(elemnet_t){
            "-ChanC1", "Канал C-1",
            "", "",
            ELEM_SINGLECOL, 4, 1,
            (logchannet_t []){
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 0, "7с Нагрузка"),
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 1, "8с Нагрузка"),
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 2, "9с Нагрузка"),
                CHAN1_LINE(WAVESEL_L0403_N3_BASE+L0403_CHAN_OUT1, 3, "10с Нагрузка"),
            },
            "nocoltitles,norowtitles,fold_h"
        }, 1
    },
    {NULL}
};

static physprops_t wavesel_physinfo[] =
{
};
