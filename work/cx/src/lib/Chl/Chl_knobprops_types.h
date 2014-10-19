#ifndef __CHL_KNOBPROPS_TYPES_H
#define __CHL_KNOBPROPS_TYPES_H


#include "Xh_types.h"
#include "Knobs_types.h"


#ifndef RANGES_INLINE
  #define RANGES_INLINE 1
#endif

enum {NUM_PROP_FLAGS = 17}; /*!!! log2(CDA_FLAG_CALCERR) + 1 */

enum
{
    KPROPS_TXT_SHD   = 0,
    KPROPS_TXT_INP   = 1,
    KPROPS_TXT_COUNT = 2,

    KPROPS_MNX_MIN   = 0,
    KPROPS_MNX_MAX   = 1,
    KPROPS_MNX_COUNT = 2,
};
typedef struct
{
    int       editable;
    Widget    form;
    Widget    txt[KPROPS_TXT_COUNT][KPROPS_MNX_COUNT];
    Widget    dash;
    // ??? modification flagging?
} kprops_range_t;

typedef struct
{
    int       initialized;
    XhWidget  box;

    Widget  pth_dpy;
    Widget  nam_dpy;
    Widget  lbl_lbl;
    Widget  src_lbl;
    Widget  val_dpy;
    Widget  raw_dpy;
    Widget  age_dpy;
    Widget  rflags_leds[NUM_PROP_FLAGS];
    Widget  col_dpy;
    Widget  stp_inp;
    Widget  gcf_inp;
    Widget  fqd_inp;
    
    Knob      the_knob;

#if RANGES_INLINE
    kprops_range_t  z[3];
    int     is_closing;
#else
    Widget  range_lbl[3];
    
    struct
    {
        int       which;

        XhWidget  box;
        Widget    title;
        Widget    i1;
        Widget    i2;
        
        XhWidget  yesno_box;
        
        double    v1;
        double    v2;
    }         r;
#endif

    struct
    {
        int       ranges[3];
        int       step;
        int       grpcoeff;
        int       frqdiv;
    }         chg;

    double    mins[3];
    double    maxs[3];

    rflags_t  rflags_shown;
    colalarm_t  colstate_shown;
    XhPixel     col_deffg;
    XhPixel     col_defbg;
} kpropsdlg_t;


#endif /* __CHL_KNOBPROPS_TYPES_H */
