#include "Knobs_includes.h"

#include "Knobs_usertext_widget.h"


typedef struct
{
    CxWidget  box;
    Widget    text_w;
} usertext_privrec_t;


static void Mouse3Handler(Widget     w  __attribute__((unused)),
                          XtPointer  closure,
                          XEvent    *event,
                          Boolean   *continue_to_dispatch)
{
  knobinfo_t            *ki  = (knobinfo_t *)          closure;
  XButtonPressedEvent   *ev  = (XButtonPressedEvent *) event;
  usertext_privrec_t    *me  = ki->widget_private;
  
    if (ev->button != Button3) return;
    *continue_to_dispatch = False;

    if (XtIsManaged(CNCRTZE(me->box))) return;
    XmTextSetString(me->text_w, ki->label != NULL? ki->label : "");
    XhStdDlgShow(me->box);
}

static void OkCB(Widget     w          __attribute__((unused)),
                 XtPointer  closure,
                 XtPointer  call_data  __attribute__((unused)))
{
  knobinfo_t            *ki  = (knobinfo_t *)          closure;
  usertext_privrec_t    *me  = ki->widget_private;
  char                  *lbl = XmTextGetString(me->text_w);

    XhStdDlgHide(me->box);

    if (ki->label == NULL  ||
        strcmp(lbl, ki->label) != 0)
    {
        safe_free(ki->label);
        ki->label = strdup(lbl);
        if (ki->vmtlink != NULL  &&
            ki->vmtlink->PropsChg != NULL)
            ki->vmtlink->PropsChg(ki, ki);
    }
    
    XtFree(lbl);
}

static XhWidget USERTEXT_Create_m(knobinfo_t *ki, XhWidget parent)
{
  usertext_privrec_t *me;
  Widget              w;
  XmString            s;
  const char         *text;
  unsigned char       halign = XmALIGNMENT_BEGINNING;
  XhWindow            win = XhWindowOf(parent);
  char                tbuf[200];
  char                lbuf[200];
    
    /* Allocate the private structure... */
    if ((me = ki->widget_private = XtMalloc(sizeof(usertext_privrec_t))) == NULL)
        return NULL;
    bzero(me, sizeof(*me));

    if (!get_knob_label(ki, &text)) text = "";//text = "???";

    w = XtVaCreateManagedWidget("rowlabel",
                                xmLabelWidgetClass,
                                CNCRTZE(parent),
                                XmNlabelString,   s = XmStringCreateLtoR(text, xh_charset),
                                XmNalignment,     halign,
                                XmNrecomputeSize, True,
                                NULL);
    XmStringFree(s);

    XtAddEventHandler(w, ButtonPressMask, False, Mouse3Handler, (XtPointer)ki);

    check_snprintf(tbuf, sizeof(tbuf), ".%s label", ki->ident);
    check_snprintf(lbuf, sizeof(lbuf), "Change .%s label to", ki->ident);
    me->box = XhCreateStdDlg(win, "usertext", tbuf, NULL, lbuf,
                             XhStdDlgFOk | XhStdDlgFCancel);
    XtAddCallback(me->box, XmNokCallback,     OkCB,     (XtPointer)ki);
    me->text_w = XtVaCreateManagedWidget("text_i", xmTextWidgetClass, me->box,
                                         XmNscrollHorizontal, False,
                                         XmNcursorPositionVisible, False,
                                         XmNeditable,              True,
                                         XmNtraversalOn,           True,
                                         XmNcolumns,               60,
                                         NULL);
    SetCursorCallback(me->text_w);

    XtVaSetValues(me->box, XmNinitialFocus, me->text_w, NULL);

    return ABSTRZE(w);
}

static void USERTEXT_PropsChg_m(knobinfo_t *ki, knobinfo_t *old_ki __attribute__((unused)))
{
  XmString       s;
  const char    *text;
  
    if (!get_knob_label(ki, &text)) text = "";//text = "???";

    XtVaSetValues(CNCRTZE(ki->indicator),
                  XmNlabelString, s = XmStringCreateLtoR(text, xh_charset),
                  NULL);
    XmStringFree(s);
}

static void USERTEXT_Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
    CommonColorize_m(ki, newstate);
}

knobs_vmt_t KNOBS_USERTEXT_VMT = {USERTEXT_Create_m, NULL, NULL, USERTEXT_Colorize_m, USERTEXT_PropsChg_m};
