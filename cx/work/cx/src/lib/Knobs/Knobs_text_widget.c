#include "Knobs_includes.h"

#include "IncDecB.h"


typedef struct
{
    int     haslabel;
    int     hasunits;
    int     compact;
    int     groupable;
    int     grouped;
    double  grpcoeff;
} textopts_t;


typedef struct
{
    Widget  form;
    Widget  grpmrk;
    Widget  label;
    Widget  text;
    Widget  incdec;
    Widget  units;
    Pixel   form_deffg;
    Pixel   form_defbg;
    Pixel   label_deffg;
    Pixel   label_defbg;
    Pixel   grpmrk_deffg;
    Pixel   grpmrk_defbg;
    Pixel   incdec_deffg;
    Pixel   incdec_defbg;
    Pixel   units_deffg;
    Pixel   units_defbg;
    int     are_defcols_got;
} text_privrec_t;


static void MarkChangeCB(Widget     w  __attribute__((unused)),
                         XtPointer  closure,
                         XtPointer  call_data)
{
  knobinfo_t                   *ki   = (knobinfo_t *)closure;
  XmToggleButtonCallbackStruct *info = (XmToggleButtonCallbackStruct *) call_data;

    ki->is_ingroup = info->set;
}

static void BlockButton2Handler(Widget     w        __attribute__((unused)),
                                XtPointer  closure  __attribute__((unused)),
                                XEvent    *event,
                                Boolean   *continue_to_dispatch)
{
  XButtonEvent *ev  = (XButtonEvent *)event;
  
    if (ev->type == ButtonPress  &&  ev->button == Button2)
        *continue_to_dispatch = False;
}

static psp_paramdescr_t text2textopts[] =
{
    PSP_P_FLAG  ("withlabel", textopts_t, haslabel,  1,     0),
    PSP_P_FLAG  ("nolabel",   textopts_t, haslabel,  0,     1),

    PSP_P_FLAG  ("withunits", textopts_t, hasunits,  1,     0),
    PSP_P_FLAG  ("nounits",   textopts_t, hasunits,  0,     1),

    PSP_P_FLAG  ("compact",   textopts_t, compact,   1,     0),

    PSP_P_FLAG  ("groupable", textopts_t, groupable, 1,     0),
    PSP_P_FLAG  ("grouped",   textopts_t, grouped,   1,     0),

    PSP_P_REAL  ("grpcoeff",  textopts_t, grpcoeff,  0.0, -1.0, -2.0),

    PSP_P_END()
};

static XhWidget DoCreate(knobinfo_t *ki, XhWidget parent, int decorated)
{
  text_privrec_t *me;

  Widget      left;
  Widget      top;

  textopts_t  opts;

  XmString    s;
  Dimension   isize;
  Dimension   mt;
  Dimension   mb;
  Dimension   lh;
  XmFontList  lfl;
  
  char       *ls;
  
    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(text_privrec_t))) == NULL)
        return NULL;
    me = (text_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2textopts);

    if (!ki->is_rw)
        decorated = opts.groupable = opts.grouped = 0;
    
    /* Container form... */
    me->form = XtVaCreateManagedWidget("textForm",
                                       xmFormWidgetClass,
                                       CNCRTZE(parent),
                                       XmNshadowThickness, 0,
                                       NULL);
    top = left = NULL;
    
    /* A label */
    if (opts.haslabel  &&  get_knob_label(ki, &ls))
    {
        me->label = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass,
                                            me->form,
                                            XmNlabelString,  s = XmStringCreateLtoR(ls, xh_charset),
                                            XmNalignment,    XmALIGNMENT_BEGINNING,
                                            NULL);
        XmStringFree(s);

        HookPropsWindow(ki, me->label);

        if (opts.compact)
            top  = me->label;
        else
            left = me->label;
    }
    
    /* "Group" mark */
    if (opts.groupable)
    {
        ki->behaviour  |= KNOB_B_IS_GROUPABLE;
        ki->is_ingroup = opts.grouped;
        
        /* Create a non-traversable checkbox */
        me->grpmrk = XtVaCreateManagedWidget("onoff_i",
                                             xmToggleButtonWidgetClass,
                                             me->form,
                                             XmNlabelString, s = XmStringCreateLtoR(" ", xh_charset),
                                             XmNtraversalOn, False,
                                             XmNvalue,       ki->is_ingroup,
                                             XmNselectColor, XhGetColor(XH_COLOR_INGROUP),
                                             XmNdetailShadowThickness, 1,
                                             NULL);
        XmStringFree(s);
        if (top  != NULL) attachtop (me->grpmrk, top,  0);
        if (left != NULL) attachleft(me->grpmrk, left, 0);
        XtAddCallback(me->grpmrk, XmNvalueChangedCallback,
                      MarkChangeCB, (XtPointer)ki);
        XtAddEventHandler(me->grpmrk, ButtonPressMask, False, BlockButton2Handler, NULL); // Temporary countermeasure against Motif Bug#1117

        /* Do all the voodoism with its height */
        XtVaGetValues(me->grpmrk,
                      XmNindicatorSize, &isize,
                      XmNmarginTop,     &mt,
                      XmNmarginBottom,  &mb,
                      XmNfontList,      &lfl,
                      NULL);
        
        s = XmStringCreate(" ", xh_charset);
        lh = XmStringHeight(lfl, s);
        XmStringFree(s);
        mt += lh / 2;
        mb += lh - lh / 2;
        
        XtVaSetValues(me->grpmrk, XmNindicatorSize, 1, NULL);
        XtVaSetValues(me->grpmrk,
                      XmNlabelString,   s = XmStringCreate("", xh_charset),
                      XmNindicatorSize, isize,
                      XmNmarginTop,     mt,
                      XmNmarginBottom,  mb,
                      XmNspacing,       0,
                      NULL);
        XmStringFree(s);

        HookPropsWindow(ki, me->grpmrk);
        
        left = me->grpmrk;
    }
    
    /* Text... */
    me->text = (ki->is_rw?CreateTextInput:CreateTextValue)(ki, me->form);
    if (top  != NULL) attachtop (me->text, top,  0);
    if (left != NULL) attachleft(me->text, left, 0);
    left = me->text;

    HookPropsWindow(ki, me->text);
    
    /* Increment/decrement arrows */
    if (decorated)
    {
        me->incdec = XtVaCreateManagedWidget("inputIncDec",
                                             xmIncDecButtonWidgetClass,
                                             me->form,
                                             XmNclientWidget,     me->text,
                                             XmNleftAttachment,   XmATTACH_WIDGET,
                                             XmNleftOffset,       0,
                                             XmNleftWidget,       me->text,
                                             XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
                                             XmNtopWidget,        me->text,
                                             XmNtopOffset,        0,
                                             NULL);
        left = me->incdec;
    }

    /* Optional "units" post-label for rw knobs */
    if (ki->is_rw  &&  opts.hasunits  &&  ki->units != NULL  &&  ki->units[0] != '\0')
    {
        me->units = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass,
                                            me->form,
                                            XmNlabelString,  s = XmStringCreateLtoR(ki->units, xh_charset),
                                            XmNalignment,    XmALIGNMENT_BEGINNING,
                                            XmNleftAttachment,   XmATTACH_WIDGET,
                                            XmNleftOffset,       0,
                                            XmNleftWidget,       left,
                                            XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
                                            XmNtopWidget,        left,
                                            XmNtopOffset,        0,
                                            NULL);
        XmStringFree(s);

        HookPropsWindow(ki, me->units);

        left = me->units;
    }
    
    me->are_defcols_got = 0;
    
    return ABSTRZE(me->form);
}

static XhWidget TEXT_Create_m    (knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, ki->is_rw);
}

static XhWidget TEXTND_Create_m  (knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, 0);
}

static void SetValue_m(knobinfo_t *ki, double v)
{
  text_privrec_t *me = (text_privrec_t *)(ki->widget_private);

    SetTextString(ki, me->text, v);
}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
  text_privrec_t *me = (text_privrec_t *)(ki->widget_private);
  Pixel         fg;
  Pixel         bg;

    if (!me->are_defcols_got)
    {
        XtVaGetValues(me->text,
                      XmNforeground, &(ki->wdci.deffg),
                      XmNbackground, &(ki->wdci.defbg),
                      NULL);
        if (1)
            XtVaGetValues(me->form,
                          XmNforeground, &(me->form_deffg),
                          XmNbackground, &(me->form_defbg),
                          NULL);
        if (me->label != NULL)
            XtVaGetValues(me->label,
                          XmNforeground, &(me->label_deffg),
                          XmNbackground, &(me->label_defbg),
                          NULL);
        if (me->grpmrk != NULL)
            XtVaGetValues(me->grpmrk,
                          XmNforeground, &(me->grpmrk_deffg),
                          XmNbackground, &(me->grpmrk_defbg),
                          NULL);
        if (me->incdec != NULL)
            XtVaGetValues(me->incdec,
                          XmNforeground, &(me->incdec_deffg),
                          XmNbackground, &(me->incdec_defbg),
                          NULL);
        if (me->units != NULL)
            XtVaGetValues(me->units,
                          XmNforeground, &(me->units_deffg),
                          XmNbackground, &(me->units_defbg),
                          NULL);
        me->are_defcols_got = 1;
    }
    
    if (1)
    {
        ChooseKnobColors(ki->color, newstate,
                         me->form_deffg, me->form_defbg,
                         &fg, &bg);
        XtVaSetValues(me->form,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }
    if (me->label != NULL)
    {
        ChooseKnobColors(ki->color, newstate,
                         me->label_deffg, me->label_defbg,
                         &fg, &bg);
        XtVaSetValues(me->label,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }
    if (me->grpmrk != NULL)
    {
        ChooseKnobColors(ki->color, newstate,
                         me->grpmrk_deffg, me->grpmrk_defbg,
                         &fg, &bg);
        XtVaSetValues(me->grpmrk,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }
    if (me->incdec != NULL)
    {
        ChooseKnobColors(ki->color, newstate,
                         me->incdec_deffg, me->incdec_defbg,
                         &fg, &bg);
        XtVaSetValues(me->incdec,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }
    if (me->units != NULL)
    {
        ChooseKnobColors(ki->color, newstate,
                         me->units_deffg, me->units_defbg,
                         &fg, &bg);
        XtVaSetValues(me->units,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }

    ChooseWColors(ki, newstate, &fg, &bg);
    XtVaSetValues(me->text,
                  XmNforeground, fg,
                  XmNbackground, bg,
                  NULL);
}

static void PropsChg_m(knobinfo_t *ki, knobinfo_t *old_ki)
{
  text_privrec_t *me = (text_privrec_t *)(ki->widget_private);

    if (ki->behaviour & KNOB_B_IS_GROUPABLE  &&  me->grpmrk != NULL)
        XtVaSetValues(me->grpmrk, XmNset, ki->is_ingroup != 0, NULL);
}


knobs_vmt_t KNOBS_TEXT_VMT     = {TEXT_Create_m,     NULL, SetValue_m, Colorize_m, PropsChg_m};
knobs_vmt_t KNOBS_TEXTND_VMT   = {TEXTND_Create_m,   NULL, SetValue_m, Colorize_m, PropsChg_m};
