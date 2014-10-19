#include "fastadc_knobplugin.h"


XhWidget FastadcKnobpluginDoCreate(knobinfo_t *ki, XhWidget parent,
                                   fastadc_knobplugin_t         *kpn,
                                   pzframe_knobplugin_realize_t  kp_realize,
                                   pzframe_gui_dscr_t *gkd,
                                   psp_paramdescr_t *privrec_tbl, void *privrec)
{
  XhWidget            ret = NULL;

  fastadc_gui_t      *my_gui = &(kpn->g);
  fastadc_gui_dscr_t *my_gkd = pzframe2fastadc_gui_dscr(gkd);

  psp_paramdescr_t   *dcnv_table  = NULL;
  char               *buf_to_free = NULL;
  psp_paramdescr_t   *gui_table   = NULL;

    FastadcGuiInit(my_gui, my_gkd);

    if ((dcnv_table = FastadcDataCreateText2DcnvTable(my_gkd->atd, &buf_to_free)) == NULL)
        goto FINISH;

    if ((gui_table = FastadcGuiCreateText2LookTable(my_gkd)) == NULL)
        goto FINISH;

    ret = PzframeKnobpluginDoCreate(ki, parent,
                                    &(kpn->kp),   kp_realize,
                                    &(my_gui->g), gkd,
                                    dcnv_table,  &(my_gui->a.dcnv),
                                    gui_table,   &(my_gui->look),
                                    privrec_tbl, privrec);

 FINISH:
    safe_free(buf_to_free);
    safe_free(dcnv_table);
    safe_free(gui_table);

    if (ret == NULL) ret = PzframeKnobpluginRedRectWidget(parent);

    ////fprintf(stderr, "\"%s\"/%s: %d\n", ki->label, kpn->kp.g->d->ftd->type_name, kpn->kp.g->pfr->bigc_sid);
    return ret;
}
