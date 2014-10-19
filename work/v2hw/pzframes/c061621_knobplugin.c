#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "c061621_gui.h"
#include "c061621_knobplugin.h"


static XhWidget C061621_Create_m(knobinfo_t *ki, XhWidget parent)
{
  fastadc_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return FastadcKnobpluginDoCreate(ki, parent,
                                     me, NULL, c061621_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_C061621_VMT  = {C061621_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t c061621_widgetinfo[] =
{
    {0, "c061621", &KNOB_C061621_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  c061621_widgetset = {c061621_widgetinfo, NULL};


void  RegisterC061621_widgetset(int look_id)
{
    c061621_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&c061621_widgetset);
}
