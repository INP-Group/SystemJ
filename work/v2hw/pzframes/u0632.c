#include "u0632_data.h"
#include "u0632_gui.h"
#include "pzframe_main.h"


int main(int argc, char *argv[])
{
  u0632_gui_t         gui;
  pzframe_gui_dscr_t *gkd = u0632_get_gui_dscr();

    U0632GuiInit(&gui);

    return PzframeMain(argc, argv,
                       "u0632", "U0632",
                       &(gui.g), gkd,
                       NULL, NULL,
                       NULL, NULL /*!!!type/look*/);
}
