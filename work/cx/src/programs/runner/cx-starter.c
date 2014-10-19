#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include <dlfcn.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#include <X11/Xmu/WinUtil.h>

#include "misc_macros.h"
#include "misclib.h"
#include "memcasecmp.h"
#include "findfilein.h"

#include "cxlib.h"
#include "cda.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Xh_fallbacks.h"
#include "Cdr.h"
#include "Chl.h"

#include "paramstr_parser.h"
#include "timeval_utils.h"

#include "pixmaps/cx-starter.xpm"


static void reportinfo(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));

static void reportinfo(const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
    fprintf (stdout, "%s cx-starter: ",
             strcurtime());
    vfprintf(stdout, format, ap);
    fprintf (stdout, "\n");
    va_end(ap);
}


/*********************************************************************
*  Variables section                                                 *
*********************************************************************/

/*  */
#if 0
static const char *app_name  = "cxstarter";
static const char *app_class = "CxStarter";
#endif

static XhWindow    onewin;

static Widget      prog_menu, prog_menu_start, prog_menu_stop, prog_menu_restart;
static Widget      serv_menu, serv_menu_start, serv_menu_stop, serv_menu_restart;

static int         prog_menu_client;
static int         serv_menu_client;

enum
{
    CMD_START = 1000,
    CMD_STOP,
    CMD_RESTART
};

enum {MAXSYSTEMS = 100};

/* Commandline "database" */

static int   option_passive = 0;

static int   config_was_read = 0;

static char *reqd_systems[MAXSYSTEMS];
static int   reqd_count  = 0;


/* Config "database" */

enum {SUBSYS_CX = 1, SUBSYS_FRGN = 2, SUBSYS_SEPR = 3};

typedef struct
{
    char  name [128];
    char  label[128];
    int   kind;          // SUBSYS_NNN
    int   ignorevals;
    char *app_name;
    char *title;
    char *comment;
    char *chaninfo;
    char *start_cmd;
    char *params;
    char *stop_cmd;
    char *statuschkcmd;
    int   statuschktime;
} cfgline_t;

static cfgline_t config_info[MAXSYSTEMS];
static int       config_count = 0;

enum {MAXCMDLEN = 1024};
static psp_paramdescr_t text2cfgline[] =
{
    PSP_P_STRING ("label",         cfgline_t, label,         NULL),
    PSP_P_FLAG   ("cx",            cfgline_t, kind,          SUBSYS_CX,   1),
    PSP_P_FLAG   ("foreign",       cfgline_t, kind,          SUBSYS_FRGN, 0),
    PSP_P_FLAG   ("ignorevals",    cfgline_t, ignorevals,    1,           0),
    PSP_P_MSTRING("app_name",      cfgline_t, app_name,      NULL, 200),
    PSP_P_MSTRING("title",         cfgline_t, title,         NULL, 200),
    PSP_P_MSTRING("comment",       cfgline_t, comment,       NULL, 200),
    PSP_P_MSTRING("chaninfo",      cfgline_t, chaninfo,      NULL, 1000),
    PSP_P_MSTRING("start",         cfgline_t, start_cmd,     NULL, MAXCMDLEN),
    PSP_P_MSTRING("stop",          cfgline_t, stop_cmd,      NULL, MAXCMDLEN),
    PSP_P_MSTRING("params",        cfgline_t, params,        NULL, MAXCMDLEN),
    PSP_P_MSTRING("statuschkcmd",  cfgline_t, statuschkcmd,  NULL, MAXCMDLEN),
    PSP_P_INT    ("statuschktime", cfgline_t, statuschktime, 0, 0, 1000000),
    PSP_P_END()
};


/* Subsystems database */

enum {NUMLOCALREGS = 1000};

typedef struct
{
    const char     *name;
    cfgline_t      *cp;
//    int             kind;

////////    subsysdescr_t *cx_info;
    
    cda_serverid_t  mainsid;

    int             srvcount;
    ledinfo_t      *leds;

    XhWidget        btn;
    XhPixel         btn_defbg;
    XhWidget        a_led;
    XhWidget        d_led;
    rflags_t        oldrflags;

    XhWidget        s_led;
    
    groupelem_t    *grouplist;

    double          localregs      [NUMLOCALREGS];
    char            localregsinited[NUMLOCALREGS];
    
    struct timeval  last_start_time;
} subsys_t;


#define CONFIG_NAME "cx-starter.conf"

static int      subsyscount = 0;
static subsys_t subsystems[MAXSYSTEMS];


/* Servers database */

enum {MAXSERVERS = 50};

typedef struct
{
    char *name;
    char *start_cmd;
    char *stop_cmd;
    char *user;
    char *params;
} srvparams_t;

static srvparams_t srv_info[MAXSERVERS];
static int         srv_count = 0;

static psp_paramdescr_t text2srvparams[] =
{
    PSP_P_MSTRING("start",  srvparams_t, start_cmd,     NULL, MAXCMDLEN),
    PSP_P_MSTRING("stop",   srvparams_t, stop_cmd,      NULL, MAXCMDLEN),
    PSP_P_MSTRING("user",   srvparams_t, user,          NULL, 32),
    PSP_P_MSTRING("params", srvparams_t, params,        NULL, MAXCMDLEN),
    PSP_P_END()
};


/*********************************************************************
*  Configuration section                                             *
*********************************************************************/

typedef enum {CFG_IS_FILE, CFG_IS_DIR} cfg_type_t;

static int  TryConfig(const char *path, void **r, cfg_type_t *type)
{
  DIR  *dp;
  FILE *fp;
  
    /* First check if it is a directory */
    dp = opendir(path);
    if (dp != NULL)
    {
        *((DIR**)r)  = dp;
        *type        = CFG_IS_DIR;
        return 0;
    }

    /* Okay, no.  Is it accessible as a file? */
    fp = fopen(path, "r");
    if (fp != NULL)
    {
        *((FILE**)r) = fp;
        *type        = CFG_IS_FILE;
        return 0;
    }

    return -1;
}

static void ReadConfigFile(const char *argv0, const char *path, FILE *fp)
{
#define CFG_SEPARATORS " \t"
#define CFG_COMMENT    "#"
    
  char        linebuf[2000];
  const char *linep;
  int         lineno = 0;
  const char *s_p;
  int         namelen;
  cfgline_t  *cp;

  int         srv;

  int         usecs;
    
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
    {
        /* Cut newline and check for '\' at end-of-line */
        do
        {
            lineno++;
            
            if (linebuf[0] == '\0') break;
            if (linebuf[strlen(linebuf) - 1] == '\n') linebuf[strlen(linebuf) - 1] = '\0';
            if (linebuf[strlen(linebuf) - 1] != '\\') break;

            linebuf[strlen(linebuf) - 1] = ' ';
            
            if (strlen(linebuf) >= sizeof(linebuf) - 2)
            {
                fprintf(stderr, "%s: %s:%d: FATAL ERROR: input buffer already full\n",
                        argv0, path, lineno);
                exit(1);
            }

            if (fgets(linebuf+strlen(linebuf),
                      sizeof(linebuf)-strlen(linebuf),
                      fp) == NULL)
            {
                fprintf(stderr, "%s: %s:%d: END-OF-FILE encountered after '\\'-line-continuation\n",
                        argv0, path, lineno);
                break;
            }
        } while (1);

        /* Skip leading whitespace */
        linep = linebuf;
        linep += strspn(linep, CFG_SEPARATORS);

        /* An empty/full-comment line? */
        if (*linep == '\0'  ||  *linep == '#') continue;

        if (*linep == '.')
        {
            linep++;

            /* Extract the keyword... */
            s_p = linep;
            while (*linep == '_'  ||  (linep != s_p  &&  *linep == '-')  ||
                   isalnum(*linep))
                linep++;
            if (s_p == linep)
            {
                fprintf(stderr, "%s: %s:%d@%d: missing .-keyword\n",
                        argv0, path, lineno, (int)(s_p - linebuf + 1));
                exit(1);
            }
            
            /* What a .-directive can it be? */
            if      (cx_strmemcasecmp("srvparams", s_p, linep - s_p) == 0)
            {
                linep += strspn(linep, CFG_SEPARATORS);

                s_p = linep;
                linep += strcspn(linep, CFG_SEPARATORS);
                if (s_p == linep)
                {
                    fprintf(stderr, "%s: %s:%d@%d: missing server spec for .srvparams\n",
                            argv0, path, lineno, (int)(s_p - linebuf + 1));
                    exit(1);
                }

                for (srv = 0;  srv < srv_count;  srv++)
                    if (cx_strmemcasecmp(srv_info[srv].name, s_p, linep - s_p) == 0)
                        break;
                if (srv >= srv_count)
                {
                    if (srv_count >= MAXSERVERS)
                    {
                        fprintf(stderr, "%s: %s:%d@%d: too many .srvparams\n",
                                argv0, path, lineno, (int)(s_p - linebuf + 1));
                        goto NEXT_LINE;
                    }

                    srv = srv_count;
                    srv_count++;

                    bzero(srv_info + srv, sizeof(srv_info[srv]));
                    srv_info[srv].name = malloc((linep - s_p) + 1);
                    memcpy(srv_info[srv].name, s_p, linep - s_p);
                    srv_info[srv].name[linep - s_p] = '\0';
                }

                if (psp_parse(linep, NULL,
                              srv_info + srv,
                              '=', CFG_SEPARATORS, CFG_COMMENT,
                              text2srvparams) != PSP_R_OK)
                {
                    fprintf(stderr, "%s: %s:%d: %s\n",
                            argv0, path, lineno, psp_error());
                    exit(1);
                }
            }
            else
            {
                fprintf(stderr, "%s: %s:%d@%d: unknown .-directive\n",
                        argv0, path, lineno, (int)(s_p - linebuf + 1));
                goto NEXT_LINE;
            }
        }
        else
        {
            if (config_count >= MAXSYSTEMS)
            {
                fprintf(stderr, "%s: sorry, too many systems upon reading \"%s\"; limit is %d\n",
                        argv0, path, MAXSYSTEMS);
                goto END_PARSE;
            }

            cp = config_info + config_count;
            
            /* Extract the keyword... */
            s_p = linep;
            while (*linep == '_'  ||  (/*linep != s_p  &&*/  *linep == '-')  ||
                   isalnum(*linep))
                linep++;

            if (s_p == linep)
            {
                fprintf(stderr, "%s: %s:%d@%d: missing system name\n",
                        argv0, path, lineno, (int)(s_p - linebuf + 1));
                exit(1);
            }

            namelen = linep - s_p;
            if (namelen > sizeof(cp->name) - 1) namelen = sizeof(cp->name) - 1;
            memcpy(cp->name, s_p, namelen);
            cp->name[namelen] = '\0';

            /* Okay, what's than? */
            linep += strspn(linep, CFG_SEPARATORS);
            if (*linep == '\0'  ||  *linep == '#')
                psp_parse(NULL, NULL,
                          cp,
                          '=', "", CFG_COMMENT,
                          text2cfgline);
            else
            {
                if (psp_parse(linep, NULL,
                              cp,
                              '=', CFG_SEPARATORS, CFG_COMMENT,
                              text2cfgline) != PSP_R_OK)
                {
                    fprintf(stderr, "%s: %s:%d: %s\n",
                            argv0, path, lineno, psp_error());
                    exit(1);
                }

                /* Check validity... */
                if (cp->kind != SUBSYS_CX)
                {
                    if (cp->start_cmd == NULL)
                        fprintf(stderr, "%s: %s:%d: warning: no 'start' command specified for '%s'\n",
                                argv0, path, lineno, cp->name);
                }
            }

            config_count++;
        }
 NEXT_LINE:;
    }
 END_PARSE:;

    fclose(fp);
}

static void ReadConfigDir (const char *argv0, const char *path, DIR  *dp)
{
  struct dirent     *entry;
  size_t             namelen;
  
  cfgline_t         *cp;
  int                initial_count = config_count;
  
  static const char *chl_db_suffix = "_db.so";
  size_t             chl_db_suflen = strlen(chl_db_suffix);
  
    for (cp = config_info + config_count;
         config_count < MAXSYSTEMS  &&  (entry = readdir(dp)) != NULL;
        )
    {
        namelen = strlen(entry->d_name);
        
        if (namelen > chl_db_suflen  &&
            memcmp(&(entry->d_name[namelen - chl_db_suflen]), chl_db_suffix, chl_db_suflen) == 0)
        {
            namelen -= chl_db_suflen;
            if (namelen > sizeof(cp->name) - 1) namelen = sizeof(cp->name) - 1;
            memcpy(cp->name, entry->d_name, namelen);
            cp->name[namelen] = '\0';
            cp->kind = SUBSYS_CX;

            cp++;
            config_count++;
        }
    }

    if (config_count >= MAXSYSTEMS)
        fprintf(stderr, "%s: %s: limit of %d systems is reached\n",
                argv0, path, MAXSYSTEMS);

    /* Sort the result (yes, we use bubble-sort...) */
    if (initial_count < config_count)
    {
      int        i, j;
      cfgline_t  tmp;
      
        for (i = initial_count;  i < config_count;  i++)
            for (j = initial_count;  j < config_count - 1;  j++)
                if (strcmp(config_info[j].name, config_info[j+1].name) > 0)
                {
                    tmp              = config_info[j];
                    config_info[j]   = config_info[j+1];
                    config_info[j+1] = tmp;
                }
    }
    
    closedir(dp);
}

typedef struct
{
    char        fname[PATH_MAX];
    void       *cs;
    cfg_type_t  cfg_type;
} config_info_t;

static void *config_check_proc(const char *name,
                               const char *path,
                               void       *privptr)
{
  config_info_t *pinfo = privptr;
  char           plen = strlen(path);
  
    check_snprintf(pinfo->fname, sizeof(pinfo->fname), "%s%s%s",
                   path,
                   plen == 0  ||  path[plen-1] == '/'? "" : "/",
                   name);
    return
        TryConfig(pinfo->fname, &(pinfo->cs), &(pinfo->cfg_type)) == 0? pinfo->fname
                                                                      : NULL;
}

static void ReadConfig(const char *argv0, const char *filespec)
{
  const char    *cfgpath;
  config_info_t  info;
  
    if (filespec != NULL)
    {
        if (TryConfig(cfgpath = filespec, &info.cs, &info.cfg_type) != 0)
        {
            fprintf(stderr, "%s: unable to open requested config \"%s\": %s\n",
                    argv0, cfgpath, strerror(errno));
            exit(1);
        }
    }
    else
        cfgpath = findfilein(CONFIG_NAME,
                             argv0,
                             config_check_proc, &info,
                             "./"
                                 FINDFILEIN_SEP "!/../configs"
                                 FINDFILEIN_SEP "$PULT_ROOT/configs"
                                 FINDFILEIN_SEP "~/pult/configs");

    if (cfgpath == NULL)
    {
        fprintf(stderr, "%s: can't find a config file!\n",
                argv0);
        exit(1);
    }

    switch (info.cfg_type)
    {
        case CFG_IS_FILE: ReadConfigFile(argv0, cfgpath, (FILE *)info.cs); break;
        case CFG_IS_DIR:  ReadConfigDir (argv0, cfgpath, (DIR *) info.cs); break;
    }

    config_was_read = 1;
}

static void ParseCommandLine(int argc, char *argv[])
{
  int  x;
    
    for (x = 1;  x < argc;  x++)
    {
        if      (strchr(argv[x], '/') != NULL)
        {
            ReadConfig(argv[0], argv[x]);
        }
        else if (strcmp(argv[x], "-passive") == 0)
            option_passive = 1;
        else
        {
            if (reqd_count >= MAXSYSTEMS)
            {
                fprintf(stderr, "%s: sorry, too many systems are requested; limit is %d\n",
                        argv[0], MAXSYSTEMS);
                exit(1);
            }
            
            reqd_systems[reqd_count] = argv[x];
            reqd_count++;
        }
    }

    if (!config_was_read)
        ReadConfig(argv[0], NULL);
}


/*********************************************************************
*  ?????? section                                                    *
*********************************************************************/

static char thishostname[100];

static void ObtainHostIdentity(void)
{
  struct utsname  ubuf;
    
    if (uname(&ubuf) != 0)
    {
        fprintf(stderr, "%s: uname(): %s\n", __FUNCTION__, strerror(errno));
        return;
    }

    strzcpy(thishostname, ubuf.nodename, sizeof(thishostname));
}

static int IsThisHost(const char *hostname)
{
  int         is_fqdn;
  const char *dp;

    ////fprintf(stderr, "thishost='%s', host='%s'\n", thishostname, hostname);
  
    if (thishostname[0] == '\0'  ||
        hostname == NULL  ||  hostname[0] == '\0') return 0;

    is_fqdn = (hostname[strlen(hostname) - 1] == '.');

    if (strcasecmp(hostname, thishostname) == 0) return 1;

    dp = strchr(thishostname, '.');
    if (dp != NULL  &&
        cx_strmemcasecmp(hostname, thishostname, dp - thishostname) == 0) return 1;
    if (strcasecmp(hostname, "localhost") == 0) return 1;
        
    return 0;
}


/*********************************************************************
*  ?????? section                                                    *
*********************************************************************/

static void EventProc(cda_serverid_t  sid __attribute__((unused)),
                      int             reason,
                      void           *privptr)
{
  int                 sys = ptr2lint(privptr);
  subsys_t           *sr  = subsystems + sys;
  rflags_t            rflags;
  cda_localreginfo_t  localreginfo;
  int                 idx;

    if (reason < CDA_R_MIN_SID_N) return;
  
    localreginfo.count       = NUMLOCALREGS;
    localreginfo.regs        = sr->localregs;
    localreginfo.regsinited  = sr->localregsinited;
    
    leds_update(sr->leds, sr->srvcount, sr->mainsid);

    CdrProcessGrouplist(reason, CDR_OPT_READONLY,
                        &rflags, &localreginfo, sr->grouplist);
    rflags &=~ CXCF_FLAG_4WRONLY_MASK;
    if (sr->cp->ignorevals)
        rflags &= (CXRF_SERVER_MASK | CXCF_FLAG_DEFUNCT | CXCF_FLAG_NOTFOUND);

    if ((rflags ^ sr->oldrflags) & CXCF_FLAG_ALARM_MASK)
    {
        idx                                      = -1;
        if (rflags & CXCF_FLAG_ALARM_RELAX)  idx = XH_COLOR_JUST_GREEN;
        if (rflags & CXCF_FLAG_ALARM_ALARM)  idx = XH_COLOR_JUST_RED;
        XtVaSetValues(sr->btn,
                      XmNbackground, idx < 0? sr->btn_defbg : XhGetColor(idx),
                      NULL);
    }

    if ((rflags ^ sr->oldrflags) & CXCF_FLAG_COLOR_MASK)
    {
        idx                                      = XH_COLOR_JUST_GREEN;
        if (rflags & CXCF_FLAG_COLOR_YELLOW) idx = XH_COLOR_BG_IMP_YELLOW;
        if (rflags & CXCF_FLAG_COLOR_RED)    idx = XH_COLOR_BG_IMP_RED;
        if (rflags & CXCF_FLAG_COLOR_WEIRD)  idx = XH_COLOR_BG_WEIRD;
        XtVaSetValues(sr->a_led,
                      XmNset,         (rflags & CXCF_FLAG_COLOR_MASK) != 0,
                      XmNselectColor, XhGetColor(idx),
                      NULL);
    }

    if ((rflags ^ sr->oldrflags) & CXCF_FLAG_SYSERR_MASK)
    {
        idx                                      = XH_COLOR_JUST_GREEN;
        if (rflags & CXCF_FLAG_DEFUNCT)      idx = XH_COLOR_BG_DEFUNCT;
        if (rflags & CXCF_FLAG_SFERR_MASK)   idx = XH_COLOR_BG_SFERR;
        if (rflags & CXCF_FLAG_HWERR_MASK)   idx = XH_COLOR_BG_HWERR;
        if (rflags & CXCF_FLAG_NOTFOUND)     idx = XH_COLOR_BG_NOTFOUND;
        XtVaSetValues(sr->d_led,
                      XmNset,         (rflags & CXCF_FLAG_SYSERR_MASK) != 0,
                      XmNselectColor, XhGetColor(idx),
                      NULL);
    }

    sr->oldrflags = rflags;
}

static void KeepaliveProc(XtPointer     closure __attribute__((unused)),
                          XtIntervalId *id      __attribute__((unused)))
{
  int                 sys;
  subsys_t           *sr;

    XtAppAddTimeOut(xh_context, 2000, KeepaliveProc, NULL);

    for (sys = 0, sr = subsystems;  sys < subsyscount;  sys++, sr++)
        if (sr->cp->kind == SUBSYS_CX)
            leds_update(sr->leds, sr->srvcount, sr->mainsid);
}


/*********************************************************************
*  ?????? section                                                    *
*********************************************************************/

static void RunCommand(const char *cmd)
{
  const char *prog_args[10];
  int         prog_argc;

  const char *prog_p;
  const char *colon;
  size_t      len;
  
  char        host_buf[100];

  pid_t       pid;

    reportinfo("%s(\"%s\")", __FUNCTION__, cmd);

    if (option_passive) return;
    
    prog_argc = 0;

    prog_p = cmd;
    
    if (*prog_p == '@')
    {
        prog_p++;
        colon = strchr(prog_p, ':');
        if (colon == NULL)
        {
            fprintf(stderr, "%s(): missing ':' (required for \"@host:command\") in \"%s\"\n",
                    __FUNCTION__, cmd);
            return;
        }

        len = colon - prog_p;
        if (len > sizeof(host_buf) - 1) len = sizeof(host_buf) - 1;
        memcpy(host_buf, prog_p, len);
        host_buf[len] = '\0';

        prog_p = colon + 1;

        prog_args[prog_argc++] = "/usr/bin/ssh";
        //prog_args[prog_argc++] = "-t"; // No-no-no! We do NOT want the "-t" by default!
        //prog_args[prog_argc++] = "-qtt"; // Maybe a "-qtt", but as for now, we'll better live without it
        prog_args[prog_argc++] = host_buf;
    }
    else
    {
        prog_args[prog_argc++] = "/bin/sh";
        prog_args[prog_argc++] = "-c";
    }

    ////fprintf(stderr, "\t'%s'\n", prog_p);
    prog_args[prog_argc++] = prog_p;
    prog_args[prog_argc]   = NULL;

    switch (pid = fork())
    {
        case 0:
            set_signal(SIGHUP, SIG_IGN);
            execv(prog_args[0], prog_args);
            exit(1);

        case -1:
            fprintf(stderr, "%s(): unable to fork(): %s\n",
                    __FUNCTION__, strerror(errno));
            break;

        default:
            reportinfo("PID=%ld", (long)pid);
    }
}


/*********************************************************************
*  ?????? section                                                    *
*********************************************************************/

static Atom  xa_WM_STATE = None;

static Window  SearchOneLevel(Display *dpy, Window win,
                              const char *res_name, const char *title)
{
  Window         ret;
    
  Window        *children;
  unsigned int   nchildren;
  Window         dummy;
  int            i;

  XClassHint     hints;
  char          *winname;

  int            r;
  Atom           type;
  int            format;
  unsigned long  nitems, after;
  unsigned char *data;
  

    if (!XQueryTree(dpy, win, &dummy, &dummy, &children, &nchildren))
        return 0;

    for (i = 0, ret = 0; i < nchildren  &&  ret == 0;  i++)
    {
        /* Does this window have a WM_STATE property? */
        XhENTER_FAILABLE();

        type = None;
        r = XGetWindowProperty(dpy, children[i], xa_WM_STATE,
                               0, 0, False,
                               AnyPropertyType, &type, &format, &nitems,
                               &after, &data);

        if (r != Success)
        {
        }
        else if (type)
        {
            /* Yes, it has WM_STATE! */

            XFree(data); // We no longer need obtained data

            if      (res_name != NULL  &&  res_name[0] != '\0')
            {
                /* Check window name... */
                if (XGetClassHint(dpy, children[i], &hints))
                {
                    if (hints.res_name != NULL  &&
                        strcmp(res_name, hints.res_name) == 0) ret = children[i];

                    if (hints.res_name  != NULL) XFree(hints.res_name);
                    if (hints.res_class != NULL) XFree(hints.res_class);
                }
            }
            else if (title != NULL  &&  title[0] != '\0')
            {
                if (XFetchName(dpy, children[i], &winname))
                {
                    if (strcmp(title, winname) == 0) ret = children[i];
                    XFree(winname);
                }
            }
        }
        else
        {
            /* No, we should recurse deeper */
            ret = SearchOneLevel(dpy, children[i], res_name, title);
        }
        
        XhLEAVE_FAILABLE();
    }
    
    if (children) XFree(children);
    
    return ret;
}

static Window  SearchForWindow(Widget w, const char *res_name, const char *title)
{
  Display      *dpy     = XtDisplay(w);
  Window        root    = RootWindowOfScreen(XtScreen(w));

    if (xa_WM_STATE == None)
    {
        xa_WM_STATE = XInternAtom(dpy, "WM_STATE", True);
        if (xa_WM_STATE == None) return 0;
    }

    return SearchOneLevel(dpy, root, res_name, title);
}

enum
{
    FLASH_COUNT  = 3,     // Number of flashes
    FLASH_PERIOD = 1000,  // Full period of flash (on+off)
};

static Widget        flash_shell = NULL;
static Widget        flash_arrow = NULL;
static GC            arrow_GC;
static XtIntervalId  flash_tid   = 0;
static int           flash_ctr;
static double        arrow_sin;
static double        arrow_cos;

static int arrow_coords[][2] =
{
#if 1
    {95, 50},
    {65, 20*1+35*0},
    {65, 35},
    {10, 35},
    {10, 65},
    {65, 65},
    {65, 80},
#else
    {10, 35},
    {90, 35},
    {90, 65},
    {10, 65},
#endif
};

static void ArrowExposeCB(Widget     w,
                          XtPointer  closure    __attribute__((unused)),
                          XtPointer  call_data  __attribute__((unused)))
{
  Dimension  my_w, my_h;
  int        size;
  XPoint     points[countof(arrow_coords)];
  int        n;
  double        x, y;
  
    XtVaGetValues(w, XmNwidth, &my_w, XmNheight, &my_h, NULL);
    size = my_w;
    if (size > my_h) size = my_h;

    for (n = 0;  n < countof(points);  n++)
    {
        x = (arrow_coords[n][0] - 50) * size / 100;
        y = (arrow_coords[n][1] - 50) * size / 100;
        points[n].x = my_w / 2 + (x * arrow_cos - y * arrow_sin);
        points[n].y = my_h / 2 + (x * arrow_sin + y * arrow_cos);
    }

    XFillPolygon(XtDisplay(w), XtWindow(w), arrow_GC, points, countof(points), Nonconvex, CoordModeOrigin);
}

static void FlashProc(XtPointer     closure __attribute__((unused)),
                      XtIntervalId *id      __attribute__((unused)))
{
    flash_ctr--;
    if ((flash_ctr & 1) != 0  &&  flash_ctr > 0)
    {
        XtManageChild  (flash_shell);
        XtManageChild  (flash_arrow);
        //XRaiseWindow(XtDisplay(flash_arrow), XtWindow(flash_arrow));
    }
    else
    {
        XtUnmanageChild(flash_shell);
        XtUnmanageChild(flash_arrow);
    }

    if (flash_ctr > 0)
        flash_tid = XtAppAddTimeOut(xh_context, FLASH_PERIOD / 2, FlashProc, NULL);
    else
        flash_tid = 0;
}

static GC AllocXhFgGC(Widget w, int idx)
{
  XGCValues  vals;

    vals.foreground = XhGetColor(idx);
    return XtGetGC(w, GCForeground, &vals);
}

static void PointToTargetWindow(Display *dpy, Window win)
{
  Window        my_root = RootWindowOfScreen(XtScreenOfObject(XhGetWindowShell(onewin)));
    
  Window        twin_root;
  int           twin_x, twin_y;
  unsigned int  twin_w, twin_h;
  unsigned int  twin_bw, twin_d;
  int           root_x, root_y;
  Window        child;

  int           my_x0, my_y0;
  Dimension     my_w,  my_h;
  int           delta_x, delta_y;
  double        angle;

    if (!XGetGeometry(dpy, win,
                      &twin_root,
                      &twin_x, &twin_y, &twin_w, &twin_h,
                      &twin_bw, &twin_d)) return;
    ////fprintf(stderr, "target: %d,%d %d*%d\n", twin_x, twin_y, twin_w, twin_h);

    if (!XTranslateCoordinates(dpy, win,
                               my_root, 0, 0,
                               &root_x, &root_y, &child)) return;
    ////fprintf(stderr, "\t%d,%d\n", root_x, root_y);

    if (flash_shell == NULL)
    {
        flash_shell = XtVaCreatePopupShell
            ("flashShell", overrideShellWidgetClass, XhGetWindowShell(onewin),
             XmNbackground,  XhGetColor(XH_COLOR_JUST_AMBER),
             XmNborderWidth, 0,
             NULL);
    }

    if (flash_arrow == NULL)
    {
        flash_arrow = XtVaCreateWidget("flashArrow",
                                       xmDrawingAreaWidgetClass,
                                       XhGetWindowWorkspace(onewin),
                                       XmNbackgroundPixmap, None,
                                       XmNleftAttachment,   XmATTACH_FORM,
                                       XmNrightAttachment,  XmATTACH_FORM,
                                       XmNtopAttachment,    XmATTACH_FORM,
                                       XmNbottomAttachment, XmATTACH_FORM,
                                       NULL);
        arrow_GC = AllocXhFgGC(flash_arrow, XH_COLOR_JUST_BLUE);
        XtAddCallback(flash_arrow, XmNexposeCallback, ArrowExposeCB, NULL);
    }

    XTranslateCoordinates(dpy, XtWindow(XhGetWindowShell(onewin)),
                          my_root, 0, 0,
                          &my_x0, &my_y0, &child);
    ////fprintf(stderr, "%d %d\n", my_x0, my_y0);
    XtVaGetValues(XhGetWindowWorkspace(onewin), XmNwidth, &my_w, XmNheight, &my_h, NULL);
    delta_x = (root_x + twin_w / 2) - (my_x0 + my_w / 2);
    delta_y = (root_y + twin_h / 2) - (my_y0 + my_h / 2);
    if (delta_x == 0  &&  delta_y == 0)
        angle = 0;
    else
        angle = atan2(delta_y, delta_x);//*0+M_PI/40;
    arrow_sin = sin(angle);
    arrow_cos = cos(angle);
    ////fprintf(stderr, "%d,%d %4.3f %5.3f %5.3f\n", delta_x, delta_y, angle, arrow_sin, arrow_cos);
    
    XtVaSetValues(flash_shell,
                  XmNx,      root_x,
                  XmNy,      root_y,
                  XmNwidth,  twin_w,
                  XmNheight, twin_h,
                  NULL);
    flash_ctr = FLASH_COUNT * 2;
    if (flash_tid != 0) XtRemoveTimeOut(flash_tid);
    FlashProc(NULL, NULL);
}

static int ChangeClientStatus(int sys, int on)
{
  subsys_t     *sr      = subsystems + sys;
  Window        win;
  
  Widget        w       = XhGetWindowShell(onewin);
  Display      *dpy     = XtDisplay(w);

  struct timeval  deadline;
  struct timeval  now;
  
  const char   *cmd;
  char          cmd_buf[PATH_MAX];
  const char   *home;
  const char   *cx_root;

  const char   *pfx1;
  const char   *pfx2;

    /* Guard against ... */
    if (on)
    {
        gettimeofday(&now, NULL);
        timeval_add_usecs(&deadline, &(sr->last_start_time), 5*1000000);
        if (!TV_IS_AFTER(now, deadline)) return 0;
    }

    win = SearchForWindow(w, sr->cp->app_name, sr->cp->title);

    if (win != 0)
    {
        if (on)
        {
            XhENTER_FAILABLE();
            /*!!! Shouldn't we use "XMapRaised(dpy, win);" here? */
            XMapWindow         (dpy, win);
            XRaiseWindow       (dpy, win);
            XSetInputFocus     (dpy, win, RevertToPointerRoot, CurrentTime);
            PointToTargetWindow(dpy, win);
            XhLEAVE_FAILABLE();

            XFlush(dpy);
            
            return 0;
        }
        else
        {
            if (sr->cp->stop_cmd != NULL)
            {
                RunCommand(sr->cp->stop_cmd);
            }
            else
            {
                XhENTER_FAILABLE();
                if (!option_passive)
                {
                    XKillClient(dpy, win);
                    reportinfo("XKillClient(\"%s\":0x%08lx)",
                               (
                                sr->cp->app_name != NULL  &&
                                sr->cp->app_name[0] != '\0'
                               ) ? sr->cp->app_name
                                 : sr->cp->title,
                               (unsigned long)win /*!!! from Xdefs.h & X.h; but gcc barks anyway... */);
                }
                XhLEAVE_FAILABLE();
                XFlush(dpy);
            }
            
            return 1;
        }
    }
    else
    {
        if (!on) return 0;
        
        cmd = sr->cp->start_cmd;
        
        if (cmd == NULL)
        {
            if (sr->cp->kind == SUBSYS_CX)
            {
                home    = getenv("HOME");
                cx_root = getenv("CX_ROOT");
                if (home == NULL) home = "/-NON_EXISTENT-";

                /*!!!DIRSTRUCT*/
                pfx1 = home;
                pfx2 = "/pult";

                if (cx_root != NULL)
                {
                    pfx1 = cx_root;
                    pfx2 = "";
                }

                snprintf(cmd_buf, sizeof(cmd_buf),
                         "cd %s%s/settings/%s; exec %s%s/bin/%s%s%s",
                         pfx1, pfx2, sr->name,
                         pfx1, pfx2, sr->name,
                         sr->cp->params==NULL? "": " ",
                         sr->cp->params==NULL? "": sr->cp->params);

                cmd = cmd_buf;
            }
        }
        
        if (cmd != NULL) RunCommand(cmd);

        gettimeofday(&(sr->last_start_time), NULL);
        
        return 1;
    }
}

static int SplitSrvspec(const char *spec,
                        char *hostbuf, size_t hostbufsize,
                        int  *number)
{
  const char *colonp;
  char       *endptr;
  size_t      len;

    if (spec == NULL  ||
        (*spec != ':'  &&  !isalnum(*spec))) return -1;
  
    colonp = strchr(spec, ':');
    if (colonp != NULL)
    {
        *number = (int)(strtol(colonp + 1, &endptr, 10));
        if (endptr == colonp + 1  ||  *endptr != '\0')
            return -1;
        len = colonp - spec;
    }
    else
    {
        *number = 0;
        len = strlen(spec);
    }

    if (len > hostbufsize - 1) len = hostbufsize - 1;
    if (len > 0) memcpy(hostbuf, spec, len);
    hostbuf[len] = '\0';

    return 0;
}

static int ChangeServerStatus(int sys, int ns, int on)
{
  int           curstatus;

  const char   *srvname;
  char          host[256];
  int           instance;
  char         *p;

  const char   *at_host;
  const char   *cf_host;

  const char   *cmd;
  char          cmd_buf[1000];
  char         *c_at;
  char         *c_colon;
  char         *c_user;
  char         *c_usr_at;
  const char   *cx_root;
  const char   *workdir;

  int           srv;
  const char   *ast_p;
  srvparams_t   sprms;
  const char   *params;
    
    /* Should we do anything at all? */
    curstatus = cda_status_of(subsystems[sys].mainsid, ns) != CDA_SERVERSTATUS_DISCONNECTED;
    if (curstatus == on) return 0;

    /* Obtain server's host and instance... */
    srvname = cda_status_srvname(subsystems[sys].mainsid, ns);
    if (SplitSrvspec(srvname, host, sizeof(host), &instance) != 0) return 0;
    for (p = host;  *p != '\0';  p++) *p = tolower(*p);
    at_host = cf_host = host;
    
    /* Dig out all parameters concerning this server */
    bzero(&sprms, sizeof(sprms));
    for (srv = 0; srv < srv_count;  srv++)
        if (strcasecmp(srvname, srv_info[srv].name) == 0
            ||
            ((ast_p = strchr(srv_info[srv].name, '*')) != NULL  &&
             cx_memcasecmp(srvname, srv_info[srv].name, ast_p - srv_info[srv].name) == 0
            )
           )
        {
            if (srv_info[srv].start_cmd != NULL) sprms.start_cmd = srv_info[srv].start_cmd;
            if (srv_info[srv].stop_cmd  != NULL) sprms.stop_cmd  = srv_info[srv].stop_cmd;
            if (srv_info[srv].user      != NULL) sprms.user      = srv_info[srv].user;
            if (srv_info[srv].params    != NULL) sprms.params    = srv_info[srv].params;
        }
    
    /* Commands on this host are run as-is, remote ones via ssh */
    if (IsThisHost(at_host)) at_host = "";
    c_at = c_colon = c_user = c_usr_at = "";
    if (at_host[0] != '\0')
    {
        c_at    = "@";
        c_colon = ":";
        
        if (sprms.user != NULL)
        {
            c_user   = sprms.user;
            c_usr_at = "@";
        }
    }

    /* Okay, let's change the status */    
    if (on)
    {
        if (sprms.start_cmd != NULL)
            cmd = sprms.start_cmd;
        else
        {
            /*!!!DIRSTRUCT*/
            workdir = "~/pult";
            if (at_host[0] == '\0')
            {
                cx_root = getenv("CX_ROOT");
                if (cx_root != NULL) workdir = cx_root;
            }

            params = sprms.params;
            snprintf(cmd_buf, sizeof(cmd_buf),
                     "%s%s%s%s%scd %s && ./sbin/cxsd -c configs/cxd.conf -f configs/blklist%s%s-%d.lst%s%s :%d",
                     c_at, c_user, c_usr_at, at_host, c_colon, workdir,
                     cf_host[0]!= '\0' ? "-"    : "", cf_host, instance,
                     params    != NULL ? " "    : "", 
                     params    != NULL ? params : "",
                     instance);

            cmd = cmd_buf;
        }
    }
    else
    {
        if (sprms.stop_cmd != NULL)
            cmd = sprms.stop_cmd;
        else
        {
            snprintf(cmd_buf, sizeof(cmd_buf),
                     "%s%s%s%s%skill `cat /var/tmp/cxd-%d.pid`",
                     c_at, c_user, c_usr_at, at_host, c_colon, instance);

            cmd = cmd_buf;
        }
    }

    RunCommand(cmd);
    
    return 1;
}

static void RunClientCB(Widget     w          __attribute__((unused)),
                        XtPointer  closure,
                        XtPointer  call_data  __attribute__((unused)))
{
  int         sys = ptr2lint(closure);
  subsys_t   *sr  = subsystems + sys;
  int         n;

    if (sr->cp->kind == SUBSYS_CX)
        for (n = 0;  n < sr->srvcount;  n++)
            if (ChangeServerStatus(sys, n, 1)) SleepBySelect(1000000);
  
    ChangeClientStatus(sys, 1);
}

static void ProgMenuCB(Widget     w          __attribute__((unused)),
                       XtPointer  closure,
                       XtPointer  call_data  __attribute__((unused)))
{
  int         cmd = ptr2lint(closure);
  int         sys = prog_menu_client & 0xFFFF;

    switch (cmd)
    {
        case CMD_START:
            ChangeClientStatus(sys, 1);
            break;

        case CMD_STOP:
            ChangeClientStatus(sys, 0);
            break;

        case CMD_RESTART:
            if (ChangeClientStatus(sys, 0)) SleepBySelect(500000);
            ChangeClientStatus    (sys, 1);
            break;
    }
}

static void ProgMenuHandler(Widget     w  __attribute__((unused)),
                            XtPointer  closure,
                            XEvent    *event,
                            Boolean   *continue_to_dispatch)
{
  int           sys  = ptr2lint(closure);
  XButtonEvent *bev  = (XButtonEvent *) event;

    if (bev->button != Button3) return;
    *continue_to_dispatch = False;

    if (XtIsManaged(prog_menu)) XtUnmanageChild(prog_menu);

    prog_menu_client = sys;
    
    XmMenuPosition(prog_menu, bev);
    XtManageChild(prog_menu);
}


static void ServMenuCB(Widget     w          __attribute__((unused)),
                       XtPointer  closure,
                       XtPointer  call_data  __attribute__((unused)))
{
  int         cmd = ptr2lint(closure);
  int         sys = serv_menu_client & 0xFFFF;
  int         ns  = (serv_menu_client >> 16) & 0xFFFF;

    switch (cmd)
    {
        case CMD_START:
            ChangeServerStatus    (sys, ns, 1);
            break;

        case CMD_STOP:
            ChangeServerStatus    (sys, ns, 0);
            break;

        case CMD_RESTART:
            if (ChangeServerStatus(sys, ns, 0)) SleepBySelect(500000);
            ChangeServerStatus    (sys, ns, 1);
            break;
    }
}

static void ServMenuHandler(Widget     w  __attribute__((unused)),
                            XtPointer  closure,
                            XEvent    *event,
                            Boolean   *continue_to_dispatch)
{
  int           srv  = ptr2lint(closure);
  XButtonEvent *bev  = (XButtonEvent *) event;

    if (bev->button != Button3) return;
    *continue_to_dispatch = False;

    if (XtIsManaged(serv_menu)) XtUnmanageChild(serv_menu);

    serv_menu_client = srv;
    
    XmMenuPosition(serv_menu, bev);
    XtManageChild(serv_menu);
}



/*********************************************************************
*  ?????? section                                                    *
*********************************************************************/

static XhWidget CreateMyLed(XhWidget parent_grid, int x)
{
  XhWidget  w;
  XmString  s;
    
    w = XtVaCreateManagedWidget("LED",
                                xmToggleButtonWidgetClass,
                                parent_grid,
                                XmNtraversalOn,    False,
                                XmNsensitive,      False,
                                XmNlabelString,    s = XmStringCreateSimple(""),
                                XmNindicatorSize,  15,
                                XmNspacing,        0,
                                XmNmarginWidth,    0,
                                XmNmarginHeight,   0,
                                XmNindicatorOn,    XmINDICATOR_BOX,
                                XmNindicatorType,  XmN_OF_MANY,
                                XmNset,            False,
                                XmNvisibleWhenOff, False,
                                NULL);
    XmStringFree(s);

    if (x >= 0) XhGridSetChildPosition(w, x, 0);
    
    return w;
}

static void CatchRightClick(Widget w, XtEventHandler proc, XtPointer client_data)
{
//    XtInsertEventHandler(w, ButtonPressMask, False, proc, client_data, XtListHead);
    XtAddEventHandler   (w, ButtonPressMask, False, proc, client_data);
}

static void AddProgMenu(Widget w, int sys)
{
    CatchRightClick(w, ProgMenuHandler, lint2ptr(sys));
    XmAddToPostFromList(prog_menu, w);
}

static void AddServMenu(Widget w, int sys, int ns)
{
    CatchRightClick(w, ServMenuHandler, lint2ptr((ns << 16) + sys));
    XmAddToPostFromList(serv_menu, w);
}

static int ParseChaninfo(const char *chaninfo,
                         excmd_t formula[], int formula_cap,
                         char *defserver_r, size_t defserver_size)
{
  int            nci;
  const char    *cip;
  const char    *p;
  size_t         len;
  char          *item;

  char           srvbuf[100];
  chanaddr_t     chan;
  
    if (defserver_size > 0) *defserver_r = '\0';
  
    for (nci = 0,                    cip = chaninfo;
         nci < formula_cap - 1  &&  *cip != '\0';
         nci++)
    {
        /* Find next chanref */
        p = strchr(cip, ',');
        if (p == NULL) p = cip + strlen(cip);
        len = p - cip;

        /* Copy it */
        item = malloc(len + 1);
        if (item == NULL) return -1;
        memcpy(item, cip, len);
        item[len] = '\0';

        /* Parse */
        if (cx_parse_chanref(item, srvbuf, sizeof(srvbuf), &chan) != 0) return -1;

        /* If it is the first ref, copy it to defserver */
        if (nci == 0  &&  defserver_size > 0)
            strzcpy(defserver_r, srvbuf, defserver_size);

        /* Okay, which command should we use? */
        if (chan != (chanaddr_t)-1)
            formula[nci] = (excmd_t)CMD_GETP_BYNAME_I(item); /*!!! In fact, we should also add "CMD_POP", for stack balance*/
        else
            formula[nci] = (excmd_t)CMD_ADDSERVER_I  (item);
        
        cip = p;
        if (*cip != '\0') cip++; // Skip ','
    }
    formula[nci] = (excmd_t)CMD_RET_I(0);

    return 0;
}

#define CLEVER_STRDUP(dst, src) do {if(dst == NULL  &&  src != NULL) dst = strdup(src);} while (0)

static void AddSubsystem(XhWidget  grid, int y, const char *name, const char *argv0)
{
  int       sys;
  subsys_t *sr;
  Widget    btn;
  XmString  s;
  XhWidget  cw;
  XhWidget  l2;
  char     *btn_label;

  unsigned char *sep_type;

  void          *handle = NULL;
  subsysdescr_t *info;
  char          *err;

  cfgline_t     *cp;
  int            i;

  char           defserver[100];
  excmd_t        chaninfos[20];
  logchannet_t   netchan;
  elemnet_t      elem;
  groupunit_t    grouping[2];
  
  int            ns;

  static cfgline_t  null_config;
  static cfgline_t  sepr_config;
  
    /* Check if it is already in the list (unless it is a separator) */
    if (name[0] != '-')
        for (sys = 0;  sys < subsyscount;  sys++)
            if (strcasecmp(name, subsystems[sys].name) == 0) return;

    /* Find this system in config */
    bzero(&null_config, sizeof(null_config));
    null_config.kind = SUBSYS_CX;
    cp = &null_config;
    for (i = 0;  i < config_count;  i++)
        if (strcasecmp(name, config_info[i].name) == 0) cp = config_info + i;
    /* Unconditionally map all '-'-separators to sepr_config */
    if (name[0] == '-')
    {
        bzero(&sepr_config, sizeof(sepr_config));
        sepr_config.kind = SUBSYS_SEPR;
        cp = &sepr_config;
    }
    
    /* Allocate space */
    sys = subsyscount;
    sr = subsystems + sys;
    bzero(sr, sizeof(*sr));
    subsyscount++;
    
    /**/
    if ((sr->name = malloc(strlen(name) + 1)) == NULL)
        perror("malloc(.name)"), exit(1);
    strcpy(sr->name, name);

    sr->cp = cp;

    if (name[0] == '-')
    {
        sep_type                     = XmSINGLE_LINE;
        if (name[1] == '-') sep_type = XmDOUBLE_LINE;
        sr->btn =
            btn = XtVaCreateManagedWidget("-", xmSeparatorWidgetClass, grid,
                                          XmNorientation,        XmHORIZONTAL,
                                          XmNseparatorType,      sep_type,
                                          XmNhighlightThickness, 0,
                                          NULL);
        XhGridSetChildPosition (btn,  0, y);
        XhGridSetChildFilling  (btn,  1, 0);
        XhGridSetChildAlignment(btn, +1, 0);

        return;
    }
    
    /* Create a button/caption for this subsystem */
    btn_label = sr->cp->label;
    if (btn_label == NULL  ||  btn_label[0] == '\0') btn_label = name;
    sr->btn =
        btn = XtVaCreateManagedWidget("push_i",
                                      xmPushButtonWidgetClass,
                                      grid,
                                      XmNlabelString, s = XmStringCreateSimple(btn_label),
                                      XmNtraversalOn, False,
                                      NULL);
    XmStringFree(s);
    XtVaGetValues(btn, XmNbackground, &(sr->btn_defbg), NULL);
    XhGridSetChildPosition (btn,  0, y);
    XhGridSetChildFilling  (btn,  1, 0);
    XhGridSetChildAlignment(btn, +1, 0);

    XtAddCallback(btn, XmNactivateCallback,
                  RunClientCB, lint2ptr(sys));
    // "AddProgMenu(btn, sys)" is called later, only if everything is OK
    
    if (cp->kind == SUBSYS_CX)
    {
        if (cp->chaninfo != NULL)
        {
            if (ParseChaninfo(cp->chaninfo,
                              chaninfos, countof(chaninfos),
                              defserver, sizeof(defserver)) != 0)
            {
                fprintf(stderr, "ParseChaninfo(%s:\"%s\"): %s\n",
                        name, cp->chaninfo, cx_strerror(errno));
                goto DEFUNCT;
            }

            bzero(&netchan,  sizeof(netchan));
            netchan.type    = LOGT_READ;
            netchan.kind    = LOGK_CALCED;
            netchan.formula = chaninfos;
            bzero(&elem,     sizeof(elem));
            elem.count      = 1;
            elem.channels   = &netchan;
            bzero(&grouping, sizeof(grouping));
            grouping[0].e   = &elem;
            
            /*!!! if() */
            sr->mainsid = cda_new_server(defserver,
                                         EventProc, lint2ptr(sys),
                                         CDA_REGULAR);
            if (sr->mainsid == CDA_SERVERID_ERROR)
            {
                fprintf(stderr, "[%s] cda_new_server(\"%s\"): %s\n", name, defserver, cx_strerror(errno));
                goto DEFUNCT;
            }
            sr->grouplist = CdrCvtGroupunits2Grouplist(sr->mainsid, grouping);
            if (sr->grouplist == NULL)
            {
                fprintf(stderr, "[%s] CdrCvtGroupunits2Grouplist(): %s\n", name, cx_strerror(errno));
                goto DEFUNCT;
            }
        }
        else
        {
            /* Find a description for it */
            if (CdrOpenDescription(name, argv0, &handle, &info, &err) != 0)
            {
                fprintf(stderr, "OpenDescription(%s): %s\n", name, err);
                goto DEFUNCT;
            }
            
            CLEVER_STRDUP(cp->app_name, info->app_name);
            CLEVER_STRDUP(cp->comment,  info->win_title);
            
            /* Initialize connection */
            sr->mainsid = cda_new_server(info->defserver,
                                         EventProc, lint2ptr(sys),
                                         CDA_REGULAR);
            if (sr->mainsid == CDA_SERVERID_ERROR)
            {
                fprintf(stderr, "[%s] cda_new_server(\"%s\"): %s\n", name, info->defserver, cx_strerror(errno));
                goto DEFUNCT;
            }
            
            if (info->phys_info_count < 0)
            {
                cda_TMP_register_physinfo_dbase((physinfodb_rec_t *)info->phys_info);
            }
            else
            {
                cda_TMP_register_physinfo_dbase(NULL);
                cda_set_physinfo(sr->mainsid, info->phys_info, info->phys_info_count);
            }
            
            /**/
            sr->grouplist = CdrCvtGroupunits2Grouplist(sr->mainsid, info->grouping);
            if (sr->grouplist == NULL)
            {
                fprintf(stderr, "[%s] CdrCvtGroupunits2Grouplist(): %s\n", name, cx_strerror(errno));
                goto DEFUNCT;
            }
            /**/
            
            //dlclose(handle);
        }
            
        cda_run_server(sr->mainsid);
        
        cw = XhCreateGrid(grid, "alarmLeds1");
        XhGridSetSize   (cw, 2, 1);
        XhGridSetGrid   (cw, 0, 0);
        XhGridSetPadding(cw, 0, 0);
        XhGridSetChildPosition (cw, 1, y);
        XhGridSetChildFilling  (cw,  0, 0);
        XhGridSetChildAlignment(cw, -1, 0);
        
        sr->a_led = CreateMyLed(cw, 0);
        sr->d_led = CreateMyLed(cw, 1);
        
        l2 = XhCreateGrid(grid, "alarmLeds2");
        XhGridSetGrid   (l2, 0, 0);
        XhGridSetPadding(l2, 0, 0);
        XhGridSetChildPosition (l2, 2, y);
        XhGridSetChildFilling  (l2,  0, 0);
        XhGridSetChildAlignment(l2, -1, 0);
        sr->srvcount = cda_status_lastn(sr->mainsid) + 1;
        if ((sr->leds = XtCalloc(sr->srvcount, sizeof(*(sr->leds)))) != NULL)
            leds_create(l2, 0-15, sr->leds, sr->srvcount, sr->mainsid);

        for (ns = 0;  ns < sr->srvcount;  ns++)
            AddServMenu(sr->leds[ns].w, sys, ns);
    }
    else
    {
        if (cp->statuschkcmd != NULL)
        {
            sr->s_led = XtVaCreateManagedWidget("LED",
                                                xmToggleButtonWidgetClass,
                                                grid,
                                                XmNtraversalOn,    False,
                                                XmNlabelString,    s = XmStringCreateSimple(""),
                                                XmNindicatorSize,  15,
                                                XmNspacing,        0,
                                                XmNmarginWidth,    0,
                                                XmNmarginHeight,   0,
                                                XmNindicatorOn,    XmINDICATOR_BOX,
                                                XmNindicatorType,  XmN_OF_MANY,
                                                XmNset,            False,
                                                XmNvisibleWhenOff, True,
                                                NULL);
            XmStringFree(s);

            XhGridSetChildPosition(sr->s_led, 2, sys + 1);
        }
    }
    AddProgMenu(btn, sys);
    
    if (cp->comment != NULL) XhSetBalloon(btn, cp->comment);

    return;

 DEFUNCT:
    XtVaSetValues    (btn, XmNsensitive, False, NULL);
    if (handle != NULL) dlclose(handle);
}

static void HelpCB(Widget     w          __attribute__((unused)),
                   XtPointer  closure,
                   XtPointer  call_data  __attribute__((unused)))
{
  static XhWidget  help_box = NULL;
  static char     *help_text =
      "CX-starter является центром управления пульта\xA0- он служит "
      "для запуска прикладных программ и серверов, "
      "а также для слежения за их состоянием.\n"
      "    В колонке \xABName\xBB расположены кнопки запуска приложений; "
      "в колонке \xAB?\xBB отображается текущее состояние приложения "
      "(выход за границы допустимых диапазонов, программные либо "
      "аппаратные ошибки, и т.д.), "
      "а в колонке \xABSrv\xBB отображается состояние серверов, "
      "используемых приложением (зеленый цвет\xA0- все в порядке, "
      "красный\xA0- сервер не запущен, синий\xA0- сервер завис).\n"
      "    <Правая кнопка мыши> на кнопке приложения или лампочке сервера "
      "вызывает меню для запуска либо останова.  Для запуска приложения "
      "достаточно нажать на его кнопку.  Если приложение уже запущено на экране "
      "(но, например, скрыто под другими окнами или свернуто в пиктограмму), "
      "то оно будет вытянуто наверх."
      ;

    if (help_box == NULL)
    {
        help_box = XhCreateStdDlg(onewin, "help", "CX-starter help",
                                  NULL, "CX-starter - справка",
                                  XhStdDlgFOk | XhStdDlgFResizable);

        XtVaCreateManagedWidget("helpMessage", xmTextWidgetClass, help_box,
                                XmNvalue,           help_text,
                                XmNcolumns,         80,
                                XmNeditMode,        XmMULTI_LINE_EDIT,
                                XmNwordWrap,        True,
                                XmNresizeHeight,    True,

                                XmNshadowThickness,       0,
                                XmNeditable,              False,
                                XmNcursorPositionVisible, False,
                                XmNtraversalOn,           False,
                                NULL);
    }

    XhStdDlgShow(help_box);
}


typedef struct
{
  XhWidget  grid;
  int       nlines;
  Widget    c1;
  Widget    c2;
  Widget    c3;
  int       e2;
  int       e3;
} scol_t;

static void InitCol(scol_t *cp, XhWidget parent)
{
  XmString  s;

    bzero(cp, sizeof(*cp));

    /* Create the container grid */
    cp->grid = XhCreateGrid(parent, "grid");
    XhGridSetPadding(cp->grid, 1, 1);
    XhGridSetSize   (cp->grid, 3, MAXSYSTEMS);
    XhGridSetGrid   (cp->grid, 1*0, 0);
    
    /* Captions */
    
    cp->c1 = XtVaCreateManagedWidget("collabel",
                                     xmLabelWidgetClass,
                                     cp->grid,
                                     XmNlabelString, s = XmStringCreateLtoR("Name", xh_charset),
                                     NULL);
    XmStringFree(s);
    XhGridSetChildPosition (cp->c1,  0, 0);
    XhGridSetChildFilling  (cp->c1,  0, 0);
    XhGridSetChildAlignment(cp->c1, -1, +1);
  
    cp->c2 = XtVaCreateManagedWidget("collabel",
                                     xmLabelWidgetClass,
                                     cp->grid,
                                     XmNlabelString, s = XmStringCreateLtoR("?", xh_charset),
                                     NULL);
    XmStringFree(s);
    XhGridSetChildPosition (cp->c2,  1, 0);
    XhGridSetChildFilling  (cp->c2,  0, 0);
    XhGridSetChildAlignment(cp->c2, -1, +1);
  
    cp->c3 = XtVaCreateManagedWidget("collabel",
                                     xmLabelWidgetClass,
                                     cp->grid,
                                     XmNlabelString, s = XmStringCreateLtoR("Srv", xh_charset),
                                     NULL);
    XmStringFree(s);
    XhGridSetChildPosition (cp->c3,  2, 0);
    XhGridSetChildFilling  (cp->c3,  0, 0);
    XhGridSetChildAlignment(cp->c3, -1, +1);
}

static void FiniCol(scol_t *cp)
{
  int  ncols;

    ncols = 3;
    if (cp->e3 == 0  &&  0)
    {
        XtUnmanageChild(cp->c3);
        ncols = 2;
        if (cp->e2 == 0)
        {
            XtUnmanageChild(cp->c2);
            ncols = 1;
        }
    }

    XhGridSetSize(cp->grid, ncols, cp->nlines + 1);
}

static void PopulateWindow(const char *argv0)
{
  XhWidget  wksp;
  Widget    cont;
  scol_t    onecol;
  Widget    leftw;
  Widget    w;
  XmString  s;
  Arg       al[20];
  int       ac;
  
  int       i;

    wksp = XhGetWindowWorkspace(onewin);
    cont = XtVaCreateManagedWidget("container",
                                   xmFormWidgetClass,
                                   wksp,
                                   XmNshadowThickness, 0,
                                   NULL);

    /* I. Create menus */

    /* 1. Program (application) menu */
    ac = 0;
    XtSetArg(al[ac], XmNpopupEnabled, XmPOPUP_DISABLED);  ac++;
    prog_menu = XmCreatePopupMenu(wksp, "prog_menu", al, ac);
    XtVaCreateManagedWidget("", xmLabelWidgetClass, prog_menu,
                            XmNlabelString, s = XmStringCreateLtoR("Application", xh_charset),
                            NULL);
    XmStringFree(s);

    XtVaCreateManagedWidget("", xmSeparatorWidgetClass, prog_menu,
                            XmNorientation, XmHORIZONTAL,
                            NULL);

    prog_menu_start   = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, prog_menu,
                                                XmNlabelString, s = XmStringCreateLtoR("Start", xh_charset),
                                                NULL);
    XmStringFree(s);
    XtAddCallback(prog_menu_start, XmNactivateCallback,
                  ProgMenuCB, (XtPointer)CMD_START);

    prog_menu_stop    = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, prog_menu,
                                                XmNlabelString, s = XmStringCreateLtoR("Stop", xh_charset),
                                                NULL);
    XmStringFree(s);
    XtAddCallback(prog_menu_stop, XmNactivateCallback,
                  ProgMenuCB, (XtPointer)CMD_STOP);

    prog_menu_restart = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, prog_menu,
                                                XmNlabelString, s = XmStringCreateLtoR("Restart", xh_charset),
                                                NULL);
    XmStringFree(s);
    XtAddCallback(prog_menu_restart, XmNactivateCallback,
                  ProgMenuCB, (XtPointer)CMD_RESTART);

    /* 2. Server menu */
    ac = 0;
    XtSetArg(al[ac], XmNpopupEnabled, XmPOPUP_DISABLED);  ac++;
    serv_menu = XmCreatePopupMenu(wksp, "serv_menu", al, ac);
    XtVaCreateManagedWidget("", xmLabelWidgetClass, serv_menu,
                            XmNlabelString, s = XmStringCreateLtoR("Server", xh_charset),
                            NULL);
    XmStringFree(s);

    XtVaCreateManagedWidget("", xmSeparatorWidgetClass, serv_menu,
                            XmNorientation, XmHORIZONTAL,
                            NULL);

    serv_menu_start   = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, serv_menu,
                                                XmNlabelString, s = XmStringCreateLtoR("Start", xh_charset),
                                                NULL);
    XmStringFree(s);
    XtAddCallback(serv_menu_start, XmNactivateCallback,
                  ServMenuCB, (XtPointer)CMD_START);

    serv_menu_stop    = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, serv_menu,
                                                XmNlabelString, s = XmStringCreateLtoR("Stop", xh_charset),
                                                NULL);
    XmStringFree(s);
    XtAddCallback(serv_menu_stop, XmNactivateCallback,
                  ServMenuCB, (XtPointer)CMD_STOP);

#if 0
    serv_menu_restart = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, serv_menu,
                                                XmNlabelString, s = XmStringCreateLtoR("Restart", xh_charset),
                                                NULL);
    XmStringFree(s);
    XtAddCallback(serv_menu_restart, XmNactivateCallback,
                  ServMenuCB, (XtPointer)CMD_RESTART);
#endif

    InitCol(&onecol, cont);

    /* IV. Populate with subsystems */
    
    if      (config_count == 0  &&  reqd_count == 0)
    {
        fprintf(stderr, "%s: no systems are requested and none are listed in config\n",
                argv0);
        exit(1);
    }
    else if (reqd_count != 0)
    {
        for (i = 0;  i < reqd_count;  i++)
        {
            AddSubsystem(onecol.grid, onecol.nlines + 1, reqd_systems[i], argv0);
            onecol.nlines++;
        }
    }
    else
    {
        for (i = 0;  i < config_count;  i++)
        {
            if (strcmp(config_info[i].name, "---") == 0)
            {
                FiniCol(&onecol);
                leftw = onecol.grid;
                InitCol(&onecol, cont);
                attachleft(onecol.grid, leftw, 10);
            }
            else
            {
                AddSubsystem(onecol.grid, onecol.nlines + 1, config_info[i].name, argv0);
                onecol.nlines++;
            }
        }
    }

    FiniCol(&onecol);

    /* V. Help button */
    w = XtVaCreateManagedWidget("helpButton",
                                xmPushButtonWidgetClass,
                                wksp,
                                XmNlabelString,     s = XmStringCreateLtoR("?", xh_charset),
                                XmNtraversalOn,     False,
                                XmNtopAttachment,   XmATTACH_WIDGET,
                                XmNtopWidget,       cont,
                                XmNleftAttachment,  XmATTACH_FORM,
                                XmNrightAttachment, XmATTACH_FORM,
                                XmNleftOffset,      10,
                                XmNrightOffset,     10,
                                NULL);
    XmStringFree(s);
    XtAddCallback(w, XmNactivateCallback, HelpCB, NULL);
    
    KeepaliveProc(NULL, NULL);
}


static void ChildrenCatcher(int sig)
{
  pid_t  pid;
  int    status;
  int    saved_errno;

    saved_errno = errno;
  
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFSIGNALED(status))
            reportinfo("PID=%ld terminated by signal %d",
                       (long)pid, WTERMSIG(status));
    }
    
    set_signal(sig, ChildrenCatcher);

    errno = saved_errno;
}


/*********************************************************************
*  Main() section                                                    *
*********************************************************************/

static char *fallbacks[] =
{
    "*.push_i.shadowThickness:10", /*!!! <- for some reason, doesn't work...*/
    "*helpButton.shadowThickness:1",
    XH_STANDARD_FALLBACKS
};

int main(int argc, char *argv[])
{
  char        app_name [100];
  char        app_class[100];
  const char *bnp;

    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    printf("%s CX-starter, CX version %d.%d\n",
           strcurtime(), CX_VERSION_MAJOR, CX_VERSION_MINOR);
        
    ChildrenCatcher(SIGCHLD);

#if 1
    XhSetColorBinding("CTL3D_BG",       "#d0dbff");
    XhSetColorBinding("CTL3D_TS",       "#f1f4ff");
    XhSetColorBinding("CTL3D_BS",       "#9ca4c0");
    XhSetColorBinding("CTL3D_BG_ARMED", "#e7edff");
#else
    XhSetColorBinding("CTL3D_BG",       "#dbdbdb");
    XhSetColorBinding("CTL3D_TS",       "#f7f7f7");
    XhSetColorBinding("CTL3D_BS",       "#a7a7a7");
#endif
    
#if 1
    //
    XhSetColorBinding("BG_DEFAULT", "#dbe6db");
    XhSetColorBinding("TS_DEFAULT", "#f4f7f4");
    XhSetColorBinding("BS_DEFAULT", "#a4ada4");
    XhSetColorBinding("BG_ARMED",   "#edf2ed");
#elif 0
    // Bolotno-zheltyj
    XhSetColorBinding("BG_DEFAULT", "#dbdb90");
    XhSetColorBinding("TS_DEFAULT", "#f4f4de");
    XhSetColorBinding("BS_DEFAULT", "#a4a46c");
#elif 1
    // VGA color 1 (dark blue)
    XhSetColorBinding("FG_DEFAULT", "#FFFFFF");
    XhSetColorBinding("BG_DEFAULT", "#1818b2");
    XhSetColorBinding("TS_DEFAULT", "#9090da");
    XhSetColorBinding("BS_DEFAULT", "#121286");
#else
    // Cold bluish-gray
    XhSetColorBinding("BG_DEFAULT", "#d0dbff");
    XhSetColorBinding("TS_DEFAULT", "#f1f4ff");
    XhSetColorBinding("BS_DEFAULT", "#9ca4c0");
#endif

    bnp = strrchr(argv[0], '/');
    if (bnp != NULL) bnp++; else bnp = argv[0];
    strzcpy(app_name,  bnp, sizeof(app_name));
    strzcpy(app_class, bnp, sizeof(app_class)); app_class[0] = toupper(app_class[0]);

    XhInitApplication(&argc, argv, app_name, app_class, fallbacks);

    onewin = XhCreateWindow("CX-starter", app_name, app_class,
                            XhWindowUnresizableMask | XhWindowCompactMask,
                            NULL,
                            NULL);
    XhSetWindowIcon(onewin, cx_starter_xpm);

    ParseCommandLine(argc, argv);
    PopulateWindow  (argv[0]);
    ObtainHostIdentity();

    XhShowWindow(onewin);
    XhRunApplication();
    
    return 0;
}
