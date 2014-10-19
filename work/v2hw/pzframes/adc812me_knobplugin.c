#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "adc812me_gui.h"
#include "adc812me_knobplugin.h"


static XhWidget ADC812ME_Create_m(knobinfo_t *ki, XhWidget parent)
{
  fastadc_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return FastadcKnobpluginDoCreate(ki, parent,
                                     me, NULL, adc812me_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_ADC812ME_VMT  = {ADC812ME_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t adc812me_widgetinfo[] =
{
    {0, "adc812me", &KNOB_ADC812ME_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  adc812me_widgetset = {adc812me_widgetinfo, NULL};


void  RegisterADC812ME_widgetset(int look_id)
{
    adc812me_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&adc812me_widgetset);
}
