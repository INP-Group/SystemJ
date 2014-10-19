#include <stdio.h>

#include "misclib.h"

#include "Chl.h"

#include "nadc200_knobplugin.h"
#include "nadc333_knobplugin.h"
#include "nadc4_knobplugin.h"
#include "nadc502_knobplugin.h"
#include "adc200me_knobplugin.h"
#include "adc812me_knobplugin.h"
#include "s2v_knobplugin.h"
#include "c061621_knobplugin.h"
#include "ottcam_knobplugin.h"
#include "u0632_knobplugin.h"

#include "pzframeclient.h"


static void CommandProc(XhWindow win, int cmd)
{
    switch (cmd)
    {
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


int main(int argc, char *argv[])
{
  int  r;

    XhSetColorBinding("GRAPH_REPERS", "#00FF00");

    RegisterNADC200_widgetset (LOGD_NADC200);
    RegisterNADC333_widgetset (LOGD_NADC333);
    RegisterNADC4_widgetset   (LOGD_NADC4);
    RegisterNADC502_widgetset (LOGD_NADC502);
    RegisterADC200ME_widgetset(LOGD_ADC200ME);
    RegisterADC812ME_widgetset(LOGD_ADC812ME);
    RegisterS2V_widgetset     (LOGD_S2V);
    RegisterC061621_widgetset (LOGD_C061621);
    RegisterOTTCAM_widgetset  (LOGD_OTTCAM);
    RegisterU0632_widgetset   (LOGD_U0632);

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             ChlGetStdToolslist(), CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
