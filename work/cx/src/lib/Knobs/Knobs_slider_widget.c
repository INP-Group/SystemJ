#include "Knobs_includes.h"

#include <Xm/ScrollBar.h>


enum {DEF_SIZE = 100};

enum
{
    SLIDER_SCALE = 0,
    SLIDER_SCROLLBAR,
    SLIDER_THERMOMETER
};

enum
{
    LAYOUT_HORZ = 0,
    LAYOUT_RTL,
    LAYOUT_COMPACT
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
    int  type;
    int  valdpy;
    int  haslabel;
    int  layout;
} slideropts_t;

typedef struct
{
    int  scale_factor;
    int  min_int;
    int  max_int;
    int  ofs_int;

    Widget  lab;
    Widget  sb;
    Widget  val;

    Pixel   sb_defbg;
    Pixel   val_deffg;
    Pixel   val_defbg;
    int     are_defcols_got;
} slider_privrec_t;


static void ChangeCB(Widget     w        __attribute__((unused)),
                     XtPointer  closure,
                     XtPointer  call_data)
{
  knobinfo_t                *ki   = (knobinfo_t *)               closure;
  slider_privrec_t          *me   = (slider_privrec_t *)(ki->widget_private);
  XmScrollBarCallbackStruct *info = (XmScrollBarCallbackStruct *)call_data;
  
    SetControlValue(ki, (double)(info->value + me->ofs_int) / me->scale_factor, 1);
}

static psp_paramdescr_t text2slideropts[] =
{
    PSP_P_INT   ("size",        slideropts_t, size,     DEF_SIZE, 10, 1000),
    
    PSP_P_FLAG  ("scale",       slideropts_t, type,     SLIDER_SCALE,       1),
    PSP_P_FLAG  ("scrollbar",   slideropts_t, type,     SLIDER_SCROLLBAR,   0),
    PSP_P_FLAG  ("thermometer", slideropts_t, type,     SLIDER_THERMOMETER, 0),
    
    PSP_P_FLAG  ("input",       slideropts_t, valdpy,   VALDPY_INPUT,       1),
    PSP_P_FLAG  ("value",       slideropts_t, valdpy,   VALDPY_VALUE,       0),
    PSP_P_FLAG  ("novalue",     slideropts_t, valdpy,   VALDPY_NOVALUE,     0),

    PSP_P_FLAG  ("withlabel",   slideropts_t, haslabel, 1,                  1),
    PSP_P_FLAG  ("nolabel",     slideropts_t, haslabel, 0,                  0),
    
    PSP_P_FLAG  ("horz",        slideropts_t, layout,   LAYOUT_HORZ,        1),
    PSP_P_FLAG  ("rtl",         slideropts_t, layout,   LAYOUT_RTL,         0),
    PSP_P_FLAG  ("compact",     slideropts_t, layout,   LAYOUT_COMPACT,     0),

    PSP_P_END()
};

static XhWidget DoCreate(knobinfo_t *ki, XhWidget parent, Boolean vertical)
{
  slider_privrec_t *me;
  Widget         container;

  slideropts_t   opts;

  int            sf;
  char          *p;
  int            decimals;
  int            i;
  int            step;
  int            initval;
  
  const char    *metric;
  XmString       s;
  const char    *a_name;
  unsigned char  a_orientation;
  unsigned char  a_direction;
  int            a_ssize;
  XtEnum         a_arrows      = XmNONE;
  XtEnum         a_slider      = XmETCHED_LINE;
  XtEnum         a_mode        = XmSLIDER;

  int            ssizepcnt     = 50; /* After adding it to max it becomes 50/150=33% */

  int            x;
  Widget         row[3];
  Widget         prev;
  Dimension      sb_dim;

  char          *ls;
    
    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(slider_privrec_t))) == NULL)
        return NULL;
    me   = (slider_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    /* Okay, what do they want? */
    if (ParseWiddepinfo(ki, &opts, sizeof(opts), text2slideropts) != PSP_R_OK)
    {
        opts.size = DEF_SIZE;
    }

    if (!ki->is_rw  &&  opts.valdpy == VALDPY_INPUT) opts.valdpy = VALDPY_VALUE;
    
    if (vertical)
    {
        a_orientation = XmVERTICAL;
        a_direction   = XmMAX_ON_TOP;
        metric        = XmNheight;
    }
    else
    {
        a_orientation = XmHORIZONTAL;
        a_direction   = XmMAX_ON_RIGHT;
        metric        = XmNwidth;
    }

    switch (opts.type)
    {
        case SLIDER_SCROLLBAR:
            a_name   = "slider_scrollbar";
            a_arrows = XmEACH_SIDE;
            a_slider = XmNONE;
            a_mode   = XmSLIDER;
            break;

        case SLIDER_THERMOMETER:
            a_name   = "slider_thermometer";
            a_arrows = XmNONE;
            a_slider = ki->is_rw? XmROUND_MARK : XmNONE;
            a_mode   = XmTHERMOMETER;

            ssizepcnt = 0;
            break;

        default: /* SLIDER_SCALE */
            a_name   = "slider_scale";
            a_arrows = XmNONE;
            a_slider = XmETCHED_LINE;
            a_mode   = XmSLIDER;
    }

    /* Find out the scale factor */
    sf = 1;
    decimals = 0;
    p = strchr(ki->dpyfmt, '.');
    if (p != NULL)
        decimals = strtol(p + 1, NULL, 10);
    for (i = 0;  i < decimals;  i++)
        sf *= 10;

    /**/
    me->scale_factor = sf;
    me->min_int      = ki->rmins[KNOBS_RANGE_DISP] * sf;
    me->max_int      = ki->rmaxs[KNOBS_RANGE_DISP] * sf;
    me->ofs_int      = me->min_int;

    /* In fact, we always shift the range to start at 0 */
    me->min_int -= me->ofs_int;
    me->max_int -= me->ofs_int;

    step = ki->incdec_step * sf;
    if (step == 0) step = 1;

    a_ssize = ((me->max_int - me->min_int) * ssizepcnt) / 100;
    if (a_ssize == 0) a_ssize = 1;
    
    /* Container form... */
    container = XtVaCreateManagedWidget(ki->is_rw? "slider_i" : "slider_o",
                                        xmFormWidgetClass,
                                        CNCRTZE(parent),
                                        XmNtraversalOn,     ki->is_rw,
                                        XmNshadowThickness, 0,
                                        NULL);

    /* Scrollbar itself... */
    initval = -me->ofs_int;
    if (initval < me->min_int) initval = me->min_int;
    if (initval > me->max_int) initval = me->max_int;
    me->sb = XtVaCreateManagedWidget(a_name, xmScrollBarWidgetClass,
                                     container,
                                     XmNminimum,             me->min_int,
                                     XmNmaximum,             me->max_int + a_ssize,
                                     XmNvalue,               initval,
                                     XmNincrement,           step,
                                     XmNorientation,         a_orientation,
                                     XmNprocessingDirection, a_direction,
                                     metric,                 opts.size,
                                     XmNsliderSize,          a_ssize,
                                     XmNeditable,            ki->is_rw,
                                     XmNtraversalOn,         ki->is_rw,
                                     XmNshowArrows,          a_arrows,
                                     XmNsliderMark,          a_slider,
                                     XmNslidingMode,         a_mode,
                                     NULL);
    XtAddCallback(me->sb, XmNdragCallback,         ChangeCB, ki);
    XtAddCallback(me->sb, XmNvalueChangedCallback, ChangeCB, ki);

    /* Label */
    if (opts.haslabel  &&  get_knob_label(ki, &ls))
    {
        me->lab = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass,
                                          container,
                                          XmNlabelString,  s = XmStringCreateLtoR(ls, xh_charset),
                                          XmNalignment,    XmALIGNMENT_BEGINNING,
                                          NULL);
        XmStringFree(s);
    }

    /* Value display */
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

    /* Lay out the widgets */
    switch (opts.layout)
    {
        case LAYOUT_COMPACT:
            if (!vertical)
            {
                prev = NULL;
                if (me->lab != NULL)
                {
                    attachleft(me->lab, NULL, 0);
                    attachtop (me->lab, NULL, 0);
                    prev = me->lab;
                }
                
                if (me->val != 0)
                {
                    attachleft(me->val, prev, 0);
                    attachtop (me->val, NULL, 0);
                    prev = me->val;
                }
                
                attachtop(me->sb, prev, 0);
            }
            else
            {
                attachright(me->sb, NULL, 0);
                
                prev = NULL;
                if (me->lab != NULL)
                {
                    attachright(me->lab, me->sb, 0);
                    attachtop  (me->lab, NULL,     0);
                    prev = me->lab;
                }
                
                if (me->val != NULL)
                {
                    attachright(me->val, me->sb, 0);
                    attachtop  (me->val, prev,     0);
                }
            }
            break;
    
        default:
            switch (opts.layout)
            {
                case LAYOUT_RTL:
                    row[0] = me->lab;
                    row[1] = me->val;
                    row[2] = me->sb;
                    break;

                default:
                    row[0] = me->lab;
                    row[1] = me->sb;
                    row[2] = me->val;
            }
            
            prev = NULL;
            for (x = 0;  x < countof(row);  x++)
                if (row[x] != NULL)
                {
                    if (prev != NULL)
                        attachleft(row[x], prev, 1);
                    else
                        attachleft(row[x], NULL, 0);
                    prev = row[x];
                }
            
            if (!vertical)
            {
                XtVaGetValues(me->sb,  XmNheight, &sb_dim, NULL);
                XtVaSetValues(me->sb,
                              XmNbottomAttachment, XmATTACH_POSITION,
                              XmNbottomPosition,   50,
                              XmNbottomOffset,     -sb_dim / 2,
                              NULL);
            }
    }

    HookPropsWindow(ki, container);
    HookPropsWindow(ki, me->sb);
    if (me->lab != NULL)
        HookPropsWindow(ki, me->lab);
    if (me->val != NULL)
        HookPropsWindow(ki, me->val);
    
    return ABSTRZE(container);
}

static XhWidget HCreate_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, False);
}

static XhWidget VCreate_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, True);
}

static void SetValue_m(knobinfo_t *ki, double v)
{
  slider_privrec_t *me   = (slider_privrec_t *)(ki->widget_private);
  int  iv = round(v * me->scale_factor) - me->ofs_int; /*!!! without the round() when v=0.3 and scale=10 (%.1f) this calc yields 2... */

    if (me->val != NULL)
        SetTextString(ki, me->val, v);
  
    if (iv < me->min_int) iv = me->min_int;
    if (iv > me->max_int) iv = me->max_int;

    XtVaSetValues(me->sb, XmNvalue, iv, NULL);
}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
  slider_privrec_t *me   = (slider_privrec_t *)(ki->widget_private);
  Pixel         fg;
  Pixel         bg;

    if (!me->are_defcols_got)
    {
        XtVaGetValues(me->sb,
                      XmNbackground, &(me->sb_defbg),
                      NULL);
        
        if (me->val != NULL)
            XtVaGetValues(me->val,
                          XmNforeground, &(me->val_deffg),
                          XmNbackground, &(me->val_defbg),
                          NULL);
        
        me->are_defcols_got = 1;
    }
        
    ChooseWColors(ki, newstate, &fg, &bg);
    XtVaSetValues(CNCRTZE(ki->indicator),
                  XmNforeground, fg,
                  XmNbackground, bg,
                  NULL);
    if (me->lab != NULL)
        XtVaSetValues(me->lab,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);

    ChooseKnobColors(ki->color, newstate,
                     ki->wdci.deffg, me->sb_defbg,
                     &fg, &bg);
    
    XtVaSetValues(me->sb,
                  XmNforeground, fg,
                  XmNbackground, bg,
                  NULL);
    
    if (me->val != NULL)
    {
        ChooseKnobColors(ki->color, newstate,
                         me->val_deffg, me->val_defbg,
                         &fg, &bg);
        XtVaSetValues(me->val,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }
}
  
knobs_vmt_t KNOBS_HSLIDER_VMT = {HCreate_m, NULL, SetValue_m, Colorize_m, NULL};
knobs_vmt_t KNOBS_VSLIDER_VMT = {VCreate_m, NULL, SetValue_m, Colorize_m, NULL};
