#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "vcamimg_data.h"
#include "vcamimg_gui.h"
#include "vcamimg_knobplugin.h"

#include "tsycamv_gui.h"
#include "tsycamv_knobplugin.h"


static XhWidget TSYCAMV_Create_m(knobinfo_t *ki, XhWidget parent)
{
  vcamimg_knobplugin_t      *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));
    return VcamimgKnobpluginDoCreate(ki, parent,
                                     me, NULL, tsycamv_get_gui_dscr(),
                                     NULL, NULL);
}


static knobs_vmt_t KNOB_TSYCAMV_VMT  = {TSYCAMV_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t tsycamv_widgetinfo[] =
{
    {0, "tsycamv", &KNOB_TSYCAMV_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  tsycamv_widgetset = {tsycamv_widgetinfo, NULL};


void  RegisterTSYCAMV_widgetset(int look_id)
{
    tsycamv_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&tsycamv_widgetset);
}
