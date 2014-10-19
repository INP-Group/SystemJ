#ifndef __KNOBSI_H
#define __KNOBSI_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include "paramstr_parser.h"
#include "Knobs_typesP.h"


/* "Abstraction" (copied from Xh_utils.h) */
#define CNCRTZE(w) ((Widget)w)
#define ABSTRZE(w) ((XhWidget)w)


extern psp_lkp_t Knobs_colors_lkp[];
extern psp_lkp_t Knobs_sizes_lkp [];

extern const char  Knobs_wdi_equals_c; 
extern const char *Knobs_wdi_separators;
extern const char *Knobs_wdi_terminators;


/* Initialization-related stuff */

int ParseWiddepinfo(knobinfo_t *ki,
                    void *rec, size_t rec_size, psp_paramdescr_t *table);
int GetKnobLabel   (knobinfo_t *ki, char **label_p);

void MaybeAssignPixmap(Widget w, const char *label, int from_widdepinfo);


/* Colorization */

void ChooseWColors(knobinfo_t *ki, colalarm_t newstate,
                   Pixel *newfg, Pixel *newbg);

void CommonColorize_m(knobinfo_t *ki, colalarm_t newstate);

void AllocStateGCs(XhWidget w, GC table[], int norm_idx);


/* User interface */

void UserBeganInputEditing(knobinfo_t *ki);
void CancelInputEditing   (knobinfo_t *ki);
void SetControlValue      (knobinfo_t *ki, double v, int fromlocal);

void KnobsMouse3Handler(Widget     w  __attribute__((unused)),
                        XtPointer  closure,
                        XEvent    *event,
                        Boolean   *continue_to_dispatch);
void HookPropsWindow(knobinfo_t *ki, Widget w);


/* Misc. UI-related functions */

int  CheckForDoubleClick(Widget   w,
                         XEvent  *event,
                         Boolean *continue_to_dispatch);


/* Common-textfield interface */

enum
{
    MOD_FINEGRAIN_MASK = ControlMask,
    MOD_BIGSTEPS_MASK  = Mod1Mask,
    MOD_NOSEND_MASK    = ShiftMask
};

Widget  CreateTextValue(knobinfo_t *ki, Widget parent);
Widget  CreateTextInput(knobinfo_t *ki, Widget parent);
void    SetTextString  (knobinfo_t *ki, Widget w, double v);

void    SetCursorCallback (Widget w);
int     ExtractDoubleValue(Widget w, double *result);
int     ExtractIntValue   (Widget w, int    *result);


/* Common-button interface */

enum
{
    TUNE_BUTTON_KNOB_F_NO_PIXMAP = 1 << 0,
    TUNE_BUTTON_KNOB_F_NO_FONT   = 1 << 1,
};

typedef struct
{
    int   color;
    int   bg;
    int   size;
    int   bold;
    char *icon;
} Knobs_buttonopts_t;
extern psp_paramdescr_t text2Knobs_buttonopts[];

void              TuneButtonKnob(Widget              w, 
                                 Knobs_buttonopts_t *opts_p, 
                                 int                 flags);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __KNOBSI_H */
