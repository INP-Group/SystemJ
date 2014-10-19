#include <stdio.h>

#include "misc_macros.h"
#include "misclib.h"

#include "Chl.h"


static void CommandProc(XhWindow win, int cmd)
{
    ChlHandleStdCommand(win, cmd);
}


int main(int argc, char *argv[])
{
  int  r;

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             ChlGetStdToolslist(), CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
