#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cx_sysdeps.h"

#include "cxlib.h"
#include "misc_macros.h"
#include "misclib.h"
#include "findfilein.h"
#include "paramstr_parser.h"

#include "cxscheduler.h"

#include "cda.h"
#include "cda_err.h"
#define __CDA_C
#include "cda_internals.h"

#ifndef NAN
  #warning The NAN macro is undefined, using strtod("NAN") instead
  #define NAN strtod("NAN", NULL)
#endif


enum {OLDTAGCYCLES        = 5};   // # of cycles after last actual read for channel to be treated as "defunct"


/*
 A note on error handling in these functions:
 EBADF        passing invalid server id
 EINVAL       passing invalid channel id returns
 EROFS        writing into ro channel (future)
 EINPROGRESS  trying to add channel to a running server connection
 */

/*
 A note how "physmodified" flag works:
 Initally the flag is MODIFIED_NOT.
 When some modification is requested, the flag is set to MODIFIED_USER;
 the write request is sent for all !=MODIFIED_NOT,
 and upon sending the request the flag is promoted to MODIFIED_SENT;
 upon answer arrival all ==MODIFIED_SENT are dropped to MODIFIED_NOT
 (but MODIFIED_USER are untouched!)

 This algorythm allows failsafe behaviour -- the flag is dropped only
 when "request is got by server"-confirmation is received, so that
 if request is sent but not yet forwarded to the driver (e.g. connection
 is lost, or the server segfaults), the request will be repeated upon
 reconnect (and in case of another failure it will be repeated once again,
 and so on).

 Of course, this doesn't save us from a case when the request *was* received
 by server, left unserved till the end of cycle (due to non-empty driver
 queue), the server had sent us a "non-fresh" answer, and segfaulted.

 But "absolutely correct" processing of this case would be a bit more
 "intelligent".  For example, we could send write request only for
 ==MODIFIED_USER, drop from ==MODIFIED_SENT to MODIFIED_NOT only upon
 tag==0, and turn back all ==MODIFIED_SENT to MODIFIED_USER in FailureProc().
 
 */

typedef enum
{
    CB_S_NEWDATA,
    CB_S_FAILURE,
    CB_S_CONNECT,
    CB_T_RECONNECT,
    CB_T_HEARTBEAT,
    CB_T_DEFUNCTHB,
} cb_reason_t;

enum
{
    CHANS_ALLOC_INCREMENT = 100,
    BIGCS_ALLOC_INCREMENT = 1
};

typedef enum
{
    ST_FREE = 0,
    ST_ANY  = 1,
    ST_DATA = 2,
    ST_BIGC = 3
} srvtype_t;

enum
{
    MODIFIED_NOT  = 0,
    MODIFIED_USER = 1,
    MODIFIED_SENT = 2
};

enum {FROZEN_LIMIT = 5};

enum {STD_CYCLESIZE = 1000000};

enum {DEFAULT_RECONNECT_TIME_USECS =    5*1000000};
enum {DEFAULT_HEARTBEAT_USECS      = 5*60*1000000};
enum {DEFAULT_DEFUNCTHB_USECS      =   10*1000000};

typedef struct
{
    double    phys_r;
    double    phys_d;
    double    physval;
    rflags_t  rflags;
    rflags_t  physrflags;
    int32     physraw;
    int32     phys_q;         /* It is here instead of after phys_d to avoid padding */
    int32     usercode;
    tag_t     phystag;
    tag_t     mdctr;          // "Modified time counter"
    int8      physmodified;
    int8      isinitialized;
} chaninfo_t;

enum {FREE_CELL_BIGC_N = -1};

typedef struct
{
    int       rninfo;
    size_t    retbufused;
    size_t    retbufunits;
    rflags_t  rflags;
    tag_t     tag;
} bigcretrec_t;

typedef struct
{
    int       bigc_n;
    int       active;
    int       nargs;
    int       retbufsize;
    int       cachectl;
    int       immediate;

    int8     *buf;
    size_t    sndargs_ofs;
    size_t    rcvargs_ofs;
    size_t    modargs_ofs;
    size_t    prsargs_ofs;
    size_t    retrec_ofs;
    size_t    data_ofs;

    int       snddata_present;
    int8     *sndbuf;
    size_t    sndbufallocd;
    size_t    sndbufused;
    size_t    sndbufunits;
} bigcinfo_t;

typedef struct
{
    cda_eventp_t  proc;
    void         *privptr;
} evcbrec_t;

typedef struct
{
    srvtype_t       type;
    int             in_use;
    int             sid; // Backreference to itself -- for fast si2sid()

    /* Callbacks */
    evcbrec_t      *auxcblist;
    size_t          auxcblistsize;

    /*  */
    cda_serverid_t  main_sid;
    int             sid_n;
    
    /* Connection management */
    int             conn;
    char            srvrspec[200]; /*!!! Replace '200' with what? */
    int             is_running;
    int             is_connected;
    int             is_suffering;
    int             cyclesize;
    time_t          last_data_time;
    int             was_data_reply;

    sl_tid_t        reconnect_tid;
    sl_tid_t        heartbeat_tid;
    sl_tid_t        defuncthb_tid;

    /* Aux. sids, used by this one's formulae */
    int             auxsidscount;
    cda_serverid_t *auxsidslist;

    /* "Hack for v2" -- channels' and bigcs' base offsets */
    int             chan_base;
    int             bigc_base;

    int             shy_data;
    
    /* Physical channels stuff */
    physprops_t    *phprops;
    int             phpropscount;

    int             physallocd;
    int             physcount;
    chaninfo_t     *chaninfo;
    
    chanaddr_t     *physlist;
    int32          *physcodes;
    tag_t          *phystags;
    rflags_t       *physrflags;

    int             req_sent;
    int             double_buffer_used;
    struct timeval  req_timestamp;

    int             old_physcount;
    int32          *old_physcodes;
    tag_t          *old_phystags;
    rflags_t       *old_physrflags;

    /* Big channels stuff */
    int             bigcallocd;  // # of allocated and initialized cells
    int             bigccount;   // # of cells which are worth-requesting
    bigcinfo_t     *bigcinfo;
} serverinfo_t;

static psp_paramdescr_t srvrspec_params[] =
{
    PSP_P_INT("chan_base",        serverinfo_t, chan_base, 0, 0, 1<<30),
    PSP_P_INT("bigc_base",        serverinfo_t, bigc_base, 0, 0, 1<<30),
    PSP_P_FLAG("persistent_data", serverinfo_t, shy_data,  0, 1),
    PSP_P_FLAG("shy_data",        serverinfo_t, shy_data,  1, 0),
    PSP_P_END()
};


enum
{
    V2_MAX_CONNS = 1000, // An arbitrary limit
    V2_ALLOC_INC = 2
};

static serverinfo_t **servers_list        = NULL;
static int           servers_list_allocd = 0;

// GetV2sidSlot()
GENERIC_SLOTARRAY_DEFINE_GROWFIXELEM(static, V2sid, serverinfo_t,
                                     servers, in_use, 0, 1,
                                     0, V2_ALLOC_INC, V2_MAX_CONNS,
                                     , , void)

static void RlsV2sidSlot(int sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);

    /*!!! For now -- we do nothing */
}

static int           reconnect_time_us = DEFAULT_RECONNECT_TIME_USECS;

static physinfodb_rec_t   *global_physinfo_db;


/*
 *  memint
 *      Does the same as memchr(), but for ints.
 *      Shouldn't we move it to some .h?
 */

static const int *memint(const int *s, int v, int n)
{
    for (; n != 0; n--, s++)
        if (*s == v) return s;

    return NULL;
}


static char progname[40] = "";

void _cda_reporterror(const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
#if 1
    fprintf (stderr, "%s %s%scda",
             strcurtime(), progname, progname[0] != '\0' ? ": " : "");
    fprintf (stderr, ": ");
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
    va_end(ap);
}

static void cda_report(cda_serverid_t  sid, const char *format, ...)
    __attribute__ ((format (printf, 2, 3)));
static void cda_report(cda_serverid_t  sid, const char *format, ...)
{
  va_list     ap;
  const char *pn;
  const char *sr;
  char        sid_buf[20];
  serverinfo_t *si;

    va_start(ap, format);
#if 1
    pn = sr = NULL;
    pn = progname;
    if (sid >= 0)
    {
        //pn = cda_argv0_of(sid);
        si = AccessV2sidSlot(sid);
        if (si != NULL)
        {
            sr = si->srvrspec; //sr = cda_sspec_of(sid);
            sprintf(sid_buf, "%d", sid);
        }
    }
    if (pn == NULL) pn = "";
    if (sr == NULL) sr = "";

    fprintf (stderr, "%s ", strcurtime());
    if (*pn != '\0')
        fprintf(stderr, "%s: ", pn);
    fprintf (stderr, "cda");
    if (sid >= 0)
    {
        fprintf (stderr, "[%d", sid);
        if (*sr != '\0')
            fprintf (stderr, ":\"%s\"", sr);
        fprintf (stderr, "]");
    }
    fprintf (stderr, ": ");
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
    va_end(ap);
}


/*********************************************************************
*                                                                    *
*  Library-internal data conversion/representation routines          *
*                                                                    *
*********************************************************************/

/*
 *  si2sid
 *      Converts a "server pointer", used internally by the library,
 *      into "server id", accepted by public cda_XXX() calls
 *      and suitable for error/debug messages.
 */

static inline int si2sid(serverinfo_t *si)
{
    return si->sid;
}


/*
 *
 *  Handle internal format:
 *      high-order 1 byte -- server id
 *      lower 3 bytes     -- channel id inside that server
 */

static inline cda_objhandle_t encode_chanhandle(cda_serverid_t sid, int objofs)
{
    return (sid << 24)  + objofs;
}

static inline void decode_objhandle(cda_objhandle_t handle,
                                    cda_serverid_t *sid_p, int *objofs_p)
{
    *sid_p    = (handle >> 24) & 0xFF;
    *objofs_p = handle & 0x00FFFFFF;
}


/*
 *  _CdaCheckSid
 *      Check if the server id is valid and represents an active record
 */

int _CdaCheckSid(cda_serverid_t sid)
{
    if (sid >= 0                                   &&
        sid < (cda_serverid_t)servers_list_allocd  &&
        AccessV2sidSlot(sid)->in_use != 0)
        return 0;

    errno = EBADF;
    return -1;
}


/*********************************************************************
*                                                                    *
*  Library-internal "serverinfo" manipulation stuff                  *
*                                                                    *
*********************************************************************/

static int  ActivateDoubleBuffer(serverinfo_t *si, int arr_inc)
{
  int32    *new_physcodes;
  tag_t    *new_phystags;
  rflags_t *new_physrflags;
  
    if (si->double_buffer_used) return 0;

    if (si->physcount != 0) /* In case of 0 channels we shouldn't copy anything */
    {
        /* A general note: we copy "physcount" values,
         but malloc "physallocd" values.  Usually it doesn't matter
         (since Activate...() is called only when "physcount" reaches
         "physallocd"), but we differentiate these just for correctness */
        
        /* Remember old values */
        si->old_physcount  = si->physcount;
        si->old_physcodes  = si->physcodes;
        si->old_phystags   = si->phystags;
        si->old_physrflags = si->physrflags;

        /* Allocate new buffers */
        new_physcodes  = malloc(sizeof(*new_physcodes)  * (si->physallocd + arr_inc));
        new_phystags   = malloc(sizeof(*new_phystags)   * (si->physallocd + arr_inc));
        new_physrflags = malloc(sizeof(*new_physrflags) * (si->physallocd + arr_inc));

        if (new_physcodes == NULL  ||  new_phystags == NULL  ||  new_physrflags == NULL)
        {
            safe_free(new_physcodes);
            safe_free(new_phystags);
            safe_free(new_physrflags);
            return -1;
        }

        si->physcodes  = new_physcodes;
        si->phystags   = new_phystags;
        si->physrflags = new_physrflags;

        /* Duplicate old codes+tags in new buffers */
        /* !!!BTW, do we really need them? Yes, we definitely need phystags and physrflags, at least */
        memcpy(si->physcodes,  si->old_physcodes,  sizeof(*(si->physcodes))  * si->old_physcount);
        memcpy(si->phystags,   si->old_phystags,   sizeof(*(si->phystags))   * si->old_physcount);
        memcpy(si->physrflags, si->old_physrflags, sizeof(*(si->physrflags)) * si->old_physcount);
        
    }

    si->double_buffer_used = 1;

    return 0;
}

static void FoldDoubleBuffer(serverinfo_t *si, int newdata_present)
{
    if (!si->double_buffer_used) return;
    
    if (newdata_present  &&  si->old_physcount != 0)
    {
        memcpy(si->physcodes,  si->old_physcodes,  sizeof(*(si->physcodes))  * si->old_physcount);
        memcpy(si->phystags,   si->old_phystags,   sizeof(*(si->phystags))   * si->old_physcount);
        memcpy(si->physrflags, si->old_physrflags, sizeof(*(si->physrflags)) * si->old_physcount);
    }

    safe_free(si->old_physcodes);  si->old_physcodes  = NULL;
    safe_free(si->old_phystags);   si->old_phystags   = NULL;
    safe_free(si->old_physrflags); si->old_physrflags = NULL;

    si->double_buffer_used = 0;
}


/*********************************************************************
*                                                                    *
*  Data pulling and conversion stuff                                 *
*                                                                    *
*********************************************************************/

static void DecodeData(serverinfo_t *si)
{
  int           i;
  chaninfo_t   *ci;
  bigcinfo_t   *bi;
  bigcretrec_t *rr;
  int32         delta;
  int           scaled_age;

    if      (si->type == ST_DATA)
    {
        for (i = 0, ci = si->chaninfo;  i < si->physcount;  i++, ci++)
        {
            ci->physval    = si->physcodes [i] / ci->phys_r - ci->phys_d;
            ci->physraw    = si->physcodes [i];
            ci->phystag    = si->phystags  [i];
            ci->physrflags = si->physrflags[i];

            /* Okay, the "orange" intellect */
            if (ci->physmodified != MODIFIED_USER)
            {
                if (si->phystags[i] < ci->mdctr) /* This check is to prevent comparison with values obtained before we made a first/write request */
                {
                    if (ci->isinitialized)
                    {
                        delta = abs(ci->usercode - ci->physraw);
                        if (delta != 0  &&  delta >= ci->phys_q)
                        {
                            ci->rflags   = (ci->rflags | CXCF_FLAG_OTHEROP)
                                                      &~ CXCF_FLAG_PRGLYCHG;
                            ci->usercode = ci->physraw;
                            // /*For ipp@oleg's debugging*/if (si->physlist[i] >= 840) fprintf(stderr, "OTHEROP, tag=%d, mdctr=%d\n", si->phystags[i], ci->mdctr);
                        }
                        else
                            ci->rflags &=~ CXCF_FLAG_OTHEROP;
                    }
                    else
                    {
                        ci->usercode = ci->physraw;
                        ci->isinitialized = 1;
                    }
                }
            }

            if (ci->phystag < MAX_TAG_T)
            {
                scaled_age = scale32via64(ci->phystag, si->cyclesize, STD_CYCLESIZE);
                if (scaled_age < 0)         scaled_age = 0;
                if (scaled_age > MAX_TAG_T) scaled_age = MAX_TAG_T;
                ci->phystag = scaled_age;
            }

            /* Goosing/DEFUNCT */
            if (ci->phystag> OLDTAGCYCLES)
                ci->rflags  |= CXCF_FLAG_DEFUNCT;
            else
                ci->rflags &=~ CXCF_FLAG_DEFUNCT;

            if (ci->mdctr < MAX_TAG_T) ci->mdctr++;
            
            if (ci->physmodified == MODIFIED_SENT)
                ci->physmodified =  MODIFIED_NOT;
        }
    }
    else if (si->type == ST_BIGC)
    {
        for (i = 0, bi = si->bigcinfo;  i < si->bigccount;  i++, bi++)
            if (bi->bigc_n != FREE_CELL_BIGC_N  &&  bi->active)
            {
                rr = (bigcretrec_t *)(bi->buf + bi->retrec_ofs);
                if (rr->tag < MAX_TAG_T)
                {
                    scaled_age = scale32via64(rr->tag, si->cyclesize, STD_CYCLESIZE);
                    if (scaled_age < 0)         scaled_age = 0;
                    if (scaled_age > MAX_TAG_T) scaled_age = MAX_TAG_T;
                    rr->tag = scaled_age;
                }
            }
    }
}

static void RequestData(serverinfo_t *si)
{
  int           r;
  int           i;
  chaninfo_t   *ci;
  bigcinfo_t   *bi;
  bigcretrec_t *rr;
  int           a;
  int32        *ra_p;
  int32        *sa_p;
  int32        *ma_p;
  int32        *pa_p;
  int           curargs;

  int8         *sndbuf;
  size_t        sndbufsize;
  size_t        sndbufunits;

    if (!si->is_running  ||  !si->is_connected) return;

    if (si->req_sent) return;
    
    cx_begin(si->conn);

    if      (si->type == ST_DATA)
    {
        if (si->physcount != 0)
            cx_getvset(si->conn, si->physlist,  si->physcount,
                       si->physcodes, si->phystags, si->physrflags);

        for (i = 0, ci = si->chaninfo;  i < si->physcount;  i++, ci++)
            if (ci->physmodified != MODIFIED_NOT)
            {
                ////fprintf(stderr, "%s: setvalue(id=%d=>%d)\n", __FUNCTION__, i, si->physlist[i]);
                cx_setvalue(si->conn, si->physlist[i], ci->usercode,
                            NULL, NULL, NULL);
                ci->physmodified = MODIFIED_SENT;
            }
    }
    else if (si->type == ST_BIGC)
    {
        for (i = 0, bi = si->bigcinfo;  i < si->bigccount;  i++, bi++)
            if (bi->bigc_n != FREE_CELL_BIGC_N  &&  bi->active)
            {
                for (a = 0,
                     ra_p = (int32 *)(bi->buf + bi->rcvargs_ofs),
                     sa_p = (int32 *)(bi->buf + bi->sndargs_ofs),
                     ma_p = (int32 *)(bi->buf + bi->modargs_ofs),
                     pa_p = (int32 *)(bi->buf + bi->prsargs_ofs),
                     curargs = 0;
                     a < bi->nargs;
                     a++, ra_p++, sa_p++, ma_p++, pa_p++)
                {
                    if (*ma_p  ||  *pa_p)
                    {
                        *ma_p = 0;
                        curargs = a + 1;
                    }
                    else
                        *sa_p = *ra_p;
                }

                if (bi->snddata_present)
                {
                    sndbuf      = bi->sndbuf;
                    sndbufsize  = bi->sndbufused;
                    sndbufunits = bi->sndbufunits;

                    bi->snddata_present = 0; // Drop the flag
                    bi->sndbufused      = 0; // and "empty" the buffer
                }
                else
                {
                    sndbuf      = NULL;
                    sndbufsize  = 0;
                    sndbufunits = 0;
                }

                rr = (bigcretrec_t *)(bi->buf + bi->retrec_ofs);
                r = cx_bigcreq(si->conn, bi->bigc_n,
                               (int32 *)(bi->buf + bi->sndargs_ofs), curargs,
                               sndbuf, sndbufsize, sndbufunits,
                               (int32 *)(bi->buf + bi->rcvargs_ofs), bi->nargs, &(rr->rninfo),
                               bi->buf + bi->data_ofs,    bi->retbufsize, &(rr->retbufused), &(rr->retbufunits),
                               &(rr->tag), &(rr->rflags),
                               bi->cachectl, bi->immediate);
                if (r != 0) cda_report(si2sid(si), "%s(): cx_bigcreq=%d: %s",
                                       __FUNCTION__, r, cx_strerror(errno));
            }
    }
    
    gettimeofday(&(si->req_timestamp), NULL);
    si->req_sent = 1;
    r = cx_run(si->conn);
    if (r < 0  &&  errno != CERUNNING)
        cda_report(si2sid(si), "%s(): cx_run()=%d: %s",
                   __FUNCTION__, r, cx_strerror(errno));
}

static void NotifyClients(serverinfo_t *si, int reason)
{
  evcbrec_t    *p;
  unsigned int  n;

    for (p = si->auxcblist, n = 0;
         n < si->auxcblistsize / sizeof(*p);
         p++, n++)
        if (p->proc != NULL)   
            p->proc(si2sid(si), reason, p->privptr);
}

static void InternalAuxsrvNotifier(cda_serverid_t sid,
                                   int reason,
                                   void *privptr)
{
  serverinfo_t   *main_si = AccessV2sidSlot((cda_serverid_t)ptr2lint(privptr));
  serverinfo_t   *this_si = AccessV2sidSlot(sid);

    if (reason == CDA_R_MAINDATA) NotifyClients(main_si, this_si->sid_n);
}


/*********************************************************************
*                                                                    *
*  Asynchronous stuff, including synchronous-time "completers"       *
*                                                                    *
*********************************************************************/

static void NewDataProc(cda_serverid_t sid);
static void FailureProc(cda_serverid_t sid);
static void ConnectProc(cda_serverid_t sid);
static void ReconnectTCB(int uniq, void *unsdptr, sl_tid_t tid, void *privptr);
static void HeartbeatTCB(int uniq, void *unsdptr, sl_tid_t tid, void *privptr);
static void DefunctHbTCB(int uniq, void *unsdptr, sl_tid_t tid, void *privptr);

static void TakeCareOfSuffering(const char *function,
                                serverinfo_t *si, int reason)
{
    if (!si->is_suffering)
        cda_report(si2sid(si), "%s(\"%s\"): %s: %s; will reconnect",
                   function, si->srvrspec,
                   cx_strreason(reason), cx_strerror(errno));
    si->is_suffering = 1;
}

static void MarkAllAsDefunct(serverinfo_t *si)
{
  int           i;
  chaninfo_t   *ci;

    if (!(si->was_data_reply)  ||  si->type != ST_DATA) return;
////fprintf(stderr, "%s()\n", __FUNCTION__);

    for (i = 0, ci = si->chaninfo;  i < si->physcount;  i++, ci++)
    {
        ci->phystag  = MAX_TAG_T;
        ci->rflags  |= CXCF_FLAG_DEFUNCT;
    }
    si->was_data_reply = 0;
    NotifyClients(si, CDA_R_MAINDATA);
}

static void NotificationProc(int   cd         __attribute__((unused)),
                             int   reason,
                             void *privptr,
                             const void *info __attribute__((unused)))
{
  cda_serverid_t  id = (cda_serverid_t)ptr2lint(privptr);
  serverinfo_t   *si = AccessV2sidSlot(id);
  
    switch (reason)
    {
        case CAR_ANSWER:
            NewDataProc(id);
            break;
            
        case CAR_KILLED: case CAR_ERRCLOSE: case CAR_CONNFAIL:
            TakeCareOfSuffering(__FUNCTION__, si, reason);
            FailureProc(id);
            break;

        case CAR_CONNECT:
            ConnectProc(id);
            break;
            
        default:
            cda_report(id, "%s: unknown async reason %d (%s)",
                       __FUNCTION__, reason, cx_strreason(reason));
    }
}


static int  DoConnect(serverinfo_t *si)
{
    return (si->type == ST_DATA ? cx_connect_n : cx_openbigc_n)
        (si->srvrspec, "???", NotificationProc, lint2ptr(si2sid(si)));
}


static void ScheduleReconnect(serverinfo_t *si)
{
  int  ec = errno;
  int  us;
  enum {MAX_US_X10 = 2*1000*1000*1000};
    
    if (si->reconnect_tid >= 0) return;

    us = reconnect_time_us;
    if (ec == CENOHOST)
    {
        if (us > MAX_US_X10 / 10)
            us = MAX_US_X10;
        else
            us *= 10;
    }

    /* Forget old connection */
    cx_close(si->conn);
    si->conn = -1;
    si->is_connected = 0;
    si->req_sent = 0;
    if (si->double_buffer_used) FoldDoubleBuffer(si, 0);
    
    /* And organize a timeout in a second... */
    si->reconnect_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                          us, ReconnectTCB, lint2ptr(si2sid(si)));
}

static void ReconnectTCB(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  serverinfo_t *si = AccessV2sidSlot(ptr2lint(privptr));

    si->reconnect_tid = -1;
    /* Have we been called after an actual nessecity had disappeared? */
    if (si->conn >= 0) return;
    
    /* Make a new connection */
    si->conn = DoConnect(si);

    if (si->conn < 0)
    {
        TakeCareOfSuffering(__FUNCTION__, si, CAR_CONNFAIL);
        ScheduleReconnect(si);
    }
}

static void NewDataProc(cda_serverid_t sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);

    si->last_data_time = time(NULL);
    si->was_data_reply = 1;
    si->req_sent = 0;
    if (si->double_buffer_used) FoldDoubleBuffer(si, 1);

    if (!si->is_running) return;
    
    DecodeData(si);
    NotifyClients(si, CDA_R_MAINDATA);
    RequestData(si);
}

static void FailureProc(cda_serverid_t sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);

    {
      int         x;
      chaninfo_t *ci;
      
        for (x = 0, ci = si->chaninfo;  x < si->physcount;  x++, ci++)
            ci->mdctr = 0;
    }
    MarkAllAsDefunct(si);
  
    ScheduleReconnect(si);
}

static void ConnectProc(cda_serverid_t sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  int           was_suffering;
  int           i;

    was_suffering = si->is_suffering;
    si->is_connected = 1;
    si->is_suffering = 0;
    si->cyclesize    = cx_getcyclesize(si->conn);
    if (si->cyclesize <= 0)
        si->cyclesize = STD_CYCLESIZE;

    if (was_suffering)
        cda_report(sid, "connected to \"%s\"", si->srvrspec);

    if (si->shy_data)
        for (i = 0;  i < si->physcount;  i++)
            si->chaninfo[i].physmodified = MODIFIED_NOT;

    RequestData(si);
}

static void HeartbeatTCB(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  serverinfo_t *si = AccessV2sidSlot(ptr2lint(privptr));

    if (si->conn >= 0  &&  si->is_running  &&  si->is_connected)
        cx_ping(si->conn);
    si->heartbeat_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                          DEFAULT_HEARTBEAT_USECS, HeartbeatTCB, lint2ptr(si2sid(si)));
}

static void DefunctHbTCB(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  serverinfo_t *si      = AccessV2sidSlot(ptr2lint(privptr));
  time_t        timenow = time(NULL);
  int           delta_t;

    if (si->was_data_reply  &&  si->type == ST_DATA  &&
        (delta_t = difftime(timenow, si->last_data_time)) >= FROZEN_LIMIT)
    {
        MarkAllAsDefunct(si);
    }

    si->defuncthb_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                          DEFAULT_DEFUNCTHB_USECS, DefunctHbTCB, lint2ptr(si2sid(si)));
}


/*********************************************************************
*                                                                    *
*  Temporary interface for global database service                   *
*                                                                    *
*********************************************************************/

int             cda_TMP_register_physinfo_dbase(physinfodb_rec_t *db)
{
    global_physinfo_db = db;

    return 0;
}


/*********************************************************************
*                                                                    *
*  Client interface for server access                                *
*                                                                    *
*********************************************************************/

static const char *find_srvrspec_tr(const char *spec)
{
  char  envname[300];
  char *p;

    if (spec == NULL) return NULL;

    check_snprintf(envname, sizeof(envname), "CX_TRANSLATE_%s", spec);
    
    for (p = envname; *p != '\0';  p++)
    {
        if (isalnum(*p)) *p = toupper(*p);
        else             *p = '_';
    }

    return getenv(envname);
}

cda_serverid_t  cda_new_server(const char *spec,
                               cda_eventp_t event_processer, void *privptr,
                               cda_conntype_t conntype)
{
  cda_serverid_t        id;
  serverinfo_t         *si;
  const char           *tr_spec;
  char                 *comma_p;
  
  int                   conn;

  int                   saved_errno;
  
  physinfodb_rec_t     *rec;

#if OPTION_HAS_PROGRAM_INVOCATION_NAME /* With GNU libc+ld we can determine the true argv[0] */
    if (progname[0] == '\0') strzcpy(progname, program_invocation_short_name, sizeof(progname));
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */

    {
      char *envv;
        
        _cda_debug_lapprox  = (envv = getenv("CDA_DEBUG_LAPPROX"))  != NULL  &&  *envv == '1';
        _cda_debug_setreg   = (envv = getenv("CDA_DEBUG_SETREG"))   != NULL  &&  *envv == '1';
        _cda_debug_setp     = (envv = getenv("CDA_DEBUG_SETP"))     != NULL  &&  *envv == '1';
        _cda_debug_setchan  = (envv = getenv("CDA_DEBUG_SETCHAN"))  != NULL  &&  *envv == '1';
        _cda_debug_physinfo = (envv = getenv("CDA_DEBUG_PHYSINFO")) != NULL  &&  *envv == '1';
    }

    cda_clear_err();

    /* 0. Do translation, if required */
    if (spec == NULL) spec = "";
    if ((tr_spec = find_srvrspec_tr(spec)) == NULL)
        tr_spec = spec;

    /* Allocate a new connection slot */
    if ((int)(id = GetV2sidSlot()) < 0) return CDA_SERVERID_ERROR;
    si = AccessV2sidSlot(id);
    
    /* Initialize and fill in the info */
    bzero(si, sizeof(*si));

    strzcpy(si->srvrspec, tr_spec, sizeof(si->srvrspec));
    /* Any options? */
    if ((comma_p = strchr(si->srvrspec, ',')) != NULL)
    {
        *comma_p++ = '\0';
        if (psp_parse(comma_p, NULL,
                      si,
                      ':', ", \t", "",
                      srvrspec_params) != PSP_R_OK)
        {
            cda_set_err("parsing servspec parameters: %s", psp_error());
            errno = EINVAL;
            goto CLEANUP_ON_ERROR;
        }
    }

    si->type            = conntype == CDA_REGULAR? ST_DATA:ST_BIGC;
    si->in_use          = 1;
    si->sid             = id;
    cda_add_evproc(id, event_processer, privptr);
    si->is_connected    = 0;
    si->cyclesize       = STD_CYCLESIZE;
    
    si->reconnect_tid = -1;
    si->heartbeat_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                          DEFAULT_HEARTBEAT_USECS, HeartbeatTCB, lint2ptr(id));
    si->defuncthb_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                          DEFAULT_DEFUNCTHB_USECS, DefunctHbTCB, lint2ptr(id));
    
    /* Try to make a connection */
    conn = si->conn = DoConnect(si);

    /* Was this attempt at least semi-successful? */
    if (conn < 0)
    {
        /* No.  Okay, may we continue connect attempts... */
        if (IS_A_TEMPORARY_CONNECT_ERROR()  ||
            errno == CENOHOST    ||
            errno == CETIMEOUT   ||  errno == CEMANYCLIENTS  ||
            errno == CESRVNOMEM  ||  errno == CEMCONN)
        {
            TakeCareOfSuffering(__FUNCTION__, si, CAR_CONNFAIL);
            FailureProc(id);
        }
        /* ...or should we drop the connection right now :-( */
        else
            goto CLEANUP_ON_ERROR;
    }

    /* Try to obtain physinfo from database */
    for (rec = global_physinfo_db;
         rec != NULL  &&  rec->srv != NULL;
         rec++)
        if (strcasecmp(tr_spec/*!!!si->srvrspec*/, rec->srv) == 0)
        {
            cda_set_physinfo(id, rec->info, rec->count);
        }
    
    return id;

 CLEANUP_ON_ERROR:
    saved_errno = errno;
    sl_deq_tout(si->reconnect_tid); si->reconnect_tid = -1;
    sl_deq_tout(si->heartbeat_tid); si->heartbeat_tid = -1;
    sl_deq_tout(si->defuncthb_tid); si->defuncthb_tid = -1;
    errno = saved_errno;
    si->in_use = 0;
    return CDA_SERVERID_ERROR;
}

cda_serverid_t  cda_add_auxsid(cda_serverid_t  sid, const char *auxspec,
                               cda_eventp_t event_processer, void *privptr)
{
  serverinfo_t   *si = AccessV2sidSlot(sid);
  const char     *tr_auxspec;
  int             x;
  cda_serverid_t  auxsid;
  cda_serverid_t *tmpsidslist;

    cda_clear_err();
  
    if (_CdaCheckSid(sid) != 0) return CDA_SERVERID_ERROR;

    if (event_processer == NULL)
    {
        event_processer = InternalAuxsrvNotifier;
        privptr         = lint2ptr(sid);
    }

    /* Step 0. Check if it requires a translation */
    if ((tr_auxspec = find_srvrspec_tr(auxspec)) == NULL)
        tr_auxspec = auxspec;
    
    /* Step 1. The same as base? */
    if (strcasecmp(si->srvrspec, tr_auxspec) == 0)
        return sid;

    /* Step 2. If not, try to find among already present auxsids */
    for (x = 0;  x < si->auxsidscount;  x++)
        if (_CdaCheckSid(si->auxsidslist[x]) == 0  &&
            strcasecmp(AccessV2sidSlot(si->auxsidslist[x])->srvrspec, tr_auxspec) == 0)
            return si->auxsidslist[x];

    /* Step 3. None found?  Okay, make a new connection... */
    auxsid = cda_new_server(auxspec, event_processer, privptr, CDA_REGULAR);
    if (auxsid == CDA_SERVERID_ERROR) return auxsid;

    si = AccessV2sidSlot(sid); /*!!! Re-cache value, since it could have changed because of realloc() in cda_new_server() */

    /* ...and remember the server in "aux servers list"... */
    tmpsidslist = safe_realloc(si->auxsidslist,
                               sizeof (cda_serverid_t) * (si->auxsidscount + 1));
    if (tmpsidslist == NULL)
    {
        cda_report(sid, "%s: safe_realloc(auxsidslist): %s",
                   __FUNCTION__, strerror(errno));
    }
    else
    {
        si->auxsidslist = tmpsidslist;
        tmpsidslist[si->auxsidscount] = auxsid;
        si->auxsidscount++;
    }

    /* ...plus record its position in that list locally... */
    /* Note:
           we save "si->auxsidscount", without a "-1", for the value to
           be instantly usable as a cause_conn_n (where 0 is the main server,
           and aux ones count from 1).
    */
    AccessV2sidSlot(auxsid)->sid_n    = si->auxsidscount;
    /* ...and "link" to parent server too */
    AccessV2sidSlot(auxsid)->main_sid = sid;
    
    return auxsid;
}

int             cda_run_server(cda_serverid_t  sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  int           x;
  
    if (_CdaCheckSid(sid) != 0) return -1;

    if (si->is_running) return 0;

    si->is_running = 1;

    RequestData(si);

    /* Run all auxilliary servers */
    for (x = 0;  x < si->auxsidscount;  x++)
        if (_CdaCheckSid(si->auxsidslist[x]) == 0)  /*!!! <-- why is this check? Now, with LOCAL auxsids, is looks redundant... */
            cda_run_server(si->auxsidslist[x]);
    
    return 0;
}

int             cda_hlt_server(cda_serverid_t  sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  int           x;
  
    if (_CdaCheckSid(sid) != 0) return -1;

    if (!si->is_running) return 0;

    si->is_running = 0;

    /* Halt all auxilliary servers */
    for (x = 0;  x < si->auxsidscount;  x++)
        cda_hlt_server(si->auxsidslist[x]);

    return 0;
}

int             cda_del_server(cda_serverid_t  sid)
{
    if (_CdaCheckSid(sid) != 0) return -1;

    /*! Here should cx_close(conn), remove signals and timeout, mark in_use:=0, free phprops */

    /*! And call cda_del_evproc() for all aux.servers */
    
    return 0;
}

int             cda_continue  (cda_serverid_t  sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  
    if (_CdaCheckSid(sid) != 0) return -1;

    RequestData(si);

    return 0;
}

int             cda_add_evproc(cda_serverid_t  sid, cda_eventp_t event_processer, void *privptr)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  evcbrec_t    *p;
  unsigned int  n;
  
    if (_CdaCheckSid(sid) != 0) return -1;
    
    if (event_processer == NULL) return 0;

    /* Check if it is already in the list */
    for (p = si->auxcblist, n = 0;
         n < si->auxcblistsize / sizeof(*p);
         p++, n++)
        if (p->proc == event_processer  &&  p->privptr == privptr)
            return 0;
            
    /* Try to find a free slot */
    for (p = si->auxcblist, n = 0;
         n < si->auxcblistsize / sizeof(*p);
         p++, n++)
        if (p->proc == NULL)
            goto FILL_IN_THE_REC;

    /* None?  Okay, let's grow the array */
    if (GrowBuf((void *)&(si->auxcblist),
                &(si->auxcblistsize),
                si->auxcblistsize + sizeof(*p)) < 0)
        return -1;

    p = si->auxcblist + (si->auxcblistsize / sizeof(*p)) - 1;

 FILL_IN_THE_REC:
    p->proc = event_processer;
    p->privptr = privptr;
    
    return 0;
}

int             cda_del_evproc(cda_serverid_t  sid, cda_eventp_t event_processer, void *privptr)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  evcbrec_t    *p;
  unsigned int  n;
  
    if (_CdaCheckSid(sid) != 0) return -1;

    /* Find requested callback */
    for (p = si->auxcblist, n = 0;
         n < si->auxcblistsize / sizeof(*p);
         p++, n++)
        if (p->proc == event_processer  &&  p->privptr == privptr)
        {
            p->proc = NULL;
            return 0;
        }

    /* Not found! */
    errno = ENOENT;

    return -1;
}

int             cda_get_reqtimestamp(cda_serverid_t  sid, struct timeval *timestamp)
{
  serverinfo_t *si = AccessV2sidSlot(sid);

    if (_CdaCheckSid(sid) != 0) return -1;
    
    if (timestamp != NULL) *timestamp = si->req_timestamp;

    return 0;
}

int             cda_set_physinfo(cda_serverid_t sid, physprops_t *info, int count)
{
  serverinfo_t *si = AccessV2sidSlot(sid);

  int           i;
  int           j;
  int           was;
  
    if (_CdaCheckSid(sid) != 0) return -1;

    if (si->phprops != NULL) free(si->phprops);

    si->phprops = NULL;
    si->phpropscount = 0;

    if (count > 0)
    {
        if ((si->phprops = malloc(count * sizeof(*info))) == NULL) return -1;
        si->phpropscount = count;

        memcpy(si->phprops, info, count * sizeof(*info));
        
        if (_cda_debug_physinfo)
            for (i = 0;  i < count;  i++)
                for (j = i + 1, was = 0;  j < count;  j++)
                    if (si->phprops[j].n == si->phprops[i].n)
                    {
                        if (!was)
                            fprintf(stderr,
                                    "Multiple physinfo(\"%s\") for channel %d:\n"
                                    "\t[%d]={%d, %8.3f, %8.3f, %d}\n",
                                    si->srvrspec, si->phprops[j].n,
                                    i, si->phprops[i].n, si->phprops[i].r, si->phprops[i].d, si->phprops[i].q);
                        was = 1;

                        fprintf(stderr,
                                "\t[%d]={%d, %8.3f, %8.3f, %d}\n",
                                j, si->phprops[j].n, si->phprops[j].r, si->phprops[j].d, si->phprops[j].q);
                    }
    }

    return 0;
}


int             cda_cyclesize     (cda_serverid_t  sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  
    if (_CdaCheckSid(sid) != 0) return -1;

    return si->cyclesize;
}


int             cda_status_lastn  (cda_serverid_t  sid)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  
    if (_CdaCheckSid(sid) != 0) return -1;

    return si->auxsidscount;
}

int             cda_status_of     (cda_serverid_t  sid, int n)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  time_t        timenow;
  
    if (_CdaCheckSid(sid) != 0) return -1;
    
    if (n < 0  ||  n > si->auxsidscount)
    {
        errno = ENOENT;
        return -1;
    }
    else if (n != 0)
        return cda_status_of(si->auxsidslist[n - 1], 0);
    else
    {
        if (!si->is_connected  /*||  !si->is_running*/) return CDA_SERVERSTATUS_DISCONNECTED;
        else
        {
            timenow = time(NULL);
            if (difftime(timenow, si->last_data_time) < FROZEN_LIMIT)
                return CDA_SERVERSTATUS_NORMAL;
            else
                return CDA_SERVERSTATUS_FROZEN;
        }
    }
}

const char     *cda_status_srvname(cda_serverid_t  sid, int n)
{
  serverinfo_t *si = AccessV2sidSlot(sid);
  
    if (_CdaCheckSid(sid) != 0) return NULL;
    
    if (n < 0  ||  n > si->auxsidscount)
    {
        errno = ENOENT;
        return NULL;
    }
    else if (n != 0)
        return cda_status_srvname(si->auxsidslist[n - 1], 0);
    else
        return si->srvrspec;
}


int             cda_srcof_physchan(cda_physchanhandle_t  chanh,   const char **name_p, int *n_p)
{
  cda_serverid_t  sid;
  int             chanofs;

    /* Decode and check address */
    decode_objhandle(chanh, &sid, &chanofs);
    if (_CdaCheckSid(sid) != 0) return -1;

    if (name_p != NULL) *name_p = cda_status_srvname(sid, 0);
    if (n_p    != NULL) *n_p    = AccessV2sidSlot(sid)->physlist[chanofs];
    
    return 1;
}

int             cda_srcof_formula (excmd_t              *formula, const char **name_p, int *n_p)
{
  excmd_t              *cp;
  cda_physchanhandle_t  handle = -1; /*To make gcc happy*/
  int                   count;

    if (formula == NULL) return -1;

    for (cp = formula, count = 0;
         (cp->cmd & OP_code) != OP_RET  &&  (cp->cmd & OP_code) != OP_RETSEP;
         cp++)
        if (cp->cmd == OP_GETP_I)
        {
            handle = cp->arg.handle;
            count++;
        }

    if (count == 1)
        return cda_srcof_physchan(handle, name_p, n_p);
    else
    {
        if (name_p != NULL) *name_p = NULL;
        if (n_p    != NULL) *n_p    = -1;
        
        return 0;
    }
}

cda_serverid_t  cda_sidof_physchan(cda_physchanhandle_t  chanh)
{
  cda_serverid_t  sid;
  int             chanofs;

    /* Decode and check address */
    decode_objhandle(chanh, &sid, &chanofs);
    if (_CdaCheckSid(sid) != 0) return CDA_SERVERID_ERROR;

    return sid;
}

int             cda_cnsof_physchan(cda_serverid_t  sid,
                                   uint8 *conns_u, int conns_u_size,
                                   cda_physchanhandle_t  chanh)
{
  serverinfo_t   *si = AccessV2sidSlot(sid);
  cda_serverid_t  chansid;
  int             chanofs;
  serverinfo_t   *chansi;
  
    if (_CdaCheckSid(sid) != 0) return -1;
    
    decode_objhandle(chanh, &chansid, &chanofs);
    if (_CdaCheckSid(sid) != 0) return -1;

    /* Check if the table has required size */
    if (conns_u_size < si->auxsidscount + 1)
    {
        errno = 0;
        return -1;
    }

    /* Is it just the main sid? */
    if (chansid == sid)
    {
        conns_u[0] = 1;
        return 0;
    }

    /* Okay -- does it belong to main sid? */
    chansi = AccessV2sidSlot(chansid);
    if (chansi->main_sid != sid)
    {
        errno = EXDEV;
        return -1;
    }

    /* Okay... */
    conns_u[chansi->sid_n] = 1;
    return 0;
}

int             cda_cnsof_formula (cda_serverid_t  sid,
                                   uint8 *conns_u, int conns_u_size,
                                   excmd_t              *formula)
{
  excmd_t              *cp;

    if (formula == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    for (cp = formula;
         (cp->cmd & OP_code) != OP_RET  &&  (cp->cmd & OP_code) != OP_RETSEP;
         cp++)
        if (cp->cmd == OP_GETP_I)
        {
            if (cda_cnsof_physchan(sid,
                                   conns_u, conns_u_size,
                                   cp->arg.handle) < 0) return -1;
        }

    return 0;
}

/*********************************************************************
*                                                                    *
*  Data access functionality                                         *
*                                                                    *
*********************************************************************/

cda_physchanhandle_t cda_add_physchan(cda_serverid_t sid, int physchan)
{
  serverinfo_t     *si = AccessV2sidSlot(sid);
  const chanaddr_t *p;
  int               chanofs;
  chaninfo_t       *ci;

  double            r;
  double            d;
  int32             q;
  int               i;
  physprops_t      *pp;
  
  chanaddr_t       *new_physlist;
  int32            *new_physcodes;
  tag_t            *new_phystags;
  rflags_t         *new_physrflags;
  chaninfo_t       *new_chaninfo;

#define GROW_ARR(name) if ((new_##name = safe_realloc(si->name, sizeof(*new_##name) * (si->physallocd + CHANS_ALLOC_INCREMENT))) == NULL) goto ALLOC_ERROR; \
    else si->name = new_##name
  
    if (_CdaCheckSid(sid) != 0) return CDA_PHYSCHANHANDLE_ERROR;

    if (si->is_running  &&  0) /*!!! <- 27.04.2005 -- pre-doublebuffer check? */
    {
        errno = EINPROGRESS;
        return CDA_PHYSCHANHANDLE_ERROR;
    }

    /* Try to find if this channel is already registered */
    p = memint(si->physlist, si->chan_base + physchan, si->physcount);
    if (p != NULL)
        return encode_chanhandle(sid, p - si->physlist);

    /* Okay, we need to add a new channel to the list -- find a space for it */
    if (si->physcount >= si->physallocd)
    {
        if (si->req_sent  &&  !si->double_buffer_used)
            if (ActivateDoubleBuffer(si, CHANS_ALLOC_INCREMENT) != 0)
                goto ALLOC_ERROR;
        
        GROW_ARR(physlist);
        GROW_ARR(physcodes);
        GROW_ARR(phystags);
        GROW_ARR(physrflags);
        GROW_ARR(chaninfo);

        si->physallocd += CHANS_ALLOC_INCREMENT;
    }

    chanofs = si->physcount++;
    ci = si->chaninfo + chanofs;
    bzero(ci, sizeof(*ci));

    /* Try to find a physinfo for this physchan */
    r = 1.0;
    d = 0.0;
    q = 0;
    for (i = 0, pp = si->phprops;  i < si->phpropscount;  i++, pp++)
        if (pp->n == physchan)
        {
            r = pp->r;
            d = pp->d;
            q = pp->q;
            break;
        }

    if      (q <  0) q = 0;
    else if (q == 0) q = 1;
    
    /* Initialize physchan properties */
    si->physlist    [chanofs] = si->chan_base + physchan;
    si->physcodes   [chanofs] = 0;
    si->phystags    [chanofs] = MAX_TAG_T;
    si->physrflags  [chanofs] = 0;
    ci->phys_r                = r;
    ci->phys_d                = d;
    ci->physval               = 0;
    ci->rflags                = CXCF_FLAG_DEFUNCT;
    ci->physrflags            = si->physrflags[chanofs];
    ci->physraw               = si->physcodes [chanofs];
    ci->phys_q                = q;
    ci->phystag               = si->phystags  [chanofs];
    ci->usercode              = 0;
    ci->physmodified          = MODIFIED_NOT;
    ci->isinitialized         = 0;

    /* And return them a handle */
    return encode_chanhandle(sid, chanofs);
    
 ALLOC_ERROR:
    errno = ENOMEM;
    return CDA_PHYSCHANHANDLE_ERROR;
}


typedef enum {VALUE_DBL,  VALUE_RAW}  dbl_or_raw_t;
typedef enum {CHG_BY_USR, CHG_BY_PRG} usr_or_prg_t;

static int setphyschanvor(cda_physchanhandle_t  chanh,
                          double  v, int32 rv, dbl_or_raw_t dbl_or_raw,
                          usr_or_prg_t usr_or_prg)
{
  cda_serverid_t  sid;
  int             chanofs;
  serverinfo_t   *si;
  chaninfo_t     *ci;

    /* Decode and check address */
    decode_objhandle(chanh, &sid, &chanofs);
    if (_CdaCheckSid(sid) != 0) return -1;
    
    si = AccessV2sidSlot(sid);

    if (chanofs >= si->physcount)
    {
        errno = EINVAL;
        return -1;
    }

    ci = si->chaninfo + chanofs;

    ////fprintf(stderr, "%s(%d,%f): sid=%d, chanofs=%d\n", __FUNCTION__, chanh, v, sid, chanofs);
    
    /* Remember new value */
    ci->rflags        &=~ (CXCF_FLAG_OTHEROP | CXCF_FLAG_PRGLYCHG);
    if (usr_or_prg == CHG_BY_PRG)
        ci->rflags    |= CXCF_FLAG_PRGLYCHG;

    ci->usercode      = dbl_or_raw == VALUE_RAW? rv
                                               : round((v + ci->phys_d) * ci->phys_r);
    ci->physmodified  = MODIFIED_USER;
    ci->mdctr         = 0;
    ci->isinitialized = 1;

    if (_cda_debug_setchan)
        fprintf(stderr, "%s setphyschan [%d:\"%s\"][%d]=%d\n",
                strcurtime_msc(),
                sid, si->srvrspec,
                si->physlist[chanofs], ci->usercode);

    return 0;
}


int  cda_setphyschanval(cda_physchanhandle_t  chanh, double  v)
{
    return setphyschanvor(chanh, v, 0,  VALUE_DBL, CHG_BY_USR);
}

int  cda_setphyschanraw(cda_physchanhandle_t  chanh, int32   rv)
{
    return setphyschanvor(chanh, 0, rv, VALUE_RAW, CHG_BY_USR);
}

int  cda_prgsetphyschanval(cda_physchanhandle_t  chanh, double  v)
{
    return setphyschanvor(chanh, v, 0,  VALUE_DBL, CHG_BY_PRG);
}

int  cda_prgsetphyschanraw(cda_physchanhandle_t  chanh, int32   rv)
{
    return setphyschanvor(chanh, 0, rv, VALUE_RAW, CHG_BY_PRG);
}

int  cda_getphyschanval(cda_physchanhandle_t  chanh, double *vp, tag_t *tag_p, rflags_t *rflags_p)
{
    return cda_getphyschanvnr(chanh, vp, NULL, tag_p, rflags_p);
}

int  cda_getphyschanraw(cda_physchanhandle_t  chanh, int32  *rp, tag_t *tag_p, rflags_t *rflags_p)
{
    return cda_getphyschanvnr(chanh, NULL, rp, tag_p, rflags_p);
}

int  cda_getphyschanvnr(cda_physchanhandle_t  chanh, double *vp, int32  *rp, tag_t *tag_p, rflags_t *rflags_p)
{
  cda_serverid_t  sid;
  int             chanofs;
  serverinfo_t   *si;
  chaninfo_t     *ci;

    /* Decode and check address */
    decode_objhandle(chanh, &sid, &chanofs);
    if (_CdaCheckSid(sid) != 0) return -1;
    
    si = AccessV2sidSlot(sid);

    if (chanofs >= si->physcount)
    {
        errno = EINVAL;
        return -1;
    }

    ci = si->chaninfo + chanofs;

    /* Get info */
    if (vp       != NULL) *vp    = ci->physval;
    if (rp       != NULL) *rp    = ci->physraw;
    if (tag_p    != NULL) *tag_p = ci->phystag;
    if (rflags_p != NULL)
        *rflags_p = (ci->rflags     & CXRF_CLIENT_MASK) |
                    (ci->physrflags & CXRF_SERVER_MASK);

    return 0;
}


int  cda_getphyschan_rd(cda_physchanhandle_t  chanh, double *rp, double *dp)
{
  cda_serverid_t  sid;
  int             chanofs;
  serverinfo_t   *si;
  chaninfo_t     *ci;

    /* Decode and check address */
    decode_objhandle(chanh, &sid, &chanofs);
    if (_CdaCheckSid(sid) != 0) return -1;
    
    si = AccessV2sidSlot(sid);

    if (chanofs >= si->physcount)
    {
        errno = EINVAL;
        return -1;
    }

    ci = si->chaninfo + chanofs;

    /* Get info */
    if (rp != NULL) *rp = ci->phys_r;
    if (dp != NULL) *dp = ci->phys_d;

    return 0;
}

int  cda_getphyschan_q (cda_physchanhandle_t  chanh, double *qp)
{
  cda_serverid_t  sid;
  int             chanofs;
  serverinfo_t   *si;
  chaninfo_t     *ci;

    /* Decode and check address */
    decode_objhandle(chanh, &sid, &chanofs);
    if (_CdaCheckSid(sid) != 0) return -1;
    
    si = AccessV2sidSlot(sid);

    if (chanofs >= si->physcount)
    {
        errno = EINVAL;
        return -1;
    }

    ci = si->chaninfo + chanofs;

    /* Get info */
    if (qp != NULL) *qp = fabs( ((double)ci->phys_q) / ci->phys_r );

    return 0;
}


cda_bigchandle_t cda_add_bigc(cda_serverid_t sid, int bigc,
                              int nargs, int retbufsize,
                              int cachectl, int immediate)
{
  serverinfo_t     *si = AccessV2sidSlot(sid);
  int               bigcofs;
  bigcinfo_t       *new_bigcinfo;
  bigcinfo_t       *bi;
  bigcretrec_t     *rr;
  size_t            bufsize;
  void             *databuf;
  size_t            sndargs_ofs;
  size_t            rcvargs_ofs;
  size_t            modargs_ofs;
  size_t            prsargs_ofs;
  size_t            retrec_ofs;
  size_t            data_ofs;

    if (_CdaCheckSid(sid) != 0) return CDA_BIGCHANHANDLE_ERROR;

    if (bigc  < 0  ||
        nargs < 0  ||  nargs > CX_MAX_BIGC_PARAMS  ||
        retbufsize < 0)
    {
        errno = EINVAL;
        return CDA_BIGCHANHANDLE_ERROR;
    }

    /* Try to find if this bigc is already registered */
    for (bigcofs = 0;  bigcofs < si->bigccount;  bigcofs++)
        if (si->bigcinfo[bigcofs].bigc_n == si->bigc_base + bigc)
            return encode_chanhandle(sid, bigcofs);

    /* Okay, let's try to find a free cell... */
    for (bigcofs = 0;  bigcofs < si->bigcallocd;  bigcofs++)
        if (si->bigcinfo[bigcofs].bigc_n == FREE_CELL_BIGC_N)
            goto CELL_FOUND;

    /* None?! Okay, let's grow this array...  */
    /*!!! Yes, we don't use double-buffer, as for regular channels,
          because with current single-bigc server support we would
          NEVER get a second bigc for an existing and running connection. */
    new_bigcinfo = safe_realloc(si->bigcinfo,
                                sizeof(bigcinfo_t) * (si->bigcallocd + BIGCS_ALLOC_INCREMENT));
    if (new_bigcinfo == NULL) return CDA_BIGCHANHANDLE_ERROR;
    si->bigcinfo = new_bigcinfo;

    for (bigcofs = si->bigcallocd;
         bigcofs < si->bigcallocd + BIGCS_ALLOC_INCREMENT;
         bigcofs++)
    {
        bi = si->bigcinfo + bigcofs;
        bzero(bi, sizeof(*bi));
        bi->bigc_n = FREE_CELL_BIGC_N;
    }

    si->bigcallocd += BIGCS_ALLOC_INCREMENT;

    bigcofs = si->bigcallocd - BIGCS_ALLOC_INCREMENT; // A shorthand for a redo of search
    
 CELL_FOUND:
     
    /* Allocate a combined buffer for all data */
    bufsize = 0;
    sndargs_ofs = bufsize;  bufsize += sizeof(int32) * nargs;
    rcvargs_ofs = bufsize;  bufsize += sizeof(int32) * nargs;
    modargs_ofs = bufsize;  bufsize += sizeof(int32) * nargs;
    prsargs_ofs = bufsize;  bufsize += sizeof(int32) * nargs;
    retrec_ofs  = bufsize;  bufsize += (sizeof(bigcretrec_t) + 3) &~ 3U;
    data_ofs    = bufsize;  bufsize += retbufsize;
     
    databuf = malloc(bufsize);
    if (databuf == NULL) return CDA_BIGCHANHANDLE_ERROR;
    bzero(databuf, bufsize);

    /* Okay, all done, let's fill in the fields */
    bi = si->bigcinfo + bigcofs;
    bzero(bi, sizeof(*bi));
    bi->bigc_n      = si->bigc_base + bigc;
    bi->active      = 1;
    bi->nargs       = nargs;
    bi->retbufsize  = retbufsize;
    bi->cachectl    = cachectl;
    bi->immediate   = immediate;

    bi->buf         = databuf;
    bi->sndargs_ofs = sndargs_ofs;
    bi->rcvargs_ofs = rcvargs_ofs;
    bi->modargs_ofs = modargs_ofs;
    bi->prsargs_ofs = prsargs_ofs;
    bi->retrec_ofs  = retrec_ofs;
    bi->data_ofs    = data_ofs;

    rr = (bigcretrec_t *)(bi->buf + bi->retrec_ofs);
    rr->tag = MAX_TAG_T;
    
    /* Modify "maximum", if required */
    if (si->bigccount <= bigcofs) si->bigccount = bigcofs + 1;
    
    return encode_chanhandle(sid, bigcofs);
}

#define CDA_BIGC_INTRO()                                  \
  cda_serverid_t  sid;                                    \
  int             bigcofs;                                \
  serverinfo_t   *si;                                     \
  bigcinfo_t     *bi;                                     \
                                                          \
    /* Decode and check address */                        \
    decode_objhandle(bigch, &sid, &bigcofs);              \
    if (_CdaCheckSid(sid) != 0) return -1;                \
                                                          \
    si = AccessV2sidSlot(sid);                            \
                                                          \
    if (bigcofs >= si->bigccount  ||                      \
        si->bigcinfo[bigcofs].bigc_n == FREE_CELL_BIGC_N) \
    {                                                     \
        errno = EINVAL;                                   \
        return -1;                                        \
    }                                                     \
                                                          \
    bi = si->bigcinfo + bigcofs;


int              cda_del_bigc(cda_bigchandle_t  bigch)
{
    CDA_BIGC_INTRO();

    /* Can't delete "live" channels... */
    if (si->req_sent)
    {
        errno = EINPROGRESS;
        return -1;
    }

    /* Okay, let's free bigc's resources and mark the cell as free */
    safe_free(bi->buf);
    safe_free(bi->sndbuf);
    bi->bigc_n = FREE_CELL_BIGC_N;

    /* And modify bigccount if needed... */
    if (si->bigccount == bigcofs + 1)
        while (si->bigccount > 0  &&
               si->bigcinfo[si->bigccount - 1].bigc_n == FREE_CELL_BIGC_N)
            si->bigccount--;
    
    return 0;
}

int cda_getbigcdata   (cda_bigchandle_t bigch, size_t ofs, size_t size, void *buf)
{
  bigcretrec_t *rr;
  
    CDA_BIGC_INTRO();

    rr = (bigcretrec_t *)(bi->buf + bi->retrec_ofs);
    
    /* Okay, do we have enough data? */
    if (ofs >= rr->retbufused) return 0;
    if (ofs + size > rr->retbufused) size = rr->retbufused - ofs;

    if (size != 0)
        memcpy(buf, bi->buf + bi->data_ofs + ofs, size);
    
    return size;
}

int cda_setbigcdata   (cda_bigchandle_t bigch, size_t ofs, size_t size, void *buf, size_t bufunits)
{
  size_t  newsize = ofs + size;

    CDA_BIGC_INTRO();

    if (newsize > bi->retbufsize)
    {
        errno = E2BIG;
        return -1;
    }

    /* Grow the buffer, additionally  */
    if (newsize > bi->sndbufallocd)
    {
        if (GrowBuf((void *)&(bi->sndbuf),
                    &(bi->sndbufallocd),
                    newsize) < 0)
            return -1;
    }

    /* Increase "used" size if necessary */
    if (bi->sndbufused < newsize)
    {
        /* Fill previously-unused part with zeros */
        if (ofs > bi->sndbufused)
            bzero(bi->sndbuf + bi->sndbufused, ofs - bi->sndbufused);
        bi->sndbufused = newsize;
    }

    /* Store and flag */
    if (size != 0)
        memcpy(bi->sndbuf + ofs, buf, size);
    bi->snddata_present = 1;
    bi->sndbufunits     = bufunits;

    return 0;
}

int cda_getbigcstats  (cda_bigchandle_t bigch, tag_t *tag_p, rflags_t *rflags_p)
{
  bigcretrec_t *rr;
  
    CDA_BIGC_INTRO();

    rr = (bigcretrec_t *)(bi->buf + bi->retrec_ofs);
    
    if (tag_p    != NULL) *tag_p    = rr->tag;
    if (rflags_p != NULL) *rflags_p = rr->rflags;

    return 0;
}

int cda_getbigcparams (cda_bigchandle_t bigch, int start, int count, int32 *params)
{
    CDA_BIGC_INTRO();

    /* Okay, what about start & count? */
    if (start < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (start >= bi->nargs) return 0;
    if (start + count > bi->nargs) count = bi->nargs - start;

    /* BEGIN "meat" */
    if (count != 0)
    {
        ///BAD!!!:    memcpy(params, bi->buf + bi->rcvargs_ofs, sizeof(*params) * count);
        memcpy(params, (int32 *)(bi->buf + bi->rcvargs_ofs) + start, sizeof(*params) * count);
    }
    /* END "meat" */
    
    return count;
}

int cda_setbigcparams (cda_bigchandle_t bigch, int start, int count, int32 *params)
{
    CDA_BIGC_INTRO();

    /* Okay, what about start & count? */
    if (start < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (start >= bi->nargs) return 0;
    if (start + count > bi->nargs) count = bi->nargs - start;

    /* BEGIN "meat" */
    if (count != 0)
    {
        memcpy  ((int32 *)(bi->buf + bi->sndargs_ofs) + start, params, sizeof(*params) * count);
        memset32((int32 *)(bi->buf + bi->modargs_ofs) + start, 1,      count);
    }
    /* END "meat" */

    return count;
}

int cda_setbigcparam_p(cda_bigchandle_t bigch, int param, int is_persistent)
{
    CDA_BIGC_INTRO();

    if (param < 0  ||  param >= bi->nargs)
    {
        errno = EINVAL;
        return -1;
    }

    *((int32 *)(bi->buf + bi->prsargs_ofs) + param) = (is_persistent != 0);

    return 0;
}

int cda_setbigc_cachectl(cda_bigchandle_t  bigch, int cachectl)
{
    CDA_BIGC_INTRO();

    bi->cachectl = cachectl;

    return 0;
}


char *cda_strserverstatus_short(int status)
{
    if      (status == CDA_SERVERSTATUS_NORMAL)       return "NORMAL";
    else if (status == CDA_SERVERSTATUS_FROZEN)       return "FROZEN";
    else if (status == CDA_SERVERSTATUS_ALMOSTREADY)  return "ALMOSTREADY";
    else if (status == CDA_SERVERSTATUS_DISCONNECTED) return "DISCONNECTED";
    else                                              return "UNKNOWN";
}
