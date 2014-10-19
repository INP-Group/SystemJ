#ifndef __LOG_FUNC_H
#define __LOG_FUNC_H

#include <stdio.h>

/*
 * In function which prints into log file - cxdlib.c::vloglineX()
 * buffsize for log string is equal 1024, and I don't know avaliable
 * memory size for same log string in cm5307_dbody.c::vDoDriverLog()
 * (this function writes log string into sndbuf).
 * So one can reserv buffer size qual 1024, assuming that more than
 * 1024-chars string never ocurs.
 */

#ifdef __STD_ABSTRACT_IFACE_H
    /* include nothing */
static inline void  MDO_LOG(const char *format, ...)
#else
//    #include "cm5307_dbody.h"
static inline void  MDoDriverLog (int devid, int level, const char *format, ...)
#endif
{
    va_list        ap;
    char           buf[1024];

    bzero(buf, sizeof(buf));

    /* Add specific headers */
#ifndef short_lpref
    #define short_lpref ""
#endif
#ifndef ver
    #define ver ""
#endif
    sprintf(buf + strlen(buf), "%s %s: ", short_lpref, ver);

    /* Print into temp buffer */
    va_start(ap, format);
    vsprintf(buf + strlen(buf), format, ap);
    va_end(ap);

#ifdef __STD_ABSTRACT_IFACE_H
    /* Do real log using abstract driver method */
    DO_LOG("%s", buf);
#else
    /* Do real log using driver method */
    DoDriverLog(devid, level, "%s", buf);
#endif

    return;
}

#endif /* __LOG_FUNC_H */
