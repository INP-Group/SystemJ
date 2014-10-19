#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "misclib.h"

#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Knobs.h"
#include "Cdr.h"
#include "Chl.h"


char myfilename[] = __FILE__;

static char *fallbacks[] = {XH_STANDARD_FALLBACKS};

int main(int argc, char *argv[])
{
  const char   *app_name  = "pult";
  const char   *app_class = "Pult";
  char         *p;
  int           arg_n;
  char         *sysname;
  DataSubsys    ds;
  char         *defserver;

    set_signal(SIGPIPE, SIG_IGN);

    XhInitApplication(&argc, argv, app_name, app_class, fallbacks);

    arg_n = 1;
    
    /* Check if we are run "as is" instead of via symlink */
    p = strrchr(myfilename, '.');
    if (p != NULL) *p = '\0';
    p = strrchr(argv[0], '/');
    if (p != NULL) p++; else p = argv[0];
    if (strcmp(p, myfilename) == 0)
    {
        if (arg_n >= argc)
        {
            fprintf(stderr,
                    "%s: this program should be run either via symlink or as\n"
                    "%s <SYSNAME>\n",
                    argv[0], argv[0]);
            exit(1);
        }
        sysname = argv[arg_n++];
    }
    else
        sysname = p;

    /* Load the subsystem... */
    ds = CdrLoadSubsystem("file", sysname);
    if (ds == NULL)
    {
        fprintf(stderr, "%s(%s): %s\n",
                argv[0], sysname, CdrLastErr());
        exit(1);
    }

    /* ...and realize it */
    defserver = "";
    if (arg_n < argc)
        defserver = argv[arg_n++];
    
    if (CdrRealizeSubsystem(ds, defserver) < 0)
    {
        fprintf(stderr, "%s: Realize(%s): %s\n",
                argv[0], sysname, CdrLastErr());
        exit(1);
    }

    /* Finally, run the application */
    if (ChlRunSubsystem(ds) < 0)
    {
        fprintf(stderr, "%s: ChlRunSubsystem(%s): %s\n",
                argv[0], sysname, ChlLastErr());
        exit(1);
    }
    
    return 0;
}
