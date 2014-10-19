#include <stdio.h>

#include "timeval_utils.h"

#include "cxsd_driver.h"
#include "vdev.h"
#include "drv_i/v3h_xa40d16_drv_i.h"
#include "drv_i/xcanadc40_drv_i.h"
#include "drv_i/xcandac16_drv_i.h"


enum
{
    XA40_BASE = 0,
    XD16_BASE = XCANADC40_NUMCHANS,

    BA40_DMS      = XCANADC40_CHAN_ADC_n_base + 0,
    BA40_STS      = XCANADC40_CHAN_ADC_n_base + 1,
    BA40_MES      = XCANADC40_CHAN_ADC_n_base + 2,
    BA40_MU1      = XCANADC40_CHAN_ADC_n_base + 3,
    BA40_MU2      = XCANADC40_CHAN_ADC_n_base + 4,
    BA40_ILK      = XCANADC40_CHAN_REGS_RD8_base,
    BA40_SW_OFF   = XCANADC40_CHAN_REGS_WR8_base,

    BD16_OUT      = XCANDAC16_CHAN_OUT_n_base,
    BD16_OUT_RATE = XCANDAC16_CHAN_OUT_RATE_n_base,
    BD16_OUT_CUR  = XCANDAC16_CHAN_OUT_CUR_n_base,
    BD16_OPR      = XCANDAC16_CHAN_REGS_RD8_base,
    BD16_SW_ON    = XCANDAC16_CHAN_REGS_WR8_base,

    SUBORD_NUMCHANS = XCANADC40_NUMCHANS + XCANDAC16_NUMCHANS
};

enum
{
    OBS_A40 = 0,
    OBS_D16 = 1,
};

enum {EIGHTDEVS = 8};

#define C3H_DMS(     n) (XA40_BASE + BA40_DMS      + (n) * 5)
#define C3H_STS(     n) (XA40_BASE + BA40_STS      + (n) * 5)
#define C3H_MES(     n) (XA40_BASE + BA40_MES      + (n) * 5)
#define C3H_MU1(     n) (XA40_BASE + BA40_MU1      + (n) * 5)
#define C3H_MU2(     n) (XA40_BASE + BA40_MU2      + (n) * 5)
#define C3H_ILK(     n) (XA40_BASE + BA40_ILK      + (n))
#define C3H_SW_OFF(  n) (XA40_BASE + BA40_SW_OFF   + (n))

#define C3H_OUT(     n) (XD16_BASE + BD16_OUT      + (n))
#define C3H_OUT_RATE(n) (XD16_BASE + BD16_OUT_RATE + (n))
#define C3H_OUT_CUR( n) (XD16_BASE + BD16_OUT_CUR  + (n))
#define C3H_OPR(     n) (XD16_BASE + BD16_OPR      + (n))
#define C3H_SW_ON(   n) (XD16_BASE + BD16_SW_ON    + (n))

static int C3H_0toN(int c, int n)
{
    switch (c)
    {
        case C3H_DMS     (0): return C3H_DMS     (n);
        case C3H_STS     (0): return C3H_STS     (n);
        case C3H_MES     (0): return C3H_MES     (n);
        case C3H_MU1     (0): return C3H_MU1     (n);
        case C3H_MU2     (0): return C3H_MU2     (n);
        case C3H_ILK     (0): return C3H_ILK     (n);
        case C3H_SW_OFF  (0): return C3H_SW_OFF  (n);

        case C3H_OUT     (0): return C3H_OUT     (n);
        case C3H_OUT_RATE(0): return C3H_OUT_RATE(n);
        case C3H_OUT_CUR (0): return C3H_OUT_CUR (n);
        case C3H_OPR     (0): return C3H_OPR     (n);
        case C3H_SW_ON   (0): return C3H_SW_ON   (n);
    }

    return -1;
}


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
    V3H_STATE_UNKNOWN   = 0,
    V3H_STATE_DETERMINE,         // 1
    V3H_STATE_INTERLOCK,         // 2
    zV3H_STATE_RST_ILK_SET,      // 3
    zV3H_STATE_RST_ILK_DRP,      // 4
    zV3H_STATE_RST_ILK_CHK,      // 5

    V3H_STATE_IS_OFF,            // 6

    V3H_STATE_SW_ON_ENABLE,      // 7
    V3H_STATE_SW_ON_SET_ON,      // 8
    V3H_STATE_SW_ON_DRP_ON,      // 9
    V3H_STATE_SW_ON_UP,          // 10

    V3H_STATE_IS_ON,             // 11

    V3H_STATE_SW_OFF_DOWN,       // 12
    V3H_STATE_SW_OFF_CHK_I,      // 13
    V3H_STATE_SW_OFF_CHK_E,      // 14

    V3H_STATE_count
};

typedef struct
{
    int                 devid;

    vdev_context_t      ctx;
    vdev_observer_t     obs[2];
    vdev_trgc_cur_t     cur[SUBORD_NUMCHANS];

    int                 vn;

    int32               out_val;
    int                 out_val_set;
    
    int32               c_ilk;
} privrec_t;


static vdev_trgc_dsc_t xcdac2v3h_mapping_model      [SUBORD_NUMCHANS] =
{
    [C3H_OUT     (0)] = {VDEV_IMPR,             -1},
    [C3H_OUT_RATE(0)] = {            VDEV_TUBE, V3H_XA40D16_CHAN_OUT_RATE},
    [C3H_OUT_CUR (0)] = {VDEV_IMPR | VDEV_TUBE, V3H_XA40D16_CHAN_OUT_CUR},
    [C3H_OPR     (0)] = {VDEV_IMPR | VDEV_TUBE, V3H_XA40D16_CHAN_OPR},
    [C3H_SW_OFF  (0)] = {VDEV_PRIV,             -1},

    [C3H_ILK     (0)] = {VDEV_IMPR | VDEV_TUBE, V3H_XA40D16_CHAN_ILK},
    [C3H_SW_ON   (0)] = {VDEV_PRIV,             -1},
    [C3H_DMS     (0)] = {            VDEV_TUBE, V3H_XA40D16_CHAN_DMS},
    [C3H_STS     (0)] = {            VDEV_TUBE, V3H_XA40D16_CHAN_STS},
    [C3H_MES     (0)] = {            VDEV_TUBE, V3H_XA40D16_CHAN_MES},
    [C3H_MU1     (0)] = {            VDEV_TUBE, V3H_XA40D16_CHAN_MU1},
    [C3H_MU2     (0)] = {            VDEV_TUBE, V3H_XA40D16_CHAN_MU2},
};
static vdev_trgc_dsc_t xcdac2v3h_mappings[EIGHTDEVS][SUBORD_NUMCHANS];

static inline void SndCVal(privrec_t *me, int sodc, int32 val)
{
  int obs_n = OBS_A40;

    if (sodc >= XD16_BASE)
    {
        obs_n = OBS_D16;
        sodc -= XD16_BASE;
    }
    fprintf(stderr, "%s ->%d.%d =%d\n", __FUNCTION__, obs_n, sodc, val);
    vdev_observer_snd_chan(me->obs + obs_n, sodc, val);
}
static inline void ReqCVal(privrec_t *me, int sodc)
{
  int obs_n = OBS_A40;

    if (sodc >= XD16_BASE)
    {
        obs_n = OBS_D16;
        sodc -= XD16_BASE;
    }
    vdev_observer_req_chan(me->obs + obs_n, sodc);
}

static int ourc2sodc[EIGHTDEVS][V3H_XA40D16_NUMCHANS];

//////////////////////////////////////////////////////////////////////

static void SwchToUNKNOWN(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    vdev_observer_forget_all(me->obs); /*!!! Forget "all" -- which of two? */
}

static void SwchToDETERMINE(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

}

static void SwchToINTERLOCK(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C3H_OUT   (me->vn), 0); // Drop setting
    SndCVal(me, C3H_SW_ON (me->vn), 0); // Drop "on" bit
    SndCVal(me, C3H_SW_OFF(me->vn), 0); // Set "off"
}

static int  IsAlwdSW_ON_ENABLE(void *devptr, int prev_state)
{
  privrec_t *me = devptr;
  int        ilk;

    ilk = me->cur[C3H_ILK    (me->vn)].val;

    return
        (prev_state == V3H_STATE_IS_OFF  ||
         prev_state == V3H_STATE_INTERLOCK)    &&
        me->cur[C3H_OUT_CUR(me->vn)].val == 0  &&
        ilk == 0;
}

static void SwchToON_ENABLE(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C3H_SW_OFF(me->vn), 1); // Drop "off"
}

static void SwchToON_SET_ON(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C3H_SW_ON (me->vn), 1); // Set "on" bit
}

static void SwchToON_DRP_ON(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C3H_SW_ON (me->vn), 1); // Drop "on" bit
}

/* 26.09.2012: added to proceed to UP only if OPR is really on */
static int  IsAlwdSW_ON_UP(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return me->cur[C3H_OPR(me->vn)].val != 0;
}

static void SwchToON_UP    (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C3H_OUT   (me->vn), me->out_val);
}

static int  IsAlwdIS_ON    (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    return (abs(me->cur[C3H_OUT_CUR(me->vn)].val) - me->out_val) <= 305;
}

static int  IsAlwdSW_OFF_DOWN(void *devptr, int prev_state)
{
  privrec_t *me = devptr;

    return
        /* Regular "ON" state... */
        prev_state == V3H_STATE_IS_ON         ||
        /* ...and "switching-to-on" stages */
        prev_state == V3H_STATE_SW_ON_ENABLE  ||
        prev_state == V3H_STATE_SW_ON_SET_ON  ||
        prev_state == V3H_STATE_SW_ON_DRP_ON  ||
        prev_state == V3H_STATE_SW_ON_UP      ||
        /* ...or the state bit is just "on" */
        me->cur[C3H_OPR(me->vn)].val != 0
        ;
}

static void SwchToSW_OFF_DOWN(void *devptr, int prev_state)
{
  privrec_t *me = devptr;

    SndCVal(me, C3H_OUT   (me->vn), 0);
    if (prev_state != V3H_STATE_IS_ON) SndCVal(me, C3H_SW_ON (me->vn), 0);
}

static int  IsAlwdSW_OFF_CHK_I(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    fprintf(stderr, "\tout_cur=%d\n", me->cur[C3H_OUT_CUR(me->vn)].val);
    return me->cur[C3H_OUT_CUR(me->vn)].val == 0;
}

static void SwchToSW_OFF_CHK_I(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    SndCVal(me, C3H_SW_OFF(me->vn), 0); // Set "off"
}

static int  IsAlwdSW_OFF_CHK_E(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;

    fprintf(stderr, "\topr=%d\n", me->cur[C3H_OPR(me->vn)].val);
    return me->cur[C3H_OPR(me->vn)].val == 0;
}

// Somehow this hack DOES work: we pass macro-name as an argument, and that named macro is expanded with 8 different parameters
#define ZZZ(m) m(0), m(1), m(2), m(3), m(4), m(5), m(6), m(7)

static vdev_state_dsc_t state_descr[] =
{
    [V3H_STATE_UNKNOWN]      = {0,       -1,                     NULL,               SwchToUNKNOWN,      NULL},
    [V3H_STATE_DETERMINE]    = {0,       -1,                     NULL,               SwchToDETERMINE,    NULL},
    [V3H_STATE_INTERLOCK]    = {0,       -1,                     NULL,               SwchToINTERLOCK,    NULL},

    [V3H_STATE_IS_OFF]       = {0,       -1,                     NULL,               NULL,               NULL},

    [V3H_STATE_SW_ON_ENABLE] = { 500000, V3H_STATE_SW_ON_SET_ON, IsAlwdSW_ON_ENABLE, SwchToON_ENABLE,    NULL},
    [V3H_STATE_SW_ON_SET_ON] = {1500000, V3H_STATE_SW_ON_DRP_ON, NULL,               SwchToON_SET_ON,    NULL},
    [V3H_STATE_SW_ON_DRP_ON] = { 500000, V3H_STATE_SW_ON_UP,     NULL,               SwchToON_DRP_ON,    NULL},
    [V3H_STATE_SW_ON_UP]     = {1,       V3H_STATE_IS_ON,        IsAlwdSW_ON_UP,     SwchToON_UP,        NULL},

    [V3H_STATE_IS_ON]        = {0,       -1,                     IsAlwdIS_ON,        NULL,               (int[]){ZZZ(C3H_OUT_CUR), -1}},

    [V3H_STATE_SW_OFF_DOWN]  = {0,       V3H_STATE_SW_OFF_CHK_I, IsAlwdSW_OFF_DOWN,  SwchToSW_OFF_DOWN,  NULL},
    [V3H_STATE_SW_OFF_CHK_I] = {0,       V3H_STATE_SW_OFF_CHK_E, IsAlwdSW_OFF_CHK_I, SwchToSW_OFF_CHK_I, (int[]){ZZZ(C3H_OUT_CUR), -1}},
    [V3H_STATE_SW_OFF_CHK_E] = {1,       V3H_STATE_IS_OFF,       IsAlwdSW_OFF_CHK_E, NULL,               (int[]){ZZZ(C3H_OPR),     -1}},
};

static int state_important_channels[countof(state_descr)][SUBORD_NUMCHANS];

//////////////////////////////////////////////////////////////////////

static int v3h_xa40d16_init_mod(void)
{
  int  vn;
  int  sn;
  int  x;
  int  sodc;

    /* Fill interesting_chan[][] mapping */
    bzero(state_important_channels, sizeof(state_important_channels));
    for (sn = 0;  sn < countof(state_descr);  sn++)
        if (state_descr[sn].impr_chans != NULL)
            for (x = 0;  state_descr[sn].impr_chans[x] >= 0;  x++)
                if (C3H_0toN(state_descr[sn].impr_chans[x], 0) >= 0) // This checks that we deal with vn=0-channel
                    for (vn = 0;  vn < EIGHTDEVS;  vn++)
                        state_important_channels[sn]
                            [
                             C3H_0toN(state_descr[sn].impr_chans[x], vn)
                            ] = 1;

    /* ...and ourc->sodc mapping too */
    for (vn = 0;  vn < EIGHTDEVS;  vn++)
    {
        for (x = 0;  x < countof(ourc2sodc[vn]);  x++)
            ourc2sodc[vn][x] = -1;
        for (x = 0;  x < countof(xcdac2v3h_mapping_model);  x++)
            if (xcdac2v3h_mapping_model[x].mode != 0)
            {
                sodc = C3H_0toN(x, vn);
                xcdac2v3h_mappings[vn][sodc] = xcdac2v3h_mapping_model[x];
                if (xcdac2v3h_mapping_model[x].ourc >= 0)
                    ourc2sodc[vn][xcdac2v3h_mapping_model[x].ourc] = sodc;
            }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

static void v3h_xa40d16_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action);
static void v3h_xa40d16_sodc_cb(int devid, void *devptr,
                                int sodc, int32 val);

static vdev_sr_chan_dsc_t state_related_channels[] =
{
    {V3H_XA40D16_CHAN_SWITCH_ON,  V3H_STATE_SW_ON_ENABLE, 0,                 CX_VALUE_DISABLED_MASK},
    {V3H_XA40D16_CHAN_SWITCH_OFF, V3H_STATE_SW_OFF_DOWN,  0,                 CX_VALUE_DISABLED_MASK},
    {V3H_XA40D16_CHAN_IS_ON,      -1,                     0,                 0},
    {-1,                          -1,                     0,                 0},
};

static void mem2strzcpy(char *dest, const char *src, size_t n, size_t len)
{
    if (len > n - 1) len = n - 1;
    if (len > 0) memcpy(dest, src, len);
    dest[len] = '\0';
}

static int v3h_xa40d16_init_d(int devid, void *devptr,
                              int businfocount, int businfo[],
                              const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;
  const char     *p  = auxinfo;
  const char     *endp;

    me->devid = devid;

    if (p == NULL  ||  *p == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s(): XCANADC40_name,XCANDAC16_name:VCh300_number spec is required in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    endp = strchr(p, ',');
    if (endp == NULL)
    {
        DoDriverLog(devid, 0,
                    "%s(): ',' expected in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    mem2strzcpy(me->obs[0].targ_name, p, sizeof(me->obs[0].targ_name), endp - p);

    p = endp + 1;
    endp = strchr(p, ':');
    if (endp == NULL)
    {
        DoDriverLog(devid, 0,
                    "%s(): ':' expected in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    mem2strzcpy(me->obs[1].targ_name, p, sizeof(me->obs[1].targ_name), endp - p);

    p = endp + 1;
    if (*p < '0'  ||  *p >= '0' + EIGHTDEVS)
    {
        DoDriverLog(devid, 0,
                    "%s(): ':N' (N in [0...%d]) expected in auxinfo",
                    __FUNCTION__, EIGHTDEVS);
        return -CXRF_CFG_PROBL;
    }
    me->vn = *p - '0';
    fprintf(stderr, "[%d]: <%s>,<%s>,%d\n", devid, me->obs[0].targ_name, me->obs[1].targ_name, me->vn);

    me->obs[0].m_count       = 2;
    me->obs[0].targ_numchans = XCANADC40_NUMCHANS;
    me->obs[0].map           = xcdac2v3h_mappings[me->vn] + XA40_BASE;
    me->obs[0].cur           = me->cur                    + XA40_BASE;
    me->obs[1].targ_numchans = XCANDAC16_NUMCHANS;
    me->obs[1].map           = xcdac2v3h_mappings[me->vn] + XD16_BASE;
    me->obs[1].cur           = me->cur                    + XD16_BASE;

    me->ctx.chan_state_n             = V3H_XA40D16_CHAN_V3H_STATE;
    me->ctx.state_unknown_val        = V3H_STATE_UNKNOWN;
    me->ctx.state_determine_val      = V3H_STATE_DETERMINE;
    me->ctx.state_descr              = state_descr;
    me->ctx.state_count              = countof(state_descr);

    me->ctx.state_related_channels   = state_related_channels;
    me->ctx.state_important_channels = state_important_channels/*[me->vn]*/;
    me->ctx.do_rw                    = v3h_xa40d16_rw_p;
    me->ctx.sodc_cb                  = v3h_xa40d16_sodc_cb;

    { int r;
    r = vdev_init(&(me->ctx), me->obs,
                     devid, devptr,
                     BIND_HEARTBEAT_PERIOD, WORK_HEARTBEAT_PERIOD);
fprintf(stderr, "INIT=%d!!!\n", r);
return r;
    }
}

static void v3h_xa40d16_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    vdev_fini(&(me->ctx));
}

static void v3h_xa40d16_sodc_cb(int devid, void *devptr,
                                int sodc, int32 val)
{
  privrec_t      *me = (privrec_t *)devptr;

    /* A special case -- derive OUT from OUT_CUR upon start */
    if (sodc == C3H_OUT_CUR(me->vn)  &&  !me->out_val_set)
    {
        me->out_val     = val;
        me->out_val_set = 1; /* Yes, we do it only ONCE, since later OUT_CUR may change upon our request, without any client's write */
        ReturnChanGroup(devid, V3H_XA40D16_CHAN_OUT, 1, &(me->out_val), &zero_rflags);
    }

    /* If not all info is gathered yet, then there's nothing else to do yet */
    if (me->ctx.cur_state == V3H_STATE_UNKNOWN) return;

    /* In fact, is a hack... */
    if (me->ctx.cur_state == V3H_STATE_DETERMINE)
    {
        if (me->cur[C3H_OPR(me->vn)].val)
            vdev_set_state(&(me->ctx), V3H_STATE_IS_ON);
        else
            vdev_set_state(&(me->ctx), V3H_STATE_IS_OFF);
    }
    
    /* Perform state transitions depending on received data */
    if (sodc == C3H_ILK(me->vn)  &&  val != 0)
    {
        if (me->c_ilk != val)
        {
            me->c_ilk = val;
            ReturnChanGroup(devid, V3H_XA40D16_CHAN_ILK, 1, &val, &zero_rflags);
        }
        vdev_set_state(&(me->ctx), V3H_STATE_INTERLOCK);
    }
    if (/* 26.09.2012: when OPR bit disappears, we should better switch off */
        sodc == C3H_OPR(me->vn)  &&  val == 0  &&
        (me->ctx.cur_state == V3H_STATE_SW_ON_DRP_ON  ||
         me->ctx.cur_state == V3H_STATE_SW_ON_UP      ||
         me->ctx.cur_state == V3H_STATE_IS_ON))
    {
        vdev_set_state(&(me->ctx), V3H_STATE_SW_OFF_DOWN);
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

static void v3h_xa40d16_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action)
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

        sodc = ourc2sodc[me->vn][x];

        if      (sodc >= 0  &&  xcdac2v3h_mappings[me->vn][sodc].mode & VDEV_TUBE)
        {
            if (action == DRVA_WRITE)
                SndCVal(me, sodc, val);
            else
                ReqCVal(me, sodc);
        }
        else if (x == V3H_XA40D16_CHAN_OUT)
        {
         fprintf(stderr, "OUT action=%d\n", action);
            if (action == DRVA_WRITE)
            {
                if (val > MAX_ALWD_VAL) val = MAX_ALWD_VAL;
                if (val < MIN_ALWD_VAL) val = MIN_ALWD_VAL;
                me->out_val     = val;
                me->out_val_set = 1;

                if (me->ctx.cur_state == V3H_STATE_IS_ON  ||
                    me->ctx.cur_state == V3H_STATE_SW_ON_UP)
                {
                    SndCVal(me, C3H_OUT(me->vn), me->out_val);
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
        else if (x == V3H_XA40D16_CHAN_RESET_STATE)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                vdev_set_state(&(me->ctx), V3H_STATE_DETERMINE);
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == V3H_XA40D16_CHAN_RESET_C_ILKS)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                me->c_ilk = 0;
                v3h_xa40d16_rw_p(devid, devptr, x, 1, NULL, DRVA_READ);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == V3H_XA40D16_CHAN_IS_ON)
        {
            val = (me->ctx.cur_state >= V3H_STATE_SW_ON_ENABLE  &&
                   me->ctx.cur_state <= V3H_STATE_IS_ON);
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == V3H_XA40D16_CHAN_V3H_STATE)
        {
            val = me->ctx.cur_state;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x >= V3H_XA40D16_CHAN_C_ILK)
        {
            ReturnChanGroup(devid, x, 1,
                            &(me->c_ilk),
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

static CxsdMainInfoRec v3h_xa40d16_main_info[] =
{
    {V3H_XA40D16_CHAN_WR_count, 1},
    {V3H_XA40D16_CHAN_RD_count, 0},
};
DEFINE_DRIVER(v3h_xa40d16, "X-CANADC40+CANDAC16-controlled VCh-300",
              v3h_xa40d16_init_mod, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(v3h_xa40d16_main_info), v3h_xa40d16_main_info,
              -1, NULL,
              v3h_xa40d16_init_d, v3h_xa40d16_term_d,
              v3h_xa40d16_rw_p, NULL);
