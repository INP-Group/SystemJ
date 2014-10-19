#include <math.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/Text.h>

#include "misc_macros.h"
#include "misclib.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "Knobs_typesP.h"
#include "KnobsI.h"
#include "ChlI.h"

#include "drv_i/nadc200_drv_i.h"

#include "ndbp_nadc200_elemplugin.h"


//////////////////////////////////////////////////////////////////////

static XFontStruct     *last_finfo;

static GC AllocXhGC(Widget w, int idx, const char *fontspec)
{
  XtGCMask   mask;  
  XGCValues  vals;
    
    mask = GCForeground | GCLineWidth;
    vals.foreground = XhGetColor(idx);
    vals.line_width = 0;

    if (fontspec != NULL  &&  fontspec[0] != '\0')
    {
        last_finfo = XLoadQueryFont(XtDisplay(w), fontspec);
        if (last_finfo == NULL)
        {
            fprintf(stderr, "Unable to load font \"%s\"; using \"fixed\"\n", fontspec);
            last_finfo = XLoadQueryFont(XtDisplay(w), "fixed");
        }
        vals.font = last_finfo->fid;

        mask |= GCFont;
    }

    return XtGetGC(w, mask, &vals);
}

static GC AllocDashGC(Widget w, int idx, int dash_length)
{
  XtGCMask   mask;  
  XGCValues  vals;
    
    mask = GCForeground | GCLineWidth | GCLineStyle | GCDashList;
    vals.foreground = XhGetColor(idx);
    vals.line_width = 0;
    vals.line_style = LineOnOffDash;
    vals.dashes     = dash_length;

    return XtGetGC(w, mask, &vals);
}

static void RefreshThe(Widget w)
{
    if (w != NULL) XClearArea(XtDisplay(w), XtWindow(w), 0, 0, 0, 0, True);
}


//////////////////////////////////////////////////////////////////////

enum
{
    CALC_NONE = 0,
    CALC_AVG  = 1,
    CALC_TG   = 2
};

enum
{
    H_TICK_LEN      = 5,
    V_TICK_LEN      = 5,
    H_TICK_SPC      = 1,
    V_TICK_SPC      = 0,
    H_TICK_MRG      = 2,
    V_TICK_MRG      = 1,
    GRID_DASHLENGTH = 6,

    DEF_G_W = 301,
    DEF_G_H = 180,

    H_TICK_STEP = 50,
    V_TICK_SEGS = 8,
    
    NUMPTS  = 4096,
};

enum
{
    DEF_Z12     = 128,
    DEF_RANGE12 = NADC200_R_2048,
};

#define LFRT_TICK_FMT "% 5.3f"

typedef struct
{
    ElemInfo              e; // A back-reference
    XhWindow              win;

    Widget                axis;
    Widget                graph;
    Pixmap                pix;

    GC                    bkgdGC[COLALARM_LAST + 1];
    GC                    axisGC;
    GC                    gridGC;
    GC                    reprGC;
    GC                    chanGC[2];
    GC                    calcGC[2];

    XFontStruct          *axis_finfo;
    XFontStruct          *chan_finfo[2];
    
    Dimension             G_W;
    Dimension             G_H;
    Dimension             m_lft;
    Dimension             m_rgt;
    Dimension             m_top;
    Dimension             m_bot;

    Knob                  k_run;
    Knob                  k_one;
    Widget                w_roller;
    int                   roller_ctr;
    
    Knob                  k_ymax;
    Knob                  k_ymin;

    Knob                  k_calc[2];
    Widget                w_rslt[2];

    Knob                  k_j[2];
    int                   v_j[2];
    Knob                  k_scale[2][2];

    Knob                  k_t0;
    Knob                  k_t1;
    Knob                  k_tk;

    Knob                  k_tmin;
    Knob                  k_tmax;

    Knob                  k_params[NADC200_NUM_PARAMS];
    
    int                   ymin;
    int                   ymax;
    int                   tmin;
    int                   tmax;

    int                   t0;
    int                   t1;

    double                tk;

    int                   calc_type[2];
    uint8                 avg_c    [2];
    double                calced   [2];

    int                   x_of_mouse;
    
    cda_serverid_t        sid;
    cda_bigchandle_t      bigch;
    tag_t                 mes_tag;
    rflags_t              mes_rflags;
    int32                 mes_info[NADC200_NUM_PARAMS];
    int32                 prev_mes_info[NADC200_NUM_PARAMS];
    uint8                 mes_data[NUMPTS*2];
    colalarm_t            curstate;
    int                   running;
    int                   oneshot;

    int                   len_set;
} adc200_privrec_t;

static inline int IS_DOUBLE_BUFFERING(adc200_privrec_t *me)
{
    return me->pix != XmUNSPECIFIED_PIXMAP;
}

static int c_ticks_at[] = {0, 32,  64, 92,  128, 160,  192, 224,  255};

static inline int C2Y(adc200_privrec_t *me, int c)
{
    return me->G_H - 1 - RESCALE_VALUE(c, me->ymin, me->ymax, 0, me->G_H-1);
}

static double C2V(adc200_privrec_t *me, uint8 c, int side)
{
  double scales[2][4] =
  {
      {0.002, 0.004, 0.008, 0.016},
      {0.008, 0.016, 0.032, 0.064}
  };
    
    return
        ((int)(c) + me->mes_info[NADC200_PARAM_ZERO1+side] - 256) *
        scales[me->v_j[side]][me->mes_info[NADC200_PARAM_RANGE1+side]];
}

static int N2NS(adc200_privrec_t *me, int n)
{
  double timing_scales[4] = {1e9/200e6/*5.0*/, 1e9/195e6/*5.128*/, 1, 1};
  int    frqdiv_scales[4] = {1, 2, 4, 8};
    
    return (int)(n *
                 timing_scales[me->mes_info[NADC200_PARAM_TIMING]&3] *
                 frqdiv_scales[me->mes_info[NADC200_PARAM_FRQDIV]&3]);
}



static inline int N2X(adc200_privrec_t *me, int n)
{
    return RESCALE_VALUE(n, me->tmin, me->tmax, 0, me->G_W-1);
}

static inline int X2N(adc200_privrec_t *me, int x)
{
    return RESCALE_VALUE(x, 0, me->G_W-1, me->tmin, me->tmax);
}

static void PerformCalcs(adc200_privrec_t *me, int refresh)
{
  int    n;
  int    t;
  uint8 *p;
  int    iv;
  char   buf[100];
    
    for (n = 0;  n < 2;  n++)
    {
        if      (me->calc_type[n] == CALC_AVG)
        {
            for (t =  me->t0, p = me->mes_data + me->t0 + NUMPTS*n, iv = 0;
                 t <= me->t1;
                 t++,          p++)
                iv += *p;
            me->avg_c [n] = iv / (me->t1 - me->t0 + 1);
            me->calced[n] = C2V(me, me->avg_c[n], n);

            sprintf(buf, "% 5.3f", me->calced[n]);
        }
        else if (me->calc_type[n] == CALC_TG)
        {
            sprintf(buf, "% 5.2f", me->tk);
        }
        else continue;

        if (refresh)
            XmTextSetString(me->w_rslt[n], buf);
    }
}

static void AxisExposureCB(Widget     w,
                           XtPointer  closure,
                           XtPointer  call_data)
{
  adc200_privrec_t *me  = (adc200_privrec_t*) closure;
  Display          *dpy = XtDisplay(w);
  Window            win = XtWindow (w);
  
  int               n;
  int               side;
  int               even;
  int               c;
  int               i;
  int               x;
  int               y;
  int               len;
  double            v;
  char              buf[100];
  
    XDrawRectangle(dpy, win, me->axisGC,
                   me->m_lft - 1, me->m_top - 1,
                   me->G_W   + 1, me->G_H   + 1);

#if 1
////    fprintf(stderr, "{\n");
    for (i = 0;  i <= V_TICK_SEGS;  i++)
    {
        c = RESCALE_VALUE((1000 * i) / V_TICK_SEGS, 0, 1000, me->ymin, me->ymax);
        y = C2Y(me, c) + me->m_top;
        even = ((i % 2) == 0);
        if (even) len = H_TICK_LEN;
        else      len = H_TICK_LEN / 2 + 1;
        
////        if (me->ymax < 128) fprintf(stderr, "i=%d: c=%-3d y=%-3d even=%d\n", i, c, y, even);
        
        XDrawLine(dpy, win, me->axisGC,
                  me->m_lft - len, y,
                  me->m_lft - 1,   y);
        XDrawLine(dpy, win, me->axisGC,
                  me->m_lft + me->G_W,           y,
                  me->m_lft + me->G_W + len - 1, y);

        if (even)
        {
            y -= (me->chan_finfo[0]->ascent + me->chan_finfo[0]->descent) / 2;

            for (side = 0;  side <= 1;  side++)
            {
                v = C2V(me, c, side);
                sprintf(buf, LFRT_TICK_FMT, v);

                if (side == 0)
                    x = me->m_lft - H_TICK_LEN - H_TICK_SPC -
                        XTextWidth(me->chan_finfo[0], buf, strlen(buf));
                else
                    x = me->m_lft + me->G_W + H_TICK_LEN + H_TICK_SPC;
                
                XDrawString(dpy, win, me->chanGC[side],
                            x, y + me->chan_finfo[side]->ascent,
                            buf, strlen(buf));
            }
        }
    }
////    fprintf(stderr, "}\n");
#else
    for (n = 0;  n < countof(c_ticks_at);  n++)
    {
        c = c_ticks_at[n];
        if (c < me->ymin  ||  c > me->ymax) continue;
        y = C2Y(me, c) + me->m_top;

        even = (n & 1) == 0;
        if (even) len = H_TICK_LEN;
        else      len = H_TICK_LEN / 2 + 1;
        
        XDrawLine(dpy, win, me->axisGC,
                  me->m_lft - len, y,
                  me->m_lft - 1,   y);
        XDrawLine(dpy, win, me->axisGC,
                  me->m_lft + me->G_W,           y,
                  me->m_lft + me->G_W + len - 1, y);

        if (even)
        {
            y -= (me->chan_finfo[0]->ascent + me->chan_finfo[0]->descent) / 2;

            for (side = 0;  side <= 1;  side++)
            {
                v = C2V(me, c, side);
                sprintf(buf, LFRT_TICK_FMT, v);

                if (side == 0)
                    x = me->m_lft - H_TICK_LEN - H_TICK_SPC -
                        XTextWidth(me->chan_finfo[0], buf, strlen(buf));
                else
                    x = me->m_lft + me->G_W + H_TICK_LEN + H_TICK_SPC;
                
                XDrawString(dpy, win, me->chanGC[side],
                            x, y + me->chan_finfo[side]->ascent,
                            buf, strlen(buf));
            }
        }
    }
#endif

#if 1
    for (i = 0;  i < me->G_W;  i += H_TICK_STEP)
    {
        n = X2N(me, i);
        x = i + me->m_rgt;
        even = (i % (H_TICK_STEP*2)) == 0;
        if (even) len = V_TICK_LEN;
        else      len = V_TICK_LEN / 2 + 1;

        XDrawLine(dpy, win, me->axisGC,
                  x, me->m_top - len,
                  x, me->m_top - 1);
        XDrawLine(dpy, win, me->axisGC,
                  x, me->m_top + me->G_H,
                  x, me->m_top + me->G_H + len - 1);

        if (even)
        {
            sprintf(buf, "%d", N2NS(me, n));

            XDrawString(dpy, win, me->axisGC,
                        x - XTextWidth(me->axis_finfo, buf, strlen(buf)) / 2,
                        me->m_top - V_TICK_LEN - V_TICK_SPC - me->axis_finfo->descent,
                        buf, strlen(buf));
            
            XDrawString(dpy, win, me->axisGC,
                        x - XTextWidth(me->axis_finfo, buf, strlen(buf)) / 2,
                        me->m_top + me->G_H +
                            V_TICK_LEN + V_TICK_SPC + me->axis_finfo->ascent,
                        buf, strlen(buf));
        }
    }
#else
    for (n = 0;  n < NUMPTS;  n += 50)
    {
        if (n < me->tmin  ||  n > me->tmax) continue;
        x = N2X(me, n) + me->m_rgt;

        even = (n % 100) == 0;
        if (even) len = V_TICK_LEN;
        else      len = V_TICK_LEN / 2 + 1;

        XDrawLine(dpy, win, me->axisGC,
                  x, me->m_top - len,
                  x, me->m_top - 1);
        XDrawLine(dpy, win, me->axisGC,
                  x, me->m_top + me->G_H,
                  x, me->m_top + me->G_H + len - 1);

        if (even)
        {
            sprintf(buf, "%d", N2NS(me, n));

            XDrawString(dpy, win, me->axisGC,
                        x - XTextWidth(me->axis_finfo, buf, strlen(buf)) / 2,
                        me->m_top - V_TICK_LEN - V_TICK_SPC - me->axis_finfo->descent,
                        buf, strlen(buf));
            
            XDrawString(dpy, win, me->axisGC,
                        x - XTextWidth(me->axis_finfo, buf, strlen(buf)) / 2,
                        me->m_top + me->G_H +
                            V_TICK_LEN + V_TICK_SPC + me->axis_finfo->ascent,
                        buf, strlen(buf));
        }
    }
#endif
}

static void DrawGraph(adc200_privrec_t *me, int do_clear)
{
  Display  *dpy = XtDisplay(me->graph);
  Drawable  drw = IS_DOUBLE_BUFFERING(me)? me->pix : XtWindow(me->graph);

  int       n;
  int       c;
  int       i;
  int       x;
  int       y;
  int       chan;
  int       ofs;
  int       xp;

    if (do_clear)
        XFillRectangle(dpy, drw, me->bkgdGC[me->curstate], 0, 0, me->G_W, me->G_H);
  
    // Grid -- horizontal lines
#if 1
    for (i = 2;  i <= V_TICK_SEGS - 2;  i += 2)
    {
        c = RESCALE_VALUE((1000 * i) / V_TICK_SEGS, 0, 1000, me->ymin, me->ymax);
        y = C2Y(me, c);
        
        XDrawLine(dpy, drw, /*c == 128? me->axisGC : */me->gridGC,
                  0, y, me->G_W-1, y);
    }
#else
    for (n = 0;  n < countof(c_ticks_at);  n += 2)
    {
        c = c_ticks_at[n];
        if (c <= me->ymin  ||  c >= me->ymax) continue;
        y = C2Y(me, c);

        XDrawLine(dpy, drw, c == 128? me->axisGC : me->gridGC,
                  0, y, me->G_W-1, y);
    }
#endif

    // Grid -- vertical lines
#if 1
    for (x = H_TICK_STEP;  x < me->G_W - 1;  x += H_TICK_STEP)
        XDrawLine(dpy, drw, me->gridGC,
                  x, 0, x, me->G_H-1);
#else
    for (n = 0;  n < NUMPTS;  n += 100)
    {
        if (n <= me->tmin  ||  n >= me->tmax) continue;
        x = N2X(me, n);

        XDrawLine(dpy, drw, me->gridGC,
                  x, 0, x, me->G_H-1);
    }
#endif

    // Repers (T0/T1)
    if (me->calc_type[0] != CALC_NONE  || me->calc_type[1] != CALC_NONE)
    {
        x = N2X(me, me->t0);
        XDrawLine(dpy, drw, me->reprGC,
                  x, 0, x, me->G_H-1);
        x = N2X(me, me->t1);
        XDrawLine(dpy, drw, me->reprGC,
                  x, 0, x, me->G_H-1);
    }

    // Data
    for (n = me->tmin;  n <= me->tmax;  n++)
    {
        x = N2X(me, n);
        for (chan = 0, ofs = 0;
             chan < 2;
             chan++,   ofs += NUMPTS)
            if (n == me->tmin)
                XDrawPoint(dpy, drw, me->chanGC[chan],
                           x,  C2Y(me, me->mes_data[ofs + n]));
            else
                XDrawLine (dpy, drw, me->chanGC[chan],
                           xp, C2Y(me, me->mes_data[ofs + n - 1]),
                           x,  C2Y(me, me->mes_data[ofs + n]));
        xp = x;
    }

    // Calculations
    for (chan = 0;  chan < 2;  chan++)
        if      (me->calc_type[chan] == CALC_AVG)
        {
            y = C2Y(me, me->avg_c[chan]);
            XDrawLine(dpy, drw, me->calcGC[chan],
                      N2X(me, me->t0), y, N2X(me, me->t1), y);
        }
        else if (me->calc_type[chan] == CALC_TG)
            ;
}

static void GraphExposureCB(Widget     w,
                            XtPointer  closure,
                            XtPointer  call_data)
{
  adc200_privrec_t *me = (adc200_privrec_t*) closure;

    DrawGraph(me, 0);
}

static void RefreshTheAxis(adc200_privrec_t *me)
{
    RefreshThe(me->axis);
}

static void RefreshTheGraph(adc200_privrec_t *me)
{
    if (IS_DOUBLE_BUFFERING(me))
    {
        DrawGraph(me, 1);
        XClearArea(XtDisplay(me->graph), XtWindow(me->graph),
                   0, 0, 0, 0, False);
    }
    else
        RefreshThe(me->graph);
}

static void UpdateBgColors(adc200_privrec_t *me)
{
  XhPixel  fg = XhGetColor(XH_COLOR_GRAPH_TITLES);
  XhPixel  bg = XhGetColor(XH_COLOR_GRAPH_BG);
  
    ChooseKnobColors(LOGC_NORMAL, me->curstate, fg, bg, &fg, &bg);
    XtVaSetValues(me->axis,  XmNbackground, bg, NULL);
    XtVaSetValues(me->graph, XmNbackground, bg, NULL);
}

static void ShowCurData(adc200_privrec_t *me)
{
  char  buf[100];
  int   n;
    
    if (me->x_of_mouse < 0) return;
    
    n = X2N(me, me->x_of_mouse);

    sprintf(buf, "pix=%d (%dns): U1=%.3fV U2=%.3fV",
            n, N2NS(me, n),
            C2V(me, me->mes_data[n],          0),
            C2V(me, me->mes_data[n + NUMPTS], 1));

    XhMakeTempMessage(me->win, buf);
}

static void T0KCB(Knob k, double v, void *privptr);
static void T1KCB(Knob k, double v, void *privptr);

static void MouseHandler(Widget     w                    __attribute__((unused)),
                         XtPointer  closure,
                         XEvent    *event,
                         Boolean   *continue_to_dispatch __attribute__((unused)))
{
  adc200_privrec_t    *me   = (adc200_privrec_t *) closure;
  XMotionEvent        *mev  = (XMotionEvent *)     event;
  XButtonEvent        *bev  = (XButtonEvent *)     event;
  XCrossingEvent      *cev  = (XCrossingEvent *)   event;

  int                  rvis = me->calc_type[0] != CALC_NONE  || me->calc_type[1] != CALC_NONE;
  int                  n;

    //fprintf(stderr, "type=%d\n", event->type);
  
    if (event->type == LeaveNotify)
    {
        me->x_of_mouse = -1;
        XhMakeTempMessage(me->win, NULL);
        return;
    }

    if      (event->type == ButtonPress  ||  event->type == ButtonRelease)
    {
        me->x_of_mouse = bev->x;
        n = X2N(me, me->x_of_mouse);
        if (rvis)
        {
            if (bev->button == Button1) SetKnobValue(me->k_t0, n), me->t0 = -1, T0KCB(me->k_t0, n, me);
            if (bev->button == Button3) SetKnobValue(me->k_t1, n), me->t1 = -1, T1KCB(me->k_t1, n, me);
        }
    }
    else if (event->type == MotionNotify)
    {
        me->x_of_mouse = mev->x;
        n = X2N(me, me->x_of_mouse);
        if (rvis)
        {
            if (mev->state & Button1Mask) SetKnobValue(me->k_t0, n), me->t0 = -1, T0KCB(me->k_t0, n, me);
            if (mev->state & Button3Mask) SetKnobValue(me->k_t1, n), me->t1 = -1, T1KCB(me->k_t1, n, me);
        }
    }
    else if (event->type == EnterNotify)
    {
        me->x_of_mouse = cev->x;
    }

    ShowCurData(me);
}

static void UpdateConnStatus(adc200_privrec_t *me)
{
    if (me->running || me->oneshot) cda_run_server(me->sid);
    else                            cda_hlt_server(me->sid);

    SetKnobValue(me->k_run, me->running? CX_VALUE_LIT_MASK : 0);
    SetKnobValue(me->k_one, me->running? CX_VALUE_DISABLED_MASK : 0);
}

static void RunKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me = (adc200_privrec_t *) privptr;
  
    me->running = !me->running;
    UpdateConnStatus(me);
}

static void OneKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me = (adc200_privrec_t *) privptr;

    me->oneshot = 1;
    UpdateConnStatus(me);
}
  
static void YminKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);

    if (val > me->ymax - 1)
        SetKnobValue(k, val = me->ymax - 1);
  
    if (me->ymin != val)
    {
        me->ymin  = val;
        RefreshTheAxis (me);
        RefreshTheGraph(me);
    }
}

static void YmaxKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);

    if (val < me->ymin + 1)
        SetKnobValue(k, val = me->ymin + 1);
  
    if (me->ymax != val)
    {
        me->ymax  = val;
        RefreshTheAxis (me);
        RefreshTheGraph(me);
    }
}

static void TminKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);

    if (val < 0)
        SetKnobValue(k, val = 0);
    if (val > me->tmax - 1)
        SetKnobValue(k, val = me->tmax - 1);

    if (me->tmin != val)
    {
        me->tmin  = val;

        if (me->t0 < val)
            SetKnobValue(me->k_t0, me->t0 = val);
        if (me->t1 < val)
            SetKnobValue(me->k_t1, me->t1 = val);

        RefreshTheAxis (me);
        RefreshTheGraph(me);
        ShowCurData    (me);
    }
}

static void TmaxKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);

    if (val > NUMPTS-1)
        SetKnobValue(k, val = NUMPTS-1);
    if (val < me->tmin + 1)
        SetKnobValue(k, val = me->tmin + 1);

    if (me->tmax != val)
    {
        me->tmax  = val;

        if (me->t0 > val)
            SetKnobValue(me->k_t0, me->t0 = val);
        if (me->t1 > val)
            SetKnobValue(me->k_t1, me->t1 = val);

        RefreshTheAxis (me);
        RefreshTheGraph(me);
        ShowCurData    (me);
    }
}

static void SetCalcKnobsSensitivity(adc200_privrec_t *me)
{
  int  t_s = me->calc_type[0] != CALC_NONE  || me->calc_type[1] != CALC_NONE;
  int  k_s = me->calc_type[0] == CALC_TG    || me->calc_type[1] == CALC_TG;
    
    XtSetSensitive(GetKnobWidget(me->k_t0), t_s);
    XtSetSensitive(GetKnobWidget(me->k_t1), t_s);
    XtSetSensitive(GetKnobWidget(me->k_tk), k_s);
}

static void CalcKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me =  (adc200_privrec_t *) privptr;
  int               val = round(v);
  int               chn = (k == me->k_calc[0]? 0 : 1);

    me->calc_type[chn] = val;
    SetCalcKnobsSensitivity(me);
    if (me->calc_type[chn] == CALC_NONE) XmTextSetString(me->w_rslt[chn], "");
    PerformCalcs   (me, 1);
    RefreshTheGraph(me);
}

static void T0KCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);

    if (val < me->tmin)
        SetKnobValue(k, val = me->tmin);
    if (val > me->t1)
        SetKnobValue(k, val = me->t1);

    if (me->t0 != val)
    {
        me->t0 = val;
        PerformCalcs   (me, 1);
        RefreshTheGraph(me);
    }
}

static void T1KCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);

    if (val > me->tmax)
        SetKnobValue(k, val = me->tmax);
    if (val < me->t0)
        SetKnobValue(k, val = me->t0);

    if (me->t1 != val)
    {
        me->t1 = val;
        PerformCalcs   (me, 1);
        RefreshTheGraph(me);
    }
}

static void TKKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me = (adc200_privrec_t *) privptr;

    me->tk = v;
    PerformCalcs   (me, 1);
    RefreshTheGraph(me);
}

static void UpdateRangeSelector(adc200_privrec_t *me, int side)
{
  int     j  = me->v_j[side];
  Widget  wl[2];
    
    wl[0] = GetKnobWidget(me->k_scale[side][0]);
    wl[1] = GetKnobWidget(me->k_scale[side][1]);
    me->k_params[NADC200_PARAM_RANGE1 + side] = me->k_scale[side][j];
    XtChangeManagedSet(wl + (1 - j), 1, NULL, NULL, wl + j, 1);
}

static void JKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);
  
  int               n   = (k == me->k_j[0] ? 0 : 1);
  
    me->v_j[n] = val;
    UpdateRangeSelector(me, n);
    RefreshTheAxis(me);
    ShowCurData   (me);
}

static void ParamKCB(Knob k, double v, void *privptr)
{
  adc200_privrec_t *me  = (adc200_privrec_t *) privptr;
  int               val = round(v);

  int              n;

    for (n = 0;  n < countof(me->k_params);  n++)
        if (k == me->k_params[n])
            cda_setbigcparams(me->bigch, n, 1, &val);
}

static void init_param(cda_bigchandle_t bigch, int n, int32 v)
{
    cda_setbigcparams (bigch, n, 1, &v);
    cda_setbigcparam_p(bigch, n, 1);
}

static void BigcEventProc(cda_serverid_t  sid __attribute__((unused)),
                          int             reason,
                          void           *privptr)
{
  adc200_privrec_t *me = privptr;
  int               n;
  colalarm_t        newstate;

  static char      roller_states[4] = "/-\\|";
  char             buf[100];
  
    if (reason != CDA_R_MAINDATA) return;
    if (!(me->running  ||  me->oneshot)) return;
    me->oneshot = 0;
    UpdateConnStatus(me);

    if (!me->len_set)
    {
      int32  v;
        
        v = NUMPTS;
        cda_setbigcparams (me->bigch, NADC200_PARAM_NUMPTS, 1, &v);
        me->len_set = 1;
    }

    cda_getbigcstats (me->bigch, &(me->mes_tag), &(me->mes_rflags));
    cda_getbigcparams(me->bigch, 0, NADC200_NUM_PARAMS, me->mes_info);
    cda_getbigcdata  (me->bigch, 0, NUMPTS*2, me->mes_data);
    cda_continue(me->sid);

    newstate = datatree_choose_knobstate(NULL, me->mes_rflags);
    if (me->curstate != newstate)
    {
        me->curstate = newstate;
        UpdateBgColors(me);
    }

    /* Display current params */
    for (n = 0;  n < NADC200_NUM_PARAMS;  n++)
        if (me->k_params[n] != NULL)
            SetKnobValue(me->k_params[n], me->mes_info[n]);
    
    PerformCalcs   (me, 1);
    RefreshTheGraph(me);
    ShowCurData    (me);

    if (
        me->mes_info[NADC200_PARAM_TIMING] != me->prev_mes_info[NADC200_PARAM_TIMING]  ||
        me->mes_info[NADC200_PARAM_FRQDIV] != me->prev_mes_info[NADC200_PARAM_FRQDIV]  ||
        me->mes_info[NADC200_PARAM_ZERO1 ] != me->prev_mes_info[NADC200_PARAM_ZERO1 ]  ||
        me->mes_info[NADC200_PARAM_RANGE1] != me->prev_mes_info[NADC200_PARAM_RANGE1]  ||
        me->mes_info[NADC200_PARAM_ZERO2 ] != me->prev_mes_info[NADC200_PARAM_ZERO2 ]  ||
        me->mes_info[NADC200_PARAM_RANGE2] != me->prev_mes_info[NADC200_PARAM_RANGE2]
       )
        RefreshTheAxis(me);
    
    memcpy(me->prev_mes_info, me->mes_info, sizeof(me->mes_info));

    me->roller_ctr = (me->roller_ctr + 1) & 3;
    sprintf(buf, "%c", roller_states[me->roller_ctr]);
    XmTextSetString(me->w_roller, buf);
}

static void SwitchContainerFoldedness(eleminfo_t *e)
{
  Widget        l1[1];
  Widget        l2[1];
  int           x;

    l1[0] = CNCRTZE(e->innage);
    l2[0] = CNCRTZE(e->folded);

    for (x = 0;  x < countof(l1);  x++) if (l1[x] == NULL) return;
    for (x = 0;  x < countof(l2);  x++) if (l2[x] == NULL) return;
    
    if (XtIsManaged(l1[0]))
        XtChangeManagedSet(l1, countof(l1), NULL, NULL, l2, countof(l2));
    else
        XtChangeManagedSet(l2, countof(l2), NULL, NULL, l1, countof(l1));
}

static void TitleClickHandler(Widget     w,
                              XtPointer  closure,
                              XEvent    *event,
                              Boolean   *continue_to_dispatch)
{
  eleminfo_t   *e   = (eleminfo_t *)  closure;

    if (CheckForDoubleClick(w, event, continue_to_dispatch))
        SwitchContainerFoldedness(e);
}

typedef struct
{
    int  bigc;
} adc200_elemopts_t;

static psp_paramdescr_t text2adc200_elemopts[] =
{
    PSP_P_INT("bigc", adc200_elemopts_t, bigc, 0, 0, 0),
    PSP_P_END()
};

int ELEM_NADC200_Create_m(XhWidget parent, ElemInfo e)
{
  adc200_privrec_t  *me;
  adc200_elemopts_t  opts;

  Dimension          fst;
  XmString           s;
  char               buf[1000];
  const char        *label_s;
  
  Widget             frame;
  Widget             lform;
  Widget             space;
  Widget             c_lft;
  Widget             c_rgt;
  Widget             c_top;
  Widget             c_bot;
  Widget             c_run;
  Widget             sform;
  
  Dimension          total_w;
  Dimension          total_h;

  Dimension          w_w;
  Dimension          w_h;

  int                side;
  Widget             side_w;
  const char        *att_side;
  Widget             cur;
  Widget             top;

  cda_serverid_t     mainsid;
  const char        *srvname;

    if ((me = e->elem_private = XtMalloc(sizeof(adc200_privrec_t))) == NULL)
        return -1;
    bzero(me, sizeof(*me));
    me->e   = e;
    me->win = XhWindowOf(parent);

    bzero(&opts, sizeof(opts));
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2adc200_elemopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
    }

    e->container = ABSTRZE(XtVaCreateManagedWidget("elementForm",
                                                   xmFormWidgetClass,
                                                   CNCRTZE(parent),
                                                   //XmNshadowThickness, 3,
                                                   NULL));
    XtVaGetValues(CNCRTZE(e->container),
                  XmNbackground,      &(e->wdci.defbg),
                  XmNshadowThickness, &fst,
                  NULL);

    label_s = e->label;
    if (label_s    == NULL) label_s = "(UNLABELED)";
    if (label_s[0] == '\0') label_s = " ";

    snprintf(buf, sizeof(buf), "{%s}", label_s);
    e->folded =
        ABSTRZE(XtVaCreateWidget("caption2",
                                 xmLabelWidgetClass,
                                 CNCRTZE(e->container),
                                 XmNleftAttachment,   XmATTACH_FORM,
                                 XmNleftOffset,       fst,
                                 XmNrightAttachment,  XmATTACH_FORM,
                                 XmNrightOffset,      fst,
                                 XmNtopAttachment,    XmATTACH_FORM,
                                 XmNtopOffset,        fst,
                                 XmNbottomAttachment, XmATTACH_FORM,
                                 XmNbottomOffset,     fst,
                                 XmNlabelString,      s = XmStringCreateLtoR(buf, xh_charset),
                                 NULL));
    XmStringFree(s);
    XhSetBalloon(e->folded, "Double-click to expand");

    XtAddEventHandler(CNCRTZE(e->folded), ButtonPressMask, False,
                      TitleClickHandler, (XtPointer)e);
    
    //////////////////////////////////////////////////////////////////
    
    e->innage = XhCreateGrid(CNCRTZE(e->container), "grid");
    XhGridSetGrid   (e->innage, 0, 0);
    XhGridSetSize   (e->innage, 3, 3);
    XhGridSetPadding(e->innage, 0, 0);
    XhGridSetSpacing(e->innage, 0, 0);

    XtVaSetValues(CNCRTZE(e->innage),
                  XmNleftAttachment,   XmATTACH_FORM,
                  XmNleftOffset,       fst,
                  XmNrightAttachment,  XmATTACH_FORM,
                  XmNrightOffset,      fst,
                  XmNtopAttachment,    XmATTACH_FORM,
                  XmNtopOffset,        fst,
                  XmNbottomAttachment, XmATTACH_FORM,
                  XmNbottomOffset,     fst,
                  NULL);
    
    //////////////////////////////////////////////////////////////////
    
    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
                                    e->innage,
                                    XmNshadowType,      XmSHADOW_IN,
                                    XmNshadowThickness, 1,
                                    NULL);
    XhGridSetChildPosition(frame, 1, 1);

    AllocStateGCs(frame, me->bkgdGC, XH_COLOR_GRAPH_BG);
    me->axisGC    = AllocXhGC  (frame, XH_COLOR_GRAPH_AXIS,   XH_TINY_FIXED_FONT); me->axis_finfo    = last_finfo;
    me->gridGC    = AllocDashGC(frame, XH_COLOR_GRAPH_GRID,   GRID_DASHLENGTH);
    me->reprGC    = AllocXhGC  (frame, XH_COLOR_GRAPH_REPERS, NULL);
    me->chanGC[0] = AllocXhGC  (frame, XH_COLOR_GRAPH_LINE1,  XH_TINY_FIXED_FONT); me->chan_finfo[0] = last_finfo;
    me->chanGC[1] = AllocXhGC  (frame, XH_COLOR_GRAPH_LINE2,  XH_TINY_FIXED_FONT); me->chan_finfo[1] = last_finfo;
    me->calcGC[0] = AllocXhGC  (frame, XH_COLOR_GRAPH_LINE3,  NULL);
    me->calcGC[1] = AllocXhGC  (frame, XH_COLOR_GRAPH_LINE4,  NULL);

    sprintf(buf, LFRT_TICK_FMT, 1.234);
    me->G_W   = DEF_G_W;
    me->G_H   = DEF_G_H;
    me->m_lft = me->m_rgt =
        H_TICK_MRG +
        XTextWidth(me->chan_finfo[0], buf, strlen(buf)) +
        H_TICK_SPC +
        H_TICK_LEN;
    me->m_top = me->m_bot =
        V_TICK_MRG +
        me->axis_finfo->ascent + me->axis_finfo->descent +
        V_TICK_SPC +
        V_TICK_LEN;
    total_w = me->m_lft + me->G_W + me->m_rgt;
    total_h = me->m_top + me->G_H + me->m_bot;
    me->axis = XtVaCreateManagedWidget("drawing_area", xmDrawingAreaWidgetClass,
                                       frame,
                                       XmNwidth,        total_w,
                                       XmNheight,       total_h,
                                       XmNmarginWidth,  0,
                                       XmNmarginHeight, 0,
                                       XmNtraversalOn,  False,
                                       NULL);
    XtAddCallback    (me->axis, XmNexposeCallback,  AxisExposureCB,  (XtPointer)me);

    me->pix = XmUNSPECIFIED_PIXMAP;
#if 0
    me->pix = XCreatePixmap(XtDisplay(me->axis),
                            RootWindowOfScreen(XtScreen(me->axis)),
                            me->G_W, me->G_H,
                            DefaultDepthOfScreen(XtScreen(me->axis)));
#endif
    
    me->graph = XtVaCreateManagedWidget("drawing_area", xmDrawingAreaWidgetClass,
                                        me->axis,
                                        XmNx,           me->m_lft,
                                        XmNy,           me->m_top,
                                        XmNwidth,       me->G_W,
                                        XmNheight,      me->G_H,
                                        XmNtraversalOn, False,
                                        XmNbackgroundPixmap, me->pix,
                                        NULL);
    me->x_of_mouse = -1;

    me->curstate = COLALARM_JUSTCREATED;
    UpdateBgColors(me);

    if (!IS_DOUBLE_BUFFERING(me))
        XtAddCallback    (me->graph, XmNexposeCallback, GraphExposureCB, (XtPointer)me);
    XtAddEventHandler(me->graph,
                      (ButtonPressMask   | ButtonReleaseMask |
                       PointerMotionMask |
                       Button1MotionMask | Button2MotionMask | Button3MotionMask |
                       EnterWindowMask   | LeaveWindowMask),
                      False, MouseHandler,  (XtPointer)me);

    //////////////////////////////////////////////////////////////////

    lform = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, e->innage,
                                    XmNshadowThickness, 0,
                                    NULL);
    XhGridSetChildPosition (lform,  0,  1);
    XhGridSetChildAlignment(lform,  0,  0);
    XhGridSetChildFilling  (lform,  1,  1);

    e->caption =
        ABSTRZE(XtVaCreateManagedWidget("caption1",
                                        xmLabelWidgetClass,
                                        lform,
                                        XmNlabelString,      s = XmStringCreateLtoR(label_s, xh_charset),
                                        NULL));
    XmStringFree(s);
    XhAssignVertLabel(e->caption, NULL, 0);
    XtAddEventHandler(CNCRTZE(e->caption), ButtonPressMask, False,
                      TitleClickHandler, (XtPointer)e);
    attachleft  (e->caption, NULL, 0);
    attachtop   (e->caption, NULL, 0);
    attachbottom(e->caption, NULL, 0);

    //////////////////////////////////////////////////////////////////
    c_run = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, e->innage,
                                    XmNshadowThickness, 0,
                                    NULL);
    XhGridSetChildPosition (c_run,  2,  0);
    XhGridSetChildAlignment(c_run, +1, -1);
    XhGridSetChildFilling  (c_run,  0, 0);
    
    me->k_run =
        CreateSimpleKnob(" rw local look=arrow_rt"
                         " comment='Periodic measurements'"
                         " widdepinfo='color=green'",
                         0, c_run, RunKCB, me);
    SetKnobValue(me->k_run, me->running = 1);

    me->k_one =
        CreateSimpleKnob(" rw local look=button label='1'"
                         " comment='One measurement'",
                         0, c_run, OneKCB, me);
    SetKnobValue(me->k_one, me->running? CX_VALUE_DISABLED_MASK : 0);
    attachleft(GetKnobWidget(me->k_one), GetKnobWidget(me->k_run), 0);

    me->k_params[NADC200_PARAM_ISTART] =
        CreateSimpleKnob(" rw look=greenled label=' ! '"
                         " comment='Fake TRIGGER'"
                         " widdepinfo='panel'",
                         0, c_run, ParamKCB, me);
    attachleft(GetKnobWidget(me->k_params[NADC200_PARAM_ISTART]), GetKnobWidget(me->k_one), 0);

    me->w_roller =
        XtVaCreateManagedWidget("text_o", xmTextWidgetClass, c_run,
                                XmNvalue, "",
                                XmNcursorPositionVisible, False,
                                XmNcolumns,               1,
                                XmNscrollHorizontal,      False,
                                XmNeditMode,              XmSINGLE_LINE_EDIT,
                                XmNeditable,              False,
                                XmNtraversalOn,           False,
                                NULL);
    me->roller_ctr = 0;
    attachleft(me->w_roller, GetKnobWidget(me->k_params[NADC200_PARAM_ISTART]), 0);
    //////////////////////////////////////////////////////////////////

    c_lft = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, lform,
                                    XmNshadowThickness, 0,
                                    NULL);
    attachright(c_lft, NULL, 0);

    space = XtVaCreateManagedWidget("", widgetClass, lform,
                                    XmNborderWidth,     0,
                                    NULL);
    attachleft (space, e->caption, 0);
    attachright(space, c_lft,      0);
    
    c_rgt = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, e->innage,
                                    XmNshadowThickness, 0,
                                    NULL);
    XhGridSetChildPosition(c_rgt, 2, 1);
    XhGridSetChildFilling (c_rgt, 0, 1);

    c_top = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, e->innage,
                                    XmNshadowThickness, 0,
                                    NULL);
    XhGridSetChildPosition(c_top, 1, 0);
    XhGridSetChildFilling (c_top, 1, 0);

    c_bot = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, e->innage,
                                    XmNshadowThickness, 0,
                                    NULL);
    XhGridSetChildPosition(c_bot, 1, 2);
    XhGridSetChildFilling (c_bot, 1, 0);

    me->k_ymin =
        CreateSimpleKnob(" rw local look=text label='Ymin:'"
                         " dpyfmt='%3.0f' "
                         " minnorm=0 maxnorm=254"
                         " widdepinfo='withlabel'",
                         0, e->innage, YminKCB, me);
    SetKnobValue(me->k_ymin, me->ymin = 0);
    XhGridSetChildPosition (GetKnobWidget(me->k_ymin),  0, 2);
    XhGridSetChildAlignment(GetKnobWidget(me->k_ymin), +1, 0);

    me->k_ymax =
        CreateSimpleKnob(" rw local look=text label='Ymax:'"
                         " dpyfmt='%3.0f' "
                         " minnorm=1 maxnorm=255"
                         " widdepinfo='withlabel'",
                         0, e->innage, YmaxKCB, me);
    SetKnobValue(me->k_ymax, me->ymax = 255);
    XhGridSetChildPosition (GetKnobWidget(me->k_ymax),  0, 0);
    XhGridSetChildAlignment(GetKnobWidget(me->k_ymax), +1, 0);

    me->k_tmin =
        CreateSimpleKnob(" rw local look=text label='Tmin:'"
                         " dpyfmt='%3.0f' "
                         " widdepinfo='withlabel'",
                         0, c_bot, TminKCB, me);
    SetKnobValue(me->k_tmin, me->tmin = 0);

    me->k_tmax =
        CreateSimpleKnob(" rw local look=text label='Tmax:'"
                         " dpyfmt='%3.0f' "
                         " widdepinfo='withlabel'",
                         0, c_bot, TmaxKCB, me);
    SetKnobValue(me->k_tmax, me->tmax = me->G_W-1);
    attachright(GetKnobWidget(me->k_tmax), NULL, 0);

    me->k_params[NADC200_PARAM_TIMING] =
            CreateSimpleKnob(" rw look=selector label=''"
                             " comment='Timing:\n200MHz quartz, 195MHz impact, or External TIMER'"
                             " widdepinfo='#C#-#T200MHz\v195Mhz\vExt'",
                             0, c_bot, ParamKCB, me);
    attachright(GetKnobWidget(me->k_params[NADC200_PARAM_TIMING]), NULL, total_w / 2 + CHL_INTERKNOB_H_SPACING);
    
    me->k_params[NADC200_PARAM_FRQDIV] =
            CreateSimpleKnob(" rw look=selector label=''"
                             " comment='Freqency divisor (ns per quant):\nf/1 (5ns), f/2 (10ns), f/4 (20ns), f/8 (40ns)'"
                             " widdepinfo='#C#-#T5ns\v10ns\v20ns\v40ns'",
                             0, c_bot, ParamKCB, me);
    attachleft(GetKnobWidget(me->k_params[NADC200_PARAM_FRQDIV]), NULL, total_w / 2 + CHL_INTERKNOB_H_SPACING);
    
    for (side = 0;  side <= 1;  side++)
    {
        side_w   = (side == 0? c_lft : c_rgt);
        att_side = (side == 0? XmNrightAttachment : XmNleftAttachment);

        top = NULL;
        
        sprintf(buf,
                " rw local look=selector label='Chan %d:'"
                " comment='Type of calculation:\nNone (-), Average value, Tangent'"
                " widdepinfo='#C#-#T-\vAvg\vtg'",
                side + 1);
        me->k_calc[side] =
            CreateSimpleKnob(buf,
                             0, side_w, CalcKCB, me);
        SetKnobValue(me->k_calc[side], me->calc_type[side] = CALC_NONE);
        cur = GetKnobWidget(me->k_calc[side]);
        attachtop(cur, NULL, CHL_INTERELEM_V_SPACING);
        XtVaSetValues(cur, att_side, XmATTACH_FORM, NULL);
        top = cur;

        me->w_rslt[side] =
            XtVaCreateManagedWidget("text_o", xmTextWidgetClass, side_w,
                                    XmNvalue, "",
                                    XmNcursorPositionVisible, False,
                                    XmNcolumns,               7,
                                    XmNscrollHorizontal,      False,
                                    XmNeditMode,              XmSINGLE_LINE_EDIT,
                                    XmNeditable,              False,
                                    XmNtraversalOn,           False,
                                    NULL);
        cur = me->w_rslt[side];
        attachtop(cur, top, CHL_INTERKNOB_V_SPACING*0);
        XtVaSetValues(cur, att_side, XmATTACH_FORM, NULL);
        top = cur;

        sprintf(buf,
                " rw local look=onoff label='J%d=1,2'"
                " comment='Jumper J%d state:\n"
                          "2,3 - ranges are ±0.256,0.512,1.024,2.048V\n"
                          "1,2 - ranges are ±1.024,2.048,4.096,8.192V'"
                "",
                5 + side, 5 + side);
        me->k_j[side] =
            CreateSimpleKnob(buf, 0, side_w, JKCB, me);
        SetKnobValue(me->k_j[side], me->v_j[side] = 1);
        cur = GetKnobWidget(me->k_j[side]);
        attachtop(cur, top, CHL_INTERELEM_V_SPACING);
        XtVaSetValues(cur, att_side, XmATTACH_FORM, NULL);
        top = cur;

        {
            sform = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, side_w,
                                            XmNshadowThickness, 0,
                                            NULL);
            XtVaSetValues(GetKnobWidget(sform), att_side, XmATTACH_FORM, NULL);

            me->k_scale[side][0] =
                CreateSimpleKnob(" rw local look=selector label=''"
                                 " comment='Measurement scale:\n±0.256V, ±0.512V, ±1.024V, ±2.048V'"
                                 " widdepinfo='#C#-#T0.256\v0.512\v1.024\v2.048'",
                                 0, sform, ParamKCB, me);

            me->k_scale[side][1] =
                CreateSimpleKnob(" rw local look=selector label=''"
                                 " comment='Measurement scale:\n±1.024V, ±2.048V, ±4.096V, ±8.192V'"
                                 " widdepinfo='#C#-#T1.024\v2.048\v4.096\v8.192'",
                                 0, sform, ParamKCB, me);
        }
        cur = sform;
        attachtop(cur, top, CHL_INTERKNOB_V_SPACING);
        XtVaSetValues(cur, att_side, XmATTACH_FORM, NULL);
        top = cur;

        me->k_params[NADC200_PARAM_ZERO1 + side] =
            CreateSimpleKnob(" rw look=text label=''"
                             " dpyfmt='%3.0f'"
                             " comment='Zero shift: 0-255'"
                             " widdepinfo='withlabel'",
                             0, side_w, ParamKCB, me);
        cur = GetKnobWidget(me->k_params[NADC200_PARAM_ZERO1 + side]);
        attachtop(cur, top, CHL_INTERKNOB_V_SPACING);
        XtVaSetValues(cur, att_side, XmATTACH_FORM, NULL);
        top = cur;

        UpdateRangeSelector(me, side);
    }
        
    me->k_t0 =
        CreateSimpleKnob(" rw local look=text label='T0:'"
                         " dpyfmt='%3.0f'"
                         " widdepinfo='withlabel'",
                         0, c_top, T0KCB, me);
    SetKnobValue(me->k_t0, me->t0 = me->tmin + 60);

    me->k_t1 =
        CreateSimpleKnob(" rw local look=text label='T1:'"
                         " dpyfmt='%3.0f'"
                         " widdepinfo='withlabel'",
                         0, c_top, T1KCB, me);
    SetKnobValue(me->k_t1, me->t1 = me->tmax - 60);
    attachright(GetKnobWidget(me->k_t1), NULL, 0);

    me->k_tk =
        CreateSimpleKnob(" rw local look=text label='k:'"
                         " dpyfmt='%5.2f'"
                         /*" minnorm=0 maxnorm=255"*/
                         " widdepinfo='withlabel'",
                         0, c_top, TKKCB, me);
    SetKnobValue(me->k_tk, me->tk = 1.234);
    XtVaGetValues(GetKnobWidget(me->k_tk), XmNwidth, &w_w, NULL);
    attachleft(GetKnobWidget(me->k_tk), NULL, (me->G_W - w_w) / 2);

    SetCalcKnobsSensitivity(me);
    
    /* Make a server connection */
    mainsid = ChlGetMainsid(XhWindowOf(parent));
    srvname = cda_status_srvname(mainsid, 0);

    if ((me->sid = cda_new_server(srvname,
                                  BigcEventProc, me,
                                  CDA_BIGC)) == CDA_SERVERID_ERROR)
    {
        fprintf(stderr, "%s: %s: cda_new_server(server=%s): %s\n",
                strcurtime(), __FUNCTION__, srvname, cx_strerror(errno));
        exit(1);
    }
    
    me->bigch = cda_add_bigc(me->sid, opts.bigc,
                             NADC200_NUM_PARAMS, sizeof(uint8)*2*NUMPTS,
                             CX_CACHECTL_SHARABLE, CX_BIGC_IMMED_YES*1);
    //init_param(me->bigch, NADC200_PARAM_FRQDIV, ADC200_FRQD_5NS);
    //init_param(me->bigch, NADC200_PARAM_RANGE1, DEF_RANGE12);
    //init_param(me->bigch, NADC200_PARAM_RANGE2, DEF_RANGE12);
    //init_param(me->bigch, NADC200_PARAM_ZERO1,  DEF_Z12);
    //init_param(me->bigch, NADC200_PARAM_ZERO2,  DEF_Z12);
    //init_param(me->bigch, NADC200_PARAM_PTSOFS, 0);
    //init_param(me->bigch, NADC200_PARAM_NUMPTS, NUMPTS);
    //init_param(me->bigch, NADC200_PARAM_STATUS, 0); /// Be-e-e...
    //init_param(me->bigch, NADC200_PARAM_TIMING, ADC200_T_200MHZ);

    UpdateConnStatus(me);

    if (IS_DOUBLE_BUFFERING(me))
        DrawGraph(me, 1);
    
    return 0;
}
