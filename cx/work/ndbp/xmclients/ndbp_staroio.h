#ifndef __NDBP_STAROIO_H
#define __NDBP_STAROIO_H


#include "Xh.h"


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __NDBP_STAROIO_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __NDBP_STAROIO_C */


void ndbp_staroio_Listen(XhWindow win, int port1, int port2, const char *chanlist);
void ndbp_staroio_NewData(void);


typedef void (*set_kt_proc_t)(void *privptr, int K, int T);
set_kt_proc_t  set_kt_proc    V(NULL);
void          *set_kt_privptr V(NULL);


#undef D
#undef V


#endif /* __NDBP_STAROIO_H */
