#include <stdio.h>

#include "misc_macros.h"
#include "misclib.h"

#include "Chl.h"

#include "kozclient.h"


static int KOZ_TABLE_Create_m(XhWidget parent, ElemInfo e)
{
    return -1;
}


chl_elemdescr_t custom_table[] =
{
    {ELEM_KOZ_TABLE, "koz_table", KOZ_TABLE_Create_m},
    {0},
};



static void CommandProc(XhWindow win, int cmd)
{
    ChlHandleStdCommand(win, cmd);
}


int main(int argc, char *argv[])
{
  int            r;

    ChlSetCustomElementsTable(custom_table);

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             ChlGetStdToolslist(), CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
