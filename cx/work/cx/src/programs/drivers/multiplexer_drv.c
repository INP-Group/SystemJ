#include <ctype.h>

#include "cxsd_driver.h"


enum {HEARTBEAT_PERIOD = 2*1000000};
enum {MAX_OUTS = 10};

typedef struct
{
    int                   out_count;
    inserver_refmetric_t  out_refs[MAX_OUTS];
    int32                 curval;
    int                   curval_set;
} privrec_t;


#if INSERVER2_STAT_CB_PRESENT
static void stat_evproc(int                 devid, void *devptr,
                        inserver_devref_t   ref,
                        int                 newstate,
                        void               *privptr)
{
  privrec_t      *me = (privrec_t *)devptr;
  int             n  = ptr2lint(privptr);

    if (newstate == DEVSTATE_OPERATING  &&  me->curval_set)
        inserver_snd_cval(devid, me->out_refs[n].chan_ref, me->curval);
}
#endif

static int multiplexer_init_d(int devid, void *devptr,
                              int businfocount, int businfo[],
                              const char *auxinfo)
{
  privrec_t      *me = (privrec_t *)devptr;
  const char     *p  = auxinfo;
  const char     *endp;
  int             r;

  char            outname[100];

    if (p == NULL)
    {
        DoDriverLog(devid, 0, "empty auxinfo, no channels to multiplex to; deactivating");
        return -CXRF_CFG_PROBL;
    }

    if (*p == '=')
    {
        p++;
        me->curval = strtol(p, &endp, 0);
        if (endp == p  ||  (*endp != '\0'  &&  !isspace(*endp)))
        {
            DoDriverLog(devid, 0, "error parsing default-value");
            return -CXRF_CFG_PROBL;
        }
        p = endp;
        me->curval_set = 1;
    }

    while (1)
    {
        check_snprintf(outname, sizeof(outname), "out%d", me->out_count);

        while (*p != '\0'  &&  isspace(*p)) p++;
        if (*p == '\0') break;

        if (me->out_count >= countof(me->out_refs))
        {
            DoDriverLog(devid, 0, "too many out-references; limit is %d", countof(me->out_refs));
            return -CXRF_CFG_PROBL;
        }
        DoDriverLog(devid, 0, "%s: about to parse %s from <%s>", __FUNCTION__, outname, p);

        if ((r = inserver_parse_d_c_ref(devid, outname,
                                        &p, me->out_refs + me->out_count)) < 0)
            return r;
#if INSERVER2_STAT_CB_PRESENT
        me->out_refs[me->out_count].stat_cb = stat_evproc;
#endif
        me->out_refs[me->out_count].privptr = lint2ptr(me->out_count);

        me->out_count++;
    }

    if (me->out_count == 0)
    {
        DoDriverLog(devid, 0, "no channels to multiplex to; deactivating");
        return -CXRF_CFG_PROBL;
    }

    me->out_refs[0].m_count = me->out_count;
    inserver_new_watch_list(devid, me->out_refs, HEARTBEAT_PERIOD);

#if !INSERVER2_STAT_CB_PRESENT
    {
      int  n;

        for (n = 0;  n < me->out_count;  n++)
            if (me->out_refs[n].chan_ref != INSERVER_CHANREF_ERROR)
                inserver_snd_cval(devid, me->out_refs[n].chan_ref, me->curval);
    }
#endif

    return DEVSTATE_OPERATING;
}


static void multiplexer_rw_p(int devid, void *devptr __attribute__((unused)), int firstchan, int count, int32 *values, int action)
{
  privrec_t      *me = (privrec_t *)devptr;
  int             n;
  rflags_t        rflags;
    
    if (action == DRVA_WRITE  &&  count >= 1)
    {
        me->curval     = *values;
        me->curval_set = 1;
        for (n = 0;  n < me->out_count;  n++)
            if (me->out_refs[n].chan_ref != INSERVER_CHANREF_ERROR)
                inserver_snd_cval(devid, me->out_refs[n].chan_ref, me->curval);
    }

    rflags = 0;
    ReturnChanGroup(devid, firstchan, 1, &(me->curval), &rflags);
}


static CxsdMainInfoRec multiplexer_main_info[] =
{
    {1,   1},
};
DEFINE_DRIVER(multiplexer, "Multiplexer for write-channels",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(multiplexer_main_info), multiplexer_main_info,
              -1, NULL,
              multiplexer_init_d, NULL,
              multiplexer_rw_p, NULL);
