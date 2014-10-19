#ifndef __KNOBS_H
#define __KNOBS_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <time.h>

#include "Xh_types.h"

#include "Knobs_types.h"


#define KNOB_LABELTIP_NOLABELTIP_PFX   '\n'
#define KNOB_LABELTIP_NOLABELTIP_PFX_S "\n"

#define KNOB_LABEL_IMG_PFX_S           "=#!="


enum
{
    SIMPLEKNOB_OPT_READONLY = 1 << 1,
};


int          CreateKnob(Knob k, XhWidget parent);

typedef void (*simpleknob_cb_t)(Knob k, double v, void *privptr);

Knob         CreateSimpleKnob(const char      *spec,     // Knob specification
                              int              options,
                              XhWidget         parent,   // Parent widget
                              simpleknob_cb_t  cb,       // Callback proc
                              void            *privptr); // Pointer to pass to cb

void         SetKnobValue(Knob k, double v);
void         SetKnobState(Knob k, colalarm_t newstate);

XhWidget     GetKnobWidget(Knob k);

void ChooseKnobColors(int color, colalarm_t newstate,
                      XhPixel  deffg, XhPixel  defbg,
                      XhPixel *newfg, XhPixel *newbg);


XhWidget  KnobsGetElemContainer(ElemInfo e);
XhWidget *KnobsGetElemInnageP  (ElemInfo e);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __KNOBS_H */
