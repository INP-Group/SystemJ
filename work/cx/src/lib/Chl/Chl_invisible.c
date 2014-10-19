#include "Chl_includes.h"

static knobs_emethods_t invisible_emethods =
{
    Chl_E_SetPhysValue_m,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int ELEM_INVISIBLE_Create_m(XhWidget parent, ElemInfo e)
{
    e->container = XtVaCreateManagedWidget("", widgetClass, parent,
                                           XmNborderWidth, 0,
                                           XmNwidth,       1,
                                           XmNheight,      1,
                                           NULL);
    e->emlink = &invisible_emethods;

    return 0;
}
