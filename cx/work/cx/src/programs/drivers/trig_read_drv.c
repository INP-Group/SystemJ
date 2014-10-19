#include "cxsd_driver.h"

/*
    auxinfo syntax:
        TRIGGER_DEV.TRIGGER_CHAN_N/DATA_DEV.DATA_CHAN_N
    Number of '/'s specifies the number of data-measurements to skip
    before returning data.
 */

enum {HEARTBEAT_PERIOD = 2*1000000};

enum
{
    RM_TRGR  = 0,
    RM_DATA  = 1,
    RM_COUNT = 2
};

typedef struct
{
    inserver_refmetric_t  rms[RM_COUNT];
    int                   n_to_skip;   // '/'=>1 means "return FIRST value after trigger"
    int                   was_trigger;
    int                   skip_count;
} privrec_t;


static void trgr_evproc(int                 devid, void *devptr,
                        inserver_chanref_t  ref,
                        int                 reason,
                        void               *privptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    DoDriverLog(devid, DRIVERLOG_DEBUG | DRIVERLOG_C_ENTRYPOINT,
                "%s(%s.%d->%d)",
                __FUNCTION__,
                me->rms[RM_TRGR].targ_name,
                me->rms[RM_TRGR].chan_n,
                me->rms[RM_TRGR].chan_ref);

    if (me->was_trigger) return;

    ////fprintf(stderr, "%s()\n", __FUNCTION__);
    
    me->was_trigger = 1;
    me->skip_count  = 0;
}

static void data_evproc(int                 devid, void *devptr,
                        inserver_chanref_t  ref,
                        int                 reason,
                        void               *privptr)
{
  privrec_t      *me = (privrec_t *)devptr;
  int32           val;
  rflags_t        rflags;

    DoDriverLog(devid, DRIVERLOG_DEBUG | DRIVERLOG_C_ENTRYPOINT,
                "%s(%s.%d->%d)",
                __FUNCTION__,
                me->rms[RM_DATA].targ_name,
                me->rms[RM_DATA].chan_n,
                me->rms[RM_DATA].chan_ref);

    if (!me->was_trigger) return;
    
    ////fprintf(stderr, "%s()\n", __FUNCTION__);
    
    me->skip_count++;
    if (me->skip_count >= me->n_to_skip)
    {
        inserver_get_cval(devid, ref,
                          &val, &rflags, NULL);
        ReturnChanGroup(devid, 0, 1, &val, &rflags);
        me->was_trigger = 0;
    }
}

static int trig_read_init_d(int devid, void *devptr,
                            int businfocount, int businfo[],
                            const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;
  const char     *p  = auxinfo;
  int             r;

    /* Parse auxinfo */
    if (auxinfo == NULL)
    {
        DoDriverLog(devid, 0, "%s(): empty auxinfo", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    if ((r = inserver_parse_d_c_ref(devid, "trgr", &p, me->rms+RM_TRGR) < 0))
        return r;

    if (*p != '/')
    {
        DoDriverLog(devid, 0, "'/' expected after trigger-chan spec");
        return -CXRF_CFG_PROBL;
    }
    while (*p == '/')
    {
        me->n_to_skip++;
        p++;
    }

    if ((r = inserver_parse_d_c_ref(devid, "data", &p, me->rms+RM_DATA) < 0))
        return r;
    
    DoDriverLog(devid, DRIVERLOG_INFO, "skip=%d", me->n_to_skip);

    /* Set watch */
    me->rms[0      ].m_count = RM_COUNT;
    me->rms[RM_TRGR].data_cb = trgr_evproc;
    me->rms[RM_DATA].data_cb = data_evproc;
    inserver_new_watch_list(devid, me->rms, HEARTBEAT_PERIOD);

    return DEVSTATE_OPERATING;
}

static void trig_read_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    /* We rely on server to release all inserver* resources */
}

/* Metric */

static CxsdMainInfoRec trig_read_main_info[] =
{
    {1,   0},
};
DEFINE_DRIVER(trig_read, "Triggered-read driver",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(trig_read_main_info), trig_read_main_info,
              -1, NULL,
              trig_read_init_d, trig_read_term_d,
              NULL, NULL);
