#ifndef __CXSD_LOGGER_H
#define __CXSD_LOGGER_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <stdarg.h>


/* Values for logline.type, the log-category nambers */
typedef enum {
    /* Logfiles */
    LOGF_SYSTEM   = 0,           /* System messages */
    LOGF_ACCESS   = 1,           /* Access log */
    LOGF_DEBUG    = 2,           /* Debug log */
    LOGF_MODULES  = 3,           /* Driver-API debug log (concerns modules' state and operation) */
    LOGF_HARDWARE = 4,           /* Drivers'/layers' debug log */

    LOGF_count    = 5,

    LOGF_DEFAULT  = LOGF_SYSTEM, /* Uncategorized log */

    /* Logging options */
    LOGF_ERRNO   = 1 << 16       /* Add the ": "+strerror(errno) at the end */
} logtype_t;

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


int   cxsd_log_init(const char *argv0, int verbosity, int consolity);
char *cxsd_log_setf(int cat_mask, const char *path);

void  logline (logtype_t type, int level, const char *format, ...)
               __attribute__ ((format (printf, 3, 4)));
void vloglineX(logtype_t type, int level,
               const char *subaddress, const char *format, va_list ap);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CXSD_LOGGER_H */
