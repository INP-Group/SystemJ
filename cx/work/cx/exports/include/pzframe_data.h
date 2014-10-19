#ifndef __PZFRAME_DATA_H
#define __PZFRAME_DATA_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "misc_types.h"
#include "misc_macros.h"
#include "paramstr_parser.h"

#include "cx.h"
#include "cda.h"


enum // pzframe_type_dscr_t.behaviour flags
{
    PZFRAME_B_ARTIFICIAL = 1 << 0,
    PZFRAME_B_NO_SVD     = 1 << 1,
    PZFRAME_B_NO_ALLOC   = 1 << 2,
};
 
enum // pzframe_paramdscr_t.param_type=>pzframe_param_info_t.type values
{
    PZFRAME_PARAM_BIGC      = 0,
    PZFRAME_PARAM_INDIFF    = 1,
    PZFRAME_PARAM_CHAN_ONLY = 2,

    PZFRAME_PARAM_RW_ONLY_MASK = 1 << 15
};

enum
{
    PZFRAME_REASON_BIGC_DATA = 0,
    PZFRAME_REASON_REG_CHANS = 1,  
};


/********************************************************************/
struct _pzframe_data_t_struct;
//--------------------------------------------------------------------

typedef void (*pzframe_data_evproc_t)(struct _pzframe_data_t_struct *pfr,
                                      int                            reason,
                                      int                            info_changed,
                                      void                          *privptr);
typedef int  (*pzframe_data_save_t)  (struct _pzframe_data_t_struct *pfr,
                                      const char *filespec,
                                      const char *subsys, const char *comment);
typedef int  (*pzframe_data_load_t)  (struct _pzframe_data_t_struct *pfr,
                                      const char *filespec);
typedef int  (*pzframe_data_filter_t)(struct _pzframe_data_t_struct *pfr,
                                      const char *filespec,
                                      time_t *cr_time,
                                      char *commentbuf, size_t commentbuf_size);

typedef struct
{
    pzframe_data_evproc_t   evproc;
    pzframe_data_save_t     save;
    pzframe_data_load_t     load;
    pzframe_data_filter_t   filter;
} pzframe_data_vmt_t;

typedef struct
{
    const char *name;
    int         n;
    double      phys_r;
    double      phys_d;
    int         change_important;  // Does affect info_changed
    int         param_type;        // PZFRAME_PARAM_nnn
} pzframe_paramdscr_t;

typedef struct
{
    const char            *type_name;
    int                    behaviour;     // PZFRAME_B_nnn

    cxdtype_t              dtype;
    int                    num_params;
    unsigned int           max_datacount;
    size_t                 dataunits;     // derived from dtype
    size_t                 max_datasize;  // =dataunits*max_datacount

    psp_paramdescr_t      *specific_params;
    pzframe_paramdscr_t   *param_dscrs;

    pzframe_data_vmt_t     vmt;           // Inheritance
} pzframe_type_dscr_t;

typedef pzframe_type_dscr_t (*pzframe_type_dscr_getter_t)(void);

/********************************************************************/

typedef struct
{
    int  phys_r;
    int  phys_d;
    int  change_important;
    int  type;
} pzframe_param_info_t;

typedef struct /* This is used for PSP'ing parameters */
{
    int     p[CX_MAX_BIGC_PARAMS];
} pzframe_params_t;

typedef struct
{
    pzframe_param_info_t  param_info[CX_MAX_BIGC_PARAMS];
    pzframe_params_t      param_iv;  // "iv" -- init values

    int                   readonly;
    int                   maxfrq;
    int                   running;
    int                   oneshot;
} pzframe_cfg_t;

extern psp_paramdescr_t  pzframe_data_text2cfg[];

/********************************************************************/

typedef struct _pzframe_mes_t_struct
{
    void                  *databuf;
    rflags_t               rflags;
    tag_t                  tag;
    int32                  info[CX_MAX_BIGC_PARAMS];
} pzframe_mes_t;

typedef struct
{
    pzframe_data_evproc_t  cb;
    void                  *privptr;
} pzframe_data_cbinfo_t;

typedef struct _pzframe_data_t_struct
{
    pzframe_type_dscr_t   *ftd;
    pzframe_cfg_t          cfg;

    cda_serverid_t         bigc_sid;
    cda_bigchandle_t       bigc_handle;
    int                    bigc_srvcount;

    cda_serverid_t         chan_sid;
    int                    chan_base;
    cda_physchanhandle_t   chan_handles [CX_MAX_BIGC_PARAMS];
    int32                  chan_vals    [CX_MAX_BIGC_PARAMS];

    struct timeval         timeupd;

//    void                  *mes_databuf;
//    rflags_t               mes_rflags;
//    tag_t                  mes_tag;
//    int32                  mes_info[CX_MAX_BIGC_PARAMS];
    pzframe_mes_t          mes;
    int32                  prv_info[CX_MAX_BIGC_PARAMS];

    // Callbacks
    pzframe_data_cbinfo_t *cb_list;
    int                    cb_list_allocd;
} pzframe_data_t;
//-- protected -------------------------------------------------------
void PzframeDataCallCBs(pzframe_data_t *pfr,
                        int             reason,
                        int             info_changed);

/********************************************************************/

static inline void
PzframeFillDscr(pzframe_type_dscr_t *ftd, const char *type_name,
                int behaviour,
                /* Bigc-related characteristics */
                cxdtype_t dtype,
                int num_params,
                unsigned int max_datacount,
                /* Description of bigc-parameters */
                psp_paramdescr_t *specific_params,
                pzframe_paramdscr_t *param_dscrs)
{
    bzero(ftd, sizeof(*ftd));

    ftd->type_name        = type_name;
    ftd->behaviour        = behaviour;
    ftd->num_params       = num_params;
    ftd->dtype            = dtype;
    ftd->dataunits        = sizeof_cxdtype(dtype);
    ftd->max_datacount    = max_datacount;
    ftd->max_datasize     = max_datacount * ftd->dataunits;

    ftd->specific_params  = specific_params;
    ftd->param_dscrs      = param_dscrs;
}

/********************************************************************/

void PzframeDataInit      (pzframe_data_t *pfr, pzframe_type_dscr_t *ftd);

int  PzframeDataRealize   (pzframe_data_t *pfr,
                           const char *srvrspec, int bigc_n,
                           cda_serverid_t def_regsid, int base_chan);

int  PzframeDataAddEvproc (pzframe_data_t *pfr,
                           pzframe_data_evproc_t cb, void *privptr);

void PzframeDataSetRunMode(pzframe_data_t *pfr,
                           int running, int oneshot);

int  PzframeDataFdlgFilter(const char *filespec,
                           time_t *cr_time,
                           char *commentbuf, size_t commentbuf_size,
                           void *privptr);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PZFRAME_DATA_H */
