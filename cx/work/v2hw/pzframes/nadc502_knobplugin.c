#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "nadc502_gui.h"
#include "nadc502_knobplugin.h"


static XhWidget NADC502_Create_m(knobinfo_t *ki, XhWidget parent)
{
  fastadc_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return FastadcKnobpluginDoCreate(ki, parent,
                                     me, NULL, nadc502_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_NADC502_VMT  = {NADC502_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t nadc502_widgetinfo[] =
{
    {0, "nadc502", &KNOB_NADC502_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  nadc502_widgetset = {nadc502_widgetinfo, NULL};


void  RegisterNADC502_widgetset(int look_id)
{
    nadc502_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&nadc502_widgetset);
}
