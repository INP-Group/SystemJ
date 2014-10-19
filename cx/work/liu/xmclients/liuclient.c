#include <stdio.h>

#include "misclib.h"

#include "Chl.h"

#include "adc200me_knobplugin.h"
#include "adc812me_knobplugin.h"
#include "one200me_calc_knobplugin.h"
#include "manyadcs_knobplugin.h"
#include "two812ch_knobplugin.h"
#include "beamprof_knobplugin.h"
#include "avgdiff_knobplugin.h"
#include "key_offon_knobplugin.h"

#define __LIUCLIENT_C
#include "liuclient.h"


static void CommandProc(XhWindow win, int cmd)
{
    switch (cmd)
    {
        default:
            ChlHandleStdCommand(win, cmd);
    }
}

static void lclregchg_proc(int lreg_n, double v)
{
  static int notfirst_cntr_chg = 0;

    //fprintf(stderr, "reg#%d:=%8.3f\n", lreg_n, v);
    if (lreg_n == REG_T16_SLOW_CNTR)
    {
        if (notfirst_cntr_chg) MANYADCS_SaveAll();
        notfirst_cntr_chg = 1;
    }
}

int main(int argc, char *argv[])
{
  int  r;

    XhSetColorBinding("GRAPH_REPERS", "#00FF00");

    RegisterADC200ME_widgetset     (LOGD_LIU_ADC200ME_ONE);
    RegisterADC812ME_widgetset     (LOGD_LIU_ADC812ME_ONE);
    RegisterONE200ME_CALC_widgetset(LOGD_LIU_ADC200ME_ONE_CALC);
    RegisterTWO812CH_widgetset     (LOGD_LIU_TWO812CH);
    RegisterBEAMPROF_widgetset     (LOGD_LIU_BEAMPROF);
    RegisterAVGDIFF_widgetset      (LOGD_LIU_AVGDIFF);
    RegisterMANYADCS_widgetset     (LOGD_LIU_ADC200ME_ANY);
    RegisterKEY_OFFON_widgetset    (LOGD_LIU_KEY_OFFON);

    cda_set_lclregchg_cb(lclregchg_proc);

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             ChlGetStdToolslist(), CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
