#include "Chl_includes.h"


typedef struct
{
    cda_serverid_t  mainsid;
    int             srvcount;
    ledinfo_t      *leds;
} leds_privrec_t;


static void LEDS_KeepaliveProc(XtPointer     closure,
                               XtIntervalId *id      __attribute__((unused)))
{
  leds_privrec_t *me= (leds_privrec_t *)closure;
    
    XtAppAddTimeOut(xh_context, 1000, LEDS_KeepaliveProc, closure);

    leds_update(me->leds, me->srvcount, me->mainsid);
}


static XhWidget LEDS_Create_m(knobinfo_t *ki, XhWidget parent)
{
  leds_privrec_t *me;
  XhWidget        leds_grid;
    
    /* Allocate the private info structure... */
    if ((ki->widget_private = XtMalloc(sizeof(leds_privrec_t))) == NULL)
        return NULL;
    me = ki->widget_private;
    bzero(me, sizeof(*me));

    /*  */
    leds_grid = XhCreateGrid(parent, "alarmLeds");
    XhGridSetGrid   (leds_grid, 0, 0);
    XhGridSetPadding(leds_grid, 0, 0);
    
    /*  */
    me->mainsid  = ChlGetMainsid(XhWindowOf(parent));
    me->srvcount = cda_status_lastn(me->mainsid) + 1;
    if ((me->leds = XtCalloc(me->srvcount, sizeof(*(me->leds)))) != NULL)
        leds_create(leds_grid, -15, me->leds, me->srvcount, me->mainsid);

    LEDS_KeepaliveProc((XtPointer)me, NULL);
    
    return leds_grid;
}

static void LEDS_SetValue_m(knobinfo_t *ki, double v __attribute__((unused)))
{
  leds_privrec_t *me = ki->widget_private;

    leds_update(me->leds, me->srvcount, me->mainsid);
}

static void Colorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any colorization attempts */
}

static knobs_vmt_t KNOB_LEDS_VMT = {LEDS_Create_m, NULL, LEDS_SetValue_m, Colorize_m, NULL};

static knobs_widgetinfo_t leds_widgetinfo[] =
{
    {LOGD_SRV_LEDS, "srv_leds",  &KNOB_LEDS_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  leds_widgetset = {leds_widgetinfo, NULL};

void RegisterLedsKnob(void)
{
  static int  is_registered = 0;

    if (is_registered) return;
    is_registered = 1;
    KnobsRegisterWidgetset(&leds_widgetset);
}
