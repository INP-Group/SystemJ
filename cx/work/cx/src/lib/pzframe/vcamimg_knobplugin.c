#include "vcamimg_knobplugin.h"


XhWidget VcamimgKnobpluginDoCreate(knobinfo_t *ki, XhWidget parent,
                                   vcamimg_knobplugin_t         *kpn,
                                   pzframe_knobplugin_realize_t  kp_realize,
                                   pzframe_gui_dscr_t *gkd,
                                   psp_paramdescr_t *privrec_tbl, void *privrec)
{
  XhWidget            ret = NULL;

  vcamimg_gui_t      *my_gui = &(kpn->g);
  vcamimg_gui_dscr_t *my_gkd = pzframe2vcamimg_gui_dscr(gkd);

  psp_paramdescr_t   *dcnv_table  = NULL;
  char               *buf_to_free = NULL;
  psp_paramdescr_t   *gui_table   = NULL;

    VcamimgGuiInit(my_gui, my_gkd);

    if ((dcnv_table = VcamimgDataCreateText2DcnvTable(my_gkd->vtd, &buf_to_free)) == NULL)
        goto FINISH;

    if ((gui_table = VcamimgGuiCreateText2LookTable(my_gkd)) == NULL)
        goto FINISH;

    ret = PzframeKnobpluginDoCreate(ki, parent,
                                    &(kpn->kp),   kp_realize,
                                    &(my_gui->g), gkd,
                                    dcnv_table,  &(my_gui->v.dcnv),
                                    gui_table,   &(my_gui->look),
                                    privrec_tbl, privrec);

 FINISH:
    safe_free(buf_to_free);
    safe_free(dcnv_table);
    safe_free(gui_table);

    if (ret == NULL) ret = PzframeKnobpluginRedRectWidget(parent);

    return ret;
}
