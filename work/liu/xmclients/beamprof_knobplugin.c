#include <stdio.h>
#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>

#include "cda.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "beamprof_knobplugin.h"


enum {UPDATE_PERIOD_MS = 200};

enum {MAXPOINTS = 10000};

enum
{
    X_DIF  = 75,
    Y_DIF  = 75,
    X_SIZE = X_DIF * 2 + 1,
    Y_SIZE = X_DIF * 2 + 1,
};

enum
{
   SUM_N = 0, DIF_N = 1,
   X_NL  = 0, Z_NL  = 1,
};

typedef struct
{
    char           *src;
    Knob            sk;
    fastadc_data_t *adc_p;
    fastadc_gui_t  *gui_p;
    int             chg;
} srcref_t;

typedef struct
{
    int    num;
    XPoint pts[MAXPOINTS];
} onemes_t;

typedef struct
{
    srcref_t        bpms[2];

    knobinfo_t     *ki;
    XhWindow        win;

    Widget          view;

    GC              axisGC;
    GC              dataGC[2];

    onemes_t        data[2];
} BEAMPROF_privrec_t;

static psp_paramdescr_t text2BEAMPROF_opts[] =
{
    PSP_P_MSTRING("sumsrc", BEAMPROF_privrec_t, bpms[SUM_N].src,  NULL, 1000),
    PSP_P_MSTRING("difsrc", BEAMPROF_privrec_t, bpms[DIF_N].src,  NULL, 1000),
    PSP_P_END()
};

static const char *src_names[2] = {"sum_source", "dif_source"};


//////////////////////////////////////////////////////////////////////

static void DrawAxis(BEAMPROF_privrec_t *me)
{
    XDrawLine(XtDisplay(me->view), XtWindow(me->view), me->axisGC,
              0,     Y_DIF, X_SIZE-1, Y_DIF);
    XDrawLine(XtDisplay(me->view), XtWindow(me->view), me->axisGC,
              X_DIF, 0,     X_DIF,    Y_SIZE-1);
}

static void DrawView(BEAMPROF_privrec_t *me, int n)
{
#if 0
    if (me->data[n].num > 0)
        XDrawPoints(XtDisplay(me->view), XtWindow(me->view), me->dataGC[n],
                    me->data[n].pts, me->data[n].num, CoordModeOrigin);
#else
  int      i;
  XPoint   pts[5];
  Display *dpy = XtDisplay(me->view);
  Window   win = XtWindow(me->view);
  GC       gc  = me->dataGC[n];

    pts[1].x = +1; pts[1].y = +0;
    pts[2].x = -1; pts[2].y = +1;
    pts[3].x = -1; pts[3].y = -1;
    pts[4].x = +1; pts[4].y = -1;
    for (i = 0;  i < me->data[n].num;  i++)
    {
      pts[0].x = me->data[n].pts[i].x;
      pts[0].y = me->data[n].pts[i].y;
      XDrawLines(dpy, win, gc,
                 pts, countof(pts), CoordModePrevious);
    }
#endif
}

static void DrawAll(BEAMPROF_privrec_t *me, int do_clear)
{
    if (do_clear)
        XClearArea(XtDisplay(me->view), XtWindow(me->view), 0, 0, 0, 0, False);
    DrawAxis(me);
    DrawView(me, 0);
    DrawView(me, 1);
}

static void ExposureCB(Widget     w         __attribute__((unused)),
                       XtPointer  closure,
                       XtPointer  call_data)
{
  BEAMPROF_privrec_t *me = closure;

    DrawAll(me, 0);
}

static void RegetData(BEAMPROF_privrec_t *me)
{
  int         x1, x2, tmpx;

  int         i;
  int         n;

  static int  phase = 0;

  enum {Re = 80};

  double      r,  x,  z;
  double      rt, xt, zt;
  double      Dx, Sx;
  double      Dz, Sz;

    x1 = x2 = -1;
    if (me->bpms[SUM_N].gui_p->rpr_on[0]) x1 = me->bpms[SUM_N].gui_p->rpr_at[0];
    if (me->bpms[SUM_N].gui_p->rpr_on[1]) x2 = me->bpms[SUM_N].gui_p->rpr_at[1];
    if (x1 > x2) {tmpx = x2; x2 = x1; x1 = tmpx;}
#if 0
#define DEG2RAD (3.14159265 / 180.0)

    if (x1 < 0  ||  x2 < 0)
    {
        x1 = 0;  x2 = 99;
    }
    if (x2 > 99) x2 = 99;

    phase = (phase + 10) % 360;
    me->data[1].num = x2 + 1;
    for (n = 0;  n < me->data[1].num;  n++)
    {
        me->data[1].pts[n].x = X_DIF + (n * sin((phase+n)*DEG2RAD));
        me->data[1].pts[n].y = Y_DIF - (n * cos((phase+n)*DEG2RAD));
    }
#else
    if (x1 < 0  ||  x2 < 0)
        me->data[1].num = 0;
    else
    {
        me->data[1].num = x2 - x1 + 1;
        if (me->data[1].num > MAXPOINTS)
            me->data[1].num = MAXPOINTS;
    }

    for (n = 0;  n < me->data[1].num;  n++)
    {
        i = x1 + n;

        if (me->bpms[SUM_N].adc_p->mes.plots[X_NL].numpts > i  &&
            me->bpms[SUM_N].adc_p->mes.plots[Z_NL].numpts > i  &&
            me->bpms[DIF_N].adc_p->mes.plots[X_NL].numpts > i  &&
            me->bpms[DIF_N].adc_p->mes.plots[Z_NL].numpts > i)
        {
            Sx = FastadcDataGetDsp(me->bpms[SUM_N].adc_p,
                                   &(me->bpms[SUM_N].adc_p->mes),
                                   X_NL, i);
            Sz = FastadcDataGetDsp(me->bpms[SUM_N].adc_p,
                                   &(me->bpms[SUM_N].adc_p->mes),
                                   Z_NL, i);
            Dx = FastadcDataGetDsp(me->bpms[DIF_N].adc_p,
                                   &(me->bpms[DIF_N].adc_p->mes),
                                   X_NL, i);
            Dz = FastadcDataGetDsp(me->bpms[DIF_N].adc_p,
                                   &(me->bpms[DIF_N].adc_p->mes),
                                   Z_NL, i);

            Sx = fabs(Sx); // Correct wrong polarity
            Sz = fabs(Sz);

            xt = Dx / (2 * Sx);
            zt = Dz / (2 * Sz);

            rt = sqrt(xt*xt + zt*zt);
            r  = (1 / (2 * rt)) * (1 - sqrt(1 - 4*rt*rt));

            x = xt * (1 + rt*rt) * Re;
            z = zt * (1 + rt*rt) * Re;
        }
        else
        {
            x = -X_DIF;
            z = +Y_DIF/2;
        }

        me->data[1].pts[n].x = X_DIF + x;
        me->data[1].pts[n].y = Y_DIF - z;
    }
#endif
}

static void UpdateProc(XtPointer     closure,
                       XtIntervalId *id      __attribute__((unused)))
{
  BEAMPROF_privrec_t *me = closure;

    if (me->bpms[0].chg  &&  me->bpms[1].chg)
    {
        /* Drop "changed" */
        me->bpms[0].chg = me->bpms[1].chg = 0;
        /* Shift generations */
        memcpy(me->data + 0, me->data + 1, sizeof(me->data[0]));

        /* Get new data */
        RegetData(me);

        /* Redisplay */
        DrawAll(me, 1);
    }

    XtAppAddTimeOut(xh_context, UPDATE_PERIOD_MS, UpdateProc, me);
}
                       
//////////////////////////////////////////////////////////////////////

static void newdata_cb(pzframe_data_t *src_pfr,
                       int   reason,
                       int   info_changed,
                       void *privptr)
{
  srcref_t *ref = privptr;

    if (reason == PZFRAME_REASON_BIGC_DATA) ref->chg = 1;
}

static void repers_cb(pzframe_gui_t *gui,
                      int   reason,
                      int   info_changed,
                      void *privptr)
{
  BEAMPROF_privrec_t *me = privptr;
  fastadc_gui_t      *src = pzframe2fastadc_gui(gui);
  fastadc_gui_t      *dst;

    if (reason == FASTADC_REASON_REPER_CHANGE)
    {
        dst = pzframe2fastadc_gui(me->bpms[(gui == me->bpms[0].gui_p)].gui_p);
        FastadcGuiSetReper(dst, 0, src->rpr_on[0]? src->rpr_at[0] : -1);
        FastadcGuiSetReper(dst, 1, src->rpr_on[1]? src->rpr_at[1] : -1);

        RegetData(me);
        DrawAll  (me, 1);
    }
}

static int ConnectToADC(BEAMPROF_privrec_t *me, int n)
{
  srcref_t *ref = me->bpms + n;

    if ((ref->sk = ChlFindKnob(me->win, ref->src)) == NULL)
    {
        fprintf(stderr, "BEAMPROF %s \"%s\" not found\n",
                src_names[n], ref->src);
        return -1;
    }
    if ((ref->adc_p =
             pzframe2fastadc_data
             (
              PzframeKnobpluginRegisterCB(ref->sk, "adc200me",
                                          newdata_cb,
                                          ref, NULL)
             )
        ) == NULL)
    {
        fprintf(stderr, "BEAMPROF %s adc200me \"%s\": failure to connect to\n",
                src_names[n], ref->src);
        return -1;
    }
    ref->gui_p = pzframe2fastadc_gui
        (PzframeKnobpluginGetGui(ref->sk, "adc200me"));
    PzframeGuiAddEvproc(ref->gui_p, repers_cb, me);

    return 0;
}

static GC AllocIdxGC(Widget w, int idx)
{
  XtGCMask   mask;  
  XGCValues  vals;
    
    mask = GCForeground | GCLineWidth;
    vals.foreground = XhGetColor(idx);
    vals.line_width = 0;

    return XtGetGC(w, mask, &vals);
}
                        
static XhWidget BEAMPROF_Create_m(knobinfo_t *ki, XhWidget parent)
{
  BEAMPROF_privrec_t *me;

    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(BEAMPROF_privrec_t))) == NULL)
        return NULL;
    me = (BEAMPROF_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, me, sizeof(*me), text2BEAMPROF_opts);

    me->ki  = ki;
    me->win = XhWindowOf(parent);

    if (ConnectToADC(me, SUM_N) < 0  ||  ConnectToADC(me, DIF_N) < 0) goto ERRRET;
fprintf(stderr, "%s() gui2=%p\n", __FUNCTION__, me->bpms[1].gui_p);

    me->axisGC    = AllocIdxGC(parent, XH_COLOR_GRAPH_AXIS);
    me->dataGC[0] = AllocIdxGC(parent, XH_COLOR_GRAPH_PREV);
    me->dataGC[1] = AllocIdxGC(parent, XH_COLOR_GRAPH_LINE1);

    me->view = XtVaCreateManagedWidget("beamProf", xmDrawingAreaWidgetClass, parent,
                                       XmNwidth,       X_SIZE,
                                       XmNheight,      Y_SIZE,
                                       XmNborderWidth, 0,
                                       XmNbackground,  XhGetColor(XH_COLOR_GRAPH_BG),
                                       NULL);
    XtAddCallback(me->view, XmNexposeCallback, ExposureCB, me);

    XtAppAddTimeOut(xh_context, UPDATE_PERIOD_MS, UpdateProc, me);

    return me->view;

 ERRRET:
    return XtVaCreateManagedWidget("beamProf", widgetClass, parent,
                                   XmNwidth,       X_SIZE,
                                   XmNheight,      Y_SIZE,
                                   XmNborderWidth, 0,
                                   XmNbackground,  XhGetColor(XH_COLOR_JUST_RED),
                                   NULL);
}

static knobs_vmt_t KNOB_BEAMPROF_VMT  = {BEAMPROF_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t beamprof_widgetinfo[] =
{
    {0, "beamprof", &KNOB_BEAMPROF_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  beamprof_widgetset = {beamprof_widgetinfo, NULL};


void  RegisterBEAMPROF_widgetset(int look_id)
{
    beamprof_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&beamprof_widgetset);
}
