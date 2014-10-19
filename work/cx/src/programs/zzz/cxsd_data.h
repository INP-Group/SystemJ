#ifndef __CXSD_DATA_H
#define __CXSD_DATA_H


#include "cxscheduler.h"
#include "fdiolib.h"

#include "cxsd_driver.h"
#include "cxsd_module.h"


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __CXSD_DATA_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __CXSD_DATA_C */


enum
{
    CXSD_DB_MAX_BUSINFOCOUNT = 20,
};


typedef int32 cycle_t;


/*==== "Database" ==================================================*/

/*

  All "tables" arrays have prefixes:
    c_*  regular channels
    b_*  big channels
    d_*  devices

 */

typedef char cxsd_devname[32];

/* Limits */
enum {CX_MAX_DEVS  = 999};
enum {CX_MAX_CHANS = 80000};
enum {CX_MAX_BIGCS = 10000};

enum {CX_MAX_TOUTS_PER_DEV          = 30};
enum {CXSD_HW_MAX_FDIOS_PER_DEV     = 30};
enum {CX_MAX_DATAREF_EVENTS_PER_DEV = 100};
enum {CX_MAX_BIGCREF_EVENTS_PER_DEV = 30};
enum {CX_MAX_BIGCRQ_HOLDERS_PER_DEV = 30};
enum {CX_MAX_DEVSTAT_EVENTS_PER_DEV = 30};

/* Actual counts */
D int   numdevs  V(0);
D int   numchans V(0);
D int   numbigcs V(0);

/* Devs database */
D int               d_state     [CX_MAX_DEVS]; // State of this dev -- one of DEVSTATE_NNN (was initialized, and wasn't terminated/disconnected)?
D int               d_loadok    [CX_MAX_DEVS]; // 0 -- OK, <0 -- error-on-load
D cxsd_devname      d_instname  [CX_MAX_DEVS];
D CxsdDriverModRec *d_metric    [CX_MAX_DEVS]; // Driver's metric; !!!for cxsd_drvmgr.c only!!!
D int               d_businfocount[CX_MAX_DEVS];
D int32             d_businfo     [CX_MAX_DEVS][CXSD_DB_MAX_BUSINFOCOUNT]; //
D int               d_drvname_o [CX_MAX_DEVS]; // Offset of "drvname" in `drvinfobuf'
D int               d_auxinfo_o [CX_MAX_DEVS]; // Offset of "auxinfo" in `drvinfobuf'
D int32             d_ofs       [CX_MAX_DEVS]; // # of first channel
D int32             d_size      [CX_MAX_DEVS]; // Total # of regular channels
D int               d_big_first [CX_MAX_DEVS]; // # of first big channel
D int               d_big_count [CX_MAX_DEVS]; // # of big channels
D void             *d_devptr    [CX_MAX_DEVS]; // Last ptr registered for this dev
D int               d_logmask   [CX_MAX_DEVS]; // Mask for logging of various classes of events

D struct timeval    d_stattime  [CX_MAX_DEVS]; // Time of last state change
D rflags_t          d_statflags [CX_MAX_DEVS]; // State/flags of current state/error
D devs_cbitem_t    *d_cblist    [CX_MAX_DEVS];
D devs_cbitem_t    *d_cblist_end[CX_MAX_DEVS];

D CxsdDevRWProc     d_doio      [CX_MAX_DEVS];
D CxsdDevBigProc    d_dobig     [CX_MAX_DEVS];


typedef struct
{
    int                        is_used;
    int                        devid; //  Backreference
    inserver_chanref_t         ref;
    inserver_chanref_evproc_t  proc;
    void                      *privptr;
    chan_cbitem_t              cbitem;
} inserver_chanref_evprocinfo_t;

typedef struct
{
    int                        is_used;
    int                        devid; //  Backreference
    inserver_bigcref_t         ref;
    inserver_bigcref_evproc_t  proc;
    void                      *privptr;
    bigc_cbitem_t              cbitem;
} inserver_bigcref_evprocinfo_t;

typedef struct
{
    int                        is_used;
    int                        devid; //  Backreference
    inserver_devref_t          ref;
    inserver_devstat_evproc_t  proc;
    void                      *privptr;
    devs_cbitem_t              cbitem;
} inserver_devstat_evprocinfo_t;

typedef struct
{
    int                        is_used;
    bigc_listitem_t            bigclistitem;
} inserver_bigcrequest_holder_t;

D inserver_chanref_evprocinfo_t d_chanref_events[CX_MAX_DEVS][CX_MAX_DATAREF_EVENTS_PER_DEV];
D inserver_bigcref_evprocinfo_t d_bigcref_events[CX_MAX_DEVS][CX_MAX_BIGCREF_EVENTS_PER_DEV];
D inserver_bigcrequest_holder_t d_bigcrq_holders[CX_MAX_DEVS][CX_MAX_BIGCRQ_HOLDERS_PER_DEV];
D inserver_devstat_evprocinfo_t d_devstat_events[CX_MAX_DEVS][CX_MAX_DEVSTAT_EVENTS_PER_DEV];


/* Regular channels database */
// Backreference
D int32    c_devid  [CX_MAX_CHANS];  // Device this channel belongs to

// Main properties
D int32    c_value  [CX_MAX_CHANS];  // Current value
D int32    c_simnew [CX_MAX_CHANS];  // New value from setv*() for simulation
D int32    c_type   [CX_MAX_CHANS];  // 0 - read, 1 - write
D cycle_t  c_time   [CX_MAX_CHANS];  // Time of last update (base for "fresh" flag)
D int8     c_rd_req [CX_MAX_CHANS];  // Read request was sent but yet unserved
D int8     c_wr_req [CX_MAX_CHANS];  // Write request was sent but yet unserved
D rflags_t c_rflags [CX_MAX_CHANS];  // Result flags
D rflags_t c_crflags[CX_MAX_CHANS];  // Accumulated/cumulative result flags

D int32    c_next_wr_val     [CX_MAX_CHANS];  // Value for next write
D int8     c_next_wr_val_p   [CX_MAX_CHANS];  // Value for next write is pending
D cycle_t  c_next_wr_val_time[CX_MAX_CHANS];  // Time when the value for "next write" has arrived
D cycle_t  c_wr_time         [CX_MAX_CHANS];  // Cycle when last "write request" was passed to driver

D chan_cbitem_t *c_cblist    [CX_MAX_CHANS];
D chan_cbitem_t *c_cblist_end[CX_MAX_CHANS];


typedef struct {
    int              devid;    // Device this channel belongs to

    // Database info
    int              nargs;             // # of args
    size_t           datasize;          // Size of accepted data (in bytes)
    size_t           dataunits;         // Units of accepted data (1/2/4)
    int              ninfo;             // # of returned int32 info
    size_t           retdatasize;       // Size of returned data (in bytes)
    size_t           retdataunits;      // Units of returned data (1/2/4)

    // Current "readily available" values -- "dynamic database"
    int              cur_args_o;        // int32
    int              cur_nargs;
    //size_t *cur_data?
    size_t           cur_datasize;
    size_t           cur_dataunits;
    int              cur_info_o;        // int32
    int32            cur_ninfo;
    int              cur_retdata_o;     // void
    size_t           cur_retdatasize;
    size_t           cur_retdataunits;  //

    rflags_t         cur_rflags;
    rflags_t         crflags;

    cycle_t          cur_time;
    
    // "Double-buffer" -- what was sent to driver (will be copied to cur_* upon response)
    int              sent_args_o;
    int              sent_nargs;
    //size_t *sent_data?
    size_t           sent_datasize;
    size_t           sent_dataunits;
        
    int8             req_sent;          // Request was sent but yet unserved

    // Request chain
    bigc_listitem_t *list;              // Current chain of requesters...
    bigc_listitem_t *listlast;          // ...and its tail
    bigc_listitem_t *curitem;           // The item whose request is currently being served

    // Callback chain
    bigc_cbitem_t   *cblist;
    bigc_cbitem_t   *cblist_end;
} bigchaninfo_t;

D bigchaninfo_t b_data[CX_MAX_BIGCS];


D void   *bigbuf     V(NULL);
D size_t  bigbufsize V(0);
D size_t  bigbufused V(0);

#define GET_BIGBUF_PTR(offset,type) (offset<0?NULL:((type*)(((char *)bigbuf)+offset)))

static inline void CONSUME_BIGBUF(int *o_p, size_t size, size_t *curofs_p)
{
    if (size == 0)
        *o_p = -1;
    else
    {
        *o_p = *curofs_p;
        *curofs_p += size;
    }
}


D void   *drvinfobuf     V(NULL);
D size_t  drvinfobufsize V(0);
D size_t  drvinfobufused V(0);

#define GET_DRVINFOBUF_PTR(offset,type) (offset<0?NULL:((type*)(((char *)drvinfobuf)+offset)))

static inline void CONSUME_DRVINFOBUF(int *o_p, size_t size, size_t *curofs_p)
{
    if (size == 0)
        *o_p = -1;
    else
    {
        *o_p = *curofs_p;
        *curofs_p += size;
    }
}


/*==== Current cycle characteristics ===============================*/

D cycle_t      current_cycle  V(MAX_TAG_T); // Current cycle #; this value is chosen in order for never-measured-channels to be "infinitely old"

static inline tag_t age_of(cycle_t t)
{
    return (current_cycle - t > MAX_TAG_T)? MAX_TAG_T : current_cycle - t;
}


/*==== A little bit of macros for interaction with drivers =========*/

D int32 active_dev V(DEVID_NOT_IN_DRIVER);

#define ENTER_DRIVER_S(devid, s) do{s=active_dev; active_dev=devid;}while(0)
#define LEAVE_DRIVER_S(s)        do{active_dev=s;                  }while(0)


#undef D
#undef V


#endif /* __CXSD_DATA_H */
