#include "Chl_includes.h"

#include <Xm/DrawingA.h>
#include "InputOnly.h"

#include "KnobsI.h"
#include "Chl.h"


//////////////////////////////////////////////////////////////////////

static knobs_emethods_t canvas_emethods =
{
    Chl_E_SetPhysValue_m,
    Chl_E_ShowAlarm_m,
    Chl_E_AckAlarm_m,
    Chl_E_ShowPropsWindow,
    Chl_E_ShowBigValWindow,
    Chl_E_NewData_m,
    Chl_E_ToHistPlot
};

//////////////////////////////////////////////////////////////////////

enum
{
    SHAPE_RECTANGLE = 0,
    SHAPE_ELLIPSE,
    SHAPE_POLYGON,
    SHAPE_POLYLINE,
    SHAPE_PIPE,
    SHAPE_STRING
};

enum
{
    FONTMOD_NORMAL     = 0,
    FONTMOD_ITALIC     = 1,
    FONTMOD_BOLD       = 2,
    FONTMOD_BOLDITALIC = 3
};

enum
{
    FONTSIZE_TINY   = -2, //  8
    FONTSIZE_SMALL  = -1, // 10
    FONTSIZE_NORMAL = 0,  // 12 (or 14, on large screens)
    FONTSIZE_LARGE  = 1,  // 18
    FONTSIZE_HUGE   = 2,  // 24
    
    FONTSIZE_MIN = FONTSIZE_TINY
};

enum
{
    TEXTALIGN_LEFT   = 0,
    TEXTALIGN_CENTER = 1,
    TEXTALIGN_RIGHT  = 2
};

static char *fontmod_strs[4] = {"medium-r", "medium-i", "bold-r", "bold-i"};
static int   fontsize_sizes[5] = {8, 10, 12, 18, 24};
static unsigned char  textalign_vals[3] = {XmALIGNMENT_BEGINNING, XmALIGNMENT_CENTER, XmALIGNMENT_END};

typedef struct
{
    int         shape;
    int         filled;

    XhPixel     sh_pixel;
    GC          sh_GC;
    
    int         bd_width;
    XhPixel     bd_pixel;
    GC          bd_GC;

    int         tx_present;
    XmString    tx_s;
    XhPixel     tx_pixel;
    XmFontList  tx_fl;
    GC          tx_GC;
    Dimension   tx_wid;
    Dimension   tx_hei;
    char        tx_align;
    int         tx_h_pos;
    int         tx_v_pos;
    int         tx_h_ofs;
    int         tx_v_ofs;
    
    XPoint     *points;
    int         numpoints;
} decor_privrec_t;

typedef struct
{
    int    width;
    int    height;

    char   sh_color[20];
    int    thickness;
    int    rounded;

    int    bd_width;
    char   bd_color[20];

    char   tx_color[20];
    char   tx_pos  [100];
    int    tx_align;
    int    fn_mod;
    int    fn_size;

    char   points[1000];
} decoropts_t;

static psp_lkp_t tx_align_lkp[] =
{
    {"left",   TEXTALIGN_LEFT},
    {"center", TEXTALIGN_CENTER},
    {"right",  TEXTALIGN_RIGHT},
    {NULL, 0}
};

static psp_lkp_t fn_mod_lkp[] =
{
    {"normal",     FONTMOD_NORMAL},
    {"italic",     FONTMOD_ITALIC},
    {"bold",       FONTMOD_BOLD},
    {"bolditalic", FONTMOD_BOLDITALIC},
    {NULL, 0}
};

static psp_lkp_t fn_size_lkp[] =
{
    {"tiny",   FONTSIZE_TINY},
    {"small",  FONTSIZE_SMALL},
    {"normal", FONTSIZE_NORMAL},
    {"large",  FONTSIZE_LARGE},
    {"huge",   FONTSIZE_HUGE},
    {NULL, 0}
};

static psp_paramdescr_t text2decoropts[] =
{
    PSP_P_INT   ("width",       decoropts_t, width,     1, 0, 0),
    PSP_P_INT   ("height",      decoropts_t, height,    1, 0, 0),

    PSP_P_STRING("color",       decoropts_t, sh_color,  ""),
    PSP_P_INT   ("thickness",   decoropts_t, thickness, 0, 0, 0),
    PSP_P_FLAG  ("rounded",     decoropts_t, rounded,   1, 0),
    
    PSP_P_INT   ("border",      decoropts_t, bd_width,  0, 0, 0),
    PSP_P_STRING("bordercolor", decoropts_t, bd_color,  ""),

    PSP_P_STRING("textcolor",   decoropts_t, tx_color,  ""),
    PSP_P_STRING("textpos",     decoropts_t, tx_pos,    ""),
    PSP_P_LOOKUP("textalign",   decoropts_t, tx_align,  TEXTALIGN_LEFT,  tx_align_lkp),
    PSP_P_LOOKUP("fontmod",     decoropts_t, fn_mod,    FONTMOD_NORMAL,  fn_mod_lkp),
    PSP_P_LOOKUP("fontsize",    decoropts_t, fn_size,   FONTSIZE_NORMAL, fn_size_lkp),

    PSP_P_STRING("points",      decoropts_t, points,    ""),
    
    PSP_P_END()
};


static void DrawDecor(knobinfo_t *ki, int on_parent)
{
  decor_privrec_t *me = (decor_privrec_t *)(ki->widget_private);

  int              x0;
  int              y0;
  Widget           target;
  Display         *dpy;
  Window           win;
  
  Position         x_w;
  Position         y_w;
  Dimension        wid;
  Dimension        hei;
  XmDirection      dir;

  int              x;
  int              y;
    
    XtVaGetValues(CNCRTZE(ki->indicator),
                  XmNx,               &x_w,
                  XmNy,               &y_w,
                  XmNwidth,           &wid,
                  XmNheight,          &hei,
                  XmNlayoutDirection, &dir,
                  NULL);

    
    target = CNCRTZE(ki->indicator);
    x0 = 0;
    y0 = 0;
    if (on_parent)
    {
        target = XtParent(target);
        x0 = x_w;
        y0 = y_w;
    }
    dpy = XtDisplay(target);
    win = XtWindow (target);
    
    switch (me->shape)
    {
        case SHAPE_RECTANGLE:
            if (me->filled)
            {
                XFillRectangle(dpy, win, me->sh_GC,
                               x0, y0,
                               wid, hei);
            }
            else
            {
                XDrawRectangle(dpy, win, me->sh_GC,
                               x0, y0,
                               wid-1, hei-1);
            }
            if (me->bd_width > 0)
                XDrawRectangle(dpy, win, me->bd_GC,
                               x0, y0,
                               wid-1, hei-1);
            break;

        case SHAPE_ELLIPSE:
            if (me->filled)
            {
                XFillArc(dpy, win, me->sh_GC,
                         x0, y0,
                         wid, hei,
                         0, 360*64);
            }
            else
            {
                XDrawArc(dpy, win, me->sh_GC,
                         x0, y0,
                         wid-1, hei-1,
                         0, 360*64);
            }
            if (me->bd_width > 0)
                XDrawArc(dpy, win, me->bd_GC,
                         x0, y0,
                         wid-1, hei-1,
                         0, 360*64);
            break;

        case SHAPE_POLYGON:
            if (me->numpoints == 0) break;
            me->points[0].x += x0;
            me->points[0].y += y0;

            if (me->filled)
            {
                XFillPolygon(dpy, win, me->sh_GC,
                             me->points, me->numpoints,
                             Complex, CoordModePrevious);
            }
            else
            {
                XDrawLines(dpy, win, me->sh_GC,
                           me->points, me->numpoints + 1,
                           CoordModePrevious);
            }
            if (me->bd_width > 0)
                XDrawLines(dpy, win, me->bd_GC,
                           me->points, me->numpoints + 1,
                           CoordModePrevious);

            me->points[0].x -= x0;
            me->points[0].y -= y0;
            break;

        case SHAPE_POLYLINE:
        case SHAPE_PIPE:
            if (me->numpoints == 0) break;
            me->points[0].x += x0;
            me->points[0].y += y0;

            XDrawLines(dpy, win, me->sh_GC,
                       me->points, me->numpoints,
                       CoordModePrevious);
            
            me->points[0].x -= x0;
            me->points[0].y -= y0;
            break;
    }

    if (me->tx_present)
    {
        if      (me->tx_h_pos < 0) x = 0 + me->tx_h_ofs;
        else if (me->tx_h_pos > 0) x = wid - me->tx_wid - me->tx_h_ofs;
        else                       x = wid / 2 - me->tx_wid / 2 + me->tx_h_ofs;

        if      (me->tx_v_pos < 0) y = 0 + me->tx_v_ofs;
        else if (me->tx_v_pos > 0) y = hei - me->tx_hei - me->tx_v_ofs;
        else                       y = hei / 2 - me->tx_hei / 2 + me->tx_v_ofs;

        XmStringDraw(dpy, win,
                     me->tx_fl, me->tx_s, me->tx_GC,
                     x0 + x, y0 + y, me->tx_wid, me->tx_align,
                     dir, NULL);
    }
}

static void DecorExposeCB(Widget     w          __attribute__((unused)),
                          XtPointer  closure,
                          XtPointer  call_data  __attribute__((unused)))
{
  knobinfo_t *ki = (knobinfo_t *) closure;
  
    DrawDecor(ki, 0);
}

static XhPixel Str2Pixel(knobinfo_t *ki, const char *name, const char *spec, const char *def)
{
  Widget    cw  = (CNCRTZE(ki->uplink->container));
  Display  *dpy = XtDisplay(cw);
  Colormap  cmap;
  XColor    xcol;
    
    if (*spec == '\0') spec = def;

    if (*spec == '%')
    {
        // For now -- no %-specs
        spec = "red";
    }

    XtVaGetValues(cw, XmNcolormap, &cmap, NULL);

    if (!XAllocNamedColor(dpy, cmap, spec, &xcol, &xcol))
    {
        fprintf(stderr, "Knob %s/\"%s\": failed to get %s=\"%s\"\n",
                ki->ident, ki->label, name, spec);
        xcol.pixel = BlackPixelOfScreen(XtScreen(cw));
    }
    
    return xcol.pixel;
}

static GC AllocPixelGC(knobinfo_t *ki, XhPixel px, int thickness, int join_round)
{
  Widget     cw  = (CNCRTZE(ki->uplink->container));
  XtGCMask   mask;  
  XGCValues  vals;

    mask = GCFunction | GCForeground;
    vals.function   = GXcopy;
    vals.foreground = px;
    if (thickness > 1)
    {
        mask |= GCLineWidth;
        vals.line_width = thickness;
        
        mask |= GCJoinStyle;
        vals.join_style = join_round? JoinRound : JoinMiter;
    }

    return XtGetGC(cw, mask, &vals);
}

static void ParseTextPosition(knobinfo_t *ki, const char *pos)
{
  decor_privrec_t *me = (decor_privrec_t *)(ki->widget_private);
  const char      *p  = pos;

    if (strcasecmp(pos, "none") == 0)
    {
        me->tx_present = 0;
        return;
    }

    me->tx_h_ofs = strtol(p, &p, 10);
    switch (tolower(*p))
    {
        case 'l': me->tx_h_pos = -1;  break;
        case 'c': me->tx_h_pos =  0;  break;
        case 'r': me->tx_h_pos = +1;  break;
        default:  return;
    }
    p++;
    me->tx_v_ofs = strtol(p, &p, 10);
    switch (tolower(*p))
    {
        case 't': me->tx_v_pos = -1;  break;
        case 'm': me->tx_v_pos =  0;  break;
        case 'b': me->tx_v_pos = +1;  break;
    }
}

static void PrepareTextParams(knobinfo_t *ki, decoropts_t *opts_p)
{
  decor_privrec_t *me  = (decor_privrec_t *)(ki->widget_private);
  Widget           cw  = (CNCRTZE(ki->uplink->container));
  Display         *dpy = XtDisplay(cw);

  char             fontspec[400];
  XFontStruct     *fs;
  XGCValues        vals;

    me->tx_s = XmStringCreateLtoR(ki->label, xh_charset);
    me->tx_align = textalign_vals[opts_p->tx_align];
  
    /* 1. Obtain a fontlist */
    check_snprintf(fontspec, sizeof(fontspec),
                   "-*lucida-%s-*-*-%d-*-*-*-*-*-*-*",
                   fontmod_strs[opts_p->fn_mod],
                   fontsize_sizes[opts_p->fn_size - FONTSIZE_MIN]);

    fs = XLoadQueryFont(dpy, fontspec);
    if (fs == NULL)
    {
        fprintf(stderr, "Knob %s/\"%s\": unable to load font \"%s\"; using \"fixed\"\n",
                ki->ident, ki->label, fontspec);
        fs = XLoadQueryFont(dpy, "fixed");
    }
    
    me->tx_fl = XmFontListCreate(fs, xh_charset);

    /* 1.5. ...and calc text size */
    XmStringExtent(me->tx_fl, me->tx_s, &(me->tx_wid), &(me->tx_hei));

    /* 2. Make a GC */
    me->tx_pixel = Str2Pixel(ki, "textcolor", opts_p->tx_color, "black");
    vals.foreground = me->tx_pixel;
    me->tx_GC = XtAllocateGC(cw, 0, GCForeground, &vals, GCFont, 0);
}

static void ParsePoints(knobinfo_t *ki, const char *points_spec)
{
  decor_privrec_t *me = (decor_privrec_t *)(ki->widget_private);
  const char      *p  = points_spec;

  XPoint           pts[100];
  int              idx;
  int              rel;
  int              curx;
  int              cury;

    for (idx = 0, curx = 0, cury = 0;
         idx < countof(pts)  &&  *p != '\0';
         /* 'idx' is incremented at the end of body */)
    {
        rel = 0;
        if (*p == '.')
        {
            p++;
            rel = 1;
        }
        
        if (*p != '+'  &&  *p != '-') break;
        pts[idx].x = strtol(p, &p, 10);
        if (*p != '+'  &&  *p != '-') break;
        pts[idx].y = strtol(p, &p, 10);
        
        if (!rel  &&  idx > 0)
        {
            pts[idx].x -= curx;
            pts[idx].y -= cury;
        }

        curx += pts[idx].x;
        cury += pts[idx].y;
        
        idx++;

        if (*p != '/') break;
        p++;
    }
    
    me->numpoints = idx;
    if (idx > 0)
    {
        /* Note: we allocate an extra point-cell at the end --
                 for the "final" point */
        me->points = (void *)XtMalloc(sizeof(*me->points) * (idx + 1));

        memcpy(me->points, pts, sizeof(*me->points) * idx);
        me->points[idx].x = me->points[0].x - curx;
        me->points[idx].y = me->points[0].y - cury;
    }
}

static XhWidget CreateDecor(knobinfo_t *ki, XhWidget parent,
                            int shape, int filled)
{
  decor_privrec_t *me;
  decoropts_t      opts;
  int              standalone;
  Widget           w;
  Dimension        wid;
  Dimension        hei;
  
  int              idx;
  int              curx;
  int              cury;
    
    /* Allocate the private info structure... */
    if ((ki->widget_private = XtMalloc(sizeof(decor_privrec_t))) == NULL)
        return NULL;
    me = (decor_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    me->shape  = shape;
    me->filled = filled;

    if (ParseWiddepinfo(ki, &opts, sizeof(opts), text2decoropts) != PSP_R_OK)
        bzero(&opts, sizeof(opts));

    if (shape == SHAPE_STRING  &&  opts.tx_color[0] == '\0')
        strzcpy(opts.tx_color, opts.sh_color, sizeof(opts.tx_color));

    ParsePoints(ki, opts.points);
    
    me->sh_GC = AllocPixelGC(ki, me->sh_pixel = Str2Pixel(ki, "color",   opts.sh_color, "#bbe0e3"), opts.thickness, opts.rounded);

    me->bd_width = opts.bd_width;
    me->bd_GC = AllocPixelGC(ki, me->bd_pixel = Str2Pixel(ki, "bdcolor", opts.bd_color, "black"), opts.bd_width > 1? opts.bd_width : 0, opts.rounded);

    me->tx_present = (ki->label != NULL  &&  ki->label[0] != '\0');
    if (me->tx_present) ParseTextPosition(ki, opts.tx_pos);
    if (me->tx_present) PrepareTextParams(ki, &opts);
    
    wid = opts.width;  if (wid < 1) wid = 1;
    hei = opts.height; if (hei < 1) hei = 1;

    switch (shape)
    {
        case SHAPE_STRING:
            if (wid < me->tx_wid) wid = me->tx_wid;
            if (hei < me->tx_hei) hei = me->tx_hei;
            break;

        case SHAPE_POLYGON:
        case SHAPE_POLYLINE:
        case SHAPE_PIPE:
            for (idx = 0, curx = 0, cury = 0;  idx < me->numpoints;  idx++)
            {
                curx += me->points[idx].x;
                cury += me->points[idx].y;
                
                if (wid < curx + 1) wid = curx + 1;
                if (hei < cury + 1) hei = cury + 1;
            }
            break;
    }
    
    /* Create the widget */
    standalone = ki->uplink == NULL  ||  ki->uplink->emlink != &canvas_emethods;
//    standalone = 1;
    
    w = XtVaCreateManagedWidget("decor",
                                standalone? xmDrawingAreaWidgetClass : xmInputOnlyWidgetClass,
                                CNCRTZE(parent),
                                XmNtraversalOn,     False,
                                XmNshadowThickness, 0,
                                XmNwidth,           wid,
                                XmNheight,          hei,
                                NULL);
    if (standalone) XtAddCallback(w, XmNexposeCallback, DecorExposeCB, (XtPointer) ki);
    
    return ABSTRZE(w);
}

static XhWidget RECTANGLE_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_RECTANGLE, 0);
}

static XhWidget FRECTANGLE_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_RECTANGLE, 1);
}

static XhWidget ELLIPSE_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_ELLIPSE, 0);
}

static XhWidget FELLIPSE_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_ELLIPSE, 1);
}

static XhWidget POLYGON_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_POLYGON, 0);
}

static XhWidget FPOLYGON_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_POLYGON, 1);
}

static XhWidget POLYLINE_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_POLYLINE, 0);
}

static XhWidget PIPE_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_PIPE, 0);
}

static XhWidget STRING_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return CreateDecor(ki, parent, SHAPE_STRING, 0);
}

static void Colorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to use InputOnly widget for output */
}

static knobs_vmt_t KNOB_RECTANGLE_VMT  = {RECTANGLE_Create_m,  NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_FRECTANGLE_VMT = {FRECTANGLE_Create_m, NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_ELLIPSE_VMT    = {ELLIPSE_Create_m,    NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_FELLIPSE_VMT   = {FELLIPSE_Create_m,   NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_POLYGON_VMT    = {POLYGON_Create_m,    NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_FPOLYGON_VMT   = {FPOLYGON_Create_m,   NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_POLYLINE_VMT   = {POLYLINE_Create_m,   NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_PIPE_VMT       = {PIPE_Create_m,       NULL, NULL, Colorize_m, NULL};
static knobs_vmt_t KNOB_STRING_VMT     = {STRING_Create_m,     NULL, NULL, Colorize_m, NULL};

static knobs_widgetinfo_t canvas_widgetinfo[] =
{
    {LOGD_RECTANGLE,  "rectangle",  &KNOB_RECTANGLE_VMT},
    {LOGD_FRECTANGLE, "frectangle", &KNOB_FRECTANGLE_VMT},
    {LOGD_ELLIPSE,    "ellipse",    &KNOB_ELLIPSE_VMT},
    {LOGD_FELLIPSE,   "fellipse",   &KNOB_FELLIPSE_VMT},
    {LOGD_POLYGON,    "polygon",    &KNOB_POLYGON_VMT},
    {LOGD_FPOLYGON,   "fpolygon",   &KNOB_FPOLYGON_VMT},
    {LOGD_POLYLINE,   "polyline",   &KNOB_POLYLINE_VMT},
    {LOGD_PIPE,       "pipe",       &KNOB_PIPE_VMT},
    {LOGD_STRING,     "string",     &KNOB_STRING_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  canvas_widgetset = {canvas_widgetinfo, NULL};

void RegisterCanvasWidgetset(void)
{
  static int  is_registered = 0;

    if (is_registered) return;
    is_registered = 1;
    KnobsRegisterWidgetset(&canvas_widgetset);
}

static int IsACanvasDecorKnob(knobinfo_t *ki)
{
  knobs_vmt_t        *vmt_in_q = ki->vmtlink; // "VMT in question"
  knobs_widgetinfo_t *info;
  
    for (info = canvas_widgetinfo;  info->look_id != 0;  info++)
        if (vmt_in_q == info->vmt) return 1;
  
    return 0;
}


//////////////////////////////////////////////////////////////////////

static int ChlFormMaker(ElemInfo e, void *privptr  __attribute__((unused)))
{
    *(KnobsGetElemInnageP(e)) =
        ABSTRZE(XtVaCreateManagedWidget("form", xmFormWidgetClass, CNCRTZE(KnobsGetElemContainer(e)),
                                        XmNshadowThickness, 0,
                                        NULL));
    
    return 0;
}

enum
{
    SIDE_LEFT   = 0,
    SIDE_RIGHT  = 1,
    SIDE_TOP    = 2,
    SIDE_BOTTOM = 3,

    NUM_SIDES   = 4
};

static struct
{
    char *name;
    char *att;
    char *wid;
    char *ofs;
} side_info[NUM_SIDES] =
{
    {"left",   XmNleftAttachment,   XmNleftWidget,   XmNleftOffset  },
    {"right",  XmNrightAttachment,  XmNrightWidget,  XmNrightOffset },
    {"top",    XmNtopAttachment,    XmNtopWidget,    XmNtopOffset   },
    {"bottom", XmNbottomAttachment, XmNbottomWidget, XmNbottomOffset},
};

typedef struct
{
    char  ainfo[NUM_SIDES][100];
} attachopts_t;

static psp_paramdescr_t text2attachopts[] =
{
    PSP_P_STRING("left",   attachopts_t, ainfo[SIDE_LEFT],   ""),
    PSP_P_STRING("right",  attachopts_t, ainfo[SIDE_RIGHT],  ""),
    PSP_P_STRING("top",    attachopts_t, ainfo[SIDE_TOP],    ""),
    PSP_P_STRING("bottom", attachopts_t, ainfo[SIDE_BOTTOM], ""),
    PSP_P_END()
};

typedef struct
{
    int   opposite;
    int   offset;
    char  sibling[100];
} attachdata_t;

static int ParseAttachmentSpec(const char *spec, attachdata_t *data)
{
  const char *p = spec;
  int         tail;

    bzero(data, sizeof(*data));
  
    if (spec == NULL) return 0;
    
    /* Skip leading spaces */
    while (*p != '\0'  &&  isspace(*p)) p++;
    if (*p == '\0') return 0;

    /* An "opposite" flag */
    if (*p == '!')
    {
        data->opposite = 1;
        p++;
    }

    /* Offset (all parsing and checking is performed by strtol(), which treates empty string as 0) */
    data->offset = strtol(p, &p, 10);

    /* Finally, a "@SIBLING" */
    if (*p == '@')
    {
        p++;
        strzcpy(data->sibling, p, sizeof(data->sibling));

        /* Trim possible trailing spaces */
        tail = strlen(data->sibling) - 1;
        while (tail >= 0  &&  isspace(data->sibling[tail]))
            data->sibling[tail--] = '\0';
    }
    
    return 1;
}

static knobinfo_t *FindChild(const char *name, knobinfo_t *kl, int ncnvs)
{
  int  n;

    for (n = 0;  n < ncnvs;  n++)
        if (kl[n].ident != NULL  &&
            strcasecmp(kl[n].ident, name) == 0) return kl + n;
  
    return NULL;
}

static void CanvasExposeHandler(Widget     w                     __attribute__((unused)),
                                XtPointer  closure,
                                XEvent    *event                 __attribute__((unused)),
                                Boolean   *continue_to_dispatch  __attribute__((unused)))
{
  ElemInfo    e = (ElemInfo)closure;
  int         n;
  knobinfo_t *ki;
  
    for (n = 0, ki = e->content;
         n < e->count;
         n++,   ki++)
        if (IsACanvasDecorKnob(ki))
            DrawDecor(ki, 1);
}

int ELEM_CANVAS_Create_m(XhWidget parent, ElemInfo e)
{
  _Chl_containeropts_t  opts;

  knobinfo_t           *kl;
  int                   ncnvs;
  int                   nattl;

  int                   n;
  knobinfo_t           *ki;

  attachopts_t          attopts;
  int                   side;
  attachdata_t          attdata;
  knobinfo_t           *sk;
  Widget                sibling;
  unsigned char         attachment_type;

    /* First, parse options */
    bzero(&opts, sizeof(opts)); // Is required, since none of the flags have defaults specified
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2_Chl_containeropts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
    }

    /*  */
    nattl = (e->ncols >> 24) & 0x000000FF;   if (opts.notitle) nattl = 0;
    ncnvs = e->count - nattl;
    kl    = e->content + nattl;

    /* Give birth to element */
    ChlCreateContainer(parent, e,
                       (opts.noshadow? CHL_CF_NOSHADOW : 0) |
                       (opts.notitle?  CHL_CF_NOTITLE  : 0) |
                       (opts.nohline?  CHL_CF_NOHLINE  : 0) |
                       (opts.nofold?   CHL_CF_NOFOLD   : 0) |
                       (opts.folded?   CHL_CF_FOLDED   : 0) |
                       (opts.fold_h?   CHL_CF_FOLD_H   : 0) |
                       (opts.sepfold?  CHL_CF_SEPFOLD  : 0) |
                       (nattl != 0?    CHL_CF_ATTITLE  : 0) |
                       (opts.title_side << CHL_CF_TITLE_SIDE_shift),
                       ChlFormMaker, NULL);
    e->emlink = &canvas_emethods;
    XtAddEventHandler(CNCRTZE(e->innage), ExposureMask, False, CanvasExposeHandler, (XtPointer)e);

    ChlPopulateContainerAttitle(e, nattl, opts.title_side);

    /* Populate the element */
    /* (Walk in reverse direction in order to make intuitive stacking-order) */
    for (n = ncnvs - 1;  n >= 0;  n--)
    {
        ki = kl + n;
        if (ChlGiveBirthToKnob(ki) != 0) /*!!!*/;
    }

    /* And perform attachments */
    for (n = ncnvs - 1;  n >= 0;  n--)
    {
        ki = kl + n;

        if (psp_parse(ki->placement, NULL,
                      &attopts,
                      Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                      text2attachopts) != PSP_R_OK)
        {
            fprintf(stderr, "Knob %s/\"%s\".placement: %s\n",
                    ki->ident, ki->label, psp_error());
            bzero(&attopts, sizeof(attopts));
        }

        for (side = 0;  side < NUM_SIDES;  side++)
            if (ParseAttachmentSpec(attopts.ainfo[side], &attdata))
            {
                sibling = NULL;
                if (attdata.sibling[0] != '\0')
                {
                    sk = FindChild(attdata.sibling, kl, ncnvs);
                    if (sk != NULL)
                        sibling = CNCRTZE(sk->indicator);
                    else
                        fprintf(stderr, "Knob %s/\"%s\".placement: %s sibling \"%s\" not found\n",
                                ki->ident, ki->label,
                                side_info[side].name, attdata.sibling);
                }

                if (sibling != NULL)
                    attachment_type = attdata.opposite? XmATTACH_OPPOSITE_WIDGET
                                                      : XmATTACH_WIDGET;
                else
                    attachment_type = attdata.opposite? XmATTACH_OPPOSITE_FORM
                                                      : XmATTACH_FORM;
                
                XtVaSetValues(CNCRTZE(ki->indicator),
                              side_info[side].att, attachment_type,
                              side_info[side].wid, sibling,
                              side_info[side].ofs, attdata.offset,
                              NULL);
            }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

