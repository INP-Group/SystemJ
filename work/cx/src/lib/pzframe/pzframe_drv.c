#include "timeval_utils.h"

#include "pzframe_drv.h"


static int32     zero_value = 0;
static rflags_t  rf_bad     = {CXRF_UNSUPPORTED};
static rflags_t  zero_rflags[CX_MAX_BIGC_PARAMS] = {[0 ... CX_MAX_BIGC_PARAMS-1] = 0};


static void pzframe_tout_p(int devid, void *devptr,
                           sl_tid_t tid,
                           void *privptr);

static void ReturnMeasurements (pzframe_drv_t *pdr, rflags_t rflags)
{
  rflags_t rm_rflags;

    rm_rflags = pdr->read_measurements(pdr);
    ReturnBigc(pdr->devid, 0, pdr->cur_args, pdr->num_params,
               pdr->retdata_p, pdr->retdatasize, pdr->retdataunits,
               rflags | rm_rflags);
    ReturnChanGroup(pdr->devid, 0, pdr->num_params, pdr->cur_args, zero_rflags);
}

static void SetDeadline(pzframe_drv_t *pdr)
{
  struct timeval   msc; // timeval-representation of MilliSeConds
  struct timeval   dln; // DeadLiNe

    if (pdr->param_waittime >=  0  &&
        pdr->cur_args[pdr->param_waittime] > 0)
    {
        msc.tv_sec  =  pdr->cur_args[pdr->param_waittime] / 1000;
        msc.tv_usec = (pdr->cur_args[pdr->param_waittime] % 1000) * 1000;
        timeval_add(&dln, &(pdr->measurement_start), &msc);
        
        pdr->tid = sl_enq_tout_at(pdr->devid, NULL, &dln,
                                  pzframe_tout_p, pdr);
    }
}

static void RequestMeasurements(pzframe_drv_t *pdr)
{
  int  r;

    memcpy(pdr->cur_args, pdr->nxt_args, sizeof(int32) * pdr->num_params);

    r = pdr->start_measurements(pdr);
    if (r == PZFRAME_R_READY)
        ReturnMeasurements(pdr, 0);
    else
    {
        pdr->measuring_now = 1;
        gettimeofday(&(pdr->measurement_start), NULL);
        SetDeadline(pdr);

        if (pdr->param_istart >= 0  &&
            (pdr->cur_args[pdr->param_istart] & CX_VALUE_LIT_MASK) != 0)
            pdr->trggr_measurements(pdr);
    }
}

static void PerformTimeoutActions(pzframe_drv_t *pdr, int do_return)
{
    pdr->measuring_now = 0;
    if (pdr->tid >= 0)
    {
        sl_deq_tout(pdr->tid);
        pdr->tid = -1;
    }
    pdr->abort_measurements(pdr);
    if (do_return)
        ReturnMeasurements (pdr, CXRF_IO_TIMEOUT);
    else
        RequestMeasurements(pdr);
}

static void pzframe_tout_p(int devid, void *devptr __attribute__((unused)),
                           sl_tid_t tid __attribute__((unused)),
                           void *privptr)
{
  pzframe_drv_t *pdr = (pzframe_drv_t *)privptr;

    pdr->tid = -1;
    if (pdr->measuring_now == 0  ||
        pdr->param_waittime < 0  ||
        pdr->cur_args[pdr->param_waittime] <= 0)
    {
        DoDriverLog(devid, DRIVERLOG_WARNING,
                    "strange timeout: measuring_now=%d, waittime=%d",
                    pdr->measuring_now,
                    pdr->param_waittime < 0 ? -123456789 : pdr->cur_args[pdr->param_waittime]);
        return;
    }

    PerformTimeoutActions(pdr, 1);
}

//////////////////////////////////////////////////////////////////////


void  pzframe_drv_init(pzframe_drv_t *pdr, int devid,
                       int                           num_params,
                       int                           param_shot,
                       int                           param_istart,
                       int                           param_waittime,
                       int                           param_stop,
                       int                           param_elapsed,
                       pzframe_validate_param_t      validate_param,
                       pzframe_start_measurements_t  start_measurements,
                       pzframe_trggr_measurements_t  trggr_measurements,
                       pzframe_abort_measurements_t  abort_measurements,
                       pzframe_read_measurements_t   read_measurements)
{
    //bzero(pdr, sizeof(*pdr)); // 30.01.2014: because MANY drivers psp_parse() into pdr->nxt_args[]
    pdr->devid              = devid;

    pdr->num_params         = num_params;
    pdr->param_shot         = param_shot;
    pdr->param_istart       = param_istart;
    pdr->param_waittime     = param_waittime;
    pdr->param_stop         = param_stop;
    pdr->param_elapsed      = param_elapsed;
    pdr->validate_param     = validate_param;
    pdr->start_measurements = start_measurements;
    pdr->trggr_measurements = trggr_measurements;
    pdr->abort_measurements = abort_measurements;
    pdr->read_measurements  = read_measurements;

    pdr->state              = 0; // In fact, unused
    pdr->measuring_now      = 0;
    pdr->tid                = -1;

    pdr->retdata_p          = NULL;
    pdr->retdatasize        = 0;
    pdr->retdataunits       = 0;
}

void  pzframe_drv_term(pzframe_drv_t *pdr)
{
    if (pdr->tid >= 0)
    {
        sl_deq_tout(pdr->tid);
        pdr->tid = -1;
    }
}

void  pzframe_drv_rw_p  (pzframe_drv_t *pdr,
                         int firstchan, int count,
                         int32 *values, int action)
{
  int         n;
  int         x;
  int32       v;

  struct timeval   now;

    if (action == DRVA_WRITE)
    {
        for (n = 0;  n < count;  n++)
        {
            x = firstchan + n;
            if      (x >= pdr->num_params)
            {
                ReturnChanGroup(pdr->devid, x, 1, &zero_value, &rf_bad);
                goto NEXT_PARAM;
            }
            
            v = pdr->validate_param(pdr, x, values[n]);
            if      (x == pdr->param_shot)
            {
                ////DoDriverLog(devid, 0, "SHOT!");
                if (pdr->measuring_now  &&  v == CX_VALUE_COMMAND)
                    pdr->trggr_measurements(pdr);
                ReturnChanGroup(pdr->devid, x, 1, &zero_value, &(zero_rflags[0]));
            }
            else if (x == pdr->param_waittime)
            {
                if (v < 0) v = 0;
                pdr->cur_args[x] = pdr->nxt_args[x] = v;
                ReturnChanGroup(pdr->devid, x, 1, pdr->nxt_args + x, &(zero_rflags[0]));
                /* Adapt to new waittime */
                if (pdr->measuring_now)
                {
                    if (pdr->tid >= 0)
                    {
                        sl_deq_tout(pdr->tid);
                        pdr->tid = -1;
                    }
                    SetDeadline(pdr);
                }
            }
            else if (x == pdr->param_stop)
            {
                if (pdr->measuring_now  &&  
                    (v &~ CX_VALUE_DISABLED_MASK) == CX_VALUE_COMMAND)
                    PerformTimeoutActions(pdr,
                                          (v & CX_VALUE_DISABLED_MASK) == 0);
                ReturnChanGroup(pdr->devid, x, 1, &zero_value, &(zero_rflags[0]));
            }
            else
            {
                if (x == pdr->param_elapsed) v = 0;
                pdr->nxt_args[x] = v;
                ReturnChanGroup(pdr->devid, x, 1, pdr->nxt_args + x, &(zero_rflags[0]));
            }
        NEXT_PARAM:;
        }
    }
    else /* DRVA_READ */
    {
        if (firstchan + count <= pdr->num_params)
            ReturnChanGroup(pdr->devid, firstchan, count, pdr->nxt_args + firstchan, zero_rflags);
        else
        {
            for (n = 0;  n < count;  n++)
            {
                x = firstchan + n;
                if (x == pdr->param_elapsed  ||  x >= pdr->num_params)
                {
                    /* Treat all channels above NUMCHANS as time-since-measurement-start */
                    if (!pdr->measuring_now)
                        v = 0;
                    else
                    {
                        gettimeofday(&now, NULL);
                        timeval_subtract(&now, &now, &(pdr->measurement_start));
                        v = now.tv_sec * 1000 + now.tv_usec / 1000;
                    }
                    ReturnChanGroup(pdr->devid, x, 1, &v, &(zero_rflags[0]));
                }
                else
                    ReturnChanGroup(pdr->devid, x, 1, pdr->nxt_args + x, &(zero_rflags[0]));
            }
        }
    }
}

void  pzframe_drv_bigc_p(pzframe_drv_t *pdr,
                         int32 *args, int nargs)
{
  int                  n;
  int32                v;
  
    if (pdr->measuring_now) return;
    
    DoDriverLog(pdr->devid, DRIVERLOG_DEBUG, "%s(): nargs=%d", __FUNCTION__ , nargs);
    for (n = 0;  n < pdr->num_params  &&  n < nargs;  n++)
    {
        v = pdr->validate_param(pdr, n, args[n]);
        if (n == pdr->param_shot  ||  n == pdr->param_stop) v = 0; // SHOT and STOP are regular_channel-only
        if (n == pdr->param_elapsed) v = 0;
        pdr->nxt_args[n] = v;
    }
    RequestMeasurements(pdr);
}

void  pzframe_drv_drdy_p(pzframe_drv_t *pdr, int do_return, rflags_t rflags)
{
    if (pdr->measuring_now == 0) return;
    pdr->measuring_now = 0;
    if (pdr->tid >= 0)
    {
        sl_deq_tout(pdr->tid);
        pdr->tid = -1;
    }
    if (do_return)
        ReturnMeasurements (pdr, rflags);
    else
        RequestMeasurements(pdr);
}

void  pzframe_set_buf_p (pzframe_drv_t *pdr, void *retdata_p)
{
    pdr->retdata_p = retdata_p;
}
