#include <ctype.h>

#include "cxsd_driver.h"


enum {HEARTBEAT_PERIOD = 2*1000000};
enum {MAX_OUTS = 10};

typedef struct
{
    int                   out_count;
    inserver_refmetric_t  out_refs[MAX_OUTS];
    int32                 out_vals[MAX_OUTS];
} privrec_t;


static int trig_write_init_d(int devid, void *devptr,
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
        DoDriverLog(devid, 0, "empty auxinfo, no channels to write to; deactivating");
        return -CXRF_CFG_PROBL;
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

        if (*p != '=')
        {
            DoDriverLog(devid, 0, "'=' expected after channel-reference for %s spec", outname);
            return -CXRF_CFG_PROBL;
        }
        p++;

        me->out_vals[me->out_count] = strtol(p, &endp, 0);
        if (endp == p  ||  (*endp != '\0'  &&  !isspace(*endp)))
        {
            DoDriverLog(devid, 0, "error parsing %s value", outname);
            return -CXRF_CFG_PROBL;
        }
        p = endp;
        
        me->out_count++;
    }

    if (me->out_count == 0)
    {
        DoDriverLog(devid, 0, "no channels to write to; deactivating");
        return -CXRF_CFG_PROBL;
    }

    me->out_refs[0].m_count = me->out_count;
    inserver_new_watch_list(devid, me->out_refs, HEARTBEAT_PERIOD);

    return DEVSTATE_OPERATING;
}


static void trig_write_rw_p(int devid, void *devptr __attribute__((unused)), int firstchan, int count, int32 *values, int action)
{
  privrec_t      *me = (privrec_t *)devptr;
  int             n;
  int32           val;
  rflags_t        rflags;
    
    if (action == DRVA_WRITE  &&  count >= 1  &&  *values == CX_VALUE_COMMAND)
        for (n = 0;  n < me->out_count;  n++)
            if (me->out_refs[n].chan_ref != INSERVER_CHANREF_ERROR)
                inserver_snd_cval(devid, me->out_refs[n].chan_ref, me->out_vals[n]);

    val    = 0;
    rflags = 0;
    ReturnChanGroup(devid, firstchan, 1, &val, &rflags);
}


static CxsdMainInfoRec trig_write_main_info[] =
{
    {1,   1},
};
DEFINE_DRIVER(trig_write, "Triggered-write (to multiple channels) driver",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(trig_write_main_info), trig_write_main_info,
              -1, NULL,
              trig_write_init_d, NULL,
              trig_write_rw_p, NULL);
