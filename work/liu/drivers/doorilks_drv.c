#include <stdio.h>

#include "timeval_utils.h"

#include "cxsd_driver.h"
#include "drv_i/doorilks_drv_i.h"
#include "drv_i/xceac124_drv_i.h"


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

enum
{
    BIND_HEARTBEAT_PERIOD = 2*1000000  // 2s
};

typedef struct
{
    int                   devid;

    inserver_refmetric_t  rms      [DOORILKS_CHAN_ILK_count];
    int32                 ilks_vals[DOORILKS_CHAN_ILK_count];
    rflags_t              ilks_flgs[DOORILKS_CHAN_ILK_count];
    int32                 mode;
    int32                 used_vals[DOORILKS_CHAN_ILK_count];
} privrec_t;

static int32 modes[][DOORILKS_CHAN_ILK_count] =
{
    {0, 0, 0, 0},  // None
    {1, 1, 1, 1},  // Work
};


//////////////////////////////////////////////////////////////////////

static void doorilks_data_evproc(int                 devid, void *devptr,
                                 inserver_chanref_t  ref,
                                 int                 reason,
                                 void               *privptr);

static int doorilks_init_d(int devid, void *devptr, 
                              int businfocount, int businfo[],
                              const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;
  const char     *p  = auxinfo;
  int             ilk;

    me->devid = devid;

    if (p == NULL  ||  *p == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s(): base-XCEAC124-device name is required in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    strzcpy(me->rms[0].targ_name, auxinfo, sizeof(me->rms[0].targ_name));

    for (ilk = 0;  ilk < DOORILKS_CHAN_ILK_count;  ilk++)
    {
        if (ilk != 0) memcpy(me->rms[ilk].targ_name, me->rms[0].targ_name,
                           sizeof(me->rms[0].targ_name));
        me->rms[ilk].chan_n  = XCEAC124_CHAN_REGS_RD8_BASE + ilk;
        me->rms[ilk].data_cb = doorilks_data_evproc;
        me->rms[ilk].privptr = lint2ptr(ilk);
    }

    me->rms[0].m_count = DOORILKS_CHAN_ILK_count;
    inserver_new_watch_list(devid, me->rms, BIND_HEARTBEAT_PERIOD);

    return DEVSTATE_OPERATING;
}

static void calc_ilk_state(privrec_t *me)
{
  int32     val;
  rflags_t  rflags;
  int       ilk;

    for (ilk = 0, val = 1, rflags = 0;
         ilk < DOORILKS_CHAN_ILK_count;
         ilk++)
        if (me->used_vals[ilk])
        {
            val    &= me->ilks_vals[ilk];
            rflags |= me->ilks_flgs[ilk];
        }

    ReturnChanGroup(me->devid, DOORILKS_CHAN_SUM_STATE, 1, &val, &rflags);
}

static void doorilks_data_evproc(int                 devid, void *devptr,
                                 inserver_chanref_t  ref,
                                 int                 reason,
                                 void               *privptr)
{
  privrec_t      *me  = (privrec_t *)devptr;
  int             ilk = ptr2lint(privptr);

    inserver_get_cval(devid, ref,
                      me->ilks_vals + ilk,
                      me->ilks_flgs + ilk,
                      NULL);
    ReturnChanGroup(me->devid, DOORILKS_CHAN_ILK_base + ilk, 1,
                    me->ilks_vals + ilk, me->ilks_flgs + ilk);
    calc_ilk_state(me);
}

static void doorilks_rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  privrec_t          *me = (privrec_t *)devptr;
  int                 n;       // channel N in values[] (loop ctl var)
  int                 x;       // channel indeX
  int32               val;     // Value
  int                 ilk;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x == DOORILKS_CHAN_SUM_STATE  ||
                 (x >= DOORILKS_CHAN_ILK_base  &&
                  x <  DOORILKS_CHAN_ILK_base + DOORILKS_CHAN_ILK_count))
        {
            // Do-nothing -- those are returned upon events
        }
        else if (x == DOORILKS_CHAN_MODE)
        {
            if (action == DRVA_WRITE)
            {
                if (val < 0)                  val = 0;
                if (val > countof(modes) - 1) val = countof(modes) - 1;

                if (me->mode != val)
                {
                    me->mode = val;
                    for (ilk = 0;  ilk < DOORILKS_CHAN_ILK_count;  ilk++)
                    {
                        me->used_vals[ilk] = modes[val][ilk];
                        ReturnChanGroup(devid,
                                        DOORILKS_CHAN_USED_base + ilk, 1,
                                        me->used_vals + ilk, &zero_rflags);
                    }
                    calc_ilk_state(me);
                }
            }

            ReturnChanGroup(devid, x, 1, &(me->mode), &zero_rflags);
        }
        else if (x >= DOORILKS_CHAN_USED_base  &&
                 x <  DOORILKS_CHAN_USED_base + DOORILKS_CHAN_ILK_count)
        {
            ilk = x - DOORILKS_CHAN_USED_base;
            if (action == DRVA_WRITE)
            {
                me->used_vals[ilk] = (val != 0);
                calc_ilk_state(me);
            }
            ReturnChanGroup(devid, x, 1, me->used_vals + ilk, &zero_rflags);
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec doorilks_main_info[] =
{
    {DOORILKS_CHAN_WR_count, 1},
    {DOORILKS_CHAN_RD_count, 0},
};
DEFINE_DRIVER(doorilks, "LIU-2 Doors Interlocks",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(doorilks_main_info), doorilks_main_info,
              -1, NULL,
              doorilks_init_d, NULL,
              doorilks_rw_p, NULL);
