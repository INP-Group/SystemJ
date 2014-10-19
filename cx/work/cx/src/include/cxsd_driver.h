#ifndef __CXSD_DRIVER_H
#define __CXSD_DRIVER_H


#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "cx.h"
#include "cx_version.h"
#include "cx_module.h"
#include "cxscheduler.h"
#include "fdiolib.h"
#include "misc_macros.h"
#include "paramstr_parser.h"


/* Helper macros and various definitions */

#define DRIVERREC_SUFFIX     _driver_rec
#define DRIVERREC_SUFFIX_STR __CX_STRINGIZE(DRIVERREC_SUFFIX)

enum {CXSRV_DRIVERREC_MAGIC   = 0x12345678};
enum {
    CXSRV_DRIVERREC_VERSION_MAJOR = 8,
    CXSRV_DRIVERREC_VERSION_MINOR = 1,
    CXSRV_DRIVERREC_VERSION = CX_ENCODE_VERSION(CXSRV_DRIVERREC_VERSION_MAJOR,
                                                CXSRV_DRIVERREC_VERSION_MINOR)
};


/* Drivers' definitions */

enum {DEVID_NOT_IN_DRIVER=-1};

enum
{
    DEVSTATE_OFFLINE   = -1,
    DEVSTATE_NOTREADY  =  0,
    DEVSTATE_OPERATING = +1
};

enum
{
    DRVA_READ  = 1,
    DRVA_WRITE = 2
};

enum
{
    RETURNCHAN_COUNT_PONG = 0
};


typedef int  (*CxsdDevInitFunc)(int devid, void *devptr,
                                int businfocount, int businfo[],
                                const char *auxinfo);
typedef void (*CxsdDevTermProc)(int devid, void *devptr);
typedef void (*CxsdDevRWProc)  (int devid, void *devptr,
                                int first, int count, int32 *values, int action);
typedef void (*CxsdDevBigProc) (int devid, void *devptr,
                                int bigc,
                                int32 *args, int nargs,
                                void  *data, size_t datasize, size_t dataunits);


typedef struct
{
    int  count;           // # of channels in this segment
    int  type;            // their type (0-ro, 1-rw)
} CxsdMainInfoRec;

typedef struct
{
    int     count;        // # of bigchannels supported by driver
    int     nargs;        // # of int32 args/settings for a bigchannel
    size_t  datasize;     // size of input data
    size_t  dataunits;    // =1,2,4 -- its units (in fact, unused)
    int     ninfo;        // # of return informational int32s
    size_t  retdatasize;  // size of returned data
    size_t  retdataunits; // =1,2,4 -- its units
} CxsdBigcInfoRec;

typedef struct
{
    cx_module_rec_t   mr;
    
    size_t            privrecsize;
    psp_paramdescr_t *paramtable;
    
    int               min_businfo_n;
    int               max_businfo_n;
    
    const char       *layer;
    int               layerver;

    void             *extensions;

    int               main_nsegs;
    CxsdMainInfoRec  *main_info;
    int               bigc_nsegs;
    CxsdBigcInfoRec  *bigc_info;

    CxsdDevInitFunc   init_dev;
    CxsdDevTermProc   term_dev;
    CxsdDevRWProc     do_rw;
    CxsdDevBigProc    do_big;
} CxsdDriverModRec;


#ifdef TRUE_DEFINE_DRIVER_H_FILE

#include TRUE_DEFINE_DRIVER_H_FILE

#else

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
    }

#endif

/* Public API */
    
enum
{
    DRIVERLOG_LEVEL_mask = (1 << 14) - 1,
    DRIVERLOG_ERRNO      = (1 << 14),

    /* Note: these are unified with LOGL_*, which are in turn unified with syslog()'s  LOG_* */
    DRIVERLOG_EMERG   = 0,
    DRIVERLOG_ALERT   = 1,
    DRIVERLOG_CRIT    = 2,
    DRIVERLOG_ERR     = 3,
    DRIVERLOG_WARNING = 4,
    DRIVERLOG_NOTICE  = 5,
    DRIVERLOG_INFO    = 6,
    DRIVERLOG_DEBUG   = 7,

    DRIVERLOG_C_shift              = 16, DRIVERLOG_C_mask              = 0xFFFF                         << DRIVERLOG_C_shift,
    /* Note: all _CN_ enums must be kept in sync with cxsd_drvmgr.c::catlist[] */
    DRIVERLOG_CN_default           = 0,  DRIVERLOG_C_DEFAULT           = DRIVERLOG_CN_default           << DRIVERLOG_C_shift,
    DRIVERLOG_CN_entrypoint        = 1,  DRIVERLOG_C_ENTRYPOINT        = DRIVERLOG_CN_entrypoint        << DRIVERLOG_C_shift,
    DRIVERLOG_CN_pktdump           = 2,  DRIVERLOG_C_PKTDUMP           = DRIVERLOG_CN_pktdump           << DRIVERLOG_C_shift,
    DRIVERLOG_CN_pktinfo           = 3,  DRIVERLOG_C_PKTINFO           = DRIVERLOG_CN_pktinfo           << DRIVERLOG_C_shift,
    DRIVERLOG_CN_dataconv          = 4,  DRIVERLOG_C_DATACONV          = DRIVERLOG_CN_dataconv          << DRIVERLOG_C_shift,
    DRIVERLOG_CN_remdrv_pktdump    = 5,  DRIVERLOG_C_REMDRV_PKTDUMP    = DRIVERLOG_CN_remdrv_pktdump    << DRIVERLOG_C_shift,
    DRIVERLOG_CN_remdrv_pktinfo    = 6,  DRIVERLOG_C_REMDRV_PKTINFO    = DRIVERLOG_CN_remdrv_pktinfo    << DRIVERLOG_C_shift,
};

#define DRIVERLOG_m_C_TO_CHECKMASK(  C)       (1 << ((C) >> DRIVERLOG_C_shift))
#define DRIVERLOG_m_C_ISIN_CHECKMASK(C, mask) ((mask & DRIVERLOG_m_C_TO_CHECKMASK(C)) != 0)
#define DRIVERLOG_m_CHECKMASK_ALL             -1


void  RegisterDevPtr (int devid, void *devptr);

void  ReturnChanGroup(int devid, int  start, int count, int32 *values, rflags_t *rflags);
void  ReturnChanSet  (int devid, int *addrs, int count, int32 *values, rflags_t *rflags);
void  ReturnBigc     (int devid, int  bigchan, int32 *info, int ninfo, void *retdata, size_t retdatasize, size_t retdataunits, rflags_t rflags);

void  DoDriverLog (int devid, int level, const char *format, ...)
          __attribute__ ((format (printf, 3, 4)));
void  vDoDriverLog(int devid, int level, const char *format, va_list ap);

int   GetCurrentCycle(void);
void  GetCurrentLogSettings(int devid, int *curlevel_p, int *curmask_p);
const char *GetDevLabel    (int devid);

void  SetDevState     (int devid, int state,
                       rflags_t rflags_to_set, const char *description);
void  ReRequestDevData(int devid);

void  StdSimulated_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action);

int   cxsd_uniq_checker(const char *func_name, int uniq);


/* "inserver" API */

typedef int        inserver_devref_t;
typedef chanaddr_t inserver_chanref_t;
typedef chanaddr_t inserver_bigcref_t;

enum {INSERVER_DEVREF_ERROR  = (inserver_devref_t) -1};
enum {INSERVER_CHANREF_ERROR = (inserver_chanref_t)-1};
enum {INSERVER_BIGCREF_ERROR = (inserver_bigcref_t)-1};

typedef void (*inserver_devstat_evproc_t)(int                 devid, void *devptr,
                                          inserver_devref_t   ref,
                                          int                 newstate,
                                          void               *privptr);
typedef void (*inserver_chanref_evproc_t)(int                 devid, void *devptr,
                                          inserver_chanref_t  ref,
                                          int                 reason,
                                          void               *privptr);
typedef void (*inserver_bigcref_evproc_t)(int                 devid, void *devptr,
                                          inserver_bigcref_t  ref,
                                          int                 reason,
                                          void               *privptr);

inserver_devref_t   inserver_get_devref (int                   devid,
                                         const char *dev_name);
inserver_chanref_t  inserver_get_chanref(int                   devid,
                                         const char *dev_name, int chan_n);
inserver_bigcref_t  inserver_get_bigcref(int                   devid,
                                         const char *dev_name, int bigc_n);

int  inserver_get_devstat       (int                        devid,
                                 inserver_devref_t          ref,
                                 int *state_p);

int  inserver_chan_is_rw        (int                        devid,
                                 inserver_chanref_t         ref);
int  inserver_req_cval          (int                        devid,
                                 inserver_chanref_t         ref);
int  inserver_snd_cval          (int                        devid,
                                 inserver_chanref_t         ref,
                                 int32  val);
int  inserver_get_cval          (int                        devid,
                                 inserver_chanref_t         ref,
                                 int32 *val_p, rflags_t *rflags_p, tag_t *tag_p);

int  inserver_req_bigc          (int                        devid,
                                 inserver_bigcref_t         ref,
                                 int32  args[], int     nargs,
                                 void  *data,   size_t  datasize, size_t  dataunits,
                                 int    cachectl);
int  inserver_get_bigc_data     (int                        devid,
                                 inserver_bigcref_t         ref,
                                 size_t  ofs, size_t  size, void *buf,
                                 size_t *retdataunits_p);
int  inserver_get_bigc_stats    (int                        devid,
                                 inserver_bigcref_t         ref,
                                 tag_t *tag_p, rflags_t *rflags_p);
int  inserver_get_bigc_params   (int                        devid,
                                 inserver_bigcref_t         ref,
                                 int  start, int  count, int32 *params);

int  inserver_add_devstat_evproc(int                        devid,
                                 inserver_devref_t          ref,
                                 inserver_devstat_evproc_t  evproc,
                                 void                      *privptr);
int  inserver_del_devstat_evproc(int                        devid,
                                 inserver_devref_t          ref,
                                 inserver_devstat_evproc_t  evproc,
                                 void                      *privptr);

int  inserver_add_chanref_evproc(int                        devid,
                                 inserver_chanref_t         ref,
                                 inserver_chanref_evproc_t  evproc,
                                 void                      *privptr);
int  inserver_del_chanref_evproc(int                        devid,
                                 inserver_chanref_t         ref,
                                 inserver_chanref_evproc_t  evproc,
                                 void                      *privptr);

int  inserver_add_bigcref_evproc(int                        devid,
                                 inserver_bigcref_t         ref,
                                 inserver_bigcref_evproc_t  evproc,
                                 void                      *privptr);
int  inserver_del_bigcref_evproc(int                        devid,
                                 inserver_bigcref_t         ref,
                                 inserver_bigcref_evproc_t  evproc,
                                 void                      *privptr);


/* 2nd layer on top of "inserver" */

#define INSERVER2_STAT_CB_PRESENT 0
typedef struct
{
    int                        m_count;
    char                       targ_name[32];
    int                        chan_n;
    int                        chan_is_bigc;
    /*!!! Should be added (and used!) upon next big upgrade*/
#if INSERVER2_STAT_CB_PRESENT
    inserver_devstat_evproc_t  stat_cb;
#endif
    inserver_chanref_evproc_t  data_cb;
    void                      *privptr;
    //
    inserver_devref_t          targ_ref;
    inserver_chanref_t         chan_ref;
    //
    int                        devid;
    int                        bind_hbt_period;
    sl_tid_t                   bind_hbt_tid;
    /*!!! Should be added (and used!) upon next big upgrade
    int                        is_suffering;
    */
} inserver_refmetric_t;

int  inserver_parse_d_c_ref(int devid, const char *what,
                            const char **pp, inserver_refmetric_t *m_p);

int  inserver_d_c_ref_plugin_parser(const char *str, const char **endptr,
                                    void *rec, size_t recsize,
                                    const char *separators, const char *terminators,
                                    void *privptr, char **errstr);

void inserver_new_watch_list(int  devid, inserver_refmetric_t *m_p, int bind_hbt_period);


#endif /* __CXSD_DRIVER_H */
