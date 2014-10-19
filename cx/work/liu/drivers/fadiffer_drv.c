#include <stdio.h>

#include "timeval_utils.h"

#include "cxsd_driver.h"

#include "drv_i/adc200me_drv_i.h"


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

enum
{
    BIND_HEARTBEAT_PERIOD = 2*1000000  // 2s
};

typedef struct
{
    int                   devid;

    inserver_refmetric_t  rfm;
    int                   nl;
    int                   maxpts;

    int32                *buf;
    int                   flipflop;
    int32                 result;
} privrec_t;

static psp_paramdescr_t fadiffer_params[] =
{
    PSP_P_PLUGIN("src",    privrec_t, rfm,    inserver_d_c_ref_plugin_parser, NULL),
    PSP_P_INT   ("maxpts", privrec_t, maxpts, 1024, 1, 100000),
    PSP_P_END()
};

//////////////////////////////////////////////////////////////////////

static void fadiffer_bigc_evproc(int                 devid, void *devptr,
                                 inserver_chanref_t  ref,
                                 int                 reason,
                                 void               *privptr);

static int fadiffer_init_d(int devid, void *devptr, 
                              int businfocount, int businfo[],
                              const char *auxinfo __attribute__((unused)))
{
  privrec_t      *me = (privrec_t *)devptr;
  size_t          bufsize;

    me->devid = devid;

    /* Allocate buffer */
    bufsize = sizeof(*(me->buf)) * me->maxpts * 2;
    me->buf = malloc(bufsize);
    if (me->buf == NULL) return -CXRF_DRV_PROBL;
    bzero(me->buf, bufsize);
    me->flipflop = 0;

    /* Juggle with chan_n/nl */
    me->nl = me->rfm.chan_n;
    me->rfm.chan_n = 0;
    if (me->nl < 0  ||  me->nl > 7)
    {
        DoDriverLog(devid, 0, "%s(): requested line %d out_of[0...7]",
                    __FUNCTION__, me->nl);
        return -CXRF_CFG_PROBL;
    }

    /* Register evproc */
    me->rfm.chan_is_bigc = 1;
    me->rfm.data_cb      = fadiffer_bigc_evproc;

    inserver_new_watch_list(devid, &(me->rfm), BIND_HEARTBEAT_PERIOD);

    return DEVSTATE_OPERATING;
}

static void fadiffer_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    safe_free(me->buf);
}

static void fadiffer_bigc_evproc(int                 devid, void *devptr,
                                 inserver_chanref_t  ref,
                                 int                 reason,
                                 void               *privptr)
{
  privrec_t      *me  = (privrec_t *)devptr;
  int32           numpts;  // NUMPTS from ADC
  int             todiff;  // How many points we are ready to get and process
  size_t          retdataunits;

  register int32 *p1;
  register int32 *p2;
  register int    x;
  register int32  result;

    /* Get NUMPTS */
    if (inserver_get_bigc_params(devid, ref,
                                 ADC200ME_PARAM_NUMPTS, 1, &numpts) != 1) /* ??? */
        return;

    /*  */
    todiff = numpts;
    if (todiff < 1)          todiff = 1;
    if (todiff > me->maxpts) todiff = me->maxpts;

    /* Get buffer pointers #1/#2 */
    p1 = me->buf + me->maxpts * me->flipflop; // Prev measurement
    me->flipflop = 1 - me->flipflop;
    p2 = me->buf + me->maxpts * me->flipflop; // This measurement

    /* Get data */
    inserver_get_bigc_data(devid, ref,
                           sizeof(int32) * numpts * me->nl,
                           sizeof(int32) * todiff,
                           p2,
                           &retdataunits);
    /*!!! Any checks?  Result, retdataunits? */

    /* Calc difference */
    for (x = todiff, result = 0;
         x > 0;
         x--)
        result += abs(*p2++ - *p1++) / 1000;
    me->result = result;

    ReturnChanGroup(devid, 0, 1, &(me->result), &zero_rflags);
}

static void fadiffer_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  privrec_t          *me = (privrec_t *)devptr;
  int                 n;       // channel N in values[] (loop ctl var)
  int                 x;       // channel indeX
  int32               val;     // Value

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        ReturnChanGroup(devid, x, 1, &(me->result), &zero_rflags);
    }
}


/* Metric */

static CxsdMainInfoRec fadiffer_main_info[] =
{
    {1, 0},
};
DEFINE_DRIVER(fadiffer, "LIU-2 FastAdc per-line/per-shot DIFFERence calculator",
              NULL, NULL,
              sizeof(privrec_t), fadiffer_params,
              0, 0,
              NULL, 0,
              countof(fadiffer_main_info), fadiffer_main_info,
              -1, NULL,
              fadiffer_init_d, fadiffer_term_d,
              fadiffer_rw_p, NULL);
