#ifndef __CM5307_DEFINE_DRIVER_H
#define __CM5307_DEFINE_DRIVER_H


#include "remdrvlet.h"

#include "cm5307_camac.h"
#define DO_NAF  do_naf
#define DO_NAFB do_nafb
#define CAMAC_REF camac_fd


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __CM5307_DEFINE_DRIVER_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __CM5307_DEFINE_DRIVER_C */


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
        camac_fd = init_cm5307_camac();                             \
        /* Any error handling? */                                   \
                                                                    \
        return remdrvlet_main(&__CX_CONCATENATE(name,DRIVERREC_SUFFIX), \
                              (privrecsize) != 0? privrec : NULL);  \
    }


D int  camac_fd          V(-1);

typedef void (*CAMAC_LAM_CB_T)(int devid, void *devptr);
const char * WATCH_FOR_LAM(int devid, void *devptr, int N, CAMAC_LAM_CB_T cb);


#undef D
#undef V


#endif /* __CM5307_DEFINE_DRIVER_H */
