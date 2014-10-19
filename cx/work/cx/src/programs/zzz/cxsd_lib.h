#ifndef __CXSD_LIB_H
#define __CXSD_LIB_H


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __CXSD_LIB_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __CXSD_LIB_C */


/* Logfile numbers */
enum {
    LOGn_DEFAULT = 0,
    LOGn_DBG1    = 1,
    LOGn_DBG2    = 2,
    LOGn_ACCESS  = 3,
    LOGn_SYSTEM  = 4,
    LOGn_MAXFILE = 4
};


/* Values for logline.type */
typedef enum {
    /* Logfiles */
    LOGF_DEFAULT = 1 <<  LOGn_DEFAULT,  /* Uncategorized log */
    LOGF_DBG1    = 1 <<  LOGn_DBG1,     /* 1st debug log */
    LOGF_DBG2    = 1 <<  LOGn_DBG2,     /* 2nd debug log */
    LOGF_ACCESS  = 1 <<  LOGn_ACCESS,   /* Access log */
    LOGF_SYSTEM  = 1 <<  LOGn_SYSTEM,   /* System messages */

    LOGF_ALL     = LOGF_DEFAULT | LOGF_DBG1 | LOGF_DBG2 | LOGF_ACCESS,

    LOGF_SYNCED  = LOGF_ACCESS,         /* Those, which are fsync()ed */

    /* Logging options */
    LOGF_ERRNO   = 1 << 16      /* Add the ": "+strerror(errno) at the end */
} logmask_t;

/* Values for logline.level (yes, these are unified with syslog()'s LOG_*) */
enum
{
    LOGL_EMERG   = 0,
    LOGL_ALERT   = 1,
    LOGL_CRIT    = 2,
    LOGL_ERR     = 3,
    LOGL_WARNING = 4,
    LOGL_NOTICE  = 5,
    LOGL_INFO    = 6,
    LOGL_DEBUG   = 7,
};


D int  logfds[LOGn_MAXFILE+1] V({[0 ... LOGn_MAXFILE] = -1});


void logline  (logmask_t type, int level, const char *format, ...)
              __attribute__ ((format (printf, 3, 4)));
void vlogline (logmask_t type, int level, const char *format, va_list ap);
void vloglineX(logmask_t type, int level, const char *subaddress, const char *format, va_list ap);
void accesslogs(void);


#undef D
#undef V


#endif /* __CXSD_LIB_H */
