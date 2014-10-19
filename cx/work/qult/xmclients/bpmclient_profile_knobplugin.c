#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>

#include "misc_macros.h"
#include "misclib.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "Cdr.h"
#include "Chl.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Xh_fallbacks.h"
#include "Knobs.h"
#include "Knobs_typesP.h"
#include "KnobsI.h"

#include "bpmclient.h"
#include "bpmclient_profile_knobplugin.h"


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
//PROFILE
//////////////////////////////////////////////////////////////////////

enum {MAXPOINTS = 14};
static int bpm_channels[14] = {0,1,2,3,4,5,6,10,11,12,13,14,15,16};

//////////////////////////////////////////////////////////////////////

enum
{
    PROFILE_HORZ_MARGIN      = 5,
    PROFILE_VERT_MARGIN      = 1,
    PROFILE_RADIUS	     = 40,
};

typedef struct
{
    GC                    bkgdGC;
    GC                    axisGC;
    GC                    dataGC[COLALARM_LAST + 1];
    GC                    udflGC;
    GC                    prevGC;
    GC                    tubeGC;
    GC                    pckpGC;
    GC                    textGC;
    XFontStruct          *text_finfo;

    cda_physchanhandle_t  v_h[2];
    double                v_v[2];
    tag_t                 v_t[2];
    rflags_t              v_f[2];
    colalarm_t            v_s;
    colalarm_t            v_u;
    double                v_o[2];

    cda_physchanhandle_t  c_h[4];
    double                c_v[4];
    
    Pixmap                pix;
    
    int                   n;
    int                   size;
    int                   radius;
    int                   maxmm;
    int                   beamsize;
    int                   beamrad;
    
    int                   lmargin;
    int                   rmargin;
    int                   tmargin;
    int                   bmargin;
    int                   width;
    int                   height;
} BPM_PROFILE_privaterec_t;

static int PROFILE_vx2x(BPM_PROFILE_privaterec_t *me, double vx)
{
    return me->lmargin + me->radius + ((vx * me->radius) / me->maxmm);
}

static int PROFILE_vy2y(BPM_PROFILE_privaterec_t *me, double vy)
{
    return me->tmargin + me->radius - ((vy * me->radius) / me->maxmm);
}

static psp_paramdescr_t text2privaterec[] =
{
    PSP_P_INT   ("n",        BPM_PROFILE_privaterec_t, n,        0,  0,  MAXPOINTS-1),
    PSP_P_INT   ("size",     BPM_PROFILE_privaterec_t, size,     80, 30, 200),
    PSP_P_INT   ("maxmm",    BPM_PROFILE_privaterec_t, maxmm,    10, 1,  20),
    PSP_P_INT   ("beamsize", BPM_PROFILE_privaterec_t, beamsize, 10, 1,  20),
    PSP_P_END()
};

#define MIN_OF(a,b)        (((int)(a) < (int)(b)) ? (a) : (b))
#define MAX_OF(a,b)        (((int)(a) > (int)(b)) ? (a) : (b))

/* from Knobs_dial_widget.c */
static void DrawWideArc(Display  *display, Drawable d, GC gc,
                        int x, int y,
			unsigned int width, unsigned int height,
			int angle1, int angle2,
			unsigned int thickness)
{
    XGCValues  cur_values;
    XGCValues  new_values;
    XtGCMask   mask;
    int        lw = MIN_OF(thickness, MIN_OF(width, height) / 2);
													
    if (thickness <= 0  ||  width <= 0  ||  height <= 0) return;
													    
    mask = 0;
    new_values.line_width = lw;  mask |= GCLineWidth;
													    
    XGetGCValues(display, gc, mask, &cur_values);
    XChangeGC   (display, gc, mask, &new_values);
															    
    XDrawArc(display, d, gc,
	x + lw / 2, y + lw / 2,
        MAX_OF(width  - lw, 1),
	MAX_OF(height - lw, 1),
	angle1, angle2);
																						    
    XChangeGC   (display, gc, mask, &cur_values);
}

static void BPM_PROFILE_Draw(knobinfo_t *ki, int do_refresh)
{
  BPM_PROFILE_privaterec_t *me = (BPM_PROFILE_privaterec_t*)ki->widget_private;
  Widget                    a   = ki->indicator;
  Display                  *dpy = XtDisplay(a);
  Window                    win = XtWindow(a);

  int                     x0, y0;
  int                     x,  y;
  
  GC                      vGC;
  int                     n;
  
  char                    buf[100];

    XFillRectangle(dpy, me->pix, me->bkgdGC, 0, 0, me->width, me->height);

    x0 = PROFILE_vx2x(me, 0);
    y0 = PROFILE_vy2y(me, 0);
  
    // Axes
    XDrawLine(dpy, me->pix, me->axisGC,
              x0 - me->radius + 1, y0,
              x0 + me->radius - 1, y0);
    XDrawLine(dpy, me->pix, me->axisGC,
              x0,                  y0 - me->radius + 1,
              x0,                  y0 + me->radius - 1);

    // Tube
    DrawWideArc(dpy, me->pix, me->tubeGC,
                me->lmargin, me->tmargin,
                me->size, me->size,
                0*64, 360*64,
                4);

    // Pickups
    for (n = 0;  n < 4;  n++)
        DrawWideArc(dpy, me->pix, me->pckpGC,
                    me->lmargin, me->tmargin,
                    me->size, me->size,
                    (n*90-10)*64, 20*64,
                    4);

    // Old value
    x = PROFILE_vx2x(me, me->v_o[0]);
    y = PROFILE_vy2y(me, me->v_o[1]);
    XFillArc(dpy, me->pix, me->prevGC,
             x - me->beamrad, y - me->beamrad,
             me->beamsize, me->beamsize,
             0*64, 360*64);

    // New value
    vGC = me->dataGC[me->v_s];
    if ((me->v_v[0] == 0.0  ||  me->v_v[1] == 0.0)  &&  me->v_u != me->v_s)
        vGC = me->udflGC;
    
    x = PROFILE_vx2x(me, me->v_v[0]);
    y = PROFILE_vy2y(me, me->v_v[1]);
    XFillArc(dpy, me->pix, vGC,
             x - me->beamrad, y - me->beamrad,
             me->beamsize, me->beamsize,
             0*64, 360*64);

    // x
    sprintf(buf, "%.1f", me->v_v[0]);
    XDrawString(dpy, me->pix, me->textGC,
                0,
                0+me->text_finfo->ascent,
                buf, strlen(buf));

    // y
    sprintf(buf, "%.1f", me->v_v[1]);
    XDrawString(dpy, me->pix, me->textGC,
                me->width - XTextWidth(me->text_finfo, buf, strlen(buf)),
                0+me->text_finfo->ascent,
                buf, strlen(buf));

    sprintf(buf, "Sum=%d", (int)(me->c_v[0]+me->c_v[1]+me->c_v[2]+me->c_v[3]));
    XDrawString(dpy, me->pix, me->textGC,
                0, me->height - me->text_finfo->descent,
                buf, strlen(buf));
    
    if (do_refresh)
        XCopyArea(dpy, me->pix, win, me->bkgdGC, 0, 0, me->width, me->height, 0, 0);
}

static void BPM_PROFILE_MouseHandler(Widget     w        __attribute__ ((unused)),
                                   XtPointer  closure,
                                   XEvent    *event,
                                   Boolean   *continue_to_dispatch  __attribute__ ((unused)))
{
    knobinfo_t          *ki  = (knobinfo_t *)closure;
    XMotionEvent        *mev = (XMotionEvent *) event;
}

static XhWidget BPM_PROFILE_Create_m(knobinfo_t *ki, XhWidget parent)
{
  Widget                    w;
  BPM_PROFILE_privaterec_t *me;
  XhPixel                   bg = XhGetColor(XH_COLOR_GRAPH_BG);

  cda_serverid_t            sid;
  int                       i;

    if ((me = ki->widget_private = XtMalloc(sizeof(BPM_PROFILE_privaterec_t))) == NULL)
        return NULL;
    bzero(me, sizeof(*me));
  
    ParseWiddepinfo(ki, me, sizeof(*me), text2privaterec);
    me->radius = me->size / 2;
    me->size   = me->radius * 2 + 1;

    me->beamrad  = me->beamsize / 2;
    if (me->beamrad > me->radius / 2) me->beamrad = me->radius / 2;
    me->beamsize = me->beamrad * 2 + 1;
  
    me->textGC = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_TITLES), bg, XH_TINY_FIXED_BOLD_FONT);
    me->text_finfo = last_finfo;

    me->lmargin = PROFILE_HORZ_MARGIN;
    me->rmargin = PROFILE_HORZ_MARGIN;
    me->tmargin = PROFILE_VERT_MARGIN + me->text_finfo->ascent + me->text_finfo->descent;
    me->bmargin = PROFILE_VERT_MARGIN + me->text_finfo->ascent + me->text_finfo->descent;
    me->width   = me->lmargin + me->size + me->rmargin;
    me->height  = me->tmargin + me->size + me->bmargin;
    
    w = XtVaCreateManagedWidget("bpm_profile",
                                xmDrawingAreaWidgetClass,
                                (Widget)parent,
                                XmNtraversalOn, False,
                                XmNbackground,  bg,
                                XmNwidth,       me->width,
                                XmNheight,      me->height,
                                NULL);
    XtAddEventHandler(w, PointerMotionMask | LeaveWindowMask, False, BPM_PROFILE_MouseHandler, (XtPointer)ki);
    ki->indicator = w;

    me->bkgdGC = AllocOneGC(w, bg,                                bg, NULL);
    me->axisGC = AllocOneGC(w, XhGetColor(XH_COLOR_GRAPH_AXIS),   bg, NULL);
    AllocStateGCs(ABSTRZE(w), me->dataGC, XH_COLOR_JUST_GREEN*0+XH_COLOR_GRAPH_BAR1);
    me->udflGC = AllocOneGC(w, AllocColor(w, "#004000"),          bg, NULL);
    me->prevGC = AllocOneGC(w, XhGetColor(XH_COLOR_GRAPH_PREV),   bg, NULL);
    me->tubeGC = AllocOneGC(w, XhGetColor(XH_COLOR_BS_DEFAULT),   bg, NULL);
    me->pckpGC = AllocOneGC(w, XhGetColor(XH_COLOR_JUST_BLUE),    bg, NULL);

    me->pix = XCreatePixmap(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
                            me->width, me->height,
                            DefaultDepthOfScreen(XtScreen(w)));
    XtVaSetValues(w, XmNbackgroundPixmap, me->pix, NULL);
    BPM_PROFILE_Draw(ki, False);
    
    sid = ChlGetMainsid(XhWindowOf(w));
    
    for (i = 0;  i < 2;  i++)
        me->v_h[i] = cda_add_physchan(sid, i == 0? KARPOV_X_C(bpm_channels[me->n])
                                                 : KARPOV_Y_C(bpm_channels[me->n]));

    me->c_h[0] = cda_add_physchan(sid, KARPOV_UX1_C(bpm_channels[me->n]));
    me->c_h[1] = cda_add_physchan(sid, KARPOV_UX2_C(bpm_channels[me->n]));
    me->c_h[2] = cda_add_physchan(sid, KARPOV_UY1_C(bpm_channels[me->n]));
    me->c_h[3] = cda_add_physchan(sid, KARPOV_UY2_C(bpm_channels[me->n]));
    
    return (XhWidget)w;
}

static void BPM_PROFILE_SetValue_m(knobinfo_t *ki __attribute__((unused)), double v __attribute__((unused)))
{
  BPM_PROFILE_privaterec_t *me = (BPM_PROFILE_privaterec_t*)ki->widget_private;
  int                       i;

#define PROFILE_SIMULATE 0

#if PROFILE_SIMULATE
    static int ctr = 0;
#endif
  
    for (i = 0;  i < 2;  i++)
    {
        me->v_o[i] = me->v_v[i];
        cda_getphyschanval(me->v_h[i],
                           &(me->v_v[i]),
                           &(me->v_t[i]),
                           &(me->v_f[i]));
        me->v_f[i] &=~ CXCF_FLAG_OTHEROP;
#if PROFILE_SIMULATE
        me->v_v[i] = sin((ctr + i*5) / 1.0) * me->maxmm / 2;
#endif
    }
    me->v_s = datatree_choose_knobstate(NULL,  me->v_f[0] | me->v_f[1]);
    me->v_u = datatree_choose_knobstate(NULL, (me->v_f[0] | me->v_f[1]) &~ CXRF_OVERLOAD);

    for (i = 0;  i < 4;  i++)
        cda_getphyschanval(me->c_h[i], &(me->c_v[i]), NULL, NULL);
    
#if PROFILE_SIMULATE
    ctr = (ctr + 1) & 0xFF;
#endif
    
    BPM_PROFILE_Draw(ki, True);
}

//////////////////////////////////////////////////////////////////////

knobs_vmt_t BPM_PROFILE_VMT = {BPM_PROFILE_Create_m, NULL, BPM_PROFILE_SetValue_m, NULL, NULL};
