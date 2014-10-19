#include "vcamimg_data.h"
#include "vcamimg_gui.h"
#include "vcamimg_main.h"

#include "ottcam_gui.h"


int main(int argc, char *argv[])
{
  vcamimg_gui_t       gui;
  pzframe_gui_dscr_t *gkd = ottcam_get_gui_dscr();

    VcamimgGuiInit(&gui, pzframe2vcamimg_gui_dscr(gkd));
    return VcamimgMain(argc, argv,
                       "ottcam", "OttCam",
                       &(gui.g), gkd);
}
