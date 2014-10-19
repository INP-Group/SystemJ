#define __CXSD_CONFIG_C
#include "cxsd_includes.h"

#include "cx_proto.h" // For CX_MAX_SERVER


/*
 *  ParseCommandLine
 *      Parses the command line options.
 *
 *  Effect:
 *      Changes the 'option_*' variables according to the -?s.
 *      Prints the help message and exits if '-h' encountered.
 *      Prints an error message to stderr and exits if illegal
 *      option is encountered.
 */

void ParseCommandLine(int argc, char *argv[])
{
  __label__  ADVICE_HELP, PRINT_HELP;

  int  c;		/* Option character */
  int  err = 0;		/* ++'ed on errors */
  unsigned long instance;
  char *endptr;

  int         log_set_mask;
  int         log_clr_mask;
  char       *log_parse_r;

    while ((c = getopt(argc, argv, "b:c:df:hl:nsu:v:w:T:D")) != EOF)
    {
        switch (c)
        {
            case 'b':
                option_basecyclesize = strtoul(optarg, &endptr, 10);
                if (endptr == optarg  ||  *endptr != '\0')
                {
                    fprintf(stderr,
                            "%s: '-b' argument should be an integer (in us)\n",
                            argv[0]);
                    err++;
                }
                break;
                
            case 'c':
                option_configfile = optarg;
                break;

            case 'd':
                option_dontdaemonize = 1;
                break;

            case 'f':
                option_dbref = optarg;
                break;
                
            case 'h':
                goto PRINT_HELP;

            case 'l':
                log_parse_r = ParseDrvlogCategories(optarg, NULL,
                                                    &log_set_mask, &log_clr_mask);
                if (log_parse_r != NULL)
                {
                    fprintf(stderr,
                            "%s: error parsing -l switch: %s\n",
                            argv[0], log_parse_r);
                    err++;
                }
                option_defdrvlog_mask =
                    (option_defdrvlog_mask &~ log_clr_mask) | log_set_mask;
                break;
                
            case 'n':
                option_norun = 1;
                break;

            case 's':
                option_simulate = 1;
                break;
                
            case 'u':
                //option_username = optarg;
                break;

            case 'v':
                if (*optarg < '0'  ||  *optarg > '9' ||
                    optarg[1] != '\0')
                {
                    fprintf(stderr,
                            "%s: argument of `-v' should be between 0 and 9\n",
                            argv[0]);
                    err++;
                }
                option_verbosity = *optarg - '0';
                break;

            case 'w':
                option_cacherfw = 1;
                if (tolower(*optarg) == 'n'  ||  tolower(*optarg) == 'f'  ||
                    *optarg == '0')
                    option_cacherfw = 0;
                break;

            case 'T':
                option_otokarevs = 1;
                if (tolower(*optarg) == 'n'  ||  tolower(*optarg) == 'f'  ||
                    *optarg == '0')
                    option_otokarevs = 0;
                break;

            case 'D':
                option_donttrapsignals = 1;
                break;

            case '?':
                err++;
        }
    }
    
    if (err) goto ADVICE_HELP;

    /* Do next arg start with ':'?  Then it must be a server #... */
    if (optind < argc  &&  argv[optind] != NULL)
    {
        if (argv[optind][0] == ':')
        {
            instance = strtoul(&(argv[optind][1]), &endptr, 10);
            if (endptr == &(argv[optind][1])  ||  *endptr != '\0')
            {
                fprintf(stderr, "%s: invalid <server> specification\n", argv[0]);
                goto ADVICE_HELP;
            }
            if (instance > CX_MAX_SERVER)
            {
                fprintf(stderr, "%s: server specification out of range 0-%d\n",
                        argv[0], CX_MAX_SERVER);
                normal_exit = 1;
                exit(1);
            }

            option_instance = instance;
            optind++;
        }
    }
    
    if (optind < argc)
    {
        fprintf(stderr, "%s: unexpected argument(s)\n", argv[0]);
        goto ADVICE_HELP;
    }

    return;

 ADVICE_HELP:
    fprintf(stderr, "Try `%s -h' for more information.\n", argv[0]);
    normal_exit = 1;
    exit(1);

 PRINT_HELP:
    printf("Usage: %s [options] [:<server>]\n"
           "\n"
           "Options:\n"
           "  -b TIME   server base cycle size (in us)\n"
           "  -c FILE   use FILE instead of " DEF_FILE_CXSD_CONF "\n"
           "  -d        don't daemonize, i.e. keep console and don't fork()\n"
           "  -D        don't trap signals\n"
           "  -f FILE   !!!TMP!!! read HW list from FILE instead of " DEF_FILE_DEVLIST "\n"
           "  -h        display this help and exit\n"
           "  -n        don't actually run, just parse the config file\n"
           "  -l LIST   default drvlog categories\n"
           "  -s        simulate hardware; for debugging only\n"
           "  -u USER   user to run as: setuid(USER), chdir(~USER/)\n"
           "  -v N      verbosity level: 0 - lowest, 9 - highest (default=%d)\n"
           "  -w Y/N    cache reads from write channels (default=yes)\n"
           "  -T N/Y    signifies presence of otokarev's buggy drivers (default=no)\n"
           "<server> is an integer number of server instance\n"
           , argv[0], DEF_OPT_VERBOSITY);
    normal_exit = 1;
    exit(0);
}


//////////////////////////////////////////////////////////////////////

enum {
    LOGFILE_flags = O_RDWR | O_CREAT | O_APPEND | O_NOCTTY,
    LOGFILE_mode  = 0644
};

#define numberof(array) ( (int)(sizeof(array) / sizeof(array[0])) )

#define ER_UXEOF "unexpected end of file"
#define ER_UXEOL "unexpected end of line"
#define ER_EXID  "identifier expected"
#define ER_NOMEM "out of memory"

static inline int isletter(int c)
{
    return isalpha(c)  ||  c == '_'  ||  c == '-';
}

static inline int isletnum(int c)
{
    return isalnum(c)  ||  c == '_'  ||  c == '-';
}


typedef char string[256];

static FILE   *fp;
static string  line;
static int     linelen;
static int     pos;
static int     prevpos;
static int     linectr;


#define Back() (pos = prevpos)


/*
 *  Report
 *      Prints an error/warning message followed with a newline
 *      on standard error.
 *
 *  Args:
 *      *type -- string, describing the type of message: error or warning.
 *      *format, ap -- as in vprintf(). 
 */

static void Report(const char *type, const char *format, va_list ap)
{
  int  true_pos = 0;
  int  x;

    /* Calc the on-screen position (expanding tabs) */
    for (x = 0; x < pos; x++)
    {
        if (line[x] == '\t')  true_pos = (true_pos + 8) &~ 7;
        else                  ++true_pos;
    }

    fprintf (stderr, "csxd: %s(%d:%d): %s: ",
                     option_configfile, linectr, true_pos + 1, type);
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
}


/*
 *  ReportWarning
 *      Reports a warning via Report().
 *
 *  Args:
 *      as in printf().
 */

static void ReportWarning(const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
    Report("WARNING", format, ap);
    va_end(ap);
}


/*
 *  ReportError
 *      Reports an error via Report() and exits with a status 1.
 *
 *  Args:
 *      as in printf()
 */

static void ReportError(const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
    Report("ERROR", format, ap);
    va_end(ap);

    normal_exit = 1;
    exit(1);
}


/*
 *  RTrim
 *      Trims trailing spaces (as defined by isspace()).
 *
 *  Args:
 *      *buf -- line to be trimmed.
 */

static void RTrim(char *buf)
{
  int tail;

    tail = strlen(buf) - 1;
    while (tail >= 0  &&  isspace(buf[tail]))  buf[tail--] = '\0';
}


/*
 *  SkipWhite
 *      Advances 'pos' to the next non-whitespace char in 'line'.
 *
 *  Returns:
 *      1 if pos<linelen, i.e. if not at the end of line,
 *      0 otherwise.
 */

static int SkipWhite(void)
{
    while (pos < linelen  &&  isspace(line[pos]))  pos++;

    return pos < linelen;
}


/*
 *  AtEOL
 *      Checks if the index is at the end of line.
 *
 *  Returns:
 *      1 if pos<linelen, i.e. if not at the end of line,
 *      0 otherwise.
 */

static int AtEOL(void)
{
    return pos >= linelen;
}


/*
 *  Expect
 *      Checks if a character in the current position is `c',
 *      issues an error otherwise.
 *
 *  Args:
 *      c -- character to expect
 */

static void Expect(char c)
{
    if (AtEOL())
            ReportError("end-of-line encountered while expected `%c'", c);
    if (line[pos] != c)
            ReportError("`%c' expected", c);
    ++pos;
}


/*
 *  WhiteExpect
 *      Does the same as Expect(), but skips whitespace before and after.
 *
 *  Args:
 *      c -- character to expect
 */

static void WhiteExpect(char c)
{
    SkipWhite();
    Expect(c);
    SkipWhite();
}


/*
 *  GetIdentifier
 *      Reads an identifier from the current position 
 *      and converts it to lowercase.
 *
 *  Args:
 *      buf -- buffer to place identifier to
 *
 *  Effect:
 *      Advances `pos' to the next non-isletnum() character.
 */

static void GetIdentifier(string buf)
{
  int   c;
  char *p=buf;

    prevpos = pos;

    if (AtEOL())               ReportError(ER_UXEOL);
    if (!isletter(line[pos]))  ReportError(ER_EXID);

    while(1) {
      c = tolower(line[pos]);
      if (!isletnum(c)) break;
      pos++;
      *(p++) = c;
    }

    *p = '\0';
}


/*
 *  ReadLine
 *      Reads a line from config file, skipping empty lines
 *      and lines, starting with hashes ('#').
 *
 *  Effect:
 *      1. Stores the asciiz data in `line' variable, truncating
 *         trailing '\n' (if any).
 *      2. Sets `linelen=strlen(line)'.
 *      3. Sets `pos' to index the first non-whitespace char.
 *      4. Increments the `linectr'.
 *
 *  Returns:
 *      1 on success,
 *      0 on error or end of file.
 */

static int ReadLine(void)
{
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        ++linectr;
        linelen = strlen(line);
        if (linelen > 0  &&  line[linelen - 1] == '\n')  line[--linelen] = '\0';
        pos = 0;
        SkipWhite();

        if (AtEOL()  ||  line[pos] == '#')  continue;

        return 1;
    }

    return 0;
}


static void SkipSection(void)
{
    while (ReadLine())
            if (line[pos] == '}') return;

    ReportError(ER_UXEOF);
}

static void SectionEnvironment(void)
{
  string  varname,
          varvalue;
  char   *p;
  char    op;

    while (ReadLine())
    {
        if (line[pos] == '}') return;

        op = '=';
        if (line[pos] == '+'  ||  line[pos] == '-')
            op = line[pos++];
        
        GetIdentifier(varname);
        for (p = varname; *p != '\0'; p++)  *p = toupper(*p);

        switch (op)
        {
            case '=':
                WhiteExpect('=');
                strcpy(varvalue, line+pos);
                setenv(varname, varvalue, 1);
                /* Fallthrough to '+' */

            case '+':
                break;

            case '-':
                unsetenv(varname);
        }
    }

    ReportError(ER_UXEOF);
}

static void DefaultEnvironment(void)
{
}


static void SectionLogging(void)
{
  enum { fd_unused = -1, fd_waiting = -2, fd_failed = -3 };

  string  category;
  string  filename;
  int     n;
  int     default_mentioned;
  int     fd;

  const char *categories[LOGn_MAXFILE+1] =
  {
    [LOGn_DEFAULT] = "default",
    [LOGn_DBG1]    = "dbg1",
    [LOGn_DBG2]    = "dbg2",
    [LOGn_ACCESS]  = "access",
    [LOGn_SYSTEM]  = "system"
  };

    for (n = 0; n < numberof(logfds); n++)  logfds[n] = fd_unused;

    while (ReadLine())
    {
        if (line[pos] == '}')
        {
            if (logfds[LOGn_DEFAULT] == fd_unused)
                    ReportError("`default' category must be specified");

            /* Make all omitted categories point to default */
            for (n = 0; n <= LOGn_MAXFILE; n++)
                    if (logfds[n] < 0)
                            logfds[n] = logfds[LOGn_DEFAULT];

            return;
        }

        default_mentioned = 0;
        do
        {
            GetIdentifier(category);

            for (n = 0; n < numberof(categories); n++)
                    if (strcmp(category, categories[n]) == 0) break;
            if (n == numberof(categories))
                    Back(), ReportError("unknown category `%s'", category);
            logfds[n] = fd_waiting;
            if (n == LOGn_DEFAULT) default_mentioned = 1;

            SkipWhite();
            if (line[pos] == ',')
            {
                ++pos;
                SkipWhite();
                continue;
            }
            Expect(':');
            SkipWhite();
            break;
        }
        while (1);

        strcpy(filename, line+pos); RTrim(filename);
        fd = open(filename, LOGFILE_flags, LOGFILE_mode);
        if (fd < 0)
        {
            (default_mentioned ? ReportError : ReportWarning)
                    ("open(%s): %s", filename, strerror(errno));
            fd = fd_failed;
        }
        for (n = 0; n < numberof(logfds); n++)
                if (logfds[n] == fd_waiting)
                        logfds[n] = fd;
    }

    ReportError(ER_UXEOF);
}

static void DefaultLogging(void)
{
  int  fd;
  int  n;

    fd = open(DEF_FILE_LOG, LOGFILE_flags, LOGFILE_mode);
    if (fd < 0) ReportError("open defaultlog (" DEF_FILE_LOG "): %s",
                            strerror(errno));
    for (n = 0; n <= LOGn_MAXFILE; n++) logfds[n] = fd;
}


static void SectionControl(void)
{
  string  dirv;
  FILE   *hf;

    while (ReadLine())
    {
        if (line[pos] == '}') return;

        GetIdentifier(dirv);

        if      (strcmp(dirv, "trusted-hosts-file") == 0)
        {
            WhiteExpect(':');
            strzcpy(config_cxhosts_file, line + pos, 
                    sizeof(config_cxhosts_file));

            /* Check if this file exists */
            hf = fopen(config_cxhosts_file, "r");
            if (hf == NULL)
            {
                ReportWarning("fopen(%s): %s; access will be allowed from any host!",
                              config_cxhosts_file, strerror(errno));
            }
            else
            {
                fclose(hf);
            }
        }

        else if (strcmp(dirv, "operators") == 0)
        {
            WhiteExpect(':');
        }

        else if (strcmp(dirv, "daemon-bin-dir") == 0)
        {
            WhiteExpect(':');

            /*strzcpy(daemon_bin_dir, line + pos,
                    sizeof(daemon_bin_dir));*/
        }
        
        else if (strcmp(dirv, "pid-file-dir") == 0)
        {
            WhiteExpect(':');

            strzcpy(config_pid_file_dir, line + pos,
                    sizeof(config_pid_file_dir));
        }
        
        else Back(), ReportError("unknown directive `%s'", dirv);
    }

    ReportError(ER_UXEOF);
}

static void DefaultControl(void)
{
    if (config_pid_file_dir[0] == '\0')
        strcpy(config_pid_file_dir, ".");
}


static void SectionDrivers(void)
{
  string  dirv;
    
    while (ReadLine())
    {
        if (line[pos] == '}') return;

        GetIdentifier(dirv);

        if (strcmp(dirv, "drivers-dir") == 0)
        {
            WhiteExpect(':');
            strzcpy(config_drivers_dir, line + pos,
                    sizeof(config_drivers_dir));
        }

        else Back(), ReportError("unknown directive `%s'", dirv);
    }
}

static void DefaultDrivers(void)
{
    if (config_drivers_dir[0] == '\0')
        strcpy(config_drivers_dir, ".");
}


/*
 *  ReadConfig
 *      Prepares the default configuration and than
 *      reads the daemon configuration file 
 *
 *  Uses:
 *      option_configfile -- name of the config file.
 */

void ReadConfig      (const char *argv0)
{
  string  id;
  int     n;

  static struct {
      const char  *name;
      void       (*parser)(void);
      void       (*defaults)(void);
      int          used;
  } sections[] = {
      {"environment", SectionEnvironment, DefaultEnvironment, 0},
      {"logging",     SectionLogging,     DefaultLogging,     0},
      {"control",     SectionControl,     DefaultControl,     0},
      {"drivers",     SectionDrivers,     DefaultDrivers,     0},
      {NULL,          NULL,               NULL,               0}
  };

    /* Read the cxd.conf if possible */
    fp = fopen(option_configfile, "r");
    if (fp != NULL)
    {
        while (ReadLine())
        {
            /* ".section" */
            if (
                 (line[pos] != '.') ||
                 (pos++, GetIdentifier(id), strcmp(id, "section"))
               )  ReportError("`.section' expected");
            SkipWhite();
            /* <sectionname> */
            GetIdentifier(id);

            /* Which section? */
            for (n = 0; sections[n].name != NULL; n++)
                    if (strcmp(sections[n].name, id) == 0)  break;
            if (sections[n].name == NULL)
                Back(), ReportError("unknown section `%s'", id);
            if (sections[n].used)
                Back(), ReportError("duplicate section `%s'", id);

            WhiteExpect('{');
            sections[n].parser();
            sections[n].used = 1;
        }

        fclose(fp);
    }
    else
    {
        fprintf(stderr, "%s: unable to open ", argv0);
        perror(option_configfile);
        normal_exit = 1;
        exit(1);
    }

    /* Check if something is still not configured */
    for (n = 0; sections[n].name != NULL; n++)
            if (!sections[n].used) sections[n].defaults();
}
