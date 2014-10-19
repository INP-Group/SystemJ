#define __CXSD_LIB_C
#include "cxsd_includes.h"

#include <sys/utsname.h>


#define OPT_LOG_MSECS 1


/*
 *  HostName
 *      Returns a name of the current processor.
 *          On a first call, does a uname() and copies the .nodename
 *      to a static area, than trims out the domain name (if any),
 *      so that only one-word name remains. If the resulting name is
 *      too long (> 31 chars), it is truncated.
 *          If, for some reason, the uname()d name is empty, sets it
 *      to "?host?".
 *
 *  Result:
 *      Pointer to the name.
 */

static char *HostName(void)
{
  static char     name[32] = "";
  struct utsname  buf;
  int             x;

    /* If it is the first call, must obtain the name */
    if (name[0] == '\0')
    {
        uname(&buf);
        strzcpy(name, buf.nodename, sizeof(name));

        for (x = 0;
             x < (signed)sizeof(name) - 1  &&  name[x] != '\0'  &&  name[x] != '.';
             x++);
        name[x] = '\0';
        if (x == 0) strcpy(name, "?host?");
    }

    return name;
}


/*
 *  logline/vlogline
 *      Writes a line to the logfile.
 *      If initlog() wasn't previously called, logs via syslog() instead of
 *      the specified file(s).
 *
 *  Args:
 *      type   -- logfile id, possibly bitwise-ORed with some flags
 *      format -- same as in printf() (is passed to vsprintf())
 *      .../ap -- additional arguments
 */

void logline  (logmask_t type, int level, const char *format, ...)
{
  va_list        ap;
  
    va_start(ap, format);
    vlogline(type, level, format, ap);
    va_end(ap);
}

void vlogline (logmask_t type, int level, const char *format, va_list ap)
{
    vloglineX(type, level, NULL, format, ap);
}

void vloglineX(logmask_t type, int level, const char *subaddress, const char *format, va_list ap)
{
  char           buf[1024];
  struct timeval timenow;
  char          *colon;
  int            logfile;
  fd_set         used;

    /* I. Check the level */
    if (level > option_verbosity) return;


    /* II. Was the logging subsystem initialized? */

    
    /* III. Create line header */
    if (subaddress == NULL  ||  subaddress[0] == '\0')
    {
        subaddress = colon = "";
    }
    else
    {
        colon = ": ";
    }
    
    gettimeofday(&timenow, NULL);

    sprintf(buf, "%s %s %s#%d: %s%s",
#if OPT_LOG_MSECS
            stroftime_msc(&timenow,       " "),
#else
            stroftime    (timenow.tv_sec, " "),
#endif
            HostName(), "cxsd", option_instance,
            subaddress, colon);

    
    /* IV. Add formatted message */
    vsprintf(buf + strlen(buf), format, ap);

    /* Append the error description if required */
    if (type & LOGF_ERRNO)
    {
        strcat(buf, ": ");
        strcat(buf, strerror(errno));
    }
    strcat(buf, "\n");

    
    /* V. Write the line to specified files, avoiding duplication */
    FD_ZERO(&used);
    for (logfile = 0;  logfile <= LOGn_MAXFILE;  logfile++)
            if (
                 (type & (1 << logfile)) &&
                 (! FD_ISSET(logfds[logfile], &used))
               )
            {
                write(logfds[logfile], buf, strlen(buf));
                if (LOGF_SYNCED & (1 << logfile)) fsync(logfds[logfile]);
                FD_SET(logfds[logfile], &used);
            }
}

void accesslogs(void)
{
  int            logfile;
  int8           junk;
  struct stat    sinfo;
  
    for (logfile = 0;  logfile <= LOGn_MAXFILE;  logfile++)
        if (fstat(logfds[logfile], &sinfo) == 0  &&  S_ISREG(sinfo.st_mode))
            read(logfds[logfile], &junk, 1);
}
