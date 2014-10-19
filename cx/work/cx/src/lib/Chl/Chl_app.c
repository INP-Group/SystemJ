#include "Chl_includes.h"
#include "Xh_fallbacks.h"
#include "cxlib.h"  // For cx_strerror()


/*********************************************************************
  General application starting procedure
*********************************************************************/

enum {DEF_LOG_PERIOD = 60*1000};


typedef struct
{
    int     notoolbar;
    int     freezable;
    int     nostatusline;
    int     is_vertical;
    int     hspc;
    int     vspc;
    int     compact;
    int     resizable;
    int     histring_size;
    double  logperiod;
    int     loaddefsettings;
    int     connect_timeout;
    int     reconnect_time;
} appopts_t;

static psp_paramdescr_t text2appopts[] =
{
    PSP_P_FLAG("notoolbar",       appopts_t, notoolbar,       1, 0),
    PSP_P_FLAG("freezable",       appopts_t, freezable,       1, 0),
    PSP_P_FLAG("nostatusline",    appopts_t, nostatusline,    1, 0),
    PSP_P_FLAG("vertical",        appopts_t, is_vertical,     1, 0),
    PSP_P_INT ("hspc",            appopts_t, hspc,           -1, 0, 0),
    PSP_P_INT ("vspc",            appopts_t, vspc,           -1, 0, 0),
    PSP_P_FLAG("compact",         appopts_t, compact,         1, 0),
    PSP_P_FLAG("resizable",       appopts_t, resizable,       1, 0),
    PSP_P_INT ("histring_size",   appopts_t, histring_size,   0, 0, 86400*10),
    PSP_P_REAL("logperiod",       appopts_t, logperiod,       0, 0, 86400),
    PSP_P_FLAG("loaddefsettings", appopts_t, loaddefsettings, 1, 0),
    PSP_P_END()
};

typedef struct
{
    char *app_name;
    char *app_class;
} cmdlineopts_t;

static psp_paramdescr_t text2cmdlineopts[] =
{
    PSP_P_MSTRING("app_name",  cmdlineopts_t, app_name,  NULL, 0),
    PSP_P_MSTRING("app_class", cmdlineopts_t, app_class, NULL, 0),
    PSP_P_END()
};

static const char *def_fallbacks[] = {XH_STANDARD_FALLBACKS};

static inline int isletnumdot(int c)
{
    return isalnum(c)  ||  c == '_'  ||  c == '-'  ||  c == '.';
}

static int only_letters_and_dot(const char *s, int count)
{
    for (;  count > 0;  s++, count--)
        if (!isletnumdot(*s)) return 0;

    return 1;
}


static XhWindow chl_global_hack_win = NULL;

static void chl_hack_strlogger(const char *str)
{
    if (chl_global_hack_win == NULL)
        fprintf(stderr, "%s %s(): chl_global_hack_win==NULL!  Str=<%s>\n",
                strcurtime(), __FUNCTION__, str);
    else
        ChlRecordEvent(chl_global_hack_win, "LOGSTR: %s", str);
}

static void chl_hack_flipflop_handler(XtPointer closure __attribute__((unused)), XtIntervalId *id __attribute__((unused)))
{
  static int val = 0;

    val = 1 - val;
    set_datatree_flipflop(val);

    XtAppAddTimeOut(xh_context,
                    2000,
                    chl_hack_flipflop_handler,
                    (XtPointer) NULL);
}


int ChlRunApp(int argc, char *argv[],
              const char *app_name,  const char  *app_class,
              const char *title,     const char **icon,
              actdescr_t *toolslist, XhCommandProc  CommandProc,
              const char *options,   const char **fallbacks,
              const char  *defserver, groupunit_t *grouping,
              physprops_t *phys_info, int  phys_info_count,
              ChlDataCallback_t newdataproc, void *privptr)
{
  XhWindow      win;
  winlogrec_t  *lr;  /* Note: can't set here, because window isn't created yet */
  histplotdlg_t *hr;
  datainfo_t   *di;
  int           flag_readonly = 0;

  int           stage;
  int           n;
  typedef enum  {PK_SWITCH, PK_SETTING, PK_SRVR, PK_FILE} pkind_t;
  pkind_t       pkind;
  const char   *p_eq; // '='
  const char   *p_sl; // '/'
  const char   *p_cl; // ':'

  const char   *p;
  char          knobname[100];
  size_t        knobnamelen;
  Knob          ki;
  double        the_v;
  char         *err;

  const char   *srvspec;
  int           toolbar_mask    = XhWindowToolbarMask;
  int           statusline_mask = XhWindowStatuslineMask;
  int           compact_mask    = 0;
  int           resizable_mask  = XhWindowUnresizableMask;

  appopts_t     opts;
  cmdlineopts_t cmdline_opts;

  char          filename[PATH_MAX];


    /* I. Parse and consider options */
    bzero(&opts, sizeof(opts)); // Is required, since none of the flags have defaults specified
    if (psp_parse(options, NULL,
                  &opts,
                  ':', " \t,", "",
                  text2appopts) != PSP_R_OK)
    {
        fprintf(stderr, "%s %s: application %s/\"%s\" options: %s\n",
                strcurtime(), argv[0], app_name, title, psp_error());
        bzero(&opts, sizeof(opts));
        opts.hspc = opts.vspc = -1;
    }
    if (toolslist == NULL) opts.notoolbar = 1;

    if (opts.notoolbar)     toolbar_mask    = 0;
    if (opts.nostatusline)  statusline_mask = 0;
    if (opts.compact)       compact_mask    = XhWindowCompactMask;
    if (opts.resizable)     resizable_mask  = 0;

    if (!opts.freezable  &&  !opts.notoolbar)
        for (n = 0;  toolslist[n].type != XhACT_END;  n++)
            if (toolslist[n].type == XhACT_CHECKBOX  &&
                toolslist[n].cmd  == cmChlFreeze)
            {
                toolslist[n].type = XhACT_NOP;
                break;
            }

    /* II. Init app */
    if (fallbacks == NULL) fallbacks = def_fallbacks;
    XhInitApplication(&argc, argv, app_name, app_class, fallbacks);

    bzero(&cmdline_opts, sizeof(cmdline_opts));
    for (stage  = 0;
         stage <= 1;
         stage++)
    {
        srvspec = NULL;
        
        for (n = 1;  n < argc;  n++)
        {
            /* Paramkind determination heuristics */
            pkind = PK_SRVR;
            p_eq = strchr(argv[n], '=');
            p_sl = strchr(argv[n], '/');
            p_cl = strchr(argv[n], ':');

            // =~ /[a-zA-Z.]=/
            if (p_eq != NULL  &&  only_letters_and_dot(argv[n], p_eq - argv[n]))
                pkind = PK_SETTING;

            // 
            if (argv[n][0] == '/'  ||  memcmp(argv[n], "./", 2) == 0  ||
                (p_sl != NULL  &&  (p_eq == NULL  ||  p_sl < p_eq)))
                pkind = PK_FILE;
            
            if (p_eq == NULL  &&  p_sl == NULL  &&
                (p_cl != NULL))
                pkind = PK_SRVR;

            if (argv[n][0] == '-')
                pkind = PK_SWITCH;


            if (pkind == PK_SWITCH)
            {
                if      (strcmp(argv[n], "-h")     == 0  ||
                         strcmp(argv[n], "-help")  == 0  ||
                         strcmp(argv[n], "--help") == 0)
                {
                    printf("Usage: %s [SERVER] {FILE-TO-LOAD} {KNOB=VALUE}\n",
                           argv[0]);
                    exit(1);
                }
                else if (strcmp(argv[n], "-readonly") == 0)
                    flag_readonly = 1;
                else
                {
                    fprintf(stderr, "%s: unknown switch \"%s\"\n",
                            argv[0], argv[n]);
                    exit(2);
                }
            }
            if (pkind == PK_SETTING)
            {
                if (psp_parse(argv[n], NULL,
                              &cmdline_opts,
                              '=', " \t,", "",
                              text2cmdlineopts) != PSP_R_OK  &&
                    stage == 1)
                {
                    p = p_eq + 1;
                    knobnamelen = p_eq - argv[n];
                    if (knobnamelen > sizeof(knobname) - 1)
                        knobnamelen = sizeof(knobname) - 1;
                    if (knobnamelen == 0)
                    {
                        check_snprintf(_Chl_lasterr_str, sizeof(_Chl_lasterr_str),
                                       "command-line settings can't start with '='");
                        return -1;
                    }
                    memcpy(knobname, argv[n], knobnamelen);
                    knobname[knobnamelen] = '\0';
                    ki = ChlFindKnob(win, knobname);
                    if (ki == NULL)
                    {
                        fprintf(stderr, "%s %s: knob \"%s\" not found\n",
                                strcurtime(), argv[0], knobname);
                    }
                    else
                    {
                        if      (ki->kind == LOGK_USRTXT)
                        {// Copy from Cdr.c::ParseUsrTxt()
                          knobinfo_t  old = *ki;
    
                            while (*p != '\0'  &&  isspace(*p)) p++;
                          
                            if (ki->label == NULL  ||
                                strcmp(p, ki->label) != 0)
                            {
                                safe_free(ki->label);
                                ki->label = strdup(p);
                                if (ki->vmtlink != NULL  &&
                                    ki->vmtlink->PropsChg != NULL)
                                    ki->vmtlink->PropsChg(ki, &old);
                            }
                        }
                        else
                        {
                            the_v = strtod (p, &err);
                            if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
                            {
                                fprintf(stderr, "%s %s: error parsing the value in \"%s\"\n",
                                        strcurtime(), argv[0], argv[n]);
                            }
                            else
                                ChlSetChanVal(win, knobname, the_v);
                        }
                    }
                }
            }
            if (pkind == PK_SRVR)
            {
                if (srvspec != NULL)
                {
                    check_snprintf(_Chl_lasterr_str, sizeof(_Chl_lasterr_str),
                                   "multiple SERVER specifications in command-line");
                    return -1;
                }
                srvspec = argv[n];
            }
            if (pkind == PK_FILE     &&  stage == 1)
            {
                ChlLoadWindowMode(win, argv[n],
                                  opts.loaddefsettings? CDR_OPT_NOLOAD_RANGES : 0);
            }
        }

        if (srvspec == NULL) srvspec = defserver;
             
        /* Perform binding after stage 0 */
        if (stage == 0)
        {
            if (cmdline_opts.app_name  != NULL) app_name  = cmdline_opts.app_name;
            if (cmdline_opts.app_class != NULL) app_class = cmdline_opts.app_class;
            /*  Create a window */
            win = XhCreateWindow(title, app_name, app_class,
                                 toolbar_mask | statusline_mask | compact_mask | resizable_mask,
                                 NULL,
                                 toolslist);
            XhSetWindowIcon   (win, icon);
            XhSetWindowCmdProc(win, CommandProc);
        
            hr = &(Priv2Of(win)->hp);
            hr->def_histring_size = opts.histring_size;
            lr = &(Priv2Of(win)->li);
            if (opts.logperiod > 0) lr->defperiod_ms = opts.logperiod * 1000;
            ////fprintf(stderr, "opts.logperiod=%f lr->defperiod=%d\n", opts.logperiod, lr->defperiod_ms);
            di = DataInfoOf(win);
            di->is_readonly = flag_readonly;
            di->is_protected = opts.loaddefsettings;
        
            /* III. Bind data */
            ChlSetSysname(win, app_name);
            /* Since here, may register logger */
            chl_global_hack_win = win;
            cda_set_strlogger(chl_hack_strlogger);
            
            chl_hack_flipflop_handler(NULL, NULL);
        
            if (phys_info_count < 0)
                cda_TMP_register_physinfo_dbase((physinfodb_rec_t *)phys_info);
            if (ChlSetServer(win, srvspec) < 0)
            {
                check_snprintf(_Chl_lasterr_str, sizeof(_Chl_lasterr_str),
                               "%s(%s): %s", __FUNCTION__, srvspec, cx_strerror(errno));
                return -1;
            }
            if (phys_info_count > 0)
                ChlSetPhysInfo(win, phys_info, phys_info_count);
            if (ChlPopulateWorkspace(win, grouping,
                                     opts.is_vertical, opts.hspc, opts.vspc) != 0)
                return -1;
        }
        /* And load settings, if requested, also after stage 0 */
        if (stage == 0  &&  opts.loaddefsettings)
        {
            check_snprintf(filename, sizeof(filename), CHL_RNGS_PATTERN_FMT, app_name);
            ChlLoadWindowMode(win, filename, CDR_OPT_NOLOAD_VALUES);
        }
    }

    if (newdataproc != NULL) ChlSetDataArrivedCallback(win, newdataproc, privptr);

    /* IV. Go! */
    XhShowWindow(win);
    XhRunApplication();

    return 0;
}

int ChlRunMulticlientApp(int argc, char *argv[],
                         const char *main_filename_c,
                         actdescr_t *toolslist, XhCommandProc  CommandProc,
                         const char **fallbacks,
                         chl_floader_t     floadproc,
                         ChlDataCallback_t newdataproc, void *privptr)
{
  int            directly; // Are we run directly (1) or via symlink (0)?
  const char    *dot_p;    // Points to '.' in "filename.c"
  const char    *fnm_p;    // Points to begin of "filename" in ".../path/filename.c"
  const char    *sys_p;    // Points to "sysname" in ".../progpath/sysname"

  void          *handle;
  subsysdescr_t *info;
  char          *err;

    directly = 0;

    sys_p = strrchr(argv[0], '/');
    if (sys_p != NULL) sys_p++; else sys_p = argv[0];

    if (main_filename_c != NULL)
    {
        dot_p = strrchr(main_filename_c, '.');
        if (dot_p == NULL) dot_p = main_filename_c + strlen(main_filename_c);
        fnm_p = strrchr(dot_p,   '/');
        if (fnm_p != NULL) fnm_p++; else fnm_p = main_filename_c;

        directly = (cx_strmemcmp(sys_p, fnm_p, dot_p - fnm_p) == 0);
    }

    if (directly)
    {
        check_snprintf(_Chl_lasterr_str, sizeof(_Chl_lasterr_str),
                       "this program isn't intended to be run directly");
        return -1;
    }
    else
    {
    }

    if (CdrOpenDescription(sys_p, argv[0], &handle, &info, &err) != 0)
    {
        check_snprintf(_Chl_lasterr_str, sizeof(_Chl_lasterr_str),
                       "OpenDescription(%s): %s", sys_p, err);
        return -1;
    }

    return ChlRunApp(argc, argv,
                     info->app_name, info->app_class,
                     info->win_title, info->icon,
                     toolslist, CommandProc,
                     info->options, fallbacks,
                     info->defserver, info->grouping,
                     info->phys_info, info->phys_info_count,
                     newdataproc, privptr);
}


/*********************************************************************
  Standard command management
*********************************************************************/

static int LoadModeFilter(const char *fname, 
                          time_t *cr_time,
                          char *labelbuf, size_t labelbufsize,
                          void *privptr  __attribute__((unused)))
{
    return ChlStatWindowMode(fname, cr_time, labelbuf, labelbufsize);
}


static void DoLoadOrSave(XhWindow win,
                         const char *filename,
                         const char *comment,
                         int do_save)
{
    do_save ? ChlSaveWindowMode(win, filename, comment)
            : ChlLoadWindowMode(win, filename,
                                DataInfoOf(win)->is_protected? CDR_OPT_NOLOAD_RANGES : 0);
}


int ChlHandleStdCommand(XhWindow window, int cmd)
{
  stddlgsrec_t *dr      = &(Priv2Of(window)->sd);
  winlogrec_t  *lr      = &(Priv2Of(window)->li);
  const char   *sysname = Priv2Of(window)->sysname;
  char          filename[PATH_MAX];
  int           do_save = (cmd == cmChlSaveOk  ||  cmd == cmChlSaveCancel);
  char          comment[400];

    
    switch (cmd)
    {
        case cmChlLoadMode:
            if (dr->load_nfndlg != NULL) XhLoadDlgHide(dr->load_nfndlg);
            check_snprintf(filename, sizeof(filename), CHL_MODE_PATTERN_FMT, sysname);
            if (dr->load_dialog == NULL)
                dr->load_dialog = XhCreateFdlg(window, "loadModeDialog", NULL, NULL, filename, cmChlLoadOk);
            XhFdlgShow(dr->load_dialog);
            break;
        
        case cmChlSaveMode:
            if (dr->save_nfndlg != NULL) XhSaveDlgHide(dr->save_nfndlg);
            check_snprintf(filename, sizeof(filename), CHL_MODE_PATTERN_FMT, sysname);
            if (dr->save_dialog == NULL)
                dr->save_dialog = XhCreateFdlg(window, "saveModeDialog", NULL, NULL, filename, cmChlSaveOk);
            XhFdlgShow(dr->save_dialog);
            break;
        
        case cmChlSaveOk:
        case cmChlLoadOk:
            XhFdlgHide(do_save? dr->save_dialog : dr->load_dialog);
            XhFdlgGetFilename(do_save? dr->save_dialog : dr->load_dialog,
                              filename, sizeof(filename));
            if (filename[0] == '\0') goto RET;
            DoLoadOrSave(window, filename, NULL, do_save);
            break;

        case cmChlSaveCancel:
        case cmChlLoadCancel:
            XhFdlgHide(do_save? dr->save_dialog : dr->load_dialog);
            break;

        case cmChlNfLoadMode:
            check_snprintf(filename, sizeof(filename), CHL_MODE_PATTERN_FMT, sysname);
            if (dr->load_nfndlg == NULL)
                dr->load_nfndlg = XhCreateLoadDlg(window, "loadModeDialog", NULL, NULL, LoadModeFilter, NULL, filename, cmChlNfLoadOk, cmChlLoadMode);
            XhLoadDlgShow(dr->load_nfndlg);
            break;

        case cmChlNfLoadOk:
        case cmChlNfLoadCancel:
            XhLoadDlgHide(dr->load_nfndlg);
            if (cmd == cmChlNfLoadOk)
            {
                XhLoadDlgGetFilename(dr->load_nfndlg, filename, sizeof(filename));
                if (filename[0] == '\0') goto RET;
                DoLoadOrSave(window, filename, NULL, 0);
            }
            break;

        case cmChlNfSaveMode:
            if (dr->save_nfndlg == NULL)
                dr->save_nfndlg = XhCreateSaveDlg(window, "saveModeDialog", NULL, NULL, cmChlNfSaveOk);
            XhSaveDlgShow(dr->save_nfndlg);
            break;

        case cmChlNfSaveOk:
        case cmChlNfSaveCancel:
            XhSaveDlgHide(dr->save_nfndlg);
            if (cmd == cmChlNfSaveOk)
            {
                XhSaveDlgGetComment(dr->save_nfndlg, comment, sizeof(comment));
                check_snprintf(filename, sizeof(filename), CHL_MODE_PATTERN_FMT, sysname);
                XhFindFilename(filename, filename, sizeof(filename));
                DoLoadOrSave(window, filename, comment, 1);
            }
            break;

        case cmChlSwitchLog:
            ChlSwitchLogging(window, lr->defperiod_ms);
            break;

        case cmChlHelp:
            ChlShowHelp(window, CHL_HELP_ALL);
            break;

        case cmChlFreeze:
            ChlFreeze(window, -1);
            break;

        case cmChlDoElog:
            CallELogWindow(window);
            break;
            
        case cmChlQuit:
            exit(0);
        
        default: return 0;
    }

 RET:
    return 1;
}


/*********************************************************************

*********************************************************************/

static void ReportLoadOrSave(XhWindow window,
                             int is_save, const char *filename, int result)
{
  const char *shortname;

    if (result != 0)
    {
        XhMakeMessage(window, "Unable to open \"%s\"", filename);
    }
    else
    {
        shortname = strrchr(filename, '/');
        if (shortname == NULL) shortname = filename;
        else                   shortname++;
        
        XhMakeMessage (window,
                       is_save ? "Режим сохранен в файл \"%s\"."
                       : "Режим считан из файла \"%s\".",
                       shortname);
        ChlRecordEvent(window,
                       "Mode was %s \"%s\"",
                       is_save? "written to" : "read from",
                       filename);
    }
}

int    ChlSaveWindowMode(XhWindow window, const char *path, const char *comment)
{
  int  r = CdrSaveGrouplistMode(DataInfoOf(window)->grouplist, path,
                                Priv2Of(window)->sysname, comment);
    
    ReportLoadOrSave(window, 1, path, r);
  
    return r;
}

int    ChlLoadWindowMode(XhWindow window, const char *path, int options)
{
  int  r = CdrLoadGrouplistMode(DataInfoOf(window)->grouplist, path, options);
    
    ReportLoadOrSave(window, 0, path, r);
  
    return r;
}

int    ChlStatWindowMode(const char *path,
                         time_t *cr_time,
                         char   *commentbuf, size_t commentbuf_size)
{
    return CdrStatGrouplistMode(path, cr_time, commentbuf, commentbuf_size);
}

int    ChlLogWindowMode (XhWindow window, const char *path,
                         const char *comment,
                         int headers, int append,
                         struct timeval *start)
{
    return CdrLogGrouplistMode(DataInfoOf(window)->grouplist, path,
                               comment,
                               headers, append, start);
}

static void LoggingTimeoutHandler(XtPointer closure, XtIntervalId *id __attribute__((unused)))
{
  XhWindow     window  = (XhWindow) closure;
  winlogrec_t *lr      = &(Priv2Of(window)->li);
  int          headers = lr->period_ms < 0;
    
    if (lr->period_ms < 0) lr->period_ms = -lr->period_ms;
    if (headers) gettimeofday(&(lr->start_tv), NULL);
    
    lr->timeout_id = XtAppAddTimeOut(xh_context,
                                     lr->period_ms,
                                     LoggingTimeoutHandler,
                                     (XtPointer) window);

    ChlLogWindowMode(window, lr->filename, NULL, headers, 1, &(lr->start_tv));
}

void   ChlStartLogging  (XhWindow window, const char *filename, int period_ms)
{
  winlogrec_t *lr      = &(Priv2Of(window)->li);
  
    if (lr->filename != NULL) ChlStopLogging(window);

    lr->filename = XtNewString(filename);

    lr->period_ms = -period_ms;
    LoggingTimeoutHandler((XtPointer)window, NULL);
    
    XhMakeMessage(window, "Включено протоколирование в файл \"%s\" (период %dм%02dс).",
                  filename, (period_ms/1000)/60, (period_ms/1000)%60);
}

void   ChlStopLogging   (XhWindow window)
{
  winlogrec_t *lr      = &(Priv2Of(window)->li);
  
    if (lr->filename == NULL) return;

    XtRemoveTimeOut(lr->timeout_id);
    lr->timeout_id = 0;
    XtFree(lr->filename);
    lr->filename = NULL;
    
    XhMakeMessage(window, "Протоколирование отключено.");
}

void   ChlSwitchLogging (XhWindow window, int period_ms)
{
  winlogrec_t *lr      = &(Priv2Of(window)->li);
  const char  *sysname = Priv2Of(window)->sysname;
  char         buf[PATH_MAX];

  fprintf(stderr, "period_ms=%d\n", period_ms);
    if (period_ms <= 0) period_ms = DEF_LOG_PERIOD;
  fprintf(stderr, "NOW period_ms=%d\n", period_ms);

    if (lr->filename != NULL)
    {
        ChlStopLogging(window);
        ChlRecordEvent(window, "Logging stopped");
    }
    else
    {
        snprintf(buf, sizeof(buf), CHL_LOG_PATTERN_FMT, sysname);
        ChlStartLogging(window, buf, period_ms);
        ChlRecordEvent(window, "Started logging with period=%dsec", period_ms/1000);
    }
}

void   ChlRecordEvent   (XhWindow window, const char *format, ...)
{
  const char  *sysname = Priv2Of(window)->sysname;
  char         buf[PATH_MAX];
  FILE        *fp;
  va_list      ap;
  
    snprintf(buf, sizeof(buf), CHL_OPS_PATTERN_FMT, sysname);
    fp = fopen(buf, "a");
    va_start(ap, format);
    if (fp != NULL)
    {
        fprintf (fp, "%s ", strcurtime());
        vfprintf(fp, format, ap);
        fprintf (fp, "\n");
        fclose(fp);
    }
    else
    {
        fprintf (stderr, "Problem opening record-log \"%s\" for append: %s\n",
                 buf, strerror(errno));
        fprintf (stderr, "The record is: %s ", strcurtime());
        vfprintf(stderr, format, ap);
        fprintf (stderr, "\n");
    }
    va_end(ap);
}

char  *ChlLastErr(void)
{
    return _Chl_lasterr_str[0] != '\0'? _Chl_lasterr_str : strerror(errno);
}
