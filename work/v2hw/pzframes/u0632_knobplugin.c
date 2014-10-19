#include <stdio.h>

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "pzframe_knobplugin.h"

#include "u0632_gui.h"
#include "u0632_knobplugin.h"


typedef struct
{
    pzframe_knobplugin_t  kp;
    u0632_gui_t           g;
} u0632_knobplugin_t;


static XhWidget U0632_Create_m(knobinfo_t *ki, XhWidget parent)
{
  XhWidget            ret = NULL;
  u0632_knobplugin_t *me;

  pzframe_gui_dscr_t *gkd = u0632_get_gui_dscr();

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));

    U0632GuiInit(&(me->g)/*, gkd*/);

    ret = PzframeKnobpluginDoCreate(ki, parent,
                                    &(me->kp),  NULL,
                                    &(me->g.g), gkd,
                                    NULL, NULL,
                                    NULL, NULL /*!!!type/look, i=...*/,
                                    NULL, NULL);
    if (ret == NULL) ret = PzframeKnobpluginRedRectWidget(parent);

    return ret;
}


static knobs_vmt_t KNOB_U0632_VMT  = {U0632_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t u0632_widgetinfo[] =
{
    {0, "u0632", &KNOB_U0632_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  u0632_widgetset = {u0632_widgetinfo, NULL};


void  RegisterU0632_widgetset(int look_id)
{
    u0632_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&u0632_widgetset);
}
