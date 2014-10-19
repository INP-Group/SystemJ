#include <stdio.h>

#include "misclib.h"

#include "cda.h"
#include "Chl.h"
#include "Knobs.h"
#include "Knobs_typesP.h"

#include "bpmclient.h"

#include "bpmclient_profile_knobplugin.h"
#include "bpmclient_graph_knobplugin.h"


static knobs_widgetinfo_t bpm_widgetinfo[] =
{
    {LOGD_BPM_GRAPH,   "bpm_graph",   &BPM_GRAPH_VMT},
    {LOGD_BPM_PROFILE, "bpm_profile", &BPM_PROFILE_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  bpm_widgetset   = {bpm_widgetinfo, NULL};


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
  int            r;
    
    KnobsRegisterWidgetset(&bpm_widgetset);

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             ChlGetStdToolslist(), CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
