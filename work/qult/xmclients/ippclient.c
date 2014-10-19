#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>

#include "misclib.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "Cdr.h"
#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Chl.h"
#include "Knobs.h"
#include "KnobsI.h"

#include "drv_i/u0632_drv_i.h"
#include "ippclient.h"

#include "pixmaps/btn_quit.xpm"
#include "pixmaps/btn_help.xpm"

#include "pixmaps/btn_save.xpm"
#include "pixmaps/btn_getbkgd.xpm"
#include "pixmaps/btn_rstbkgd.xpm"
#include "pixmaps/btn_pause.xpm"


/*#### _defines ####################################################*/

static double factor[4] = {61e-15, 15.3e-15, 3.81e-15, 0.953e-15};

static double r2q[4] = {5*400.00e3, 5*100.00e3, 5*25.00e3, 5*6.25e3};

enum {NUM_BKGD_STEPS = 5};


/*#### _globals ####################################################*/

static int bkgd_ctl = -1; /* =0 - normal operation, <0 - bkgd off, >0 - bkgd step N */


/*####  ####*/

#include "Knobs_typesP.h"
#include "Xh_globals.h"

#define CNCRTZE(w) ((Widget)w)
#define ABSTRZE(w) ((XhWidget)w)


static XFontStruct     *last_finfo;

static GC AllocOneGC(XhWidget w, XhPixel fg, XhPixel bg, const char *fontspec)
{
  XtGCMask   mask;
  XGCValues  vals;
    
    mask            = GCFunction | GCGraphicsExposures | GCForeground | GCBackground | GCLineWidth;
    vals.function   = GXcopy;
    vals.graphics_exposures = False;
    vals.foreground = fg;
    vals.background = bg;
    vals.line_width = 0;

    if (fontspec != NULL  &&  fontspec[0] != '\0')
    {
        last_finfo = XLoadQueryFont(XtDisplay(CNCRTZE(w)), fontspec);
        if (last_finfo == NULL)
        {
            fprintf(stderr, "Unable to load font \"%s\"; using \"fixed\"\n", fontspec);
            last_finfo = XLoadQueryFont(XtDisplay(CNCRTZE(w)), "fixed");
        }
        vals.font = last_finfo->fid;

        mask |= GCFont;
    }

    return XtGetGC(w, mask, &vals);
}

static void DrawMultilineString(Display *dpy, Drawable d,
                                GC gc, XFontStruct *finfo,
                                int x, int y,
                                const char *string)
{
  const char *p;
  const char *endp;
  size_t      len;
    
    for (p = string;  *p != '\0';)
    {
        endp = strchr(p, '\n');
        if (endp != NULL) len = endp - p;
        else              len = strlen(p);
        
        XDrawImageString(dpy, d, gc, x, y + finfo->ascent, p, len);
        
        y += finfo->ascent + finfo->descent;
        p += len;
        if (endp != NULL) p++;
    }
}

typedef struct _ipp_privrec_t_struct *ipp_rec_p_t;

typedef void (*ipp_drawer_t)(ipp_rec_p_t me);

enum {MAX_CHANS_PER_BOXELEM = 32};

typedef struct _ipp_privrec_t_struct
{
    ElemInfo              e; // A back-reference
    const char           *ident;

    GC                    bkgdGC;
    GC                    axisGC;
    GC                    reprGC;
    GC                    prevGC;
    GC                    outlGC;
    GC                    dat1GC[COLALARM_LAST + 1];
    GC                    dat2GC[COLALARM_LAST + 1];
    GC                    textGC;
    XFontStruct          *text_finfo;
    
    Widget                hgrm;
    Pixmap                pix;
    int                   wid;
    int                   hei;
    ipp_drawer_t          do_draw;
    int                   magn;

    int                   numchans;
    cda_physchanhandle_t  v_h   [MAX_CHANS_PER_BOXELEM];  // Handles
    double                v_v[2][MAX_CHANS_PER_BOXELEM];  // Values
    tag_t                 v_t[2][MAX_CHANS_PER_BOXELEM];  // Tags
    rflags_t              v_f[2][MAX_CHANS_PER_BOXELEM];  // Flags
    colalarm_t            v_s[2][MAX_CHANS_PER_BOXELEM];  // Statuses
    double                v_b[MAX_CHANS_PER_BOXELEM];     // Background
    double                v_a[NUM_BKGD_STEPS][MAX_CHANS_PER_BOXELEM];  // Accumulation-for-background

    Knob                  i_ki;
    Knob                  r_ki;
    
    int                   b_gmin;
    int                   b_gmax;
} ipp_privrec_t;

ipp_privrec_t *ipp_recs_array[50]; // :-(
int            ipp_recs_count = 0; // :-(

typedef void (*magn_change_cb_t)(ipp_privrec_t *me, int new_magn);
typedef struct
{
    uint32            magicnumber;
    int               magn;
    magn_change_cb_t  cb;
    ipp_privrec_t    *me;
} magn_privrec_t;


enum
{
    NUM_COLS = 15,
    NUM_ROWS = 15,
    PIXELS_PER_COL = 10,
    PIXELS_PER_ROW = 10*0+9,
    COL_OF_0 = 7,
    ROW_OF_0 = 7,

    GRF_WIDTH  = PIXELS_PER_COL * NUM_COLS,
    GRF_HEIGHT = PIXELS_PER_ROW * NUM_ROWS,
    GRF_SPACING = 10,

    GRF_FULL_WIDTH  = GRF_WIDTH + GRF_SPACING + GRF_WIDTH,
    GRF_FULL_HEIGHT = GRF_HEIGHT
};

static int  magn_factors[] = {1, 2, 4, 10, 20, 50, 100, 200, 1000, 5000};

static void DoDraw(ipp_privrec_t *me, int do_refresh)
{
  Display *dpy = XtDisplay(me->hgrm);
  Window   win = XtWindow (me->hgrm);

    if (me->pix != 0)
    {
        XFillRectangle(dpy, me->pix, me->bkgdGC, 0, 0, me->wid, me->hei);
        me->do_draw(me);
        if (do_refresh)
            XCopyArea(dpy, me->pix, win,
                      me->bkgdGC,
                      0, 0, me->wid, me->hei,
                      0, 0);
    }
    else
    {
        if (win == 0  ||  !do_refresh) return;
        XFillRectangle(dpy, win, me->bkgdGC, 0, 0, me->wid, me->hei);
        me->do_draw(me);
    }
}

//////////////////////////////////////////////////////////////////////

static void DrawHist(Display *dpy, Drawable d,
                     int numbars, double vals[], colalarm_t states[],
                     int x0, int pix_per_col, int magn,
                     GC posGC[], GC negGC[], GC outlGC, GC prevGC, int is_prev)
{
  int         i;
  colalarm_t  state;
  GC          barGC;

  int         hgt;
  int         x1, y1, x2, y2;
  int         y0 = ROW_OF_0 * PIXELS_PER_ROW + (PIXELS_PER_ROW / 2);
  
    for (i = 0;  i < numbars;  i++)
    {
        hgt = (vals[i] * magn) * (ROW_OF_0 * PIXELS_PER_ROW) / (1 << 13);
        
        x1 = x0 + i * pix_per_col;
        x2 = x1 + pix_per_col - 1;

        state = states[i];
        /* Is overflow? */
        if (vals[i] <= -8191  ||  vals[i] >= +8191) state = COLALARM_HWERR;

        barGC = (vals[i] > 0? posGC : negGC)[state];
        
        if (state != COLALARM_NONE  &&  state != 0)
        {
            y1 = 0;
            y2 = GRF_HEIGHT - 1;
        }
        else if (hgt >= 0)
        {
            y1 = y0 - hgt;
            y2 = y0;
        }
        else
        {
            y1 = y0;
            y2 = y0 - hgt;
        }

        if (is_prev) barGC = prevGC;
        
        XFillRectangle(dpy, d, barGC,
                       x1, y1,
                       x2 - x1 + 1, y2 - y1 + 1);
        
        XDrawRectangle(dpy, d, outlGC,
                       x1, y1,
                       x2 - x1 + 0, y2 - y1 + 0);  /*!!! Damn!!!  For some strange reason width/height=1 means 2(!!!) pixels!!! */
    }
}

static void IPP_XY_Draw(ipp_privrec_t *me)
{
  Display  *dpy = XtDisplay(me->hgrm);
  Drawable  drw = me->pix != 0? me->pix : XtWindow(me->hgrm);
  int       dim;
  int       gen;
  int       xg, yg;
  int       x0, y0;

  int       sum;
  int       i;
  char      buf[100];

    for (dim = 0;  dim < 2;  dim++)
    {
        /* Calc geometry */
        xg = (GRF_WIDTH + GRF_SPACING) * dim;
        yg = 0;
        x0 = COL_OF_0 * PIXELS_PER_COL + (PIXELS_PER_COL / 2) + xg;
        y0 = ROW_OF_0 * PIXELS_PER_ROW + (PIXELS_PER_ROW / 2) + yg;

        /* Draw axis */
        XDrawLine(dpy, drw, me->axisGC,
                  xg, y0, xg + GRF_WIDTH - 1, y0);
        XDrawLine(dpy, drw, me->axisGC,
                  x0, yg, x0, yg + GRF_HEIGHT - 1);

        /* Draw histograms.
           Generations are processed from 1 to 0 (from oldest to newest) --
           for correct display order */
        for (gen = 1;  gen >= 0;  gen--)
        {
            DrawHist(dpy, drw,
                     NUM_COLS, &(me->v_v[gen][NUM_COLS*dim]), &(me->v_s[gen][NUM_COLS*dim]),
                     xg, PIXELS_PER_COL, me->magn,
                     me->dat1GC, me->dat1GC, me->outlGC, me->prevGC, gen);
        }

        /* Display stats */
        for (sum = 0, i = 0;  i < NUM_COLS;  i++)
            sum += me->v_v[0][NUM_COLS*dim + i];
        snprintf(buf, sizeof(buf), "% d", sum);
        XDrawString(dpy, drw, me->textGC,
                    1 + xg, 1 + me->text_finfo->ascent,
                    buf, strlen(buf));
    }
}

//////////////////////////////////////////////////////////////////////

enum
{
    ESPECTR_NUM_CHANS = 30
};

// Electron Charge 
#define eQ      (1.62e-19) 
// Input Line Capacity 
#define CAmp    (5.7e-9) 

static inline double Energy_Mev(double ISpectr, int i)
{
    return (8.1e-4 * ISpectr * (3.7 + (i + ESPECTR_NUM_CHANS) * 65. / 59.));
}

static void MouseHandler(Widget     w                    __attribute__((unused)),
                         XtPointer  closure,
                         XEvent    *event,
                         Boolean   *continue_to_dispatch __attribute__((unused)))
{
  ipp_privrec_t *me  = (ipp_privrec_t *) closure;
  XMotionEvent  *mev = (XMotionEvent *)  event;
  XButtonEvent  *bev = (XButtonEvent *)  event;
  
  int            x;
  int            b;
  int            touch_min = 0;
  int            touch_max = 0;

    if (event->type == MotionNotify)
    {
        x = mev->x;
        if (mev->state & Button1Mask) touch_min = 1;
        if (mev->state & Button3Mask) touch_max = 1;
    }
    else
    {
        x = bev->x;
        if (bev->button == Button1) touch_min = 1;
        if (bev->button == Button3) touch_max = 1;
    }
  
    b = x / PIXELS_PER_COL;
    if (b < 0)                   b = 0;
    if (b > ESPECTR_NUM_CHANS-1) b = ESPECTR_NUM_CHANS-1;
    
    if      (touch_min  &&  touch_max)
        me->b_gmin = me->b_gmax = b;
    else if (touch_min)
    {
        if (b > me->b_gmax) b = me->b_gmax;
        me->b_gmin = b;
    }
    else if (touch_max)
    {
        if (b < me->b_gmin) b = me->b_gmin;
        me->b_gmax = b;
    }
    else return;

    DoDraw(me, True);
}

static void IPP_ESPECTR_Draw(ipp_privrec_t *me)
{
  Display  *dpy = XtDisplay(me->hgrm);
  Drawable  drw = me->pix != 0? me->pix : XtWindow(me->hgrm);
  int       gen;
  int       x;
  
  double    curI;
  int       R_N;

  int       sum;
  double    num_sum;
  double    energy_sum;
  double    gate_num_sum;
  double    gate_energy_sum;
  
  int       i;
  double    v;
  double    count;
  double    mevs;

  char      buf[200];
  Dimension  I_w;
  Dimension  I_h;

    curI = 0;  if (me->i_ki != NULL) curI = me->i_ki->curv;
    R_N  = 0;  if (me->r_ki != NULL) R_N  = me->r_ki->curv;

    for (gen = 1;  gen >= 0;  gen--)
    {
        DrawHist(dpy, drw,
                 ESPECTR_NUM_CHANS, &(me->v_v[gen][0]), &(me->v_s[gen][0]),
                 0, PIXELS_PER_COL, me->magn,
                 me->dat1GC, me->dat2GC, me->outlGC, me->prevGC, gen);
    }
    x = me->b_gmin * PIXELS_PER_COL;
    XDrawLine(dpy, drw, me->reprGC, x, 0, x, GRF_FULL_HEIGHT-1);
    x = me->b_gmax * PIXELS_PER_COL + (PIXELS_PER_COL-1);
    XDrawLine(dpy, drw, me->reprGC, x, 0, x, GRF_FULL_HEIGHT-1);

    for (i = 0, sum = 0, energy_sum = 0.0, num_sum = 0.0, gate_num_sum = 0.0, gate_energy_sum = 0.0;
         i < ESPECTR_NUM_CHANS;
         i++)
    {
        v = me->v_v[0][i];
        count = v * r2q[R_N];
        mevs  = v * r2q[R_N] * Energy_Mev(curI, i);

        sum        += v;
        num_sum    += count;
        energy_sum += mevs;

        if (i >= me->b_gmin  &&  i <= me->b_gmax)
        {
            gate_num_sum    += count;
            gate_energy_sum += mevs;
        }
    }

    if (me->e != NULL  &&  me->e->count >= 4)
        XtVaGetValues(me->e->content[3].indicator,
                      XmNwidth,  &I_w,
                      XmNheight, &I_h,
                      NULL);
    else I_w = I_h = 0;
#if 1
    sprintf(buf, "N=% .2e\n[%.2f,%.2f]N=%.2e",
            num_sum,
            Energy_Mev(curI, me->b_gmin), Energy_Mev(curI, me->b_gmax),
            gate_num_sum);
    DrawMultilineString(dpy, drw, me->textGC, me->text_finfo,
                        0, I_h, buf);
#else
    sprintf(buf, " "/*"%6d "*/"N=% .2e"/*" E=%.2e"*/, /*sum, */num_sum/*, energy_sum*/);
    DrawMultilineString(dpy, drw, me->textGC, me->text_finfo,
                        I_w, 0, buf);
    sprintf(buf, " Gate [%.2f,%.2f]:\n N=%.2e"/*" E=%.2e"*/,
            Energy_Mev(curI, me->b_gmin), Energy_Mev(curI, me->b_gmax),
            gate_num_sum/*, gate_energy_sum*/);
    DrawMultilineString(dpy, drw, me->textGC, me->text_finfo,
                        0, I_h, buf);
#endif
    
    /////SetKnobValue(me->k_n, num_sum);
}

//--------------------------------------------------------------------

typedef struct
{
    char  i_name[100];
    char  r_name[100];
    char  x_name[100];
} ippknobopts_t;

static psp_paramdescr_t text2espectropts[] =
{
    PSP_P_STRING("i", ippknobopts_t, i_name, ""),
    PSP_P_STRING("r", ippknobopts_t, r_name, ""),
    PSP_P_STRING("x", ippknobopts_t, x_name, ""),
    PSP_P_END()
};

static void IppKnobMagnCB(ipp_privrec_t *me, int new_magn)
{
    //fprintf(stderr, "x=%d\n", new_magn);
    me->magn = new_magn;
    DoDraw(me, True);
}
static void BindToMagn(knobinfo_t *ki, const char *x_name)
{
  ipp_privrec_t  *me = ki->widget_private;
  Knob            x_ki;
  magn_privrec_t *x_me;

    if (x_name[0] != '\0')
    {
        x_ki = datatree_FindNodeFrom(NULL, x_name, ki->uplink);
        if (x_ki == NULL)
            fprintf(stderr, "%s::%s(%s/\"%s\"): x=\"%s\" not found\n",
                    __FILE__, __FUNCTION__, ki->ident, ki->label, x_name);
        else
        {
            x_me = x_ki->widget_private;
            if (x_ki->type != LOGT_WRITE1    ||
                x_ki->look != LOGD_IPP_MAGN  ||
                x_me == NULL)
                fprintf(stderr, "%s::%s(%s/\"%s\"): x=\"%s\" doesn't seem to be a MAGN knob\n",
                        __FILE__, __FUNCTION__, ki->ident, ki->label, x_name);
            else
            {
                x_me->cb = IppKnobMagnCB;
                x_me->me = me;
            }
        }
    }
}

static void ExposureCB(Widget     w,
                       XtPointer  closure,
                       XtPointer  call_data)
{
  ipp_privrec_t  *me = (ipp_privrec_t *)closure;

    XhRemoveExposeEvents(ABSTRZE(w));
    DoDraw(me, 1);
}

static XhWidget zzz(knobinfo_t *ki, XhWidget parent,
                    int wid, int hei, ipp_drawer_t drawer,
                    int chancount)
{
  ipp_privrec_t  *me;

  XhPixel         bg = XhGetColor(XH_COLOR_GRAPH_BG);
  cda_serverid_t  sid;
  int             firstchan;
  int             n;

    if ((me = ki->widget_private = XtMalloc(sizeof(*me))) == NULL)
        return NULL;
    bzero(me, sizeof(*me));
    me->ident   = ki->ident;

    me->wid     = wid;
    me->hei     = hei;
    me->do_draw = drawer;
    me->magn    = magn_factors[0];

    sid = cda_sidof_physchan(ki->physhandle);
    if (sid == CDA_SERVERID_ERROR) return NULL;

    cda_srcof_physchan(ki->physhandle, NULL, &firstchan);
    //fprintf(stderr, "%s: %d->%d,%d\n", ki->ident, ki->physhandle, sid, firstchan);
    me->numchans = chancount;
    for (n = 0;  n < chancount;  n++)
        me->v_h[n] = cda_add_physchan(sid, firstchan + n);

    me->bkgdGC = AllocOneGC(parent, bg,                                 bg, NULL);
    me->axisGC = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_AXIS),    bg, NULL);
    me->reprGC = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_REPERS),  bg, NULL);
    me->prevGC = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_PREV),    bg, NULL);
    me->outlGC = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_OUTL),    bg, NULL);

    if (0)
        me->pix = XCreatePixmap(XtDisplay(parent),
                                RootWindowOfScreen(XtScreen(parent)),
                                wid, hei,
                                DefaultDepthOfScreen(XtScreen(parent)));
    else
        me->pix = 0;

    me->hgrm = XtVaCreateManagedWidget("hgrm", xmDrawingAreaWidgetClass,
                                       parent,
                                       XmNtraversalOn,      False,
                                       XmNwidth,            wid,
                                       XmNheight,           hei,
                                       XmNbackgroundPixmap, me->pix != 0? me->pix : XmUNSPECIFIED_PIXMAP,
                                       NULL);
    if (me->pix == 0)
        XtAddCallback(me->hgrm, XmNexposeCallback, ExposureCB, me);

    // :-(
    ipp_recs_array[ipp_recs_count++] = me;

    return ABSTRZE(me->hgrm);
}

static void yyy(ipp_privrec_t *me)
{
  int            n;
  int            g;

#define SIMULATE_DATA 0

#ifdef SIMULATE_DATA
  static int ctr = 0;
#endif
  
    /* 1. Shift generation */
    memcpy(me->v_v[1], me->v_v[0], sizeof(me->v_v[1]));
    memcpy(me->v_t[1], me->v_t[0], sizeof(me->v_t[1]));
    memcpy(me->v_f[1], me->v_f[0], sizeof(me->v_f[1]));
    memcpy(me->v_s[1], me->v_s[0], sizeof(me->v_s[1]));

    /* 2. Obtain new data */
    for (n = 0;  n < me->numchans;  n++)
    {
        cda_getphyschanval(me->v_h[n],
                           &(me->v_v[0][n]),
                           &(me->v_t[0][n]),
                           &(me->v_f[0][n]));
        me->v_f[0][n] &=~ CXCF_FLAG_OTHEROP;
        me->v_s[0][n] = datatree_choose_knobstate(NULL, me->v_f[0][n]);

#if SIMULATE_DATA
        me->v_v[0][n] = (sin((n+ctr)*3.1415/5*2/NUM_COLS + n/30) * (1<<13) / 10);
#endif

        /* 2.5 "Background" support */
        if (bkgd_ctl == 0) me->v_v[0][n] -= me->v_b[n];
        if (bkgd_ctl >  0) me->v_a[bkgd_ctl-1][n] = me->v_v[0][n];
        if (bkgd_ctl == 1)
        {
            me->v_b[n] = 0;
            for (g = 0;  g < NUM_BKGD_STEPS;  g++)
                me->v_b[n] += me->v_a[g][n];
            me->v_b[n] /= NUM_BKGD_STEPS;
        }
    }
    
#if SIMULATE_DATA
    ctr = (ctr + 1) & 0xFF;
#endif

    DoDraw(me, 1);
    XFlush(XtDisplay(me->hgrm));
    XhProcessPendingXEvents();
}

static XhWidget KNOB_XY_Create_m(knobinfo_t *ki, XhWidget parent)
{
  XhWidget        w;
  ipp_privrec_t  *me;
  ippknobopts_t   opts;

    w = zzz(ki, parent,
            GRF_FULL_WIDTH, GRF_FULL_HEIGHT, IPP_XY_Draw,
            NUM_COLS*2);
    if (w == NULL) return w;
    me = ki->widget_private;

    ParseWiddepinfo(ki, &opts, sizeof(opts), text2espectropts);
    BindToMagn(ki, opts.x_name);

    AllocStateGCs(parent, me->dat1GC, XH_COLOR_GRAPH_BAR1);

    me->textGC = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_TITLES), XhGetColor(XH_COLOR_GRAPH_BG), XH_FIXED_BOLD_FONT);
    me->text_finfo = last_finfo;

    DoDraw(me, False);

    return w;
}

static void KNOB_XY_SetValue_m(knobinfo_t *ki, double v)
{
    yyy((ipp_privrec_t *)(ki->widget_private));
}

static XhWidget KNOB_ESPECTR_Create_m(knobinfo_t *ki, XhWidget parent)
{
  XhWidget        w;
  ipp_privrec_t  *me;
  ippknobopts_t   opts;

    w = zzz(ki, parent,
            GRF_FULL_WIDTH, GRF_FULL_HEIGHT, IPP_ESPECTR_Draw,
            ESPECTR_NUM_CHANS);
    if (w == NULL) return w;
    me = ki->widget_private;

    ParseWiddepinfo(ki, &opts, sizeof(opts), text2espectropts);
    if (opts.i_name[0] != '\0')
    {
        me->i_ki = datatree_FindNodeFrom(NULL, opts.i_name, ki->uplink);
        if (me->i_ki == NULL)
            fprintf(stderr, "%s::%s(%s/\"%s\"): i=\"%s\" not found\n",
                    __FILE__, __FUNCTION__, ki->ident, ki->label, opts.i_name);
    }
    if (opts.r_name[0] != '\0')
    {
        me->r_ki = datatree_FindNodeFrom(NULL, opts.r_name, ki->uplink);
        if (me->r_ki == NULL)
            fprintf(stderr, "%s::%s(%s/\"%s\"): r=\"%s\" not found\n",
                    __FILE__, __FUNCTION__, ki->ident, ki->label, opts.r_name);
    }
    BindToMagn(ki, opts.x_name);

    XtAddEventHandler(me->hgrm,
                      ButtonPressMask   | ButtonReleaseMask | PointerMotionMask |
                      Button1MotionMask | Button2MotionMask | Button3MotionMask,
                      False, MouseHandler, (XtPointer)me);

    me->b_gmin = 0;  me->b_gmax = ESPECTR_NUM_CHANS - 1;
    
    AllocStateGCs(parent, me->dat1GC, XH_COLOR_JUST_RED);
    AllocStateGCs(parent, me->dat2GC, XH_COLOR_JUST_BLUE);

    me->textGC = AllocOneGC(parent, XhGetColor(XH_COLOR_GRAPH_TITLES), XhGetColor(XH_COLOR_GRAPH_BG), XH_FIXED_BOLD_FONT);
    me->text_finfo = last_finfo;

    DoDraw(me, False);

    return w;
}

static void KNOB_ESPECTR_SetValue_m(knobinfo_t *ki, double v)
{
    yyy((ipp_privrec_t *)(ki->widget_private));
}

static void MagnKnobKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  magn_privrec_t *me = privptr;

    me->magn = magn_factors[(int)(round(v))];
    if (me->cb != NULL)
        me->cb(me->me, me->magn);
}
static XhWidget KNOB_MAGN_Create_m(knobinfo_t *ki, XhWidget parent)
{
  magn_privrec_t *me;
  Knob            k_magn;
  char            spec[1000];
  int             i;

    if ((me = ki->widget_private = XtMalloc(sizeof(*me))) == NULL)
        return NULL;
    bzero(me, sizeof(*me));

    strcpy(spec, "look=selector label='m' widdepinfo='#+\v#T");
    for (i = 0;  i < countof(magn_factors);  i++)
        sprintf(spec+strlen(spec), "x%d%s",
                magn_factors[i],
                i < countof(magn_factors)-1? "\v" : "");
    strcat(spec, "' comment='Масштаб отображения'");
    k_magn = CreateSimpleKnob(spec, 0, parent, MagnKnobKCB, (void *)me);

    return GetKnobWidget(k_magn);
}


static void NoColorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}

static knobs_vmt_t KNOB_XY_VMT      = {KNOB_XY_Create_m,      NULL, KNOB_XY_SetValue_m,      NoColorize_m, NULL};
static knobs_vmt_t KNOB_ESPECTR_VMT = {KNOB_ESPECTR_Create_m, NULL, KNOB_ESPECTR_SetValue_m, NoColorize_m, NULL};
static knobs_vmt_t KNOB_MAGN_VMT    = {KNOB_MAGN_Create_m,    NULL, NULL,                    NoColorize_m, NULL};

static knobs_widgetinfo_t ippclient_widgetinfo[] =
{
    {LOGD_IPP_XY,      "ipp_xy",      &KNOB_XY_VMT},
    {LOGD_IPP_ESPECTR, "ipp_espectr", &KNOB_ESPECTR_VMT},
    {LOGD_IPP_MAGN,    "ipp_magn",    &KNOB_MAGN_VMT},
    {0, NULL, NULL}
};

static knobs_widgetset_t  ippclient_widgetset = {ippclient_widgetinfo, NULL};

//////////////////////////////////////////////////////////////////////

static const char *sign_lp_s    = "# Data";
static const char *subsys_lp_s  = "#!SUBSYSTEM:";
static const char *crtime_lp_s  = "#!CREATE-TIME:";
static const char *comment_lp_s = "#!COMMENT:";

static int IppSaveData(const char *sysname, const char *filespec, const char *comment)
{
  FILE          *fp;
  time_t         timenow = time(NULL);
  const char    *cp;
  int            i;
  ipp_privrec_t *me;
  int            x;

    if ((fp = fopen(filespec, "w")) == NULL) return -1;

    fprintf(fp, "%s %s\n", sign_lp_s, sysname);
    fprintf(fp, "%s ipp,%s\n", subsys_lp_s, sysname);
    fprintf(fp, "%s %lu %s", crtime_lp_s, (unsigned long)(timenow), ctime(&timenow)); /*!: No '\n' because ctime includes a trailing one. */
    fprintf(fp, "%s ", comment_lp_s);
    if (comment != NULL)
        for (cp = comment;  *cp != '\0';  cp++)
            fprintf(fp, "%c", !iscntrl(*cp)? *cp : ' ');
    fprintf(fp, "\n");

    for (i = 0;  i < ipp_recs_count;  i++)
    {
        me = ipp_recs_array[i];
        fprintf(fp, "%-10s", me->ident);
        for (x = 0;  x < MAX_CHANS_PER_BOXELEM;  x++)
            fprintf(fp, " %6d", (int)me->v_v[0][x]);
        fprintf(fp, "\n");
    }
    
//    for (i = 0;  i < the_plot->plot_used;  i++)
//        fprintf(fp, "%4d %6.3f %6.3f\n", i, the_plot->plot_x[i], the_plot->plot_y[i]);
    
    fclose(fp);

    return 0;
}


/*####  ####*/

enum
{
    cmGetBkgd = 1,
    cmRstBkgd = 2,
};

static actdescr_t toolslist[] =
{
    XhXXX_TOOLCMD(cmChlNfSaveMode, "Сохранить режим",    btn_save_xpm),
    XhXXX_TOOLCMD(cmGetBkgd,   "Набрать фон",        btn_getbkgd_xpm),
    XhXXX_TOOLCMD(cmRstBkgd,   "Сбросить фон",       btn_rstbkgd_xpm),
    XhXXX_TOOLCHK(cmChlFreeze, "Сделать паузу, скушать \253Твикс\273",
                                                     btn_pause_xpm),
    XhXXX_TOOLLEDS(),
    XhXXX_TOOLCMD(cmChlHelp,   "Краткая справка",    btn_help_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD(cmChlQuit,   "Выйти из программы", btn_quit_xpm),
    XhXXX_END()
};

static XhWidget save_nfndlg = NULL;

static void CommandProc(XhWindow win, int cmd)
{
  char          filename[PATH_MAX];
  char          comment[400];
  
    switch (cmd)
    {
        case cmGetBkgd:
            bkgd_ctl = NUM_BKGD_STEPS;
            XhMakeMessage(win, "Включен набор фона, усреднение по %d", bkgd_ctl);
            ChlRecordEvent(win, "Background collection started (averaging by %d)", bkgd_ctl);
            break;
        
        case cmRstBkgd:
            bkgd_ctl = -1;
            XhMakeMessage(win, "Фон сброшен");
            ChlRecordEvent(win, "Background is reset");
            break;
        
        case cmChlHelp:
            ChlShowHelp(win, CHL_HELP_COLORS);
            break;
            
        case cmChlNfSaveMode:
            if (save_nfndlg != NULL) XhSaveDlgHide(save_nfndlg);
            if (save_nfndlg == NULL)
                save_nfndlg = XhCreateSaveDlg(win, "saveDataDialog", NULL, NULL, cmChlNfSaveOk);
            XhSaveDlgShow(save_nfndlg);
            break;
            
        case cmChlNfSaveOk:
        case cmChlNfSaveCancel:
            XhSaveDlgHide(save_nfndlg);
            if (cmd == cmChlNfSaveOk)
            {
                XhSaveDlgGetComment(save_nfndlg, comment, sizeof(comment));
                check_snprintf(filename, sizeof(filename), CHL_DATA_PATTERN_FMT, ChlGetSysname(win));
                XhFindFilename(filename, filename, sizeof(filename));
                if (IppSaveData(ChlGetSysname(win), filename, comment) == 0)
                    XhMakeMessage(win, "Data saved to \"%s\"", filename);
                else
                    XhMakeMessage(win, "Error saving to file: %s", strerror(errno));
            }
            break;
            
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


static void NewDataProc(XhWindow  win,
                   void     *privptr __attribute__((unused)),
                   int       synthetic)
{
    if (synthetic) return;

    if (bkgd_ctl > 0)
    {
        bkgd_ctl--;
        if (bkgd_ctl == 0)
        {
            XhMakeMessage(win, "Набор фона закончен");
            ChlRecordEvent(win, "Background collection finished");
        }
        else
            XhMakeMessage(win, "Набор фона: осталось %d", bkgd_ctl);
    }
}


int main(int argc, char *argv[])
{
  int            r;

    XhSetColorBinding("GRAPH_REPERS", "#00D000");
    KnobsRegisterWidgetset(&ippclient_widgetset);

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             toolslist, CommandProc,
                             NULL,
                             NULL,
                             NewDataProc, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
