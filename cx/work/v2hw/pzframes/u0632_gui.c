#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>

#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "KnobsI.h"

#include "u0632_data.h"
#include "u0632_gui.h"


//////////////////////////////////////////////////////////////////////
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

    return XtGetGC(CNCRTZE(w), mask, &vals);
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
//////////////////////////////////////////////////////////////////////

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

static void ExposureCB(Widget     w,
                       XtPointer  closure,
                       XtPointer  call_data)
{
  u0632_gui_t *gui = (u0632_gui_t *)closure;

    XhRemoveExposeEvents(ABSTRZE(w));
    gui->draw(gui);
}

static void DrawXY(u0632_gui_t *gui)
{
}

static void DrawES(u0632_gui_t *gui)
{
}

static int DoPopulate(u0632_gui_t *gui, Widget parent)
{
  XhWidget        xp = ABSTRZE(parent);
  XhPixel         bg = XhGetColor(XH_COLOR_GRAPH_BG);

    gui->bkgdGC = AllocOneGC(xp, bg,                                 bg, NULL);
    gui->axisGC = AllocOneGC(xp, XhGetColor(XH_COLOR_GRAPH_AXIS),    bg, NULL);
    gui->reprGC = AllocOneGC(xp, XhGetColor(XH_COLOR_GRAPH_REPERS),  bg, NULL);
    gui->prevGC = AllocOneGC(xp, XhGetColor(XH_COLOR_GRAPH_PREV),    bg, NULL);
    gui->outlGC = AllocOneGC(xp, XhGetColor(XH_COLOR_GRAPH_OUTL),    bg, NULL);
    if (gui->type == U0632_GUI_TYPE_ES)
    {
        AllocStateGCs(xp, gui->dat1GC, XH_COLOR_JUST_RED);
        AllocStateGCs(xp, gui->dat2GC, XH_COLOR_JUST_BLUE);
        gui->draw = DrawES;
    }
    else
    {
        AllocStateGCs(xp, gui->dat1GC, XH_COLOR_GRAPH_BAR1);
        gui->draw = DrawXY;
    }
    gui->textGC = AllocOneGC(xp, XhGetColor(XH_COLOR_GRAPH_TITLES), XhGetColor(XH_COLOR_GRAPH_BG), XH_FIXED_BOLD_FONT);
    gui->text_finfo = last_finfo;
    
    gui->wid    = GRF_FULL_WIDTH;
    gui->hei    = GRF_FULL_HEIGHT;

    gui->hgrm = XtVaCreateManagedWidget("hgrm", xmDrawingAreaWidgetClass,
                                       parent,
                                       XmNtraversalOn,      False,
                                       XmNwidth,            gui->wid,
                                       XmNheight,           gui->hei,
                                       XmNbackground,       bg,
                                       NULL);
    XtAddCallback(gui->hgrm, XmNexposeCallback, ExposureCB, gui);

    gui->g.top = gui->hgrm;

    return 0;
}

//////////////////////////////////////////////////////////////////////

static int DoRealize(pzframe_gui_t *pzframe_gui,
                     Widget parent,
                     const char *srvrspec, int bigc_n,
                     cda_serverid_t def_regsid, int base_chan)
{
  u0632_gui_t *gui = pzframe2u0632_gui(pzframe_gui);

    return U0632GuiRealize(gui, parent,
                           srvrspec, bigc_n,
                           def_regsid, base_chan);
}

static void UpdateBG(pzframe_gui_t *pzframe_gui)
{
  u0632_gui_t *gui = pzframe2u0632_gui(pzframe_gui);

  XhPixel fg = XhGetColor(XH_COLOR_GRAPH_TITLES);
  XhPixel bg = XhGetColor(XH_COLOR_GRAPH_BG);

    ChooseKnobColors(LOGC_NORMAL, gui->g.curstate, fg, bg, &fg, &bg);
    //!!!
}

static void DoRenew(pzframe_gui_t *pzframe_gui,
                    int  info_changed)
{
  u0632_gui_t *gui = pzframe2u0632_gui(pzframe_gui);

    //!!! calc something?  Zero?

    gui->draw(gui);
}

static pzframe_gui_vmt_t u0632_gui_vmt =
{
    .realize  = DoRealize,
    .evproc   = NULL,
    .newstate = UpdateBG,
    .do_renew = DoRenew,
};

pzframe_gui_dscr_t *u0632_get_gui_dscr(void)
{
  static pzframe_gui_dscr_t  dscr;
  static int                 dscr_inited;

    if (!dscr_inited)
    {
        bzero(&dscr, sizeof(dscr));
        dscr.ftd = u0632_get_type_dscr();
        dscr.vmt = u0632_gui_vmt;

        dscr_inited = 1;
    }
    return &dscr;
}

//////////////////////////////////////////////////////////////////////

void  U0632GuiInit   (u0632_gui_t *gui)
{
  pzframe_gui_dscr_t *d = u0632_get_gui_dscr();

    bzero(gui, sizeof(*gui));
    PzframeDataInit(&(gui->h), d->ftd);
    PzframeGuiInit (&(gui->g), &(gui->h), d);
}

int   U0632GuiRealize(u0632_gui_t *gui, Widget parent,
                      const char *srvrspec, int bigc_n,
                      cda_serverid_t def_regsid, int base_chan)
{
  int  r;

    if ((r = PzframeDataRealize(&(gui->h),
                                srvrspec, bigc_n,
                                def_regsid, base_chan)) < 0) return r;
    PzframeGuiRealize(&(gui->g));

    DoPopulate(gui, parent);

    return 0;
}
