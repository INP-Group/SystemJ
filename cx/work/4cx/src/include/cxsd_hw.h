#ifndef __CXSD_HW_H
#define __CXSD_HW_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "cxsd_driver.h"
#include "cxsd_db.h"


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __CXSD_HW_C
  #define D
  #define V(value...) = value
#else
  #define D extern
  #define V(value...)
#endif /* __CXSD_HW_C */


/* Limits */

enum
{
    CXSD_HW_MAX_LYRS          = 99,
    CXSD_HW_MAX_DEVS          = 999,
    CXSD_HW_MAX_CHANS         = 99999,
};


/* Types */

typedef struct
{
    int                  state;         // One of DEVSTATE_NNN
    int                  logmask;       // Mask for logging of various classes of events

    /* Properties */
    CxsdDbDevLine_t     *db_ref;        // Its line in cxsd_hw_cur_db[]
    CxsdDriverModRec    *metric;        // Driver's metric
    int                  lyrid;         // ID of layer serving this driver
    int                  first;         // # of first channel
    int                  count;         // Total # of channels

    /* Driver-support data */
    void                *devptr;        // Device's private pointer
} cxsd_hw_dev_t;

typedef struct
{
    int                  active;
    int                  logmask;       // Mask for logging of various classes of events

    char                 lyrname[32];
    CxsdLayerModRec     *metric;
} cxsd_hw_lyr_t;

typedef void (*cxsd_hw_chan_evproc_t)(int    uniq,
                                      void  *privptr1,
                                      int    globalchan,
                                      int    reason,
                                      void  *privptr2);
typedef struct
{
    int                    evmask;
    int                    uniq;
    void                  *privptr1;
    cxsd_hw_chan_evproc_t  evproc;
    void                  *privptr2;
} cxsd_hw_chan_cbrec_t;
typedef struct
{
    int8            rw;                // 0 -- readonly, 1 -- read/write
    int8            rd_req;            // Read request was sent but yet unserved
    int8            wr_req;            // Write request was sent but yet unserved
    int8            next_wr_val_p;     // Value for next write is pending

    int             devid;             // "Backreference" -- device this channel belongs to
    int             boss;              // if >= 0
    cxdtype_t       dtype;             // Data type
    int             nelems;            // Max # of units
    size_t          usize;             // Size of 1 unit, =sizeof_cxdtype(dtype)

    int             chancol;           // !!!"Color" of this channel (default=0)

    void           *current_val;       // Pointer to current-value...
    int             current_nelems;    // ...and # of units in it
    void           *next_wr_val;       // Pointer to next-value-to-write
    int             next_wr_nelems;    // ...and # of units in it
    rflags_t        rflags;            // Result flags
    rflags_t        crflags;           // Accumulated/cumulative result flags
    cx_time_t       timestamp;         // !!!
    int             timestamp_cn;

    cxsd_hw_chan_cbrec_t *cb_list;
    int             cb_list_allocd;
} cxsd_hw_chan_t;


/* Current-HW-database */

D CxsdDb          cxsd_hw_cur_db;

// Buffers
D uint8          *cxsd_hw_current_val_buf V(NULL);
D uint8          *cxsd_hw_next_wr_val_buf V(NULL);


// Actual counts
D int             cxsd_hw_numlyrs  V(0);
D int             cxsd_hw_numdevs  V(0);
D int             cxsd_hw_numchans V(0);

//
D cxsd_hw_lyr_t   cxsd_hw_layers  [CXSD_HW_MAX_LYRS];
D cxsd_hw_dev_t   cxsd_hw_devices [CXSD_HW_MAX_DEVS];
D cxsd_hw_chan_t  cxsd_hw_channels[CXSD_HW_MAX_CHANS];

D int             cxsd_hw_defdrvlog_mask V(0);


/* Public API */

int  CxsdHwSetDb   (CxsdDb db);
int  CxsdHwActivate(const char *argv0);

int  CxsdHwSetPath    (const char *path);
int  CxsdHwSetSimulate(int state);
int  CxsdHwSetCycleDur(int cyclesize_us);
int  CxsdHwGetCurCycle(void);
int  CxsdHwTimeChgBack(void);


/* Inserver API */

enum
{
    CXSD_HW_CHAN_R_UPDATE  = 0,  CXSD_HW_CHAN_EVMASK_UPDATE = 1 << CXSD_HW_CHAN_R_UPDATE,
};

typedef struct
{
    int   globalchan;
    int   reason;
    int   evmask;
} CxsdHwChanEvCallInfo_t;

int  CxsdHwResolveChan  (const char *name,
                         int        *phys_count,
                         double     *rds_buf,
                         int         rds_buf_cap,
                         char      **ident_p,
                         char      **label_p,
                         char      **tip_p,
                         char      **comment_p,
                         char      **geoinfo_p,
                         char      **rsrvd6_p,
                         char      **units_p,
                         char      **dpyfmt_p);

int  CxsdHwDoIO         (int  requester,
                         int  action,
                         int  count, int *globalchans,
                         cxdtype_t *dtypes, int *nelems, void **values);
int  CxsdHwLockChannels (int  requester,
                         int  count, int *globalchans,
                         int  operation);

int  CxsdHwAddChanEvproc(int  uniq, void *privptr1,
                         int                    globalchan,
                         int                    evmask,
                         cxsd_hw_chan_evproc_t  evproc,
                         void                  *privptr2);
int  CxsdHwDelChanEvproc(int  uniq, void *privptr1,
                         int                    globalchan,
                         int                    evmask,
                         cxsd_hw_chan_evproc_t  evproc,
                         void                  *privptr2);
int  CxsdHwCallChanEvprocs(int globalchan, CxsdHwChanEvCallInfo_t *info);


/* A little bit of macros for interaction with drivers */

D int32 active_devid V(DEVID_NOT_IN_DRIVER);

#define ENTER_DRIVER_S(devid, s) do {s = active_devid; active_devid = devid;} while (0)
#define LEAVE_DRIVER_S(s)        do {active_devid = s;} while (0)


#define DO_CHECK_POSITIVE_SANITY(func_name, errret)                          \
    if      (devid == 0)                                                     \
    {                                                                        \
        logline(LOGF_MODULES, DRIVERLOG_WARNING, "%s(devid=%d/active=%d): devid==0", \
                func_name, devid, active_devid);                             \
        return errret;                                                       \
    }                                                                        \
    else if (devid > 0)                                                      \
    {                                                                        \
        if (devid >= cxsd_hw_numdevs)                                        \
        {                                                                    \
            logline(LOGF_MODULES, DRIVERLOG_WARNING, "%s(devid=%d/active=%d): devid out of [1...numdevs=%d]", \
                    func_name, devid, active_devid, cxsd_hw_numdevs);        \
            return errret;                                                   \
        }                                                                    \
        if (cxsd_hw_devices[devid].state == DEVSTATE_OFFLINE)                \
        {                                                                    \
            logline(LOGF_MODULES, DRIVERLOG_WARNING, "%s(devid=%d/active=%d): is OFFLINE", \
                func_name, devid, active_devid);                             \
            return errret;                                                   \
        }                                                                    \
    }

#define DO_CHECK_SANITY_OF_DEVID(func_name, errret)                          \
    CHECK_POSITIVE_SANITY(errret)                                            \
    else /*  devid < 0 */                                                    \
    {                                                                        \
        logline(LOGF_MODULES, DRIVERLOG_WARNING, "%s(devid=%d/active=%d): operation not supported for layers", \
                func_name, devid, active_devid);                             \
        return errret;                                                       \
    }

#define CHECK_POSITIVE_SANITY(errret) \
    DO_CHECK_POSITIVE_SANITY(__FUNCTION__, errret)
#define CHECK_SANITY_OF_DEVID(errret) \
    DO_CHECK_SANITY_OF_DEVID(__FUNCTION__, errret)


//////////////////////////////////////////////////////////////////////

void cxsd_hw_do_cleanup(int uniq);


#undef D
#undef V


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CXSD_HW_H */
