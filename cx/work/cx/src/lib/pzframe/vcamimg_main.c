#include "vcamimg_main.h"


int  VcamimgMain(int argc, char *argv[],
                 const char *def_app_name, const char *def_app_class,
                 pzframe_gui_t *gui, pzframe_gui_dscr_t *gkd)
{
  vcamimg_gui_t      *my_gui = pzframe2vcamimg_gui     (gui);
  vcamimg_gui_dscr_t *my_gkd = pzframe2vcamimg_gui_dscr(gkd);

  psp_paramdescr_t   *dcnv_table  = NULL;
  char               *buf_to_free = NULL;
  psp_paramdescr_t   *gui_table   = NULL;

    if ((dcnv_table = VcamimgDataCreateText2DcnvTable(my_gkd->vtd, &buf_to_free)) == NULL)
        return 1;

    if ((gui_table = VcamimgGuiCreateText2LookTable(my_gkd)) == NULL)
        return 1;

    return PzframeMain(argc, argv,
                       def_app_name, def_app_class,
                       gui, gkd,
                       dcnv_table, &(my_gui->v.dcnv),
                       gui_table,  &(my_gui->look));

    /* Note: dcnv_table, buf_to_free, gui_table should be free()d, but
       there's no big reason to do this just before exit() */
}
