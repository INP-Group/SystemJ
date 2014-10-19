#include "Knobs_includes.h"


static XhWidget DoCreate(knobinfo_t *ki, XhWidget parent, Boolean vertical)
{
  Widget         w;
  unsigned char  orientation = vertical? XmVERTICAL : XmHORIZONTAL;
  unsigned char  septype     = XmSHADOW_ETCHED_IN;
    
    ki->behaviour |= KNOB_B_VALIGN_FILL | KNOB_B_HALIGN_FILL;
  
    w = XtVaCreateManagedWidget("separator",
                                xmSeparatorWidgetClass,
                                CNCRTZE(parent),
                                XmNorientation,   orientation,
                                XmNseparatorType, septype,
                                XmNhighlightThickness, 0,
                                NULL);
  
    /* The HookPropsWindow() should NOT be called for decorative widgets */
  
    return ABSTRZE(w);
}

static XhWidget HCreate_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, False);
}

static XhWidget VCreate_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, True);
}


knobs_vmt_t KNOBS_HSEP_VMT = {HCreate_m, NULL, NULL, NULL, NULL};
knobs_vmt_t KNOBS_VSEP_VMT = {VCreate_m, NULL, NULL, NULL, NULL};
