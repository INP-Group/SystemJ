/*********************************************************************
*                                                                    *
*  cx-console.c                                                      *
*                                                                    *
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "cxlib.h"
#include "misc_macros.h"
#include "misclib.h"


static int istty;
static int conn;

static char *hostname = NULL;
static char *username = NULL;


static void ParseCommandLine(int argc, char *argv[])
{
  __label__  ADVICE_HELP, PRINT_HELP;

  int  c;		/* Option character */
  int  err = 0;		/* ++'ed on errors */

    while ((c = getopt(argc, argv, "hl:")) != EOF)
            switch (c)
            {
                case 'h':
                    goto PRINT_HELP;

                case 'l':
                    username = optarg;
                    break;

                case '?':
                    err++;
            }

    if (err) goto ADVICE_HELP;

    if (optind < argc)  hostname = argv[optind++];

    if (optind < argc)
    {
        fprintf(stderr, "%s: extra argument(s) \"%s\"...\n",
                        argv[0], argv[optind]);
        goto ADVICE_HELP;
    }

    return;

 ADVICE_HELP:
    fprintf(stderr, "Try `%s -h' for more information.\n", argv[0]);
    exit(1);

 PRINT_HELP:
    printf("Usage: %s [options] [host]\n"
           "\n"
           "Options:\n"
           "  -h        display this help and exit\n"
           "  -l USER   login as user USER\n"
           , argv[0]);
    exit(0);
}


static int prompt(void)
{
    if (!istty)  return 1;

    printf("%s", cx_getprompt(conn));
    fflush(stdout);

    return 1;
}

static void notifier(int         cd      __attribute__((unused)),
                     int         reason,
                     void       *privptr __attribute__((unused)),
                     const void *info)
{
    ////fprintf(stderr, "%s: reason=%d\n", __FUNCTION__, reason);
    /* Okay, what's the problem? */
    switch (reason)
    {
        case CAR_ECHO:
            printf("\n\a%s", (const char *)info);
            break;

        case CAR_KILLED:
            fprintf(stderr, "\n%s cx-console: killed\n", strcurtime());
            exit(1);

        case CAR_ERRCLOSE:
            /* Pull out `errno' value for this connection  */
            /* Note: it is okay to use cx_result() from within a notifier here,
             since calling cx_result() for *closed* connections is valid --
             see cx_result()'s code and comments */
            cx_result(conn);
            fprintf(stderr, "\n%s cx-console: connection is closed on error: %s\n",
                    strcurtime(), cx_strerror(errno));
            exit(1);
    }
}

int main(int argc, char *argv[])
{
  char        cmdline[256];
  const char *reply;
  int         r;

    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    /* Decide whether we should print prompts, banners etc. */
    istty = isatty(0) && isatty(1);

    /* Parse the command line */
    ParseCommandLine(argc, argv);
    if (!hostname)  hostname = "localhost";
    if (!username)  username = getenv("LOGNAME");

    /* Log into CX */
    conn = cx_consolelogin(hostname, username, &reply);
    if (conn < 0)
    {
        fprintf(stderr, "%s %s: %s: %s\n",
                strcurtime(), argv[0], hostname, cx_strerror(errno));
        
        return 1;
    }
    if (istty) printf("%s", reply);
    cx_setnotifier(conn, notifier, NULL);

    /* Now, get the commands */
    while (prompt()  &&  fgets(cmdline, sizeof(cmdline), stdin) != NULL)
    {
        if (cmdline[0] == '\0'  ||  cmdline[0] == '\n') continue;

        r = cx_execcmd(conn, cmdline, &reply);

        if (r < 0)                      {cx_perror(argv[0]); cx_close(conn); return 1;}
        if (r == 0  &&  reply != NULL)  printf("%s", reply);
        if (r > 0)                      break;
    }

    cx_close(conn);

    return 0;
}
