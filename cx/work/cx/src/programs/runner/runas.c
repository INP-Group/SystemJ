#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <sys/types.h>


/*
  Note: maybe it would be a good idea to do prctl(PR_SET_KEEPCAPS, 1)
  in PerformCaps()?
 
 */


enum
{
    EC_HELP   = 1,
    EC_SYNTAX = 2,
    EC_INVAL  = 3,
    EC_ERROR  = 4
};

#define DEF_SPEC "."

static int    global_argc;
static char **global_argv;

static int    prog_start;


/* Parameters section */

uid_t s_uid          = -1;
gid_t s_gid          = -1;
gid_t s_groups[100]; // 100 seems to be an "always reasonable" limit
gid_t s_groups_count = 0;


static void Fatal(int code, const char *format, ...)
    __attribute__ ((format (printf, 2, 3)));

static void Fatal(int code, const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
    fprintf (stderr, "%s: ", global_argv[0]);
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
    va_end(ap);

    exit(code);
}

static void Warning(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));

static void Warning(const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
    fprintf (stderr, "%s: WARNING: ", global_argv[0]);
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
    va_end(ap);
}


typedef void (*parser_f)(const char *info);
typedef void (*perform_f)(void);

typedef struct
{
    char        c;
    int         has_arg;
    int         specified;
    parser_f    parser;
    perform_f   perform;
    const char *description;
} action_t;


static void ParseNice(const char *info)
{

}

static void PerformNice(void)
{

}


static void ParsePrio(const char *info)
{

}

static void PerformPrio(void)
{

}


static void ParseCaps(const char *info)
{
    if (strcmp(info, "?") == 0)
    {
        /*!!! Print help for capabilities */
        
        exit(EC_HELP);
    }
}

static void PerformCaps(void)
{

}


static void ParseGID(const char *info)
{
  struct group  *g;
  struct passwd *p;
    
    if (strcmp(info, DEF_SPEC) == 0)
    {
        if (s_uid == -1)
            Fatal(EC_SYNTAX, "\"-g %s\" requires a prior \"-u UID\" specification", DEF_SPEC);
        
        if ((p = getpwuid(s_uid)) == NULL)
            Fatal(EC_ERROR, "-g %s: failed to find out group of uid=%d", DEF_SPEC, s_uid);

        s_gid = p->pw_gid;
    }
    else
    {
        if (isdigit(info[0]))
        {
            s_gid = atoi(info);
            if (getgrgid(s_gid) == NULL)
                Warning("no group with gid=%d", s_gid);
        }
        else
        {
            if ((g = getgrnam(info)) == NULL)
                Fatal(EC_ERROR, "unknown group \"%s\"", info);
            
            s_gid = g->gr_gid;
        }
    }
}

static void PerformGID(void)
{
    if (setgid(s_gid) < 0)
        Fatal(EC_ERROR, "setgid() failed, %s", strerror(errno));
}


static void ParseGroups(const char *info)
{
  const char    *cur;
  const char    *comma;
  size_t         len;
  char           buf[100]; // Is 99 chars enough for any {user,group}name?
  char         **m;
  struct group  *g;
  struct passwd *p;
  gid_t          gid;
    
    if (strcmp(info, DEF_SPEC) == 0)
    {
        if (s_uid == -1)
            Fatal(EC_SYNTAX, "\"-G %s\" requires a prior \"-u UID\" specification", DEF_SPEC);

        /* Get this uid's name */
        if ((p = getpwuid(s_uid)) == NULL)
            Fatal(EC_ERROR, "-G %s: failed to find out username of uid=%d", DEF_SPEC, s_uid);
        strncpy(buf, p->pw_name, sizeof(buf) - 1); buf[sizeof(buf) - 1] = '\0';
        
        /* Okay, let's find all the groups this uid belongs to... */
        while ((g = getgrent()) != NULL  &&  s_groups_count < sizeof(s_groups)/sizeof(s_groups[0]))
        {
            for (m = g->gr_mem;  *m != NULL;  m++)
            {
                if (strcmp(*m, buf) == 0)
                {
                    s_groups[s_groups_count++] = g->gr_gid;
                    break;
                }
            }
        }
        endgrent();
    }
    else
    {
        for (cur = info;
             s_groups_count < sizeof(s_groups)/sizeof(s_groups[0]);
             cur = comma)
        {
            /* Find the next comma or '\0' */
            if ((comma = strchr(cur, ',')) == NULL)
                comma = cur + strlen(cur);

            /* Is the empty string remaining? */
            if (comma == cur) break;

            len = comma - cur;
            if (len > sizeof(buf) - 1) len = sizeof(buf) - 1;
            
            memcpy(buf, cur, len);
            buf[len] = '\0';

            /* Find the referred group */
            if (isdigit(buf[0]))
            {
                gid = atoi(buf);
                if (getgrgid(gid) == NULL)
                    Warning("no group with gid=%d", gid);
            }
            else
            {
                if ((g = getgrnam(info)) == NULL)
                    Fatal(EC_ERROR, "unknown group \"%s\"", buf);
                
                gid = g->gr_gid;
            }

            /* Store gid */
            s_groups[s_groups_count++] = gid;
            
            /* Are we at the end of spec? */
            if (*comma == '\0') break;
            
            /* Skip the comma */
            comma++;
        }
    }
}

static void PerformGroups(void)
{
    if (setgroups(s_groups_count, s_groups) < 0)
        Fatal(EC_ERROR, "setgroups() failed, %s", strerror(errno));
}


static void ParseRoot(const char *info)
{

}

static void PerformRoot(void)
{

}


static void ParseDir(const char *info)
{

}

static void PerformDir(void)
{

}


static void ParseUID(const char *info)
{
  struct passwd *p;
    
    if (isdigit(info[0]))
    {
        s_uid = atoi(info);
        if (getpwuid(s_uid) == NULL)
            Warning("no user with uid=%d", s_uid);
    }
    else
    {
        if ((p = getpwnam(info)) == NULL)
            Fatal(EC_ERROR, "unknown user \"%s\"", info);

        s_uid = p->pw_uid;
    }
}

static void PerformUID(void)
{
    if (setuid(s_uid) < 0)
        Fatal(EC_ERROR, "setuid() failed, %s", strerror(errno));
}


static void ParseHelp(const char *info);


action_t actions[] =
{
    {'n',  1, 0, ParseNice,   PerformNice,   "NICE    set nice value"},
    {'p',  1, 0, ParsePrio,   PerformPrio,   "PRIO    set priority"},
    {'c',  1, 0, ParseCaps,   PerformCaps,   "CAPS    set capabilities (\"-c \\?\" for help)"},
    {'g',  1, 0, ParseGID,    PerformGID,    "GID     set primary group"},
    {'G',  1, 0, ParseGroups, PerformGroups, "GROUPS  set supplementary groups"},
    {'r',  1, 0, ParseRoot,   PerformRoot,   "ROOT    cd() and chroot() to specified dir"},
    {'d',  1, 0, ParseDir,    PerformDir,    "DIR     cd to specified dir"},
    {'u',  1, 0, ParseUID,    PerformUID,    "UID     set uid"},
    {'h',  0, 0, ParseHelp,   NULL,          "        show this message"},
    {'\0', 0, 0, NULL, NULL, NULL}
};


static void ParseHelp(const char *info)
{
  int x;
    
    printf("Usage: %s [ACTION]... COMMAND [ARG]...\n", global_argv[0]);
    printf("Runs a command with specified rights\n");
    printf("Available actions (actions are performed in the specified order):\n");
    printf("\n");

    for (x = 0;  actions[x].c != '\0';  x++)
        printf("  -%c %s\n", actions[x].c, actions[x].description);

    printf("\n");
    printf("'%s' for GID, GROUPS, ROOT and DIR takes default values from UID.\n", DEF_SPEC);
    
    exit(EC_HELP);
}


static void CheckSUGID(void)
{
    if (getuid() != geteuid()  ||  getgid() != getegid())
        Fatal(EC_INVAL, "this program is NOT intended to be run setuid/setgid");
}


static void ParseCommandLine(void)
{
  int         arg;
  int         c;
  const char *info;
  int         x;
  
    for (arg = 1;
         arg < global_argc  &&  global_argv[arg][0] == '-'  && (c = global_argv[arg][1]) != '\0';
         arg++)
    {
        for (x = 0;  actions[x].c != '\0';  x++)
        {
            if (actions[x].c == c)
            {
                /* Handle duplicates */
                if (actions[x].specified)
                    Fatal(EC_SYNTAX, "duplicate '-%c' option", c);
                actions[x].specified = 1;
                
                info = NULL;
                if (actions[x].has_arg)
                {
                    if (global_argv[arg][2] != '\0')
                    {
                        info = &(global_argv[arg][2]);
                    }
                    else
                    {
                        arg++;
                        if (arg >= global_argc)
                        {
                            fprintf(stderr, "%s: the '-%c' option requires an argument\n",
                                    global_argv[0], c);
                            goto ADVICE_HELP;
                        }
                        
                        info = global_argv[arg];
                    }
                }
                else
                {
                    if (global_argv[arg][2] != '\0')
                    {
                        fprintf(stderr, "%s: the '-%c' option doesn't accept an argument\n",
                                global_argv[0], c);
                        goto ADVICE_HELP;
                    }
                }
                
                actions[x].parser(info);

                break;
            }
        }

        /* Have we found that option? */
        if (actions[x].c == '\0')
        {
            fprintf(stderr, "%s: unknown option '-%c'.\n", global_argv[0], c);
            goto ADVICE_HELP;
        }
    }

    prog_start = arg;
    
    return;

 ADVICE_HELP:
    fprintf(stderr, "Try \"%s -h\" for help.\n", global_argv[0]);
    exit(EC_SYNTAX);
}


static void PerformActions(void)
{
  int  x;
    
    for (x = 0;  actions[x].c != '\0';  x++)
        if (actions[x].specified)
            actions[x].perform();
}


static void RunProgram(void)
{
    if (prog_start >= global_argc)
    {
        fprintf(stderr, "%s: no program to run specified\n", global_argv[0]);
        fprintf(stderr, "Try \"%s -h\" for help.\n", global_argv[0]);
        exit(EC_SYNTAX);
    }

    execv(global_argv[prog_start], global_argv + prog_start);

    Fatal(EC_ERROR, "execv(\"%s\"): %s",
          global_argv[prog_start], strerror(errno));
}


int main(int argc, char *argv[])
{
    global_argc = argc;
    global_argv = argv;
    
    CheckSUGID();
    ParseCommandLine();
    PerformActions();
    RunProgram();
    
    return EC_ERROR;
}
