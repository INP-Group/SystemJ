
#include "timeval_utils.h"

#include "cxsd_driver.h"
#include "vdev.h"


static rflags_t  zero_rflags = 0;


static void WipeRefs(vdev_observer_t *o_p)
{
  int  x;

    o_p->targ_state = DEVSTATE_NOTREADY;
    o_p->targ_ref   = INSERVER_DEVREF_ERROR;
    for (x = 0;  x < o_p->targ_numchans;  x++)
    {
        o_p->cur[x].ref  = INSERVER_CHANREF_ERROR;
        o_p->cur[x].rcvd = 0;
    }
}

static void observer_stat_evproc(int                 devid, void *devptr,
                                 inserver_devref_t   ref,
                                 int                 newstate,
                                 void               *privptr)
{
  vdev_observer_t *o_p = privptr;

    fprintf(stderr, "%d/%s %d's newstate=%d\n", devid, __FUNCTION__, ref, newstate);
    o_p->targ_state = newstate;
    if  (newstate == DEVSTATE_OFFLINE)
        WipeRefs(o_p);

    //!!! Call client!
    if (o_p->stat_cb != NULL)
        o_p->stat_cb(devid, devptr, o_p, o_p->privptr);
}

static void observer_trgc_evproc(int                 devid, void *devptr,
                                 inserver_chanref_t  ref,
                                 int                 reason,
                                 void               *privptr)
{
  vdev_trgc_cur_t *cause_p = privptr;
  vdev_observer_t *o_p     = cause_p->o_p;
  int              trgc    = cause_p - o_p->cur; // We use a hack: privptr points to the channel "metric", which has a pointer to observer, and than we calculate channel # by subtracting "cur" pointer from channel's

    inserver_get_cval(devid, ref,
                      &(o_p->cur[trgc].val), &(o_p->cur[trgc].flgs), NULL);
    o_p->cur[trgc].rcvd = 1;
////if (o_p->cur[trgc].flgs & CXRF_UNSUPPORTED) fprintf(stderr, "[%d] .%d UNSUPPORTED\n", devid, ref);

    if (o_p->trgc_cb != NULL)
        o_p->trgc_cb(devid, devptr, o_p, trgc, o_p->privptr);
}

static void vdev_bind_hbt(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr)
{
  vdev_observer_t *obs = privptr;
  vdev_observer_t *o_p = obs;
  int              num = o_p->m_count;
  int              trgc;

    obs->bind_hbt_tid = -1;

    do
    {
        if (o_p->targ_name[0] != '\0'  &&
            o_p->targ_ref == INSERVER_DEVREF_ERROR)
        {
            o_p->targ_ref = inserver_get_devref(devid, o_p->targ_name);
            if (o_p->targ_ref != INSERVER_DEVREF_ERROR)
            {
                o_p->is_suffering = 0;
                inserver_get_devstat(devid, o_p->targ_ref, &(o_p->targ_state));
                if (o_p->targ_state != DEVSTATE_OFFLINE)
                {
                    inserver_add_devstat_evproc(devid, o_p->targ_ref, observer_stat_evproc, o_p);
                    for (trgc = 0;  trgc < o_p->targ_numchans;  trgc++)
                        if (o_p->map[trgc].mode != 0)
                        {
                            o_p->cur[trgc].o_p = o_p;
                            o_p->cur[trgc].ref =
                                inserver_get_chanref(devid, o_p->targ_name, trgc);
                            o_p->cur[trgc].isrw = inserver_chan_is_rw(devid, o_p->cur[trgc].ref);
                            inserver_add_chanref_evproc(devid,
                                                        o_p->cur[trgc].ref,
                                                        observer_trgc_evproc, o_p->cur + trgc);
                            vdev_observer_req_chan(o_p, trgc);
                        }
                }
                if (o_p->stat_cb != NULL)
                    o_p->stat_cb(devid, devptr, o_p, o_p->privptr);
            }
            else
            {
                if (o_p->is_suffering == 0)
                    DoDriverLog(devid, 0, "target dev \"%s\" not found", o_p->targ_name);
                o_p->is_suffering = 1;
            }
        }

        o_p++;
        num--;
    }
    while (num > 0);

    obs->bind_hbt_tid = sl_enq_tout_after(obs->devid, obs->devptr, obs->bind_hbt_period, vdev_bind_hbt, privptr);
}

int  vdev_observer_init(vdev_observer_t *obs,
                        int devid, void *devptr,
                        int bind_hbt_period)
{
  vdev_observer_t *o_p    = obs;
  int              num    = o_p->m_count;
  int              offset = 0;

    do
    {
        o_p->targ_offset     = offset;
        o_p->devid           = devid;
        o_p->devptr          = devptr;
        obs->bind_hbt_period = bind_hbt_period;
        WipeRefs(o_p);

        offset += o_p->targ_numchans;

        o_p++;
        num--;
    }
    while (num > 0);
    vdev_bind_hbt(obs->devid, obs->devptr, -1, obs);

    return 0;
}

int  vdev_observer_fini(vdev_observer_t *obs)
{
    /*!!!*/

    return 0;
}

void vdev_observer_req_chan(vdev_observer_t *o_p, int trgc)
{
  tag_t            age;

    if (o_p->cur[trgc].isrw)
    {
        inserver_get_cval(o_p->devid, o_p->cur[trgc].ref,
                          NULL, NULL, &age);
        if (age == 0) // We DON'T try to return non-fresh data -- rather wait till it becomes really fresh
            observer_trgc_evproc(o_p->devid, o_p->devptr,
                                 o_p->cur[trgc].ref,
                                 0,
                                 o_p->cur + trgc);
        else
            fprintf(stderr, "\t\tread(%d/%d/%d): age=%d\n", trgc, o_p->map[trgc].ourc, o_p->cur[trgc].ref, age);
    }
    else
        inserver_req_cval(o_p->devid, o_p->cur[trgc].ref);
}

void vdev_observer_snd_chan(vdev_observer_t *o_p, int trgc, int32 val)
{
    if (trgc >= 0  &&  trgc < o_p->targ_numchans  &&
        o_p->cur[trgc].ref != INSERVER_CHANREF_ERROR)
        inserver_snd_cval(o_p->devid, o_p->cur[trgc].ref, val);
}

void vdev_observer_forget_all(vdev_observer_t *o_p)
{
  int  trgc;

    for (trgc = 0;  trgc < o_p->targ_numchans;  trgc++)
        o_p->cur[trgc].rcvd = 0;
}


//////////////////////////////////////////////////////////////////////

static int CalcSubordDevState(vdev_context_t *ctx)
{
  vdev_observer_t *o_p = ctx->obs;
  int              num = o_p->m_count;

  int              off_count;
  int              nrd_count;

    off_count = nrd_count = 0;
    do
    {
        if (o_p->targ_state == DEVSTATE_OFFLINE)  off_count++;
        if (o_p->targ_state == DEVSTATE_NOTREADY) nrd_count++;

        o_p++;
        num--;
    }
    while (num > 0);

    if      (off_count > 0) return DEVSTATE_OFFLINE;
    else if (nrd_count > 0) return DEVSTATE_NOTREADY;
    else                    return DEVSTATE_OPERATING;
}

static int SelectOurDevState(int subord_state)
{
    if (subord_state == DEVSTATE_OFFLINE) return DEVSTATE_NOTREADY;
    else                                  return subord_state;
}

static void vdev_work_hbt(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr)
{
  vdev_context_t    *ctx = (vdev_context_t *)privptr;
  
  struct timeval     now;
  vdev_state_dsc_t *curs = ctx->state_descr + ctx->cur_state;
  vdev_state_dsc_t *nxts;

    if (curs->next_state >= 0  &&
        curs->delay_us   >  0  &&
        (gettimeofday(&(now), NULL), TV_IS_AFTER(now, ctx->next_state_time)))
    {
        nxts = ctx->state_descr + curs->next_state;
        if (nxts->sw_alwd == NULL  ||
            nxts->sw_alwd(devptr, ctx->cur_state))
            vdev_set_state(ctx, curs->next_state);
    }

    sl_enq_tout_after(ctx->devid, ctx->devptr, ctx->work_hbt_period, vdev_work_hbt, ctx);
}

static void vdev_stat_cb(int devid, void *devptr,
                         vdev_observer_t *o_p,
                         void *privptr)
{
  vdev_context_t *ctx = (vdev_context_t *)privptr;

  int             subord_state = CalcSubordDevState(ctx);
  int             our_state    = SelectOurDevState (subord_state);

fprintf(stderr, "%d/%s subord=%d our=%d\n", devid, __FUNCTION__, subord_state, our_state);
    SetDevState(devid, our_state, 0, NULL); // Here should try to make some description
    if      (subord_state == DEVSTATE_OFFLINE)
    {
        vdev_set_state(ctx, ctx->state_unknown_val);
    }
    else if (subord_state == DEVSTATE_NOTREADY)
    {
        if (ctx->cur_state != ctx->state_unknown_val  &&
            ctx->state_determine_val >= 0)
            vdev_set_state(ctx, ctx->state_determine_val);
    }
}

static int AllImportantAreReady(vdev_context_t *ctx)
{
  vdev_observer_t *o_p = ctx->obs;
  int              num = o_p->m_count;
  int              trgc;

    do
    {
        for (trgc = 0;  trgc < o_p->targ_numchans;  trgc++)
            if (o_p->map[trgc].mode & VDEV_IMPR  &&
                o_p->cur[trgc].rcvd == 0)
                return 0;

        o_p++;
        num--;
    }
    while (num > 0);

    return 1;
}

static void vdev_trgc_cb(int devid, void *devptr,
                         vdev_observer_t *o_p,
                         int trgc,
                         void *privptr)
{
  vdev_context_t *ctx  = (vdev_context_t *)privptr;
  int32           val  = o_p->cur[trgc].val;
  int             sodc = o_p->targ_offset + trgc;
  int             mode = o_p->map[trgc].mode;

  struct timeval    now;
  vdev_state_dsc_t *curs = ctx->state_descr + ctx->cur_state;
  vdev_state_dsc_t *nxts;

    /* Perform tubbing */
    if (mode & VDEV_TUBE)
        ReturnChanGroup(devid, o_p->map[trgc].ourc, 1,
                        &(o_p->cur[trgc].val), &(o_p->cur[trgc].flgs));

    /* Check if we are ready to operate */
    if (ctx->cur_state == ctx->state_unknown_val  &&
        ctx->state_determine_val >= 0             &&
        mode & VDEV_IMPR                          &&
        AllImportantAreReady(ctx))
        vdev_set_state(ctx, ctx->state_determine_val);

    /* Call the client */
    if (ctx->sodc_cb != NULL)
        ctx->sodc_cb(devid, devptr, sodc, val);

    /* If not all info is gathered yet, then there's nothing else to do yet */
    if (ctx->cur_state == ctx->state_unknown_val) return;

    /* Perform state transitions depending on current state */
    if (/* There is a state to switch to */
        curs->next_state >= 0
        &&
        /* ...and no delay or delay has expired */
        (curs->delay_us <= 0  ||
         (gettimeofday(&(now), NULL),
          TV_IS_AFTER(now, ctx->next_state_time))))
    {
        nxts = ctx->state_descr + curs->next_state;
        if (/* This channel IS "interesting"... */
            ctx->state_important_channels != NULL    &&
            ctx->state_important_channels[(/*ctx->cur_state*/curs->next_state * ctx->total_numchans) + sodc]  &&
            /* ...and condition is met */
            nxts->sw_alwd != NULL  &&
            nxts->sw_alwd(devptr, ctx->cur_state))
        {
            vdev_set_state(ctx, curs->next_state);
        }
    }
}

int  vdev_init(vdev_context_t *ctx, vdev_observer_t *obs,
               int devid, void *devptr,
               int bind_hbt_period, int work_hbt_period)
{
  vdev_observer_t *o_p = obs;
  int              num = o_p->m_count;
  int              r;

    ctx->total_numchans = 0;
    /* Fill in our callbacks */
    do
    {
        o_p->stat_cb = vdev_stat_cb;
        o_p->trgc_cb = vdev_trgc_cb;
        o_p->privptr = ctx;

        ctx->total_numchans += o_p->targ_numchans;

        o_p++;
        num--;
    }
    while (num > 0);

    ctx->obs             = obs;
    ctx->devid           = devid;
    ctx->devptr          = devptr;
    ctx->work_hbt_period = work_hbt_period;

    r = vdev_observer_init(obs, devid, devptr, bind_hbt_period);

    vdev_work_hbt(devid, devptr, -1, ctx);

    return SelectOurDevState(CalcSubordDevState(ctx));
}

int  vdev_fini(vdev_context_t *ctx)
{
    /*!!!*/

    return 0;
}

void vdev_set_state(vdev_context_t *ctx, int nxt_state)
{
  int                 prev_state = ctx->cur_state;
  int32               val;
  struct timeval      timenow;
  vdev_sr_chan_dsc_t *sdp;

    if (nxt_state < 0  ||  nxt_state > ctx->state_count)
    {
        return;
    }

    if (nxt_state == ctx->cur_state) return;

    ctx->cur_state = nxt_state;
    val = ctx->cur_state;
    if (ctx->chan_state_n >= 0)
        ReturnChanGroup(ctx->devid, ctx->chan_state_n, 1, &val, &zero_rflags);
    fprintf(stderr, "[%d] state:=%d\n", ctx->devid, val);

    bzero(&(ctx->next_state_time), sizeof(ctx->next_state_time));
    if (ctx->state_descr[nxt_state].delay_us > 0)
    {
        gettimeofday(&timenow, NULL);
        timeval_add_usecs(&(ctx->next_state_time),
                          &timenow,
                          ctx->state_descr[nxt_state].delay_us);
    }

    if (ctx->state_descr[nxt_state].swch_to != NULL)
        ctx->state_descr[nxt_state].swch_to(ctx->devptr, prev_state);

    for (sdp = ctx->state_related_channels;
         sdp != NULL  &&  sdp->ourc >= 0  &&  ctx->do_rw != NULL;
         sdp++)
        ctx->do_rw(ctx->devid, ctx->devptr, sdp->ourc, 1, NULL, DRVA_READ);
}
