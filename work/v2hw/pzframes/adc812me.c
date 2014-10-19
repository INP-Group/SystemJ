#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_main.h"

#include "adc812me_gui.h"


int main(int argc, char *argv[])
{
  fastadc_gui_t       gui;
  pzframe_gui_dscr_t *gkd = adc812me_get_gui_dscr();

    FastadcGuiInit(&gui, pzframe2fastadc_gui_dscr(gkd));
    return FastadcMain(argc, argv,
                       "adc812me", "ADC812ME",
                       &(gui.g), gkd);
}
