#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "cda.h"
#include "cda_err.h"


static char cda_lasterr_str[200] = "";


char *cda_lasterr(void)
{
    return cda_lasterr_str[0] != '\0'? cda_lasterr_str : strerror(errno);
}

void cda_clear_err(void)
{
    cda_lasterr_str[0] = '\0';
}

void cda_set_err  (const char *format, ...)
{
  va_list  ap;
  
    va_start(ap, format);
    cda_vset_err(format, ap);
    va_end(ap);
}

void cda_vset_err (const char *format, va_list ap)
{
    vsnprintf(cda_lasterr_str, sizeof(cda_lasterr_str), format, ap);
    cda_lasterr_str[sizeof(cda_lasterr_str) - 1] = '\0';
}
