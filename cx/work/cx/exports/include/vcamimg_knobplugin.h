#ifndef __VCAMIMG_KNOBPLUGIN_H
#define __VCAMIMG_KNOBPLUGIN_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "pzframe_knobplugin.h"
#include "vcamimg_gui.h"


typedef struct _vcamimg_knobplugin_t_struct
{
    pzframe_knobplugin_t  kp;
    vcamimg_gui_t         g;
} vcamimg_knobplugin_t;


XhWidget VcamimgKnobpluginDoCreate(knobinfo_t *ki, XhWidget parent,
                                   vcamimg_knobplugin_t         *kpn,
                                   pzframe_knobplugin_realize_t  kp_realize,
                                   pzframe_gui_dscr_t *gkd,
                                   psp_paramdescr_t *privrec_tbl, void *privrec);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __VCAMIMG_KNOBPLUGIN_H */
