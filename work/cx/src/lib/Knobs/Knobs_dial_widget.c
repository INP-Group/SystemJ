#include "Knobs_includes.h"
#include <math.h>

#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <Xm/DrawP.h>


enum {DEF_SIZE = 100};

enum
{
    DIAL_DIAL,
    DIAL_KNOB,
    DIAL_GAUGE
};

enum
{
    VALDPY_INPUT = 0,
    VALDPY_VALUE,
    VALDPY_NOVALUE
};

typedef struct
{
    int  size;
    int  valdpy;
    int  haslabel;
    int  is_square;
} dialopts_t;

typedef struct
{
    int           type;
    
    Widget        title;
    Widget        dial;
    Widget        val;
    Widget        inc_b;
    Widget        dec_b;
    GC            gc;
    GC            gc_ylw;
    GC            gc_red;
    double        curval;
    int           is_dragging;
    int           is_squaring;

    int           is_inc;
    XtIntervalId  timer;
    unsigned int  click_state;

    Boolean       hasfocus;
    
    Pixel         val_deffg;
    Pixel         val_defbg;
    int           are_defcols_got;

    XmFontList    tickFont;

    GC            background_GC;
    
    Dimension     shadow_thickness;
    
    Dimension     highlight_thickness;
    Pixel         highlight_color;
    GC            highlight_GC;

    Pixel         top_shadow_color;
    GC            top_shadow_GC;

    Pixel         bottom_shadow_color;
    GC            bottom_shadow_GC;

    Pixel         ring_color;
    GC            ring_GC;

    GC            yellow_GC;
    GC            red_GC;
} dial_privrec_t;


static void _RingColorDefault(Widget widget, int offset, XrmValue *value )
{
   XmeGetDefaultPixel (widget, XmBACKGROUND, offset, value);
}


static  XtResource dialRes[] =
{
    {
        "tickFontList",        XmCFontList,           XmRFontList,
        sizeof(XmFontList), XtOffsetOf(dial_privrec_t, tickFont),
        NULL, 0
    },
    {
        XmNhighlightThickness, XmCHighlightThickness, XmRHorizontalDimension,
        sizeof(Dimension),  XtOffsetOf(dial_privrec_t, highlight_thickness),
        XmRImmediate, (XtPointer) 2
    },
    {
        "ringBackground",   "RingBackground",         XmRPixel,
        sizeof(Pixel),      XtOffsetOf(dial_privrec_t, ring_color),
        XmRCallProc,  (XtPointer) _RingColorDefault
    },
};


enum
{
    INITIAL_DELAY = 250,
    REPEAT_DELAY  = 50
};


enum {HL_THICKNESS = 1}; // "XmNhighlightThickness" for dial

enum
{
    DRAW_CLEAR       = 1 << 0,
    DRAW_HIGHLIGHT   = 1 << 1,
    DRAW_DECORATIONS = 1 << 2,
    DRAW_VALUE       = 1 << 3,
    DRAW_EVERYTHING  = 0xFFFFFFFF &~ DRAW_CLEAR
};

static void ObtnDialGeometry(knobinfo_t *ki, int *x0_p, int *y0_p, int *r_p)
{
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  Dimension width, height;
    
    XtVaGetValues(me->dial,
                  XmNwidth,  &width,
                  XmNheight, &height,
                  NULL);
    
    *x0_p = (width - 1)  / 2;
    *y0_p = (height - 1) / 2;
    *r_p  = *y0_p - me->highlight_thickness;
}

static void CalcDialGeometry(Widget w, int *x0, int *y0, int *h2, int *v2)
{
  Dimension width, height;
    
    XtVaGetValues(w,
                  XmNwidth,  &width,
                  XmNheight, &height,
                  NULL);
    
    *x0 = (width - 1)  / 2;
    *y0 = (height - 1) / 2;
    *h2 = *x0 - HL_THICKNESS;
    *v2 = *y0 - HL_THICKNESS;
    if (*h2 > *v2) *h2 = *v2;
    if (*v2 > *h2) *v2 = *h2;
}

enum
{
    DEG_MINDISP  = -60,
    DEG_MAXDISP  = 240,
    DEG_DISPSIZE = DEG_MAXDISP - DEG_MINDISP + 1,
};
#define DEG2RAD (3.14159265 / 180.0)

#define MIN_OF(a,b)        (((int)(a) < (int)(b)) ? (a) : (b))
#define MAX_OF(a,b)        (((int)(a) > (int)(b)) ? (a) : (b))

/* Inspired by XmeDrawCircle() */
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

static void DialDraw(Widget w, knobinfo_t *ki, int parts)
{
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  
  Display      *dpy = XtDisplay(w);
  Window        win = XtWindow(w);
  GC            gc  = me->gc;

  unsigned int  hs = me->highlight_thickness;
  
  int           x0;
  int           y0;
  int           r;
  int           r_al;  // Arrow Short/Long
  int           r_as;
  int           r_tsi; // Tick Short/Long Inner/Outer
  int           r_tso;
  int           r_tli;
  int           r_tlo;

  int           t;
  int           degs;
  int           ri;
  int           ro;
  int           x1, y1, x2, y2;
  
  XPoint        arrow[4];
  double        min, max, cur;
  int           icur;
  
    ObtnDialGeometry(ki, &x0, &y0, &r);

    r_al      = r *  80 / 100;
    r_as      = r *  33 / 100;
    r_tsi     = r *  95 / 100;
    r_tso     = r * 100 / 100;
    r_tli     = r *  85 / 100;
    r_tlo     = r * 100 / 100;

    if (me->type == DIAL_KNOB)
    {
        r_tsi = r *  85 / 100;
        r_tso = r *  95 / 100;
    }
    
    if (parts & DRAW_CLEAR)
        XClearArea(dpy, win, 0, 0, 0, 0, False);

    if (parts & DRAW_DECORATIONS  &&  0)
    {
        XDrawRectangle(dpy, win,
                       me->bottom_shadow_GC,
                       x0 - r,
                       y0 - r,
                       r * 2 + 1,
                       r * 2 + 1);
        XDrawPoint(dpy, win, me->top_shadow_GC, x0 - r, y0 - r);
        XDrawPoint(dpy, win, me->top_shadow_GC, x0 + r, y0 - r);
        XDrawPoint(dpy, win, me->top_shadow_GC, x0 - r, y0 + r);
        XDrawPoint(dpy, win, me->top_shadow_GC, x0 + r, y0 + r);
    }
    
    if (parts & DRAW_DECORATIONS)
    {
        /* Ring/knob body */
        switch (me->type)
        {
            case DIAL_KNOB:
                ro = r_tli - 2;
                ri = ro *  66 / 100;
                DrawWideArc(dpy, win,
                            me->top_shadow_GC,
                            x0 - ro,
                            y0 - ro,
                            ro * 2 + 1,
                            ro * 2 + 1,
                            45*64, +180*64,
                            ro - ri + 1);
                DrawWideArc(dpy, win,
                            me->bottom_shadow_GC,
                            x0 - ro,
                            y0 - ro,
                            ro * 2 + 1,
                            ro * 2 + 1,
                            45*64, -180*64,
                            ro - ri + 1);
                break;
            
            default:
                XFillArc   (dpy, win,
                            me->ring_GC,
                            x0 - r,
                            y0 - r,
                            r * 2 + 1,
                            r * 2 + 1,
                            0, 360*64);
                DrawWideArc(dpy, win,
                            me->ring_GC,
                            x0 - r,
                            y0 - r,
                            r * 2 + 1,
                            r * 2 + 1,
                            0, 360*64,
                            1);
        }

        /* Red/yellow ranges */
#if 0
        DrawWideArc(dpy, win,
                    me->yellow_GC,
                    x0 - r_tlo,
                    y0 - r_tlo,
                    r_tlo * 2 + 1,
                    r_tlo * 2 + 1,
                    (180-DEG_MINDISP)*64, -(DEG_MAXDISP - DEG_MINDISP)*64,
//                    0*64, +20*64,
                    r_tlo - r_tli + 1);
#endif
        
        /* Ticks */
        for (t = 0;  t <= 20;  t++)
        {
            degs = DEG_MINDISP + t * (DEG_MAXDISP - DEG_MINDISP) / 20;
            
            ri = r_tsi;
            ro = r_tso;
            if ((t % 10) == 0)
            {
                ri = r_tli;
                ro = r_tlo;
            }
            
            x1 = x0 + (int) ((double) ro * cos(degs*DEG2RAD));
            y1 = y0 - (int) ((double) ro * sin(degs*DEG2RAD));
            x2 = x0 + (int) ((double) ri * cos(degs*DEG2RAD));
            y2 = y0 - (int) ((double) ri * sin(degs*DEG2RAD));
            
            XDrawLine(dpy, win, gc, x1, y1, x2, y2);
        }
    }

    if (parts & DRAW_VALUE)
    {
        min = ki->rmins[KNOBS_RANGE_DISP];
        max = ki->rmaxs[KNOBS_RANGE_DISP];
        cur = me->curval;
        if (cur < min) cur = min;
        if (cur > max) cur = max;
        icur = DEG_MAXDISP + (-DEG_DISPSIZE * (cur - min)) / (max - min);
        
        switch (me->type)
        {
            case DIAL_KNOB:
                ro = r_tli - 2;
                ri = ro *  66 / 100;
                XDrawLine(dpy, win, gc,
                          x0, y0,
                          x0 + (int)(ri * cos(icur*DEG2RAD)),
                          y0 - (int)(ri * sin(icur*DEG2RAD)));
                break;
            
            default:
                if (!(parts & DRAW_DECORATIONS))
                {
                    XFillArc   (dpy, win, me->ring_GC,
                                x0 - r_al,
                                y0 - r_al,
                                r_al * 2 + 1,
                                r_al * 2 + 1,
                                0, 360*64);
                    DrawWideArc(dpy, win, me->ring_GC,
                                x0 - r_al,
                                y0 - r_al,
                                r_al * 2 + 1,
                                r_al * 2 + 1,
                                0, 360*64,
                                1);
                }
                
                arrow[0].x = x0 + (int) (r_al * cos(icur * DEG2RAD));
                arrow[0].y = y0 - (int) (r_al * sin(icur * DEG2RAD));
                arrow[1].x = x0 + (int) (r_as * cos((icur+160) * DEG2RAD));
                arrow[1].y = y0 - (int) (r_as * sin((icur+160) * DEG2RAD));
                arrow[2].x = x0 + (int) (r_as * cos((icur-160) * DEG2RAD));
                arrow[2].y = y0 - (int) (r_as * sin((icur-160) * DEG2RAD));
                arrow[3].x = arrow[0].x;
                arrow[3].y = arrow[0].y;
                
                XFillPolygon(dpy, win, gc, arrow, 4, Convex, CoordModeOrigin);
                XDrawLines(dpy, win, gc, arrow, 4, CoordModeOrigin);
        }
    }

    if (parts & DRAW_HIGHLIGHT)
        DrawWideArc(dpy, win,
                    me->hasfocus? me->highlight_GC:((XmManagerWidget)(((XmPrimitiveWidget)w)->core.parent))->manager.background_GC,
                    x0 - r - hs,
                    y0 - r - hs,
                    (r + hs) * 2 + 1,
                    (r + hs) * 2 + 1,
                    0, 360*64,
                    hs);

}

static void DialExposeCB(Widget     w,
                         XtPointer  closure,
                         XtPointer  call_data  __attribute__((unused)))
{
  knobinfo_t *ki = (knobinfo_t *) closure;
  
    DialDraw(w, ki, DRAW_EVERYTHING);
}

static void FocusHandler(Widget     w,
                         XtPointer  closure,
                         XEvent    *event,
                         Boolean   *continue_to_dispatch __attribute__((unused)))
{
  XCrossingEvent    *cev         = (XCrossingEvent    *)event;
  XFocusChangeEvent *fev         = (XFocusChangeEvent *)event;
  knobinfo_t        *ki          = (knobinfo_t *)       closure;
  dial_privrec_t    *me          = (dial_privrec_t *)(ki->widget_private);
  Boolean            newhasfocus = me->hasfocus;

    switch (event->type)
    {
        case FocusIn:
            newhasfocus = True;
            break;

        case FocusOut:
            newhasfocus = False;
            break;
    }
  
    if (newhasfocus != me->hasfocus)
    {
////        fprintf(stderr, "focus: type=%d, mode=%d, %d=>%d\n", event->type, fev->mode, me->hasfocus, newhasfocus);
        me->hasfocus = newhasfocus;
        DialDraw(w, ki, DRAW_HIGHLIGHT);
    }
}

static void UpdateGC(knobinfo_t *ki, Pixel fg)
{
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  GC         gc;
  XGCValues  vals;
    
    if (me->gc != NULL)
    {
        XtReleaseGC(me->dial, me->gc);
        me->gc = NULL;
    }

    vals.foreground = fg;
    gc = XtGetGC(me->dial, GCForeground, &vals);
    if (gc != NULL) me->gc = gc;
}

static void IncDec(knobinfo_t *ki, int action_inc, unsigned int mod_state)
{
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  
  int           factor;
  double        delta;
  double        v = me->curval;

    factor = 100;
    if (mod_state & MOD_FINEGRAIN_MASK) factor = 10;
    if (mod_state & MOD_BIGSTEPS_MASK)  factor = 1000;
    delta = (ki->incdec_step * factor) / 100;
  
    if (action_inc)
        v += delta;
    else
        v -= delta;
    
    SetControlValue(ki, v, 1);
}

static void ArrowsHandler(Widget     w,
                          XtPointer  closure,
                          XEvent    *event,
                          Boolean   *continue_to_dispatch __attribute__((unused)))
{
  XKeyEvent    *ev = (XKeyEvent *)  event;
  knobinfo_t   *ki = (knobinfo_t *) closure;
  KeySym        ks;
  int           is_inc;
  
    if (event->type != KeyPress) return;
    ks = XKeycodeToKeysym(XtDisplay(w), ev->keycode, 0);
    
    if      (ks == XK_Down    ||  ks == XK_Left  ||
             ks == XK_KP_Down ||  ks == XK_KP_Left)
        is_inc = 0;
    else if (ks == XK_Up      ||  ks == XK_Right    ||
             ks == XK_KP_Up   ||  ks == XK_KP_Right)
        is_inc = 1;
    else return;

    IncDec(ki, is_inc, ev->state);
}

static void TimerProc(XtPointer closure,
                      XtIntervalId *id __attribute__((unused)))
{
  knobinfo_t *ki   = (knobinfo_t *) closure;
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  
  
    /* We use timer:=0 as a "stop" flag */
    if (me->timer == 0) return;

    /* Purge just-had-shot timer ID */
    me->timer = 0;

    /* Trigger action... */
    IncDec(ki, me->is_inc, me->click_state);
    
    /* Schedule next shot */
    me->timer =
        XtAppAddTimeOut(XtWidgetToApplicationContext(me->dial),
                        REPEAT_DELAY,
                        TimerProc,
                        (XtPointer)ki);
}

static void ArmAction(int is_inc, XtPointer closure, XtPointer call_data)
{
  knobinfo_t                  *ki   = (knobinfo_t *)                  closure;
  dial_privrec_t              *me   = (dial_privrec_t *)(ki->widget_private);
  XmArrowButtonCallbackStruct *info = (XmArrowButtonCallbackStruct *) call_data;
  XButtonEvent                *ev   = (XButtonEvent *)                (info->event);
  
    if (ev->type != ButtonPress) return;

    /* Pass focus to the widget */
    XmProcessTraversal(me->dial, XmTRAVERSE_CURRENT);
    
    /* Remember the action for future repetitions */
    me->is_inc = is_inc;
    
    /* Trigger action */
    IncDec(ki, is_inc, ev->state);

    /* And initiate autorepeat */
    if (me->timer == 0)
        me->timer =
            XtAppAddTimeOut(XtWidgetToApplicationContext(me->dial),
                            INITIAL_DELAY,
                            TimerProc,
                            (XtPointer)ki);
    me->click_state = ev->state;
}

static void DecArmCB(Widget     w          __attribute__((unused)),
                     XtPointer  closure,
                     XtPointer  call_data  __attribute__((unused)))
{
    ArmAction(0, closure, call_data);
}

static void IncArmCB(Widget     w          __attribute__((unused)),
                     XtPointer  closure,
                     XtPointer  call_data  __attribute__((unused)))
{
    ArmAction(1, closure, call_data);
}

static void DisarmCB(Widget     w          __attribute__((unused)),
                     XtPointer  closure,
                     XtPointer  call_data  __attribute__((unused)))
{
  knobinfo_t *ki   = (knobinfo_t *) closure;
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  
  
    /* Stop autorepeat */
    if (me->timer != 0)
    {
        XtRemoveTimeOut(me->timer);
        me->timer = 0;
    }
}

static void MouseHandler(Widget w,
                         XtPointer  closure,
                         XEvent    *event,
                         Boolean   *continue_to_dispatch __attribute__((unused)))
{
  XButtonEvent        *pev  = (XButtonEvent *) event;
  XMotionEvent        *mev  = (XMotionEvent *) event;
  knobinfo_t          *ki   = (knobinfo_t *)   closure;
  dial_privrec_t      *me = (dial_privrec_t *)(ki->widget_private);
  int                  x, y;
  int                  x0, y0, h2, v2;
  int                  dx, dy;
  double               angle;
  double               v;

#define RAD2DEG (180.0 / 3.14159265)
  
    /* Check if this event doesn't interest us */
    if (((pev->type == ButtonPress  ||  pev->type == ButtonRelease)  &&  pev->button != 1)
        ||
        (mev->type == MotionNotify  &&  !me->is_dragging))
        return;
  
    /* In fact, these two event structs are identical down to "state" inclusive, but I prefer to be "C-correct" :-) */
    if (mev->type == MotionNotify)
    {
        x = mev->x;
        y = mev->y;
    }
    else
    {
        x = pev->x;
        y = pev->y;
    }
    
    /* Should we begin dragging? */
    if (pev->type == ButtonPress)
        me->is_dragging = 1;

    /* Okay, now handle those coords */

    CalcDialGeometry(w, &x0, &y0, &h2, &v2);
    dx = x - x0;  dy = y0 - y;  /* Note: +dy is *up* */

    /* If too close to center, do nothing (to avoid 'spazzing') */
    if (abs(dx) < 3  &&  abs(dy) < 3) goto SKIP;

    /* figure out angle of vector dx,dy */
    if (dx==0) {     /* special case */
        if (dy>0) angle =  90.0;
        else      angle = -90.0;
    }
    else if (dx>0) angle = atan((double)  dy / (double)  dx) * RAD2DEG;
    else           angle = atan((double) -dy / (double) -dx) * RAD2DEG + 180.0;
    
    /* map angle into range: -90..270, then into to value */
    if (angle > 270.0) angle -= 360.0;
    if (angle < -90.0) angle += 360.0;

    /* Force angle into displayed sector */
    if (angle < DEG_MINDISP) angle = DEG_MINDISP;
    if (angle > DEG_MAXDISP) angle = DEG_MAXDISP;
    
    v = (
         (ki->rmaxs[KNOBS_RANGE_DISP] - ki->rmins[KNOBS_RANGE_DISP])
         *
         ((double)DEG_MAXDISP - angle) / (double)DEG_DISPSIZE
        )
        + ki->rmins[KNOBS_RANGE_DISP];

    SetControlValue(ki, v, 1);

 SKIP:
    /* Should we end dragging? */
    if (pev->type == ButtonRelease)
        me->is_dragging = 0;
}

static void DialResizeCB(Widget     w,
                         XtPointer  closure,
                         XtPointer  call_data __attribute__((unused)))
{
  Dimension            width;
  knobinfo_t          *ki   = (knobinfo_t *)   closure;
  dial_privrec_t      *me = (dial_privrec_t *)(ki->widget_private);

    if (me->is_squaring > 5) return;
  
    XtVaGetValues(w, XmNwidth,  &width, NULL);
    me->is_squaring++;
    XtVaSetValues(w, XmNheight, width,  NULL);
    me->is_squaring--;
}

static psp_paramdescr_t text2dialopts[] =
{
    PSP_P_INT   ("size",        dialopts_t, size,      DEF_SIZE, 20, 1000),
    
    PSP_P_FLAG  ("input",       dialopts_t, valdpy,    VALDPY_INPUT,     1),
    PSP_P_FLAG  ("value",       dialopts_t, valdpy,    VALDPY_VALUE,     0),
    PSP_P_FLAG  ("novalue",     dialopts_t, valdpy,    VALDPY_NOVALUE,   0),
    
    PSP_P_FLAG  ("withlabel",   dialopts_t, haslabel,  1,                1),
    PSP_P_FLAG  ("nolabel",     dialopts_t, haslabel,  0,                0),

    PSP_P_FLAG  ("square",      dialopts_t, is_square, 1,                1),
    PSP_P_FLAG  ("=",           dialopts_t, is_square, 0,                0),
    
    PSP_P_END()
};

static GC GetPixmapBasedGC(Widget w,
                           Pixel foreground,
                           Pixel background,
                           Pixmap pixmap)
{
    XGCValues values;
    XtGCMask  valueMask;
    
    valueMask = GCForeground | GCBackground;
    values.foreground = foreground;
    values.background = background;
    
   return (XtGetGC (w, valueMask, &values));
}

static XhWidget DoCreate(knobinfo_t *ki, XhWidget parent, int type)
{
  Widget      container;

  dialopts_t  opts;

  XmString    s;
  Dimension   fst;
  
  Dimension   horz_offset;
  Dimension   vert_offset;
  Dimension   to_skip;
  Widget      prev;
  Dimension   val_hgt;
  Dimension   val_wid;

  dial_privrec_t *me;
  Pixel       fg, bg;

  char       *ls;
  
    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(dial_privrec_t))) == NULL)
        return NULL;
    me = (dial_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    me->type = type;
    
    /* Parse parameters from widdepinfo */
    if (ParseWiddepinfo(ki, &opts, sizeof(opts), text2dialopts) != PSP_R_OK)
    {
        opts.size = DEF_SIZE;
    }

    if (!ki->is_rw  &&  opts.valdpy == VALDPY_INPUT) opts.valdpy = VALDPY_VALUE;

    /* Obtain our private resources */
    XtGetSubresources(CNCRTZE(parent), (XtPointer) me,
                      "dialDial", "XmDrawingArea",
                      dialRes, XtNumber(dialRes),
                      NULL, 0);
    
    /* Create the container... */
    container = XtVaCreateManagedWidget("dial",
                                   xmFormWidgetClass,
                                   CNCRTZE(parent),
                                   XmNtraversalOn,  ki->is_rw,
                                   NULL);
    XtVaGetValues(container, XmNshadowThickness, &fst, NULL);
    
    horz_offset = fst;
    vert_offset = fst;
    
    /* The label... */
    to_skip = vert_offset;
    prev = NULL;
    if (opts.haslabel  &&  get_knob_label(ki, &ls))
    {
        me->title = XtVaCreateManagedWidget("dialTitle",
                                            xmLabelWidgetClass,
                                            container,
                                            XmNlabelString,     s = XmStringCreateLtoR(ls, xh_charset),
                                            XmNrecomputeSize,   False,
                                            XmNmarginWidth,     0,
                                            XmNmarginHeight,    0,
                                            XmNalignment,       XmALIGNMENT_CENTER,
                                            XmNleftAttachment,  XmATTACH_FORM,
                                            XmNleftOffset,      horz_offset,
                                            XmNrightAttachment, XmATTACH_FORM,
                                            XmNrightOffset,     horz_offset,
                                            XmNtopAttachment,   XmATTACH_FORM,
                                            XmNtopOffset,       to_skip,
                                            NULL);
        XmStringFree(s);
        to_skip = 0;
        prev = me->title;
    }

    /* The dial area... */
    me->dial = XtVaCreateManagedWidget("dialDial",
                                       xmDrawingAreaWidgetClass,
                                       container,
                                       XmNtraversalOn,     ki->is_rw,
                                       XmNwidth,           opts.size,
                                       XmNheight,          opts.size,
                                       XmNleftAttachment,  XmATTACH_FORM,
                                       XmNleftOffset,      horz_offset,
                                       XmNrightAttachment, XmATTACH_FORM,
                                       XmNrightOffset,     horz_offset,
                                       XmNtopAttachment,   XmATTACH_WIDGET,
                                       XmNtopWidget,       prev,
                                       XmNtopOffset,       to_skip,
                                       NULL);
    XtAddCallback(me->dial, XmNexposeCallback,
                  DialExposeCB, (XtPointer) ki);
    if (ki->is_rw)
    {
        XtAddEventHandler(me->dial, FocusChangeMask|EnterWindowMask|LeaveWindowMask,     False, FocusHandler,  (XtPointer)ki);
        XtAddEventHandler(me->dial, KeyPressMask,                                        False, ArrowsHandler, (XtPointer)ki);
        XtAddEventHandler(me->dial, ButtonPressMask|ButtonReleaseMask|Button1MotionMask, False, MouseHandler,  (XtPointer)ki);
    }
    if (opts.is_square  &&  0)
    {
        XtAddCallback(me->dial, XmNresizeCallback,
                      DialResizeCB, (XtPointer) ki);
        ki->behaviour |= KNOB_B_HALIGN_FILL;
    }

    XtVaGetValues(me->dial,
                  XmNbackground,        &bg,
                  XmNshadowThickness,   &(me->shadow_thickness),
                  XmNhighlightColor,    &(me->highlight_color),
                  XmNtopShadowColor,    &(me->top_shadow_color),
                  XmNbottomShadowColor, &(me->bottom_shadow_color),
                  NULL);
    me->background_GC    = GetPixmapBasedGC(me->dial, bg,                        bg, XmUNSPECIFIED_PIXMAP);
    me->highlight_GC     = GetPixmapBasedGC(me->dial, me->highlight_color,     bg, XmUNSPECIFIED_PIXMAP);
    me->top_shadow_GC    = GetPixmapBasedGC(me->dial, me->top_shadow_color,    bg, XmUNSPECIFIED_PIXMAP);
    me->bottom_shadow_GC = GetPixmapBasedGC(me->dial, me->bottom_shadow_color, bg, XmUNSPECIFIED_PIXMAP);
    me->ring_GC          = GetPixmapBasedGC(me->dial, me->ring_color,          bg, XmUNSPECIFIED_PIXMAP);
    ChooseWColors(ki, COLALARM_YELLOW, &fg, &bg);
    me->yellow_GC        = GetPixmapBasedGC(me->dial, bg,                        bg, XmUNSPECIFIED_PIXMAP);
    ChooseWColors(ki, COLALARM_RED,    &fg, &bg);
    me->red_GC           = GetPixmapBasedGC(me->dial, bg,                        bg, XmUNSPECIFIED_PIXMAP);
    
    /* The value indicator... */
    switch (opts.valdpy)
    {
        case VALDPY_INPUT:
            me->val = CreateTextInput(ki, container);
            break;
        
        case VALDPY_NOVALUE:
            /* No display widget */
            break;
            
        default: /* VALDPY_VALUE */
            me->val = CreateTextValue(ki, container);
    }
    if (me->val != NULL)
        XtVaSetValues(me->val,
                      XmNmarginWidth,      0,
                      XmNmarginHeight,     0,
                      NULL);

    if (me->val != NULL  &&  ki->is_rw)
    {
        /* Left arrow... */
        me->dec_b = XtVaCreateManagedWidget("dialButton",
                                            xmArrowButtonWidgetClass,
                                            container,
                                            XmNtraversalOn,      False,
                                            XmNarrowDirection,   XmARROW_LEFT,
                                            NULL);
        XtAddCallback(me->dec_b, XmNarmCallback,    DecArmCB, (XtPointer)ki);
        XtAddCallback(me->dec_b, XmNdisarmCallback, DisarmCB, (XtPointer)ki);
        
        /* Right arrow... */
        me->inc_b = XtVaCreateManagedWidget("dialButton",
                                            xmArrowButtonWidgetClass,
                                            container,
                                            XmNtraversalOn,      False,
                                            XmNarrowDirection,   XmARROW_RIGHT,
                                            NULL);
        XtAddCallback(me->inc_b, XmNarmCallback,    IncArmCB, (XtPointer)ki);
        XtAddCallback(me->inc_b, XmNdisarmCallback, DisarmCB, (XtPointer)ki);
    }

    if (me->title != NULL) HookPropsWindow(ki, me->title);
                           HookPropsWindow(ki, me->dial);
    if (me->val   != NULL) HookPropsWindow(ki, me->val);
    if (me->dec_b != NULL) HookPropsWindow(ki, me->dec_b);
    if (me->inc_b != NULL) HookPropsWindow(ki, me->inc_b);

    /* Attach bottom line of widgets */
    if (me->val != NULL)
    {
        XtVaSetValues(me->val,
                      XmNtopAttachment,    XmATTACH_WIDGET,
                      XmNtopWidget,        me->dial,
                      XmNbottomAttachment, XmATTACH_FORM,
                      XmNbottomOffset,     vert_offset,
                      NULL);
        XtVaGetValues(me->val,
                      XmNwidth,  &val_wid,
                      XmNheight, &val_hgt,
                      NULL);
        
        if (me->dec_b != NULL  &&  me->inc_b != NULL)
        {
            XtVaGetValues(me->val, XmNheight, &val_hgt, NULL);
            XtVaSetValues(me->dec_b,
                          XmNtopAttachment,    XmATTACH_WIDGET,
                          XmNtopWidget,        me->dial,
                          XmNleftAttachment,   XmATTACH_FORM,
                          XmNleftOffset,       horz_offset,
                          XmNheight,           val_hgt,
                          NULL);
            XtVaSetValues(me->inc_b,
                          XmNtopAttachment,    XmATTACH_WIDGET,
                          XmNtopWidget,        me->dial,
                          XmNrightAttachment,  XmATTACH_FORM,
                          XmNrightOffset,      horz_offset,
                          XmNheight,           val_hgt,
                          NULL);
#if 0
            XtVaSetValues(me->val,
                          XmNleftAttachment,   XmATTACH_WIDGET,
                          XmNleftWidget,       me->dec_b,
                          XmNrightAttachment,  XmATTACH_WIDGET,
                          XmNrightWidget,      me->inc_b,
                          NULL);
        }
        else
        {
            XtVaSetValues(me->val,
                          XmNleftAttachment,  XmATTACH_FORM,
                          XmNleftOffset,      horz_offset,
                          XmNrightAttachment, XmATTACH_FORM,
                          XmNrightOffset,     horz_offset,
                          NULL);
#endif
        }
        XtVaSetValues(me->val,
                      XmNleftAttachment, XmATTACH_POSITION,
                      XmNleftPosition,   50,
                      XmNleftOffset,     -val_wid / 2,
                      NULL);
    }
    else
        XtVaSetValues(me->dial,
                      XmNbottomAttachment, XmATTACH_FORM,
                      XmNbottomOffset,     vert_offset,
                      NULL);
    
    return ABSTRZE(container);
}

static void SetValue_m(knobinfo_t *ki, double v)
{
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  double  old_v = me->curval;
    
    me->curval = v;
    if (v != old_v) DialDraw(me->dial, ki, DRAW_VALUE);
    
    if (me->val != NULL)
        SetTextString(ki, me->val, v);
}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
  dial_privrec_t *me = (dial_privrec_t *)(ki->widget_private);
  Pixel         fg;
  Pixel         bg;
  
    ChooseWColors(ki, newstate, &fg, &bg);
    
                             XtVaSetValues(CNCRTZE(ki->indicator),
                                                          XmNforeground, fg, XmNbackground, bg, NULL);
    if (me->title != NULL) XtVaSetValues(me->title,   XmNforeground, fg, XmNbackground, bg, NULL);
                           XtVaSetValues(me->dial,    XmNforeground, fg, XmNbackground, bg, NULL);
    if (me->dec_b != NULL) XtVaSetValues(me->dec_b,   XmNforeground, fg, XmNbackground, bg, NULL);
    if (me->inc_b != NULL) XtVaSetValues(me->inc_b,   XmNforeground, fg, XmNbackground, bg, NULL);
    
    UpdateGC(ki, fg);

    if (me->val != NULL)
    {
        if (!me->are_defcols_got)
        {
            XtVaGetValues(me->val,
                          XmNforeground, &(me->val_deffg),
                          XmNbackground, &(me->val_defbg),
                          NULL);
            me->are_defcols_got = 1;
        }
        
        ChooseKnobColors(ki->color, newstate,
                         me->val_deffg, me->val_defbg,
                         &fg, &bg);
        XtVaSetValues(me->val,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }
}

static XhWidget DIAL_Create_m (knobinfo_t *ki, XhWidget parent)
{return DoCreate(ki, parent, DIAL_DIAL);}

static XhWidget KNOB_Create_m (knobinfo_t *ki, XhWidget parent)
{return DoCreate(ki, parent, DIAL_KNOB);}

static XhWidget GAUGE_Create_m(knobinfo_t *ki, XhWidget parent)
{return DoCreate(ki, parent, DIAL_GAUGE);}


knobs_vmt_t KNOBS_DIAL_VMT  = {DIAL_Create_m,  NULL, SetValue_m, Colorize_m, NULL};
knobs_vmt_t KNOBS_KNOB_VMT  = {KNOB_Create_m,  NULL, SetValue_m, Colorize_m, NULL};
knobs_vmt_t KNOBS_GAUGE_VMT = {GAUGE_Create_m, NULL, SetValue_m, Colorize_m, NULL};
