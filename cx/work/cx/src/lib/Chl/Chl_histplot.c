#include "Chl_includes.h"

#include "timeval_utils.h"

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/ScrollBar.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#include "Xh_fallbacks.h"

#include "KnobsI.h" // For SetTextString()

#ifndef NAN
  #warning The NAN macro is undefined, using strtod("NAN") instead
  #define NAN strtod("NAN", NULL)
#endif

#include "pixmaps/btn_mini_save.xpm"


enum
{
    PLOTMODE_LINE = 0,
    PLOTMODE_WIDE = 1,
    PLOTMODE_DOTS = 2,
    PLOTMODE_BLOT = 3,
};


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

static GC AllocThGC(Widget w, int idx, const char *fontspec)
{
  XtGCMask   mask;  
  XGCValues  vals;
    
    mask = GCForeground | GCLineWidth;
    vals.foreground = XhGetColor(idx);
    vals.line_width = 2;

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

//////////////////////////////////////////////////////////////////////

enum
{
    H_TICK_LEN      = 5,
    V_TICK_LEN      = 5,
    H_TICK_SPC      = 1,
    V_TICK_SPC      = 0,
    H_TICK_MRG      = 2,
    V_TICK_MRG      = 1,
    GRID_DASHLENGTH = 6,

    DEF_PLOT_W = 600,
    DEF_PLOT_H = 400,

    H_TICK_STEP = 30,
    V_TICK_SEGS = 8,
    V_TICK_STEP = 50,
    
    DEF_RINGSIZE = 86400,
};

static int x_scale_factors[] = {1, 10, 60};


typedef histplotdlg_t dlgrec_t;
static dlgrec_t *ThisDialog(XhWindow win);


static int ShouldUseLog(Knob ki)
{
  const char *p = ki->dpyfmt + strlen(ki->dpyfmt) - 1;

    return
      tolower(*p) == 'e'                     &&
      ki->rmins[KNOBS_RANGE_DISP] >= 1e-100  &&
      ki->rmaxs[KNOBS_RANGE_DISP] >= 1e-100;
}

typedef void (*do_flush_t)(Display *dpy, Window win, GC gc,
                           XPoint *points, int *npts_p);

static void FlushXLines (Display *dpy, Window win, GC gc,
                         XPoint *points, int *npts_p)
{
    if (*npts_p == 0) return;
    if (*npts_p == 1) XDrawPoint(dpy, win, gc, points[0].x, points[0].y);
    else              XDrawLines(dpy, win, gc, points, *npts_p, CoordModeOrigin);

    points[0] = points[(*npts_p) - 1];
    *npts_p = 1;
}

static void FlushXPoints(Display *dpy, Window win, GC gc,
                         XPoint *points, int *npts_p)
{
    if (*npts_p == 0) return;
    XDrawPoints(dpy, win, gc, points, *npts_p, CoordModeOrigin);

    points[0] = points[(*npts_p) - 1];
    *npts_p = 1;
}

static void FlushXBlots(Display *dpy, Window win, GC gc,
                        XPoint *points, int *npts_p)
{
  int     n;
  XPoint  pts[5];
    
    if (*npts_p == 0) return;
    pts[1].x = +1; pts[1].y = +0;
    pts[2].x = -1; pts[2].y = +1;
    pts[3].x = -1; pts[3].y = -1;
    pts[4].x = +1; pts[4].y = -1;
    for (n = 0;  n < *npts_p;  n++)
    {
        pts[0].x = points[n].x;
        pts[0].y = points[n].y;
        XDrawLines(dpy, win, gc, pts, countof(pts), CoordModePrevious);
    }

    points[0] = points[(*npts_p) - 1];
    *npts_p = 1;
}

static void DrawGraph(histplotdlg_t *rec, int do_clear)
{
  Display    *dpy = XtDisplay(rec->graph);
  Window      win = XtWindow (rec->graph);
  Dimension   grf_w;
  Dimension   grf_h;

  int         row;
  Knob        ki;
  int         is_logarithmic;
  double      mindisp;
  double      maxdisp;

  int         dash_ofs;
  int         v_tick_segs;
  int         i;
  int         limit;
  int         t;
  double      v;
  int         x;
  int         y;
  int         x_mul;
  int         x_div;

  enum       {PTSBUFSIZE = 1000};
  XPoint      points[PTSBUFSIZE];
  int         npoints;
  do_flush_t  do_flush;
  int         is_wide;

    if (win == 0) return; // A guard for resize-called-before-realized
    XtVaGetValues(rec->graph, XmNwidth, &grf_w, XmNheight, &grf_h, NULL);
  
    if (do_clear)
        XFillRectangle(dpy, win, rec->bkgdGC, 0, 0, grf_w, grf_h);

    do_flush = (rec->mode == PLOTMODE_DOTS)? FlushXPoints : FlushXLines;
    if (rec->mode == PLOTMODE_BLOT) do_flush = FlushXBlots;
    is_wide  = rec->mode == PLOTMODE_WIDE;

    // Grid -- horizontal lines
    v_tick_segs = ((grf_h - V_TICK_STEP*2) / (V_TICK_STEP*2)) * 2 + 2;
    dash_ofs = rec->horz_offset % (GRID_DASHLENGTH * 2);
    for (i = 2;  i <= v_tick_segs - 2;  i += 2)
    {
        y = grf_h-1 - RESCALE_VALUE(i, 0, v_tick_segs, 0, grf_h-1);
        y = RESCALE_VALUE(i, 0, v_tick_segs, grf_h-1, 0);
        
        XDrawLine(dpy, win, rec->gridGC,
                  grf_w-1+dash_ofs, y, 0, y);
    }
    
    // Grid -- vertical lines
    for (t = (H_TICK_STEP*2) - rec->horz_offset % (H_TICK_STEP*2);
         t < grf_w;
         t += H_TICK_STEP*2)
    {
        x = grf_w - 1 - t;
        XDrawLine(dpy, win, rec->gridGC,
                  x, 0, x, grf_h-1);
    }
    
    for (row = rec->rows_used - 1;  row >= 0;  row--)
    {
        if (!rec->show[row]) continue;
        
        ki = rec->target[row];

        x_mul = ki->histring_frqdiv;   if (x_mul <= 0) x_mul = 1;
        x_div = rec->x_scale;

        limit = (grf_w + rec->horz_offset) * rec->x_scale;
        if (limit > ki->histring_used)
            limit = ki->histring_used;

        mindisp = ki->rmins[KNOBS_RANGE_DISP];
        maxdisp = ki->rmaxs[KNOBS_RANGE_DISP];
        is_logarithmic = ShouldUseLog(ki);
        if (is_logarithmic)
        {
            ////fprintf(stderr, "LOG(%s)!\n", ki->ident);
            mindisp = log(mindisp);
            maxdisp = log(maxdisp);
        }
        
        for (t = rec->horz_offset * rec->x_scale, npoints = 0;
             t < limit;
             t++)
        {
            v = ki->histring[
                             (
                              ki->histring_start
                              +
                              (ki->histring_used - 1 - t)
                             )
                             %
                             ki->histring_size
                            ];
            if (is_logarithmic)
            {
                //v = ki->rmaxs[KNOBS_RANGE_DISP] / 100 * t;
                if (v <= 1e-100) v = log(1e-100);
                else             v = log(v);

                y = RESCALE_VALUE(v,
                                  mindisp, maxdisp,
                                  grf_h - 1, 0);
            }
            else
            {
                y = RESCALE_VALUE(v,
                                  mindisp, maxdisp,
                                  grf_h - 1, 0);
            }
            if      (y < -32767) y = -32767;
            else if (y > +32767) y = +32767;
            x = grf_w - 1 - ((t * x_mul) / x_div) + rec->horz_offset;

            if (npoints == PTSBUFSIZE) do_flush(dpy, win, rec->chanGC[is_wide][row % XH_NUM_DISTINCT_LINE_COLORS],
                                                points, &npoints);
            points[npoints].x = x;
            points[npoints].y = y;
            npoints++;

            /*
             This is  for frqdiv>1; this check is performed AFTER adding
             the point for a line to be drawn towards it from previous point.
             */
            if (x < 0) break; 
        }
        
        do_flush(dpy, win, rec->chanGC[is_wide][row % XH_NUM_DISTINCT_LINE_COLORS],
                 points, &npoints);
    }
}

static void DrawAxis(histplotdlg_t *rec, int do_clear)
{
  Display   *dpy = XtDisplay(rec->axis);
  Window     win = XtWindow (rec->axis);
  Dimension  cur_w;
  Dimension  cur_h;
  int        grf_w;
  int        grf_h;

  typedef enum
  {
      VLPLACE_BOTH    = 0,
      VLPLACE_EVENODD = 1,
      VLPLACE_HALVES  = 2
  } vlplace_t; // "vl" -- Vertical axis Legend
  vlplace_t  vlplace;
  int        lr;
  int        num_lr[2];
  int        h_lr  [2];
  int        y_lr  [2];

  int        cyclesize;

  int        even;
  int        t;
  int        secs;
  int        v_tick_segs;
  int        i;
  int        x;
  int        y;
  int        h;
  int        len;
  char       buf[1000];
  int        frsize;

  int        row;
  int        num_on;
  int        num_used;
  Knob       ki;
  double     v;
  char      *p;
  int        twid;

    if (win == 0) return; // A guard for resize-called-before-realized
    XtVaGetValues(rec->axis, XmNwidth, &cur_w, XmNheight, &cur_h, NULL);

    cyclesize = cda_cyclesize(ChlGetMainsid(XhWindowOf(rec->axis)));
    if (cyclesize <= 0) cyclesize = 1000000;
    if (cyclesize != rec->prev_cyclesize) do_clear = 1;
    rec->prev_cyclesize = cyclesize;

    if (do_clear)
        XFillRectangle(dpy, win, rec->bkgdGC, 0, 0, cur_w, cur_h);

    grf_w = cur_w - rec->m_lft - rec->m_rgt;
    grf_h = cur_h - rec->m_top - rec->m_bot;

    if (grf_w <= 0  ||  grf_h <= 0) return;

    XDrawRectangle(dpy, win, rec->axisGC,
                   rec->m_lft - 1, rec->m_top - 1,
                   grf_w + 1,      grf_h + 1);

    for (t = rec->horz_offset + H_TICK_STEP - 1, t -= t % H_TICK_STEP;
         t < rec->horz_offset + grf_w;
         t += H_TICK_STEP)
    {
        x = cur_w - rec->m_rgt - 1 - t + rec->horz_offset;
        even = (t % (H_TICK_STEP*2)) == 0;
        if (even) len = V_TICK_LEN;
        else      len = V_TICK_LEN / 2 + 1;

        XDrawLine(dpy, win, rec->axisGC,
                  x, rec->m_top - len,
                  x, rec->m_top - 1);
        XDrawLine(dpy, win, rec->axisGC,
                  x, rec->m_top + grf_h,
                  x, rec->m_top + grf_h + len - 1);

        if (even)
        {
            secs = (((double)t) * cyclesize / 1000000) * rec->x_scale;
            
            if      (secs < 59)
                sprintf(buf,            "%d",                               -secs);
            else if (secs < 3600)
                sprintf(buf,      "-%d:%02d",               secs / 60,       secs % 60);
            else
                sprintf(buf, "-%d:%02d:%02d", secs / 3600, (secs / 60) % 60, secs % 60);

            XDrawString(dpy, win, rec->axisGC,
                        x - XTextWidth(rec->axis_finfo, buf, strlen(buf)) / 2,
                        rec->m_top - V_TICK_LEN - V_TICK_SPC - rec->axis_finfo->descent,
                        buf, strlen(buf));

            XDrawString(dpy, win, rec->axisGC,
                        x - XTextWidth(rec->axis_finfo, buf, strlen(buf)) / 2,
                        rec->m_top + grf_h +
                            V_TICK_LEN + V_TICK_SPC + rec->axis_finfo->ascent,
                        buf, strlen(buf));
        }
    }

    /* Count visible rows */
    for (row = 0, num_on = 0;  row < rec->rows_used;  row++)
        num_on += rec->show[row];

    /* VL-place initialization */
    vlplace = VLPLACE_BOTH;
    num_lr[0] = num_lr[1] = num_on;
    if (num_on > 6)
    {
        vlplace = 0? VLPLACE_EVENODD : VLPLACE_HALVES;
        num_lr[1] = num_on / 2;  // With this order more rows go to left
        num_lr[0] = num_on - num_lr[1];
    }

    v_tick_segs = ((grf_h - V_TICK_STEP*2) / (V_TICK_STEP*2)) * 2 + 2;
    for (i = 0;  i <= v_tick_segs;  i++)
    {
        h = RESCALE_VALUE(i, 0, v_tick_segs, 0, grf_h-1);
        y = rec->m_top + grf_h - 1 - h;
        even = ((i % 2) == 0);
        if (even) len = H_TICK_LEN;
        else      len = H_TICK_LEN / 2 + 1;

        XDrawLine(dpy, win, rec->axisGC,
                  rec->m_lft - len, y,
                  rec->m_lft - 1,   y);
        XDrawLine(dpy, win, rec->axisGC,
                  rec->m_lft + grf_w,           y,
                  rec->m_lft + grf_w + len - 1, y);

        if (even)
        {
            for (lr = 0;  lr <= 1;  lr++)
            {
                h_lr[lr] = (rec->chan_finfo[0]->ascent + rec->chan_finfo[0]->descent)
                    * num_lr[lr];
                y_lr[lr] = y - h_lr[lr] /2;
                if (y_lr[lr] > cur_h - rec->m_bot - h_lr[lr])
                    y_lr[lr] = cur_h - rec->m_bot - h_lr[lr];
                if (y_lr[lr] < rec->m_top)
                    y_lr[lr] = rec->m_top;
            }

            for (row = 0, num_used = 0;  row < rec->rows_used;  row++)
            {
                if (!rec->show[row]) continue;

                ki = rec->target[row];

                if (ShouldUseLog(ki))
                    v = exp(
                            RESCALE_VALUE(i,
                                          0,                           v_tick_segs,
                                          log(ki->rmins[KNOBS_RANGE_DISP]), log(ki->rmaxs[KNOBS_RANGE_DISP]))
                           );
                                          
                else
                    v = RESCALE_VALUE(i,
                                      0,                           v_tick_segs,
                                      ki->rmins[KNOBS_RANGE_DISP], ki->rmaxs[KNOBS_RANGE_DISP]);

                p = snprintf_dbl_trim(buf, sizeof(buf), ki->dpyfmt, v, 1);
                frsize = sizeof(buf)-1 - (p - buf) - strlen(p);
                if (frsize > 0  &&  ki->units != NULL)
                    snprintf(p + strlen(p), frsize, "%s", ki->units);
                twid = XTextWidth(rec->chan_finfo[row % XH_NUM_DISTINCT_LINE_COLORS], p, strlen(p));

                for (lr = 0;  lr <= 1;  lr++)
                    if (vlplace == VLPLACE_BOTH
                        ||
                        (
                         vlplace == VLPLACE_EVENODD  &&
                         (num_used & 1) == (lr & 1)
                        )
                        ||
                        (
                         vlplace == VLPLACE_HALVES  &&
                         (
                          (lr == 0  &&  num_used <  num_lr[0])  ||
                          (lr == 1  &&  num_used >= num_lr[0])
                         )
                        )
                       )
                    {
                        if (lr == 0) x = rec->m_lft         - H_TICK_LEN - H_TICK_SPC - twid;
                        else         x = rec->m_lft + grf_w + H_TICK_LEN + H_TICK_SPC;

                        XDrawString(dpy, win, rec->chanGC[0][row % XH_NUM_DISTINCT_LINE_COLORS],
                                    x,
                                    y_lr[lr] + rec->chan_finfo[row % XH_NUM_DISTINCT_LINE_COLORS]->ascent,
                                    p, strlen(p));

                        y_lr[lr] += rec->chan_finfo[0]->ascent + rec->chan_finfo[0]->descent;
                    }

                num_used++;
            }
        }
    }
}

static void GraphExposureCB(Widget     w,
                            XtPointer  closure,
                            XtPointer  call_data)
{
  histplotdlg_t *rec = (histplotdlg_t *) closure;

    if (rec->ignore_graph_expose) return;
    XhRemoveExposeEvents(ABSTRZE(w));
    DrawGraph(rec, False);
}

static void AxisExposureCB(Widget     w,
                           XtPointer  closure,
                           XtPointer  call_data)
{
  histplotdlg_t *rec = (histplotdlg_t *) closure;

    if (rec->ignore_axis_expose) return;
    XhRemoveExposeEvents(ABSTRZE(w));
    DrawAxis(rec, False);
}

static void GraphResizeCB(Widget     w,
                          XtPointer  closure,
                          XtPointer  call_data)
{
  histplotdlg_t *rec = (histplotdlg_t *) closure;

    XhAdjustPreferredSizeInForm(ABSTRZE(w));
    if (XhCompressConfigureEvents(ABSTRZE(w)) == 0)
    {
        rec->ignore_graph_expose = 0;
        DrawGraph(rec, True);
    }
    else
        rec->ignore_graph_expose = 1;
}

static void AxisResizeCB(Widget     w,
                         XtPointer  closure,
                         XtPointer  call_data)
{
  histplotdlg_t *rec = (histplotdlg_t *) closure;

    XhAdjustPreferredSizeInForm(ABSTRZE(w));
    if (XhCompressConfigureEvents(ABSTRZE(w)) == 0)
    {
        rec->ignore_axis_expose = 0;
        DrawAxis(rec, True);
    }
    else
        rec->ignore_axis_expose = 1;
}

static int SetHorzbarParams(histplotdlg_t *rec)
{
  int  ret = 0;

  Dimension  v_span;
  int        n_maximum;
  int        n_slidersize;
  int        n_value;

    XtVaGetValues(rec->graph, XmNwidth, &v_span, NULL);

    n_maximum = rec->horz_maximum;
    if (n_maximum < v_span) n_maximum = v_span;

    n_slidersize = v_span;

    if (n_maximum    == rec->o_horzbar_maximum  &&
        n_slidersize == rec->o_horzbar_slidersize)
        return ret;
    rec->o_horzbar_maximum    = n_maximum;
    rec->o_horzbar_slidersize = n_slidersize;

    XtVaGetValues(rec->horzbar, XmNvalue, &n_value, NULL);
    if (n_value > n_maximum - n_slidersize)
    {
        rec->horz_offset = n_value = n_maximum - n_slidersize;
        ret = 1;
    }

    XtVaSetValues(rec->horzbar,
                  XmNminimum,       0,
                  XmNmaximum,       n_maximum,
                  XmNsliderSize,    n_slidersize,
                  XmNvalue,         n_value,
                  XmNincrement,     1,
                  XmNpageIncrement, /*n_maximum / 10*/v_span,
                  NULL);

    return ret;
}

static void HorzScrollCB(Widget     w,
                         XtPointer  closure,
                         XtPointer  call_data)
{
  histplotdlg_t *rec = closure;

  XmScrollBarCallbackStruct *info = (XmScrollBarCallbackStruct *)call_data;

   rec->horz_offset = info->value;
   DrawGraph(rec, True);
   DrawAxis (rec, True);
}

#define GOOD_ATTACHMENTS 1
static void SetAttachments(histplotdlg_t *rec)
{
#if GOOD_ATTACHMENTS
    XtVaSetValues(rec->grid,
                  XmNtopAttachment,     XmATTACH_FORM,
                  XmNrightAttachment,   XmATTACH_FORM,
                  XmNleftAttachment,    XmATTACH_NONE,
                  NULL);

    XtVaSetValues(rec->w_r,
                  XmNrightAttachment,   XmATTACH_WIDGET,
                  XmNrightWidget,       rec->grid,
                  XmNleftAttachment,    XmATTACH_NONE,
                  NULL);

    XtVaSetValues(rec->w_l,
                  XmNleftAttachment,    XmATTACH_FORM,
                  NULL);

    XtVaSetValues(rec->graph,
                  XmNleftAttachment,    XmATTACH_WIDGET,
                  XmNleftWidget,        rec->w_l,
                  XmNrightAttachment,   XmATTACH_WIDGET,
                  XmNrightWidget,       rec->w_r,
                  XmNtopAttachment,     XmATTACH_FORM,
                  XmNbottomAttachment,  XmATTACH_WIDGET,
                  XmNbottomWidget,      rec->horzbar,
                  NULL);

    XtVaSetValues(rec->axis,
                  XmNleftAttachment,    XmATTACH_OPPOSITE_WIDGET,
                  XmNleftWidget,        rec->w_l,
                  XmNrightAttachment,   XmATTACH_OPPOSITE_WIDGET,
                  XmNrightWidget,       rec->w_r,
                  XmNtopAttachment,     XmATTACH_FORM,
                  XmNbottomAttachment,  XmATTACH_WIDGET,
                  XmNbottomWidget,      rec->horzbar,
                  NULL);

    XtVaSetValues(rec->horzbar,
                  XmNleftAttachment,    XmATTACH_WIDGET,
                  XmNleftWidget,        rec->w_l,
                  XmNrightAttachment,   XmATTACH_WIDGET,
                  XmNrightWidget,       rec->w_r,
                  XmNbottomAttachment,  XmATTACH_FORM,
                  NULL);

#else
    XtVaSetValues(rec->grid,
                  XmNtopAttachment,     XmATTACH_FORM,
                  XmNrightAttachment,   XmATTACH_FORM,
                  NULL);

    XtVaSetValues(rec->axis,
                  XmNleftAttachment,    XmATTACH_FORM,
                  XmNrightAttachment,   XmATTACH_WIDGET,
                  XmNrightWidget,       rec->grid,
                  XmNtopAttachment,     XmATTACH_FORM,
                  XmNbottomAttachment,  XmATTACH_FORM,
                  NULL);

    XtVaSetValues(rec->graph,
                  XmNleftAttachment,    XmATTACH_FORM,
                  XmNrightAttachment,   XmATTACH_WIDGET,
                  XmNrightWidget,       rec->grid,
                  XmNtopAttachment,     XmATTACH_FORM,
                  XmNbottomAttachment,  XmATTACH_FORM,
                  NULL);
#endif
}

static void CalcSetMargins(histplotdlg_t *rec)
{
  int     row;
  int     side;
  int     curw;
  int     maxw;
  Knob    ki;
  double  v;
  char    buf[1000];
  int     frsize;
  char   *p;
  
    for (row = 0, maxw = 0;
         row < rec->rows_used;
         row++)
    {
        ki = rec->target[row];
        for (side = 0;  side <= 1;  side++)
        {
            v = ((side == 0)? ki->rmins : ki->rmaxs)[KNOBS_RANGE_DISP];

            p = snprintf_dbl_trim(buf, sizeof(buf), ki->dpyfmt, v, 1);
            frsize = sizeof(buf)-1 - strlen(buf);
            if (frsize > 0  &&  ki->units != NULL)
                snprintf(p + strlen(p), frsize, "%s", ki->units);

            curw = XTextWidth(rec->chan_finfo[row % XH_NUM_DISTINCT_LINE_COLORS], p, strlen(p));

            if (maxw < curw) maxw = curw;
        }
    }
    
    rec->m_lft = rec->m_rgt = 
        H_TICK_MRG +
        maxw +
        H_TICK_SPC +
        H_TICK_LEN;
    rec->m_top = rec->m_bot =
        V_TICK_MRG +
        rec->axis_finfo->ascent + rec->axis_finfo->descent +
        V_TICK_SPC +
        V_TICK_LEN;

#if GOOD_ATTACHMENTS
    XtVaSetValues(rec->w_l,
                  XmNwidth, rec->m_lft,
                  NULL);
    XtVaSetValues(rec->w_r,
                  XmNwidth, rec->m_rgt,
                  NULL);
    XtVaSetValues(rec->graph,
                  XmNtopOffset,    rec->m_top,
                  XmNbottomOffset, rec->m_bot,
                  NULL);
#else
    XtVaSetValues(rec->graph,
                  XmNleftOffset,   rec->m_lft,
                  XmNrightOffset,  rec->m_rgt,
                  XmNtopOffset,    rec->m_top,
                  XmNbottomOffset, rec->m_bot,
                  NULL);
#endif
}


static void RenewPlotRow(histplotdlg_t *rec, int  row, Knob  ki, int showness)
{
  char     *label;
  XmString  s;
  
  Boolean   is_set = showness? XmSET : XmUNSET;
  Boolean   cur_set;

    rec->target[row] = ki;
    rec->show  [row] = showness;

    label = ki->label;
    if (label == NULL) label = ki->ident;
    if (label == NULL  ||  label[0] == '\0') label = " ";
    XtVaSetValues(rec->label[row],
                  XmNlabelString, s = XmStringCreateLtoR(label, xh_charset),
                  NULL);
    XmStringFree(s);

    XtVaGetValues(rec->label[row], XmNset, &cur_set, NULL);
    if (cur_set != is_set)
        XtVaSetValues(rec->label[row], XmNset, is_set, NULL);

    XtVaSetValues(rec->val_dpy[row],
                  XmNcolumns, GetTextColumns(ki->dpyfmt, ki->units),
                  NULL);
    rec->colstate_s[row] = COLALARM_UNINITIALIZED;
}

static void UpdatePlotRow(histplotdlg_t *rec, int  row)
{
  Knob   ki = rec->target[row];
  Pixel  fg;
  Pixel  bg;
    
    SetTextString(ki, rec->val_dpy[row], ki->curv);

    if (ki->colstate != rec->colstate_s[row])
    {
        ChooseKnobColors(ki->color, ki->colstate,
                         rec->deffg, rec->defbg,
                         &fg, &bg);
        XtVaSetValues(rec->val_dpy[row],
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
        rec->colstate_s[row] = ki->colstate;
    }
}

static void SetPlotCount(histplotdlg_t *rec, int count)
{
  int  row;
    
    rec->rows_used = count;
    
    if (count > 0)
    {
        XtManageChildren    (rec->label,   count);
        XtManageChildren    (rec->val_dpy, count);
        if (!rec->fixed)
        {
            XtManageChildren(rec->b_up,    count);
            XtManageChildren(rec->b_dn,    count);
            XtManageChildren(rec->b_rm,    count);
        }
    }

    if (count < MAX_HISTPLOT_LINES_PER_BOX)
    {
        XtUnmanageChildren    (rec->label   + count, MAX_HISTPLOT_LINES_PER_BOX - count);
        XtUnmanageChildren    (rec->val_dpy + count, MAX_HISTPLOT_LINES_PER_BOX - count);
        if (!rec->fixed)
        {
            XtUnmanageChildren(rec->b_up    + count, MAX_HISTPLOT_LINES_PER_BOX - count);
            XtUnmanageChildren(rec->b_dn    + count, MAX_HISTPLOT_LINES_PER_BOX - count);
            XtUnmanageChildren(rec->b_rm    + count, MAX_HISTPLOT_LINES_PER_BOX - count);
        }
    }
    
    for (row = 0;  row < count;  row++)
    {
        XhGridSetChildPosition    (rec->label  [row], 0, row);
        XhGridSetChildPosition    (rec->val_dpy[row], 1, row);
        if (!rec->fixed)
        {
            XhGridSetChildPosition(rec->b_up   [row], 2, row);
            XhGridSetChildPosition(rec->b_dn   [row], 3, row);
            XhGridSetChildPosition(rec->b_rm   [row], 4, row);
        }
    }

    XhGridSetSize(rec->grid, rec->fixed? 2 : 5, count);
}

static int CalcPlotParams(histplotdlg_t *rec)
{
  int   row;
  int   maxpts;
  Knob  ki;

    for (row = 0, maxpts = 0;  row < rec->rows_used;  row++)
    {
        ki = rec->target[row];
        if (maxpts < ki->histring_used)
            maxpts = ki->histring_used;
    }
    
    rec->horz_maximum = maxpts / rec->x_scale;

    return SetHorzbarParams(rec);
}

static void DisplayPlot(histplotdlg_t *rec)
{
  int   row;
  
    for (row = 0;  row < rec->rows_used;  row++)
        UpdatePlotRow(rec, row);

    CalcPlotParams(rec);
    
    DrawGraph(rec, True);
}

static void ShowOnOffCB(Widget     w,
                        XtPointer  closure,
                        XtPointer  call_data)
{
  histplotdlg_t *rec = ThisDialog(XhWindowOf(w));
  int            row = ptr2lint(closure);
  XmToggleButtonCallbackStruct *info = (XmToggleButtonCallbackStruct *) call_data;
  
    rec->show[row] = (info->set == XmUNSET? 0 : 1);
    
    DrawGraph     (rec, True);
    DrawAxis      (rec, True);
}

static void PlotUpCB(Widget     w,
                     XtPointer  closure,
                     XtPointer  call_data  __attribute__((unused)))
{
  histplotdlg_t *rec = ThisDialog(XhWindowOf(w));
  int            row = ptr2lint(closure);
  Knob           k_this;
  Knob           k_prev;
  int            s_this;
  int            s_prev;
  
    if (row == 0) return;
    k_this = rec->target[row];      s_this = rec->show[row];
    k_prev = rec->target[row - 1];  s_prev = rec->show[row - 1];
    RenewPlotRow (rec, row,     k_prev, s_prev);
    RenewPlotRow (rec, row - 1, k_this, s_this);
    UpdatePlotRow(rec, row);
    UpdatePlotRow(rec, row - 1);

    DrawGraph     (rec, True);
    DrawAxis      (rec, True);
}

static void PlotDnCB(Widget     w,
                     XtPointer  closure,
                     XtPointer  call_data  __attribute__((unused)))
{
  histplotdlg_t *rec = ThisDialog(XhWindowOf(w));
  int            row = ptr2lint(closure);
  Knob           k_this;
  Knob           k_next;
  int            s_this;
  int            s_next;

    if (row == rec->rows_used - 1) return;
    k_this = rec->target[row];      s_this = rec->show[row];
    k_next = rec->target[row + 1];  s_next = rec->show[row + 1];
    RenewPlotRow (rec, row,     k_next, s_next);
    RenewPlotRow (rec, row + 1, k_this, s_this);
    UpdatePlotRow(rec, row);
    UpdatePlotRow(rec, row + 1);

    DrawGraph     (rec, True);
    DrawAxis      (rec, True);
}

static void PlotRmCB(Widget     w,
                     XtPointer  closure,
                     XtPointer  call_data  __attribute__((unused)))
{
  histplotdlg_t *rec = ThisDialog(XhWindowOf(w));
  int            row = ptr2lint(closure);
  int            y;
  
    if (rec->rows_used == 1  &&  rec->box != NULL) XhStdDlgHide(rec->box);

    for (y = row;  y < rec->rows_used - 1;  y++)
    {
        RenewPlotRow (rec, y, rec->target[y + 1], rec->show[y + 1]);
        UpdatePlotRow(rec, y);
    }
    SetPlotCount(rec, rec->rows_used - 1);

    CalcSetMargins(rec);
    DrawGraph     (rec, True);
    DrawAxis      (rec, True);
}

static void PlotMouse3Handler(Widget     w,
                              XtPointer  closure,
                              XEvent    *event,
                              Boolean   *continue_to_dispatch)
{
  histplotdlg_t       *rec = ThisDialog(XhWindowOf(w));
  int                  row = ptr2lint(closure);
  Knob                 ki  = rec->target[row];

    KnobsMouse3Handler(w, (XtPointer)ki, event, continue_to_dispatch);
}

static XhWidget MakePlotButton(XhWidget grid, int x, int row,
                               const char *caption, XtCallbackProc  callback)
{
  XhWidget  b;
  XmString  s;
  
    b = XtVaCreateWidget("push_o", xmPushButtonWidgetClass, CNCRTZE(grid),
                         XmNtraversalOn, False,
                         XmNsensitive,   1,
                         XmNlabelString, s = XmStringCreateLtoR(caption, xh_charset),
                         NULL);
    XhGridSetChildPosition (b,  x,  row);
    XhGridSetChildFilling  (b,  0,  0);
    XhGridSetChildAlignment(b, -1, -1);
    XtAddCallback(b, XmNactivateCallback, callback, lint2ptr(row));

    return b;
}

static void XScaleKCB(Knob k, double v, void *privptr)
{
  histplotdlg_t                *rec  = privptr;

    rec->x_scale = x_scale_factors[(int)(round(v))];
    CalcPlotParams(rec);
    DrawGraph(rec, True);
    DrawAxis (rec, True);
}

static void ModeKCB(Knob k, double v, void *privptr)
{
  histplotdlg_t                *rec  = privptr;

    rec->mode = (int)(round(v));
    DrawGraph(rec, True);
}

static void SavePlot(XhWindow win, const char *filename, const char *comment)
{
  histplotdlg_t  *rec = &(Priv2Of(win)->hp);

  FILE           *fp;
  struct timeval  timenow;
  int             cyclesize;
  struct timeval  timestart;
  struct timeval  timeoft;
  struct timeval  tv_x, tv_y, tv_r;

  int             row;
  int             t;
  Knob            ki;
  double          v;
  
  int             max_used;

    if ((fp = fopen(filename, "w")) == NULL) return;

    /* Calculate statistics and time values */
    for (row = 0, max_used = 0;  row < rec->rows_used;  row++)
        if (max_used < rec->target[row]->histring_used)
            max_used = rec->target[row]->histring_used;

    gettimeofday(&timenow, NULL);
    cyclesize = cda_cyclesize(ChlGetMainsid(XhWindowOf(rec->axis)));
    if (cyclesize <= 0) cyclesize = 1000000;

    /* Title line */
    fprintf(fp, "#Time(s-01.01.1970) YYYY-MM-DD-HH:MM:SS.mss secs-since0");
    for (row = 0;  row < rec->rows_used;  row++)
    {
        ki = rec->target[row];
        fprintf(fp, " %s",
                ki->ident != NULL  &&  ki->ident[0] != '\0'? ki->ident : "UNNAMED");
        if (ki->units != NULL  &&  ki->units[0] != '\0')
            fprintf(fp, "(%s)", ki->units);
    }
    fprintf(fp, "\n");

    for (t = max_used - 1;  t >= 0;  t--)
    {
        /* Calculate timeval of t */
        tv_x = timenow;
        tv_y.tv_sec  =      ((double)t) * cyclesize / 1000000;
        tv_y.tv_usec = fmod(((double)t) * cyclesize , 1000000);
        timeval_subtract(&timeoft, &tv_x, &tv_y);

        /* Store it as start, if needed */
        if (t == max_used - 1) timestart = timeoft;

        /* Calculate time since start */
        tv_x = timeoft;
        tv_y = timestart;
        timeval_subtract(&tv_r, &tv_x, &tv_y);
        
        fprintf(fp, "%lu.%03d %s.%03d %7lu.%03d",
                (long)    timeoft.tv_sec,       (int)(timeoft.tv_usec / 1000),
                stroftime(timeoft.tv_sec, "-"), (int)(timeoft.tv_usec / 1000),
                (long)    tv_r.tv_sec,          (int)(tv_r.tv_usec    / 1000));

        for (row = 0;  row < rec->rows_used;  row++)
        {
            ki = rec->target[row];
            if (ki->histring_used > t)
                v = ki->histring[
                                 (ki->histring_start + (ki->histring_used - 1 - t))
                                 %
                                 ki->histring_size
                                ];
            else
                v = NAN;

            fprintf(fp, " ");
            fprintf(fp, ki->dpyfmt, v);
        }

        fprintf(fp, "\n");
    }
    
    fclose(fp);
    
    XhMakeMessage (win, "History-plot saved to \"%s\".", filename);
    ChlRecordEvent(win, "History-plot saved to \"%s\".", filename);
}

static void SaveCB(Widget     w,
                   XtPointer  closure    __attribute__((unused)),
                   XtPointer  call_data  __attribute__((unused)))
{
  XhWindow             win = XhWindowOf(w);
  histplotdlg_t       *rec = ThisDialog(win);
  const char          *sysname = Priv2Of(win)->sysname;
  char                 filename[PATH_MAX];
  
    check_snprintf(filename, sizeof(filename), CHL_PLOT_PATTERN_FMT, sysname);
    XhFindFilename(filename, filename, sizeof(filename));
    SavePlot (win, filename, NULL);
}

static void OkCB(Widget     w,
                 XtPointer  closure    __attribute__((unused)),
                 XtPointer  call_data  __attribute__((unused)))
{
  histplotdlg_t       *rec = ThisDialog(XhWindowOf(w));
  
    XhStdDlgHide(rec->box);
}


enum
{
    CHPC_FIXED        = 1 << 0,
    CHPC_NO_SAVE_BUTT = 1 << 1,
    CHPC_NO_MODE_SWCH = 1 << 2,
    CHPC_NO_XSCL_SWCH = 1 << 3,   // "XSCL" -- X-SCaLe
};


static void CreateHistPlotContent(histplotdlg_t *rec,
                                  Widget parent_form, Widget bottom,
                                  int def_w, int def_h,
                                  int flags)
{
  XhPixel        bg  = XhGetColor(XH_COLOR_GRAPH_BG);
  Widget         save;
  Knob           k_md;
  Widget         xmod;
  Knob           k_xs;
  Widget         xscl;

  XmString       s;
  int            row;

    rec->fixed = ((flags & CHPC_FIXED) != 0);

    if ((flags & CHPC_NO_SAVE_BUTT) == 0)
    {
        save = XtVaCreateManagedWidget("push_i", xmPushButtonWidgetClass, parent_form,
                                       XmNlabelString,      s = XmStringCreateLtoR("Save", xh_charset),
                                       XmNtraversalOn,      False,
                                       XmNbottomAttachment, XmATTACH_WIDGET,
                                       XmNbottomWidget,     bottom,
                                       XmNbottomOffset,     bottom != NULL? CHL_INTERKNOB_V_SPACING : 0,
                                       XmNrightAttachment,  XmATTACH_FORM,
                                       XmNrightOffset,      CHL_INTERKNOB_H_SPACING,
                                       NULL);
        XmStringFree(s);
        XhAssignPixmap(save, btn_mini_save_xpm);
        XhSetBalloon  (save, "Save plots to file");
        XtAddCallback (save, XmNactivateCallback, SaveCB, NULL);

        bottom = save;
    }

    ////printf("%s: %d*%d\n", __FUNCTION__, def_w, def_h);
    if ((flags & CHPC_NO_MODE_SWCH) == 0)
    {
        k_md = CreateSimpleKnob(" rw look=selector label=''"
                                " widdepinfo='#T---\v===\v. . .\v***'",
                                0, parent_form,
                                ModeKCB, rec);
        xmod = GetKnobWidget(k_md);
        XtVaSetValues(xmod, XmNtraversalOn, False, NULL);
        attachbottom (xmod, bottom, bottom != NULL? CHL_INTERKNOB_V_SPACING : 0);
        attachright  (xmod, NULL,   CHL_INTERKNOB_H_SPACING);
        bottom = xmod;
    }

    if ((flags & CHPC_NO_XSCL_SWCH) == 0)
    {
        k_xs = CreateSimpleKnob(" rw look=selector label=''"
                                " widdepinfo='#T1:1\v1:10\v1:60'",
                                0, parent_form,
                                XScaleKCB, rec);
        xscl = GetKnobWidget(k_xs);
        XtVaSetValues(xscl, XmNtraversalOn, False, NULL);
        attachbottom (xscl, bottom, bottom != NULL? CHL_INTERKNOB_V_SPACING : 0);
        attachright  (xscl, NULL,   CHL_INTERKNOB_H_SPACING);
        bottom = xscl;
    }

    rec->bkgdGC    = AllocXhGC  (parent_form, XH_COLOR_GRAPH_BG,     NULL);
    rec->axisGC    = AllocXhGC  (parent_form, XH_COLOR_GRAPH_AXIS,   XH_TINY_FIXED_FONT); rec->axis_finfo = last_finfo;
    rec->gridGC    = AllocDashGC(parent_form, XH_COLOR_GRAPH_GRID,   GRID_DASHLENGTH);
    for (row = 0;
         row < MAX_HISTPLOT_LINES_PER_BOX  &&  row < XH_NUM_DISTINCT_LINE_COLORS;
         row++)
    {
        rec->chanGC [0][row] = AllocXhGC(parent_form, XH_COLOR_GRAPH_LINE1 + row, XH_TINY_FIXED_FONT);
        rec->chan_finfo[row] = last_finfo;
        rec->chanGC [1][row] = AllocThGC(parent_form, XH_COLOR_GRAPH_LINE1 + row, NULL);
    }

    /* Note:
           here we use the fact that widgets are displayed top-to-bottom
           in the order of creation: 'graph' is displayed above 'axis'
           because it is created earlier.
    */
    rec->graph = XtVaCreateManagedWidget("graph_graph", xmDrawingAreaWidgetClass,
                                         parent_form,
                                         XmNbackground,  bg,
                                         XmNwidth,       def_w,
                                         XmNheight,      def_h,
                                         XmNtraversalOn, False,
                                         NULL);
    XtAddCallback(rec->graph, XmNexposeCallback, GraphExposureCB, (XtPointer)rec);
    XtAddCallback(rec->graph, XmNresizeCallback, GraphResizeCB,   (XtPointer)rec);

    rec->horzbar = XtVaCreateManagedWidget("horzbar", xmScrollBarWidgetClass,
                                           parent_form,
                                           XmNbackground,          bg,
                                           XmNorientation,         XmHORIZONTAL,
                                           XmNhighlightThickness,  0,
                                           XmNprocessingDirection, XmMAX_ON_LEFT,
                                           NULL);
    SetHorzbarParams(rec);
    XtAddCallback(rec->horzbar, XmNdragCallback,         HorzScrollCB, rec);
    XtAddCallback(rec->horzbar, XmNvalueChangedCallback, HorzScrollCB, rec);

    rec->axis  = XtVaCreateManagedWidget("graph_axis", xmDrawingAreaWidgetClass,
                                         parent_form,
                                         XmNbackground,  bg,
                                         XmNwidth,       def_w,
                                         XmNheight,      def_h,
                                         XmNtraversalOn, False,
                                         NULL);
    XtAddCallback(rec->axis,  XmNexposeCallback, AxisExposureCB,  (XtPointer)rec);
    XtAddCallback(rec->axis,  XmNresizeCallback, AxisResizeCB,    (XtPointer)rec);

    rec->w_l = XtVaCreateManagedWidget("", widgetClass, parent_form,
                                       XmNborderWidth, 0,
                                       XmNwidth,       1,
                                       XmNheight,      1,
                                       NULL);
    rec->w_r = XtVaCreateManagedWidget("", widgetClass, parent_form,
                                       XmNborderWidth, 0,
                                       XmNwidth,       1,
                                       XmNheight,      1,
                                       NULL);
    
    rec->grid = XhCreateGrid(parent_form, "grid");
    XhGridSetGrid   (rec->grid, 0,                       0);
    XhGridSetSpacing(rec->grid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(rec->grid, 0,                       0);
    XtVaSetValues(rec->grid, XmNbackground, bg, NULL);

    for (row = 0;  row < MAX_HISTPLOT_LINES_PER_BOX;  row++)
    {
        rec->label[row] =
            XtVaCreateWidget("onoff_o", xmToggleButtonWidgetClass, rec->grid,
                             XmNbackground,  bg,
                             XmNforeground,  XhGetColor(XH_COLOR_GRAPH_LINE1 + (row % XH_NUM_DISTINCT_LINE_COLORS)),
                             XmNselectColor, XhGetColor(XH_COLOR_GRAPH_LINE1 + (row % XH_NUM_DISTINCT_LINE_COLORS)),
                             XmNlabelString, s = XmStringCreateLtoR("", xh_charset),
                             XmNtraversalOn, False,
                             NULL);
        XmStringFree(s);
        XhGridSetChildPosition (rec->label[row],  0,  row);
        XhGridSetChildFilling  (rec->label[row],  0,  0);
        XhGridSetChildAlignment(rec->label[row], -1, -1);
        XtAddCallback(rec->label[row], XmNvalueChangedCallback, ShowOnOffCB, lint2ptr(row));

        rec->val_dpy[row] =
            XtVaCreateWidget("text_o", xmTextWidgetClass, rec->grid,
                             XmNbackground,            bg,
                             XmNscrollHorizontal,      False,
                             XmNcursorPositionVisible, False,
                             XmNeditable,              False,
                             XmNtraversalOn,           False,
                             NULL);
        XhGridSetChildPosition (rec->val_dpy[row],  1,  row);
        XhGridSetChildFilling  (rec->val_dpy[row],  0,  0);
        XhGridSetChildAlignment(rec->val_dpy[row], -1, -1);
        
        XtInsertEventHandler(rec->label  [row], ButtonPressMask, False, PlotMouse3Handler, lint2ptr(row), XtListHead);
        XtInsertEventHandler(rec->val_dpy[row], ButtonPressMask, False, PlotMouse3Handler, lint2ptr(row), XtListHead);

        if (!rec->fixed)
        {
            rec->b_up[row] = MakePlotButton(rec->grid, 2, row, "^", PlotUpCB);
            rec->b_dn[row] = MakePlotButton(rec->grid, 3, row, "v", PlotDnCB);
            rec->b_rm[row] = MakePlotButton(rec->grid, 4, row, "-", PlotRmCB);
        }
    }

    XtVaGetValues(rec->val_dpy[0],
                  XmNforeground, &(rec->deffg),
                  XmNbackground, &(rec->defbg),
                  NULL);
    
    SetAttachments(rec);
    CalcSetMargins(rec);
}

static void AddToHistPlot(histplotdlg_t *rec, Knob ki)
{
  int            row;
  
#if 1
    if (CdrActivateKnobHistory(ki, rec->def_histring_size > 10? rec->def_histring_size
                                                              : DEF_RINGSIZE) < 0) return;
#else
    /* Should we allocate a buffer? */
    if (ki->histring == NULL)
    {
        ki->histring_size = rec->def_histring_size > 10? rec->def_histring_size
                                                       : DEF_RINGSIZE;
        ki->histring      = malloc(ki->histring_size * sizeof(double));
    }
#endif

    /* Check if this knob is already in a list */
    for (row = 0;
         row < rec->rows_used  &&  rec->target[row] != ki;
         row++);
  
    if (row >= rec->rows_used)
    {
    
        if (rec->rows_used >= MAX_HISTPLOT_LINES_PER_BOX)
        {
            for (row = 0;  row < rec->rows_used - 1;  row++)
            {
                RenewPlotRow (rec, row, rec->target[row + 1], rec->show[row + 1]);
                UpdatePlotRow(rec, row);
            }
            rec->rows_used--;
        }

        row = rec->rows_used;

        RenewPlotRow (rec, row, ki, 1);
        UpdatePlotRow(rec, row);
        SetPlotCount (rec, row + 1);

        CalcSetMargins(rec);
        DrawGraph     (rec, True);
        DrawAxis      (rec, True);
    }
}

static histplotdlg_t *ThisDialog(XhWindow win)
{
  histplotdlg_t *rec = &(Priv2Of(win)->hp);
  XhPixel        bg  = XhGetColor(XH_COLOR_GRAPH_BG);

  Widget         frame;
  Widget         form;
  Widget         ok;

  XmString       s;
  
  char           buf[100];
  
    if (rec->initialized) return rec;

    rec->x_scale = x_scale_factors[0];
    
    check_snprintf(buf, sizeof(buf), "%s: History plot", Priv2Of(win)->sysname);
    rec->box = XhCreateStdDlg(win, "histPlot", buf, NULL, NULL,
                              XhStdDlgFNothing | XhStdDlgFNoMargins | XhStdDlgFResizable | XhStdDlgFZoomable);

    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
                                    rec->box,
                                    XmNshadowType,      XmSHADOW_IN,
                                    XmNshadowThickness, 1,
                                    NULL);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass,
                                   frame,
                                   XmNbackground,      bg,
                                   XmNshadowThickness, 0,
                                   NULL);

    ok = XtVaCreateManagedWidget("ok", xmPushButtonWidgetClass, form,
                                 XmNlabelString,      s = XmStringCreateLtoR("Close", xh_charset),
                                 XmNshowAsDefault,    1,
                                 XmNrightAttachment,  XmATTACH_FORM,
                                 XmNrightOffset,      CHL_INTERKNOB_H_SPACING,
                                 XmNbottomAttachment, XmATTACH_FORM,
                                 XmNbottomOffset,     CHL_INTERKNOB_V_SPACING,
                                 NULL);
    XmStringFree(s);
    XtAddCallback(ok, XmNactivateCallback, OkCB, NULL);
    XtVaSetValues(rec->box, XmNdefaultButton, ok, NULL);

    CreateHistPlotContent(rec, form, ok, DEF_PLOT_W, DEF_PLOT_H, 0);
    
    rec->initialized = 1;
    return rec;
}

void Chl_E_ToHistPlot(Knob ki)
{
  histplotdlg_t *rec = ThisDialog(XhWindowOf(ki->indicator));

    if (rec->fixed) return;
  
    AddToHistPlot(rec, ki);
    
    if (rec->box != NULL) XhStdDlgShow(rec->box); // ==NULL with LOGD_HISTPLOT
}

void UpdatePlotWindow(XhWindow win)
{
  histplotdlg_t  *rec = &(Priv2Of(win)->hp);
  struct timeval  timenow;
  struct timeval  timediff;
      

    if (!rec->initialized) return;
    
    if (rec->box == NULL  ||  XtIsManaged(rec->box))
    {
        gettimeofday(&timenow, NULL);
        timeval_subtract(&timediff, &timenow, &(rec->timeupd));
        if (timediff.tv_sec > 0  ||
            timediff.tv_usec >= 1000000 / 5/*!!!5Hz*/)
        {
            DisplayPlot(rec);
            rec->timeupd = timenow;
        }
    }
}

void UpdatePlotProps (XhWindow win)
{
  histplotdlg_t *rec = &(Priv2Of(win)->hp);

    if (!rec->initialized) return;
    
    if (rec->box == NULL  ||  XtIsManaged(rec->box))
    {
        CalcSetMargins(rec);
        DisplayPlot   (rec);
        DrawAxis      (rec, True);
    }
}

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int   w;
    int   h;
    char *plots[MAX_HISTPLOT_LINES_PER_BOX];

    int   mode;
    int   no_mode;
    int   fixed;
} histplotopts_t;

static int AddPluginParser(const char *str, const char **endptr,
                           void *rec, size_t recsize,
                           const char *separators, const char *terminators,
                           void *privptr __attribute__((unused)), char **errstr)
{
  char       **plots = rec;
  int          count = recsize / sizeof(*plots);

  const char  *beg_p;
  const char  *srcp;
  size_t       len;

  int          nl;

    if (str == NULL) return PSP_R_OK; // Initialization: (NULL->value)

    /* Advance until terminator (NO quoting/backslashing support */
    for (srcp = beg_p = str;
         *srcp != '\0'                       &&
         strchr(terminators, *srcp) == NULL  &&
         strchr(separators,  *srcp) == NULL;
         srcp++);
    len = srcp - beg_p;

    if (endptr != NULL) *endptr = srcp;

    if (rec == NULL) return PSP_R_OK; // Skipping: (str->NULL)

    if (srcp == beg_p) return PSP_R_OK; // Empty value -- do nothing

    /* Find unused cell */
    for (nl = 0;  nl < count  &&  plots[nl] != NULL;  nl++) ;
    if (nl < count)
    {
        if ((plots[nl] = malloc(len + 1)) == NULL)
        {
            *errstr = "malloc() failure";
            return PSP_R_USRERR;
        }
        memcpy(plots[nl], beg_p, len);
        plots[nl][len] = '\0';
    }
    else
    {
        *errstr = "too many plots requested";
        return PSP_R_USRERR;
    }

    return PSP_R_OK;
}

static psp_lkp_t mode_lkp[] =
{
    {"line", PLOTMODE_LINE},
    {"wide", PLOTMODE_WIDE},
    {"dots", PLOTMODE_DOTS},
    {"blot", PLOTMODE_BLOT},
    {NULL, 0}
};

static psp_paramdescr_t text2histplotopts[] =
{
    PSP_P_INT    ("width",  histplotopts_t, w,        DEF_PLOT_W, 50, 5000),
    PSP_P_INT    ("height", histplotopts_t, h,        DEF_PLOT_H, 50, 5000),

    PSP_P_MSTRING("plot1",  histplotopts_t, plots[0], NULL, 100),
    PSP_P_MSTRING("plot2",  histplotopts_t, plots[1], NULL, 100),
    PSP_P_MSTRING("plot3",  histplotopts_t, plots[2], NULL, 100),
    PSP_P_MSTRING("plot4",  histplotopts_t, plots[3], NULL, 100),
    PSP_P_MSTRING("plot5",  histplotopts_t, plots[4], NULL, 100),
    PSP_P_MSTRING("plot6",  histplotopts_t, plots[5], NULL, 100),
    PSP_P_MSTRING("plot7",  histplotopts_t, plots[6], NULL, 100),
    PSP_P_MSTRING("plot8",  histplotopts_t, plots[7], NULL, 100),
    PSP_P_MSTRING("plot9",  histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX >  8?  8 : 0], NULL, 100),
    PSP_P_MSTRING("plot10", histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX >  9?  9 : 0], NULL, 100),
    PSP_P_MSTRING("plot11", histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX > 10? 10 : 0], NULL, 100),
    PSP_P_MSTRING("plot12", histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX > 11? 11 : 0], NULL, 100),
    PSP_P_MSTRING("plot13", histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX > 12? 12 : 0], NULL, 100),
    PSP_P_MSTRING("plot14", histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX > 13? 13 : 0], NULL, 100),
    PSP_P_MSTRING("plot15", histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX > 14? 14 : 0], NULL, 100),
    PSP_P_MSTRING("plot16", histplotopts_t, plots[MAX_HISTPLOT_LINES_PER_BOX > 15? 15 : 0], NULL, 100),
    PSP_P_PLUGIN ("add",    histplotopts_t, plots, AddPluginParser, NULL),

    PSP_P_LOOKUP ("mode",   histplotopts_t, mode,     PLOTMODE_LINE, mode_lkp),
    PSP_P_FLAG   ("nomode", histplotopts_t, no_mode,  1, 0),
    
    PSP_P_FLAG   ("fixed",  histplotopts_t, fixed,    1, 0),
    PSP_P_FLAG   ("chgbl",  histplotopts_t, fixed,    0, 1),

    PSP_P_END()
};

static XhWidget HISTPLOT_Create_m(knobinfo_t *ki, XhWidget parent)
{
  histplotdlg_t  *rec = &(Priv2Of(XhWindowOf(parent))->hp);
  histplotopts_t  opts;
  
  XhPixel         bg  = XhGetColor(XH_COLOR_GRAPH_BG);
  Widget          w;
  Widget          form;

  XhWindow        win = XhWindowOf(parent);
  int             n;
  Knob            victim;
  
    if (rec->initialized)
    {
        fprintf(stderr, "%s(%s/\"%s\"): attempt to use multiple HISTPLOTs.\n",
                __FUNCTION__, ki->ident, ki->label);
        w = XtVaCreateManagedWidget("", widgetClass, parent,
                                    XmNborderWidth, 0,
                                    XmNwidth,       10,
                                    XmNheight,      10,
                                    XmNbackground,  XhGetColor(XH_COLOR_JUST_RED),
                                    NULL);
        return ABSTRZE(w);
    }

    ParseWiddepinfo(ki, &opts, sizeof(opts), text2histplotopts);
    rec->mode = opts.mode;
    rec->x_scale = x_scale_factors[0];
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass,
                                   CNCRTZE(parent),
                                   XmNbackground,      bg,
                                   XmNshadowThickness, 0,
                                   NULL);

    CreateHistPlotContent(rec, form, NULL, opts.w, opts.h,
                          (opts.no_mode? CHPC_NO_MODE_SWCH : 0) |
                          (opts.fixed?   CHPC_FIXED        : 0));

    for (n = 0;  n < countof(opts.plots);  n++)
        if (opts.plots[n] != NULL)
        {
            victim = ChlFindKnob(win, opts.plots[n]);
            if (victim == NULL)
                fprintf(stderr, "%s(%s/\"%s\"): plot%d: knob \"%s\" not found\n",
                        __FUNCTION__, ki->ident, ki->label, n+1, opts.plots[n]);
            else
                AddToHistPlot(rec, victim);
        }
    
    rec->initialized = 1;
    
    return ABSTRZE(form);
}

static void NoColorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}

static knobs_vmt_t KNOB_HISTPLOT_VMT  = {HISTPLOT_Create_m,  NULL, NULL, NoColorize_m, NULL};

static knobs_widgetinfo_t histplot_widgetinfo[] =
{
    {LOGD_HISTPLOT, "histplot", &KNOB_HISTPLOT_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  histplot_widgetset = {histplot_widgetinfo, NULL};

void RegisterHistplotKnob(void)
{
  static int  is_registered = 0;

    if (is_registered) return;
    is_registered = 1;
    KnobsRegisterWidgetset(&histplot_widgetset);
}
