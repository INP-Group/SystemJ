#ifndef __CDA_INTERNALS_H
#define __CDA_INTERNALS_H


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __CDA_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __CDA_C */


D int _cda_debug_lapprox  V(0);
D int _cda_debug_setreg   V(0);
D int _cda_debug_setp     V(0);
D int _cda_debug_setchan  V(0);
D int _cda_debug_physinfo V(0);


void _cda_reporterror(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));


int _CdaCheckSid(cda_serverid_t sid);


#undef D
#undef V


#endif /* __CDA_INTERNALS_H */
