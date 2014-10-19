#include "Chl_includes.h"

#include "KnobsI.h"
#include "Xh_fallbacks.h"

#include <Xm/PushB.h>


typedef struct
{
    Widget      btn;
    XhWidget    box;
    XhPixel     deffg;
    XhPixel     defbg;
    XhPixel     defam;
    int         logc;
    rflags_t    currflags;
    colalarm_t  curcolstate;
    
    Boolean     is_pressed;
} subwin_privrec_t;


static void DoColorize(ElemInfo  e)
{
  subwin_privrec_t    *me       = (subwin_privrec_t *)(e->elem_private);
  int                  logc     = me->logc;
  colalarm_t           colstate = me->curcolstate;
  XhPixel              fg       = me->deffg;
  XhPixel              bg       = me->defbg;
  XhPixel              am       = me->defam;

    //if (logc == LOGC_VIC  &&  me->is_pressed) logc = LOGC_NORMAL; /*!!! A dirty hack!!!*/
    if (colstate == COLALARM_NONE)
    {
        if (e->currflags & CXCF_FLAG_ALARM_RELAX) colstate = COLALARM_RELAX;
        if (e->currflags & CXCF_FLAG_OTHEROP)     colstate = COLALARM_OTHEROP;
    }
    if (me->is_pressed) bg = am;
    ChooseKnobColors(logc, colstate,
                     fg, bg, &fg, &bg);
    if (e->currflags & CXCF_FLAG_ALARM_ALARM) bg = XhGetColor(XH_COLOR_JUST_RED);
    XtVaSetValues(me->btn,
                  XmNforeground, fg,
                  XmNbackground, bg,
                  NULL);
}

static void ReflectShowness(ElemInfo  e)
{
  subwin_privrec_t    *me = (subwin_privrec_t *)(e->elem_private);
  Boolean              is_managed = XtIsManaged(CNCRTZE(me->box));

    if (is_managed == me->is_pressed) return;
    me->is_pressed = is_managed;

    XhInvertButton(ABSTRZE(me->btn));
    XtVaSetValues(me->btn,
                  XmNarmColor, me->is_pressed? me->defbg : me->defam,
                  NULL);
    DoColorize(e);
}

static void ShowHideSubwin(ElemInfo  e)
{
  subwin_privrec_t    *me = (subwin_privrec_t *)(e->elem_private);

    if (XtIsManaged(CNCRTZE(me->box)))
        XhStdDlgHide(me->box);
    else
        XhStdDlgShow(me->box);

    ReflectShowness(e);
}

static void ActivateCB(Widget     w         __attribute__((unused)),
                       XtPointer  closure,
                       XtPointer  call_data __attribute__((unused)))
{
  ElemInfo             e   = (ElemInfo) closure;

    ShowHideSubwin(e);
}

static void Mouse3Handler(Widget     w  __attribute__((unused)),
                          XtPointer  closure,
                          XEvent    *event,
                          Boolean   *continue_to_dispatch)
{
  ElemInfo             e  = (ElemInfo) closure;
  XButtonPressedEvent *ev = (XButtonPressedEvent *) event;
  subwin_privrec_t    *me = (subwin_privrec_t *)(e->elem_private);

    if (ev->button != Button3) return;
    *continue_to_dispatch = False;
    XmProcessTraversal(me->btn, XmTRAVERSE_CURRENT);
    ShowHideSubwin(e);
}

static void CloseCB(Widget     w         __attribute__((unused)),
                    XtPointer  closure,
                    XtPointer  call_data __attribute__((unused)))
{
  ElemInfo             e   = (ElemInfo) closure;

    ReflectShowness(e);
}

static void SUBWIN_NewData_m(ElemInfo e, int synthetic)
{
  subwin_privrec_t *me = (subwin_privrec_t *)e->elem_private;
  colalarm_t        colstate;
    
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

static knobs_emethods_t subwin_emethods =
{
    Chl_E_SetPhysValue_m,
    Chl_E_ShowAlarm_m,
    Chl_E_AckAlarm_m,
    Chl_E_ShowPropsWindow,
    Chl_E_ShowBigValWindow,
    SUBWIN_NewData_m,
    Chl_E_ToHistPlot
};

typedef struct
{
    _Chl_multicolopts_t  elem;
    Knobs_buttonopts_t   button;
    int                  logc;
    char                 label[100];

    /* for window decors */
    int                  resizable;
    int                  compact;
    int                  noclsbtn;
} subwinopts_t;

static psp_lkp_t logcs_lkp[] =
{
    {"normal",    LOGC_NORMAL},
    {"hilited",   LOGC_HILITED},
    {"important", LOGC_IMPORTANT},
    {"vic",       LOGC_VIC},
    {"dim",       LOGC_DIM},
    {NULL, 0}
};

static psp_paramdescr_t text2subwinopts[] =
{
    PSP_P_INCLUDE("elem",      text2_Chl_multicolopts, PSP_OFFSET_OF(subwinopts_t, elem)),
    PSP_P_INCLUDE("button",    text2Knobs_buttonopts,  PSP_OFFSET_OF(subwinopts_t, button)),
    PSP_P_LOOKUP ("logc",      subwinopts_t, logc,      LOGC_NORMAL, logcs_lkp),
    PSP_P_STRING ("label",     subwinopts_t, label,     ""),

    /* for window decors */
    PSP_P_FLAG   ("resizable", subwinopts_t, resizable, 1,           0),
    PSP_P_FLAG   ("compact",   subwinopts_t, compact,   1,           0),
    PSP_P_FLAG   ("noclsbtn",  subwinopts_t, noclsbtn,  1,           0),
    PSP_P_END()
};

static int DoCreate(XhWidget parent, ElemInfo e, int v4)
{
  subwinopts_t         opts;
  subwin_privrec_t    *me;
  XmString             s;
  char                *ls;
  int                  dlgflags;

    if ((me = e->elem_private = XtMalloc(sizeof(subwin_privrec_t))) == NULL)
        return -1;
    bzero(me, sizeof(*me));

    bzero(&opts, sizeof(opts));
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2subwinopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
        opts.elem.hspc = opts.elem.vspc = opts.elem.colspc = -1;
    }
    if (opts.logc <= 0) opts.logc = LOGC_NORMAL;
    me->logc = opts.logc;

    ls = opts.label;
    if (*ls == '\0') ls = "...";
    me->btn = XtVaCreateManagedWidget("push_i",
                                      xmPushButtonWidgetClass,
                                      CNCRTZE(parent),
                                      XmNtraversalOn, True,
                                      XmNlabelString, s = XmStringCreateLtoR(ls, xh_charset),
                                      NULL);
    XmStringFree(s);
    XtAddCallback(me->btn, XmNactivateCallback, ActivateCB, (XtPointer)e);
    XtAddEventHandler(me->btn, ButtonPressMask, False, Mouse3Handler, (XtPointer)e);
    if (e->label != NULL) XhSetBalloon(ABSTRZE(me->btn), e->label);

    TuneButtonKnob(me->btn, &(opts.button), 0);

    dlgflags = XhStdDlgFOk | XhStdDlgFNoDefButton;
    if (opts.resizable) dlgflags |= XhStdDlgFResizable | XhStdDlgFZoomable;
    if (opts.compact)   dlgflags |= XhStdDlgFNoMargins;
    if (opts.noclsbtn)  dlgflags |= XhStdDlgFNothing;
    me->box = XhCreateStdDlg(XhWindowOf(parent), e->ident, e->label,
                              "Close", NULL, dlgflags);
    XtAddCallback(XtParent(CNCRTZE(me->box)), XmNpopdownCallback, CloseCB, (XtPointer)e);

    XtVaGetValues(me->btn,
                  XmNforeground, &(me->deffg),
                  XmNbackground, &(me->defbg),
                  XmNarmColor,   &(me->defam),
                  NULL);
    me->curcolstate = COLALARM_JUSTCREATED;
    DoColorize(e);

    if (v4)
    {
        e->innage = me->box;
        if (e->count > 0)
            ChlGiveBirthToKnob(e->content);
    }
    else
    {
        ELEM_MULTICOL_Create_m_as(me->box, e, &(opts.elem));
    }
    e->emlink        = &subwin_emethods;
    e->container     = ABSTRZE(me->btn);

    return 0;
}

int ELEM_SUBWIN_Create_m  (XhWidget parent, ElemInfo e)
{
    return DoCreate(parent, e, 0);
}

int ELEM_SUBWINV4_Create_m(XhWidget parent, ElemInfo e)
{
    return DoCreate(parent, e, 1);
}
