#include "cxsd_includes.h"

#include "sendqlib.h" // for public_funcs_list[] only


#define DO_CHECK_SANITY_OF_DEVID_WO_STATE(func_name, errret)                \
    do {                                                                    \
        if (devid < 0  ||  devid >= numdevs)                                \
        {                                                                   \
            logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d): devid out of [0..numdevs=%d)", \
                    func_name, devid, active_dev, numdevs);                 \
            return errret;                                                  \
        }                                                                   \
    } while (0)
#define DO_CHECK_SANITY_OF_DEVID(func_name, errret)                         \
    do {                                                                    \
        DO_CHECK_SANITY_OF_DEVID_WO_STATE(func_name, errret);               \
        if (d_state[devid] == DEVSTATE_OFFLINE)                             \
        {                                                                   \
            logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d): state=OFFLINE", \
                    func_name, devid, active_dev);                          \
            return errret;                                                  \
        }                                                                   \
    } while (0)

#define CHECK_SANITY_OF_DEVID_WO_STATE(errret) \
    DO_CHECK_SANITY_OF_DEVID_WO_STATE(__FUNCTION__, errret)
#define CHECK_SANITY_OF_DEVID(errret) \
    DO_CHECK_SANITY_OF_DEVID(__FUNCTION__, errret)


int  cxsd_uniq_checker(const char *func_name, int uniq)
{
  int                  devid = uniq;

    if (uniq == 0  ||  uniq == DEVID_NOT_IN_DRIVER) return 0;

    DO_CHECK_SANITY_OF_DEVID(func_name, -1);

    return 0;
}


void        RegisterDevPtr   (int devid, void *devptr)
{
    CHECK_SANITY_OF_DEVID();

    d_devptr[devid] = devptr;
}

void DoDriverLog (int devid, int level, const char *format, ...)
{
  va_list        ap;
  
    va_start(ap, format);
    vDoDriverLog(devid, level, format, ap);
    va_end(ap);
}

void vDoDriverLog(int devid, int level, const char *format, va_list ap)
{
  int         log_type = LOGF_DBG2;
  char        subaddress[200];
  int         category = level & DRIVERLOG_C_mask;
  const char *catname;
    
    if (level & DRIVERLOG_ERRNO) log_type |= LOGF_ERRNO;

    if (devid >= 0  &&  devid < numdevs)
    {
        if (category != DRIVERLOG_C_DEFAULT  &&
            !DRIVERLOG_m_C_ISIN_CHECKMASK(category, d_logmask[devid]))
            return;

        catname = GetDrvlogCatName(category);
        snprintf(subaddress, sizeof(subaddress),
                 "%s[%d]%s%s%s",
                 d_metric[devid]->mr.name, devid, d_instname[devid],
                 catname[0] != '\0'? "/" : "", catname);
    }
    else if (0 /*!!!"is-in-{init,term}_{drv,lyr}*/)
    {
    }
    else
    {
        if (category != DRIVERLOG_C_DEFAULT  &&
            !DRIVERLOG_m_C_ISIN_CHECKMASK(category, option_defdrvlog_mask))
            return;

        catname = GetDrvlogCatName(category);
        snprintf(subaddress, sizeof(subaddress),
                 "NOT-IN-DRIVER%s%s",
                 catname[0] != '\0'? "/" : "", catname);
    }
    
    vloglineX(log_type, level & DRIVERLOG_LEVEL_mask, subaddress, format, ap);
}


int  GetCurrentCycle(void)
{
    return current_cycle;
}


void GetCurrentLogSettings(int devid, int *curlevel_p, int *curmask_p)
{
    CHECK_SANITY_OF_DEVID();
    
    if (curlevel_p != NULL) *curlevel_p = option_verbosity;
    if (curmask_p  != NULL) *curmask_p  = d_logmask[devid];
}


const char *GetDevLabel    (int devid)
{
    CHECK_SANITY_OF_DEVID(NULL);

    return d_instname[devid];
}


void  SetDevState     (int devid, int state,
                       rflags_t rflags_to_set, const char *description)
{
    CHECK_SANITY_OF_DEVID_WO_STATE();

    /* Is `state' sane? */
    if (state != DEVSTATE_OFFLINE  &&  state != DEVSTATE_NOTREADY  &&  state != DEVSTATE_OPERATING)
    {
        logline(LOGF_SYSTEM, 0, "%s: (devid=%d/active=%d) request for non-existent state %d",
                __FUNCTION__, devid, active_dev, state);
        return;
    }
    
    /* No-op? */
    if (state == d_state[devid]) return;
    /* Escaping from OFFLINE is forbidden */
    if (d_state[devid] == DEVSTATE_OFFLINE)
    {
        logline(LOGF_SYSTEM, 0, "%s: (devid=%d/active=%d) attempt to move from DEVSTATE_OFFLINE to %d",
                __FUNCTION__, devid, active_dev, state);
        return;
    }

    if      (state == DEVSTATE_OFFLINE)   TermDev       (devid, rflags_to_set);
    else if (state == DEVSTATE_NOTREADY)  FreezeDev     (devid, rflags_to_set);
    else if (state == DEVSTATE_OPERATING) ReviveDev     (devid);
}

void  ReRequestDevData(int devid)
{
    CHECK_SANITY_OF_DEVID();

    ReRequestDevChans(devid);
    ReRequestDevBigcs(devid);
}

void StdSimulated_rw_p(int devid, void *devptr __attribute__((unused)), int firstchan, int count, int32 *values, int action)
{
  int       chan;
  int       globalchan;
  int32     nextval;
  rflags_t  empty_rflags = 0;
    
    for (chan = firstchan, globalchan = firstchan + d_ofs[devid];
         chan < firstchan + count;
         chan++,           globalchan++)
    {
        /* Calculation formula is copied from HandleSimulatedHardware() */
        nextval = c_type[globalchan]? (action == DRVA_WRITE? values[chan - firstchan]
                                                           : c_value[globalchan])
                                    : globalchan * 1000 + current_cycle;
        ReturnChanGroup(devid, chan, 1, &nextval, &empty_rflags);
    }
}


/* "inserver" API */

inserver_devref_t   inserver_get_devref (int                   devid,
                                         const char *dev_name)
{
  int  dev_n;
    
    if (dev_name == NULL  ||  *dev_name == '\0')
    {
        return INSERVER_DEVREF_ERROR;
    }

    for (dev_n = 1;  dev_n < numdevs;  dev_n++)
        if (strcasecmp(dev_name, d_instname[dev_n]) == 0)
            return dev_n;

    return INSERVER_DEVREF_ERROR;
}

inserver_chanref_t  inserver_get_chanref(int                   devid,
                                         const char *dev_name, int chan_n)
{
  int  dev_n;
    
    if (dev_name == NULL  ||  *dev_name == '\0'  ||  chan_n < 0)
    {
        return INSERVER_CHANREF_ERROR;
    }

    for (dev_n = 1;  dev_n < numdevs;  dev_n++)
        if (strcasecmp(dev_name, d_instname[dev_n]) == 0)
        {
            if (chan_n >= d_size[dev_n]) return INSERVER_CHANREF_ERROR;
            return d_ofs[dev_n] + chan_n;
        }

    return INSERVER_CHANREF_ERROR;
}

inserver_bigcref_t  inserver_get_bigcref(int                   devid,
                                         const char *dev_name, int bigc_n)
{
  int  dev_n;
    
    if (dev_name == NULL  ||  *dev_name == '\0'  ||  bigc_n < 0)
    {
        return INSERVER_BIGCREF_ERROR;
    }

    for (dev_n = 1;  dev_n < numdevs;  dev_n++)
        if (strcasecmp(dev_name, d_instname[dev_n]) == 0)
        {
            if (bigc_n >= d_big_count[dev_n]) return INSERVER_CHANREF_ERROR;
            return d_big_first[dev_n] + bigc_n;
        }

    return INSERVER_BIGCREF_ERROR;
}

int  inserver_get_devstat       (int                        devid,
                                 inserver_devref_t          ref,
                                 int *state_p)
{
    if (ref < 0  ||  ref >= numdevs)
    {
        return -1;
    }

    if (state_p != NULL)
        *state_p = d_state[ref];

    return 0;
}

int  inserver_chan_is_rw        (int                        devid,
                                 inserver_chanref_t         ref)
{
    if (ref < 0  ||  ref >= numchans  ||
        d_state[c_devid[ref]] == DEVSTATE_OFFLINE)
    {
        return -1;
    }

    return c_type[ref];
}

int  inserver_req_cval          (int                        devid,
                                 inserver_chanref_t         ref)
{
    if (ref < 0  ||  ref >= numchans  ||
        d_state[c_devid[ref]] == DEVSTATE_OFFLINE)
    {
        return -1;
    }

    MarkRangeForIO(ref, 1, DRVA_READ, NULL);

    return 0;
}

int  inserver_snd_cval          (int                        devid,
                                 inserver_chanref_t         ref,
                                 int32  val)
{
    if (ref < 0  ||  ref >= numchans  ||
        d_state[c_devid[ref]] == DEVSTATE_OFFLINE)
    {
        return -1;
    }

    MarkRangeForIO(ref, 1, DRVA_WRITE, &val);

    return 0;
}

int  inserver_get_cval          (int                        devid,
                                 inserver_chanref_t         ref,
                                 int32 *val_p, rflags_t *rflags_p, tag_t *tag_p)
{
    if (ref < 0  ||  ref >= numchans)
    {
        return -1;
    }

    if (val_p    != NULL) *val_p    = c_value  [ref];
    if (rflags_p != NULL) *rflags_p = c_rflags [ref];
    if (tag_p    != NULL) *tag_p    = FreshFlag(ref);

    return 0;
}

static void inserver_bigcreq_callback(int bigchan __attribute__((unused)), void *closure)
{
  inserver_bigcrequest_holder_t *hldr = closure;
  
    hldr->is_used = 0;
}
static void inserver_bigcreq_on_stop (int bigchan __attribute__((unused)), void *closure)
{
  inserver_bigcrequest_holder_t *hldr = closure;
  
    DelBigcRequest(&(hldr->bigclistitem));
    hldr->is_used = 0;
}
int  inserver_req_bigc          (int                        devid,
                                 inserver_bigcref_t         ref,
                                 int32  args[], int     nargs,
                                 void  *data,   size_t  datasize, size_t  dataunits,
                                 int    cachectl)
{
  int                            hid;
  inserver_bigcrequest_holder_t *hldr;

    if (ref < 0  ||  ref >= numbigcs  ||
        d_state[b_data[ref].devid] == DEVSTATE_OFFLINE)
    {
        return -1;
    }

    /*!!! Should perform parameter checks here!!!  */

    for (hid = 0, hldr = &(d_bigcrq_holders[devid][0]);
         hid < countof(d_bigcrq_holders[0]);
         hid++,   hldr++)
        if (hldr->is_used == 0)
        {
            if (AddBigcRequest(ref, &(hldr->bigclistitem),
                               inserver_bigcreq_callback, hldr,
                               args, nargs,
                               data, datasize, dataunits,
                               cachectl) == 0)
            {
                hldr->bigclistitem.on_stop = inserver_bigcreq_on_stop;
                hldr->is_used = 1;
                return 0;
            }
            else return -1;
        }
    
    return -1;
}

int  inserver_get_bigc_data     (int                        devid,
                                 inserver_bigcref_t         ref,
                                 size_t  ofs, size_t  size, void *buf,
                                 size_t *retdataunits_p)
{
  bigchaninfo_t   *bp = b_data + ref;
  uint8           *data_p;

    if (ref < 0  ||  ref >= numbigcs)
    {
        return -1;
    }

    if (retdataunits_p != NULL) *retdataunits_p = bp->cur_retdataunits;

    if (ofs >= bp->cur_retdatasize) return 0;
    if (ofs + size > bp->cur_retdatasize) size = bp->cur_retdatasize - ofs;
    
    data_p = GET_BIGBUF_PTR(bp->cur_retdata_o, void);
    memcpy(buf, data_p + ofs, size);
    
    return size;
}

int  inserver_get_bigc_stats    (int                        devid,
                                 inserver_bigcref_t         ref,
                                 tag_t *tag_p, rflags_t *rflags_p)
{
  bigchaninfo_t   *bp = b_data + ref;

    if (ref < 0  ||  ref >= numbigcs)
    {
        return -1;
    }

    if (tag_p    != NULL) *tag_p    = age_of(bp->cur_time);
    if (rflags_p != NULL) *rflags_p =        bp->cur_rflags;

    return 0;
}

int  inserver_get_bigc_params   (int                        devid,
                                 inserver_bigcref_t         ref,
                                 int  start, int  count, int32 *params)
{
  bigchaninfo_t   *bp = b_data + ref;
  int32           *prms_p;

    if (ref < 0  ||  ref >= numbigcs)
    {
        return -1;
    }

    if (start < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (start >= bp->cur_ninfo) return 0;
    if (start + count > bp->cur_ninfo) count = bp->cur_ninfo - start;

    prms_p = GET_BIGBUF_PTR(bp->cur_info_o, void);
    memcpy(params, prms_p + start, count * sizeof(*params));
    
    return count;
}

static void inserver_devs_callback(int mgid, devs_cbitem_t *cbrec,
                                   int newstate)
{
  inserver_devstat_evprocinfo_t *info = cbrec->privptr;
  inserver_devstat_evproc_t      proc;
  int                            devid;
  void                          *privptr;

    proc    = info->proc;
    devid   = info->devid;
    privptr = info->privptr;

    /*
     * Deactivate in case of "fatal" change.
     * Note: it deactivates first, than calls.
     */
    if (newstate == DEVSTATE_OFFLINE  ||  newstate < 0)
    {
        info->is_used = 0;
        DelDevsOnchgCallback(mgid, cbrec);
    }

    proc(devid, d_devptr[info->devid], mgid, newstate, privptr);
         
}
int  inserver_add_devstat_evproc(int                        devid,
                                 inserver_devref_t          ref,
                                 inserver_devstat_evproc_t  evproc,
                                 void                      *privptr)
{
  int                            evid;
  inserver_devstat_evprocinfo_t *info;

    CHECK_SANITY_OF_DEVID(-1);

    if (ref < 0  ||  ref >= numdevs    ||
        d_state[ref] == DEVSTATE_OFFLINE  ||
        evproc == NULL)
    {
        logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d, ref=%d, evproc=%p)!",
                __FUNCTION__, devid, active_dev, ref, evproc);

        return -1;
    }

    /* Check if such evproc is already registered */
    for (evid = 0, info = &(d_devstat_events[devid][0]);
         evid < countof(d_devstat_events[0]);
         evid++,   info++)
        if (info->is_used            &&
            info->proc    == evproc  &&
            info->privptr == privptr)
        {
            /*!!! Note: that's a DUPLICATE registration... */
            return 0;
        }

    /* Find a free slot */
    for (evid = 0, info = &(d_devstat_events[devid][0]);
         evid < countof(d_devstat_events[0]);
         evid++,   info++)
        if (info->is_used == 0)
        {
            if (AddDevsOnchgCallback(ref, &(info->cbitem)) == 0)
            {
                info->is_used = 1;
                
                info->devid   = devid;
                info->ref     = ref;
                info->proc    = evproc;
                info->privptr = privptr;

                info->cbitem.callback = inserver_devs_callback;
                info->cbitem.privptr  = info;

                return 0;
            }
            else
                return -1;
        }
    
    logline(LOGF_DBG1, 0, "%s(devid=%d/active=%d): failed to find free inserver-event slot",
            __FUNCTION__, devid, active_dev);
    return -1;

}

int  inserver_del_devstat_evproc(int                        devid, 
                                 inserver_devref_t          ref,
                                 inserver_devstat_evproc_t  evproc,
                                 void                      *privptr)
{
  int                            evid;
  inserver_devstat_evprocinfo_t *info;

    CHECK_SANITY_OF_DEVID(-1);

    if (ref < 0  ||  ref >= numdevs)
    {
        return -1;
    }

    for (evid = 0, info = &(d_devstat_events[devid][0]);
         evid < countof(d_devstat_events[0]);
         evid++,   info++)
        if (info->is_used            &&
            info->ref     == ref     &&
            info->proc    == evproc  &&
            info->privptr == privptr)
        {
            /*!!!!!!!!!!!!!!!!!*/
            if (DelDevsOnchgCallback(ref, &(info->cbitem)) == 0)
            {
                info->is_used = 0;

                return 0;
            }
            else
                return -1;
        }

    return -1;
}

static void inserver_chan_callback(int chan, chan_cbitem_t *cbrec)
{
  inserver_chanref_evprocinfo_t *info = cbrec->privptr;

    info->proc(info->devid, d_devptr[info->devid],
               chan,
               0,
               info->privptr);
}
static void inserver_chan_on_stop (int chan, chan_cbitem_t *cbrec)
{
  inserver_chanref_evprocinfo_t *info = cbrec->privptr;

    inserver_del_chanref_evproc(info->devid, info->ref, info->proc, info->privptr);
}
int  inserver_add_chanref_evproc(int                        devid,
                                 inserver_chanref_t         ref,
                                 inserver_chanref_evproc_t  evproc,
                                 void                      *privptr)
{
  int                            evid;
  inserver_chanref_evprocinfo_t *info;

    CHECK_SANITY_OF_DEVID(-1);

    if (ref < 0  ||  ref >= numchans  ||
        d_state[c_devid[ref]] == DEVSTATE_OFFLINE  ||
        evproc == NULL)
    {
        logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d, ref=%d, evproc=%p)!",
                __FUNCTION__, devid, active_dev, ref, evproc);
        return -1;
    }

    /* Check if such evproc is already registered */
    for (evid = 0, info = &(d_chanref_events[devid][0]);
         evid < countof(d_chanref_events[0]);
         evid++,   info++)
        if (info->is_used            &&
            info->proc    == evproc  &&
            info->privptr == privptr)
        {
            /*!!! Note: that's a DUPLICATE registration... */
            return 0;
        }

    /* Find a free slot */
    for (evid = 0, info = &(d_chanref_events[devid][0]);
         evid < countof(d_chanref_events[0]);
         evid++,   info++)
        if (info->is_used == 0)
        {
            if (AddChanOnretCallback(ref, &(info->cbitem)) == 0)
            {
                info->is_used = 1;
                
                info->devid   = devid;
                info->ref     = ref;
                info->proc    = evproc;
                info->privptr = privptr;
                
                info->cbitem.callback = inserver_chan_callback;
                info->cbitem.on_stop  = inserver_chan_on_stop;
                info->cbitem.privptr  = info;

                return 0;
            }
            else
                return -1;
        }
    
    logline(LOGF_DBG1, 0, "%s(devid=%d/active=%d): failed to find free inserver-event slot",
            __FUNCTION__, devid, active_dev);
    return -1;
}

int  inserver_del_chanref_evproc(int                        devid,
                                 inserver_chanref_t         ref,
                                 inserver_chanref_evproc_t  evproc,
                                 void                      *privptr)
{
  int                            evid;
  inserver_chanref_evprocinfo_t *info;

    CHECK_SANITY_OF_DEVID(-1);

    for (evid = 0, info = &(d_chanref_events[devid][0]);
         evid < countof(d_chanref_events[0]);
         evid++,   info++)
        if (info->is_used            &&
            info->ref     == ref     &&
            info->proc    == evproc  &&
            info->privptr == privptr)
        {
            /*!!!!!!!!!!!!!!!!!*/
            if (DelChanOnretCallback(ref, &(info->cbitem)) == 0)
            {
                info->is_used = 0;

                return 0;
            }
            else
                return -1;
        }

    return -1;
}

static void inserver_bigc_callback(int bigc, bigc_cbitem_t *cbrec)
{
  inserver_bigcref_evprocinfo_t *info = cbrec->privptr;

    info->proc(info->devid, d_devptr[info->devid],
               bigc,
               0,
               info->privptr);
}
static void inserver_bigc_on_stop (int bigc, bigc_cbitem_t *cbrec)
{
  inserver_bigcref_evprocinfo_t *info = cbrec->privptr;

    inserver_del_bigcref_evproc(info->devid, info->ref, info->proc, info->privptr);
}
int  inserver_add_bigcref_evproc(int                        devid,
                                 inserver_bigcref_t         ref,
                                 inserver_bigcref_evproc_t  evproc,
                                 void                      *privptr)
{
  int                            evid;
  inserver_bigcref_evprocinfo_t *info;

    CHECK_SANITY_OF_DEVID(-1);

    if (ref < 0  ||  ref >= numchans  ||
        d_state[b_data[ref].devid] == DEVSTATE_OFFLINE  ||
        evproc == NULL)
    {
        logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d, ref=%d, evproc=%p)!",
                __FUNCTION__, devid, active_dev, ref, evproc);
        return -1;
    }

    /* Check if such evproc is already registered */
    for (evid = 0, info = &(d_bigcref_events[devid][0]);
         evid < countof(d_bigcref_events[0]);
         evid++,   info++)
        if (info->is_used            &&
            info->proc    == evproc  &&
            info->privptr == privptr)
        {
            /*!!! Note: that's a DUPLICATE registration... */
            return 0;
        }

    /* Find a free slot */
    for (evid = 0, info = &(d_bigcref_events[devid][0]);
         evid < countof(d_bigcref_events[0]);
         evid++,   info++)
        if (info->is_used == 0)
        {
            if (AddBigcOnretCallback(ref, &(info->cbitem)) == 0)
            {
                info->is_used = 1;
                
                info->devid   = devid;
                info->ref     = ref;
                info->proc    = evproc;
                info->privptr = privptr;
                
                info->cbitem.callback = inserver_bigc_callback;
                info->cbitem.on_stop  = inserver_bigc_on_stop;
                info->cbitem.privptr  = info;

                return 0;
            }
            else
                return -1;
        }
    
    logline(LOGF_DBG1, 0, "%s(devid=%d/active=%d): failed to find free inserver-event slot",
            __FUNCTION__, devid, active_dev);
    return -1;
}

int  inserver_del_bigcref_evproc(int                        devid,
                                 inserver_bigcref_t         ref,
                                 inserver_bigcref_evproc_t  evproc,
                                 void                      *privptr)
{
  int                            evid;
  inserver_bigcref_evprocinfo_t *info;

    CHECK_SANITY_OF_DEVID(-1);

    for (evid = 0, info = &(d_bigcref_events[devid][0]);
         evid < countof(d_bigcref_events[0]);
         evid++,   info++)
        if (info->is_used            &&
            info->ref     == ref     &&
            info->proc    == evproc  &&
            info->privptr == privptr)
        {
            /*!!!!!!!!!!!!!!!!!*/
            if (DelBigcOnretCallback(ref, &(info->cbitem)) == 0)
            {
                info->is_used = 0;

                return 0;
            }
            else
                return -1;
        }

    return -1;
}


/* 2nd layer on top of "inserver" */

enum
{
    CHAN_REF_VAL_NOT_SUFFERING = 0,
    CHAN_REF_VAL_IS_SUFFERING  = INSERVER_CHANREF_ERROR,
};

static inline int isletnum(int c)
{
    return isalnum(c)  ||  c == '_'  ||  c == '-';
}
int  inserver_parse_d_c_ref(int devid, const char *what,
                            const char **pp, inserver_refmetric_t *m_p)
{
  const char *p = *pp;
  const char *beg_p;
  size_t      len;
  char        chanbuf[10];

    while (*p != '\0'  &&  isspace(*p)) p++;

    /* Device-instance name */
    beg_p = p;
    while (*p != '\0'  &&  isletnum(*p)) p++;
    len = p - beg_p;
    if (len == 0)
    {
        DoDriverLog(devid, 0, "device-name for %s spec expected", what);
        return -CXRF_CFG_PROBL;
    }
    if (len > sizeof(m_p->targ_name) - 1)
        len = sizeof(m_p->targ_name) - 1;
    memcpy(m_p->targ_name, beg_p, len); m_p->targ_name[len] = '\0';

    /* Separating dot */
    if (*p != '.')
    {
        DoDriverLog(devid, 0, "'.' expected after device-name for %s spec", what);
        return -CXRF_CFG_PROBL;
    }
    p++;

    /* Channel number */
    beg_p = p;
    while (*p != '\0'  &&  isdigit(*p)) p++;
    len = p - beg_p;
    if (len == 0)
    {
        DoDriverLog(devid, 0, "channel-number expected after '.' for %s spec", what);
        return -CXRF_CFG_PROBL;
    }
    if (len > sizeof(chanbuf) - 1)
        len = sizeof(chanbuf) - 1;
    memcpy(chanbuf, beg_p, len); chanbuf[len] = '\0';
    m_p->chan_n = atoi(chanbuf);

    *pp = p;
    return 0;
}
int  inserver_d_c_ref_plugin_parser(const char *str, const char **endptr,
                                    void *rec, size_t recsize __attribute__((unused)),
                                    const char *separators __attribute__((unused)), const char *terminators __attribute__((unused)),
                                    void *privptr, char **errstr)
{
  const char           *what = privptr;
  inserver_refmetric_t *m_p  = rec;
  int                   parse_r;

    if (what == NULL) what = "???";

    /* Initialize to "none" */
    m_p->targ_name[0] = '\0';
    m_p->chan_n       = -1;

    if (str != NULL)
    {
        parse_r = inserver_parse_d_c_ref(DEVID_NOT_IN_DRIVER, what,
                                         &str, m_p);
        if (parse_r != 0)
        {
            *errstr = "inserver_parse_d_c_ref() returned error";
            return PSP_R_USRERR;
        }
        *endptr = str;
    }

    return PSP_R_OK;
}

static void inserver2_stat_evproc(int                 devid, void *devptr,
                                  inserver_devref_t   ref,
                                  int                 newstate,
                                  void               *privptr)
{
  inserver_refmetric_t *m_p = privptr;

    DoDriverLog(devid, DRIVERLOG_DEBUG | DRIVERLOG_C_ENTRYPOINT,
                "%s(%s.%d)->%d",
               __FUNCTION__, m_p->targ_name, m_p->chan_n, newstate);
    if (newstate == DEVSTATE_OFFLINE)
    {
        m_p->targ_ref = INSERVER_DEVREF_ERROR;
        m_p->chan_ref = INSERVER_CHANREF_ERROR;
        /* We rely on server to release all inserver* resources */
    }
#if INSERVER2_STAT_CB_PRESENT
    if (m_p->stat_cb != NULL)
    {
        m_p->stat_cb(devid, devptr,
                     m_p->targ_ref,
                     newstate,
                     m_p->privptr);
    }
#endif
}

static void inserver2_try_to_bind(int devid, void *devptr,
                                  inserver_refmetric_t *m_p)
{
  inserver_chanref_t  prev_chan_ref = m_p->chan_ref;

    m_p->targ_ref = inserver_get_devref  (devid, m_p->targ_name);
    if (m_p->targ_ref == INSERVER_DEVREF_ERROR)
    {
        if (prev_chan_ref == CHAN_REF_VAL_NOT_SUFFERING)
        {
            DoDriverLog(devid, 0, "target dev \"%s\" not found", m_p->targ_name);
            m_p->chan_ref = CHAN_REF_VAL_IS_SUFFERING;
        }
        return;
    }

    if (!m_p->chan_is_bigc)
        m_p->chan_ref = inserver_get_chanref(devid, m_p->targ_name, m_p->chan_n);
    else
        m_p->chan_ref = inserver_get_bigcref(devid, m_p->targ_name, m_p->chan_n);
    if (m_p->chan_ref == INSERVER_CHANREF_ERROR)
    {
        if (prev_chan_ref == CHAN_REF_VAL_NOT_SUFFERING)
        {
            DoDriverLog(devid, 0, "invalid target %s %s.%d",
                        m_p->chan_is_bigc? "bigc" : "chan",
                        m_p->targ_name, m_p->chan_n);
            m_p->chan_ref = CHAN_REF_VAL_IS_SUFFERING;
        }
        m_p->targ_ref = INSERVER_DEVREF_ERROR;
        return;
    }

    DoDriverLog(devid, DRIVERLOG_DEBUG | DRIVERLOG_C_DEFAULT,
                "%s(%s.%d): targ=%d chan=%d",
               __FUNCTION__, m_p->targ_name, m_p->chan_n, m_p->targ_ref, m_p->chan_ref);

    inserver_add_devstat_evproc    (devid, m_p->targ_ref,  inserver2_stat_evproc, m_p);
    if (m_p->data_cb != NULL)
    {
        if (!m_p->chan_is_bigc)
            inserver_add_chanref_evproc(devid, m_p->chan_ref, m_p->data_cb, m_p->privptr);
        else
            inserver_add_bigcref_evproc(devid, m_p->chan_ref, m_p->data_cb, m_p->privptr);
    }
#if INSERVER2_STAT_CB_PRESENT
    if (m_p->stat_cb != NULL)
    {
        m_p->stat_cb(devid, devptr,
                     m_p->targ_ref,
                     d_state[m_p->targ_ref],
                     m_p->privptr);
    }
#else
    if (!m_p->chan_is_bigc)
        inserver_req_cval(devid, m_p->chan_ref);
    else
        inserver_req_bigc(devid, m_p->chan_ref,
                          NULL, 0,
                          NULL, 0, 0,
                          CX_CACHECTL_SHARABLE);
#endif
}

static void inserver2_bind_hbt(int devid, void *devptr,
                               sl_tid_t tid,
                               void *privptr)
{
  inserver_refmetric_t *f_p = privptr;
  inserver_refmetric_t *m_p = f_p;
  int                   num = f_p->m_count;

    f_p->bind_hbt_tid = -1;
    do
    {
        if (m_p->targ_ref == INSERVER_DEVREF_ERROR  &&  m_p->targ_name[0] != '\0')
            inserver2_try_to_bind(devid, devptr, m_p);
        m_p++;
        num--;
    }
    while (num > 0);

    f_p->bind_hbt_tid = sl_enq_tout_after(devid, devptr, f_p->bind_hbt_period, inserver2_bind_hbt, privptr);
}

void inserver_new_watch_list(int  devid, inserver_refmetric_t *m_p, int bind_hbt_period)
{
  inserver_refmetric_t *c_p = m_p;
  int                   num = m_p->m_count;

    do
    {
        c_p->targ_ref        = INSERVER_DEVREF_ERROR;
        c_p->chan_ref        = CHAN_REF_VAL_NOT_SUFFERING;
        c_p->devid           = devid;
        c_p->bind_hbt_period = bind_hbt_period;
        c_p++;
        num--;
    }
    while (num > 0);
    inserver2_bind_hbt(devid, NULL, -1, m_p);
}


/*
 *  List of symbols which aren't used by server,
 *  but need to be present for drivers.
 */

#include "misclib.h"
void *public_funcs_list [] =
{
    set_fd_flags,
    sq_init,
    n_read,
    n_write,
};
