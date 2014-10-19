#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_main.h"

#include "s2v_gui.h"


int main(int argc, char *argv[])
{
  fastadc_gui_t       gui;
  pzframe_gui_dscr_t *gkd = s2v_get_gui_dscr();

    FastadcGuiInit(&gui, pzframe2fastadc_gui_dscr(gkd));
    return FastadcMain(argc, argv,
                       "s2v", "S2V",
                       &(gui.g), gkd);
}
