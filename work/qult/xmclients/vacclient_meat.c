#include "vacclient_includes.h"

#include <math.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>

#include "Xh_globals.h"
#include "Xh_fallbacks.h"
#include "Knobs_typesP.h"
#include "KnobsI.h"


typedef struct
{
    Widget      da;
    knobinfo_t *ki;
} bar_t;


static Widget       numeric_view;
static Widget       graphic_view;
static int          displayed_part = 1;
static int          is_logarithmic = 1;

static groupelem_t *vac_grouplist = NULL;

static int          numbars = 0;
static bar_t       *bars    = NULL;

static knobinfo_t *active_fki = NULL;

static int          cur_alarm      = 0;
static int          alarm_flipflop = 0;
static int          alarm_acked    = 0;

static Widget       i_scale;
static Widget       u_scale;


static GC bkgdGC[2];
static GC axis_GC;
static GC outl_GC;
static GC ilim_GC;
static GC ibarGC[COLALARM_LAST + 1];
static GC ired_GC;
static GC ured_GC;
static GC urlx_GC;

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


static void CreateGCs(Widget w)
{
  int  i;
    
    bkgdGC[0] = AllocXhGC(w, XH_COLOR_GRAPH_BG,        NULL);
    bkgdGC[1] = AllocXhGC(w, XH_COLOR_JUST_RED,        NULL);

    axis_GC    = AllocXhGC(w, XH_COLOR_GRAPH_AXIS,     XH_TINY_FIXED_FONT);
    outl_GC    = AllocXhGC(w, XH_COLOR_GRAPH_OUTL,     NULL);
    ilim_GC    = AllocXhGC(w, XH_COLOR_GRAPH_REPERS,   NULL);

    AllocStateGCs(ABSTRZE(w), ibarGC, XH_COLOR_JUST_GREEN*0+XH_COLOR_GRAPH_BAR1);

    ired_GC = AllocXhGC(w, XH_COLOR_JUST_RED,          NULL);
    ured_GC = AllocXhGC(w, XH_COLOR_JUST_BLUE,         NULL);
    urlx_GC = AllocXhGC(w, XH_COLOR_GRAPH_PREV,        NULL);
}


/* Begin geometry management ( */
enum
{
    COLHEIGHT = 128,

    MIN_I = 0, MAX_I = 1100, I_FACTOR = 10, I_STEP = 100,
    MIN_U = 0, MAX_U = 300,  U_FACTOR = 4,  U_STEP = 32,
    
    TICK_LEN = 4,

    SCALE_I = 0,
    SCALE_U = 1,
};

static int v2y(double v, int min_v, int max_v, int height)
{
  double  s_v;  // Scaled (if required, logarithmically) v
  double  s_h;  // ~ h
    
    s_v = v - min_v;
    s_h = max_v - min_v;
  
    if (is_logarithmic)
    {
        if (s_v <= 0) return COLHEIGHT - 1;
        s_v = log(s_v + 1);
        s_h = log(s_h + 1);
    }
    
    return (COLHEIGHT - 1) -
        (int)( (s_v * (COLHEIGHT - 1)) / s_h );
}

static inline int I2y(double I)
{
    return v2y(I, MIN_I, MAX_I, COLHEIGHT);
}

static inline int U2y(double U)
{
    return v2y(U, MIN_U, MAX_U, COLHEIGHT);
}

/* End geometry management ) */

static void DrawBar(Widget da, knobinfo_t *fki)
{
  Display    *dpy   = XtDisplay(da);
  Window      win   = XtWindow (da);

  GC          barGC;
  GC          bkgGC;
  
  double      i_mes = fki[KNOB_OFS_IMES].curv;
  double      i_lim = fki[KNOB_OFS_ISET].curv;

  rflags_t    iflags = fki[KNOB_OFS_IMES].currflags
                     | fki[KNOB_OFS_ISET].currflags
                     | fki[KNOB_OFS_IALM].currflags;
  rflags_t    uflags = fki[KNOB_OFS_UALM].currflags;
  colalarm_t  istate = datatree_choose_knobstate(fki + KNOB_OFS_IALM, iflags); // KNOB_OFS_IALM -- for "RELAX" state

  Dimension   col_width;

  int         y0, ylim;
  int         y1, y2;
  int         x1, x2;
  
  enum
  {
      L_MARGIN = 1,
      R_MARGIN = 1,
  };

    /* 1. Select GCs */
    if      ((iflags & CXCF_FLAG_ALARM_ALARM) != 0)
        barGC = ired_GC;
    else if (istate == COLALARM_NONE  &&  (uflags & CXCF_FLAG_ALARM_MASK) != 0)
        barGC = (uflags & CXCF_FLAG_ALARM_ALARM)? ured_GC : urlx_GC;
    else
        barGC = ibarGC[istate];

    if      (istate == COLALARM_RELAX)
        bkgGC = ibarGC[COLALARM_RELAX];
    else if (istate == COLALARM_NONE  &&  (uflags & CXCF_FLAG_ALARM_RELAX) != 0)
        bkgGC = urlx_GC;
    else
        bkgGC = bkgdGC[alarm_flipflop];

    /* 2. Obtain+calculate geometry */
    XtVaGetValues(da, XmNwidth, &col_width, NULL);

    x1 = L_MARGIN;
    x2 = col_width - R_MARGIN - 1;

    y0   = I2y(0);
    ylim = I2y(i_lim);
    y1   = I2y(i_mes);
    y2   = y0;

    if ((iflags & CXCF_FLAG_SYSERR_MASK) != 0)
        y1 = 0;
    
    XDrawLine(dpy, win, axis_GC,
              0, y0, col_width-1, y0);
    
    XFillRectangle(dpy, win, barGC,
                   x1, y1,
                   x2 - x1 + 1, y2 - y1 + 1);

    XDrawRectangle(dpy, win, outl_GC,
                   x1,          y1,
                   x2 - x1 + 0, y2 - y1 + 0);

    if (y1 > 0)
        XFillRectangle(dpy, win, bkgGC,
                       x1,          0,
                       x2 - x1 + 1, y1 - 1 + 1);
    
    if (i_lim > 0) XDrawLine(dpy, win, ilim_GC, x1, ylim, x2, ylim);

    if (L_MARGIN > 0)
        XFillRectangle(dpy, win, bkgGC,
                       0, 0,
                       L_MARGIN, COLHEIGHT - 1/* "-1" -- to preserve axis */);
    if (R_MARGIN > 0)
        XFillRectangle(dpy, win, bkgGC,
                       col_width - R_MARGIN, 0,
                       R_MARGIN, COLHEIGHT - 1/* "-1" -- to preserve axis */);
}

static void BarExposureCB(Widget     w,
                       XtPointer  closure,
                       XtPointer  call_data __attribute__((unused)))
{
    DrawBar(w, (knobinfo_t *)closure);
}

static void DisplayAllBars(void)
{
  int  n;
  
    for (n = 0;  n < numbars;  n++)
        DrawBar(bars[n].da, bars[n].ki);
}

static char *put_trimmed_val(knobinfo_t *ki, char *buf, size_t bufsize)
{
  char *p;
  int   frsize;

    p = snprintf_dbl_trim(buf, bufsize, ki->dpyfmt, ki->curv, 1);
    frsize = bufsize-1 - (p - buf) - strlen(p);
    if (frsize > 0  &&  ki->units != NULL)
        snprintf(p + strlen(p), frsize, "%s", ki->units);

    return p;
}

static void DisplayBarStats(XhWindow win)
{
  knobinfo_t *imes_ki;
  knobinfo_t *umes_ki;
  char        i_buf[100];
  char        u_buf[100];
  char       *i_p;
  char       *u_p;
    
  char         lbuf[1000];
  char         tbuf[1000];
  char        *caption;
  char        *tip;

    if (active_fki == NULL) return;

    if (active_fki->uplink != NULL)
        get_knob_rowcol_label_and_tip(active_fki->uplink->rownames,
                                      (active_fki - active_fki->uplink->content) / KNOBS_PER_COLUMN,
                                      active_fki,
                                      lbuf, sizeof(lbuf), tbuf, sizeof(tbuf),
                                      &caption, &tip);
    else
        caption = "";
            
    imes_ki = active_fki + KNOB_OFS_IMES;
    umes_ki = active_fki + KNOB_OFS_UMES;
    
    i_p = put_trimmed_val(imes_ki, i_buf, sizeof(i_buf));
    u_p = put_trimmed_val(umes_ki, u_buf, sizeof(u_buf));
    
    XhMakeTempMessage(win, "%s: I=%s U=%s", caption, i_p, u_p);
}

static void PointerHandler(Widget     w,
                           XtPointer  closure,
                           XEvent    *event,
                           Boolean   *continue_to_dispatch)
{
  knobinfo_t *fki = (knobinfo_t *)closure;
  XhWindow    win = XhWindowOf(ABSTRZE(w));
  
    if (event->type == LeaveNotify)
    {
        active_fki = NULL;
        XhMakeTempMessage(win, "");
    }
    else
    {
        active_fki = fki;
        DisplayBarStats(win);
    }
}


static void ClickHandler(Widget     w  __attribute__((unused)),
                         XtPointer  closure,
                         XEvent    *event,
                         Boolean   *continue_to_dispatch)
{
  knobinfo_t          *fki    = (knobinfo_t *)closure;
  XButtonPressedEvent *ev     = (XButtonPressedEvent *) event;
  rflags_t             aflags = fki[KNOB_OFS_IALM].currflags
                              | fki[KNOB_OFS_UALM].currflags;
  
    if (ev->button == Button1)
    {
        *continue_to_dispatch = False;

        if ((aflags & CXCF_FLAG_ALARM_ALARM)         != 0     &&
            fki->uplink                              != NULL  &&
            fki->uplink->emlink                      != NULL  &&
            fki->uplink->emlink->AckAlarm            != NULL)
        {
            fki->uplink->emlink->AckAlarm(fki->uplink);
            alarm_acked = 1;
        }
    }
    else if (ev->button == Button3  &&  (ev->state & ControlMask) != 0)
    {
        *continue_to_dispatch = False;

        if (fki->uplink                              != NULL  &&
            fki->uplink->emlink                      != NULL  &&
            fki->uplink->emlink->ShowBigVal          != NULL)
            fki->uplink->emlink->ShowBigVal(fki);
    }
    else if (ev->button == Button3  &&  (ev->state & ShiftMask) != 0)
    {
        *continue_to_dispatch = False;

        if (fki->uplink                              != NULL  &&
            fki->uplink->emlink                      != NULL  &&
            fki->uplink->emlink->ToHistPlot          != NULL)
            fki->uplink->emlink->ToHistPlot(fki);
    }
}


static void ScaleExposureCB(Widget     w,
                            XtPointer  closure,
                            XtPointer  call_data __attribute__((unused)))
{
  int         is_i  = (ptr2lint(closure) == SCALE_I);
    
  Display    *dpy   = XtDisplay(w);
  Window      win   = XtWindow (w);

  Dimension   width;

  double      maxv;
  double      factor;
  double      step;
  
  double      v;
  double      t;
  int         i;
  int         y;

  char        buf[100];
  
    XtVaGetValues(w, XmNwidth, &width, NULL);

    XFillRectangle(dpy, win, bkgdGC[alarm_flipflop],
                   0, 0, width, COLHEIGHT);
    
    if (is_i)
    {
        maxv   = MAX_I;
        factor = I_FACTOR;
        step   = I_STEP;
    }
    else
    {
        maxv   = MAX_U;
        factor = U_FACTOR;
        step   = U_STEP;
    }
    
    XDrawLine(dpy, win, axis_GC, width-1, 0, width-1, COLHEIGHT-1);

    if (is_logarithmic)
    {
        for (v = 0;  v <= maxv; /*NOP*/)
        {
            y = is_i? I2y(v) : U2y(v);
            XDrawLine(dpy, win, axis_GC, width - TICK_LEN, y, width - 1, y);
            
            sprintf(buf, "%.0f", v);
            XDrawString(dpy, win, axis_GC,
                        width - TICK_LEN - XTextWidth(last_finfo, buf, strlen(buf)),
                        y,
                        buf, strlen(buf));

            if (v > 0)
                for (t = v; t < v * I_FACTOR;  t += v)
                {
                    y = I2y(t);
                    XDrawLine(dpy, win, axis_GC, width - TICK_LEN/2, y, width - 1, y);
                }
            
            if (v == 0) v  = 1;
            else        v *= factor;
        }
    }
    else
    {
        for (v = 0;  v <= maxv;  v += step)
        {
            y = is_i? I2y(v) : U2y(v);
            XDrawLine(dpy, win, axis_GC, width - TICK_LEN, y, width - 1, y);
            
            sprintf(buf, "%.0f", v);
            XDrawString(dpy, win, axis_GC,
                        width - TICK_LEN - XTextWidth(last_finfo, buf, strlen(buf)),
                        y,
                        buf, strlen(buf));
        }
    }
}

static void DecorateGrid(XhWidget grid, int col)
{
  XhWidget     l;
  XmString     s;
  char         buf[100];

    if (grid == NULL) return;

    l = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                XmNlabelString, s = XmStringCreateLtoR("I, µA", xh_charset),
                                NULL);
    XmStringFree(s);
    XhGridSetChildPosition (l,  0,  1);
    XhGridSetChildFilling  (l,  0,  0);
    XhGridSetChildAlignment(l, +1, -1);


    sprintf(buf, "%.0f", (double)MAX_I);
    i_scale = XtVaCreateManagedWidget("histscale", xmDrawingAreaWidgetClass, grid,
                                      XmNheight,      COLHEIGHT,
                                      XmNwidth,       TICK_LEN + XTextWidth(last_finfo, buf, strlen(buf)),
                                      XmNbackground,  XhGetColor(XH_COLOR_GRAPH_BG),
                                      XmNtraversalOn, False,
                                      NULL);
    XtAddCallback(i_scale, XmNexposeCallback, ScaleExposureCB, lint2ptr(SCALE_I));
    XhGridSetChildPosition (i_scale,  0,  0);
    XhGridSetChildFilling  (i_scale,  0,  0);
    XhGridSetChildAlignment(i_scale, +1, -1);

#if 0
    l = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                XmNlabelString, s = XmStringCreateLtoR("U, kV", xh_charset),
                                NULL);
    XmStringFree(s);
    XhGridSetChildPosition (l,  col+1,  1);
    XhGridSetChildFilling  (l,  0,  0);
    XhGridSetChildAlignment(l, -1, -1);

    sprintf(buf, "%.0f", (double)MAX_U);
    u_scale = XtVaCreateManagedWidget("histscale", xmDrawingAreaWidgetClass, grid,
                                      XmNheight,      COLHEIGHT,
                                      XmNwidth,       TICK_LEN + XTextWidth(last_finfo, buf, strlen(buf)),
                                      XmNbackground,  XhGetColor(XH_COLOR_GRAPH_BG),
                                      XmNtraversalOn, False,
                                      NULL);
    XtAddCallback(u_scale, XmNexposeCallback, ScaleExposureCB, lint2ptr(SCALE_U));
    XhGridSetChildPosition (u_scale,  col+1,  0);
    XhGridSetChildFilling  (u_scale,  0,      0);
    XhGridSetChildAlignment(u_scale, -1,     -1);
#endif
    
    XhGridSetSize(grid, col + 1, 2);
}

static void MakeHistogram(Widget parent_form, groupelem_t *grouplist, int is_vertical)
{
  int          bar_n;
    
  groupelem_t *gu;
  XhWidget     grid = NULL;
  int          col;

  int          n;
  int          x;
  knobinfo_t  *fki;
  
  char         lbuf[1000];
  char         tbuf[1000];
  char        *caption;
  char        *tip;

  XhWidget     da;
  XhWidget     l;
  XmString     s;

    /* I. Count bars */
    for (numbars = 0, gu = grouplist; gu->ei != NULL;  gu++)
    {
        if ((gu->ei->ncols &0x0000FFFF) != KNOBS_PER_COLUMN) continue;
        numbars += gu->ei->count / KNOBS_PER_COLUMN;
    }

    bars = (void *)(XtCalloc(numbars, sizeof(bar_t)));

    /* II. Create bars */
    for (bar_n = 0, gu = grouplist; gu->ei != NULL;  gu++)
    {
        if ((gu->ei->ncols &0x0000FFFF) != KNOBS_PER_COLUMN) continue;

        if (grid == NULL  ||  gu->fromnewline)
        {
            DecorateGrid(grid, col);
            
            grid = XhCreateGrid(parent_form, "grid");
            XhGridSetGrid   (grid, 0, 0);
            XhGridSetSpacing(grid, 0, 1);
            XhGridSetPadding(grid, 0, 0);
            col = 0;
        }

        for (x = 0, n = 0;
                    n < gu->ei->count - (KNOBS_PER_COLUMN - 1);
             x++,   n += KNOBS_PER_COLUMN,                      col++, bar_n++)
        {
            fki = gu->ei->content + n;
            
            get_knob_rowcol_label_and_tip(gu->ei->rownames, x, fki,
                                          lbuf, sizeof(lbuf), tbuf, sizeof(tbuf),
                                          &caption, &tip);
            if (caption == NULL  ||  caption[0] == '\0') caption = " ";
            
            da = XtVaCreateManagedWidget("histbar", xmDrawingAreaWidgetClass, grid,
                                         XmNheight,      COLHEIGHT,
                                         XmNbackground,  XhGetColor(XH_COLOR_GRAPH_BG),
                                         XmNtraversalOn, False,
                                         NULL);
            XhGridSetChildPosition (da, col + 1,  0);
            XhGridSetChildFilling  (da, 1,        0);
            XhGridSetChildAlignment(da, 0,       +1);
            XtAddCallback(da, XmNexposeCallback, BarExposureCB, (XtPointer)fki);

            l = XtVaCreateManagedWidget("histlabel", xmLabelWidgetClass, grid,
                                        XmNlabelString,        s = XmStringCreateLtoR(caption, xh_charset),
                                        XmNmarginWidth,        2,
                                        XmNmarginHeight,       0,
                                        XmNmarginLeft,         0,
                                        XmNmarginRight,        0,
                                        XmNmarginTop,          0,
                                        XmNmarginBottom,       0,
                                        XmNshadowThickness,    0,
                                        XmNhighlightThickness, 0,
                                        NULL);
            XmStringFree(s);
            XhAssignVertLabel(l, NULL, +1);
            
            if (tip != NULL  &&  tip[0] != '\0')
            {
                if (*tip == KNOB_LABELTIP_NOLABELTIP_PFX) tip++;
                XhSetBalloon(l, tip);
            }
            
            XhGridSetChildPosition (l, col + 1,  1);
            XhGridSetChildFilling  (l, 0,        0);
            XhGridSetChildAlignment(l, 0,       -1);

            bars[bar_n].da = da;
            bars[bar_n].ki = fki;

            XtAddEventHandler(da, EnterWindowMask, False, PointerHandler, fki);
            XtAddEventHandler(da, LeaveWindowMask, False, PointerHandler, fki);
            XtAddEventHandler(da, ButtonPressMask, False, ClickHandler,   fki);

            /* Assign the same event handlers to a label */
            XtAddEventHandler(l,  EnterWindowMask, False, PointerHandler, fki);
            XtAddEventHandler(l,  LeaveWindowMask, False, PointerHandler, fki);
            XtAddEventHandler(l,  ButtonPressMask, False, ClickHandler,   fki);
        }
    }
    
    DecorateGrid(grid, col);
}

static void NewDataArrived(XhWindow window, void *privptr, int synthetic)
{
  groupelem_t *gu;
  int          new_alarm;
    
    /* Alarm display management */

    /* a. Find out new alarm status */
    for (new_alarm = 0, gu = vac_grouplist;  gu->ei != NULL;  gu++)
        if (gu->ei->currflags & CXCF_FLAG_ALARM_ALARM) new_alarm = 1;
  
    /* b. Make decisions */
    if (new_alarm  > cur_alarm) alarm_acked = 0;
    if (new_alarm == 0  ||  alarm_acked)
        alarm_flipflop = 0;
    else
        alarm_flipflop = ! alarm_flipflop;

    if (cur_alarm  ||  new_alarm) ScaleExposureCB(i_scale, (XtPointer)SCALE_I, NULL);
    cur_alarm = new_alarm;
    
    /* Do display */
    DisplayAllBars();
    DisplayBarStats(window);
}

void CreateMeat(XhWindow  win, int argc, char *argv[])
{
  const char     *srvspec;

    ChlSetSysname(win, app_name);
    
    srvspec = app_defserver;
    if (argc > 1  &&  argv[1] != NULL  &&  argv[1][0] != '\0')
        srvspec = argv[1];

    if (ChlSetServer(win, srvspec) < 0)
    {
        fprintf(stderr, "%s: %s: %s(%s): %s\n",
                strcurtime(), argv[0], __FUNCTION__, srvspec, cx_strerror(errno));
        exit(1);
    }
    ChlSetPhysInfo  (win, app_phys_info, app_phys_info_count);

    /****/
    vac_grouplist = CdrCvtGroupunits2Grouplist(ChlGetMainsid(win), app_grouping);
    if (vac_grouplist == 0)
    {
        fprintf(stderr, "%s: %s: CdrCvtGroupunits2Grouplist(): %s",
                strcurtime(), argv[0], CdrLastErr());
        exit(1);
    }
    ChlSetGrouplist(win, vac_grouplist);

    CreateGCs(XhGetWindowWorkspace(win));
    
    numeric_view = XtVaCreateWidget("container",
                                    xmFormWidgetClass,
                                    XhGetWindowWorkspace(win),
                                    XmNshadowThickness, 0,
                                    NULL);
    graphic_view = XtVaCreateWidget("container",
                                    xmFormWidgetClass,
                                    XhGetWindowWorkspace(win),
                                    XmNshadowThickness, 0,
                                    NULL);
    XtManageChild(displayed_part == 0? numeric_view : graphic_view);
    XhSetCommandOnOff(win, cmSwitchView,  displayed_part != 0);
    XhSetCommandOnOff(win, cmSwitchScale, is_logarithmic != 0);

    ChlLayOutGrouping(numeric_view, vac_grouplist, 0, -1, -1);
    MakeHistogram    (graphic_view, vac_grouplist, 0);

    ChlSetDataArrivedCallback(win, NewDataArrived, NULL);
    /****/
    
    ChlAddLeds  (win);
    ChlStart    (win);
    XhShowWindow(win);
}

void SetDisplayedPart (XhWindow  win, int part)
{
    if      (part == 0) displayed_part = 0;
    else if (part >  0) displayed_part = 1;
    else                displayed_part = !displayed_part;

    if (displayed_part == 0)
        XtChangeManagedSet(&graphic_view, 1, NULL, NULL, &numeric_view, 1);
    else
        XtChangeManagedSet(&numeric_view, 1, NULL, NULL, &graphic_view, 1);
    
    XhSetCommandOnOff(win, cmSwitchView,  displayed_part != 0);
}

void SetDisplayedScale(XhWindow  win, int is_log)
{
    if      (is_log == 0) is_logarithmic = 0;
    else if (is_log >  0) is_logarithmic = 1;
    else                  is_logarithmic = !is_logarithmic;
    
    DisplayAllBars();
    if (i_scale) XClearArea(XtDisplay(i_scale), XtWindow(i_scale), 0, 0, 0, 0, True);
    if (u_scale) XClearArea(XtDisplay(u_scale), XtWindow(u_scale), 0, 0, 0, 0, True);
    
    XhSetCommandOnOff(win, cmSwitchScale, is_logarithmic != 0);
}
