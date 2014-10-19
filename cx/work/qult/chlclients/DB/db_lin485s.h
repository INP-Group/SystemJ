#include "drv_i/nkshd485_drv_i.h"

#include "common_elem_macros.h"


enum
{
    K485_BASE = 0,
};

#define KSH_LINE_START(n, l) \
    HLABEL_CELL(l), \
    {"-go-l", NULL, NULL, NULL, NULL, NULL, \
     LOGT_WRITE1, LOGD_ARROW_UP, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
     (excmd_t[]){ \
         CMD_GETP_I(K485_BASE + NKSHD485_NUMCHANS * n + NKSHD485_CHAN_STATUS_KM), \
         CMD_RET \
     }, \
     (excmd_t[]){ \
         CMD_PUSH_I(-1000000), \
         CMD_SETP_I(K485_BASE + NKSHD485_NUMCHANS * n + NKSHD485_CHAN_GO_N_STEPS), \
         CMD_RET \
     }, \
    },  \
    {"-go-r", NULL, NULL, NULL, NULL, NULL, \
     LOGT_WRITE1, LOGD_ARROW_DN, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
     (excmd_t[]){ \
         CMD_GETP_I(K485_BASE + NKSHD485_NUMCHANS * n + NKSHD485_CHAN_STATUS_KP), \
         CMD_RET \
     }, \
     (excmd_t[]){ \
         CMD_PUSH_I(1000000), \
         CMD_SETP_I(K485_BASE + NKSHD485_NUMCHANS * n + NKSHD485_CHAN_GO_N_STEPS), \
         CMD_RET \
     }, \
    },  \
    {"-stop", "STOP", NULL, NULL, NULL, NULL, \
     LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
     (excmd_t[]){ \
         CMD_GETP_I(K485_BASE + NKSHD485_NUMCHANS * n + NKSHD485_CHAN_STATUS_GOING), \
         CMD_RET \
     }, \
     (excmd_t[]){ \
         CMD_PUSH_I(1), \
         CMD_SETP_I(K485_BASE + NKSHD485_NUMCHANS * n + NKSHD485_CHAN_STOP), \
         CMD_RET \
     }, \
    }

static groupunit_t lin485s_grouping[] =
{
    GLOBELEM_START(NULL, NULL, 1, 1)
        {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP}
    GLOBELEM_END("noshadow,notitle,nocoltitles,norowtitles", 0),

    GLOBELEM_START(NULL, NULL, 5*4, 5)

        #define NKSHD485_BASE  K485_BASE + NKSHD485_NUMCHANS * 0
        #define NKSHD485_NAME  "0"
        #define NKSHD485_IDENT "0"
        KSH_LINE_START(0, NKSHD485_NAME),
        #include "elem_subwin_nkshd485.h"
        ,

        #define NKSHD485_BASE  K485_BASE + NKSHD485_NUMCHANS * 1
        #define NKSHD485_NAME  "1"
        #define NKSHD485_IDENT "1"
        KSH_LINE_START(1, NKSHD485_NAME),
        #include "elem_subwin_nkshd485.h"
        ,
    
        #define NKSHD485_BASE  K485_BASE + NKSHD485_NUMCHANS * 2
        #define NKSHD485_NAME  "2"
        #define NKSHD485_IDENT "2"
        KSH_LINE_START(2, NKSHD485_NAME),
        #include "elem_subwin_nkshd485.h"
        ,
    
        #define NKSHD485_BASE  K485_BASE + NKSHD485_NUMCHANS * 3
        #define NKSHD485_NAME  "3"
        #define NKSHD485_IDENT "3"
        KSH_LINE_START(3, NKSHD485_NAME),
        #include "elem_subwin_nkshd485.h"
        ,
    
    GLOBELEM_END("noshadow,notitle,nocoltitles,norowtitles", GU_FLAG_FROMNEWLINE),

    {NULL}
};

static physprops_t lin485s_physinfo[] =
{
    {K485_BASE + NKSHD485_NUMCHANS * 0 + NKSHD485_CHAN_DEV_VERSION, 10, 0, 0},
    {K485_BASE + NKSHD485_NUMCHANS * 1 + NKSHD485_CHAN_DEV_VERSION, 10, 0, 0},
    {K485_BASE + NKSHD485_NUMCHANS * 2 + NKSHD485_CHAN_DEV_VERSION, 10, 0, 0},
    {K485_BASE + NKSHD485_NUMCHANS * 3 + NKSHD485_CHAN_DEV_VERSION, 10, 0, 0},
};
