#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "s2v_gui.h"
#include "s2v_knobplugin.h"


static XhWidget S2V_Create_m(knobinfo_t *ki, XhWidget parent)
{
  fastadc_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return FastadcKnobpluginDoCreate(ki, parent,
                                     me, NULL, s2v_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_S2V_VMT  = {S2V_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t s2v_widgetinfo[] =
{
    {0, "s2v", &KNOB_S2V_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  s2v_widgetset = {s2v_widgetinfo, NULL};


void  RegisterS2V_widgetset(int look_id)
{
    s2v_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&s2v_widgetset);
}
