#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "nadc333_gui.h"
#include "nadc333_knobplugin.h"


static XhWidget NADC333_Create_m(knobinfo_t *ki, XhWidget parent)
{
  fastadc_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return FastadcKnobpluginDoCreate(ki, parent,
                                     me, NULL, nadc333_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_NADC333_VMT  = {NADC333_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t nadc333_widgetinfo[] =
{
    {0, "nadc333", &KNOB_NADC333_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  nadc333_widgetset = {nadc333_widgetinfo, NULL};


void  RegisterNADC333_widgetset(int look_id)
{
    nadc333_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&nadc333_widgetset);
}
