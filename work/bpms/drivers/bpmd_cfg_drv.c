#include <ctype.h>
#include <math.h>
#include <stdio.h>

#include "cxsd_driver.h"
#include "vdev.h"
#include "timeval_utils.h"
#include "memcasecmp.h"

#include "drv_i/bpmd_cfg_drv_i.h"

#include "drv_i/bpmd_drv_i.h"
//#include "drv_i/bpmd_dls_drv_i.h"

//////////////////////////////////////////////////////////////////////

// BCPC -- Bpm Config Param Channel
enum
{
    BCPC_SHOT       = BPMD_PARAM_SHOT,
    BCPC_STOP       = BPMD_PARAM_STOP,
    BCPC_ISTART     = BPMD_PARAM_ISTART,
    BCPC_WAITTM     = BPMD_PARAM_WAITTIME,
    BCPC_CALC_STATS = BPMD_PARAM_CALC_STATS,

    BCPC_NUMPTS     = BPMD_PARAM_NUMPTS,
    BCPC_PTSOFS     = BPMD_PARAM_PTSOFS,

    BCPC_DELAY      = BPMD_PARAM_DELAY,
    BCPC_ATTEN      = BPMD_PARAM_ATTEN,
    BCPC_DECMTN     = BPMD_PARAM_DECMTN,

    BCPC_AVG0       = BPMD_PARAM_AVG0,
    BCPC_AVG1       = BPMD_PARAM_AVG1,
    BCPC_AVG2       = BPMD_PARAM_AVG2,
    BCPC_AVG3       = BPMD_PARAM_AVG3,
    
    BCPC_ELAPSD     = BPMD_PARAM_ELAPSED,
    SUBORD_NUMCHANS = BPMD_NUM_PARAMS,
};

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

enum
{
    WORK_HEARTBEAT_PERIOD =      100 * 1000, // 100ms/10Hz
    BIND_HEARTBEAT_PERIOD = 2 * 1000 * 1000  // 2s
};

enum
{
    BPMD_STATE_UNKNOWN = 0,               // 0
    BPMD_STATE_DETERMN,                   // 1

    BPMD_STATE_AUTOMAT,                   // 2

    BPMD_STATE_ELCTRUN,                   // 3
    BPMD_STATE_ELCTINJ,                   // 4
    BPMD_STATE_ELCTDLS,                   // 5

    BPMD_STATE_PSTRRUN,                   // 6
    BPMD_STATE_PSTRINJ,                   // 7
    BPMD_STATE_PSTRDLS,                   // 8

    BPMD_STATE_FREERUN,                   // 9

    BPMD_STATE_ELCTRUN_SWITCH_ON,         // 10
    BPMD_STATE_ELCTRUN_STOP_PREV,         // 11
    BPMD_STATE_ELCTRUN_RD_PARAMS,         // 12
    BPMD_STATE_ELCTRUN_WR_PARAMS,         // 13
    BPMD_STATE_ELCTRUN_STRT_NEXT,         // 14

    BPMD_STATE_ELCTINJ_SWITCH_ON,         // 15
    BPMD_STATE_ELCTINJ_STOP_PREV,         // 16
    BPMD_STATE_ELCTINJ_WR_PARAMS,         // 17
    BPMD_STATE_ELCTINJ_STRT_NEXT,         // 18

    BPMD_STATE_PSTRRUN_SWITCH_ON,         // 19
    BPMD_STATE_PSTRRUN_STOP_PREV,         // 20
    BPMD_STATE_PSTRRUN_WR_PARAMS,         // 21
    BPMD_STATE_PSTRRUN_STRT_NEXT,         // 22

    BPMD_STATE_PSTRINJ_SWITCH_ON,         // 23
    BPMD_STATE_PSTRINJ_STOP_PREV,         // 24
    BPMD_STATE_PSTRINJ_WR_PARAMS,         // 25
    BPMD_STATE_PSTRINJ_STRT_NEXT,         // 26

    BPMD_STATE_count,
};

static char *state_list[] =
{
    "STATE_UNKNOWN",
    "STATE_DETERMN",

    "STATE_AUTOMAT",

    "STATE_ELCTRUN",
    "STATE_ELCTINJ",
    "STATE_ELCTDLS",

    "STATE_PSTRRUN",
    "STATE_PSTRINJ",
    "STATE_PSTRDLS",

    "STATE_FREERUN",

    "STATE_ELCTRUN_SWITCH_ON",
    "STATE_ELCTRUN_STOP_PREV",
    "STATE_ELCTRUN_RD_PARAMS",
    "STATE_ELCTRUN_WR_PARAMS",
    "STATE_ELCTRUN_STRT_NEXT",

    "STATE_ELCTINJ_SWITCH_ON",
    "STATE_ELCTINJ_STOP_PREV",
    "STATE_ELCTINJ_WR_PARAMS",
    "STATE_ELCTINJ_STRT_NEXT",

    "STATE_PSTRRUN_SWITCH_ON",
    "STATE_PSTRRUN_STOP_PREV",
    "STATE_PSTRRUN_WR_PARAMS",
    "STATE_PSTRRUN_STRT_NEXT",

    "STATE_PSTRINJ_SWITCH_ON",
    "STATE_PSTRINJ_STOP_PREV",
    "STATE_PSTRINJ_WR_PARAMS",
    "STATE_PSTRINJ_STRT_NEXT",
};

static const char* decode_state(const int state)
{
  static char buf[100];
  
    if (state < 0  ||  state >= (signed)(sizeof(state_list) / sizeof(state_list[0])))
    {
        sprintf(buf, "Unknown state code: %d", state);
        return buf;
    }

    return state_list[state];
}

#include "bpmd_cfg_drv_table.h"

typedef struct
{
    int                 devid;

    vdev_context_t      ctx;
    vdev_observer_t     obs;
    vdev_trgc_cur_t     cur[SUBORD_NUMCHANS];
    inserver_refmetric_t vr;

    int32               parameters_save[BPMD_STATE_count][SUBORD_NUMCHANS];
    int                 prev_stable_state;

    /* all about delay scan data */
} privrec_t;

static vdev_trgc_dsc_t sodc_mapping[SUBORD_NUMCHANS] =
{
    [BCPC_STOP]       = {VDEV_PRIV,             -1},
    [BCPC_SHOT]       = {VDEV_PRIV,             -1},

    [BCPC_ISTART]     = {VDEV_PRIV,             -1},

    [BCPC_NUMPTS]     = {VDEV_IMPR | VDEV_TUBE, BPMD_CFG_CHAN_NUMPTS},
    [BCPC_PTSOFS]     = {VDEV_IMPR | VDEV_TUBE, BPMD_CFG_CHAN_PTSOFS},
    [BCPC_DECMTN]     = {VDEV_IMPR | VDEV_TUBE, BPMD_CFG_CHAN_DECMTN},

    [BCPC_DELAY ]     = {VDEV_IMPR | VDEV_TUBE, BPMD_CFG_CHAN_DELAY },
    [BCPC_ATTEN ]     = {VDEV_IMPR | VDEV_TUBE, BPMD_CFG_CHAN_ATTEN },

    [BCPC_WAITTM]     = {VDEV_PRIV,             -1},
};

static inline void SndCVal(privrec_t *me, int sodc, int32 val)
{
    fprintf(stderr, "\t%d:=%d\n", sodc, val);
    vdev_observer_snd_chan(&(me->obs), sodc, val);
}

static inline void ReqCVal(privrec_t *me, int sodc)
{
    vdev_observer_req_chan(&(me->obs), sodc);
}

static int ourc2sodc[BPMD_CFG_CHAN_COUNT];

//////////////////////////////////////////////////////////////////////

static void ManualSaveParam(privrec_t *me, int state)
{
  int  x;
    fprintf(stderr,"%s) Manual Save [%d/%s] \n", __FUNCTION__, state, decode_state(state));

    /* Save current  */
    for (x = 0;  x < SUBORD_NUMCHANS;  x++)
	me->parameters_save[state][x] = me->cur[x].val;
}

static void ActualizeState (privrec_t *me, int state, int istart)
{
  int  x;
    
    /* Save previous */
    for (x = 0;  x < SUBORD_NUMCHANS;  x++)
	me->parameters_save[me->prev_stable_state][x] = me->cur[x].val;

    /* Alchemy... */
    switch (me->prev_stable_state)
    {
	case BPMD_STATE_ELCTRUN:
	    me->parameters_save[BPMD_STATE_ELCTINJ][BPMD_PARAM_DELAY] = me->cur[BPMD_PARAM_DELAY].val;
	    break;
	case BPMD_STATE_ELCTINJ:
	    me->parameters_save[BPMD_STATE_ELCTRUN][BPMD_PARAM_DELAY] = me->cur[BPMD_PARAM_DELAY].val;
	    break;
	case BPMD_STATE_PSTRRUN:
	    me->parameters_save[BPMD_STATE_PSTRINJ][BPMD_PARAM_DELAY] = me->cur[BPMD_PARAM_DELAY].val;
	    break;
	case BPMD_STATE_PSTRINJ:
	    me->parameters_save[BPMD_STATE_PSTRRUN][BPMD_PARAM_DELAY] = me->cur[BPMD_PARAM_DELAY].val;
	    break;
	default:
	    break;
    }

    /* Actualize requested */
    SndCVal(me, BCPC_NUMPTS, me->parameters_save[state][BCPC_NUMPTS]);
    SndCVal(me, BCPC_PTSOFS, me->parameters_save[state][BCPC_PTSOFS]);
    SndCVal(me, BCPC_DECMTN, me->parameters_save[state][BCPC_DECMTN]);
    
    SndCVal(me, BCPC_DELAY,  me->parameters_save[state][BCPC_DELAY] );
    SndCVal(me, BCPC_ATTEN,  me->parameters_save[state][BCPC_ATTEN] );

    SndCVal(me, BCPC_ISTART, istart);
    SndCVal(me, BCPC_WAITTM, 3600 * 1000);

#if 1
    fprintf(
            stderr, "%s() ctx->cur_state = %s, me->prev_stable_state = %s\n",
	    __FUNCTION__,
	    decode_state(me->ctx.cur_state), decode_state(me->prev_stable_state)
	   );
#endif
}

static int IsReReqAlwd(int devid, void *devptr)
{
  privrec_t *me = devptr;
  int r         = devid;

    me = me; // just to make gcc happy

    switch(me->ctx.cur_state)
    {
        case BPMD_STATE_ELCTRUN:
        case BPMD_STATE_ELCTINJ:
        case BPMD_STATE_PSTRRUN:
        case BPMD_STATE_PSTRINJ:
            r = 1;
	    break;
        default:
            r = 0;
	    break;
    }
    return r;
}

static void SwchToRequest (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t            *me     = devptr;
  inserver_refmetric_t *p_refm = &(me->vr);

    me->vr.data_cb(me->devid, devptr, p_refm->chan_ref, 0, NULL);
}

///////

static void SwchToUNKNOWN  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
    vdev_observer_forget_all(&me->obs);
}

static void SwchToDETERMN  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
}

static void SwchToAUTOMAT  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
}

///////

static int  IsAlwdToFREERUN(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    return me->obs.targ_ref != INSERVER_DEVREF_ERROR; /*!!!???  && targ_state==DEVSTATE_OPERATING ?*/
}

static void SwchToFREERUN  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
    ActualizeState(me, BPMD_STATE_FREERUN, 0);
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK * 0); /* Why *0 is here?? */
}

///////

static int  IsAlwdToELCTDLS(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    me = me; // just to make gcc happy
    return 0;
}

static void SwchToELCTDLS  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    me = me; // just to make gcc happy
}

static int  IsAlwdToPSTRDLS(void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    me = me; // just to make gcc happy
    return 0;
}

static void SwchToPSTRDLS  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    me = me; // just to make gcc happy
}

///////

static void SwchToELCTRUN_SWITCH_ON  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
}

static void SwchToELCTRUN_STOP_PREV  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

static void SwchToELCTRUN_WR_PARAMS  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    ActualizeState(me, BPMD_STATE_ELCTRUN, 1);
}

static void SwchToELCTRUN_STRT_NEXT  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

///////

static void SwchToELCTINJ_SWITCH_ON  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
}

static void SwchToELCTINJ_STOP_PREV  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

static void SwchToELCTINJ_WR_PARAMS  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    ActualizeState(me, BPMD_STATE_ELCTINJ, 0);
}

static void SwchToELCTINJ_STRT_NEXT  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

///////

static void SwchToPSTRRUN_SWITCH_ON  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
}

static void SwchToPSTRRUN_STOP_PREV  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

static void SwchToPSTRRUN_WR_PARAMS  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    ActualizeState(me, BPMD_STATE_PSTRRUN, 1);
}

static void SwchToPSTRRUN_STRT_NEXT  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

///////

static void SwchToPSTRINJ_SWITCH_ON  (void *devptr, int prev_state)
{
  privrec_t *me = devptr;
    me->prev_stable_state = prev_state;
}

static void SwchToPSTRINJ_STOP_PREV  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

static void SwchToPSTRINJ_WR_PARAMS  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    ActualizeState(me, BPMD_STATE_PSTRINJ, 0);
}

static void SwchToPSTRINJ_STRT_NEXT  (void *devptr, int prev_state __attribute__((unused)))
{
  privrec_t *me = devptr;
    SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
}

///////

static vdev_state_dsc_t state_descr[] =
{
    [BPMD_STATE_UNKNOWN] = {0, -1, NULL,            SwchToUNKNOWN, NULL},
    [BPMD_STATE_DETERMN] = {0, -1, NULL,            SwchToDETERMN, NULL},

    [BPMD_STATE_AUTOMAT] = {0, -1, NULL,            SwchToAUTOMAT, NULL},
    [BPMD_STATE_FREERUN] = {0, -1, IsAlwdToFREERUN, SwchToFREERUN, NULL},

    /* Switching to ELCTDLS regime */
    [BPMD_STATE_ELCTDLS] = {0, -1, NULL,            SwchToELCTDLS, NULL},
    /* Switching to PSTRDLS regime */
    [BPMD_STATE_PSTRDLS] = {0, -1, NULL,            SwchToPSTRDLS, NULL},

    /* Switching to ELCTRUN regime */
        [BPMD_STATE_ELCTRUN_SWITCH_ON] = { 1*1000, BPMD_STATE_ELCTRUN_STOP_PREV, NULL, SwchToELCTRUN_SWITCH_ON, NULL},
        [BPMD_STATE_ELCTRUN_STOP_PREV] = { 1*1000, BPMD_STATE_ELCTRUN_WR_PARAMS, NULL, SwchToELCTRUN_STOP_PREV, NULL},
	[BPMD_STATE_ELCTRUN_WR_PARAMS] = { 1*1000, BPMD_STATE_ELCTRUN_STRT_NEXT, NULL, SwchToELCTRUN_WR_PARAMS, NULL}, //(int[]){BCPC_STOP, -1}},
	[BPMD_STATE_ELCTRUN_STRT_NEXT] = { 1*1000, BPMD_STATE_ELCTRUN,           NULL, SwchToELCTRUN_STRT_NEXT, NULL}, //(int[]){BCPC_WAITTM, -1}},
    [BPMD_STATE_ELCTRUN]               = {     -1, BPMD_STATE_ELCTRUN,           NULL, SwchToRequest,           NULL}, //(int[]){BCPC_STOP, -1}},

    /* Switching to ELCTINJ regime */
        [BPMD_STATE_ELCTINJ_SWITCH_ON] = { 1*1000, BPMD_STATE_ELCTINJ_STOP_PREV, NULL, SwchToELCTINJ_SWITCH_ON, NULL},
        [BPMD_STATE_ELCTINJ_STOP_PREV] = { 1*1000, BPMD_STATE_ELCTINJ_WR_PARAMS, NULL, SwchToELCTINJ_STOP_PREV, NULL},
        [BPMD_STATE_ELCTINJ_WR_PARAMS] = { 1*1000, BPMD_STATE_ELCTINJ_STRT_NEXT, NULL, SwchToELCTINJ_WR_PARAMS, NULL}, //(int[]){BCPC_STOP, -1}},
        [BPMD_STATE_ELCTINJ_STRT_NEXT] = { 1*1000, BPMD_STATE_ELCTINJ,           NULL, SwchToELCTINJ_STRT_NEXT, NULL}, //(int[]){BCPC_WAITTM, -1}},
    [BPMD_STATE_ELCTINJ]               = {     -1, BPMD_STATE_ELCTINJ,           NULL, SwchToRequest,           NULL}, //(int[]){BCPC_STOP, -1}},

    /* Switching to PSTRRUN regime */
        [BPMD_STATE_PSTRRUN_SWITCH_ON] = { 1*1000, BPMD_STATE_PSTRRUN_STOP_PREV, NULL, SwchToPSTRRUN_SWITCH_ON, NULL},
        [BPMD_STATE_PSTRRUN_STOP_PREV] = { 1*1000, BPMD_STATE_PSTRRUN_WR_PARAMS, NULL, SwchToPSTRRUN_STOP_PREV, NULL},
        [BPMD_STATE_PSTRRUN_WR_PARAMS] = { 1*1000, BPMD_STATE_PSTRRUN_STRT_NEXT, NULL, SwchToPSTRRUN_WR_PARAMS, NULL}, //(int[]){BCPC_STOP, -1}},
        [BPMD_STATE_PSTRRUN_STRT_NEXT] = { 1*1000, BPMD_STATE_PSTRRUN,           NULL, SwchToPSTRRUN_STRT_NEXT, NULL}, //(int[]){BCPC_WAITTM, -1}},
    [BPMD_STATE_PSTRRUN]               = {     -1, BPMD_STATE_PSTRRUN,           NULL, SwchToRequest,           NULL}, //(int[]){BCPC_STOP, -1}},

    /* Switching to PSTRINJ regime */
        [BPMD_STATE_PSTRINJ_SWITCH_ON] = { 1*1000, BPMD_STATE_PSTRINJ_STOP_PREV, NULL, SwchToPSTRINJ_SWITCH_ON, NULL},
        [BPMD_STATE_PSTRINJ_STOP_PREV] = { 1*1000, BPMD_STATE_PSTRINJ_WR_PARAMS, NULL, SwchToPSTRINJ_STOP_PREV, NULL},
        [BPMD_STATE_PSTRINJ_WR_PARAMS] = { 1*1000, BPMD_STATE_PSTRINJ_STRT_NEXT, NULL, SwchToPSTRINJ_WR_PARAMS, NULL}, //(int[]){BCPC_STOP, -1}},
        [BPMD_STATE_PSTRINJ_STRT_NEXT] = { 1*1000, BPMD_STATE_PSTRINJ,           NULL, SwchToPSTRINJ_STRT_NEXT, NULL}, //(int[]){BCPC_WAITTM, -1}},
    [BPMD_STATE_PSTRINJ]               = {     -1, BPMD_STATE_PSTRINJ,           NULL, SwchToRequest,           NULL}, //(int[]){BCPC_STOP, -1}},
};

static int state_important_channels[countof(state_descr)][SUBORD_NUMCHANS];

//////////////////////////////////////////////////////////////////////

static void bpmd_cfg_rw_p(int devid, void *devptr,
                          int firstchan, int count, int32 *values, int action);

static vdev_sr_chan_dsc_t state_related_channels[] =
{
    {BPMD_CFG_CHAN_REGIME_AUTOMAT,   BPMD_STATE_AUTOMAT,           CX_VALUE_DISABLED_MASK,   CX_VALUE_DISABLED_MASK},
    {BPMD_CFG_CHAN_REGIME_FREERUN,   BPMD_STATE_FREERUN,           0                     ,   CX_VALUE_DISABLED_MASK},

    {BPMD_CFG_CHAN_REGIME_ELCTRUN,   BPMD_STATE_ELCTRUN_SWITCH_ON, 0,                        CX_VALUE_DISABLED_MASK},
    {BPMD_CFG_CHAN_REGIME_ELCTINJ,   BPMD_STATE_ELCTINJ_SWITCH_ON, 0,                        CX_VALUE_DISABLED_MASK},
    {BPMD_CFG_CHAN_REGIME_ELCTDLS,   BPMD_STATE_ELCTDLS,           0,                        CX_VALUE_DISABLED_MASK},

    {BPMD_CFG_CHAN_REGIME_PSTRRUN,   BPMD_STATE_PSTRRUN_SWITCH_ON, 0,                        CX_VALUE_DISABLED_MASK},
    {BPMD_CFG_CHAN_REGIME_PSTRINJ,   BPMD_STATE_PSTRINJ_SWITCH_ON, 0,                        CX_VALUE_DISABLED_MASK},
    {BPMD_CFG_CHAN_REGIME_PSTRDLS,   BPMD_STATE_PSTRDLS,           0,                        CX_VALUE_DISABLED_MASK},

    {-1,                             -1,                           0,                        0},
};

//////////////////////////////////////////////////////////////////////

static int bpmd_cfg_init_mod(void) /*!!! move upper */
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
    for (x = 0;  x < countof(sodc_mapping);  x++)
        if (sodc_mapping[x].mode != 0  &&  sodc_mapping[x].ourc >= 0)
            ourc2sodc[sodc_mapping[x].ourc] = x;

    return 0;
}

#if INSERVER2_STAT_CB_PRESENT
static void vect_stat_evproc(int                 devid, void *devptr,
                             inserver_devref_t   ref,
                             int                 newstate,
                             void               *privptr)
{
  privrec_t          *me = (privrec_t *)devptr;

    if (newstate == DEVSTATE_OPERATING)
        inserver_req_bigc(devid, me->vr.chan_ref,
                          NULL, 0,
                          NULL, 0, 0,
                          CX_CACHECTL_SHARABLE);
}
#endif
static void vect_bigc_evproc(int                 devid, void *devptr,
                             inserver_bigcref_t  ref,
                             int                 reason  __attribute__((unused)),
                             void               *privptr __attribute__((unused)))
{
  privrec_t          *me = (privrec_t *)devptr;

    /* NB!!!!!: 'reason' value is 0 for a moment */

#if 0
    fprintf(
            stderr, "%s() ctx->cur_state = %s, me->prev_stable_state = %s, ref = %d\n",
	    __FUNCTION__,
	    decode_state(me->ctx.cur_state), decode_state(me->prev_stable_state), ref
	   );
#endif

    if ( IsReReqAlwd(devid, devptr) )
    {
	inserver_req_bigc(devid, ref,
			  NULL, 0,
			  NULL, 0, 0,
			  1 ? CX_CACHECTL_FORCE : CX_CACHECTL_SHARABLE);
    }
    else
    {
//	fprintf(stderr, "fuck off\n");
    }
}

static int bpmd_cfg_init_d(int devid, void *devptr,
                           int businfocount __attribute__((unused)), int businfo[] __attribute__((unused)),
                           const char *auxinfo)
{
  privrec_t          *me = (privrec_t *)devptr;

  const char         *p  = auxinfo;
  const char         *b;
  char               *scp;

  char                bpmstrid[100];
  size_t              bpmstridlen;

  const bpmd_info_t  *bpmd_info;

    me->devid = devid;

    IsAlwdToELCTDLS(devptr, 0);
    IsAlwdToPSTRDLS(devptr, 0);

    /* I. Split the auxinfo into BPM string ID, subordinate driver string reference and optional drvletinfo */
    /* "(bpm_str_id/subordinate_driver_string_reference)" */

    /* 0. Is there anything at all? */
    if (auxinfo == NULL  ||  auxinfo[0] == '\0')
    {
        DoDriverLog(devid, 0, "%s: auxinfo is empty...", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    /* 1. BPM string identifier */
    p = strchr(b = auxinfo, '/');
    if (p == NULL  ||  p == b)
    {
        DoDriverLog(devid, 0, "%s: auxinfo should start with 'bpm_str_ID/'", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    bpmstridlen= p - b;
    for (bpmd_info = bpmd_info_db;  bpmd_info->strID != NULL;  bpmd_info++)
        if (cx_memcasecmp(bpmd_info->strID, b, bpmstridlen) == 0) break;

    if (bpmd_info->strID == NULL)
    {
        DoDriverLog(devid, 0, "%s: unknown bpm bpm_str_ID", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    memcpy(me->parameters_save, bpmd_info->tbt_def_params, sizeof(int32) * BPMD_STATE_count * SUBORD_NUMCHANS);

    p++;

    /* 2. Subordinate driver string reference */
    if (p == NULL  ||  *p == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s(): base-BPMD-device name is required in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    strzcpy(me->obs.targ_name, p, sizeof(me->obs.targ_name));
    scp = strchr(me->obs.targ_name, ':'); if (scp != NULL) *scp = '\0';
    me->obs.targ_numchans = countof(me->cur);

    me->obs.map           = sodc_mapping;
    me->obs.cur           = me->cur;

    me->ctx.chan_state_n             = BPMD_CFG_CHAN_BPMD_STATE;
    me->ctx.state_unknown_val        = BPMD_STATE_UNKNOWN;
    me->ctx.state_determine_val      = BPMD_STATE_DETERMN;
    me->ctx.state_descr              = state_descr;
    me->ctx.state_count              = countof(state_descr);

    me->ctx.state_related_channels   = state_related_channels;
    me->ctx.state_important_channels = state_important_channels;
    me->ctx.do_rw                    = bpmd_cfg_rw_p;
    me->ctx.sodc_cb                  = NULL;

    strzcpy(me->vr.targ_name, me->obs.targ_name, sizeof(me->vr.targ_name));
    me->vr.chan_n       = 0;
    me->vr.chan_is_bigc = 1;
#if INSERVER2_STAT_CB_PRESENT
    me->vr.stat_cb      = vect_stat_evproc;
#endif
    me->vr.data_cb      = vect_bigc_evproc;
    inserver_new_watch_list(devid, &(me->vr), BIND_HEARTBEAT_PERIOD);

    return vdev_init(&(me->ctx), &(me->obs),
                     devid, devptr,
                     BIND_HEARTBEAT_PERIOD, WORK_HEARTBEAT_PERIOD);
}

static void bpmd_cfg_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    vdev_fini(&(me->ctx));
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

static void bpmd_cfg_rw_p(int devid, void *devptr,
                          int firstchan, int count, int32 *values, int action)
{
  privrec_t          *me = (privrec_t *)devptr;
  int                 n;       // channel N in values[] (loop ctl var)
  int                 x;       // channel indeX
  int32               val;     // value
  int                 sodc;
  vdev_sr_chan_dsc_t *sdp;
  int                 alwd;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        sodc = ourc2sodc[x];

        if      (sodc >= 0  &&  sodc_mapping[sodc].mode & VDEV_TUBE)
        {
            if (me->obs.targ_state == DEVSTATE_OPERATING)
            {
                if (action == DRVA_WRITE)
                {
                    SndCVal(me, sodc, val);

                    /*!!!???*/
		    if (
			(sodc_mapping[sodc].mode & VDEV_IMPR)
			&&
			(me->ctx.cur_state == BPMD_STATE_ELCTINJ  ||  me->ctx.cur_state == BPMD_STATE_PSTRINJ)
		       )
			SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);
		}
                else
                    ReqCVal(me, sodc);
            }
        }

        else if ((sdp = find_sdp(x)) != NULL  &&
                 sdp->state >= 0              /*&&
                 state_descr[sdp->state].sw_alwd != NULL*/)
        {
            alwd =
                state_descr[sdp->state].sw_alwd == NULL  ||
                state_descr[sdp->state].sw_alwd(me, sdp->state);  /* ???? */

	    if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND  &&  alwd)
            {
		fprintf(stderr, "%s:%d) found & go to next state: %s\n", __FUNCTION__, __LINE__, decode_state(sdp->state));
                vdev_set_state(&(me->ctx), sdp->state);
            }
            val = alwd ? sdp->enabled_val : sdp->disabled_val;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
#if 0
        else if (x == BPMD_CFG_CHAN_REGIME_RESET)
        {
    	    if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
    		vdev_set_state(&(me->ctx), BPMD_STATE_DETERMN);
    	    val = 0;
    	    ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
#endif
        else if (x == BPMD_CFG_CHAN_FORCE_RELOAD)
        {
    	    if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
		SndCVal(me, BCPC_STOP, CX_VALUE_COMMAND | CX_VALUE_DISABLED_MASK);    
    	    val = 0;
    	    ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }

        else if (x == BPMD_CFG_CHAN_REGIME_MSAVE)
        {
    	    if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
		ManualSaveParam(me, me->ctx.cur_state);
	    val = 0;
    	    ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }

	else if (x == BPMD_CFG_CHAN_BPMD_STATE)
        {
            val = me->ctx.cur_state;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }

        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}

/* Metric */

static CxsdMainInfoRec bpmd_cfg_main_info[] =
{
    {BPMD_CFG_CHAN_WR_count, 1},
    {BPMD_CFG_CHAN_RD_count, 0},
};

DEFINE_DRIVER(bpmd_cfg, "PC driver for BPMD configuration",
              bpmd_cfg_init_mod, NULL,
              sizeof(privrec_t), NULL,
              0, 20, /*!!! CXSD_DB_MAX_BUSINFOCOUNT */
              NULL, 0,
              countof(bpmd_cfg_main_info), bpmd_cfg_main_info,
              -1, NULL,
              bpmd_cfg_init_d, bpmd_cfg_term_d, bpmd_cfg_rw_p, NULL);
