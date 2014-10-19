#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "misclib.h"
#include "memcasecmp.h"
#include "cxscheduler.h"
#include "timeval_utils.h"

#include "cxlib.h"
#include "cxdata.h"
#include "cda.h"
#include "Cdr.h"
#include "Knobs_typesP.h"


//////////////////////////////////////////////////////////////////////

#define CHL_MODE_PATTERN_FMT "mode_%s_*.dat"
#define CHL_DATA_PATTERN_FMT "data_%s_*.dat"
#define CHL_LOG_PATTERN_FMT  "log_%s.dat"
#define CHL_OPS_PATTERN_FMT  "events_%s.log"

/*
 *  A slightly modified version of Xh_fdlg.c::XhFindFilename()
 */

/*
 *  Note:
 *      "pattern" and "buf" MAY point to the same space -- it is safe.
 */

static void ChooseFilename(const char *pattern,
                           char *buf, size_t bufsize)
{
  const char         *p;
  const char         *prefix;
  size_t              prefixlen;
  const char         *suffix;
  size_t              suffixlen;
  time_t              timenow = time(NULL);
  struct tm          *tp;
  char                filename[PATH_MAX];
  int                 sofs;

  enum {NDIGITS = 3};
  char                sstr[NDIGITS+1];
  int                 maxserial;
  int                 x;
  struct stat         statbuf;

    p = strchr(pattern, '*');
    if (p == NULL)
    {
        *buf = '\0';
        return;
    }
    
    prefix    = pattern;
    prefixlen = p - pattern;
    suffix    = p + 1;
    suffixlen = strlen(pattern) - prefixlen - 1;

    tp = localtime(&timenow);
    
    snprintf(filename, sizeof(filename), "%*s%04d%02d%02d-%02d%02d_%*s%s",
             (int)prefixlen, "",
             tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday,
             tp->tm_hour, tp->tm_min,
             NDIGITS, "",
             suffix);
    memcpy(filename, prefix, prefixlen);
    sofs = prefixlen + strlen("20070408-1205_");

    for (maxserial = 1, x = 1;  x <= NDIGITS;  x++) maxserial *= 10;
    --maxserial;

    for (x = 0;  x <= maxserial;  x++)
    {
        sprintf(sstr, "%0*d", NDIGITS, x);
        memcpy(filename + sofs, sstr, NDIGITS);

        /* Note: "lstat" instead of "stat" because we want to find
         an unused FILENAME, not a non-existing FILE. */
        if (lstat(filename, &statbuf) != 0) break;
    }
    if (x > maxserial) memset(filename + sofs, 'Z', NDIGITS);
    
    strzcpy(buf, filename, bufsize);
}

//////////////////////////////////////////////////////////////////////

enum
{
    EC_NORMAL   = 0,
    EC_CMD_ERR  = 1,
    EC_INIT_ERR = 1,
};

enum
{
    //
    VERB_NONE = 0,
    VERB_SOME = 1,
    VERB_ALL  = 2,

    //
    QUIET_NONE = 0,
    QUIET_SOME = 1,
    QUIET_ERRS = 2,
};

enum
{
    SERVERS_ALIVE_WAIT_PERIOD = 10,
};

typedef char* (*cmdcheck_t)(const char *param); // NULL - ok, else error string
typedef char* (*cmdproc_t) (const char *param); //

typedef struct _cmddescr_t_struct
{
    const char *name;
    int         periodic;  
    const char *parname;
    const char *help;
    cmdcheck_t  check;
    cmdproc_t   proc;
} cmddescr_t;

typedef struct _cmdinstns_t_struct
{
    int           in_use;
    cmddescr_t   *descr;
    sl_tid_t      tid;
    int           period;  // >0 -- periodic, ==0 -- immediate, <0 -- on-data,
    const char   *param;
} cmdinstns_t;


enum {NUMLOCALREGS = 1000};
static cda_serverid_t  mainsid;
static groupelem_t    *grouplist;
static double          localregs      [NUMLOCALREGS];
static char            localregsinited[NUMLOCALREGS];

static int             modified = 0;

int                    srvcount;

int                   *srvready;
sl_tid_t               srvwait_tid;
enum {SRVWAIT_NONE = 0, SRVWAIT_READY = 1, SRVWAIT_QUIT = 2}
                       waiting_srvready;

static char *argv0;
static char *subsys;
static char *defserver          = NULL;
    
static int   option_force       = 0;
static int   option_keep        = 0;
static int   option_quiet       = 0;
static int   option_verbose     = 0;
static int   option_interactive = 0;

static cmdinstns_t *cmd_queue        = NULL;
static int          cmd_queue_allocd = 0;
static int          cmd_queue_ex_idx = 0;

static struct timeval pause_tv;
static sl_tid_t       pause_tid        = -1;

static cmdinstns_t *prd_list         = NULL;
static int          prd_list_allocd  = 0;

static struct timeval timestamps['Z' - 'A' + 1];

//////////////////////////////////////////////////////////////////////

static char * load_check(const char *param)
{
    if (param[0] == '\0')        return "non-empty filename required";
    if (strcmp(param, ".") == 0) return "specific filename required";
    return NULL;
}
static char * load_proc (const char *param)
{
  int   r;

    r = CdrLoadGrouplistMode(grouplist, param, 0);
    modified = 1;
    
    return NULL;
}

static char * save_check(const char *param)
{
    if (param[0] == '\0')        return "non-empty filename required";
    return NULL;
}
static char * save_proc (const char *param)
{
  int   r;
  char  filename[PATH_MAX];
    
    printf("%s(%s)\n", __FUNCTION__, param);

    if (strcmp(param, ".") == 0)
    {
        check_snprintf(filename, sizeof(filename), CHL_MODE_PATTERN_FMT, subsys);
        ChooseFilename(filename, filename, sizeof(filename));
        param = filename;
    }
    r = CdrSaveGrouplistMode(grouplist, param, subsys, argv0);
    
    return NULL;
}

static char * log_check (const char *param)
{
    if (param[0] == '\0')        return "non-empty filename required";
    return NULL;
}
static char * log_proc  (const char *param)
{
  int   r;
  char  filename[PATH_MAX];
  
  static int             first_log = 1;
  static struct timeval  start_tv;
    
    printf("%s(%s)\n", __FUNCTION__, param);

    if (strcmp(param, ".") == 0)
    {
        check_snprintf(filename, sizeof(filename), CHL_LOG_PATTERN_FMT, subsys);
        param = filename;
    }
    if (first_log) gettimeofday(&start_tv, NULL);
    r = CdrLogGrouplistMode(grouplist, param, NULL, first_log, 1, &start_tv);
    first_log = 0;
    
    return NULL;
}

static char * html_check(const char *param)
{
    if (param[0] == '\0')        return "non-tmpty filename required";
    return NULL;
}
static char * html_proc (const char *param)
{
    printf("%s(%s)\n", __FUNCTION__, param);
    return NULL;
}

static char * set_check (const char *param)
{
  knobinfo_t  *ki;
  char        *p;  // Note: this check-function modifies its param from "KNOB=VALUE" to "KNOB\0VALUE"
  char        *err;
  char         knobname[1000];
  size_t       knobnamelen;

    p = strchr(param, '=');
    if (p == NULL) return "KNOB=VALUE expected";
    knobnamelen = p - param;
    if (knobnamelen > sizeof(knobname) - 1) return "KNOB name too long";
    memcpy(knobname, param, knobnamelen); knobname[knobnamelen] = '\0';
    p++;
    strtod(p, &err);
    if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
        return "invalid VALUE specified";
  
    ki = CdrFindKnob(grouplist, knobname);
    if (ki == NULL) return "no such knob";
    if (!ki->is_rw) return "read-only knob";
    
    return NULL;
}
static char * set_proc  (const char *param)
{
  knobinfo_t  *ki;
  const char  *p;
  cda_localreginfo_t  localreginfo;
  char         knobname[1000];
  size_t       knobnamelen;
  
    localreginfo.count       = NUMLOCALREGS;
    localreginfo.regs        = localregs;
    localreginfo.regsinited  = localregsinited;
    
    p = strchr(param, '=');
    if (p == NULL) return "KNOB=VALUE expected";
    knobnamelen = p - param;
    if (knobnamelen > sizeof(knobname) - 1) return "KNOB name too long";
    memcpy(knobname, param, knobnamelen); knobname[knobnamelen] = '\0';
    p++;
    ki = CdrFindKnob(grouplist, knobname);
    CdrSetKnobValue(ki, strtod(p, NULL), 0, &localreginfo);
    modified = 1;
  
    return NULL;
}

static char * print_check(const char *param)
{
  knobinfo_t  *ki;

    ki = CdrFindKnob(grouplist, param);
    if (ki == NULL) return "no such knob";
  
    return NULL;
}
static char * print_proc (const char *param)
{
  knobinfo_t  *ki;

    ki = CdrFindKnob(grouplist, param);
    printf("%s=", param);
    printf(ki->dpyfmt, ki->curv);
    printf("\n");
    
    return NULL;
}

static char * pause_check(const char *param)
{
  double  v;
  char   *err;

    v = strtod(param, &err);
    if (err == param  ||  *err != '\0')
        return "numeric value (in milliseconds) expected";

    return NULL;
}
static char * pause_proc (const char *param)
{
  double  v;
  char   *err;
  int     ms;
  struct timeval now;

    v = strtod(param, &err);
    if (err == param  ||  *err != '\0')
        return "numeric value (in milliseconds) expected";
    ms = v;
    
    gettimeofday(&now, NULL);
    pause_tv.tv_sec  =  ms / 1000;
    pause_tv.tv_usec = (ms % 1000) * 1000;
    timeval_add(&pause_tv, &pause_tv, &now);

    return NULL;
}

static char * timestamp_check(const char *param)
{
  char    l;

    l = toupper(*param);
    if (l < 'A'  ||  l > 'Z'  ||  param[1] != '\0')
        return "label from A to Z expected";
    timestamps[l-'A'].tv_sec = 1;

    return NULL;
}
static char * timestamp_proc (const char *param)
{
  char    l;

    l = toupper(*param);

    gettimeofday(timestamps + (l-'A'), NULL);

    return NULL;
}

static char * waituntil_check(const char *param)
{
  char    l;
  double  v;
  char   *err;

    l = toupper(*param);
    if (l < 'A'  ||  l > 'Z')
        return "timestamp label from A to Z expected, followed by delay";
    if (timestamps[l-'A'].tv_sec == 0)
        return "timestamp wasn't previously set";

    param++;
    v = strtod(param, &err);
    if (err == param  ||  *err != '\0')
        return "+NNN (numeric value in milliseconds) expected after timestamp label";

    return NULL;
}
static char * waituntil_proc (const char *param)
{
  char    l;
  double  v;
  char   *err;
  int     ms;
  struct timeval dly;

    l = toupper(*param);
    param++;
    v = strtod(param, &err);

    ms = v;
    
    dly.tv_sec  =  ms / 1000;
    dly.tv_usec = (ms % 1000) * 1000;
    timeval_add(&pause_tv, timestamps + (l-'A'), &dly);
    //fprintf(stderr, "%d.%d +%d,%d -> %d.%d\n", timestamps[l-'A'].tv_sec, timestamps[l-'A'].tv_usec, dly.tv_sec, dly.tv_usec, pause_tv.tv_sec, pause_tv.tv_usec);

    return NULL;
}

static char * interactive_proc(const char *param __attribute__((unused)))
{
    option_interactive = 1;
    return NULL;
}

static char * quit_proc(const char *param __attribute__((unused)))
{
    if (option_verbose >= VERB_SOME) printf("%s: quit\n", argv0);
    exit(EC_NORMAL);
    return NULL;
}


static cmddescr_t cmd_set[] =
{
    {"load",      0, "FILENAME",   "load mode from file",        load_check,      load_proc},
    {"save",      1, "FILENAME|.", "save mode to file",          save_check,      save_proc},
    {"log",       1, "FILENAME|.", "log mode to file",           log_check,       log_proc},
    {"html",      1, "FILENAME|.", "dump mode to .html file",    html_check,      html_proc},
    {"set",       0, "KNOB=VALUE", "set VALUE in channel KNOB",  set_check,       set_proc},
    {"print",     1, "KNOB",       "print value of KNOB",        print_check,     print_proc},
    {"pause",     0, "MILLISECONDS",    "delay before next commands", pause_check, pause_proc},
    {"timestamp", 0, "L",          "set timestamp (L=[a-z])",    timestamp_check, timestamp_proc},
    {"waituntil", 0, "L+MS",       "wait until timestamp L+MS",  waituntil_check, waituntil_proc},
    {"interactive", 0, NULL,       "read commands from stdin",   NULL,            interactive_proc},
    {"quit",      0, NULL,         "quit",                       NULL,            quit_proc},
    {NULL, 0, NULL, NULL, NULL, NULL}
};

//////////////////////////////////////////////////////////////////////

static cmdinstns_t * GetCmdSlot(cmdinstns_t **q_p, int *allocd_p)
{
  int          idx;
  cmdinstns_t *slot;
  cmdinstns_t *new_q;

    /* Try to find a free cell */
    for (idx = 0;  idx < *allocd_p; idx++)
        if (!(*q_p)[idx].in_use) goto SLOT_FOUND;

    /* Not found...  Okay -- let's add one more */
    new_q = safe_realloc(*q_p, sizeof(cmdinstns_t) * (*allocd_p + 1));
    *q_p = new_q;
    (*allocd_p)++;
    
 SLOT_FOUND:
    slot = *q_p + idx;
    bzero(slot, sizeof(*slot));
    slot->in_use = 1;

    return slot;
}

static void FreeCmdSlot(cmdinstns_t *slot)
{
    safe_free(slot->param);
    if (slot->tid != 0) sl_deq_tout(slot->tid);
    bzero(slot, sizeof(*slot));
}

static void PrintCmd (FILE *fp, cmdinstns_t *slot)
{
    fprintf(fp, "%s", slot->descr->name);
    if (slot->period != 0)    fprintf(fp, "/%d", slot->period);
    if (slot->param  != NULL) fprintf(fp, " %s", slot->param);
}

static void ListQueue(cmdinstns_t *queue, int queue_allocd)
{
  int          idx;
  
    for (idx = 0;  idx < queue_allocd;  idx++)
        if (queue[idx].in_use)
        {
            PrintCmd(stdout, queue + idx);
            printf("\n");
        }
}

//////////////////////////////////////////////////////////////////////

static void ShowHelp(void)
{
  cmddescr_t  *cd;
  int          maxwid;
  int          width;
  const char  *p_p;
  const char  *s_p;
  const char  *a_p;
  
  static const char *period_suffix = "[/PERIOD]";
    
    printf("Usage: %s [OPTIONS] SUBSYSTEM [DEFSERVER] [COMMANDS]\n", argv0);
    printf(
           "Options:\n"
           "  -f  force operation even if some servers are unavailable\n"
           "  -h  show this help\n"
           "  -k  keep going upon errors\n"
           "  -q  quiet operation (-qq suppresses runtime error messages too)\n"
           "  -v  verbose operation\n"
          );
    printf("Commands:\n");

    for (cd = cmd_set, maxwid = 0; cd->name != NULL;  cd++)
    {
        p_p = cd->periodic != 0   ? period_suffix : "";
        s_p = cd->parname  != NULL? " "           : "";
        a_p = cd->parname  != NULL? cd->parname   : "";
        width = strlen(cd->name) + strlen(p_p) + strlen(s_p) + strlen(a_p);

        if (maxwid < width) maxwid = width;
    }

    for (cd = cmd_set;             cd->name != NULL;  cd++)
    {
        p_p = cd->periodic > 0   ? period_suffix : "";
        s_p = cd->parname != NULL? " "           : "";
        a_p = cd->parname != NULL? cd->parname   : "";
        width = strlen(cd->name) + strlen(p_p) + strlen(s_p) + strlen(a_p);
        
        printf("  %s%s%s%s%*s  %s\n",
               cd->name, p_p, s_p, a_p, maxwid-width, "",
               cd->help);
    }

    printf(
           "Notes:\n"
           "  PERIOD is specified in seconds; commands without PERIOD are executed once\n"
           "  PERIOD of -1 \n"
           "  When \".\" is specified as FILENAME, the name is chosen automatically\n"
          );
}

static void ParseCommandLine(int argc, char *argv[])
{
  int         c;              /* Option character */
  int         err = 0;        /* ++'ed on errors */

  const char  *cmdname;
  const char  *slash_p;
  int          cmdnamelen;
  int          period;

  cmddescr_t  *cd;
  cmdinstns_t *slot;

    while ((c = getopt(argc, argv, "fhkqv")) != EOF)
    {
        switch (c)
        {
            case 'f':
                option_force   = 1;
                break;
            case 'k':
                option_keep    = 1;
                break;
            case 'q':
                option_quiet++;
                option_verbose = 0;
                break;
            case 'v':
                option_verbose++;
                option_quiet   = 0;
                break;
            case 'h':
                ShowHelp();
                exit(EC_NORMAL);
            case ':':
            case '?':
                err++;
        }
    }

    if (err != 0)
    {
        ShowHelp();
        exit(EC_CMD_ERR);
    }

    if (optind >= argc)
    {
        fprintf(stderr, "%s: subsystem name must be specified. Use \"%s -h\" for help\n",
                argv0, argv0);
        exit(EC_CMD_ERR);
    }
    subsys = argv[optind++];

    if (optind < argc  &&  strchr(argv[optind], ':') != NULL)
        defserver = argv[optind++];

    for (; optind < argc;  optind++)
    {
        cmdname = argv[optind];
        slash_p = strchr(cmdname, '/');
        if (slash_p == NULL) cmdnamelen = strlen(cmdname);
        else                 cmdnamelen = slash_p - cmdname;
        if (slash_p == NULL) period = 0;
        else                 period = strtol(slash_p + 1, NULL, 10);
        
        for (cd = cmd_set; cd->name != NULL;  cd++)
            if (cx_strmemcasecmp(cd->name, cmdname, cmdnamelen) == 0) break;

        if (cd->name == NULL)
        {
            fprintf(stderr, "%s: unknown command '%.*s'\n",
                    argv0, cmdnamelen, cmdname);
            if (!option_keep) exit(EC_CMD_ERR);
            goto NEXT_CMD;
        }

        if (cd->periodic == 0  &&  period != 0)
        {
            fprintf(stderr, "%s: the '%s' command doesn't support periodic operation\n",
                    argv0, cd->name);
            if (!option_keep) exit(EC_CMD_ERR);
            period = 0;
        }

        if (cd->parname != NULL  &&  optind + 1 >= argc)
        {
            fprintf(stderr, "%s: the '%s' command requires an argument\n",
                    argv0, cd->name);
            if (!option_keep) exit(EC_CMD_ERR);
            goto NEXT_CMD;
        }

        slot = GetCmdSlot(&cmd_queue, &cmd_queue_allocd);

        slot->descr  = cd;
        slot->period = period;
        if (cd->parname != NULL) slot->param = strdup(argv[++optind]);
        
 NEXT_CMD:;
    }
}

static int ExecOneCmd(cmdinstns_t *slot)
{
  char               *err;

    if (!option_quiet)
    {
        fprintf (stdout, "%s ", strcurtime_msc());
        PrintCmd(stdout, slot);
        fprintf (stdout, "\n");
    }

    pause_tv.tv_sec = pause_tv.tv_usec = 0;
    if (slot->descr->proc != NULL)
        err = slot->descr->proc(slot->param);
    else
        err = NULL;
    
    if (err != NULL)
    {
        fprintf (stderr, "%s: \"", argv0);
        PrintCmd(stderr, slot);
        fprintf (stderr, "\": %s\n", err);
        if (!option_keep) exit(EC_CMD_ERR);
    }

    return err != NULL? -1 : 0;
}

static void PeriodicExecutor(int uniq, void *unsdptr, sl_tid_t tid, void *privptr);
static void ExecPeriodicCmd(cmdinstns_t *slot)
{
    if (ExecOneCmd(slot) < 0)
    {}
    else
        slot->tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                      slot->period*1000, PeriodicExecutor, lint2ptr(slot - prd_list));
}

static void PeriodicExecutor(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
    ExecPeriodicCmd(prd_list + ptr2lint(privptr));
}

static void DoQuitProc(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
    exit(EC_NORMAL);
}

static void ExecCommandsInQueue(void);
static void PauseEnd(int uniq, void *unsdptr1, sl_tid_t tid, void *unsdptr2)
{
    pause_tid = -1;
    ExecCommandsInQueue();
}
static void ExecCommandsInQueue(void)
{
  cmdinstns_t        *slot;
  cmdinstns_t        *pp;

    if (srvwait_tid != 0) sl_deq_tout(srvwait_tid);
    srvwait_tid = -1;

    waiting_srvready = SRVWAIT_NONE;

    for (; cmd_queue_ex_idx < cmd_queue_allocd; cmd_queue_ex_idx++)
        if ((slot = cmd_queue + cmd_queue_ex_idx)->in_use)
        {
            if      (slot->period == 0)
            {
                ExecOneCmd(slot);
                if (pause_tv.tv_sec > 0  ||  pause_tv.tv_usec > 0)
                {
                    pause_tid = sl_enq_tout_at(0, NULL,
                                               &pause_tv, PauseEnd, NULL);
                    cmd_queue_ex_idx++;
                    return;
                }
            }
            else
            {
                pp = GetCmdSlot(&prd_list, &prd_list_allocd);
                pp->descr  = slot->descr;
                pp->period = slot->period;
                if (slot->param != NULL)
                    pp->param = strdup(slot->param);

                if (pp->period > 0)
                {
                    ExecPeriodicCmd(pp);
                }
            }
        }

    if (!(option_interactive || prd_list_allocd > 0))
    {
        if (!modified) exit(EC_NORMAL);
        srvwait_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                        SERVERS_ALIVE_WAIT_PERIOD*1000000, DoQuitProc, NULL);
        bzero(srvready, srvcount * sizeof(*srvready));
        waiting_srvready = SRVWAIT_QUIT;
    }
}

static void DataProc(cda_serverid_t  sid     __attribute__((unused)),
                     int             reason,
                     void           *privptr __attribute__((unused)))
{
  rflags_t            rflags;
  cda_localreginfo_t  localreginfo;

  int                 i;
  int                 ready;

  int                 idx;
  cmdinstns_t        *slot;
  char               *err;

    if (reason < CDA_R_MIN_SID_N) return;
  
    localreginfo.count       = NUMLOCALREGS;
    localreginfo.regs        = localregs;
    localreginfo.regsinited  = localregsinited;
    
    CdrProcessGrouplist(reason, (reason != CDA_R_MAINDATA)? CDR_OPT_SYNTHETIC : 0,
                        &rflags, &localreginfo, grouplist);

    if (waiting_srvready != SRVWAIT_NONE)
    {
        for (i = 0, ready = 0;  i < srvcount;  i++)
        {
            if (cda_status_of(mainsid, i) == CDA_SERVERSTATUS_NORMAL) srvready[i] = 1;
            if (srvready[i]) ready++;
        }

        if (ready == srvcount)
        {
            if (waiting_srvready == SRVWAIT_READY)
            {
                if (option_verbose >= VERB_SOME)
                    printf("%s: successfully connected to all %d server(s)\n",
                           argv0, ready);

                ExecCommandsInQueue();
            }
            else /* waiting_for_quit */
            {
                cda_continue(mainsid);
                exit(EC_NORMAL);
            }
        }
    }


    for (idx = 0;  idx < prd_list_allocd;  idx++)
        if ((slot = prd_list + idx)->in_use  &&
            slot->period < 0                 &&
            (1)                              &&
            slot->descr->proc != NULL)
        {
            err = slot->descr->proc(slot->param);
        }
    
    ////////////////
}

static void SrvWaitProc(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  int                 i;
  int                 ready;

    srvwait_tid = -1;
  
    for (i = 0, ready = 0;  i < srvcount;  i++)
        if (srvready[i]) ready++;
    
    if (ready < srvcount)
    {
        if (option_quiet < QUIET_ERRS)
            fprintf(stderr, "%s: only %d server(s) of %d ready. %s.\n",
                    argv0, ready, srvcount,
                    option_force? "Continuing anyway" : "Aborting");
        if (!option_force) exit(EC_INIT_ERR);
    }

    ExecCommandsInQueue();
}

static void LoadDescription(void)
{
  const char    *srvref;
  void          *handle;
  subsysdescr_t *info;
  char          *err;
    
    if (CdrOpenDescription(subsys, argv0, &handle, &info, &err) != 0)
    {
        fprintf(stderr, "%s: OpenDescription(\"%s\"): %s\n", argv0, subsys, err);
        exit(EC_INIT_ERR);
    }

    srvref  = defserver != NULL? defserver : info->defserver;
    mainsid = cda_new_server(srvref,
                             DataProc, NULL,
                             CDA_REGULAR);
    if (mainsid == CDA_SERVERID_ERROR)
    {
        fprintf(stderr, "%s: cda_new_server(\"%s\"): %s\n", argv0, srvref, cx_strerror(errno));
        exit(EC_INIT_ERR);
    }

    if (info->phys_info_count < 0)
    {
        cda_TMP_register_physinfo_dbase((physinfodb_rec_t *)info->phys_info);
    }
    else
    {
        cda_TMP_register_physinfo_dbase(NULL);
        cda_set_physinfo(mainsid, info->phys_info, info->phys_info_count);
    }

    grouplist = CdrCvtGroupunits2Grouplist(mainsid, info->grouping);
    if (grouplist == NULL)
    {
        fprintf(stderr, "%s: CdrCvtGroupunits2Grouplist(): %s\n", argv0, cx_strerror(errno));
        exit(EC_INIT_ERR);
    }

    srvcount = cda_status_lastn(mainsid) + 1;
    srvready = malloc(srvcount * sizeof(*srvready));
    bzero(srvready, srvcount * sizeof(*srvready));
    waiting_srvready = (srvcount != 0)? SRVWAIT_READY : SRVWAIT_NONE;
    if (waiting_srvready != SRVWAIT_NONE)
        srvwait_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                        SERVERS_ALIVE_WAIT_PERIOD*1000000, SrvWaitProc, NULL);
    
    cda_run_server(mainsid);
}

static void UseDescription(void)
{
}

static void CheckCommands(void)
{
  int          idx;
  cmdinstns_t *slot;
  char        *err;
  char         buf[300];
  
    for (idx = 0;  idx < cmd_queue_allocd;  idx++)
    {
        slot = cmd_queue + idx;
        err = NULL;
        if      (slot->descr->parname != NULL  &&  slot->param[0] == '\0')
            sprintf(err = buf, "non-empty %s required", slot->descr->parname);
        else if (slot->descr->check != NULL)
            err = slot->descr->check(slot->param);

        if (err != NULL)
        {
            fprintf (stderr, "%s: \"", argv0);
            PrintCmd(stderr, slot);
            fprintf (stderr, "\": %s\n", err);
            if (!option_keep) exit(EC_CMD_ERR);
            FreeCmdSlot(slot);
        }
    }
}


int main(int argc, char *argv[])
{
    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);

    argv0 = argv[0];

    ParseCommandLine(argc, argv);
    LoadDescription ();
    UseDescription  ();
    CheckCommands   ();
    //ListQueue       (cmd_queue, cmd_queue_allocd);
    sl_main_loop    ();
    
    return 0;
}
