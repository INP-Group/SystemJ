#ifndef __PZFRAME_DRV_H
#define __PZFRAME_DRV_H


#include "cxsd_driver.h"


enum
{
    PZFRAME_R_READY = +1,
    PZFRAME_R_DOING =  0,
    PZFRAME_R_ERROR = -1,
    PZFRAME_R_IGNOR = -1, // For now -- the same as ERROR
};


struct _pzframe_drv_t_struct;

typedef int32     (*pzframe_validate_param_t)    (struct _pzframe_drv_t_struct *pdr,
                                                  int n, int32 v);
typedef int       (*pzframe_start_measurements_t)(struct _pzframe_drv_t_struct *pdr);
typedef int       (*pzframe_trggr_measurements_t)(struct _pzframe_drv_t_struct *pdr);
typedef int       (*pzframe_abort_measurements_t)(struct _pzframe_drv_t_struct *pdr);
typedef rflags_t  (*pzframe_read_measurements_t) (struct _pzframe_drv_t_struct *pdr);

typedef struct _pzframe_drv_t_struct
{
    int                           devid;

    int                           num_params;
    int                           param_shot;
    int                           param_istart;
    int                           param_waittime;
    int                           param_stop;
    int                           param_elapsed;

    pzframe_validate_param_t      validate_param;
    pzframe_start_measurements_t  start_measurements;
    pzframe_trggr_measurements_t  trggr_measurements;
    pzframe_abort_measurements_t  abort_measurements;
    pzframe_read_measurements_t   read_measurements;

    int                           state; // Opaque -- used by pzframe_drv internally
    int                           measuring_now;
    sl_tid_t                      tid;
    struct timeval                measurement_start;
    int32                         cur_args[CX_MAX_BIGC_PARAMS];
    int32                         nxt_args[CX_MAX_BIGC_PARAMS];
    void                         *retdata_p;
    size_t                        retdatasize;
    size_t                        retdataunits;
} pzframe_drv_t;


void  pzframe_drv_init(pzframe_drv_t *pdr, int devid,
                       int                           num_params,
                       int                           param_shot,
                       int                           param_istart,
                       int                           param_waittime,
                       int                           param_stop,
                       int                           param_elapsed,
                       pzframe_validate_param_t      validate_param,
                       pzframe_start_measurements_t  start_measurements,
                       pzframe_trggr_measurements_t  trggr_measurements,
                       pzframe_abort_measurements_t  abort_measurements,
                       pzframe_read_measurements_t   read_measurements);
void  pzframe_drv_term(pzframe_drv_t *pdr);

void  pzframe_drv_rw_p  (pzframe_drv_t *pdr,
                         int firstchan, int count,
                         int32 *values, int action);
void  pzframe_drv_bigc_p(pzframe_drv_t *pdr,
                         int32 *args, int nargs);
void  pzframe_drv_drdy_p(pzframe_drv_t *pdr, int do_return, rflags_t rflags);
void  pzframe_set_buf_p (pzframe_drv_t *pdr, void *retdata_p);


#endif /* __PZFRAME_DRV_H */
