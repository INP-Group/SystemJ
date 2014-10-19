#include "vacclient_includes.h"


static XhWindow  onewin;
static char      myfilename[] = __FILE__;

int main(int argc, char *argv[])
{
    OpenSubsysDescription(myfilename, argv[0]);
    onewin = CreateApplicationWindow(argc, argv);
    CreateMeat(onewin, argc, argv);
    RunApplication();

    return 0;
}
