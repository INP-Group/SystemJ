#include "cxsd_lib_includes.h"


//////////////////////////////////////////////////////////////////////

void        SetDevState      (int devid, int state,
                              rflags_t rflags_to_set, const char *description)
{
}

void        ReRequestDevData (int devid)
{
}


void        DoDriverLog      (int devid, int level,
                              const char *format, ...)
{
  va_list        ap;
  
    va_start(ap, format);
    vDoDriverLog(devid, level, format, ap);
    va_end(ap);
}

/*!!!This block doesn't belong here!!!*/
//////////////////////////////////////////////////////////////////////
typedef struct
{
    const char *name;
    int         c;
} catdesc_t;

/* Note: this list should be kept in sync/order with
   DRIVERLOG_CN_ enums in cxsd_driver.h */
static catdesc_t  catlist[] =
{
    {"DEFAULT",           DRIVERLOG_C_DEFAULT},
    {"ENTRYPOINT",        DRIVERLOG_C_ENTRYPOINT},
    {"PKTDUMP",           DRIVERLOG_C_PKTDUMP},
    {"PKTINFO",           DRIVERLOG_C_PKTINFO},
    {"DATACONV",          DRIVERLOG_C_DATACONV},
    {"REMDRV_PKTDUMP",    DRIVERLOG_C_REMDRV_PKTDUMP},
    {"REMDRV_PKTINFO",    DRIVERLOG_C_REMDRV_PKTINFO},
    {NULL,         0}
};

/*!!! a temporary measure, since god knows into which .h to place declaration */
static const char *GetDrvlogCatName(int category)
{
    category = (category & DRIVERLOG_C_mask) >> DRIVERLOG_C_shift;
    if (category > DRIVERLOG_CN_default  ||  category < countof(catlist) - 1)
        return catlist[category].name;
    else
        return "???";
}
//////////////////////////////////////////////////////////////////////
void        vDoDriverLog     (int devid, int level,
                              const char *format, va_list ap)
{
#if 1
  int         log_type = LOGF_HARDWARE;
  char        subaddress[200];
  int         category = level & DRIVERLOG_C_mask;
  const char *catname;
  cxsd_hw_dev_t    *dev_p;
  cxsd_hw_lyr_t    *lyr_p;

    if (level & DRIVERLOG_ERRNO) log_type |= LOGF_ERRNO;

    if      (devid > 0  &&   devid < cxsd_hw_numdevs)
    {
        dev_p = cxsd_hw_devices + devid;
        if (category != DRIVERLOG_C_DEFAULT        &&
            category <= DRIVERLOG_C__max_allowed_  &&
            !DRIVERLOG_m_C_ISIN_CHECKMASK(category, dev_p->logmask))
            return;

        catname = GetDrvlogCatName(category);
        snprintf(subaddress, sizeof(subaddress),
                 "%s[%d]%s%s%s",
                 dev_p->db_ref->typename, devid, dev_p->db_ref->instname,
                 catname[0] != '\0'? "/" : "", catname);
    }
    else if (devid < 0  &&  -devid < cxsd_hw_numlyrs)
    {
        lyr_p = cxsd_hw_layers + -devid;
        if (category != DRIVERLOG_C_DEFAULT        &&
            category <= DRIVERLOG_C__max_allowed_  &&
            !DRIVERLOG_m_C_ISIN_CHECKMASK(category, lyr_p->logmask))
            return;

        catname = GetDrvlogCatName(category);
        snprintf(subaddress, sizeof(subaddress),
                 "lyr:%s[%d]%s%s",
                 lyr_p->lyrname, devid,
                 catname[0] != '\0'? "/" : "", catname);
    }
    else /* DEVID_NOT_IN_DRIVER or some overflow/illegal... */
    {
        if (category != DRIVERLOG_C_DEFAULT        &&
            category <= DRIVERLOG_C__max_allowed_  &&
            !DRIVERLOG_m_C_ISIN_CHECKMASK(category, cxsd_hw_defdrvlog_mask))
            return;

        catname = GetDrvlogCatName(category);
        snprintf(subaddress, sizeof(subaddress),
                 "NOT-IN-DRIVER=%d%s%s",
                 devid, catname[0] != '\0'? "/" : "", catname);
    }

    vloglineX(log_type, level & DRIVERLOG_LEVEL_mask, subaddress, format, ap);
#else
    fprintf (stderr, "%s: ", strcurtime());
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
}


int         RegisterDevPtr   (int devid, void *devptr)
{
    CHECK_SANITY_OF_DEVID(-1);

    /*!!! Should we forbid replacing devptr when privrecsize>0?  */
    cxsd_hw_devices[devid].devptr = devptr;
    
    return 0;
}


#define MAY_USE_INT64 1
#define MAY_USE_FLOAT 1
void ReturnDataSet    (int devid,
                       int count,
                       int *addrs, cxdtype_t *dtypes, int *nelems,
                       void **values, rflags_t *rflags, cx_time_t *timestamps)
{
  cxsd_hw_dev_t  *dev = cxsd_hw_devices + devid;

  struct timeval  timenow;
  cx_time_t       timestamp;
  
  int             x;
  int             chan;
  int             globalchan;
  cxsd_hw_chan_t *chn_p;
  int             nels;
  uint8          *src;
  uint8          *dst;

  int             srpr;
  size_t          ssiz;
  int             repr;
  size_t          size;

  int32           iv32;
#if MAY_USE_INT64
  int64           iv64;
#endif
#if MAY_USE_FLOAT
  float64         fv64;
#endif

  CxsdHwChanEvCallInfo_t  call_info;

    CHECK_SANITY_OF_DEVID();

    /* Check the 'count' */
    if (count == 0) return;
    if (count < 0  ||  count > dev->count)
    {
        logline(LOGF_MODULES, LOGL_WARNING,
                "%s(devid=%d/active=%d): count=%d, out_of[1...dev->count=%d]",
                __FUNCTION__, devid, active_devid, count, dev->count);
        return;
    }

    gettimeofday(&timenow, NULL);
    timestamp.sec  = timenow.tv_sec;
    timestamp.nsec = timenow.tv_usec * 1000;

    /* I. Update */
    for (x = 0;  x < count;  x++)
    {
        /* Get pointer to the channel in question */
        chan = addrs[x];
        if (chan < 0  ||  chan > dev->count)
        {
            logline(LOGF_MODULES, LOGL_WARNING,
                    "%s(devid=%d/active=%d): addrs[%d]=%d, out_of[0...dev->count=%d)",
                    __FUNCTION__, devid, active_devid, x, chan, dev->count);
            goto NEXT_TO_UPDATE;
        }
        globalchan = chan + dev->first;
        chn_p = cxsd_hw_channels + globalchan;

        /* Check nelems */
        nels = nelems[x];
        if      (nels < 0)
        {
            logline(LOGF_MODULES, LOGL_WARNING,
                    "%s(devid=%d/active=%d): negative nelems[%d:%d]=%d",
                    __FUNCTION__, devid, active_devid, x, chan, nels);
            goto NEXT_TO_UPDATE;
        }
        else if (chn_p->nelems == 1  &&  nels != 1)
        {
            logline(LOGF_MODULES, LOGL_WARNING,
                    "%s(devid=%d/active=%d): nelems[%d:%d]=%d, should be =1 for scalar",
                    __FUNCTION__, devid, active_devid, x, chan, nels);
            goto NEXT_TO_UPDATE;
        }
        else if (nels > chn_p->nelems)
        {
            /* Too many?  Okay, just limit */
            logline(LOGF_MODULES, LOGL_INFO,
                    "%s(devid=%d/active=%d): nelems[%d:%d]=%d, truncating to limit=%d",
                    __FUNCTION__, devid, active_devid, x, chan, nels, chn_p->nelems);
            nels = chn_p->nelems;
        }

        /* Store data */
#if 1
        srpr = reprof_cxdtype(dtypes[x]);
        ssiz = sizeof_cxdtype(dtypes[x]);
        repr = reprof_cxdtype(chn_p->dtype);
        size = sizeof_cxdtype(chn_p->dtype);

        /* a. Identical */
        if      (srpr == repr  &&  ssiz == size)
        {
            chn_p->current_nelems = nels;
            if (nels > 0)
                memcpy(chn_p->current_val, values[x], nels * chn_p->usize);
        }
        /* b. Integer */
        else if (srpr == CXDTYPE_REPR_INT  &&  repr == CXDTYPE_REPR_INT)
        {
            chn_p->current_nelems = nels;
            src = values[x];
            dst = chn_p->current_val;
#if MAY_USE_INT64
            if (ssiz == sizeof(int64)  ||  size == sizeof(int64))
                while (nels > 0)
                {
                    // Read datum, converting to int64
                    switch (dtypes[x])
                    {
                        case CXDTYPE_INT32:  iv64 = *((  int32*)src);     break;
                        case CXDTYPE_UINT32: iv64 = *(( uint32*)src);     break;
                        case CXDTYPE_INT16:  iv64 = *((  int16*)src);     break;
                        case CXDTYPE_UINT16: iv64 = *(( uint16*)src);     break;
                        case CXDTYPE_INT64:
                        case CXDTYPE_UINT64: iv64 = *(( uint64*)src);     break;
                        case CXDTYPE_INT8:   iv64 = *((  int8 *)src);     break;
                        default:/*   UINT8*/ iv64 = *(( uint8 *)src);     break;
                    }
                    src += ssiz;

                    // Store datum, converting from int64
                    switch (chn_p->dtype)
                    {
                        case CXDTYPE_INT32:
                        case CXDTYPE_UINT32:      *((  int32*)dst) = iv64; break;
                        case CXDTYPE_INT16:
                        case CXDTYPE_UINT16:      *((  int16*)dst) = iv64; break;
                        case CXDTYPE_INT64:
                        case CXDTYPE_UINT64:      *(( uint64*)dst) = iv64; break;
                        default:/*   *INT8*/      *((  int8 *)dst) = iv64; break;
                    }
                    dst += size;

                    nels--;
                }
            else
#endif
            while (nels > 0)
            {
                // Read datum, converting to int32
                switch (dtypes[x])
                {
                    case CXDTYPE_INT32:
                    case CXDTYPE_UINT32: iv32 = *((  int32*)src);     break;
                    case CXDTYPE_INT16:  iv32 = *((  int16*)src);     break;
                    case CXDTYPE_UINT16: iv32 = *(( uint16*)src);     break;
                    case CXDTYPE_INT8:   iv32 = *((  int8 *)src);     break;
                    default:/*   UINT8*/ iv32 = *(( uint8 *)src);     break;
                }
                src += ssiz;

                // Store datum, converting from int32
                switch (chn_p->dtype)
                {
                    case CXDTYPE_INT32:
                    case CXDTYPE_UINT32:      *((  int32*)dst) = iv32; break;
                    case CXDTYPE_INT16:
                    case CXDTYPE_UINT16:      *((  int16*)dst) = iv32; break;
                    default:/*   *INT8*/      *((  int8 *)dst) = iv32; break;
                }
                dst += size;

                nels--;
            }
        }
#if MAY_USE_FLOAT
        /* c. Float: float->float, float->int, int->float */
        else if ((srpr == CXDTYPE_REPR_FLOAT  ||  srpr == CXDTYPE_REPR_INT)  &&
                 (repr == CXDTYPE_REPR_FLOAT  ||  repr == CXDTYPE_REPR_INT))
        {
            chn_p->current_nelems = nels;
            src = values[x];
            dst = chn_p->current_val;
            while (nels > 0)
            {
                // Read datum, converting to float64 (double)
                switch (dtypes[x])
                {
                    case CXDTYPE_INT32:  fv64 = *((  int32*)src);     break;
                    case CXDTYPE_UINT32: fv64 = *(( uint32*)src);     break;
                    case CXDTYPE_INT16:  fv64 = *((  int16*)src);     break;
                    case CXDTYPE_UINT16: fv64 = *(( uint16*)src);     break;
                    case CXDTYPE_DOUBLE: fv64 = *((float64*)src);     break;
                    case CXDTYPE_SINGLE: fv64 = *((float32*)src);     break;
                    case CXDTYPE_INT64:  fv64 = *((  int64*)src);     break;
                    case CXDTYPE_UINT64: fv64 = *(( uint64*)src);     break;
                    case CXDTYPE_INT8:   fv64 = *((  int8 *)src);     break;
                    default:/*   UINT8*/ fv64 = *(( uint8 *)src);     break;
                }
                src += ssiz;

                // Store datum, converting from float64 (double)
                switch (chn_p->dtype)
                {
                    case CXDTYPE_INT32:       *((  int32*)dst) = fv64; break;
                    case CXDTYPE_UINT32:      *(( uint32*)dst) = fv64; break;
                    case CXDTYPE_INT16:       *((  int16*)dst) = fv64; break;
                    case CXDTYPE_UINT16:      *(( uint16*)dst) = fv64; break;
                    case CXDTYPE_DOUBLE:      *((float64*)dst) = fv64; break;
                    case CXDTYPE_SINGLE:      *((float32*)dst) = fv64; break;
                    case CXDTYPE_INT64:       *((  int64*)dst) = fv64; break;
                    case CXDTYPE_UINT64:      *(( uint64*)dst) = fv64; break;
                    case CXDTYPE_INT8:        *((  int8 *)dst) = fv64; break;
                    default:/*   UINT8*/      *((  int8 *)dst) = fv64; break;
                }
                dst += size;

                nels--;
            }
        }
#endif
        /* d. Incompatible */
        else
        {
            chn_p->current_nelems = chn_p->nelems;
            bzero(chn_p->current_val, chn_p->nelems * chn_p->usize);
        }
#else
        /*!!!DATACONV*/
        if (dtypes[x] != chn_p->dtype)
        {
            /* No data conversion for now... */
            chn_p->current_nelems = chn_p->nelems;
            bzero(chn_p->current_val, chn_p->nelems * chn_p->usize);
        }
        else
        {
            chn_p->current_nelems = nels;
            memcpy(chn_p->current_val, values[x], nels * chn_p->usize);
        }
#endif

        chn_p->crflags |= (chn_p->rflags = rflags[x]);
        /* Timestamp (with appropriate copying, of requested) */
        if (timestamps != NULL  &&  timestamps[x].sec != 0  &&
            chn_p->timestamp_cn <= 0)
            chn_p->timestamp = timestamps[x];
        else
            chn_p->timestamp =
                (chn_p->timestamp_cn <= 0)? timestamp
                                          : cxsd_hw_channels[chn_p->timestamp_cn].timestamp;

 NEXT_TO_UPDATE:;
    }

    /* II. Call who requested */
    call_info.reason = CXSD_HW_CHAN_R_UPDATE;
    call_info.evmask = 1 << call_info.reason;

    for (x = 0;  x < count;  x++)
    {
        /* Get pointer to the channel in question */
        chan = addrs[x];
        if (chan < 0  ||  chan > dev->count)
            goto NEXT_TO_CALL;
        globalchan = chan + dev->first;

        CxsdHwCallChanEvprocs(globalchan, &call_info);
 NEXT_TO_CALL:;
    }
}


void SetChanProps     (int devid,
                       int count,
                       int *addrs, cxdtype_t *dtypes, int *nelems,
                       void **min_alwds, void **max_alwds, void **quants,
                       int *fresh_ages)
{
}


void StdSimulated_rw_p(int devid, void *devptr __attribute__((unused)),
                       int action,
                       int count, int *addrs,
                       cxdtype_t *dtypes, int *nelems, void **values)
{
  int              n;
  int              x;
  int              g;
  cxdtype_t        dt;
  void            *nvp;
  int              nel;
  int              cycle = CxsdHwGetCurCycle();
  
  struct
  {
      int8         i8;
      int16        i16;
      int32        i32;
      int64        i64;

#if 1 /*!!!*/
      float        f32;
      double       f64;
#endif
  
      int8         t8;
      int32        t32;
  }                v;

  static rflags_t  zero_rflags = 0;

    if (devid < 0)
    {
        logline(LOGF_MODULES, DRIVERLOG_WARNING,
                "%s(devid=%d/active=%d): attempt to call as-if-from layer",
                __FUNCTION__, devid, active_devid);
        return;
    }
    CHECK_SANITY_OF_DEVID();

    for (n = 0;  n < count;  n++)
    {
        x = addrs[n];
        g = x + cxsd_hw_devices[devid].first;

        bzero(&v, sizeof(v));
        
        dt = dtypes[n];
        switch (dt)
        {
            case CXDTYPE_INT8:   case CXDTYPE_UINT8:  nvp = &v.i8;  break;
            case CXDTYPE_INT16:  case CXDTYPE_UINT16: nvp = &v.i16; break;
            case CXDTYPE_INT32:  case CXDTYPE_UINT32: nvp = &v.i32; break;
            case CXDTYPE_INT64:  case CXDTYPE_UINT64: nvp = &v.i64; break;
            case CXDTYPE_SINGLE:                      nvp = &v.f32; break;
            case CXDTYPE_DOUBLE:                      nvp = &v.f64; break;
            case CXDTYPE_TEXT:                        nvp = &v.t8;  break;
            case CXDTYPE_UCTEXT:                      nvp = &v.t32; break;
            default:
                logline(LOGF_MODULES, DRIVERLOG_ERR,
                        "%s(devid=%d/active=%d): unrecognized dtypes[n=%d/chan=%d/global=%d]=%d",
                        __FUNCTION__, devid, active_devid, n, x, g, dt);
                return;
        }

        if      (action == DRVA_WRITE  &&  cxsd_hw_channels[g].rw)
        {
            nvp = values;
            nel = nelems[n];
        }
        else if (cxsd_hw_channels[g].rw)
        {
            /* That must be initial read-of-w-channels.
               We can't do anything with it, just init with 0. */
            nvp = cxsd_hw_channels[g].current_val;
            nel = cxsd_hw_channels[g].current_nelems;
        }
        else   /* Read of a readonly channel */
        {
            v.i8  = g        + cycle;
            v.i16 = g * 10   + cycle;
            v.i32 = g * 1000 + cycle;
            v.f32 = g + cycle / 1000;
            v.f64 = g + cycle / 1000;
            v.t8  = 'a' + ((g + cycle) % 26);
            v.t32 = 'A' + ((g + cycle) % 26);

            nel = 1;
        }

        ReturnDataSet(devid,
                      1,
                      &x, &dt, &nel,
                      &nvp, &zero_rflags, NULL);
    }
}

const char * GetDevTypename(int devid)
{
  cxsd_hw_dev_t  *dev = cxsd_hw_devices + devid;

    CHECK_SANITY_OF_DEVID(NULL);

    return dev->db_ref->typename;
}

void       * GetLayerVMT   (int devid)
{
  cxsd_hw_dev_t  *dev = cxsd_hw_devices + devid;

    CHECK_SANITY_OF_DEVID(NULL);

    if (dev->lyrid == 0)
    {
        logline(LOGF_MODULES, DRIVERLOG_ERR,
                "%s(devid=%d/active=%d): request for layer-VMT from non-layered device",
                __FUNCTION__, devid, active_devid);
        return NULL;
    }

    return cxsd_hw_layers[-dev->lyrid].metric->vmtlink;
}

const char * GetLayerInfo  (const char *lyrname, int bus_n)
{
  const char        *ret = NULL;
  int                n;
  CxsdDbLayerinfo_t *lio;

    for (n = 0,  lio = cxsd_hw_cur_db->liolist;
         n < cxsd_hw_cur_db->numlios;
         n++,    lio++)
        if (strcasecmp(lyrname, lio->lyrname) == 0)
        {
            if (bus_n == lio->bus_n
                ||
                (lio->bus_n < 0  && ret == NULL))
                ret = lio->info;
        }

    return ret;
}


int  cxsd_uniq_checker(const char *func_name, int uniq)
{
  int                  devid = uniq;

    if (uniq == 0  ||  uniq == DEVID_NOT_IN_DRIVER) return 0;

    DO_CHECK_SANITY_OF_DEVID(func_name, -1);

    return 0;
}
