#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


#include "cx-run-agent.h"


static void say(char what)
{
    write(1, &what, 1);
}

static void line_feed(void)
{
    say('\n');
}

static void report_failure(char where)
{
  char *errs;
  char  nbuf[100];
    
    say(CXRA_RSP_FAILURE);
    say(where);
    
    errs = strerror(errno);
    sprintf(nbuf, "%d", errno);
    
    write(1, nbuf, strlen(nbuf));
    say(':');
    write(1, errs, strlen(errs));
    line_feed();
}

static void Prepare(void)
{

}

static void RunProgram(int argc, char *argv[])
{
  int  r;
  int  first_arg = 1;
  int  use_path  = 0;

    /* Should we use execvp() instead of execv()? */
    if (argc >= 2  &&  strcmp(argv[first_arg], CXRA_USE_PATH_STR) == 0)
    {
        first_arg++;
        use_path = 1;
    }
  
    /* Are there any args? */
    if (argc <= first_arg)
    {
        say(CXRA_PARAM_ERROR);
        exit(1);
    }

    /* Fork */

    /* And exec requested prog */
    r = (use_path? execvp : execv)(argv[first_arg], argv + first_arg);
    report_failure(CXRA_PLACE_EXEC);
}

static void Interact(void)
{
}

int main(int argc, char *argv[])
{
    write(1, CXRA_AGENT_STARTED_STR, strlen(CXRA_AGENT_STARTED_STR));
    write(1, "\n", 1);

    Prepare();
    RunProgram(argc, argv);
    Interact();
    
    return 0;
}
