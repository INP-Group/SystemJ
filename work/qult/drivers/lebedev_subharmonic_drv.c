#include <stdio.h>

#include "timeval_utils.h"

#include "cxsd_driver.h"
#include "drv_i/lebedev_subharmonic_drv_i.h"


static rflags_t  zero_rflags = 0;

enum
{
    BIND_HEARTBEAT_PERIOD = 2*1000000  // 2s
};


typedef struct
{
    int                 devid;

    char                targ_name[32];
    inserver_devref_t   targ_ref;
    int                 targ_state;
    inserver_chanref_t  sodc_ref;
} privrec_t;


//////////////////////////////////////////////////////////////////////

static void WipeRefs(privrec_t *me)
{
    me->targ_state = DEVSTATE_NOTREADY;
    me->targ_ref   = INSERVER_DEVREF_ERROR;
    me->sodc_refs  = INSERVER_CHANREF_ERROR;
}

static void ReportState(privrec_t *me)
{
  int state = me->targ_state;

    if (state == DEVSTATE_OFFLINE) state = DEVSTATE_NOTREADY;
    SetDevState(me->devid, state, 0, state == DEVSTATE_NOTREADY? "target NOTREADY":NULL);
}


static void stat_evproc(int                 devid, void *devptr,
                        inserver_devref_t   ref,
                        int                 newstate,
                        void               *privptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    me->targ_state = newstate;
    ReportState(me);
    if      (newstate == DEVSTATE_OFFLINE)
    {
        SetIstState(me, IST_STATE_UNKNOWN);
        WipeRefs(me);
    }
    else if (newstate == DEVSTATE_NOTREADY)
    {
        if (me->ist_state != IST_STATE_UNKNOWN)
            SetIstState(me, IST_STATE_DETERMINE);
    }
}

static void sodc_evproc(int                 devid, void *devptr,
                        inserver_chanref_t  ref,
                        int                 reason,
                        void               *privptr)
{
  privrec_t       *me   = (privrec_t *)devptr;
  int              sodc = ptr2lint(privptr);
  int              mode = sodc_mapping[sodc].mode;

  struct timeval   now;
  state_descr_t   *curs = state_descr + me->ist_state;
  state_descr_t   *nxts;

  int32            val;
  int              alli;
  int              x;

    inserver_get_cval(devid, ref,
                      me->sodc_vals + sodc, me->sodc_flgs + sodc, NULL);
    me->sodc_rcvd[sodc] = 1;
    val = me->sodc_vals[sodc];

    /* A special case -- derive OUT from OUT_CUR upon start */
    if (sodc == C20C_OUT_CUR  &&  !me->out_val_set)
    {
        me->out_val     = val;
        me->out_val_set = 1; /* Yes, we do it only ONCE, since later OUT_CUR may change upon our request, without any client's write */
        ReturnChanGroup(devid, IST_XCDAC20_CHAN_OUT, 1, &(me->out_val), &zero_rflags);
    }

    /* Perform tubbing */
    if (mode & SODC_TUBE)
        ReturnChanGroup(devid, sodc_mapping[sodc].ourc, 1,
                        me->sodc_vals + sodc, me->sodc_flgs + sodc);

    /* Check if we are ready to operate */
    if (mode & SODC_IMPR  &&  me->ist_state == IST_STATE_UNKNOWN)
    {
        for (x = 0,  alli = 1;
             x < countof(me->sodc_rcvd);
             x++)
            if (sodc_mapping[x].mode & SODC_IMPR)
                //fprintf(stderr, "\tch20=%d/%d:%c\n", x, sodc_mapping[x].ourc, me->sodc_rcvd[x]? '+':'-'),
                alli &= me->sodc_rcvd[x];

        if (alli)
            SetIstState(me, IST_STATE_DETERMINE);
    }

    /* If not all info is gathered yet, then there's nothing else to do yet */
    if (me->ist_state == IST_STATE_UNKNOWN) return;

    /* In fact, is a hack... */
    if (me->ist_state == IST_STATE_DETERMINE)
    {
        if (me->sodc_vals[C20C_OPR])
            SetIstState(me, IST_STATE_IS_ON);
        else
            SetIstState(me, IST_STATE_IS_OFF);
    }
    
    /* Perform state transitions depending on received data and/or current state */
    if (sodc >= C20C_ILK_base  &&
        sodc <  C20C_ILK_base + C20C_ILK_count  &&
        val != 0  &&
        ( // Lock-unaffected states
         me->ist_state != IST_STATE_RST_ILK_SET  &&
         me->ist_state != IST_STATE_RST_ILK_DRP  &&
         me->ist_state != IST_STATE_RST_ILK_CHK
        )
       )
    {
        SetIstState(me, IST_STATE_INTERLOCK);
    }
    else if (/* There is a state to switch to */
             curs->next_state >= 0
             &&
             /* ...and no delay or delay has expired */
             (curs->delay_us <= 0  ||
              (gettimeofday(&(now), NULL),
               TV_IS_AFTER(now, me->next_state_time))))
    {
        nxts = state_descr + curs->next_state;
        if (curs->next_state == IST_STATE_SW_OFF_CHK_I  &&  sodc == C20C_OUT_CUR) fprintf(stderr, " nxts=%d sodc=%d, impr=%d is_alwd=%p,%p\n", IST_STATE_SW_OFF_CHK_I, sodc, interesting_chan[curs->next_state][sodc], nxts->sw_alwd, IsAlwdSW_OFF_CHK_I);
        if (/* This channel IS is "interesting"... */
            interesting_chan[curs->next_state][sodc] &&
            /* ...and condition is met */
            nxts->sw_alwd != NULL  &&
            nxts->sw_alwd(me, me->ist_state))
        {
            SetIstState(me, curs->next_state);
        }
    }
}

static void req_chan(privrec_t *me, int sodc)
{
  tag_t            age;

    if (me->sodc_isrw[sodc])
    {
        inserver_get_cval(me->devid, me->sodc_refs[sodc],
                          NULL, NULL, &age);
        if (age == 0) // We DON'T try to return non-fresh data -- rather wait till it becomes really fresh
            sodc_evproc(me->devid, me,
                        me->sodc_refs[sodc],
                        0,
                        lint2ptr(sodc));
        else
            fprintf(stderr, "\t\tread(%d/%d/%d): age=%d\n", sodc, sodc_mapping[sodc].ourc, me->sodc_refs[sodc], age);
    }
    else
        inserver_req_cval(me->devid, me->sodc_refs[sodc]);
}

static void lebedev_subharmonic_bind_hbt(int devid, void *devptr,
                                         sl_tid_t tid,
                                         void *privptr)
{
  privrec_t       *me = (privrec_t *)devptr;
  
  int              sodc;

    if (me->targ_ref == INSERVER_DEVREF_ERROR)
    {
        me->targ_ref = inserver_get_devref(devid, me->targ_name);
        if (me->targ_ref != INSERVER_DEVREF_ERROR)
        {
            inserver_get_devstat(devid, me->targ_ref, &(me->targ_state));
            inserver_add_devstat_evproc(devid, me->targ_ref, stat_evproc, NULL);
            for (sodc = 0;  sodc < countof(sodc_mapping);  sodc++)
                if (sodc_mapping[sodc].mode != 0)
                {
                    me->sodc_refs[sodc] =
                        inserver_get_chanref(devid, me->targ_name, sodc);
                    me->sodc_isrw[sodc] = inserver_chan_is_rw(devid, me->sodc_refs[sodc]);
                    inserver_add_chanref_evproc(devid,
                                                me->sodc_refs[sodc],
                                                sodc_evproc, lint2ptr(sodc));
                    req_chan(me, sodc);
                }
            ReportState(me);
        }
    }

    sl_enq_tout_after(devid, devptr, BIND_HEARTBEAT_PERIOD, lebedev_subharmonic_bind_hbt, NULL);
}

static int lebedev_subharmonic_init_d(int devid, void *devptr, 
                                      int businfocount, int businfo[],
                                      const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;
  const char     *p  = auxinfo;

    me->devid = devid;

    if (p == NULL  ||  *p == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s(): base-XCDAC20-device name is required in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    strzcpy(me->targ_name, auxinfo, sizeof(me->targ_name));
    SetIstState(me, IST_STATE_UNKNOWN);
    WipeRefs(me);
    lebedev_subharmonic_bind_hbt(devid, me, 0, NULL);

    return DEVSTATE_OPERATING;
}

static void lebedev_subharmonic_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  privrec_t      *me = (privrec_t *)devptr;
  int32           val;     // Value

    if (action == DRVA_WRITE)
    {
        val = values[0];
    }
    else
    {
    }

}


/* Metric */

static CxsdMainInfoRec lebedev_subharmonic_main_info[] =
{
    {LEBEDEV_SUBHARMONIC_NUMCHANS, 1},
};
DEFINE_DRIVER(lebedev_subharmonic, "Lebedev's Subharmonic",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(lebedev_subharmonic_main_info), lebedev_subharmonic_main_info,
              -1, NULL,
              lebedev_subharmonic_init_d, NULL,
              lebedev_subharmonic_rw_p, NULL);
