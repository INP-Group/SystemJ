#include "cxsd_includes.h"

#include "cxsd_fe_cxv2.h"

#include <syslog.h>


/* ==== Cleanup stuff ============================================= */

static void InterceptSignals(void *SIGFATAL_handler,
                             void *SIGCHLD_handler,
                             void *exitproc0 __attribute__((unused)),
                             void *exitproc2 __attribute__((unused)))
{
  static int interesting_signals[] =
  {
      SIGHUP,  SIGINT,    SIGQUIT, SIGILL,  SIGABRT, SIGIOT,  SIGBUS,
      SIGFPE,  SIGUSR1,   SIGSEGV, SIGUSR2, SIGALRM, SIGTERM,
#ifdef SIGSTKFLT
      SIGSTKFLT,
#endif
      SIGXCPU, SIGVTALRM, SIGPROF,
      -1
  };

  int        x;

    /* Intercept signals */
    for (x = 0;  interesting_signals[x] >= 0;  x++)
        set_signal(interesting_signals[x], SIGFATAL_handler);
    
    set_signal(SIGCHLD, SIGCHLD_handler? SIGCHLD_handler : SIG_IGN);
    
    set_signal(SIGPIPE, SIG_IGN);
    set_signal(SIGCONT, SIG_IGN);

    /* And register a call-at-exit handler */
#if OPTION_USE_ON_EXIT
    on_exit(exitproc2, NULL);
#else
    atexit(exitproc0);
#endif /* OPTION_USE_ON_EXIT */
}

static void DoClean(void)
{
    /* For now we do nothing -- since we have ho children. */
}

static void onsig(int sig)
{
    logline(LOGF_SYSTEM, 0, "signal %d (\"%s\") arrived", sig, strsignal(sig));
    DoClean();
    errno = 0;
    exit(sig);
}

static void exitproc2(int code, void *arg __attribute__((unused)))
{
    if (pid_file_created) unlink(pid_file);
    if (normal_exit) return;
    logline(LOGF_SYSTEM, 0, "exit(%d), errno=%d (\"%s\")", 
            code, errno, strerror(errno));
}

static void exitproc0(void)
{
    if (pid_file_created) unlink(pid_file);
    if (normal_exit) return;
    logline(LOGF_SYSTEM, 0, "exit, errno=%d (\"%s\")",
                          errno, strerror(errno));
}

static void PrepareClean(void)
{
    if (!option_donttrapsignals)
        InterceptSignals(onsig, NULL, exitproc0, exitproc2);
}

/* ---- End of cleanup stuff -------------------------------------- */

static void TestHWConfig(const char *argv0)
{
  FILE       *fp;
  const char *filename;

    filename = option_dbref;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr,
                "%s: %s(): fopen(\"%s\"): %s\n",
                argv0, __FUNCTION__, filename, strerror(errno));
        normal_exit = 1;
        exit(1);
    }
    fclose(fp);
}

static void ReadHWConfig(const char *argv0)
{
    if (option_simulate)
        logline(LOGF_SYSTEM, 0, "Simulating the hardware");

    SimulateDatabase(argv0);
}

/*
 *  Daemonize
 *      Makes the program into daemon.
 *      Forks into another process, switches into a new session,
 *      closes stdin/stdout/stderr.
 */

static void Daemonize(void)
{
  int fd;

    /* Substitute with a child to release the caller */
    switch (fork())
    {
        case  0:  break;                                 /* Child */
        case -1:  perror("cxsd can't fork()"); exit(1);  /* Failed to fork */
        default:  normal_exit = 1; exit(0);              /* Parent */
    }

    /* Switch to a new session */
    if (setsid() < 0)
    {
        perror("cxsd can't setsid()");
        exit(1);
    }

    /* Get rid of stdin, stdout and stderr files and terminal */
    /* We don't need to disassociate from tty explicitly, thanks to setsid() */
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1) {
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        if (fd > 2) close(fd);
    }

    errno = 0;
}

/*
 *  CreatePidFile
 *      Creates a file containing the pid, so that "kill `cat cxd-**.pid`"
 *      can be used to terminate the daemon. Upon exit the file is removed.
 */

static void CreatePidFile(const char *argv0)
{
  FILE *fp;
  
    sprintf(pid_file, FILE_CXSDPID_FMT, config_pid_file_dir, option_instance);

    fp = fopen(pid_file, "w");
    if (fp == NULL)
    {
        logline(LOGF_SYSTEM | LOGF_ERRNO, 0,
                "%s: %s: unable to create pidfile \"%s\"",
                argv0, __FUNCTION__, pid_file);
        DoClean();
        normal_exit = 1;
        exit(1);
    }
    if (fprintf(fp, "%d\n", getpid()) < 0  ||  fclose(fp) < 0)
    {
        logline(LOGF_SYSTEM | LOGF_ERRNO, 0,
                "%s: %s: unable to write pidfile \"%s\"",
                argv0, __FUNCTION__, pid_file);
        DoClean();
        normal_exit = 1;
        exit(1);
    }

    pid_file_created = 1;
}

//////////////////////////////////////////////////////////////////////

static void BeginOfCycle(void)
{
    cxsd_fe_cxv2_cycle_beg(); //HandleSubscribedChannels();
}

static void EndOfCycle(void)
{
    //logline(LOGF_DBG1, 0, "---- STROBE EndOfCycle ----");
  
    HandleSimulatedHardware();
    cxsd_fe_cxv2_cycle_end();
  
    ++current_cycle;
}

static struct timeval  cycle_start;  /* The time when the current cycle had started */
static struct timeval  cycle_end;
static sl_tid_t        cycle_tout;

static void CycleCallback(int uniq, void *unsdptr,
                          sl_tid_t tid,                   void *privptr)
{
    EndOfCycle();

    cycle_start = cycle_end;
    timeval_add_usecs          (&cycle_end, &cycle_start, option_basecyclesize);
    cycle_tout = sl_enq_tout_at(0, NULL, /*!!!uniq*/
                                &cycle_end, CycleCallback, NULL);

    BeginOfCycle();
}

static void InitCycles(void)
{
    gettimeofday               (&cycle_start, NULL);
    timeval_add_usecs          (&cycle_end, &cycle_start, option_basecyclesize);
    cycle_tout = sl_enq_tout_at(0, NULL, /*!!!uniq*/
                                &cycle_end, CycleCallback, NULL);

    BeginOfCycle();
}
//////////////////////////////////////////////////////////////////////

enum {ACCESS_HBT_SECS = 600};

static void PerformAccess(int uniq, void *unsdptr,
                          sl_tid_t tid, void *privptr)
{
  int             pid_fd;
  char            pid_buf[1];

    /* Access pid-file... */
    if (pid_file_created  &&  (pid_fd = open(pid_file, O_RDONLY)) >= 0)
    {
        read(pid_fd, pid_buf, sizeof(pid_buf));
        close(pid_fd);
    }
    
    /* ...and logs */
    accesslogs();

    sl_enq_tout_after(0, NULL, /*!!!uniq*/
                      ACCESS_HBT_SECS * 1000000, PerformAccess, NULL);
}

int main(int argc, char *argv[])
{
    ParseCommandLine(argc, argv);
    PrepareClean();

    ReadConfig  (argv[0]);
    openlog("cxsd", 0, LOG_LOCAL0);
    //InitLog()!!!
    sl_set_uniq_checker  (cxsd_uniq_checker);
    fdio_set_uniq_checker(cxsd_uniq_checker);
    TestHWConfig(argv[0]);

    if (cxsd_fe_cxv2_activate() < 0) {normal_exit = 1; exit(1);}
    if (!option_dontdaemonize) Daemonize();

    CreatePidFile(argv[0]);
    sl_enq_tout_after(0, NULL, /*!!!uniq*/
                      ACCESS_HBT_SECS * 1000000, PerformAccess, NULL);

    if (option_norun) {normal_exit = 1; exit(0);}

    ReadHWConfig(argv[0]);
    RequestReadOfWriteChannels();
    InitCycles();

    logline(LOGF_SYSTEM, 0, "*** Start ***");
    sl_main_loop();

    return 1;
}
