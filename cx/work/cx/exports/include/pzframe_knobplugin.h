#ifndef __PZFRAME_KNOBPLUGIN_H
#define __PZFRAME_KNOBPLUGIN_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "Knobs_typesP.h"
#include "pzframe_gui.h"


struct _pzframe_knobplugin_t_struct;

typedef int  (*pzframe_knobplugin_realize_t)
    (struct _pzframe_knobplugin_t_struct *kpn,
     Knob                                 k);

typedef void (*pzframe_knobplugin_evproc_t)(pzframe_data_t *pfr,
                                            int   reason,
                                            int   info_changed,
                                            void *privptr);

typedef struct
{
    pzframe_knobplugin_evproc_t  cb;
    void                        *privptr;
} pzframe_knobplugin_cb_info_t;


typedef struct _pzframe_knobplugin_t_struct
{
    pzframe_gui_t                *g;

    // Callbacks
    int                           low_level_registered;
    pzframe_knobplugin_cb_info_t *cb_list;
    int                           cb_list_allocd;
} pzframe_knobplugin_t;


XhWidget PzframeKnobpluginDoCreate(knobinfo_t *ki, XhWidget parent,
                                   pzframe_knobplugin_t         *kpn,
                                   pzframe_knobplugin_realize_t  kp_realize,
                                   pzframe_gui_t *gui, pzframe_gui_dscr_t *gkd,
                                   psp_paramdescr_t *table2, void *rec2,
                                   psp_paramdescr_t *table4, void *rec4,
                                   psp_paramdescr_t *table7, void *rec7);



void     PzframeKnobpluginNoColorize_m(knobinfo_t *ki  __attribute__((unused)),
                                       colalarm_t newstate  __attribute__((unused)));

XhWidget PzframeKnobpluginRedRectWidget(XhWidget parent);


/* Public services */
pzframe_data_t
    *PzframeKnobpluginRegisterCB(Knob                         k,
                                 const char                  *rqd_type_name,
                                 pzframe_knobplugin_evproc_t  cb,
                                 void                        *privptr,
                                 const char                 **name_p);
pzframe_gui_t
    *PzframeKnobpluginGetGui    (Knob                         k,
                                 const char                  *rqd_type_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PZFRAME_KNOBPLUGIN_H */
