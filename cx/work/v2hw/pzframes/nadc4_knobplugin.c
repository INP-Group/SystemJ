#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "nadc4_gui.h"
#include "nadc4_knobplugin.h"


static XhWidget NADC4_Create_m(knobinfo_t *ki, XhWidget parent)
{
  fastadc_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return FastadcKnobpluginDoCreate(ki, parent,
                                     me, NULL, nadc4_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_NADC4_VMT  = {NADC4_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t nadc4_widgetinfo[] =
{
    {0, "nadc4", &KNOB_NADC4_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  nadc4_widgetset = {nadc4_widgetinfo, NULL};


void  RegisterNADC4_widgetset(int look_id)
{
    nadc4_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&nadc4_widgetset);
}
