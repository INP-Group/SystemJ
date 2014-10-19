#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "cxlib.h"

int       conn;
enum      {NUM = 1000};
int32     values[NUM];
tag_t     tags  [NUM];
rflags_t  rflags[NUM];
int       r;

int       option_quiet;


static void Check(const char *where)
{
    if (r >= 0) return;
    cx_perror2(where, "cx-setchan");
    exit(1);
}

static int DecodeCommand(const char *command, chanaddr_t *chanp, int32 *vp)
{
  char *err;
  char *vstart;
  
    errno = 0;
    *chanp = strtol(command, &err, 0);
    if      (errno == ERANGE)
    {
        fprintf(stderr, "\"%s\": channel number is out of \"long\" range\n",
                command);
        exit(1);
    }
    else if (errno != 0)
    {
        fprintf(stderr, "\"%s\": !!!error!!! converting channel number to \"long\": %s\n",
                command, strerror(errno));
        exit(1);
    }

    if (err == command)
    {
        fprintf(stderr, "\"%s\": syntax error\n", command);
        exit(1);
    }

    /* Find the '=' sign (if any) */
    for (; ; err++)
    {
        if (*err == '\0') return 0;
        if (*err == '=')  break;
    }

    /* Skip everything up to a digit */
    for (; ; err++)
    {
        if (*err == '\0')
        {
            fprintf(stderr, "\"%s\": syntax error after '='\n", command);
            return 0;
        }
        if (isdigit(*err)  ||  *err == '-') break;
    }

    vstart = err;
    errno = 0;
    *vp = strtol(vstart, &err, 0);
    /*!!! Should do more checks*/
    
    return 1;
}

static void ShowHelp(const char *argv0)
{
    printf("Usage: %s [-q] <server> COMMAND {COMMAND}\n"
           "    where the COMMAND is:\n"
           "        chan?        display channel value\n"
           "        chan=value   set channel value\n"
           "    -q switches to quiet mode\n",
           argv0);
    exit(1);
}

int main(int argc, char *argv[])
{
  int         c;
  int         firstcmd;
  int         numcmds;
  int         cmd;
  int         action;
  chanaddr_t  chan;
  int32       v;
    
    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);

    while ((c = getopt(argc, argv, "qh")) != EOF)
        switch (c)
        {
            case 'q':
                option_quiet = 1;
                break;

            case 'h':
            case '?':
                ShowHelp(argv[0]);
        }

    if (optind >= argc)
        ShowHelp(argv[0]);

    r = conn = cx_connect(argv[optind++], argv[0]);
    Check("cx_connect");

    r = cx_begin(conn);
    Check("cx_begin");

    firstcmd = optind;
    
    numcmds = argc - firstcmd;
    if (numcmds <= 0) return 0;
    if (numcmds > NUM)
    {
        fprintf(stderr, "%s: WARNING: too many commands, will execute only first %d\n",
                argv[0], NUM);
        numcmds = NUM;
    }

    /* Prepare the request... */
    for (cmd = 0;  cmd < numcmds; cmd++)
    {
        action = DecodeCommand(argv[firstcmd + cmd], &chan, &v);
        if (action == 0)
        {
            cx_getvalue(conn, chan,    &(values[cmd]), &(tags[cmd]), &(rflags[cmd]));
            Check("cx_getvalue");
        }
        else
        {
            cx_setvalue(conn, chan, v, &(values[cmd]), &(tags[cmd]), &(rflags[cmd]));
            Check("cx_setvalue");
        }
    }

    /* Send the request... */
    r = cx_run(conn);
    Check("cx_run");

    /* Print the results */
    for (cmd = 0;  cmd < numcmds  &&  !option_quiet; cmd++)
    {
        action = DecodeCommand(argv[firstcmd + cmd], &chan, &v);
        printf("%d", chan);
        if (action != 0)
        {
            printf(":=%d", v);
        }
        printf("=%d\n", values[cmd]); /*!!!ERRSTATS Should also display "tag" and "status" info here */
    }

    return 0;
}
