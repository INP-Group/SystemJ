#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "misc_macros.h"
#include "misclib.h"

#include "cx.h"
#include "cxlib.h"


enum
{
    EC_OK      = 0,
    EC_HELP    = 1,
    EC_USR_ERR = 2,
    EC_ERR     = 3,
};

static int   option_help = 0;

static char *srvref;
static int   conn;


int main(int argc, char *argv[])
{
  int   c;
  int   err = 0;
  int   saved_errno;
    
    set_signal(SIGPIPE, SIG_IGN);

    while ((c = getopt(argc, argv, "h")) != EOF)
        switch (c)
        {
            case 'h':
                option_help = 1;
                break;

            case ':':
            case '?':
                err++;
        }

    if (option_help)
    {
        printf("Usage: %s [OPTIONS] SERVER {COMMANDS}\n"
               "    -h  show this help\n",
               argv[0]);
        exit(EC_HELP);
    }

    if (err)
    {
        fprintf(stderr, "Try '%s -h' for help.\n", argv[0]);
        exit(EC_USR_ERR);
    }

    if (optind >= argc)
    {
        fprintf(stderr, "%s: missing SERVER reference.\n", argv[0]);
        exit(EC_USR_ERR);
    }

    srvref = argv[optind++];

    //////////////////////////////////////////////////////////////////

    conn = cx_open(0, NULL, srvref, 0, argv[0], NULL, NULL, NULL);
    if (conn < 0)
    {
        saved_errno = errno;
        fprintf(stderr, "%s %s: cx_open(\"%s\"): %s\n",
                strcurtime(), argv[0], srvref, cx_strerror(saved_errno));
        exit(EC_ERR);
    }

    cx_close(conn);
    
    return EC_OK;
}
