#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cx_sysdeps.h"
#include "misclib.h"

#include "remdrv_proto.h"
#include "remcxsd.h"


void InitRemDevRec(remcxsd_dev_t *dev)
{
    bzero(dev, sizeof(*dev));
    dev->in_use   = 1;
    dev->s        = -1;
    dev->s_fdh    = -1;
    dev->fhandle  = -1;
    dev->tid      = -1;
    dev->loglevel = 0;
    dev->logmask  = DRIVERLOG_m_CHECKMASK_ALL;
}

void remcxsd_debug (const char *format, ...)
{
  va_list  ap;

    fprintf(stderr, "%s ", strcurtime());
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
    fprintf(stderr, "%s: ", program_invocation_short_name);
#endif
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}
