#include <stdio.h>

#include "timeval_utils.h"

#include "cxsd_driver.h"
#include "vdev.h"
#include "drv_i/ist_xcdac20_drv_i.h"
#include "drv_i/xcdac20_drv_i.h"


enum
{
    C20C_OUT            = XCDAC20_CHAN_OUT_n_base      + 0,
    C20C_OUT_RATE       = XCDAC20_CHAN_OUT_RATE_n_base + 0,
    C20C_SWITCH_ON      = XCDAC20_CHAN_REGS_WR8_base + 0,
    C20C_ENABLE         = XCDAC20_CHAN_REGS_WR8_base + 1,
    C20C_RST_ILKS_B2    = XCDAC20_CHAN_REGS_WR8_base + 2,
    C20C_RST_ILKS_B3    = XCDAC20_CHAN_REGS_WR8_base + 3,
    C20C_REVERSE_ON     = XCDAC20_CHAN_REGS_WR8_base + 6,
    C20C_REVERSE_OFF    = XCDAC20_CHAN_REGS_WR8_base + 7,

    C20C_DO_CALB_DAC    = XCDAC20_CHAN_DO_CALB_DAC,
    C20C_AUTOCALB_ONOFF = XCDAC20_CHAN_AUTOCALB_ONOFF,
    C20C_AUTOCALB_SECS  = XCDAC20_CHAN_AUTOCALB_SECS,
    C20C_DIGCORR_MODE   = XCDAC20_CHAN_DIGCORR_MODE,

    C20C_DCCT1          = XCDAC20_CHAN_ADC_n_base + 0,
    C20C_DCCT2          = XCDAC20_CHAN_ADC_n_base + 1, // REVERSE: status: +5V:positive, -5V:negative
    C20C_DAC            = XCDAC20_CHAN_ADC_n_base + 2,
    C20C_U2             = XCDAC20_CHAN_ADC_n_base + 3,
    C20C_ADC4           = XCDAC20_CHAN_ADC_n_base + 4, // REVERSE: true I: 5V~=1000A
    C20C_ADC_DAC        = XCDAC20_CHAN_ADC_n_base + 5,
    C20C_ADC_0V         = XCDAC20_CHAN_ADC_n_base + 6,
    C20C_ADC_P10V       = XCDAC20_CHAN_ADC_n_base + 7,
    C20C_OUT_CUR        = XCDAC20_CHAN_OUT_CUR_n_base + 0,

    C20C_DIGCORR_FACTOR = XCDAC20_CHAN_DIGCORR_FACTOR,

    C20C_OPR            = XCDAC20_CHAN_REGS_RD8_base + 0,
    C20C_ILK_base       = XCDAC20_CHAN_REGS_RD8_base + 1,
      C20C_ILK_count    = 7,
    C20C_ILK_OUT_PROT   = C20C_ILK_base + 0,
    C20C_ILK_WATER      = C20C_ILK_base + 1,
    C20C_ILK_IMAX       = C20C_ILK_base + 2,
    C20C_ILK_UMAX       = C20C_ILK_base + 3,
    C20C_ILK_BATTERY    = C20C_ILK_base + 4,
    C20C_ILK_PHASE      = C20C_ILK_base + 5,
    C20C_ILK_TEMP       = C20C_ILK_base + 6,

    SUBORD_NUMCHANS = XCDAC20_NUMCHANS
};
enum {C20C_POLARITY = C20C_ADC4};


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

enum
{
    MAX_ALWD_VAL =   9999999,
    MIN_ALWD_VAL = -10000305
};

enum
{
    WORK_HEARTBEAT_PERIOD =    100000, // 100ms/10Hz
    BIND_HEARTBEAT_PERIOD = 2*1000000  // 2s
};

enum
{
    IST_STATE_UNKNOWN   = 0,
    IST_STATE_DETERMINE,         // 1
    IST_STATE_INTERLOCK,         // 2
    IST_STATE_RST_ILK_SET,       // 3
    IST_STATE_RST_ILK_DRP,       // 4
    IST_STATE_RST_ILK_CHK,       // 5

    IST_STATE_IS_OFF,            // 6

    IST_STATE_SW_ON_ENABLE,      // 7
    IST_STATE_SW_ON_SET_ON,      // 8
    IST_STATE_SW_ON_DRP_ON,      // 9
    IST_STATE_SW_ON_UP,          // 10

    IST_STATE_IS_ON,             // 11

    IST_STATE_SW_OFF_DOWN,       // 12
    IST_STATE_SW_OFF_CHK_I,      // 13
    IST_STATE_SW_OFF_CHK_E,      // 14

    IST_STATE_RV_ZERO,           // 15
    IST_STATE_RV_SET_DO,         // 16
    IST_STATE_RV_DRP_DO,         // 17
    IST_STATE_RV_CHK,            // 18

    IST_STATE_count
};


typedef struct
{
    int                 devid;

    int                 reversable;

    vdev_context_t      ctx;
    vdev_observer_t     obs;
    vdev_trgc_cur_t     cur[SUBORD_NUMCHANS];

    int32               out_val;
    int                 out_val_set;

    int32               cur_sign;
    int32               rqd_sign;
    
    int32               c_ilks_vals[IST_XCDAC20_CHAN_C_ILK_count];
} privrec_t;

static vdev_trgc_dsc_t xcdac2ist_mapping[SUBORD_NUMCHANS] =
{
    [C20C_OUT]            = {VDEV_IMPR,             -1},
    [C20C_OUT_RATE]       = {            VDEV_TUBE, IST_XCDAC20_CHAN_OUT_RATE},
    [C20C_SWITCH_ON]      = {VDEV_PRIV,             -1},
    [C20C_ENABLE]         = {VDEV_PRIV,             -1},
    [C20C_RST_ILKS_B2]    = {VDEV_PRIV,             -1},
    [C20C_RST_ILKS_B3]    = {VDEV_PRIV,             -1},
    [C20C_REVERSE_ON]     = {VDEV_PRIV,             -1},
    [C20C_REVERSE_OFF]    = {VDEV_PRIV,             -1},

    [C20C_DO_CALB_DAC]    = {            VDEV_TUBE, IST_XCDAC20_CHAN_DO_CALB_DAC},
    [C20C_AUTOCALB_ONOFF] = {            VDEV_TUBE, IST_XCDAC20_CHAN_AUTOCALB_ONOFF},
    [C20C_AUTOCALB_SECS]  = {            VDEV_TUBE, IST_XCDAC20_CHAN_AUTOCALB_SECS},
    [C20C_DIGCORR_MODE]   = {            VDEV_TUBE, IST_XCDAC20_CHAN_DIGCORR_MODE},

    [C20C_DCCT1]          = {VDEV_PRIV | /*!!!REVERS*/0/*VDEV_TUBE*/, IST_XCDAC20_CHAN_DCCT1},
    [C20C_DCCT2]          = {VDEV_IMPR | /*!!!REVERS*/0/*VDEV_TUBE*/, IST_XCDAC20_CHAN_DCCT2}, // IMPR for reversable ISTs, where it shows polarity
    [C20C_DAC]            = {            VDEV_TUBE, IST_XCDAC20_CHAN_DAC},
    [C20C_U2]             = {            VDEV_TUBE, IST_XCDAC20_CHAN_U2},
    [C20C_ADC4]           = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ADC4},  // IMPR for reversable ISTs, where it shows polarity
    [C20C_ADC_DAC]        = {            VDEV_TUBE, IST_XCDAC20_CHAN_ADC_DAC},
    [C20C_ADC_0V]         = {            VDEV_TUBE, IST_XCDAC20_CHAN_ADC_0V},
    [C20C_ADC_P10V]       = {            VDEV_TUBE, IST_XCDAC20_CHAN_ADC_P10V},
    [C20C_OUT_CUR]        = {VDEV_IMPR | /*!!!REVERS*/0/*VDEV_TUBE*/, IST_XCDAC20_CHAN_OUT_CUR},

    [C20C_DIGCORR_FACTOR] = {            VDEV_TUBE, IST_XCDAC20_CHAN_DIGCORR_FACTOR},

    [C20C_OPR]            = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_OPR},
    [C20C_ILK_OUT_PROT]   = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ILK_OUT_PROT},
    [C20C_ILK_WATER]      = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ILK_WATER},
    [C20C_ILK_IMAX]       = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ILK_IMAX},
    [C20C_ILK_UMAX]       = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ILK_UMAX},
    [C20C_ILK_BATTERY]    = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ILK_BATTERY},
    [C20C_ILK_PHASE]      = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ILK_PHASE},
    [C20C_ILK_TEMP]       = {VDEV_IMPR | VDEV_TUBE, IST_XCDAC20_CHAN_ILK_TEMP},
};                               


static inline void SndCVal(privrec_t *me, int sodc, int32 val)
{
    vdev_observer_snd_chan(&(me->obs), sodc, val);
}
static inline void ReqCVal(privrec_t *me, int sodc)
{
    vdev_observer_req_chan(&(me->obs), sodc);
}

static int ourc2sodc[IST_XCDAC20_NUMCHANS];

//////////////////////////////////////////////////////////////////////

static int32 sign_of(int32 v)
{
    if      (v < 0) return -1;
    else if (v > 0) return +1;
    else            return  0;
}

//////////////////////////////////////////////////////////////////////

static void SwchToUNKNOWN(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    vdev_observer_forget_all(&me->obs);
}

static void SwchToDETERMINE(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    /*REVERS*/
    if (me->reversable)
    {
        if      (me->cur[C20C_POLARITY].val < -4000000) me->cur_sign = -1;
        else if (me->cur[C20C_POLARITY].val > +4000000) me->cur_sign = +1;
        else /*??? set to some error? */;
    }
    else
        me->cur_sign = +1;

    if (!me->out_val_set)
    {
        me->out_val     = me->cur[C20C_OUT_CUR].val * me->cur_sign;/*REVERS*/
        me->out_val_set = 1; /* Yes, we do it only ONCE, since later OUT_CUR may change upon our request, without any client's write */
        ReturnChanGroup(me->devid, IST_XCDAC20_CHAN_OUT, 1, &(me->out_val), &zero_rflags);
    }

    if (me->cur[C20C_OPR].val)
        vdev_set_state(&(me->ctx), IST_STATE_IS_ON);
    else
        vdev_set_state(&(me->ctx), IST_STATE_IS_OFF);
}

static void SwchToINTERLOCK(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_OUT,       0);
    SndCVal(me, C20C_SWITCH_ON, 0);
    SndCVal(me, C20C_ENABLE,    0);
}

static int  IsAlwdRST_ILK_SET(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return me->ctx.cur_state == IST_STATE_INTERLOCK
        /* 26.09.2012: following added to allow [Reset] in other cases */
        || (me->ctx.cur_state >= IST_STATE_RST_ILK_SET  &&  me->ctx.cur_state <= IST_STATE_RST_ILK_CHK)
        ;
}

static int  IsAlwdRST_ILK_CHK(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
  int        sodc;

    for (sodc = C20C_ILK_base;  sodc < C20C_ILK_base + C20C_ILK_count;  sodc++)
        if (me->cur[sodc].val) return 0;

    return 1;
}

static void Drop_c_ilks(privrec_t *me)
{
  int  x;

    for (x = 0;  x < IST_XCDAC20_CHAN_C_ILK_count;  x++)
    {
        me->c_ilks_vals[x] = 0;
        ReturnChanGroup(me->devid, IST_XCDAC20_CHAN_C_ILK_base + x, 1,
                        me->c_ilks_vals + x, &zero_rflags);
    }
}

static void SwchToRST_ILK_SET(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    Drop_c_ilks(me);
    SndCVal(me, C20C_RST_ILKS_B2, 1);
    SndCVal(me, C20C_RST_ILKS_B3, 1);
}

static void SwchToRST_ILK_DRP(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_RST_ILKS_B2, 0);
    SndCVal(me, C20C_RST_ILKS_B3, 0);
}

static int  IsAlwdSW_ON_ENABLE(void *devptr, int prev_state)
{
  privrec_t *me = devptr;
  int        ilk;
  int        x;

    for (x = 0, ilk = 0;  x < C20C_ILK_count;  x++)
        ilk |= me->cur[C20C_ILK_base + x].val;

    return
        (prev_state == IST_STATE_IS_OFF  ||
         prev_state == IST_STATE_INTERLOCK)  &&
        me->cur[C20C_OUT_CUR].val == 0       &&
        ilk == 0;
}

static void SwchToON_ENABLE(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_ENABLE,    1);
}

static void SwchToON_SET_ON(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_SWITCH_ON, 1);
}

static void SwchToON_DRP_ON(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_SWITCH_ON, 0);
}

/* 26.09.2012: added to proceed to UP only if OPR is really on */
static int  IsAlwdSW_ON_UP(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return me->cur[C20C_OPR].val != 0;
}

static void SwchToON_UP    (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    if (me->reversable            &&
        sign_of(me->out_val) != 0 &&
        sign_of(me->out_val) != me->cur_sign)
    {
        /* A "hack" for reverse: "call" polarity-switch sequence */
        vdev_set_state(&(me->ctx), IST_STATE_RV_ZERO);
        return;
    }

    SndCVal(me, C20C_OUT,       abs(me->out_val)/*REVERS*/);
}

static int  IsAlwdIS_ON    (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return (abs(me->cur[C20C_OUT_CUR].val) - /*REVERS*/abs(me->out_val)) <= 305;
}

static int  IsAlwdSW_OFF_DOWN(void *devptr, int prev_state)
{
  privrec_t *me = devptr;

    return
        /* Regular "ON" state... */
        prev_state == IST_STATE_IS_ON         ||
        /* ...and "switching-to-on" stages */
        prev_state == IST_STATE_SW_ON_ENABLE  ||
        prev_state == IST_STATE_SW_ON_SET_ON  ||
        prev_state == IST_STATE_SW_ON_DRP_ON  ||
        prev_state == IST_STATE_SW_ON_UP      ||
        /* ...or the state bit is just "on" */
        me->cur[C20C_OPR].val != 0
        ;
}

static void SwchToSW_OFF_DOWN(void *devptr, int prev_state)
{
  privrec_t *me = devptr;

    ////fprintf(stderr, "%s()\n", __FUNCTION__);
    SndCVal(me, C20C_OUT,    0);
    if (prev_state != IST_STATE_IS_ON) SndCVal(me, C20C_SWITCH_ON, 0);
}

static int  IsAlwdSW_OFF_CHK_I(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    ////fprintf(stderr, "\tout_cur=%d\n", me->cur[C20C_OUT_CUR].val);
    return me->cur[C20C_OUT_CUR].val == 0;
}

static void SwchToSW_OFF_CHK_I(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_ENABLE, 0);
}

static int  IsAlwdSW_OFF_CHK_E(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    fprintf(stderr, "\topr=%d\n", me->cur[C20C_OPR].val);
    return me->cur[C20C_OPR].val == 0;
}

static void SwchToRV_ZERO(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    me->rqd_sign = sign_of(me->out_val); // Remember target-sign

    SndCVal(me, C20C_OUT,    0);
}

static int  IsAlwdRV_SET_DO(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return me->cur[C20C_OUT_CUR].val == 0;
}

static void SwchToRV_SET_DO(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me,
            me->rqd_sign < 0? C20C_REVERSE_ON : C20C_REVERSE_OFF,
            1);
}

static void SwchToRV_DRP_DO(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me,
            me->rqd_sign < 0? C20C_REVERSE_ON : C20C_REVERSE_OFF,
            0);
}

static int  IsAlwdRV_CHK(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    if      (me->cur[C20C_POLARITY].val < -4000000) me->cur_sign = -1;
    else if (me->cur[C20C_POLARITY].val > +4000000) me->cur_sign = +1;
    else return 0/* Hasn't changed+stabilized yet */;

    return me->cur_sign == me->rqd_sign;
}

static vdev_state_dsc_t state_descr[] =
{
    [IST_STATE_UNKNOWN]      = {0,       -1,                     NULL,               SwchToUNKNOWN,      NULL},
    [IST_STATE_DETERMINE]    = {0,       -1,                     NULL,               SwchToDETERMINE,    NULL},
    [IST_STATE_INTERLOCK]    = {0,       -1,                     NULL,               SwchToINTERLOCK,    NULL},

    [IST_STATE_RST_ILK_SET]  = {500000,  IST_STATE_RST_ILK_DRP,  IsAlwdRST_ILK_SET,  SwchToRST_ILK_SET,  NULL},
    [IST_STATE_RST_ILK_DRP]  = {500000,  IST_STATE_RST_ILK_CHK,  NULL,               SwchToRST_ILK_DRP,  NULL},
    [IST_STATE_RST_ILK_CHK]  = {1,       IST_STATE_IS_OFF,       IsAlwdRST_ILK_CHK,  NULL,               (int[]){C20C_ILK_OUT_PROT, C20C_ILK_WATER, C20C_ILK_IMAX, C20C_ILK_UMAX, C20C_ILK_BATTERY, C20C_ILK_PHASE, C20C_ILK_TEMP , -1}},

    [IST_STATE_IS_OFF]       = {0,       -1,                     NULL,               NULL,               NULL},

    [IST_STATE_SW_ON_ENABLE] = { 500000, IST_STATE_SW_ON_SET_ON, IsAlwdSW_ON_ENABLE, SwchToON_ENABLE,    NULL},
    [IST_STATE_SW_ON_SET_ON] = {1500000, IST_STATE_SW_ON_DRP_ON, NULL,               SwchToON_SET_ON,    NULL},
    [IST_STATE_SW_ON_DRP_ON] = { 500000*10, IST_STATE_SW_ON_UP,     NULL,               SwchToON_DRP_ON,    NULL},
    [IST_STATE_SW_ON_UP]     = {1,       IST_STATE_IS_ON,        IsAlwdSW_ON_UP,     SwchToON_UP,        NULL},

    [IST_STATE_IS_ON]        = {0,       -1,                     IsAlwdIS_ON,        NULL,               (int[]){C20C_OUT_CUR, -1}},

    [IST_STATE_SW_OFF_DOWN]  = {0,       IST_STATE_SW_OFF_CHK_I, IsAlwdSW_OFF_DOWN,  SwchToSW_OFF_DOWN,  NULL},
    [IST_STATE_SW_OFF_CHK_I] = {0,       IST_STATE_SW_OFF_CHK_E, IsAlwdSW_OFF_CHK_I, SwchToSW_OFF_CHK_I, (int[]){C20C_OUT_CUR, -1}},
    [IST_STATE_SW_OFF_CHK_E] = {1,       IST_STATE_IS_OFF,       IsAlwdSW_OFF_CHK_E, NULL,               (int[]){C20C_OPR, -1}},

    [IST_STATE_RV_ZERO]      = {0,       IST_STATE_RV_SET_DO,    NULL,               SwchToRV_ZERO,      NULL},
    [IST_STATE_RV_SET_DO]    = { 500000, IST_STATE_RV_DRP_DO,    IsAlwdRV_SET_DO,    SwchToRV_SET_DO,    (int[]){C20C_OUT_CUR, -1}},
    [IST_STATE_RV_DRP_DO]    = { 500000, IST_STATE_RV_CHK,       NULL,               SwchToRV_DRP_DO,    NULL},
    [IST_STATE_RV_CHK]       = { 500000, IST_STATE_SW_ON_UP,     IsAlwdRV_CHK,       NULL,               (int[]){C20C_POLARITY, -1}},

};

static int state_important_channels[countof(state_descr)][SUBORD_NUMCHANS];

//////////////////////////////////////////////////////////////////////

static int ist_xcdac20_init_mod(void)
{
  int  sn;
  int  x;

    /* Fill interesting_chan[][] mapping */
    bzero(state_important_channels, sizeof(state_important_channels));
    for (sn = 0;  sn < countof(state_descr);  sn++)
        if (state_descr[sn].impr_chans != NULL)
            for (x = 0;  state_descr[sn].impr_chans[x] >= 0;  x++)
                state_important_channels[sn][state_descr[sn].impr_chans[x]] = 1;

    /* ...and ourc->sodc mapping too */
    for (x = 0;  x < countof(ourc2sodc);  x++)
        ourc2sodc[x] = -1;
    for (x = 0;  x < countof(xcdac2ist_mapping);  x++)
        if (xcdac2ist_mapping[x].mode != 0  &&  xcdac2ist_mapping[x].ourc >= 0)
            ourc2sodc[xcdac2ist_mapping[x].ourc] = x;

    return 0;
}

//////////////////////////////////////////////////////////////////////

static void ist_xcdac20_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action);
static void ist_xcdac20_sodc_cb(int devid, void *devptr,
                                int sodc, int32 val);

static vdev_sr_chan_dsc_t state_related_channels[] =
{
    {IST_XCDAC20_CHAN_SWITCH_ON,  IST_STATE_SW_ON_ENABLE, 0,                 CX_VALUE_DISABLED_MASK},
    {IST_XCDAC20_CHAN_SWITCH_OFF, IST_STATE_SW_OFF_DOWN,  0,                 CX_VALUE_DISABLED_MASK},
    {IST_XCDAC20_CHAN_RST_ILKS,   IST_STATE_RST_ILK_SET,  CX_VALUE_LIT_MASK, 0},
    {IST_XCDAC20_CHAN_IS_ON,      -1,                     0,                 0},
    {-1,                          -1,                     0,                 0},
};

static int ist_xcdac20_init_d(int devid, void *devptr, 
                              int businfocount, int businfo[],
                              const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;
  const char     *p  = auxinfo;

    me->devid = devid;

    me->cur_sign = +1;

    if (p != NULL  &&  *p == '-')
    {
        me->reversable = 1;
        p++;
    }
    if (p == NULL  ||  *p == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s(): base-XCDAC20-device name is required in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    strzcpy(me->obs.targ_name, p, sizeof(me->obs.targ_name));
    me->obs.targ_numchans = countof(me->cur);
    me->obs.map           = xcdac2ist_mapping;
    me->obs.cur           = me->cur;

    me->ctx.chan_state_n             = IST_XCDAC20_CHAN_IST_STATE;
    me->ctx.state_unknown_val        = IST_STATE_UNKNOWN;
    me->ctx.state_determine_val      = IST_STATE_DETERMINE;
    me->ctx.state_descr              = state_descr;
    me->ctx.state_count              = countof(state_descr);

    me->ctx.state_related_channels   = state_related_channels;
    me->ctx.state_important_channels = state_important_channels;
    me->ctx.do_rw                    = ist_xcdac20_rw_p;
    me->ctx.sodc_cb                  = ist_xcdac20_sodc_cb;

    return vdev_init(&(me->ctx), &(me->obs),
                     devid, devptr,
                     BIND_HEARTBEAT_PERIOD, WORK_HEARTBEAT_PERIOD);
}

static void ist_xcdac20_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    vdev_fini(&(me->ctx));
}

static void ist_xcdac20_sodc_cb(int devid, void *devptr,
                                int sodc, int32 val)
{
  privrec_t      *me = (privrec_t *)devptr;
  int             ilk_n;

#if 0
    /* A special case -- derive OUT from OUT_CUR upon start */
    if (sodc == C20C_OUT_CUR  &&  !me->out_val_set)
    {
        me->out_val     = val;
        me->out_val_set = 1; /* Yes, we do it only ONCE, since later OUT_CUR may change upon our request, without any client's write */
        /*!!!REVERS*/
        ReturnChanGroup(devid, IST_XCDAC20_CHAN_OUT, 1, &(me->out_val), &zero_rflags);
    }
#endif

    /* If not all info is gathered yet, then there's nothing else to do yet */
    if (me->ctx.cur_state == IST_STATE_UNKNOWN) return;

#if 0
    /* In fact, is a hack... */
    if (me->ctx.cur_state == IST_STATE_DETERMINE)
    {
        /*REVERS*/
        if (me->reversable)
        {
            if      (me->cur[C20C_POLARITY].val < -4000000) me->cur_sign = -1;
            else if (me->cur[C20C_POLARITY].val > +4000000) me->cur_sign = +1;
            else /*??? set to some error? */;
        }
        else
            me->cur_sign = +1;

        if (!me->out_val_set)
        {
            me->out_val     = val;
            me->out_val_set = 1; /* Yes, we do it only ONCE, since later OUT_CUR may change upon our request, without any client's write */
            /*!!!REVERS*/
            ReturnChanGroup(devid, IST_XCDAC20_CHAN_OUT, 1, &(me->out_val), &zero_rflags);
        }

        if (me->cur[C20C_OPR].val)
            vdev_set_state(&(me->ctx), IST_STATE_IS_ON);
        else
            vdev_set_state(&(me->ctx), IST_STATE_IS_OFF);
    }
#endif
    
    /* Perform state transitions depending on received data */
    if (sodc >= C20C_ILK_base  &&
        sodc <  C20C_ILK_base + C20C_ILK_count)
    {
        ilk_n = sodc - C20C_ILK_base;
        if (val != 0  &&  me->c_ilks_vals[ilk_n] != val)
        {
            me->c_ilks_vals[ilk_n] = val;
            ReturnChanGroup(devid, IST_XCDAC20_CHAN_C_ILK_base + ilk_n, 1, &val, &zero_rflags);
        }

        if (val != 0  &&
            (// Lock-unaffected states
             me->ctx.cur_state != IST_STATE_RST_ILK_SET  &&
             me->ctx.cur_state != IST_STATE_RST_ILK_DRP  &&
             me->ctx.cur_state != IST_STATE_RST_ILK_CHK
            )
           )
            vdev_set_state(&(me->ctx), IST_STATE_INTERLOCK);
    }
    else if (/* 26.09.2012: when OPR bit disappears, we should better switch off */
             sodc == C20C_OPR  &&  val == 0  &&
             (me->ctx.cur_state == IST_STATE_SW_ON_DRP_ON  ||
              me->ctx.cur_state == IST_STATE_SW_ON_UP      ||
              me->ctx.cur_state == IST_STATE_IS_ON))
    {
        vdev_set_state(&(me->ctx), IST_STATE_SW_OFF_DOWN);
    }
    else if (sodc == C20C_OUT_CUR)
    {
        val *= me->cur_sign;
        ReturnChanGroup(devid, IST_XCDAC20_CHAN_OUT_CUR, 1, &val,
                        &(me->cur[C20C_OUT_CUR].flgs));
    }
    else if (sodc == C20C_DCCT1)
    {
        val *= me->cur_sign;
        ReturnChanGroup(devid, IST_XCDAC20_CHAN_DCCT1, 1, &val,
                        &(me->cur[C20C_DCCT1].flgs));
    }
    else if (sodc == C20C_DCCT2)
    {
        val *= me->cur_sign;
        ReturnChanGroup(devid, IST_XCDAC20_CHAN_DCCT2, 1, &val,
                        &(me->cur[C20C_DCCT2].flgs));
    }
}

static vdev_sr_chan_dsc_t *find_sdp(int ourc)
{
  vdev_sr_chan_dsc_t *sdp;

    for (sdp = state_related_channels;
         sdp != NULL  &&  sdp->ourc >= 0;
         sdp++)
        if (sdp->ourc == ourc) return sdp;

    return NULL;
}

static void ist_xcdac20_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  privrec_t          *me = (privrec_t *)devptr;
  int                 n;       // channel N in values[] (loop ctl var)
  int                 x;       // channel indeX
  int32               val;     // Value
  int                 sodc;
  vdev_sr_chan_dsc_t *sdp;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        sodc = ourc2sodc[x];

        if      (sodc >= 0  &&  xcdac2ist_mapping[sodc].mode & VDEV_TUBE)
        {
            if (action == DRVA_WRITE)
                SndCVal(me, sodc, val);
            else
                ReqCVal(me, sodc);
        }
        else if (x == IST_XCDAC20_CHAN_OUT)
        {
            if (action == DRVA_WRITE)
            {
                if (val > MAX_ALWD_VAL) val = MAX_ALWD_VAL;
                if (val < MIN_ALWD_VAL) val = MIN_ALWD_VAL;
                /*REVERS*/
                if (val < 0  &&  me->reversable == 0) val = 0;
                me->out_val     = val;
                me->out_val_set = 1;

                if (me->ctx.cur_state == IST_STATE_IS_ON  ||
                    me->ctx.cur_state == IST_STATE_SW_ON_UP)
                {
                    /*REVERS*/
                    if (me->reversable            &&
                        sign_of(me->out_val) != 0 &&
                        sign_of(me->out_val) != me->cur_sign)
                    {
                        /* Call polarity-switch sequence, than leading to SW_ON_UP */
                        vdev_set_state(&(me->ctx), IST_STATE_RV_ZERO);
                    }
                    else
                        SndCVal(me, C20C_OUT, me->out_val);
                }
            }
            if (me->out_val_set)
                ReturnChanGroup(devid, x, 1, &(me->out_val), &zero_rflags);
        }
        else if ((sdp = find_sdp(x)) != NULL  &&
                 sdp->state >= 0          &&
                 state_descr[sdp->state].sw_alwd != NULL)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND  &&
                state_descr[sdp->state].sw_alwd(me, me->ctx.cur_state))
                vdev_set_state(&(me->ctx), sdp->state);

            val = state_descr[sdp->state].sw_alwd(me, sdp->state)? sdp->enabled_val
                                                                 : sdp->disabled_val;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == IST_XCDAC20_CHAN_RESET_STATE)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                vdev_set_state(&(me->ctx), IST_STATE_DETERMINE);
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == IST_XCDAC20_CHAN_RESET_C_ILKS)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                Drop_c_ilks(me);
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == IST_XCDAC20_CHAN_IS_ON)
        {
            val = (me->ctx.cur_state >= IST_STATE_SW_ON_ENABLE  &&
                   me->ctx.cur_state <= IST_STATE_IS_ON)
                ||
                  (me->ctx.cur_state >= IST_STATE_RV_ZERO  &&
                   me->ctx.cur_state <= IST_STATE_RV_CHK);
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == IST_XCDAC20_CHAN_IST_STATE)
        {
            val = me->ctx.cur_state;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x >= IST_XCDAC20_CHAN_C_ILK_base  &&
                 x <  IST_XCDAC20_CHAN_C_ILK_base + IST_XCDAC20_CHAN_C_ILK_count)
        {
            ReturnChanGroup(devid, x, 1,
                            me->c_ilks_vals + x - IST_XCDAC20_CHAN_C_ILK_base,
                            &zero_rflags);
        }
        else if (x == IST_XCDAC20_CHAN_CUR_POLARITY)
        {
            ReturnChanGroup(devid, x, 1,
                            &(me->cur_sign),
                            &zero_rflags);
        }
        else if (x == IST_XCDAC20_CHAN_OUT_CUR  ||
                 x == IST_XCDAC20_CHAN_DCCT1    ||
                 x == IST_XCDAC20_CHAN_DCCT2)
            /* Returned from sodc_cb() */;
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec ist_xcdac20_main_info[] =
{
    {IST_XCDAC20_CHAN_WR_count, 1},
    {IST_XCDAC20_CHAN_RD_count, 0},
};
DEFINE_DRIVER(ist_xcdac20, "X-CDAC-controlled IST",
              ist_xcdac20_init_mod, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(ist_xcdac20_main_info), ist_xcdac20_main_info,
              -1, NULL,
              ist_xcdac20_init_d, ist_xcdac20_term_d,
              ist_xcdac20_rw_p, NULL);
