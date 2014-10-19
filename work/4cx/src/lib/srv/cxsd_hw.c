#define __CXSD_HW_C

#include "cxsd_lib_includes.h"


static int             MustSimulateHardware = 0;

static int             basecyclesize        = 1000000;
static struct timeval  cycle_start;  /* The time when the current cycle had started */
static struct timeval  cycle_end;
static sl_tid_t        cycle_tid            = -1;
static int             current_cycle        = 0;
static int             cycle_pass_count     = -1;
static int             cycle_pass_count_lim = 10;

//////////////////////////////////////////////////////////////////////

static inline int CheckGlobalchan(int globalchan)
{
    if (globalchan > 0  ||  globalchan < cxsd_hw_numchans) return 0;

    errno = EBADF;
    return -1;
}

//--------------------------------------------------------------------

// GetChanCbSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, ChanCb, cxsd_hw_chan_cbrec_t,
                                 cb, evmask, 0, 1,
                                 0, 2, 0,
                                 chn_p->, chn_p,
                                 cxsd_hw_chan_t *chn_p, cxsd_hw_chan_t *chn_p)

static void RlsChanCbSlot(int id, cxsd_hw_chan_t *chn_p)
{
  cxsd_hw_chan_cbrec_t *p = AccessChanCbSlot(id, chn_p);

    p->evmask = 0;
}

//////////////////////////////////////////////////////////////////////

static void CxsdHwList(FILE *fp)
{
  int                lyr_n;
  int                devid;
  int                lio_n;

  cxsd_hw_lyr_t     *lyr_p;
  cxsd_hw_dev_t     *dev_p;

    for (lyr_n = 0,  lyr_p = cxsd_hw_layers + lyr_n;
         lyr_n < cxsd_hw_numlyrs;
         lyr_n++,    lyr_p++)
    {
        fprintf(fp, "layer[%d] <%s>\n", lyr_n, lyr_p->lyrname);
    }
    
    for (devid = 0,  dev_p = cxsd_hw_devices + devid;
         devid < cxsd_hw_numdevs;
         devid++,    dev_p++)
    {
        fprintf(fp, "dev[%d] \"%s\" <%s>@%d:<%s>; [%d,+%d)\n",
                devid, dev_p->db_ref->instname, dev_p->db_ref->typename,
                dev_p->lyrid, cxsd_hw_layers[-dev_p->lyrid].lyrname,
                dev_p->first, dev_p->count);
    }
}

static int chan_evproc_remover(cxsd_hw_chan_cbrec_t *p, void *privptr)
{
    /*!!! should call RlsChanCbSlot() somehow, but it requires "id" instead of "p" */
    p->evmask = 0;

    return 0;
}
int  CxsdHwSetDb   (CxsdDb db)
{
  CxsdDb             new_db;
  size_t             devlistsize;
  size_t             liolistsize;

  int                lyr_n;
  int                devid;
  int                lio_n;
  CxsdDbDevLine_t   *db_d;
  CxsdDbDevLine_t   *hw_d;
  CxsdDbLayerinfo_t *db_l;
  CxsdDbLayerinfo_t *hw_l;

  int                stage;
  int                g;
  int                x;

  cxsd_hw_lyr_t     *lyr_p;
  cxsd_hw_dev_t     *dev_p;
  CxsdChanInfoRec   *grp_p;
  cxsd_hw_chan_t    *chn_p;
  int                nchans;
  
  size_t             current_val_bufsize;
  size_t             next_wr_val_bufsize;
  size_t             usize;                // Size of data-units
  size_t             csize;                // Channel data size

  static CxsdDbDevLine_t zero_dev =
  {
      .is_builtin   = 1,
      
      .instname     = "!DEV-0!",
      .typename     = "!FAKE-0!",
      .drvname      = "",
      .lyrname      = "",

      .changrpcount = 1,
      .businfocount = 0,

      .changroups   =
      {
          {
              .count  = 100,           // Reserve first 100 channels
              .rw     = 0,
              .dtype  = CXDTYPE_INT32, // Let them be simplest -- int32
              .nelems = 1,
          }
      },
      .businfo      = {},

      .options      = NULL,
      .auxinfo      = NULL,
  };

    /* I. Replicate DB */

    /* 1. Allocate space */
    /* 1.0. For descriptor itself... */
    new_db = malloc(sizeof(*new_db));
    if (new_db == NULL) return -1;
    bzero(new_db, sizeof(*new_db));

    /* 1.1. ...and for list of devices... */
    new_db->numdevs = 1 + db->numdevs;
    devlistsize = sizeof(*(new_db->devlist)) * new_db->numdevs;
    new_db->devlist = malloc(devlistsize);
    if (new_db->devlist == NULL) goto CLEANUP;
    bzero(new_db->devlist, devlistsize);

    /* 1.2. ...and for layer-infos, if any */
    new_db->numlios = db->numlios;
    liolistsize = sizeof(*(new_db->liolist)) * new_db->numlios;
    new_db->liolist = malloc(liolistsize);
    if (new_db->liolist == NULL) goto CLEANUP;
    bzero(new_db->liolist, liolistsize);

    /* 2. Copy data */
    /* 2.1. Devices */
    /* 2.1.0. Starting a builtin zero-id-device */
    new_db->devlist[0] = zero_dev;
    /* 2.1.1. And than regular list */
    for (devid = 1,  db_d = db->devlist + 0,  hw_d = new_db->devlist + 1;
         devid < new_db->numdevs;
         devid++,    db_d++,                  hw_d++)
    {
        /* First, duplicate record... */
        *hw_d = *db_d;
        /* ...but null-out malloc()'d fields */
        hw_d->options = NULL;
        hw_d->auxinfo = NULL;

        /* Now duplicate strings, if required */
        if (db_d->options != NULL  &&
            (hw_d->options = strdup(db_d->options)) == NULL)
            goto CLEANUP;
        if (db_d->auxinfo != NULL  &&
            (hw_d->auxinfo = strdup(db_d->auxinfo)) == NULL)
            goto CLEANUP;
    }
    
    /* 2.2. Layer-infos */
    for (lio_n = 0,  db_l = db->liolist + 0,  hw_l = new_db->liolist + 0;
         lio_n < new_db->numlios;
         lio_n++,    db_l++,                  hw_l++)
    {
        /* First, duplicate record... */
        *hw_l = *db_l;
        /* ...but null-out malloc()'d fields */
        hw_l->info = NULL;

        /* Now duplicate strings, if required */
        if (db_l->info != NULL  &&
            (hw_l->info = strdup(db_l->info)) == NULL)
            goto CLEANUP;
    }
    
    /* Finally, make this DB current */
    /*!!! deregister all cxsd_hw_channels[].cb_list */
    {
      int  globalchan;

        for (globalchan = 0;  globalchan < cxsd_hw_numchans;  globalchan++)
            ForeachChanCbSlot(chan_evproc_remover,
                              NULL,
                              cxsd_hw_channels + globalchan);
    }
    CxsdDbDestroy(cxsd_hw_cur_db);
    cxsd_hw_cur_db = new_db;

    /* II. Free old data */
    safe_free(cxsd_hw_current_val_buf); cxsd_hw_current_val_buf = NULL;
    safe_free(cxsd_hw_next_wr_val_buf); cxsd_hw_next_wr_val_buf = NULL;

    /* III. Clear info */
    cxsd_hw_numlyrs  = 0;
    cxsd_hw_numdevs  = 0;
    cxsd_hw_numchans = 0;
    bzero(&cxsd_hw_layers,   sizeof(cxsd_hw_layers));
    bzero(&cxsd_hw_devices,  sizeof(cxsd_hw_devices));
    bzero(&cxsd_hw_channels, sizeof(cxsd_hw_channels));
    for (devid = 0;  devid < countof(cxsd_hw_devices);  devid++)
        cxsd_hw_devices[devid].state = DEVSTATE_OFFLINE;
    
    /* IV. Fill devices' and channels' properties according to now-current DB,
           adding devices one-by-one */

    /* Put aside a zero-id layer */
    cxsd_hw_numlyrs = 1;
    strzcpy(cxsd_hw_layers[0].lyrname, "!LYR-0!", sizeof(cxsd_hw_layers[0].lyrname));
    
    /* On stage 0 we just count sizes, and
       fill properties on stage 1 */
    for (stage = 0;  stage <= 1;  stage++)
    {
        current_val_bufsize = 0;
        next_wr_val_bufsize = 0;

        /* Enumerate devices */
        for (devid = 0, hw_d = cxsd_hw_cur_db->devlist, nchans = 0;
             devid < cxsd_hw_cur_db->numdevs;
             devid++,   hw_d++)
        {
            dev_p = cxsd_hw_devices + devid;

            dev_p->db_ref = hw_d;

            /* Remember 1st (0th!) channel number */
            dev_p->first = nchans;
            
            /* Go through channel groups */
            for (g = 0,  grp_p = hw_d->changroups;
                 g < hw_d->changrpcount;
                 g++,    grp_p++)
            {
                usize = sizeof_cxdtype(grp_p->dtype);
                csize = usize * grp_p->nelems;

                /* Iterate individual channels */
                for (x = 0,  chn_p = cxsd_hw_channels + nchans;
                     x < grp_p->count;
                     x++,    chn_p++,  nchans++)
                {
                    chn_p->rw     = grp_p->rw;
                    chn_p->devid  = devid;
                    chn_p->boss   = -1; /*!!!*/
                    chn_p->dtype  = grp_p->dtype;
                    chn_p->nelems = grp_p->nelems;
                    chn_p->usize  = usize;
                    
                    if (stage)
                    {
                        chn_p->current_val = cxsd_hw_current_val_buf + current_val_bufsize;
                        if (chn_p->nelems == 1) /* Pre-set 1 for scalar channels */
                            chn_p->current_nelems = 1;
                        if (grp_p->rw)
                            chn_p->next_wr_val = cxsd_hw_next_wr_val_buf + next_wr_val_bufsize;
                    }

                    current_val_bufsize += csize;
                    if (grp_p->rw) next_wr_val_bufsize += csize;
                }
            }

            /* Fill in # of channels */
            dev_p->count = nchans - dev_p->first;

            /* On stage 1 perform layer allocation if required */
            if (stage  &&  hw_d->lyrname[0] != '\0')
            {
                for (lyr_n = 1,  lyr_p = cxsd_hw_layers + 1;
                     lyr_n < cxsd_hw_numlyrs;
                     lyr_n++,    lyr_p++)
                    if (strcasecmp(hw_d->lyrname, lyr_p->lyrname) == 0) break;

                /* Didn't we find it? */
                if (lyr_n == cxsd_hw_numlyrs) /*!!! Check for overflow! */
                {
                    /* Allocate a layer... */
                    cxsd_hw_numlyrs++;
                    strzcpy(lyr_p->lyrname, hw_d->lyrname, sizeof(lyr_p->lyrname));
                    /* ...and make its name all-lowercase! */
                    for (x = 0; lyr_p->lyrname[x] != '\0';  x++)
                        lyr_p->lyrname[x] = tolower(lyr_p->lyrname[x]);
                }

                dev_p->lyrid = -lyr_n;
            }
        }

        /* Allocate buffers */
        if (stage == 0)
        {
            if (current_val_bufsize > 0  &&
                (cxsd_hw_current_val_buf = malloc(current_val_bufsize)) == NULL)
                goto CLEANUP;
            if (next_wr_val_bufsize > 0  &&
                (cxsd_hw_next_wr_val_buf = malloc(next_wr_val_bufsize)) == NULL)
                goto CLEANUP;
        }
    }
    cxsd_hw_numdevs  = devid;
    cxsd_hw_numchans = nchans;

    if (1) CxsdHwList(stderr);
    
    return 0;

CLEANUP:
    /* At which step are we now? */
    if (new_db != cxsd_hw_cur_db)
    {
        /* Dispose of new_db only if it isn't set as current */
        CxsdDbDestroy(new_db);
    }
    else
    {
    }
    return -1;
}


static int LoadLayer (const char        *argv0,
                      const char        *lyrname,
                      CxsdLayerModRec  **metric_p);

static int LoadDriver(const char        *argv0,
                      const char        *typename, const char *drvname,
                      CxsdDriverModRec **metric_p);

static void InitDevice(int devid);

static int  CheckDevice(int devid);


int  CxsdHwActivate(const char *argv0)
{
  int                lyr_n;
  int                devid;
  cxsd_hw_lyr_t     *lyr_p;
  cxsd_hw_dev_t     *dev_p;

    /* Load layers first */
    for (lyr_n = 1,  lyr_p = cxsd_hw_layers + 1;
         lyr_n < cxsd_hw_numlyrs;
         lyr_n++,    lyr_p++)
    {
        if (LoadLayer(argv0,
                      lyr_p->lyrname,
                      &(lyr_p->metric)) == 0)
        {
            lyr_p->logmask = cxsd_hw_defdrvlog_mask; /*!!! Should use what specified in [:OPTIONS] */
            if (MustSimulateHardware             ||
                lyr_p->metric->init_lyr == NULL  ||
                lyr_p->metric->init_lyr(-lyr_n) == 0)
            {
                lyr_p->active = 1;
            }
        }
    }

    /* And than load drivers */
    for (devid = 1,  dev_p = cxsd_hw_devices + 1;
         devid < cxsd_hw_numdevs;
         devid++,    dev_p++)
    {
        if (
            (dev_p->lyrid == 0  ||  cxsd_hw_layers[-dev_p->lyrid].active)
            &&
            LoadDriver(argv0,
                       dev_p->db_ref->typename, dev_p->db_ref->drvname,
                       &(dev_p->metric)) == 0
            &&
            CheckDevice(devid) == 0
           )
        {
            InitDevice(devid);
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

static cx_module_desc_t lyr_desc =
{
    "layer",
    CXSD_LAYER_MODREC_MAGIC,
    CXSD_LAYER_MODREC_VERSION
};

static cx_module_desc_t drv_desc =
{
    "driver",
    CXSD_DRIVER_MODREC_MAGIC,
    CXSD_DRIVER_MODREC_VERSION
};

static CXLDR_DEFINE_CONTEXT(lyr_ldr_context,
                            CXLDR_FLAG_NONE,
                            NULL,
                            "", "_lyr.so",
                            "", CXSD_LAYER_MODREC_SUFFIX_STR,
                            CxsdModuleChecker_cm,
                            CxsdModuleInit_cm, CxsdModuleFini_cm,
                            NULL,
                            NULL, 0);

static int LoadLayer (const char        *argv0,
                      const char        *lyrname,
                      CxsdLayerModRec  **metric_p)
{
  int                 r;

    r = cxldr_get_module(&lyr_ldr_context,
                         lyrname,
                         argv0,
                         (void **)metric_p,
                         &lyr_desc);

    return r;
}

/////////////////

static CXLDR_DEFINE_CONTEXT(drv_ldr_context,
                            CXLDR_FLAG_NONE,
                            NULL,
                            "", "_drv.so",
                            "", CXSD_DRIVER_MODREC_SUFFIX_STR,
                            CxsdModuleChecker_cm,
                            CxsdModuleInit_cm, CxsdModuleFini_cm,
                            NULL,
                            NULL, 0);

static int LoadDriver(const char        *argv0,
                      const char        *typename, const char *drvname,
                      CxsdDriverModRec **metric_p)
{
  int                 r;

    r = cxldr_get_module(&drv_ldr_context,
                         drvname != NULL  &&  drvname[0] != '\0'? drvname
                                                                : typename,
                         argv0,
                         (void **)metric_p,
                         &drv_desc);

    ////fprintf(stderr, "%s(%s/%s): %d\n", __FUNCTION__, typename, drvname, r);
    
    return r;
}

////////////////////////

static int  CheckDevice(int devid)
{
  cxsd_hw_dev_t    *dev;
  CxsdDbDevLine_t  *db_ref;
  CxsdDriverModRec *metric;

    dev    = cxsd_hw_devices + devid;
    db_ref = dev->db_ref;
    metric = dev->metric;

    /* 1. If there's a layer -- check version compatibility. */

    /* 2. Check conformance of channels */
    
    return 0;
}

static void InitDevice(int devid)
{
  cxsd_hw_dev_t    *dev;
  CxsdDbDevLine_t  *db_ref;
  CxsdDriverModRec *metric;
  int               state;
  
    /*!!! Check validity of devid!*/

    dev    = cxsd_hw_devices + devid;
    db_ref = dev->db_ref;
    metric = dev->metric;
    
    dev->logmask = cxsd_hw_defdrvlog_mask; /*!!! Should use what specified in [:OPTIONS] */

    if (!MustSimulateHardware  &&
        metric->init_dev != NULL) /*!!! An extremely lame check!  This "!=NULL" shouldn't be here at all! */
    {
        if (db_ref->businfocount < metric->min_businfo_n  ||
            db_ref->businfocount > metric->max_businfo_n)
        {
            DoDriverLog(devid, DRIVERLOG_ERR, "businfocount=%d, out_of[%d,%d]",
                        db_ref->businfocount,
                        metric->min_businfo_n, metric->max_businfo_n);
            /*!!! Bark -- how? */
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_CFG_PROBL, "businfocount out of range");
            return;
        }

        /* Allocate privrecsize bytes */
        if (metric->privrecsize != 0)
        {
            dev->devptr = malloc(metric->privrecsize);
            if (dev->devptr == NULL)
            {
            }
            bzero(dev->devptr, metric->privrecsize);

            /* Do psp_parse() if required */
            if (metric->paramtable != NULL)
            {
                if (psp_parse(db_ref->auxinfo, NULL,
                              dev->devptr,
                              '=', " \t", "",
                              metric->paramtable) != PSP_R_OK)
                {
                    /*!!! Should do a better-style logging... */
                    DoDriverLog(devid, DRIVERLOG_ERR,
                                "psp_parse(auxinfo)@InitDevice: %s",
                                psp_error());
                    /*!!! Bark and set device as inactive!!! */
                }
            }
        }

        state = DEVSTATE_OPERATING;
        if (metric->init_dev != NULL)
            state = metric->init_dev(devid, dev->devptr,
                                     db_ref->businfocount, db_ref->businfo,
                                     db_ref->auxinfo);
        /*!!! Check state!!! */
    }
}

////////////////////////

int  CxsdHwSetPath    (const char *path)
{
    return
        cxldr_set_path(&lyr_ldr_context, path) == 0
        &&
        cxldr_set_path(&drv_ldr_context, path) == 0
        ? 0 : -1;
}

int  CxsdHwSetSimulate(int state)
{
    MustSimulateHardware = (state != 0);

    return 0;
}


static void BeginOfCycle(void)
{
    //!!! HandleSubscriptions(); ???
    //!!! Call frontends' "end_cycle" methods
    ////fprintf(stderr, "%s %s\n", strcurtime_msc(), __FUNCTION__);
}

static void EndOfCycle  (void)
{
    //!!! HandleSimulatedHardware();
    //!!! Call frontends' "end_cycle" methods

    ++current_cycle;
}

static void ResetCycles(void)
{
  struct timeval  now;

    gettimeofday(&now, NULL);
    cycle_pass_count = 0;
    cycle_start = now;
    timeval_add_usecs(&cycle_end, &cycle_start, basecyclesize);
}

static void CycleCallback(int       uniq     __attribute__((unused)),
                          void     *privptr1 __attribute__((unused)),
                          sl_tid_t  tid      __attribute__((unused)),
                          void     *privptr2 __attribute__((unused)))
{
  struct timeval  now;

    cycle_tid = -1;

    if (current_cycle == 0) BeginOfCycle();
    EndOfCycle();

    cycle_start = cycle_end;
    timeval_add_usecs(&cycle_end, &cycle_start, basecyclesize);

    ///write(2, "zzz\n", 4);
    gettimeofday(&now, NULL);
    /* Adapt to time change -- check if desired time had already passed */
    if (TV_IS_AFTER(now, cycle_end))
    {
        /* Have we reached the limit? */
        if (cycle_pass_count < 0  /* For init at 1st time */ ||
            cycle_pass_count >= cycle_pass_count_lim)
        {
            ///fprintf(stderr, "\t%s :=0\n", strcurtime_msc());
            ResetCycles();
        }
        /* No, try more */
        else
        {
            cycle_pass_count++;
            ///fprintf(stderr, "\t%s pass_count=%d\n", strcurtime_msc(), cycle_pass_count);
        }
    }
    else
    {
        cycle_pass_count = 0;

        timeval_add_usecs(&now, &now, basecyclesize * 2);
    }

    cycle_tid = sl_enq_tout_at(0, NULL,
                               &cycle_end, CycleCallback, NULL);

    BeginOfCycle();
}

int  CxsdHwSetCycleDur(int cyclesize_us)
{
    if (cycle_tid >= 0)
    {
        sl_deq_tout(cycle_tid);
        cycle_tid = -1;
    }

    basecyclesize = cyclesize_us;
    bzero(&cycle_end, sizeof(cycle_end));
    CycleCallback(0, NULL, cycle_tid, NULL);

    return 0;
}

int  CxsdHwGetCurCycle(void)
{
    return current_cycle;
}

int  CxsdHwTimeChgBack(void)
{
    if (cycle_tid >= 0)
    {
        sl_deq_tout(cycle_tid);
        cycle_tid = -1;
    }
    ResetCycles();
    CycleCallback(0, NULL, cycle_tid, NULL);

    return -1;
}


//////////////////////////////////////////////////////////////////////

static int chan_evproc_checker(cxsd_hw_chan_cbrec_t *p, void *privptr)
{
  cxsd_hw_chan_cbrec_t *model = privptr;

    return
        p->evmask   == model->evmask    &&
        p->uniq     == model->uniq      &&
        p->privptr1 == model->privptr1  &&
        p->evproc   == model->evproc    &&
        p->privptr1 == model->privptr1;
}

int  CxsdHwAddChanEvproc(int  uniq, void *privptr1,
                         int                    globalchan,
                         int                    evmask,
                         cxsd_hw_chan_evproc_t  evproc,
                         void                  *privptr2)
{
  cxsd_hw_chan_t       *chn_p = cxsd_hw_channels + globalchan;
  cxsd_hw_chan_cbrec_t *p;
  int                   n;
  cxsd_hw_chan_cbrec_t  rec;

    if (CheckGlobalchan(globalchan) != 0) return -1;
    if (evmask == 0  ||
        evproc == NULL)                   return 0;

    /* Check if it is already in the list */
    rec.evmask   = evmask;
    rec.uniq     = uniq;
    rec.privptr1 = privptr1;
    rec.evproc   = evproc;
    rec.privptr2 = privptr2;
    if (ForeachChanCbSlot(chan_evproc_checker, &rec, chn_p) >= 0) return 0;

    n = GetChanCbSlot(chn_p);
    if (n < 0) return -1;
    p = AccessChanCbSlot(n, chn_p);

    p->evmask   = evmask;
    p->uniq     = uniq;
    p->privptr1 = privptr1;
    p->evproc   = evproc;
    p->privptr2 = privptr2;

    return 0;
}

int  CxsdHwDelChanEvproc(int  uniq, void *privptr1,
                         int                    globalchan,
                         int                    evmask,
                         cxsd_hw_chan_evproc_t  evproc,
                         void                  *privptr2)
{
  cxsd_hw_chan_t       *chn_p = cxsd_hw_channels + globalchan;
  int                   n;
  cxsd_hw_chan_cbrec_t  rec;

    if (CheckGlobalchan(globalchan) != 0) return -1;
    if (evmask == 0)                      return 0;

    /* Find requested callback */
    rec.evmask   = evmask;
    rec.uniq     = uniq;
    rec.privptr1 = privptr1;
    rec.evproc   = evproc;
    rec.privptr2 = privptr2;
    n = ForeachChanCbSlot(chan_evproc_checker, &rec, chn_p);
    if (n < 0)
    {
        /* Not found! */
        errno = ENOENT;
        return -1;
    }

    RlsChanCbSlot(n, chn_p);

    return 0;
}

static int chan_evproc_caller(cxsd_hw_chan_cbrec_t *p, void *privptr)
{
  CxsdHwChanEvCallInfo_t *info = privptr;

    if (p->evmask & info->evmask)
        p->evproc(p->uniq, p->privptr1,
                  info->globalchan, info->reason, p->privptr2);
    return 0;
}
int  CxsdHwCallChanEvprocs(int globalchan, CxsdHwChanEvCallInfo_t *info)
{
  cxsd_hw_chan_t       *chn_p = cxsd_hw_channels + globalchan;

    if (CheckGlobalchan(globalchan) != 0) return -1;

    info->globalchan = globalchan;
    ForeachChanCbSlot(chan_evproc_caller, info, chn_p);

    return 0;
}


static int chan_evproc_cleanuper(cxsd_hw_chan_cbrec_t *p, void *privptr)
{
  int  uniq = ptr2lint(privptr);

    if (p->uniq == uniq)
        /*!!! should call RlsChanCbSlot() somehow, but it requires "id" instead of "p" */
        p->evmask = 0;

    return 0;
}
void cxsd_hw_do_cleanup(int uniq)
{
  int  globalchan;

    for (globalchan = 0;  globalchan < cxsd_hw_numchans;  globalchan++)
        ForeachChanCbSlot(chan_evproc_cleanuper,
                          lint2ptr(uniq),
                          cxsd_hw_channels + globalchan);
}
