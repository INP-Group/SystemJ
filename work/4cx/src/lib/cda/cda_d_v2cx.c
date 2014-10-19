#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include "misc_macros.h"
#include "cxscheduler.h"

#include "cx.h"
#include "cda.h"
#include "cdaP.h"

#include "cda_d_v2cx.h"
typedef uint8  tag_t; // Since it is absent in v4's cx.h (otherwise compatible with v2's)
#include "v2cxlib_renames.h"
#include "v2subsysaccess.h"


//////////////////////////////////////////////////////////////////////

static int IsATemporaryCxError(void)
{
    return
        IS_A_TEMPORARY_CONNECT_ERROR()  ||
        errno == CENOHOST    ||
        errno == CETIMEOUT   ||  errno == CEMANYCLIENTS  ||
        errno == CESRVNOMEM  ||  errno == CEMCONN;
}

/////////
enum {MAX_TAG_T = (1 << (sizeof(tag_t) * 8)) - 1};
/////////
enum {FROZEN_LIMIT = 5};
enum {STD_CYCLESIZE = 1000000};
enum {DEFAULT_HEARTBEAT_USECS      = 5*60*1000000};
enum {DEFAULT_DEFUNCTHB_USECS      =   10*1000000};
/////////

enum
{
    CHANS_ALLOC_INCREMENT = 100,
};

enum
{
    MODIFIED_NOT  = 0,
    MODIFIED_USER = 1,
    MODIFIED_SENT = 2
};

typedef struct
{
    int32          phys_q;
    int32          usercode;
    tag_t          mdctr;
    int8           physmodified;
    int8           isinitialized;
} chaninfo_t;

typedef struct
{
    cda_srvconn_t   sid;
    int             uniq;
    int             cd;
    int             is_suffering;
    int             was_data_reply;
    int             cyclesize;
    time_t          last_data_time;

    sl_tid_t        rcn_tid;
    sl_tid_t        hbt_tid;
    sl_tid_t        dfc_tid;

    int             physallocd;
    int             physcount;
    chaninfo_t     *chaninfo;
    /* For cda_dat_p_update_dataset() */
    cda_dataref_t  *refs;
    void          **vps;
    cxdtype_t      *dtypes;
    int            *nelems;
    // no rflags_t *rflags -- physrflags used instead
    cx_time_t      *timestamps;
    /* For cx_getvset */
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
} cda_d_v2cx_privrec_t;

// -------------------------------------------------------------------

static int  ActivateDoubleBuffer(cda_d_v2cx_privrec_t *si, int arr_inc)
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

static void FoldDoubleBuffer(cda_d_v2cx_privrec_t *si, int newdata_present)
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



//////////////////////////////////////////////////////////////////////


static int determine_name_type(const char *name,
                               char srvrspec[], size_t srvrspec_size,
                               char **channame_p)
{
  int         srv_len;
  const char *colon_p;
  const char *p;
  char       *vp;

    /* Defaults */
    srv_len = 0;
    *channame_p = name;

    /* Doesn't start with alphanumeric? */
    if (!isalnum(*name)) goto DO_RET;

    /* Is there a colon at all? */
    colon_p = strchr(name, ':');
    if (colon_p == NULL) goto DO_RET;

    /* Check than EVERYTHING preceding ':' can constitute a hostname */
    for (p = name;  p < colon_p;  p++)
        if (*p == '\0'  ||  (!isalnum(*p)  &&  *p != '.')) goto DO_RET;
    /* Okay, skip ':' */
    p++;
    /* ':' should be followed by a digit */
    if (*p == '\0'  ||  !isdigit(*p)) goto DO_RET;
    /* Okay, skip digits... */
    while (*p != '\0'  &&  isdigit(*p)) p++;
    //
    if (*p != '.') goto DO_RET;

    ////////////////////
    srv_len = p - name;
    *channame_p = p + 1;

 DO_RET:
    if (srv_len > 0)
    {
        if (srv_len > srvrspec_size - 1)
            srv_len = srvrspec_size - 1;
        memcpy(srvrspec, name, srv_len);
    }
    srvrspec[srv_len] = '\0';
    for (vp = srvrspec;  *vp != '\0';  vp++) *vp = tolower(*vp);

    return srv_len != 0;
}

static int  channame2chan_n(const char *channame, int channamelen)
{
  int                   x;
  int                   acc;
  int                   chan_n;

    for (x = 0,   chan_n = 0, acc = 0;
         x < channamelen;
         x++)
    {
        if (channame[x] == '.')
        {
            chan_n += acc;
            acc = 0;
        }
        else if (channame[x] >= '0'  &&  channame[x] <= '9')
        {
            acc = acc * 10 + (channame[x] - '0');
        }
        else
            return -1;
    }
    chan_n += acc;

    return chan_n;
}

static int  cda_d_v2cx_new_chan(cda_dataref_t ref, const char *name,
                                cxdtype_t dtype, int nelems)
{
  cda_d_v2cx_privrec_t *me;

  char                  srvrspec[200];
  const char           *channame;
  int                   w_srv;
  const char           *at_p;
  size_t                channamelen;
  const char           *params;

  int                   chan_n;
  double                phys_r;
  double                phys_d;
  double                phys_rds[2];
  int                   phys_q;
  v2subsysaccess_strs_t strs;

  int               chanofs;
  chaninfo_t       *ci;

  cda_dataref_t    *new_refs;
  void            **new_vps;
  cxdtype_t        *new_dtypes;
  int              *new_nelems;
  cx_time_t        *new_timestamps;

  chanaddr_t       *new_physlist;
  int32            *new_physcodes;
  tag_t            *new_phystags;
  rflags_t         *new_physrflags;
  chaninfo_t       *new_chaninfo;

#define GROW_ARR(name) \
    if ((new_##name = safe_realloc(me->name, sizeof(*new_##name) * (me->physallocd + CHANS_ALLOC_INCREMENT))) == NULL) \
        goto ALLOC_ERROR; \
    else me->name = new_##name

    phys_r = 1.0;
    phys_d = 0.0;

    w_srv = determine_name_type(name, srvrspec, sizeof(srvrspec), &channame);
    ////fprintf(stderr, "\t%s(%d, \"%s\")\n\t\t[%s]%s\n", __FUNCTION__, ref, name, srvrspec, channame);

    if (strcasecmp(srvrspec, "unknown") == 0)
    {
        srvrspec[0] = '\0';
        w_srv = 0;
    }

//fprintf(stderr, "<%s><%s>\n", srvrspec, channame);
    at_p = strchr(channame, '@');
    if (at_p != NULL)
    {
        channamelen = at_p - channame;
        params      = at_p + 1;
    }
    else
    {
        channamelen = strlen(channame);
        params      = "";
    }

    if (channamelen < 1)
    {
        cda_set_err("empty CHANNEL name");
        return CDA_DAT_P_ERROR;
    }

    /*  */
    chan_n = channame2chan_n(channame, channamelen);
    if (chan_n < 0)
    {
        if (!w_srv) srvrspec[0] = '\0';
        if (v2subsysaccess_resolve("kukareku", /*!!! cda_dat_p_argv0_of_ref() */
                                   channame,
                                   w_srv? NULL : srvrspec,
                                   w_srv?    0 : sizeof(srvrspec),
                                   &chan_n,
                                   &phys_r, &phys_d, &phys_q,
                                   &strs) < 0)
        {
            cda_set_err("unable to resolve CHANNEL name");
            return CDA_DAT_P_ERROR;
        }
        //fprintf(stderr, "\tr=%8.3f, d=%8.3f\n", phys_r, phys_d);
        if (!w_srv) w_srv = srvrspec[0] != '\0';
    }
    //fprintf(stderr, "\t%s!%d\n", srvrspec, chan_n);

    if (w_srv)
    {
        me = cda_dat_p_get_server(ref, &CDA_DAT_P_MODREC_NAME(v2cx), srvrspec);
        if (me == NULL) return CDA_DAT_P_ERROR;
    }
    else
        return CDA_DAT_P_ERROR;

    /* Okay -- here we have all required info, as well as a server-rec */

    /* Okay, we need to add a new channel to the list -- find a space for it */
    if (me->physcount >= me->physallocd)
    {
        if (me->req_sent  &&  !me->double_buffer_used)
            if (ActivateDoubleBuffer(me, CHANS_ALLOC_INCREMENT) != 0)
                goto ALLOC_ERROR;
        
        GROW_ARR(refs);
        GROW_ARR(vps);
        GROW_ARR(dtypes);
        GROW_ARR(nelems);
        GROW_ARR(timestamps);

        GROW_ARR(physlist);
        GROW_ARR(physcodes);
        GROW_ARR(phystags);
        GROW_ARR(physrflags);
        GROW_ARR(chaninfo);

        me->physallocd += CHANS_ALLOC_INCREMENT;
    }

    chanofs = me->physcount++;
    ci = me->chaninfo + chanofs;
    bzero(ci, sizeof(*ci));

    /* Physinfo? */

    /* Initialize physchan properties */
    me->refs        [chanofs] = ref;
    me->vps         [chanofs] = NULL;
    me->dtypes      [chanofs] = CXDTYPE_INT32;
    me->nelems      [chanofs] = 1;
    me->timestamps  [chanofs].sec = me->timestamps  [chanofs].nsec = 0;
    //-
    me->physlist    [chanofs] = chan_n;
    me->physcodes   [chanofs] = 0;
    me->phystags    [chanofs] = MAX_TAG_T;
    me->physrflags  [chanofs] = 0;
    ci->phys_q                = 0;
    ci->usercode              = 0;
    ci->physmodified          = MODIFIED_NOT;
    ci->isinitialized         = 0;
//fprintf(stderr, "refs[%d]=%d\n", chanofs, me->refs[chanofs]);

    cda_dat_p_set_hwr     (ref, chanofs);
    phys_rds[0] = phys_r;
    phys_rds[1] = phys_d;
    cda_dat_p_set_phys_rds (ref, 1, phys_rds);
    cda_dat_p_set_fresh_age(ref, FROZEN_LIMIT*1000000);
    cda_dat_p_set_strings  (ref, strs.ident, strs.label,
                            strs.tip, strs.comment, strs.geoinfo, strs.rsrvd6,
                            strs.units, strs.dpyfmt);

    return CDA_DAT_P_NOTREADY;

 ALLOC_ERROR:
    errno = ENOMEM;
    return CDA_DAT_P_ERROR;
}

static void cda_d_v2cx_snd_data(void *pdt_privptr, int hwr,
                                cxdtype_t dtype, int nelems, void *value)
{
  cda_d_v2cx_privrec_t *me = pdt_privptr;
  chaninfo_t   *ci;
  int32         v32;

    if (hwr < 0  ||  hwr >= me->physcount)
    {
        /*!!!Bark? */
        return;
    }
    if (nelems != 1)
    {
        /*!!!Bark? */
        return;
    }
    if      (dtype == CXDTYPE_INT32)  v32 =       *((int32 *)value);
    else if (dtype == CXDTYPE_DOUBLE) v32 = round(*((double*)value));
    else
    {
        /*!!!Bark? */
        return;
    }
fprintf(stderr, "%s(): v=%d\n", __FUNCTION__, v32);

    ci = me->chaninfo + hwr;
    ci->usercode     = v32;
    ci->physmodified = MODIFIED_USER;
    ci->mdctr        = 0;
    ci->isinitialized = 1;
}

//////////////////////////////////////////////////////////////////////

static void MarkAllAsDefunct(cda_d_v2cx_privrec_t *me)
{
fprintf(stderr, "%s\n", __FUNCTION__);
    me->was_data_reply = 0;
    cda_dat_p_defunct_dataset    (me->sid,
                                  me->physcount,
                                  me->refs);
    cda_dat_p_update_server_cycle(me->sid);
}

static void DecodeData (cda_d_v2cx_privrec_t *me)
{
  int             i;
  chaninfo_t     *ci;
  struct timeval  now;
  int32           delta;

    gettimeofday(&now, NULL);
    me->last_data_time = now.tv_sec;
    for (i = 0, ci = me->chaninfo;  i < me->physcount;  i++, ci++)
    {
        me->vps       [i]      = me->physcodes + i;
        me->timestamps[i].sec  = now.tv_sec;
        me->timestamps[i].nsec = now.tv_usec * 1000;
        /*!!! Subtract tag, handle MAX_TAG_T ages */
        if (me->phystags[i] == MAX_TAG_T)
            me->timestamps[i].sec = 1;
        else
            me->timestamps[i].sec -= scale32via64(me->phystags[i],
                                                  me->cyclesize,
                                                  STD_CYCLESIZE);

        /* Okay, the "orange" intellect */
        if (ci->physmodified != MODIFIED_USER)
        {
            if (me->phystags[i] < ci->mdctr) /* This check is to prevent comparison with values obtained before we made a first/write request */
            {
                if (ci->isinitialized)
                {
                    delta = abs(ci->usercode - me->physcodes[i]);
                    if (delta != 0  &&  delta >= ci->phys_q)
                    {
                        me->physrflags[i] |= CXCF_FLAG_OTHEROP;
                        ci->usercode = me->physcodes[i];
                    }
                    else
                        me->physrflags[i] &=~ CXCF_FLAG_OTHEROP;
                }
                else
                {
                    ci->usercode = me->physcodes[i];
                    ci->isinitialized = 1;
                }
            }
        }
        if (ci->mdctr < MAX_TAG_T) ci->mdctr++;
        if (ci->physmodified == MODIFIED_SENT)
            ci->physmodified =  MODIFIED_NOT;
    }
}

static void RequestData(cda_d_v2cx_privrec_t *me)
{
  int           r;
  int           i;
  chaninfo_t   *ci;

    if (me->req_sent) return;
    
    cx_begin(me->cd);

    if (me->physcount != 0)
        cx_getvset(me->cd, me->physlist,  me->physcount,
                   me->physcodes, me->phystags, me->physrflags);

    for (i = 0, ci = me->chaninfo;  i < me->physcount;  i++, ci++)
        if (ci->physmodified != MODIFIED_NOT)
        {
            ////fprintf(stderr, "%s: setvalue(id=%d=>%d)\n", __FUNCTION__, i, me->physlist[i]);
            cx_setvalue(me->cd, me->physlist[i], ci->usercode,
                        NULL, NULL, NULL);
            ci->physmodified = MODIFIED_SENT;
        }

    me->req_sent = 1;
    r = cx_run(me->cd);
    if (r < 0  &&  errno != CERUNNING)
        cda_dat_p_report(me->sid, "%s(): cx_run()=%d: %s",
                         __FUNCTION__, r, cx_strerror(errno));
}

static void NewDataProc(cda_d_v2cx_privrec_t *me)
{
//    fprintf(stderr, "%s()\n", __FUNCTION__);
    me->was_data_reply = 1;
    me->req_sent = 0;
    if (me->double_buffer_used) FoldDoubleBuffer(me, 1);

    DecodeData(me);
    cda_dat_p_update_dataset     (me->sid,
                                  me->physcount,
                                  me->refs,
                                  me->vps,
                                  me->dtypes,
                                  me->nelems,
                                  me->physrflags,
                                  me->timestamps);
    cda_dat_p_update_server_cycle(me->sid);

    RequestData(me);
}

// -------------------------------------------------------------------

static void ReconnectTCB(int uniq, void *unsdptr,
                         sl_tid_t tid, void *privptr);

static void SuccessProc(cda_d_v2cx_privrec_t *me, int from_new)
{
    if (me->is_suffering)
    {
        cda_dat_p_report(me->sid, "connected.");
        me->is_suffering = 0;
    }
    
    sl_deq_tout(me->rcn_tid); me->rcn_tid = -1;

    me->cyclesize    = cx_getcyclesize(me->cd);
    if (me->cyclesize <= 0)
        me->cyclesize = STD_CYCLESIZE;

    cda_dat_p_set_server_state(me->sid, CDA_DAT_P_OPERATING);

    RequestData(me);
}

static void FailureProc(cda_d_v2cx_privrec_t *me, int from_new, int reason)
{
  int                   ec = errno;
  int                   us;
  enum {MAX_US_X10 = 2*1000*1000*1000};

    us = 1*1000*1000;
    if (ec == CENOHOST)
    {
        if (us > MAX_US_X10 / 10)
            us = MAX_US_X10;
        else
            us *= 10;
    }

    if (!me->is_suffering)
    {
        cda_dat_p_report(me->sid, "%s: %s; will reconnect.",
                   cx_strreason(reason), cx_strerror(ec));
        me->is_suffering = 1;
    }

    /* Notify cda */
    cda_dat_p_set_server_state(me->sid, CDA_DAT_P_NOTREADY);
    
    /* Forget old connection */
    if (me->cd >= 0) cx_close(me->cd);
    me->cd = -1;
    me->req_sent = 0;

    {
      int         x;
      chaninfo_t *ci;

        for (x = 0, ci = me->chaninfo;  x < me->physcount;  x++, ci++)
            ci->mdctr = 0;
    }
    MarkAllAsDefunct(me);

    /* And organize a timeout in a second... */
    me->rcn_tid = sl_enq_tout_after(me->uniq, NULL,
                                    us, ReconnectTCB, me);
}

static void NotificationProc(int   cd         __attribute__((unused)),
                             int   reason,
                             void *privptr,
                             const void *info __attribute__((unused)))
{
  cda_d_v2cx_privrec_t *me = (cda_d_v2cx_privrec_t *)privptr;
  int                   ec = errno;

    ////fprintf(stderr, "%s: reason=%d\n", __FUNCTION__, reason);
    switch (reason)
    {
        case CAR_ANSWER:
            NewDataProc(me);
            break;
        
        case CAR_CONNECT:
            SuccessProc(me, 0);
            break;

        case CAR_CONNFAIL:
        case CAR_ERRCLOSE:
        case CAR_KILLED:
            FailureProc(me, 0, reason);
            break;
    }
}

static void ReconnectTCB(int uniq, void *unsdptr,
                         sl_tid_t tid, void *privptr)
{
  cda_d_v2cx_privrec_t *me = privptr;

    me->rcn_tid = -1;

    me->cd  = cx_connect_n(cda_dat_p_srvrn_of(me->sid),
                           cda_dat_p_argv0_of(me->sid),
                           NotificationProc, me);
    if (me->cd < 0)
        FailureProc(me, 0, CAR_CONNFAIL);
}

static void HeartbeatTCB(int uniq, void *unsdptr,
                         sl_tid_t tid, void *privptr)
{
  cda_d_v2cx_privrec_t *me = privptr;

    me->hbt_tid = -1;

    if (me->cd >= 0  /*&&  si->is_running  &&  si->is_connected*/)
        cx_ping(me->cd);

    me->hbt_tid = sl_enq_tout_after(me->uniq, NULL,
                                    DEFAULT_HEARTBEAT_USECS,
                                    HeartbeatTCB, me);
}

static void DefunctHbTCB(int uniq, void *unsdptr,
                         sl_tid_t tid, void *privptr)
{
  cda_d_v2cx_privrec_t *me = privptr;

    me->dfc_tid = -1;

    if (me->was_data_reply  &&
        difftime(time(NULL), me->last_data_time) >= FROZEN_LIMIT)
        MarkAllAsDefunct(me);

    me->dfc_tid = sl_enq_tout_after(me->uniq, NULL,
                                    DEFAULT_DEFUNCTHB_USECS,
                                    DefunctHbTCB, me);
}


static int  cda_d_v2cx_new_srv (cda_srvconn_t  sid, void *pdt_privptr,
                                int            uniq,
                                const char    *srvrspec,
                                const char    *argv0)
{
  cda_d_v2cx_privrec_t *me = pdt_privptr;
  int                   ec;

fprintf(stderr, "ZZZ %s(%s)\n", __FUNCTION__, srvrspec);
    me->sid  = sid;
    me->uniq = uniq;
    me->cd   = cx_connect_n(cda_dat_p_srvrn_of(me->sid),
                            cda_dat_p_argv0_of(me->sid),
                            NotificationProc, me);
    me->rcn_tid = -1;
    me->hbt_tid = sl_enq_tout_after(me->uniq, NULL,
                                    DEFAULT_HEARTBEAT_USECS,
                                    HeartbeatTCB, me);
    me->dfc_tid = sl_enq_tout_after(me->uniq, NULL,
                                    DEFAULT_DEFUNCTHB_USECS,
                                    DefunctHbTCB, me);

    if (me->cd < 0)
    {
        if (!IsATemporaryCxError())
        {
            ec = errno;
            cda_set_err("cx_open(\"%s\"): %s",
                        cda_dat_p_srvrn_of(me->sid), cx_strerror(ec));
            return CDA_DAT_P_ERROR;
        }
        
        FailureProc(me, 1, CAR_CONNFAIL);
    }
    else
        /*SuccessProc(me, 1)*/;

    return CDA_DAT_P_NOTREADY;
}

static void cda_d_v2cx_del_srv (cda_srvconn_t  sid, void *pdt_privptr)
{
  cda_d_v2cx_privrec_t *me = pdt_privptr;
  
    if (me->cd >= 0)
        cx_close(me->cd);
    sl_deq_tout(me->rcn_tid); me->rcn_tid = -1;
}

//////////////////////////////////////////////////////////////////////

CDA_DEFINE_DAT_PLUGIN(v2cx, "CXv2 data-access plugin",
                      NULL, NULL,
                      sizeof(cda_d_v2cx_privrec_t),
                      '.', ':', '@',
                      cda_d_v2cx_new_chan, NULL,
                      cda_d_v2cx_snd_data, NULL,
                      cda_d_v2cx_new_srv,  cda_d_v2cx_del_srv);
