#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_main.h"

#include "c061621_gui.h"


int main(int argc, char *argv[])
{
  fastadc_gui_t       gui;
  pzframe_gui_dscr_t *gkd = c061621_get_gui_dscr();

    FastadcGuiInit(&gui, pzframe2fastadc_gui_dscr(gkd));
    return FastadcMain(argc, argv,
                       "c061621", "C061621",
                       &(gui.g), gkd);
}
