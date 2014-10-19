#include "Knobs_includes.h"

#include "cxdata.h" /* It is here instead of _includes.h since no other Knobs_*.c may use it */

#include "Knobs_text_widget.h"
#include "Knobs_alarmonoffled_widget.h"
#include "Knobs_dial_widget.h"
#include "Knobs_button_widget.h"
#include "Knobs_selector_widget.h"
#include "Knobs_choicebs_widget.h"
#include "Knobs_light_widget.h"
#include "Knobs_slider_widget.h"
#include "Knobs_sep_widget.h"
#include "Knobs_label_widget.h"
#include "Knobs_usertext_widget.h"


#define KNOB_LINE(name) {LOGD_ ## name, #name, &KNOBS_ ## name ## _VMT}
static knobs_widgetinfo_t widgetinfo[] =
{
    KNOB_LINE(TEXT),
    KNOB_LINE(TEXTND),
    KNOB_LINE(ALARM),
    KNOB_LINE(ONOFF),
    KNOB_LINE(GREENLED),
    KNOB_LINE(AMBERLED),
    KNOB_LINE(REDLED),
    KNOB_LINE(BLUELED),
    KNOB_LINE(DIAL),
    KNOB_LINE(BUTTON),
    KNOB_LINE(SELECTOR),
    KNOB_LINE(LIGHT),
    KNOB_LINE(HSLIDER),
    KNOB_LINE(VSLIDER),
    KNOB_LINE(ARROW_LT),
    KNOB_LINE(ARROW_RT),
    KNOB_LINE(ARROW_UP),
    KNOB_LINE(ARROW_DN),
    KNOB_LINE(KNOB),
    KNOB_LINE(GAUGE),
    KNOB_LINE(HSEP),
    KNOB_LINE(VSEP),
    KNOB_LINE(HLABEL),
    KNOB_LINE(VLABEL),
    KNOB_LINE(RADIOBTN),
    KNOB_LINE(CHOICEBS),
    KNOB_LINE(USERTEXT),
    {0, NULL, NULL}
};
static knobs_widgetset_t  std_widgetset   = {widgetinfo, NULL};
static knobs_widgetset_t *first_widgetset = &std_widgetset;

void KnobsRegisterWidgetset(knobs_widgetset_t *widgetset)
{
    widgetset->next = first_widgetset;
    first_widgetset = widgetset;
}

knobs_vmt_t   *KnobsGetVmtByID  (int id)
{
  knobs_widgetset_t *widgetset;
  knobs_widgetinfo_t *item;

    for (widgetset = first_widgetset;
         widgetset != NULL;
         widgetset = widgetset->next)
        for (item = widgetset->list;  item->vmt != NULL;  item++)
            if (item->look_id == id) return item->vmt;

    return NULL;
}

knobs_vmt_t   *KnobsGetVmtByName(const char *name)
{
  knobs_widgetset_t *widgetset;
  knobs_widgetinfo_t *item;

    for (widgetset = first_widgetset;
         widgetset != NULL;
         widgetset = widgetset->next)
        for (item = widgetset->list;  item->vmt != NULL;  item++)
            if (strcasecmp(item->name, name) == 0) return item->vmt;

    return NULL;
}

int            KnobsName2ID     (const char *name)
{
  knobs_widgetset_t *widgetset;
  knobs_widgetinfo_t *item;

    for (widgetset = first_widgetset;
         widgetset != NULL;
         widgetset = widgetset->next)
        for (item = widgetset->list;  item->vmt != NULL;  item++)
            if (strcasecmp(item->name, name) == 0) return item->look_id;

    return -1;
}


int          CreateKnob(Knob k, XhWidget parent)
{
#if 1
  knobs_vmt_t *vmt;
  char        *tp;

    /* Find appropriate widget type */
    vmt = KnobsGetVmtByID(k->look);
    if (vmt == NULL)
    {
        fprintf(stderr, "%s::%s: no widget type %d (%s/%s)\n",
                __FILE__, __FUNCTION__, k->look, k->ident, k->label);
        vmt = KnobsGetVmtByID(LOGD_TEXT);
    }

    if (vmt->Colorize == NULL) vmt->Colorize = CommonColorize_m;
    
    /* Create the widget */
    k->vmtlink   = vmt;
    k->indicator = vmt->Create(k, parent);

    /* Colorize it */
    XtVaGetValues(CNCRTZE(k->indicator),
                  XmNforeground, &(k->wdci.deffg),
                  XmNbackground, &(k->wdci.defbg),
                  NULL);
    if (k->vmtlink->Colorize != NULL)
        k->vmtlink->Colorize(k, k->colstate);

    /* Attach tooltip */
    if (get_knob_tip(k, &tp))
        XhSetBalloon(k->indicator, tp);
#else
  XmString  s;
    
    k->indicator = XtVaCreateManagedWidget("zzz", xmLabelWidgetClass, parent,
                                            XmNlabelString, s = XmStringCreateSimple(k->ident),
                                            NULL);
    XmStringFree(s);
#endif

    return (k->indicator != NULL)? 0 : -1;
}

