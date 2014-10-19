#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "misc_macros.h"

#include "remdrvlet.h"
#include "remdrv_proto.h"


static int         option_port          = REMDRV_DEFAULT_PORT;
static int         option_dontdaemonize = 0;
static const char *argv_params[2];


static void ShowHelp(const char *argv0)
{
    fprintf(stderr, "Usage: %s [-d] [-p port] [prefix [suffix]]\n", argv0);
    fprintf(stderr, "    '-d' switch prevents daemonization\n");
    exit(1);
}

static void ParseCommandLine(int argc, char *argv[])
{
  int             argn;
  int             pidx;
    
    for (argn = 1, pidx = 0;  argn < argc;  argn++)
    {
        if (argv[argn][0] == '-')
        {
            switch (argv[argn][1])
            {
                case '\0':
                    fprintf(stderr, "%s: '-' requires a switch letter\n", argv[0]);

                case 'h':
                    ShowHelp(argv[0]);

                case 'd':
                    option_dontdaemonize = 1;
                    break;

                case 'p':
                    argn++;
                    if (argn >= argc)
                    {
                        fprintf(stderr, "%s: '-p' requires a port number", argv[0]);
                        ShowHelp(argv[0]);
                    }
                    option_port = atoi(argv[argn]);
                    if (option_port <= 0)
                    {
                        fprintf(stderr, "%s: bad port spec '%s'\n", argv[0], argv[argn]);
                        exit(1);
                    }
                    break;

                default:
                    fprintf(stderr, "%s: unknown option '%s'\n", argv[0], argv[argn]);
                    ShowHelp(argv[0]);
            }
        }
        else
        {
            if (pidx < countof(argv_params)) argv_params[pidx++] = argv[argn];
        }
    }
}


static void SIGCHLD_handler(int signum  __attribute__((unused)));

static void InstallSIGCHLD_handler(void)
{
    signal(SIGCHLD, SIGCHLD_handler);
}

static void SIGCHLD_handler(int signum  __attribute__((unused)))
{
  int  status;
    
//    fprintf(stderr, "SIGCHLD\n");
    while (waitpid(-1, &status, WNOHANG) > 0);
    InstallSIGCHLD_handler();
}

int remdrvlet_srvmain(int argc, char *argv[],
                      remdrvlet_newdev_t newdev_p,
                      remdrvlet_newcon_t newcon_p)
{
    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* Intercept signals */
    signal(SIGPIPE, SIG_IGN);
    InstallSIGCHLD_handler();

    ParseCommandLine(argc, argv);
    return remdrvlet_listener(argv[0],
                              option_port, option_dontdaemonize,
                              newdev_p,
                              newcon_p);
}

const char *remdrvlet_srvmain_param(int n)
{
    if (n >= 0  &&  n < countof(argv_params)) return argv_params[n];
    else                                      return NULL;
}
