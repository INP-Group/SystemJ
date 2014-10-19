
#include "cxsd_driver.h"
#include "timeval_utils.h"

#include "drv_i/c0609_drv_i.h"


enum {ANY_A = 0};
enum {DEFAULT_D = 0, DEFAULT_T = 2};
enum
{
    TOU_HBT_USECS = 1000*1000,
    MAX_MES_USECS = (960 + 100)*1000,
};

typedef struct
{
    int             N;
    int             KAS;

    int             D;
    int             T;

    sl_tid_t        tid;
    struct timeval  dln; // DeadLiNe
    int             locked;
    int             cur;
    int             rqn;
    int             requested[C0609_CHAN_ADC_n_count];
} c0609_privrec_t;

static psp_paramdescr_t c0609_params[] =
{
    PSP_P_INT ("D", c0609_privrec_t, D, DEFAULT_D, 0, 1),
    PSP_P_INT ("T", c0609_privrec_t, T, DEFAULT_T, 0, 7),
    PSP_P_END()
};

static void LAM_CB(int devid, void *devptr);
static void TOU_CB(int devid, void *devptr, sl_tid_t tid, void *privptr);

static int c0609_init_d(int devid, void *devptr,
                        int businfocount, int *businfo,
                        const char *auxinfo)
{
  c0609_privrec_t *me = (c0609_privrec_t*)devptr;

  const char      *errstr;
  int              junk;

    me->N   = businfo[0];
    me->KAS = (businfocount > 1)? businfo[1] : -1;

    me->cur = -1;
    me->rqn = 0;

    DO_NAF(CAMAC_REF, me->N, ANY_A, 24, &junk); // Stop
    DO_NAF(CAMAC_REF, me->N, ANY_A, 10, &junk); // Drop LAM

    if ((errstr = WATCH_FOR_LAM(devid, devptr, me->N, LAM_CB)) != NULL)
    {
        DoDriverLog(devid, DRIVERLOG_ERRNO, "WATCH_FOR_LAM(): %s", errstr);
        return -CXRF_DRV_PROBL;
    }

    me->tid = sl_enq_tout_after(devid, devptr, TOU_HBT_USECS, TOU_CB, NULL);

    return DEVSTATE_OPERATING;
}

static void StartMeasurement(int devid, c0609_privrec_t *me, int x)
{
  int              cword;
  int              junk;

    me->cur = x;

    if (me->KAS > 0) DO_NAF(CAMAC_REF, me->KAS, ANY_A, 16, &x);
    cword = (me->D << 3) | me->T;
    DO_NAF(CAMAC_REF, me->N, ANY_A, 16, &cword); // Write control word
    DO_NAF(CAMAC_REF, me->N, ANY_A, 26, &junk);  // Start

    gettimeofday     (&(me->dln), NULL);
    timeval_add_usecs(&(me->dln), &(me->dln), MAX_MES_USECS); // Shouldn't we use per-T times?
}

static void ReadAndReturnMeasurement(int devid, c0609_privrec_t *me, rflags_t rflags)
{
  int32            value;

  int              junk;
  int              v;

  int              this_cur;
  int              left;
  int              x;

//fprintf(stderr, "%s(cur=%d, rqn=%d)\n", __FUNCTION__, me->cur, me->rqn);
    DO_NAF(CAMAC_REF, me->N, ANY_A, 10, &junk); // Drop LAM

    if (me->cur < 0) return;  // A guard against "spontaneous"/inherited LAMs

    rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, ANY_A, 0,  &v)); // Read the code
    // Overload?
    if (v == 0)
    {
        value   = 100000000; // 100V
        rflags |= CXRF_OVERLOAD;
    }
    else
    {
        value   = ((022000 * 1000) - ((v * 1000) / (1 << me->T))) /
                   (1 + (15 * me->D));
    }

    // Drop request
    me->requested[me->cur] = 0;
    me->rqn--;
    this_cur = me->cur;
    me->cur = -1;

    /*!!! A theoretically-possible race condition: if working as
          a driver INSIDE server, if another _rw request is issued
          while inside ReturnChanGroup() -- cur,rqn,requested[] operation
          will be affected.
          Thus, a "lock" is added, so that _rw_p() wouldn't
          call StartMeasurement() if "locked". */
    me->locked++;
    ReturnChanGroup(devid, this_cur, 1, &value, &rflags);
    me->locked--;

    /* Anything more to measure? */
    if (me->rqn > 0)
        for (left = C0609_CHAN_ADC_n_count, x = this_cur + 1;
             left > 0;
             left--,                        x++)
        {
            if (x >= C0609_CHAN_ADC_n_count) x = 0; // Loop/wrap
            if (me->requested[x])
            {
                StartMeasurement(devid, me, x);
                break;
            }
        }
//    fprintf(stderr, "AFTER: cur=%d, rqn=%d, [0]=%d\n", me->cur, me->rqn, me->requested[0]);
}

static void LAM_CB(int devid, void *devptr)
{
    ReadAndReturnMeasurement(devid, devptr, 0);
}

static void TOU_CB(int devid, void *devptr, sl_tid_t tid, void *privptr)
{
  c0609_privrec_t *me = (c0609_privrec_t*)devptr;
  struct timeval   now;

    me->tid = -1;
    if (me->cur >= 0)
    {
        gettimeofday(&now, NULL);
        if (TV_IS_AFTER(now, me->dln))
            ReadAndReturnMeasurement(devid, devptr, CXRF_IO_TIMEOUT);
    }
    me->tid = sl_enq_tout_after(devid, devptr, TOU_HBT_USECS, TOU_CB, privptr);
}

static void c0609_rw_p(int devid, void *devptr,
                       int first, int count, int32 *values, int action)
{
  c0609_privrec_t *me = (c0609_privrec_t*)devptr;

  int              n;
  int              x;

  int32            value;
  rflags_t         rflags;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;

        if      (0  &&
                 (x >= 1  &&  x <= 4))
        {
            value  = 0;
            rflags = 0;
            if (x == 1) value  = me->cur    * 1000;
            if (x == 2) value  = me->rqn    * 1000;
            if (x == 3) value  = me->locked * 1000;
            if (x == 4) value  = me->requested[0] * 1000;
            ReturnChanGroup(devid, x, 1, &value, &rflags);
        }
        else if ((x > 0  &&  me->KAS <= 0)  ||  x >= C0609_CHAN_ADC_n_count)
        {
            value  = 0;
            rflags = CXRF_UNSUPPORTED;
            ReturnChanGroup(devid, x, 1, &value, &rflags);
        }
        else if (me->requested[x] == 0)
        {
            me->requested[x] = 1;
            me->rqn++;
//fprintf(stderr, "%s(rqn=%d)\n", __FUNCTION__, me->rqn);
            if (me->rqn == 1  &&  me->locked == 0)
                StartMeasurement(devid, me, x);// ReadAndReturnMeasurement(devid, devptr, CXRF_IO_TIMEOUT);
        }
        /* else do nothing */

    }
}


DEFINE_DRIVER(c0609, "C0609",
              NULL, NULL,
              sizeof(c0609_privrec_t), c0609_params,
              1, 2,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              c0609_init_d, NULL, c0609_rw_p, NULL);
