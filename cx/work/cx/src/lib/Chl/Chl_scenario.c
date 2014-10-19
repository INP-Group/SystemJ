#include "Chl_includes.h"

#include <Xm/PushB.h>

#include "KnobsI.h"
#include "Xh_fallbacks.h"

#include "Cdr_script.h"


typedef struct
{
    knobinfo_t           *ki;
    
    Widget                form;
    Widget                start_btn;
    Widget                stop_btn;

    int                   state_rqd;

    lse_context_t         context;
    XtIntervalId          timer;
    Pixel                 lit_pixel;
} scenario_privrec_t;

typedef struct
{
    Knobs_buttonopts_t  button;

    int                 stop;

    int                 rq_shift;
    int                 rq_alt;

    const char         *scenario;
} scenarioopts_t;

static int ScenarioPluginParser(const char *str, const char **endptr,
                                void *rec, size_t recsize __attribute__((unused)),
                                const char *separators __attribute__((unused)), const char *terminators __attribute__((unused)),
                                void *privptr, char **errstr)
{
  const char **dp = rec;

    *dp = str;
    if (str != NULL)
    {
        if (endptr != NULL) *endptr = str + strlen(str);
    }

    return PSP_R_OK;
}

static psp_paramdescr_t text2scenarioopts[] =
{
    PSP_P_PLUGIN ("scenario", scenarioopts_t, scenario, ScenarioPluginParser, NULL),

    PSP_P_INCLUDE("button",   text2Knobs_buttonopts, PSP_OFFSET_OF(scenarioopts_t, button)),

    PSP_P_FLAG   ("stop",     scenarioopts_t, stop,     1,                 0),
    PSP_P_FLAG   ("nostop",   scenarioopts_t, stop,     0,                 1),

    PSP_P_FLAG   ("shift",    scenarioopts_t, rq_shift, 1,                 0),
    PSP_P_FLAG   ("alt",      scenarioopts_t, rq_alt,   1,                 0),

    PSP_P_END()
};

static void DoHilite(scenario_privrec_t *me)
{
    XtVaSetValues(me->start_btn, XmNforeground, me->lit_pixel, NULL);
}

static void UnHilite(scenario_privrec_t *me)
{
    XtVaSetValues(me->start_btn, XmNforeground, me->ki->wdci.deffg, NULL);
}

static void TimerProc(XtPointer closure,
                      XtIntervalId *id __attribute__((unused)))
{
  scenario_privrec_t         *me   = closure;

  int                         r;

    if (me->timer == 0) return;
    me->timer = 0;
    
    r = lse_cont(&(me->context));
    if (r > 0)
        me->timer = XtAppAddTimeOut(XtWidgetToApplicationContext(me->start_btn),
                                    (r + 999) / 1000,
                                    TimerProc, me);
    else
    {
        UnHilite(me);
        lse_stop(&(me->context));
    }
}

static void StartCB(Widget     w,
                    XtPointer  closure,
                    XtPointer  call_data)
{
  scenario_privrec_t         *me   = closure;
  XmPushButtonCallbackStruct *info = call_data;

  unsigned int                state;

  int                         r;

    state = 0;
    if (info->event->type == KeyPress     ||  info->event->type == KeyRelease)
        state = ((XKeyEvent *)   (info->event))->state;
    if (info->event->type == ButtonPress  ||  info->event->type == ButtonRelease)
        state = ((XButtonEvent *)(info->event))->state;

    if ((state & me->state_rqd) != me->state_rqd) return;

    if (lse_is_running(&(me->context))) return;

    /* Okay -- may do... */
    DoHilite(me);
    r = lse_run(&(me->context), 0);
    if (r > 0)
        me->timer = XtAppAddTimeOut(XtWidgetToApplicationContext(me->start_btn),
                                    (r + 999) / 1000,
                                    TimerProc, me);
    else
    {
        UnHilite(me);
        lse_stop(&(me->context));
    }
}

static void StopCB(Widget     w,
                   XtPointer  closure,
                   XtPointer  call_data __attribute__((unused)) )
{
  scenario_privrec_t *me = closure;

    UnHilite(me);
    lse_stop(&(me->context));
    if (me->timer != 0)
    {
        XtRemoveTimeOut(me->timer);
        me->timer = 0;
    }
}


static XhWidget SCENARIO_Create_m(knobinfo_t *ki, XhWidget parent)
{
  scenario_privrec_t *me;
  XmString            s;
  char               *ls;

  scenarioopts_t      opts;

    /* Allocate the private info structure... */
    if ((me = ki->widget_private = XtMalloc(sizeof(*me))) == NULL)
        return NULL;
    bzero(me, sizeof(*me));
    me->ki = ki;

    /* Parse... */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2scenarioopts);
    me->lit_pixel = XhGetColor(opts.button.color);
    
    /* Initialize context */
    Cdr_script_init(&(me->context),
                    ki->ident, opts.scenario,
                    DataInfoOf(XhWindowOf(parent))->grouplist);

    /* Create widgets */
    if (opts.stop)
    {
        me->form = XtVaCreateManagedWidget("form",
                                           xmFormWidgetClass,
                                           CNCRTZE(parent),
                                           XmNshadowThickness, 0,
                                           NULL);

        me->stop_btn = XtVaCreateManagedWidget("push_i",
                                               xmPushButtonWidgetClass,
                                               me->form,
                                               XmNtraversalOn,      True,
                                               XmNlabelString,      s = XmStringCreateLtoR("X", xh_charset),
                                               XmNrightAttachment,  XmATTACH_FORM,
                                               XmNtopAttachment,    XmATTACH_FORM,
                                               XmNbottomAttachment, XmATTACH_FORM,
                                               NULL);
        XmStringFree(s);
        TuneButtonKnob(me->stop_btn, &(opts.button), TUNE_BUTTON_KNOB_F_NO_PIXMAP);
        XtAddCallback (me->stop_btn, XmNactivateCallback, StopCB, me);
    }

    get_knob_label(ki, &ls);
    me->start_btn = XtVaCreateManagedWidget("push_i",
                                            xmPushButtonWidgetClass,
                                            me->form != NULL? me->form : CNCRTZE(parent),
                                            XmNtraversalOn,      True,
                                            XmNlabelString,      s = XmStringCreateLtoR(ls, xh_charset),
                                            NULL);
    XmStringFree(s);
    if (me->form != NULL)
        XtVaSetValues(me->start_btn,
                      XmNleftAttachment,   XmATTACH_FORM,
                      XmNrightAttachment,  XmATTACH_WIDGET,
                      XmNrightWidget,      me->stop_btn,
                      XmNtopAttachment,    XmATTACH_FORM,
                      XmNbottomAttachment, XmATTACH_FORM,
                      NULL);
    TuneButtonKnob(me->start_btn, &(opts.button), 0);
    XtAddCallback (me->start_btn, XmNactivateCallback, StartCB, me);

    return ABSTRZE(me->form != NULL? me->form : me->start_btn);
}

static void NoColorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}

static knobs_vmt_t KNOB_SCENARIO_VMT  = {SCENARIO_Create_m,  NULL, NULL, NoColorize_m, NULL};

static knobs_widgetinfo_t scenario_widgetinfo[] =
{
    {LOGD_SCENARIO, "scenario", &KNOB_SCENARIO_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  scenario_widgetset = {scenario_widgetinfo, NULL};

void RegisterScenarioWidgetset(void)
{
  static int  is_registered = 0;

    if (is_registered) return;
    is_registered = 1;
    KnobsRegisterWidgetset(&scenario_widgetset);
}
