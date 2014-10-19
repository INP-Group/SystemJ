#include "Knobs_includes.h"

static XhWidget Create_m(knobinfo_t *ki, XhWidget parent)
{
  Widget    w;
  char     *text;
  Pixel     lo, hi;

    if (!get_knob_label(ki, &text)) text = "   ";
  
    w = XtVaCreateManagedWidget("light",
                                xmTextWidgetClass,
                                CNCRTZE(parent),
                                XmNvalue,                 text,
                                XmNcolumns,               strlen(text),
                                XmNcursorPositionVisible, False,
                                XmNeditable,              False,
                                XmNtraversalOn,           False,
                                NULL);

    XtVaGetValues(w, XmNtopShadowColor, &hi, XmNbottomShadowColor, &lo, NULL);
    XtVaSetValues(w, XmNtopShadowColor,  lo, XmNbottomShadowColor,  hi, NULL);
    
    HookPropsWindow(ki, w);
    
    return ABSTRZE(w);
}

static void SetValue_m(knobinfo_t *ki __attribute__((unused)), double v __attribute__((unused)))
{
}

knobs_vmt_t KNOBS_LIGHT_VMT = {Create_m, NULL, SetValue_m, CommonColorize_m, NULL};
