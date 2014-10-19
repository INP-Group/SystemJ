#include <stdio.h>

#include "timeval_utils.h"

#include "cxsd_driver.h"
#include "vdev.h"
#include "drv_i/v1k_xcdac20_drv_i.h"
#include "drv_i/xcdac20_drv_i.h"


enum
{
    C20C_OUT            = XCDAC20_CHAN_OUT_n_base      + 0,
    C20C_OUT_RATE       = XCDAC20_CHAN_OUT_RATE_n_base + 0,
    C20C_SW_ON          = XCDAC20_CHAN_REGS_WR8_base + 0,
    C20C_SW_OFF         = XCDAC20_CHAN_REGS_WR8_base + 1,
    C20C_RST_ILKS       = XCDAC20_CHAN_REGS_WR8_base + 2,

    C20C_DO_CALB_DAC    = XCDAC20_CHAN_DO_CALB_DAC,
    C20C_AUTOCALB_ONOFF = XCDAC20_CHAN_AUTOCALB_ONOFF,
    C20C_AUTOCALB_SECS  = XCDAC20_CHAN_AUTOCALB_SECS,
    C20C_DIGCORR_MODE   = XCDAC20_CHAN_DIGCORR_MODE,

    C20C_CURRENT1M      = XCDAC20_CHAN_ADC_n_base + 0,
    C20C_VOLTAGE2M      = XCDAC20_CHAN_ADC_n_base + 1,
    C20C_CURRENT2M      = XCDAC20_CHAN_ADC_n_base + 2,
    C20C_OUTVOLTAGE1M   = XCDAC20_CHAN_ADC_n_base + 3,
    C20C_OUTVOLTAGE2M   = XCDAC20_CHAN_ADC_n_base + 4,
    C20C_ADC_DAC        = XCDAC20_CHAN_ADC_n_base + 5,
    C20C_ADC_0V         = XCDAC20_CHAN_ADC_n_base + 6,
    C20C_ADC_P10V       = XCDAC20_CHAN_ADC_n_base + 7,
    C20C_OUT_CUR        = XCDAC20_CHAN_OUT_CUR_n_base + 0,

    C20C_DIGCORR_FACTOR = XCDAC20_CHAN_DIGCORR_FACTOR,

    C20C_OPR            = XCDAC20_CHAN_REGS_RD8_base + 4,
    C20C_ILK_base       = XCDAC20_CHAN_REGS_RD8_base + 0,
      C20C_ILK_count    = 4,
    C20C_ILK_OVERHEAT   = C20C_ILK_base + 0,
    C20C_ILK_EMERGSHT   = C20C_ILK_base + 1,
    C20C_ILK_CURRPROT   = C20C_ILK_base + 2,
    C20C_ILK_LOADOVRH   = C20C_ILK_base + 3,

    SUBORD_NUMCHANS = XCDAC20_NUMCHANS
};


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
    V1K_STATE_UNKNOWN   = 0,
    V1K_STATE_DETERMINE,         // 1
    V1K_STATE_INTERLOCK,         // 2
    V1K_STATE_RST_ILK_SET,       // 3
    V1K_STATE_RST_ILK_DRP,       // 4
    V1K_STATE_RST_ILK_CHK,       // 5

    V1K_STATE_IS_OFF,            // 6

    V1K_STATE_SW_ON_ENABLE,      // 7
    V1K_STATE_SW_ON_SET_ON,      // 8
    V1K_STATE_SW_ON_DRP_ON,      // 9
    V1K_STATE_SW_ON_UP,          // 10

    V1K_STATE_IS_ON,             // 11

    V1K_STATE_SW_OFF_DOWN,       // 12
    V1K_STATE_SW_OFF_CHK_I,      // 13
    V1K_STATE_SW_OFF_CHK_E,      // 14

    V1K_STATE_count
};


typedef struct
{
    int                 devid;

    vdev_context_t      ctx;
    vdev_observer_t     obs;
    vdev_trgc_cur_t     cur[SUBORD_NUMCHANS];

    int32               out_val;
    int                 out_val_set;
    
    int32               c_ilks_vals[V1K_XCDAC20_CHAN_C_ILK_count];
} privrec_t;


static vdev_trgc_dsc_t xcdac2v1k_mapping[SUBORD_NUMCHANS] =
{
    [C20C_OUT]            = {VDEV_IMPR,             -1},
    [C20C_OUT_RATE]       = {            VDEV_TUBE, V1K_XCDAC20_CHAN_OUT_RATE},
    [C20C_SW_ON]          = {VDEV_PRIV,             -1},
    [C20C_SW_OFF]         = {VDEV_PRIV,             -1},
    [C20C_RST_ILKS]       = {VDEV_PRIV,             -1},

    [C20C_DO_CALB_DAC]    = {            VDEV_TUBE, V1K_XCDAC20_CHAN_DO_CALB_DAC},
    [C20C_AUTOCALB_ONOFF] = {            VDEV_TUBE, V1K_XCDAC20_CHAN_AUTOCALB_ONOFF},
    [C20C_AUTOCALB_SECS]  = {            VDEV_TUBE, V1K_XCDAC20_CHAN_AUTOCALB_SECS},
    [C20C_DIGCORR_MODE]   = {            VDEV_TUBE, V1K_XCDAC20_CHAN_DIGCORR_MODE},

    [C20C_CURRENT1M]      = {            VDEV_TUBE, V1K_XCDAC20_CHAN_CURRENT1M},
    [C20C_VOLTAGE2M]      = {            VDEV_TUBE, V1K_XCDAC20_CHAN_VOLTAGE2M},
    [C20C_CURRENT2M]      = {            VDEV_TUBE, V1K_XCDAC20_CHAN_CURRENT2M},
    [C20C_OUTVOLTAGE1M]   = {            VDEV_TUBE, V1K_XCDAC20_CHAN_OUTVOLTAGE1M},
    [C20C_OUTVOLTAGE2M]   = {            VDEV_TUBE, V1K_XCDAC20_CHAN_OUTVOLTAGE2M},
    [C20C_ADC_DAC]        = {            VDEV_TUBE, V1K_XCDAC20_CHAN_ADC_DAC},
    [C20C_ADC_0V]         = {            VDEV_TUBE, V1K_XCDAC20_CHAN_ADC_0V},
    [C20C_ADC_P10V]       = {            VDEV_TUBE, V1K_XCDAC20_CHAN_ADC_P10V},
    [C20C_OUT_CUR]        = {VDEV_IMPR | VDEV_TUBE, V1K_XCDAC20_CHAN_OUT_CUR},

    [C20C_DIGCORR_FACTOR] = {            VDEV_TUBE, V1K_XCDAC20_CHAN_DIGCORR_FACTOR},

    [C20C_OPR]            = {VDEV_IMPR | VDEV_TUBE, V1K_XCDAC20_CHAN_OPR},
    [C20C_ILK_OVERHEAT]   = {VDEV_IMPR | VDEV_TUBE, V1K_XCDAC20_CHAN_ILK_OVERHEAT},
    [C20C_ILK_EMERGSHT]   = {VDEV_IMPR | VDEV_TUBE, V1K_XCDAC20_CHAN_ILK_EMERGSHT},
    [C20C_ILK_CURRPROT]   = {VDEV_IMPR | VDEV_TUBE, V1K_XCDAC20_CHAN_ILK_CURRPROT},
    [C20C_ILK_LOADOVRH]   = {VDEV_IMPR | VDEV_TUBE, V1K_XCDAC20_CHAN_ILK_LOADOVRH},
};                               


static inline void SndCVal(privrec_t *me, int sodc, int32 val)
{
    vdev_observer_snd_chan(&(me->obs), sodc, val);
}
static inline void ReqCVal(privrec_t *me, int sodc)
{
    vdev_observer_req_chan(&(me->obs), sodc);
}

static int ourc2sodc[V1K_XCDAC20_NUMCHANS];

//////////////////////////////////////////////////////////////////////

static void SwchToUNKNOWN(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    vdev_observer_forget_all(&me->obs);
}

static void SwchToDETERMINE(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

}

static void SwchToINTERLOCK(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_OUT,       0);
    SndCVal(me, C20C_SW_ON,     0);
    SndCVal(me, C20C_SW_OFF,    1);
}

static int  IsAlwdRST_ILK_SET(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return me->ctx.cur_state == V1K_STATE_INTERLOCK
        /* 26.09.2012: following added to allow [Reset] in other cases */
        || (me->ctx.cur_state >= V1K_STATE_RST_ILK_SET  &&  me->ctx.cur_state <= V1K_STATE_RST_ILK_CHK)
        ;
}

static int  IsAlwdRST_ILK_CHK(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
  int        sodc;

    for (sodc = C20C_ILK_base;  sodc < C20C_ILK_base + C20C_ILK_count;  sodc++)
        if (me->cur[sodc].val == 0) return 0;

    return 1;
}

static void Drop_c_ilks(privrec_t *me)
{
  int  x;

    for (x = 0;  x < V1K_XCDAC20_CHAN_C_ILK_count;  x++)
    {
        me->c_ilks_vals[x] = 0;
        ReturnChanGroup(me->devid, V1K_XCDAC20_CHAN_C_ILK_base + x, 1,
                        me->c_ilks_vals + x, &zero_rflags);
    }
}

static void SwchToRST_ILK_SET(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    Drop_c_ilks(me);
    SndCVal(me, C20C_RST_ILKS, 1);
}

static void SwchToRST_ILK_DRP(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_RST_ILKS, 0);
}

static int  IsAlwdSW_ON_ENABLE(void *devptr, int prev_state)
{
  privrec_t *me = devptr;
  int        ilk;
  int        x;

    for (x = 0, ilk = 0;  x < C20C_ILK_count;  x++)
        ilk |= !(me->cur[C20C_ILK_base + x].val); // Note: Invert

    return
        (prev_state == V1K_STATE_IS_OFF  ||
         prev_state == V1K_STATE_INTERLOCK)  &&
        me->cur[C20C_OUT_CUR].val == 0       &&
        ilk == 0;
}

static void SwchToON_ENABLE(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_SW_OFF, 0);
}

static void SwchToON_SET_ON(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_SW_ON,  1);
}

static void SwchToON_DRP_ON(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_SW_ON,  0);
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

    SndCVal(me, C20C_OUT,       me->out_val);
}

static int  IsAlwdIS_ON    (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return (abs(me->cur[C20C_OUT_CUR].val) - me->out_val) <= 305;
}

static int  IsAlwdSW_OFF_DOWN(void *devptr, int prev_state)
{
  privrec_t *me = devptr;

    return
        /* Regular "ON" state... */
        prev_state == V1K_STATE_IS_ON         ||
        /* ...and "switching-to-on" stages */
        prev_state == V1K_STATE_SW_ON_ENABLE  ||
        prev_state == V1K_STATE_SW_ON_SET_ON  ||
        prev_state == V1K_STATE_SW_ON_DRP_ON  ||
        prev_state == V1K_STATE_SW_ON_UP      ||
        /* ...or the state bit is just "on" */
        me->cur[C20C_OPR].val != 0
        ;
}

static void SwchToSW_OFF_DOWN(void *devptr, int prev_state)
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_OUT,    0);
    if (prev_state != V1K_STATE_IS_ON) SndCVal(me, C20C_SW_ON, 0);
}

static int  IsAlwdSW_OFF_CHK_I(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    fprintf(stderr, "\tout_cur=%d\n", me->cur[C20C_OUT_CUR].val);
    return me->cur[C20C_OUT_CUR].val == 0;
}

static void SwchToSW_OFF_CHK_I(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C20C_SW_ON,  0);
    SndCVal(me, C20C_SW_OFF, 1);
}

static int  IsAlwdSW_OFF_CHK_E(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    fprintf(stderr, "\topr=%d\n", me->cur[C20C_OPR].val);
    return me->cur[C20C_OPR].val == 0;
}

static vdev_state_dsc_t state_descr[] =
{
    [V1K_STATE_UNKNOWN]      = {0,       -1,                     NULL,               SwchToUNKNOWN,      NULL},
    [V1K_STATE_DETERMINE]    = {0,       -1,                     NULL,               SwchToDETERMINE,    NULL},
    [V1K_STATE_INTERLOCK]    = {0,       -1,                     NULL,               SwchToINTERLOCK,    NULL},

    [V1K_STATE_RST_ILK_SET]  = {500000,  V1K_STATE_RST_ILK_DRP,  IsAlwdRST_ILK_SET,  SwchToRST_ILK_SET,  NULL},
    [V1K_STATE_RST_ILK_DRP]  = {500000,  V1K_STATE_RST_ILK_CHK,  NULL,               SwchToRST_ILK_DRP,  NULL},
    [V1K_STATE_RST_ILK_CHK]  = {1,       V1K_STATE_IS_OFF,       IsAlwdRST_ILK_CHK,  NULL,               (int[]){C20C_ILK_OVERHEAT, C20C_ILK_EMERGSHT, C20C_ILK_CURRPROT, C20C_ILK_LOADOVRH, -1}},

    [V1K_STATE_IS_OFF]       = {0,       -1,                     NULL,               NULL,               NULL},

    [V1K_STATE_SW_ON_ENABLE] = { 500000, V1K_STATE_SW_ON_SET_ON, IsAlwdSW_ON_ENABLE, SwchToON_ENABLE,    NULL},
    [V1K_STATE_SW_ON_SET_ON] = {1500000, V1K_STATE_SW_ON_DRP_ON, NULL,               SwchToON_SET_ON,    NULL},
    [V1K_STATE_SW_ON_DRP_ON] = { 500000, V1K_STATE_SW_ON_UP,     NULL,               SwchToON_DRP_ON,    NULL},
    [V1K_STATE_SW_ON_UP]     = {1,       V1K_STATE_IS_ON,        IsAlwdSW_ON_UP,     SwchToON_UP,        NULL},

    [V1K_STATE_IS_ON]        = {0,       -1,                     IsAlwdIS_ON,        NULL,               (int[]){C20C_OUT_CUR, -1}},

    [V1K_STATE_SW_OFF_DOWN]  = {0,       V1K_STATE_SW_OFF_CHK_I, IsAlwdSW_OFF_DOWN,  SwchToSW_OFF_DOWN,  NULL},
    [V1K_STATE_SW_OFF_CHK_I] = {0,       V1K_STATE_SW_OFF_CHK_E, IsAlwdSW_OFF_CHK_I, SwchToSW_OFF_CHK_I, (int[]){C20C_OUT_CUR, -1}},
    [V1K_STATE_SW_OFF_CHK_E] = {1,       V1K_STATE_IS_OFF,       IsAlwdSW_OFF_CHK_E, NULL,               (int[]){C20C_OPR, -1}},
};

static int state_important_channels[countof(state_descr)][SUBORD_NUMCHANS];

//////////////////////////////////////////////////////////////////////

static int v1k_xcdac20_init_mod(void)
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
    for (x = 0;  x < countof(xcdac2v1k_mapping);  x++)
        if (xcdac2v1k_mapping[x].mode != 0  &&  xcdac2v1k_mapping[x].ourc >= 0)
            ourc2sodc[xcdac2v1k_mapping[x].ourc] = x;

    return 0;
}

//////////////////////////////////////////////////////////////////////

static void v1k_xcdac20_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action);
static void v1k_xcdac20_sodc_cb(int devid, void *devptr,
                                int sodc, int32 val);

static vdev_sr_chan_dsc_t state_related_channels[] =
{
    {V1K_XCDAC20_CHAN_SWITCH_ON,  V1K_STATE_SW_ON_ENABLE, 0,                 CX_VALUE_DISABLED_MASK},
    {V1K_XCDAC20_CHAN_SWITCH_OFF, V1K_STATE_SW_OFF_DOWN,  0,                 CX_VALUE_DISABLED_MASK},
    {V1K_XCDAC20_CHAN_RST_ILKS,   V1K_STATE_RST_ILK_SET,  CX_VALUE_LIT_MASK, 0},
    {V1K_XCDAC20_CHAN_IS_ON,      -1,                     0,                 0},
    {-1,                          -1,                     0,                 0},
};

static int v1k_xcdac20_init_d(int devid, void *devptr, 
                              int businfocount, int businfo[],
                              const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;
  const char     *p  = auxinfo;

    me->devid = devid;

    if (p == NULL  ||  *p == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s(): base-XCDAC20-device name is required in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    strzcpy(me->obs.targ_name, auxinfo, sizeof(me->obs.targ_name));
    me->obs.targ_numchans = countof(me->cur);
    me->obs.map           = xcdac2v1k_mapping;
    me->obs.cur           = me->cur;

    me->ctx.chan_state_n             = V1K_XCDAC20_CHAN_V1K_STATE;
    me->ctx.state_unknown_val        = V1K_STATE_UNKNOWN;
    me->ctx.state_determine_val      = V1K_STATE_DETERMINE;
    me->ctx.state_descr              = state_descr;
    me->ctx.state_count              = countof(state_descr);

    me->ctx.state_related_channels   = state_related_channels;
    me->ctx.state_important_channels = state_important_channels;
    me->ctx.do_rw                    = v1k_xcdac20_rw_p;
    me->ctx.sodc_cb                  = v1k_xcdac20_sodc_cb;

    return vdev_init(&(me->ctx), &(me->obs),
                     devid, devptr,
                     BIND_HEARTBEAT_PERIOD, WORK_HEARTBEAT_PERIOD);
}

static void v1k_xcdac20_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    vdev_fini(&(me->ctx));
}

static void v1k_xcdac20_sodc_cb(int devid, void *devptr,
                                int sodc, int32 val)
{
  privrec_t      *me = (privrec_t *)devptr;
  int             ilk_n;

    /* A special case -- derive OUT from OUT_CUR upon start */
    if (sodc == C20C_OUT_CUR  &&  !me->out_val_set)
    {
        me->out_val     = val;
        me->out_val_set = 1; /* Yes, we do it only ONCE, since later OUT_CUR may change upon our request, without any client's write */
        ReturnChanGroup(devid, V1K_XCDAC20_CHAN_OUT, 1, &(me->out_val), &zero_rflags);
    }

    /* If not all info is gathered yet, then there's nothing else to do yet */
    if (me->ctx.cur_state == V1K_STATE_UNKNOWN) return;

    /* In fact, is a hack... */
    if (me->ctx.cur_state == V1K_STATE_DETERMINE)
    {
        if (me->cur[C20C_OPR].val)
            vdev_set_state(&(me->ctx), V1K_STATE_IS_ON);
        else
            vdev_set_state(&(me->ctx), V1K_STATE_IS_OFF);
    }
    
    /* Perform state transitions depending on received data */
    if (sodc >= C20C_ILK_base  &&
        sodc <  C20C_ILK_base + C20C_ILK_count)
    {
        val = !val; // Invert
        ilk_n = sodc - C20C_ILK_base;
        /* "Tube" (inverted) first, ... */
        ReturnChanGroup(devid, V1K_XCDAC20_CHAN_ILK_base + ilk_n, 1, &val, &zero_rflags);
        if (val == 1  &&  me->c_ilks_vals[ilk_n] != val)
        {
            me->c_ilks_vals[ilk_n] = val;
            ReturnChanGroup(devid, V1K_XCDAC20_CHAN_C_ILK_base + ilk_n, 1, &val, &zero_rflags);
        }
        /* ...than check */
        if (val != 0 /* Note: this is INVERTED value */  &&
            ( // Lock-unaffected states
             me->ctx.cur_state != V1K_STATE_RST_ILK_SET  &&
             me->ctx.cur_state != V1K_STATE_RST_ILK_DRP  &&
             me->ctx.cur_state != V1K_STATE_RST_ILK_CHK
            )
           )
            vdev_set_state(&(me->ctx), V1K_STATE_INTERLOCK);
    }
    else if (/* 26.09.2012: when OPR bit disappears, we should better switch off */
             sodc == C20C_OPR  &&  val == 0  &&
             (me->ctx.cur_state == V1K_STATE_SW_ON_DRP_ON  ||
              me->ctx.cur_state == V1K_STATE_SW_ON_UP      ||
              me->ctx.cur_state == V1K_STATE_IS_ON))
    {
        vdev_set_state(&(me->ctx), V1K_STATE_SW_OFF_DOWN);
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

static void v1k_xcdac20_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action)
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

        if      (sodc >= 0  &&  xcdac2v1k_mapping[sodc].mode & VDEV_TUBE)
        {
            if (action == DRVA_WRITE)
                SndCVal(me, sodc, val);
            else
                ReqCVal(me, sodc);
        }
        else if (x == V1K_XCDAC20_CHAN_OUT)
        {
            if (action == DRVA_WRITE)
            {
                if (val > MAX_ALWD_VAL) val = MAX_ALWD_VAL;
                if (val < MIN_ALWD_VAL) val = MIN_ALWD_VAL;
                me->out_val     = val;
                me->out_val_set = 1;

                if (me->ctx.cur_state == V1K_STATE_IS_ON  ||
                    me->ctx.cur_state == V1K_STATE_SW_ON_UP)
                {
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
        else if (x == V1K_XCDAC20_CHAN_RESET_STATE)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                vdev_set_state(&(me->ctx), V1K_STATE_DETERMINE);
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == V1K_XCDAC20_CHAN_RESET_C_ILKS)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                Drop_c_ilks(me);
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == V1K_XCDAC20_CHAN_IS_ON)
        {
            val = (me->ctx.cur_state >= V1K_STATE_SW_ON_ENABLE  &&
                   me->ctx.cur_state <= V1K_STATE_IS_ON);
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == V1K_XCDAC20_CHAN_V1K_STATE)
        {
            val = me->ctx.cur_state;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x >= V1K_XCDAC20_CHAN_C_ILK_base  &&
                 x <  V1K_XCDAC20_CHAN_C_ILK_base + V1K_XCDAC20_CHAN_C_ILK_count)
        {
            ReturnChanGroup(devid, x, 1,
                            me->c_ilks_vals + x - V1K_XCDAC20_CHAN_C_ILK_base,
                            &zero_rflags);
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec v1k_xcdac20_main_info[] =
{
    {V1K_XCDAC20_CHAN_WR_count, 1},
    {V1K_XCDAC20_CHAN_RD_count, 0},
};
DEFINE_DRIVER(v1k_xcdac20, "X-CDAC-controlled V1000",
              v1k_xcdac20_init_mod, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(v1k_xcdac20_main_info), v1k_xcdac20_main_info,
              -1, NULL,
              v1k_xcdac20_init_d, v1k_xcdac20_term_d,
              v1k_xcdac20_rw_p, NULL);
