#ifndef __BIVME2_DEFINE_DRIVER_H
#define __BIVME2_DEFINE_DRIVER_H


#include "remdrvlet.h"

#include "bivme2_io.h"


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __BIVME2_DEFINE_DRIVER_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __BIVME2_DEFINE_DRIVER_C */


#define DEFINE_DRIVER(name, comment,                                \
                      init_mod, term_mod,                           \
                      privrecsize, paramtable,                      \
                      min_businfo_n, max_businfo_n,                 \
                      layer, layerver,                              \
                      main_nsegs, main_info,                        \
                      bigc_nsegs, bigc_info,                        \
                      init_dev, term_dev, rw_p, bigc_p)             \
    CxsdDriverModRec  __CX_CONCATENATE(name,DRIVERREC_SUFFIX) =     \
    {                                                               \
        {                                                           \
            CXSRV_DRIVERREC_MAGIC, CXSRV_DRIVERREC_VERSION,         \
            __CX_STRINGIZE(name), comment,                          \
            init_mod, term_mod,                                     \
        },                                                          \
        privrecsize, paramtable,                                    \
        min_businfo_n, max_businfo_n,                               \
        layer, layerver,                                            \
        NULL,                                                       \
        main_nsegs, main_info,                                      \
        bigc_nsegs, bigc_info,                                      \
        init_dev, term_dev, rw_p, bigc_p                            \
    };                                                              \
    int main(void)                                                  \
    {                                                               \
      static int8  privrec[privrecsize];                            \
                                                                    \
        return remdrvlet_main(&__CX_CONCATENATE(name,DRIVERREC_SUFFIX), \
                              (privrecsize) != 0? privrec : NULL);  \
    }


/* VME/BIVME2-specific API should go here */


#undef D
#undef V


#endif /* __BIVME2_DEFINE_DRIVER_H */
