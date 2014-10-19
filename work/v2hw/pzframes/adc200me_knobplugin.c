#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "adc200me_gui.h"
#include "adc200me_knobplugin.h"


static XhWidget ADC200ME_Create_m(knobinfo_t *ki, XhWidget parent)
{
  fastadc_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return FastadcKnobpluginDoCreate(ki, parent,
                                     me, NULL, adc200me_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_ADC200ME_VMT  = {ADC200ME_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t adc200me_widgetinfo[] =
{
    {0, "adc200me", &KNOB_ADC200ME_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  adc200me_widgetset = {adc200me_widgetinfo, NULL};


void  RegisterADC200ME_widgetset(int look_id)
{
    adc200me_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&adc200me_widgetset);
}
