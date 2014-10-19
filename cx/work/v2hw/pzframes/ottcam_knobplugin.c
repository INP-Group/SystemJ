#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "vcamimg_data.h"
#include "vcamimg_gui.h"
#include "vcamimg_knobplugin.h"

#include "ottcam_gui.h"
#include "ottcam_knobplugin.h"


static XhWidget OTTCAM_Create_m(knobinfo_t *ki, XhWidget parent)
{
  vcamimg_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return VcamimgKnobpluginDoCreate(ki, parent,
                                     me, NULL, ottcam_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_OTTCAM_VMT  = {OTTCAM_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t ottcam_widgetinfo[] =
{
    {0, "ottcam", &KNOB_OTTCAM_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  ottcam_widgetset = {ottcam_widgetinfo, NULL};


void  RegisterOTTCAM_widgetset(int look_id)
{
    ottcam_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&ottcam_widgetset);
}
