#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>

#include "paramstr_parser.h"
#include "seqexecauto.h"

#include "cda.h"
#include "Cdr.h"
#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Chl.h"
#include "Knobs.h"
#include "KnobsI.h"

#include "drv_i/nkshd485_drv_i.h"
#include "common_elem_macros.h"


const char *app_name      = "nkshd485tool";
const char *app_class     = "NKShD485Tool";
const char *app_title     = "nKShD485 test app";
const char *app_defserver = "";

enum {NKSHD485_BASE = 0};
enum {
    REG_CYCLE_COUNT   = 100,
    REG_CYCLE_GO      = 101,
    REG_CYCLE_STOP    = 102,
    REG_CYCLE_CURSTEP = 103,
};

static groupunit_t app_grouping[] =
{

#define COLORLED_LINE(name, comnt, dtype, n) \
    {name, NULL, NULL, NULL, NULL, comnt, LOGT_READ, dtype, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + n)}
#define BUT_LINE(name, title, n) \
    {name, title, "", NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(n)}
    GLOBELEM_START("ctl", "Direct control", 3, 1)
        SUBELEM_START("info", "Info", 2, 2)
            {"devver", "Dev.ver.", NULL, "%4.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_DEV_VERSION)},
            {"devsrl", "serial#",  NULL, "%3.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_DEV_SERIAL)},
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
        SUBELEM_START("config", "Config", 5, 1)
            SUBELEM_START("curr", "Current", 2, 2)
                {"work", "Work", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_WORK_CURRENT)},
                {"hold", "Hold", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_HOLD_CURRENT)},
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            {"hold_delay", "Hold dly", "1/30s", "%3.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_HOLD_DELAY), .minnorm=0, .maxnorm=255, .placement="horz=right"},
            {"cfg_byte", "Cfg byte", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BYTE)},
            SUBELEM_START("vel", "Velocity", 2, 2)
                {"min", "[",   "/s",   "%5.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_MIN_VELOCITY), .minnorm=32, .maxnorm=12000},
                {"max", "-",   "]/s",   "%5.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_MAX_VELOCITY), .minnorm=32, .maxnorm=12000},
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            {"accel",   "Accel", "/s^2", "%5.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_ACCELERATION), .minnorm=32, .maxnorm=65535, .placement="horz=center"},
        SUBELEM_END("noshadow,nocoltitles,fold_h", NULL),
        SUBELEM_START("operate", "Operation", 2, 1)
            SUBELEM_START("flags", "Flags", 8 , 8)
                COLORLED_LINE("ready", "Ready",       LOGD_GREENLED, 0),
                COLORLED_LINE("going", "Going",       LOGD_GREENLED, 1),
                COLORLED_LINE("k1",    "K-",          LOGD_REDLED,   2),
                COLORLED_LINE("k2",    "K+",          LOGD_REDLED,   3),
                COLORLED_LINE("k3",    "Sensor",      LOGD_REDLED,   4),
                COLORLED_LINE("s_b5",  "Prec. speed", LOGD_AMBERLED, 5),
                COLORLED_LINE("s_b6",  "K acc",       LOGD_AMBERLED, 6),
                COLORLED_LINE("s_b7",  "BC",          LOGD_AMBERLED, 7),
            SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", "horz=right"),
            SUBELEM_START("steps", "Steps", 6, 3)
                {"numsteps", NULL, NULL, "%8.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_NUM_STEPS)},
                BUT_LINE("go",         "GO",          NKSHD485_BASE + NKSHD485_CHAN_GO),
#if 0
                BUT_LINE("go_unaccel", "GO unaccel.", NKSHD485_BASE + NKSHD485_CHAN_GO_WO_ACCEL),
#else
                {"stopcond", "StopC", NULL, "%3.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_GO_WO_ACCEL)},
#endif
                EMPTY_CELL(),
                BUT_LINE("stop",       "STOP",             NKSHD485_BASE + NKSHD485_CHAN_STOP),
                BUT_LINE("switchoff",  "Switch OFF",       NKSHD485_BASE + NKSHD485_CHAN_SWITCHOFF),
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
        SUBELEM_END("noshadow,nocoltitles", NULL),
    GLOBELEM_END("nocoltitles,norowtitles", 0),

    GLOBELEM_START("cycling", "Cycling", 4, 1)
        {"cycles", "# cycles", NULL, "%4.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_PUSH_I(100), CMD_SETLCLREGDEFVAL_I(REG_CYCLE_COUNT),
                CMD_GETLCLREG_I(REG_CYCLE_COUNT), CMD_RET,
            },
            (excmd_t[]){
                CMD_QRYVAL, CMD_SETLCLREG_I(REG_CYCLE_COUNT),
                CMD_RET,
            },
        },
        {"do", "Do!", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_PUSH_I(100), CMD_SETLCLREGDEFVAL_I(REG_CYCLE_GO),
                CMD_GETLCLREG_I(REG_CYCLE_GO), CMD_RET,
            },
            (excmd_t[]){
                CMD_QRYVAL, CMD_SETLCLREG_I(REG_CYCLE_GO),
                CMD_RET,
            },
        },
        {"stop", "Stop", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_GETP_I(NKSHD485_BASE + NKSHD485_CHAN_STOP), CMD_POP,
                CMD_PUSH_I(100), CMD_SETLCLREGDEFVAL_I(REG_CYCLE_STOP),
                CMD_GETLCLREG_I(REG_CYCLE_STOP), CMD_RET,
            },
            (excmd_t[]){
                CMD_QRYVAL, CMD_SETLCLREG_I(REG_CYCLE_STOP),
                CMD_RET,
            },
        },
        {"curstep", "Cur #", NULL, "%4.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t[]){
                CMD_PUSH_I(-1), CMD_SETLCLREGDEFVAL_I(REG_CYCLE_CURSTEP),
                CMD_GETLCLREG_I(REG_CYCLE_CURSTEP), CMD_RET,
            },
        },
    GLOBELEM_END("nocoltitles", 0),

    {NULL}
};

static physprops_t app_phys_info[] =
{
};

//////////////////////////////////////////////////////////////////////

static XhWindow  onewin = NULL;
static sea_context_t *prc_ctx_p = NULL;

enum {
    EVT_DATA = 1,
    EVT_EOC  = 2,
};

static void make_message_method(sea_context_t *cp, const char *format, va_list ap)
{
    XhVMakeMessage(onewin, format, ap);
}

//////////

static void onecycle_cleanup_method(sea_context_t *cp, sea_rslt_t reason)
{
    ChlSetChanVal(onewin, "ctl.operate.steps.stop", 1.0);
    if (prc_ctx_p != NULL) sea_check(prc_ctx_p, EVT_EOC);
}

static sea_rslt_t SetStepsAction(sea_context_t *ctx __attribute__((unused)), sea_step_t *step)
{
    ChlSetChanVal(onewin, "ctl.operate.steps.numsteps", (double)(ptr2lint(step->actarg)));
    return SEA_RSLT_NONE;
}

static sea_rslt_t GoAction(sea_context_t *ctx __attribute__((unused)), sea_step_t *step)
{
    ChlSetChanVal(onewin, "ctl.operate.steps.go",       1.0 + 0 * (double)(ptr2lint(step->actarg)));
    return SEA_RSLT_NONE;
}

static sea_rslt_t GoChecker(sea_context_t *ctx __attribute__((unused)), sea_step_t *step, int evtype __attribute__((unused)))
{
  int         dir = ptr2lint(step->actarg);
  const char *kn;
  double      kv;

    if      (dir < 0) kn = "ctl.operate.flags.k1";
    else if (dir > 0) kn = "ctl.operate.flags.k2";
    else              kn = "ctl.operate.flags.ready";
    ChlGetChanVal(onewin, kn, &kv);
    return kv != 0.0? SEA_RSLT_SUCCESS : SEA_RSLT_NONE;
}

static sea_step_t onecycle_prog[] =
{
    {"SetLeft",  SetStepsAction, SEA_SUCCESS_CHECK, LINT2PTR(-2000000), 0,        1000000, NULL},
    {"GoLeft",   GoAction,       GoChecker,         LINT2PTR(-1),       EVT_DATA, 1000000, NULL},
    {"SetRight", SetStepsAction, SEA_SUCCESS_CHECK, LINT2PTR(+2000000), 0,        1000000, NULL},
    {"GoRight",  GoAction,       GoChecker,         LINT2PTR(+1),       EVT_DATA, 1000000, NULL},
    SEA_END()
};

static sea_context_t onecycle_ctx = {onecycle_cleanup_method, make_message_method};

////////////

static int numcycles;
static int curcycle;
static int stopping;

static void process_cleanup_method(sea_context_t *cp, sea_rslt_t reason)
{
    stopping  = 1;
    prc_ctx_p = NULL;
    ChlSetLclReg (onewin, REG_CYCLE_CURSTEP, -1);
    sea_stop(&onecycle_ctx);
}


static sea_rslt_t PrcInitAction(sea_context_t *ctx __attribute__((unused)), sea_step_t *step __attribute__((unused)))
{
  double v;

    stopping = 0;
    prc_ctx_p = ctx;

    ChlGetLclReg(onewin, REG_CYCLE_COUNT, &v);
    numcycles = v;
    if (numcycles <= 0) return SEA_RSLT_FATAL;

    curcycle = 0;
    ChlSetLclReg(onewin, REG_CYCLE_CURSTEP, curcycle);
    return SEA_RSLT_SUCCESS;
}

static sea_rslt_t PrcCycleAction(sea_context_t *ctx __attribute__((unused)), sea_step_t *step __attribute__((unused)))
{
    ChlSetLclReg(onewin, REG_CYCLE_CURSTEP, curcycle);
    sea_run(&onecycle_ctx, "zzz", onecycle_prog);
    return SEA_RSLT_NONE;
}

static sea_rslt_t PrcCycleChecker(sea_context_t *ctx, sea_step_t *step, int evtype __attribute__((unused)))
{
    if (stopping)
    {
        fprintf(stderr, "zzz!\n");
        return SEA_RSLT_NONE;
    }
    curcycle++;
    if (curcycle >= numcycles) return SEA_RSLT_SUCCESS;
    return sea_goto(ctx, step->label);
}

static sea_step_t process_prog[] =
{
    {"init",  PrcInitAction,  SEA_SUCCESS_CHECK, NULL, 0, 0, NULL},
    {"cycle", PrcCycleAction, PrcCycleChecker,   NULL, 0, 0, NULL},
    SEA_END()
};
static sea_context_t process_ctx  = {process_cleanup_method, make_message_method};


static void lclregchg_proc(int lreg_n, double v)
{
    fprintf(stderr, "reg#%d:=%8.3f\n", lreg_n, v);

    if (onewin == NULL) return;

    if      (lreg_n == REG_CYCLE_GO)
    {
        sea_run(&process_ctx, "zzz", process_prog);
    }
    else if (lreg_n == REG_CYCLE_STOP)
    {
        stopping = 1;
        sea_stop(&process_ctx);
    }
}

static void newdata_proc(XhWindow window, void *privptr, int synthetic)
{
    //fprintf(stderr, "%s(synthetic=%d)\n", __FUNCTION__, synthetic);

    if (onewin == NULL) onewin = window;

    sea_check(&onecycle_ctx, EVT_DATA);
}

int main(int argc, char *argv[])
{
    cda_set_lclregchg_cb(lclregchg_proc);
    return ChlRunApp(argc, argv,
                     app_name, app_class,
                     app_title, NULL,
                     NULL, NULL,
                     "", NULL,
                     app_defserver, app_grouping,
                     app_phys_info, countof(app_phys_info),
                     newdata_proc, NULL);
}
