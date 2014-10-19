#include "cxsd_driver.h"
#include "timeval_utils.h"

#include "drv_i/softwform_drv_i.h"

enum {HEARTBEAT_PERIOD = 2*1000000};
enum
{
    DEF_EXEC_PERIOD = 100000
};

typedef struct
{

    int32                 period;
    int32                 maxpts;
    inserver_refmetric_t  target;

    int32                 cur_x;
    int32                 numpts;

    sl_tid_t              tid;
    struct timeval        nwts;  // Next-Write TimeStamp

    SOFTWFORM_DATUM_T     data[0];
} privrec_t;

typedef struct
{
    int32                 period;
    int32                 maxpts;
    inserver_refmetric_t  target;
} softwform_opts_t;

static psp_paramdescr_t softwform_text2opts[] =
{
    PSP_P_PLUGIN("target", softwform_opts_t, target, inserver_d_c_ref_plugin_parser, NULL),
    PSP_P_INT   ("maxpts", softwform_opts_t, maxpts, 1024, 1, SOFTWFORM_MAX_NUMPTS),
    PSP_P_INT   ("period", softwform_opts_t, period, DEF_EXEC_PERIOD/1000, 1, 1000000),
    PSP_P_END()
};

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;


static void HeartBeat(int devid, void *devptr,
                      sl_tid_t tid,
                      void *privptr)
{
  privrec_t           *me = (privrec_t *) devptr;

    me->tid = -1;

        fprintf(stderr, "BEAT %d\n", me->cur_x);
    if (me->cur_x >= 0)
    {
        fprintf(stderr, "BEAT %d\n", me->cur_x);
        
        if (me->target.chan_ref != INSERVER_CHANREF_ERROR)
        {
            inserver_snd_cval(devid, me->target.chan_ref, me->data[me->cur_x]);
            me->cur_x++;
            if (me->cur_x >= me->numpts)
                me->cur_x = -1;
        }
        else
            me->cur_x = -1;
    }

    timeval_add_usecs(&(me->nwts), &(me->nwts), me->period * 1000);
    me->tid = sl_enq_tout_at(devid, devptr, &(me->nwts), HeartBeat, NULL);
}

static int softwform_init_d(int devid, void *devptr,
                            int businfocount, int businfo[],
                            const char *auxinfo)
{
  privrec_t        *me;
  softwform_opts_t  opts;

    bzero(&opts, sizeof(opts));
    if (psp_parse(auxinfo, NULL,
                  &(opts),
                  '=', " \t", "",
                  softwform_text2opts) != PSP_R_OK)
    {
        DoDriverLog(devid, 0, "psp_parse(auxinfo): %s", psp_error());
        return -CXRF_CFG_PROBL;
    }

    me = malloc(sizeof(*me) + opts.maxpts * sizeof(SOFTWFORM_DATUM_T));
    if (me == NULL) return -CXRF_DRV_PROBL;
    bzero(me, sizeof(*me));

    me->period = opts.period;
    me->maxpts = opts.maxpts;
    me->target = opts.target;

    me->cur_x  = -1;

    RegisterDevPtr(devid, me);

    inserver_new_watch_list(devid, &(me->target), HEARTBEAT_PERIOD);

    gettimeofday(&(me->nwts), NULL);
    HeartBeat(devid, me, -1, NULL);

    return DEVSTATE_OPERATING;
}

static void softwform_term_d(int devid, void *devptr)
{
    safe_free(devptr);
}

static void softwform_rw_p(int devid, void *devptr __attribute__((unused)), int firstchan, int count, int32 *values, int action)
{
  privrec_t        *me = (privrec_t *)devptr;
  int               n;     // channel N in values[] (loop ctl var)
  int               x;     // channel indeX
  int32             val;   // Value

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if (action == DRVA_WRITE) fprintf(stderr, "ch %d=%d\n", x, val);
        if      (x == SOFTWFORM_CHAN_START)
        {
            if (action == DRVA_WRITE) fprintf(stderr, "START\n");
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND  &&
                me->cur_x < 0 /* Don't try to start already running table */  &&
                me->numpts > 0)
            {
                me->cur_x = 0;
                if (me->tid >= 0) sl_deq_tout(me->tid);
                HeartBeat(devid, me, -1, NULL);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == SOFTWFORM_CHAN_STOP)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                me->cur_x = -1;
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == SOFTWFORM_CHAN_CUR_X)
        {
            ReturnChanGroup(devid, x, 1, &(me->cur_x),  &zero_rflags);
        }
        else if (x == SOFTWFORM_CHAN_NUMPTS)
        {
            ReturnChanGroup(devid, x, 1, &(me->numpts), &zero_rflags);
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}

static void softwform_bigc_p(int devid, void *devptr,
                             int bigc,
                             int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
  privrec_t        *me = (privrec_t *)devptr;

    /* If anything is wrong, don't modify table and treat request as "read" */
    if (me->cur_x < 0  &&
        datasize  > 0  &&
        dataunits == sizeof(SOFTWFORM_DATUM_T))
    {
        me->numpts = datasize / sizeof(SOFTWFORM_DATUM_T);
        if (me->numpts > me->maxpts)
            me->numpts = me->maxpts;
        memcpy(me->data, data, me->numpts * sizeof(SOFTWFORM_DATUM_T));
    }
    ReturnBigc(devid, bigc,
               &(me->numpts), 1,
               me->data, sizeof(SOFTWFORM_DATUM_T) * me->numpts, sizeof(SOFTWFORM_DATUM_T), 0);
}

static CxsdMainInfoRec softwform_main_info[] =
{
    {10,   1},
    {10,   0},
};
DEFINE_DRIVER(softwform, "Software waveform-DAC implementation",
              NULL, NULL,
              0, NULL,
              0, 0,
              NULL, 0,
              countof(softwform_main_info), softwform_main_info,
              -1, NULL,
              softwform_init_d, softwform_term_d,
              softwform_rw_p, softwform_bigc_p);
