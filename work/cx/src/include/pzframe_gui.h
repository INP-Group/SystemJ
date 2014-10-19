#ifndef __PZFRAME_GUI_H
#define __PZFRAME_GUI_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <X11/Intrinsic.h>

#include "fps_counter.h"
#include "paramstr_parser.h"
 
#include "Xh.h"
#include "Knobs.h"
#include "Chl.h"

#include "pzframe_data.h"


/********************************************************************/
struct _pzframe_gui_t_struct;
//--------------------------------------------------------------------

typedef Widget (*pzframe_gui_mkstdctl_t) (struct _pzframe_gui_t_struct *gui,
                                          Widget parent,
                                          int kind, int a, int b);

typedef Widget (*pzframe_gui_mkparknob_t)(struct _pzframe_gui_t_struct *gui,
                                          Widget parent, const char *spec, int pn);

typedef int    (*pzframe_gui_realize_t)  (struct _pzframe_gui_t_struct *gui,
                                          Widget parent,
                                          const char *srvrspec, int bigc_n,
                                          cda_serverid_t def_regsid, int base_chan);
typedef void   (*pzframe_gui_evproc_t)   (struct _pzframe_gui_t_struct *gui,
                                          int   reason,
                                          int   info_changed,
                                          void *privptr);
typedef void   (*pzframe_gui_newstate_t) (struct _pzframe_gui_t_struct *gui);
typedef void   (*pzframe_gui_do_renew_t) (struct _pzframe_gui_t_struct *gui,
                                          int   info_changed);

typedef struct
{
    pzframe_gui_realize_t   realize;
    pzframe_gui_evproc_t    evproc;
    pzframe_gui_newstate_t  newstate;
    pzframe_gui_do_renew_t  do_renew;
} pzframe_gui_vmt_t;

/********************************************************************/

enum
{
    PZFRAME_GUI_BHV_NORESIZE = 1 << 0,
};

typedef struct
{
    pzframe_type_dscr_t *ftd;
    int                  bhv;
    pzframe_gui_vmt_t    vmt; // Inheritance
} pzframe_gui_dscr_t;

typedef void (*pzframe_gui_dscr_filler_t)(pzframe_gui_dscr_t *);

/********************************************************************/

typedef struct
{
    void *p;
    int   n;
} pzframe_gui_dpl_t;

typedef struct
{
    int     foldctls;
    int     nocontrols;
    int     noleds;
    int     savebtn;
    char   *subsysname;
} pzframe_gui_look_t;

extern psp_paramdescr_t  pzframe_gui_text2look[];


typedef struct
{
    pzframe_gui_evproc_t  cb;
    void                 *privptr;
} pzframe_gui_cbinfo_t;

typedef struct _pzframe_gui_t_struct
{
    pzframe_data_t         *pfr;
    pzframe_gui_dscr_t     *d;

    Widget                  top;
    colalarm_t              curstate;

    pzframe_gui_look_t      look;

    Knob                    k_params[CX_MAX_BIGC_PARAMS];
    Knob                    k_regcns[CX_MAX_BIGC_PARAMS];

    Widget                  commons;
    Widget                  local_leds;
    ledinfo_t              *leds;
    Widget                  save_button;
    Widget                  loop_button;
    Widget                  once_button;
    Widget                  roller;
    Widget                  fps_show;
    Widget                  time_show;
    int                     roller_ctr;
    fps_ctr_t               fps_ctr;

    /* Param-passing-duplets */
    pzframe_gui_dpl_t       Param_prm  [CX_MAX_BIGC_PARAMS];

    // Callbacks
    pzframe_gui_cbinfo_t   *cb_list;
    int                     cb_list_allocd;
} pzframe_gui_t;
//-- protected -------------------------------------------------------
void PzframeGuiCallCBs(pzframe_gui_t *gui,
                       int            reason,
                       int            info_changed);

//////////////////////////////////////////////////////////////////////

void  PzframeGuiInit       (pzframe_gui_t *gui, pzframe_data_t *pfr, 
                            pzframe_gui_dscr_t *gkd);
void  PzframeGuiRealize    (pzframe_gui_t *gui);
void  PzframeGuiUpdate     (pzframe_gui_t *gui, int info_changed);

int   PzframeGuiAddEvproc  (pzframe_gui_t *gui,
                            pzframe_gui_evproc_t cb, void *privptr);

// "protected"?  For chidren only...
void  PzframeGuiMkCommons  (pzframe_gui_t *gui, Widget parent);
void  PzframeGuiMkLeds     (pzframe_gui_t *gui, Widget parent, int in_toolbar);
Widget PzframeGuiMkparknob (pzframe_gui_t *gui,
                            Widget parent, const char *spec, int pn);


#ifdef __cplusplus
}
#endif /* __cplusplus */
 

#endif /* __PZFRAME_GUI_H */
