#include "Knobs_includes.h"


typedef struct
{
    int    last_lit;
    Pixel  lit_pixel;
} button_privrec_t;


static void ActivateCB(Widget                      w __attribute__((unused)),
                       knobinfo_t                 *ki,
                       XmPushButtonCallbackStruct *info)
{
    if (ki->is_rw) SetControlValue(ki, CX_VALUE_COMMAND, 1);
}

static void SetButtonCallbacks(Widget w, knobinfo_t *ki)
{
    XtAddCallback(w, XmNactivateCallback,
                  (XtCallbackProc) ActivateCB, (XtPointer)ki);

    HookPropsWindow(ki, w);
}

static void SwapShadowsCB(Widget     w,
                          XtPointer  closure   __attribute__((unused)),
                          XtPointer  call_data __attribute__((unused)))
{
  Pixel  ts, bs;
  
    XtVaGetValues(w,
                  XmNtopShadowColor,    &ts,
                  XmNbottomShadowColor, &bs,
                  NULL);

    XtVaSetValues(w,
                  XmNtopShadowColor,    bs,
                  XmNbottomShadowColor, ts,
                  NULL);
}


static XhWidget BUTTON_Create_m(knobinfo_t *ki, XhWidget parent)
{
  button_privrec_t   *me;

  Widget              w;
  XmString            s;
  char               *ls;

  Knobs_buttonopts_t  opts;

    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2Knobs_buttonopts);
    
    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(button_privrec_t))) == NULL)
        return NULL;
    me = (button_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    if (opts.color <= 0) opts.color = XH_COLOR_JUST_RED;
    
    me->lit_pixel = XhGetColor(opts.color);

    ki->behaviour |= KNOB_B_IS_LIGHT | KNOB_B_IS_BUTTON | KNOB_B_IGN_OTHEROP | KNOB_B_INCDECSTEP_FXD;

    get_knob_label(ki, &ls);
    w = XtVaCreateManagedWidget(ki->is_rw? "push_i" : "push_o",
                                xmPushButtonWidgetClass,
                                CNCRTZE(parent),
                                XmNtraversalOn, ki->is_rw,
                                XmNlabelString, s = XmStringCreateLtoR(ls, xh_charset),
                                NULL);
    XmStringFree(s);
    TuneButtonKnob(w, &opts, 0);

    SetButtonCallbacks(w, ki);
    
    return ABSTRZE(w);
}

static XhWidget ARROW_Create(knobinfo_t *ki, XhWidget parent, unsigned char dir)
{
  button_privrec_t   *me = (button_privrec_t *)(ki->widget_private);

  Widget              w;
  int                 has_shadow;
  
  Knobs_buttonopts_t  opts;

  XmString            s;
  Dimension           lw, lh;
  Dimension           asize;

  typedef struct
  {
      XmFontList  font;
      Dimension   margin_height;
      Dimension   margin_top;   
      Dimension   margin_bottom;
      Dimension   shadow_thickness;
      Dimension   highlight_thickness;
      Dimension   border_width;
  } labemu_t;
  labemu_t   lei;
  XtResource labemuRes[] = {
      {
          XmNfontList,           XmCFontList,           XmRFontList,
          sizeof(XmFontList), XtOffsetOf(labemu_t, font),
          NULL, 0
      },
      {
          XmNmarginHeight,       XmCMarginHeight,       XmRVerticalDimension,
          sizeof(Dimension),  XtOffsetOf(labemu_t, margin_height),
          XmRImmediate, (XtPointer)2
      },
      {
          XmNmarginTop,          XmCMarginTop,          XmRVerticalDimension,
          sizeof(Dimension),  XtOffsetOf(labemu_t, margin_top),
          XmRImmediate, (XtPointer)0
      },
      
      {
          XmNmarginBottom,       XmCMarginBottom,       XmRVerticalDimension,
          sizeof(Dimension),  XtOffsetOf(labemu_t, margin_bottom),
          XmRImmediate, (XtPointer)0
      },
      {
          XmNshadowThickness,    XmCShadowThickness,    XmRHorizontalDimension,
          sizeof (Dimension), XtOffsetOf(labemu_t, shadow_thickness),
          XmRImmediate, (XtPointer) 2
      },
      {
          XmNhighlightThickness, XmCHighlightThickness, XmRHorizontalDimension,
          sizeof (Dimension), XtOffsetOf(labemu_t, highlight_thickness),
          XmRImmediate, (XtPointer) 0
      },
      {
          XmNborderWidth,        XmCBorderWidth,        XmRHorizontalDimension,
          sizeof (Dimension), XtOffsetOf(labemu_t, border_width),
          XmRImmediate, (XtPointer) 0
      },
  };
    
    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2Knobs_buttonopts);
    
    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(button_privrec_t))) == NULL)
        return NULL;
    me = (button_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    me->lit_pixel = XhGetColor(opts.color);

    ki->behaviour |= KNOB_B_IS_LIGHT | KNOB_B_IS_BUTTON | KNOB_B_IGN_OTHEROP | KNOB_B_INCDECSTEP_FXD;
    
    s = XmStringCreate("N", xh_charset);
    XtGetSubresources(CNCRTZE(parent), (XtPointer)&lei,
                      ki->is_rw? "push_i" : "push_o", "XmPushButton",
                      labemuRes, XtNumber(labemuRes),
                      NULL, 0);
    XmStringExtent(lei.font, s, &lw, &lh);
    
    if (opts.size > 0)
    {
      char         fontspec[100];
      XFontStruct *finfo;
      XmFontList   flist;
      
        check_snprintf(fontspec, sizeof(fontspec),
                       "-*-" XH_FONT_PROPORTIONAL_FMLY "-%s-r-*-*-%d-*-*-*-*-*-*-*",
                       opts.bold? XH_FONT_BOLD_WGHT : XH_FONT_MEDIUM_WGHT,
                       opts.size);
        finfo = XLoadQueryFont(XtDisplay(CNCRTZE(parent)), fontspec);
        if (finfo != NULL)
        {
            flist = XmFontListCreate(finfo, xh_charset);
            if (flist != NULL)
            {
                XmStringExtent(flist, s, &lw, &lh);
                XmFontListFree(flist);
            }
            XFreeFont(XtDisplay(CNCRTZE(parent)), finfo);
        }
    }

    XmStringFree(s);
    asize = (
             lei.border_width +
             lei.highlight_thickness +
             lei.shadow_thickness +
             lei.margin_height
            ) * 2 +
        lei.margin_top + lh + lei.margin_bottom;
//    fprintf(stderr, "\tmh=%d mt=%d mb=%d\n", lei.margin_height, lei.margin_top, lei.margin_bottom);
//    fprintf(stderr, "\tlw=%d lh=%d\n", lw, lh);
//    fprintf(stderr, "asize=%d\n", asize);

    has_shadow = 1;
    if (!ki->is_rw) has_shadow = 0;
    
    w = XtVaCreateManagedWidget(ki->is_rw? "arrow_i" : "arrow_o",
                                xmArrowButtonWidgetClass,
                                CNCRTZE(parent),
                                XmNtraversalOn,    ki->is_rw,
                                XmNarrowDirection, dir,
                                XmNwidth,          asize,
                                XmNheight,         asize,
                                NULL);
    TuneButtonKnob(w, &opts,
                   TUNE_BUTTON_KNOB_F_NO_PIXMAP|TUNE_BUTTON_KNOB_F_NO_FONT);

    if (has_shadow)
    {
        XtAddCallback(w, XmNarmCallback,    SwapShadowsCB, NULL);
        XtAddCallback(w, XmNdisarmCallback, SwapShadowsCB, NULL);
    }
    else
    {
        XtVaSetValues(w, XmNshadowThickness, 0, NULL);
    }

    SetButtonCallbacks(w, ki);
    
    return ABSTRZE(w);
}

static XhWidget ARROW_LT_Create_m(knobinfo_t *ki, XhWidget parent)
{return ARROW_Create(ki, parent, XmARROW_LEFT);}

static XhWidget ARROW_RT_Create_m(knobinfo_t *ki, XhWidget parent)
{return ARROW_Create(ki, parent, XmARROW_RIGHT);}

static XhWidget ARROW_UP_Create_m(knobinfo_t *ki, XhWidget parent)
{return ARROW_Create(ki, parent, XmARROW_UP);}

static XhWidget ARROW_DN_Create_m(knobinfo_t *ki, XhWidget parent)
{return ARROW_Create(ki, parent, XmARROW_DOWN);}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate);

static void PerformColorization(knobinfo_t *ki, colalarm_t newstate, double curv)
{
  button_privrec_t   *me = (button_privrec_t *)(ki->widget_private);

  Pixel         fg;
  Pixel         bg;
  Boolean  is_lit     = (((int)(curv)) & CX_VALUE_LIT_MASK)      != 0;
  
    ChooseWColors(ki, newstate, &fg, &bg);

    if (is_lit) fg = me->lit_pixel;
  ////fprintf(stderr, "\t%s, is_lit=%d, fg=%08x\n", __FUNCTION__, is_lit, fg);
    
    XtVaSetValues(CNCRTZE(ki->indicator),
                  XmNforeground, fg,
                  XmNbackground, bg,
                  NULL);
}

static void SetValue_m(knobinfo_t *ki, double v)
{
  button_privrec_t   *me = (button_privrec_t *)(ki->widget_private);

  Boolean  is_lit     = (((int)v) & CX_VALUE_LIT_MASK)      != 0;
  Boolean  is_enabled = (((int)v) & CX_VALUE_DISABLED_MASK) == 0  /*&&  ki->is_rw*/;

  ////fprintf(stderr, "%s:=%1.0f, is_lit=%d, last_lit=%d\n", __FUNCTION__, v, is_lit, me->last_lit);
  
    /* Handle B_IS_LIGHT protocol */
    if (is_lit != me->last_lit)
    {
        PerformColorization(ki, ki->colstate, v);
        me->last_lit = is_lit;
    }
  
    /* Handle B_IS_BUTTON::disableable protocol */
    if (XtIsSensitive(CNCRTZE(ki->indicator)) != is_enabled)
        XtSetSensitive(CNCRTZE(ki->indicator), is_enabled);
}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
    PerformColorization(ki, newstate, ki->curv);
}


knobs_vmt_t KNOBS_BUTTON_VMT   = {BUTTON_Create_m,   NULL, SetValue_m, Colorize_m, NULL};
knobs_vmt_t KNOBS_ARROW_LT_VMT = {ARROW_LT_Create_m, NULL, SetValue_m, Colorize_m, NULL};
knobs_vmt_t KNOBS_ARROW_RT_VMT = {ARROW_RT_Create_m, NULL, SetValue_m, Colorize_m, NULL};
knobs_vmt_t KNOBS_ARROW_UP_VMT = {ARROW_UP_Create_m, NULL, SetValue_m, Colorize_m, NULL};
knobs_vmt_t KNOBS_ARROW_DN_VMT = {ARROW_DN_Create_m, NULL, SetValue_m, Colorize_m, NULL};
