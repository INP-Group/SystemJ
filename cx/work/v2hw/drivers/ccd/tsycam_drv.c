#include <stdio.h>
#include <stdlib.h>

#include "misc_macros.h"

#include "cxsd_driver.h"

#include "tsycamlib.h"


enum {PIX_W = 660, PIX_H = 500};

enum
{
    TSYCAM_PARAM_W    = 0,
    TSYCAM_PARAM_H    = 1,
    TSYCAM_PARAM_K    = 2,
    TSYCAM_PARAM_T    = 3,
    TSYCAM_PARAM_SYNC = 4,
    TSYCAM_PARAM_MISS = 5,

    NUM_TSYCAM_PARAMS = 6
};

typedef struct
{
    void       *ref;
    int         pulling;

    int         devid;
    sl_tid_t    act_tid;

    int         param_K;
    int         param_T;
    int         param_SYNC;
    
    int32       curdata[PIX_H][PIX_W];
} privrec_t;


static void tsycam_fd_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr);

static int tsycam_init_d(int devid, void *devptr, 
                         int businfocount, int businfo[],
                         const char *auxinfo)
{
  privrec_t *me = (privrec_t *) devptr;

    me->devid   = devid;
    me->act_tid = -1;

    /* Create an object */
    if ((me->ref =
         tc_create_camera_object(auxinfo, 0,
                                 PIX_W, PIX_H,
                                 &(me->curdata[0][0]), sizeof(me->curdata[0][0]))
        ) == NULL)
        goto ERREXIT;

    tc_set_parameter(me->ref, "K",    (me->param_K    = 100));
    tc_set_parameter(me->ref, "T",    (me->param_T    = 200));
    tc_set_parameter(me->ref, "SYNC", (me->param_SYNC = 1));

    /* Register everything except future timeout */
    sl_add_fd(devid, devptr, tc_fd_of_object(me->ref), SL_RD, tsycam_fd_p, NULL);

    return DEVSTATE_OPERATING;
    
 ERREXIT:
    return -1;
}

static void tsycam_tout_p(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr);

static void ScheduleTimeout(privrec_t *me, int usecs)
{
    if (usecs == 0) return;
    if (usecs > 0)
        me->act_tid = sl_enq_tout_after(me->devid, me, usecs, tsycam_tout_p, NULL);
    else
    {
        sl_deq_tout(me->act_tid);
        me->act_tid = -1;
    }
}

static void EndPullPicture(int devid, privrec_t *me)
{
  int32  retinfo[NUM_TSYCAM_PARAMS];

  DoDriverLog(devid, 0, __FUNCTION__);
  
    if (!me->pulling) return;
    me->pulling = 0;
    
    tc_decode_frame(me->ref);

    if (0)
    {
      int  i;
      int  s = 200;
        
        bzero(me->curdata, sizeof(me->curdata));
        for (i = 0; i < s; i++)
        {
            me->curdata[0][i]   = 1000;
            me->curdata[s-1][i] = 1000;
            me->curdata[i][0]   = 1000;
            me->curdata[i][s-1] = 1000;
            me->curdata[i][i]   = 500;
            me->curdata[i][s-1-i] = 500;
        }

    }
    
    //ProcessPicture();

    bzero(retinfo, sizeof(retinfo));
    retinfo[TSYCAM_PARAM_W]    = PIX_W;
    retinfo[TSYCAM_PARAM_H]    = PIX_H;
    retinfo[TSYCAM_PARAM_K]    = me->param_K;
    retinfo[TSYCAM_PARAM_T]    = me->param_T;
    retinfo[TSYCAM_PARAM_SYNC] = me->param_SYNC;
    retinfo[TSYCAM_PARAM_MISS] = tc_missing_count(me->ref);

    ReturnBigc(devid, 0, retinfo, NUM_TSYCAM_PARAMS, me->curdata, sizeof(me->curdata), sizeof(me->curdata[0][0]), 0/*!!!ERRSTATS*/);
}

static void tsycam_fd_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr)
{
  privrec_t *me = (privrec_t *)devptr;
  int        r;
  int        usecs;
    
    r = tc_fdio_proc(me->ref, &usecs);
    if (r >= 0) ScheduleTimeout(me, usecs);
    if (r == 0) /*fprintf(stderr, "%s: =0!!!\n", __FUNCTION__),*/ EndPullPicture(devid, me);
}

static void tsycam_tout_p(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr)
{
  privrec_t *me = (privrec_t *)devptr;
  int        r;
  int        usecs;
    
  DoDriverLog(devid, 0, "!!!! TIMEOUT !!!!, missing=%d", tc_missing_count(me->ref));

    me->act_tid = -1;
    r = tc_timeout_proc(me->ref, &usecs);
    if (r == 0) /*fprintf(stderr, "%s: =0!!!\n", __FUNCTION__),*/ EndPullPicture(devid, me);
    if (r >= 0) ScheduleTimeout(me, usecs);
}


#define GETPARAM(name) if (nargs > TSYCAM_PARAM_##name) \
    tc_set_parameter(me->ref, #name, (me->param_##name = args[TSYCAM_PARAM_##name]));
static void tsycam_bigc_p(int devid, void *devptr, int chan __attribute__((unused)), int32 *args, int nargs, void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  privrec_t *me = (privrec_t *)devptr;
  int        r;
  int        usecs;

    me->pulling = 1;
  
    /* Obtain parameters (if any) */
    GETPARAM(K);
    GETPARAM(T);
    GETPARAM(SYNC);

    /* Request picture */
    r = tc_request_new_frame(me->ref, &usecs);
    if (r < 0)
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "tc_request_new_frame()");
    else
        ScheduleTimeout(me, usecs);
}


DEFINE_DRIVER(tsycam, "Tsyganov's camera",
              NULL, NULL,
              0, NULL,
              0, 0,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              tsycam_init_d, NULL,
              NULL, tsycam_bigc_p);
