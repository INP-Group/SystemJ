#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>

#include "misc_macros.h"
#include "misclib.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "Cdr.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Xh_fallbacks.h"
#include "Knobs.h"
#include "Knobs_typesP.h"
#include "KnobsI.h"

#include "bpmclient.h"
#include "bpmclient_graph_knobplugin.h"


//////////////////////////////////////////////////////////////////////
static XFontStruct     *last_finfo;

static GC AllocOneGC(Widget w, Pixel fg, Pixel bg, const char *fontspec)
{
  XtGCMask   mask;
  XGCValues  vals;
    
    mask            = GCForeground | GCBackground | GCLineWidth;
    vals.foreground = fg;
    vals.background = bg;
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

static Pixel AllocColor(Widget w, const char *spec)
{
  Colormap     cmap;
  XColor       xcol;

    XtVaGetValues(w, XmNcolormap, &cmap, NULL);
    if (!XAllocNamedColor(XtDisplay(w), cmap, spec, &xcol, &xcol))
    {
        fprintf(stderr, "Failed to allocate color '%s'\n", spec);
        xcol.pixel = BlackPixelOfScreen(XtScreen(w));
    }

    return xcol.pixel;
}

//////////////////////////////////////////////////////////////////////


/********************************************************************
*  LOGD_BPM_GRAPH                                                   *
********************************************************************/

enum {MAXPOINTS = 14};
static int bpm_channels[14] = {0,1,2,3,4,5,6,10,11,12,13,14,15,16};

//////////////////////////////////////////////////////////////////////

enum
{
    GRAPH_TICK_LEN         = 4,
    GRAPH_TICKS_PER_HALF   = 10,
    GRAPH_TITLES_PER_TICKS = 5,
    GRAPH_TITLES_SPC       = 1,
    GRID_DASHLENGTH        = 6,
};

typedef struct
{
    GC  bkgdGC;
    GC  axisGC;
    GC  gridGC;
    GC  titlGC;
    GC  dataGC[COLALARM_LAST + 1];
    GC  udflGC;
    GC  prevGC;
    GC  lineGC;
    XFontStruct *titl_finfo;

    int is_y;
    int count;
    cda_physchanhandle_t  v_h[MAXPOINTS];
    double                v_v[MAXPOINTS];
    tag_t                 v_t[MAXPOINTS];
    rflags_t              v_f[MAXPOINTS];
    colalarm_t            v_s[MAXPOINTS];
    colalarm_t            v_u[MAXPOINTS];
    double                v_o[MAXPOINTS];

    Pixmap pix;

    /* Geometry */
    int pointsize2;
    int pxppt;
    int maxy;
    int maxmm;

    int lmargin;
    int rmargin;
    int tmargin;
    int bmargin;
    int width;
    int height;
} BPM_GRAPH_privaterec_t;

static int GRAPH_n2x(BPM_GRAPH_privaterec_t *me, int n)
{
    return me->lmargin + me->pxppt * n;
}

static int GRAPH_v2y(BPM_GRAPH_privaterec_t *me, double v)
{
    return
        + me->tmargin
        + me->maxy
        - ((v * me->maxy) / me->maxmm);
}

static int GRAPH_calc_width (BPM_GRAPH_privaterec_t *me)
{
    return me->lmargin + me->pxppt * (me->count-1) + me->rmargin;
}

static int GRAPH_calc_height(BPM_GRAPH_privaterec_t *me)
{
    return me->tmargin + (me->maxy * 2 + 1) + me->bmargin;
}

static const char *GRAPH_get_tick_fmt(BPM_GRAPH_privaterec_t *me)
{
    if ((me->maxmm % GRAPH_TICKS_PER_HALF) == 0) return "% .0fmm";
    else                                         return "% .1fmm";
}

static psp_paramdescr_t text2privaterec[] =
{
    PSP_P_FLAG  ("x",          BPM_GRAPH_privaterec_t, is_y,       0, 1),
    PSP_P_FLAG  ("y",          BPM_GRAPH_privaterec_t, is_y,       1, 0),
    PSP_P_INT   ("count",      BPM_GRAPH_privaterec_t, count,      1,   1,  MAXPOINTS),
    PSP_P_INT   ("pointsize",  BPM_GRAPH_privaterec_t, pointsize2, 9,   1,  30),
    PSP_P_INT   ("pxppt",      BPM_GRAPH_privaterec_t, pxppt,      35,  10, 100),
    PSP_P_INT   ("maxy",       BPM_GRAPH_privaterec_t, maxy,       100, 10, 1000),
    PSP_P_INT   ("maxmm",      BPM_GRAPH_privaterec_t, maxmm,      10,  1,  20),
    
    PSP_P_END()
};

static void BPM_GRAPH_Draw(knobinfo_t *ki, int do_refresh)
{
  BPM_GRAPH_privaterec_t *me  = (BPM_GRAPH_privaterec_t*)ki->widget_private;
  Widget                  a   = CNCRTZE(ki->indicator);
  Display                *dpy = XtDisplay(a);
  Window                  win = XtWindow(a);

  const char             *fmt = GRAPH_get_tick_fmt(me);
  int                     x0, y0;
  int                     xe;
  int                     x,  y;
  int                     xp, yp;
  double                  v;
  int                     stage;
  int                     n;
  char                    buf[100];

  GC                      vGC;

    XFillRectangle(dpy, me->pix, me->bkgdGC, 0, 0, me->width, me->height);

    /* Draw axes */
    x0 = GRAPH_n2x(me, 0);
    y0 = GRAPH_v2y(me, 0);
    xe = GRAPH_n2x(me, me->count-1);
    XDrawLine(dpy, me->pix, me->axisGC, x0, y0, xe, y0);
    XDrawLine(dpy, me->pix, me->axisGC,
              x0, GRAPH_v2y(me, -me->maxmm),
              x0, GRAPH_v2y(me, +me->maxmm));

    x = x0 - GRAPH_TICK_LEN;
    for (n = 0;  n <= GRAPH_TICKS_PER_HALF;  n++)
        for (stage = 0;  stage <= (n == 0? 0 : 1);  stage++)
        {
            v = (me->maxmm / (double)GRAPH_TICKS_PER_HALF) * n;
            if (stage != 0) v = -v;
            y = GRAPH_v2y(me, v);
            XDrawLine(dpy, me->pix, me->axisGC, x, y, x0, y);
            ////XDrawLine(dpy, me->pix, me->gridGC, x0+1, y, xe, y);

            if ((n % GRAPH_TITLES_PER_TICKS) == 0)
            {
                sprintf(buf, fmt, v);
                XDrawString(dpy, me->pix, me->titlGC,
                            x0 - GRAPH_TICK_LEN - GRAPH_TITLES_SPC - XTextWidth(me->titl_finfo, buf, strlen(buf)),
                            y + me->titl_finfo->ascent - (me->titl_finfo->ascent + me->titl_finfo->descent) / 2,
                            buf, strlen(buf));
            }
        }
    
    /* Draw old data */
    for (n = 0;  n < me->count;  n++)
    {
        x = GRAPH_n2x(me, n);
        y = GRAPH_v2y(me, me->v_o[n]);

        XFillRectangle(dpy, me->pix, me->prevGC,
                       x - me->pointsize2, y - me->pointsize2,
                       me->pointsize2*2+1, me->pointsize2*2+1);

        if (n > 0)
            XDrawLine(dpy, me->pix, me->prevGC, xp, yp, x, y);

        xp = x;
        yp = y;
    }

    /* Draw new values */
    for (stage = 0;  stage <= 1;  stage++)
        for (n = 0;  n < me->count;  n++)
        {
        x = GRAPH_n2x(me, n);
        y = GRAPH_v2y(me, me->v_v[n]);

            if (stage)
            {
                vGC = me->dataGC[me->v_s[n]];
                if (me->v_v[n] == 0.0  &&  me->v_u[n] != me->v_s[n])
                    vGC = me->udflGC;
                
                XFillRectangle(dpy, me->pix, vGC,
                               x - me->pointsize2, y - me->pointsize2,
                               me->pointsize2*2+1, me->pointsize2*2+1);
            }
            else
            {
                if (n > 0)
                    XDrawLine(dpy, me->pix, me->lineGC, xp, yp, x, y);
            }
                
            xp = x;
            yp = y;
        }

    if (do_refresh)
        XCopyArea(dpy, me->pix, win, me->bkgdGC, 0, 0, me->width, me->height, 0, 0);
}

static void BPM_GRAPH_MouseHandler(Widget     w        __attribute__ ((unused)),
                                   XtPointer  closure,
                                   XEvent    *event,
                                   Boolean   *continue_to_dispatch  __attribute__ ((unused)))
{
  knobinfo_t          *ki = (knobinfo_t *)closure;
  XMotionEvent        *mev  = (XMotionEvent *) event;
}

static XhWidget BPM_GRAPH_Create_m(knobinfo_t *ki, XhWidget parent)
{
    Widget                    w;
    BPM_GRAPH_privaterec_t   *me;
    XhPixel                   bg = XhGetColor(XH_COLOR_GRAPH_BG);
    char                      buf[100];

    cda_serverid_t            sid;
    int                       n;
    int                       chan;

    if ((me = ki->widget_private = XtMalloc(sizeof(BPM_GRAPH_privaterec_t))) == NULL)
        return NULL;
    bzero(me, sizeof(*me));
  
    ParseWiddepinfo(ki, me, sizeof(*me), text2privaterec);
    me->pointsize2 = me->pointsize2 / 2;

    me->titlGC  = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_TITLES), bg, XH_TINY_FIXED_FONT);
    me->titl_finfo = last_finfo;
    sprintf(buf, GRAPH_get_tick_fmt(me), -(double)(me->maxmm));
    
    me->lmargin = GRAPH_TICK_LEN  + 1 + XTextWidth(me->titl_finfo, buf, strlen(buf)) + GRAPH_TITLES_SPC;
    me->rmargin = me->pointsize2 + 1;
    me->tmargin = me->pointsize2 + 1;
    me->bmargin = me->pointsize2 + 1;
    me->width  = GRAPH_calc_width (me);
    me->height = GRAPH_calc_height(me);
  
    w = XtVaCreateManagedWidget("bpm_graph",
                                xmDrawingAreaWidgetClass,
                                CNCRTZE(parent),
                                XmNtraversalOn, False,
                                XmNbackground,  bg,
                                XmNwidth,       me->width,
                                XmNheight,      me->height,
                                NULL);
    XtAddEventHandler(w, PointerMotionMask | LeaveWindowMask, False, BPM_GRAPH_MouseHandler, (XtPointer)ki);
    ki->indicator = w;

    me->bkgdGC = AllocOneGC(w, bg,                               bg, NULL);
    me->axisGC = AllocOneGC(w, XhGetColor(XH_COLOR_GRAPH_AXIS),  bg, NULL);
    me->gridGC = AllocDashGC(w, XH_COLOR_GRAPH_REPERS, GRID_DASHLENGTH);
    AllocStateGCs(ABSTRZE(w), me->dataGC, XH_COLOR_GRAPH_BAR1);
    me->udflGC = AllocOneGC(w, AllocColor(w, "#004000"),         bg, NULL);
    me->prevGC = AllocOneGC(w, XhGetColor(XH_COLOR_GRAPH_PREV),  bg, NULL);
    me->lineGC = AllocOneGC(w, XhGetColor(XH_COLOR_GRAPH_GRID),  bg, NULL);

    me->pix = XCreatePixmap(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
                            me->width, me->height,
                            DefaultDepthOfScreen(XtScreen(w)));
    XtVaSetValues(w, XmNbackgroundPixmap, me->pix, NULL);
    BPM_GRAPH_Draw(ki, False);
    
    sid = ChlGetMainsid(XhWindowOf(w));
    
    for (n = 0;  n < me->count;  n++)
    {
        chan = me->is_y? KARPOV_Y_C(bpm_channels[n])
                       : KARPOV_X_C(bpm_channels[n]);
        me->v_h[n] = cda_add_physchan(sid, chan);
        ////fprintf(stderr, "[%d]:%d\n", n, chan);
    }
    
    return (XhWidget)w;
}

static void BPM_GRAPH_SetValue_m(knobinfo_t *ki __attribute__((unused)), double v __attribute__((unused)))
{
  BPM_GRAPH_privaterec_t *me = (BPM_GRAPH_privaterec_t*)ki->widget_private;
  int                     n;

#define GRAPH_SIMULATE 0

#if GRAPH_SIMULATE
  static int ctr = 0;
#endif
  
    for (n = 0;  n < me->count;  n++)
    {
        me->v_o[n] = me->v_v[n];
        cda_getphyschanval(me->v_h[n],
                           &(me->v_v[n]),
                           &(me->v_t[n]),
                           &(me->v_f[n]));
        me->v_f[n] &=~ CXCF_FLAG_OTHEROP;
        me->v_s[n] = datatree_choose_knobstate(NULL, me->v_f[n]);
        me->v_u[n] = datatree_choose_knobstate(NULL, me->v_f[n] &~ CXRF_OVERLOAD);
#if GRAPH_SIMULATE
        me->v_v[n] = sin((ctr + n) / 1.0) * me->maxmm / 2;
#endif
    }

#if GRAPH_SIMULATE
    ctr = (ctr + 1) & 0xFF;
#endif
    
    BPM_GRAPH_Draw(ki, True);
}

//////////////////////////////////////////////////////////////////////

knobs_vmt_t BPM_GRAPH_VMT   = {BPM_GRAPH_Create_m,   NULL, BPM_GRAPH_SetValue_m,   NULL, NULL};
