#ifndef __CDA_ERR_H
#define __CDA_ERR_H


#include <stdarg.h>


void cda_clear_err(void);
void cda_set_err  (const char *format, ...)
    __attribute__((format (printf, 1, 2)));
void cda_vset_err (const char *format, va_list ap);


#endif /* __CDA_ERR_H */
