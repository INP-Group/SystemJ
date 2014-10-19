#include "Chl_includes.h"

#include "KnobsI.h"
#include "Xh_fallbacks.h"

#include <Xm/CascadeB.h>
#include <Xm/RowColumn.h>

#include <X11/xpm.h>
#include "submenu_triangle.xpm"


typedef struct
{
    Widget      bar;
    Widget      btn;
    Widget      menu;
    XhPixel     deffg;
    XhPixel     defbg;
    rflags_t    currflags;
    colalarm_t  curcolstate;
} submenu_privrec_t;


static void DoColorize(ElemInfo  e)
{
  submenu_privrec_t *me       = (submenu_privrec_t *)(e->elem_private);
  colalarm_t         colstate = me->curcolstate;
  XhPixel            fg       = me->deffg;
  XhPixel            bg       = me->defbg;

    if (colstate == COLALARM_NONE)
    {
        if (e->currflags & CXCF_FLAG_ALARM_RELAX) colstate = COLALARM_RELAX;
        if (e->currflags & CXCF_FLAG_OTHEROP)     colstate = COLALARM_OTHEROP;
    }

    ChooseKnobColors(LOGC_NORMAL, colstate,
                     fg, bg, &fg, &bg);
    if (e->currflags & CXCF_FLAG_ALARM_ALARM) bg = XhGetColor(XH_COLOR_JUST_RED);
    XtVaSetValues(me->btn,
                  XmNforeground, fg,
                  XmNbackground, bg,
                  NULL);
}

static void SUBMENU_NewData_m(ElemInfo e, int synthetic)
{
  submenu_privrec_t *me = (submenu_privrec_t *)e->elem_private;
  colalarm_t         colstate;
    
    Chl_E_NewData_m(e, synthetic);
    
    colstate = datatree_choose_knobstate(NULL, e->currflags);
    if (me->currflags != e->currflags  ||  me->curcolstate != colstate)
    {
        //fprintf(stderr, "flags=%08x state=%d/%s\n", e->currflags, colstate, CdrStrcolalarmShort(colstate));
        me->currflags   = e->currflags;
        me->curcolstate = colstate;
        DoColorize(e);
    }
}


static knobs_emethods_t submenu_emethods =
{
    Chl_E_SetPhysValue_m,
    Chl_E_ShowAlarm_m,
    Chl_E_AckAlarm_m,
    Chl_E_ShowPropsWindow,
    Chl_E_ShowBigValWindow,
    SUBMENU_NewData_m,
    Chl_E_ToHistPlot
};


typedef struct
{
    Knobs_buttonopts_t  button;
    char                label[100];
} submenuopts_t;

static psp_paramdescr_t text2submenuopts[] =
{
    PSP_P_INCLUDE("button",    text2Knobs_buttonopts,
                  PSP_OFFSET_OF(submenuopts_t, button)),
    PSP_P_STRING ("label",     submenuopts_t, label,     ""),
    PSP_P_END()
};

int ELEM_SUBMENU_Create_m(XhWidget parent, ElemInfo e)
{
  submenuopts_t      opts;
  submenu_privrec_t *me;
  int                ncols = e->ncols & 0x0000FFFF;
  XmString           s;
  char              *ls;

  int                n;

  Arg                al[20];
  int                ac;

  Pixmap             triangle;
  XpmAttributes      attrs;
  XpmColorSymbol     symbols[2];

    if ((me = e->elem_private = XtMalloc(sizeof(submenu_privrec_t))) == NULL)
        return -1;
    bzero(me, sizeof(*me));

    bzero(&opts, sizeof(opts));
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2submenuopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
    }

    ls = e->label;
    if (ls == NULL) ls = "v";

    e->emlink = &submenu_emethods;

    me->bar = XtVaCreateManagedWidget("submenu_bar", xmRowColumnWidgetClass,
                                      CNCRTZE(parent),
                                      XmNrowColumnType,   XmMENU_BAR,
                                      XmNmarginWidth,     0,
                                      XmNmarginHeight,    0,
                                      XmNshadowThickness, 0,
                                      NULL);
    e->container = me->bar;

    ac = 0;
    me->menu  = XmCreatePulldownMenu(me->bar, "submenu_menu", al, ac);
    e->innage = me->menu;
    if (ncols > 1)
        XtVaSetValues(me->menu,
                      XmNnumColumns,  ncols,
                      XmNpacking,     XmPACK_COLUMN,
                      XmNorientation, XmHORIZONTAL,
                      NULL);
    if (opts.label[0] != '\0')
    {
        XtVaSetValues(me->menu,
                      XmNtearOffModel, XmTEAR_OFF_ENABLED,
                      XmNtearOffTitle, s = XmStringCreateLtoR(opts.label, xh_charset),
                      NULL);
        XmStringFree(s);
    }
    for (n = 0;  n < e->count;  n++)
        ChlGiveBirthToKnob(e->content + n);

    me->btn = XtVaCreateManagedWidget("submenuBtn", xmCascadeButtonWidgetClass,
                                      me->bar,
                                      XmNsubMenuId,       me->menu,
                                      //XmNmarginWidth,     0,
                                      //XmNshadowThickness, 0,
                                      XmNlabelString,     s = XmStringCreateLtoR(ls, xh_charset),
                                      NULL);
    XmStringFree(s);
    if (opts.label[0] != '\0') XhSetBalloon(ABSTRZE(me->btn), opts.label);

    TuneButtonKnob(me->btn, &(opts.button), 0);

    XtVaGetValues(me->btn,
                  XmNforeground, &(me->deffg),
                  XmNbackground, &(me->defbg),
                  NULL);
    me->curcolstate = COLALARM_JUSTCREATED;
    DoColorize(e);

    symbols[0].name  = NULL;
    symbols[0].value = "none";
    symbols[0].pixel = me->defbg;
    symbols[1].name  = NULL;
    symbols[1].value = "#0000FFFFFFFF";
    symbols[1].pixel = me->defbg;

    attrs.colorsymbols = symbols;
    attrs.numsymbols   = 2;
    attrs.valuemask    = XpmColorSymbols;

    if (XpmCreatePixmapFromData(XtDisplay(me->btn),
                                XRootWindowOfScreen(XtScreen(me->btn)), 
                                submenu_triangle_xpm,
                                &triangle, NULL, &attrs) == XpmSuccess)
        XtVaSetValues(me->btn, XmNcascadePixmap, triangle, NULL);

    return 0;
}
