#include <stdio.h>


#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"

#include "one200me_calc_knobplugin.h"


typedef struct
{
    char *source;
} ONE200ME_CALC_privrec_t;

static psp_paramdescr_t text2ONE200ME_CALC_opts[] =
{
    PSP_P_MSTRING("source", ONE200ME_CALC_privrec_t, source,  NULL, 1000),
    PSP_P_END()
};


static XhWidget ONE200ME_CALC_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return XtVaCreateManagedWidget("", widgetClass, parent,
                                   XmNwidth,       10,
                                   XmNheight,      10,
                                   XmNborderWidth, 0,
                                   NULL);
}

static knobs_vmt_t KNOB_ONE200ME_CALC_VMT  = {ONE200ME_CALC_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t one200me_calc_widgetinfo[] =
{
    {0, "one200me_calc", &KNOB_ONE200ME_CALC_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  one200me_calc_widgetset = {one200me_calc_widgetinfo, NULL};


void  RegisterONE200ME_CALC_widgetset(int look_id)
{
    one200me_calc_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&one200me_calc_widgetset);
}
