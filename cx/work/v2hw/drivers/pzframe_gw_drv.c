#include <stdio.h>

#include "cxlib.h"
#include "cda.h"

#include "cxsd_driver.h"


static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

typedef struct
{
    int                   devid;

    const char           *src;
    //char           src[100];
    int                   base_chan;
    size_t                dataunits;
    int                   datacount;
    size_t                datasize;
    int                   nargs;
    int                   cachectl;

    cda_serverid_t        bigc_sid;
    cda_bigchandle_t      bigc_handle;
    void                 *databuf;

    cda_serverid_t        chan_sid;
    int                   chan_base;
    cda_physchanhandle_t  chan_handles [CX_MAX_BIGC_PARAMS];
} pzframe_gw_privrec_t;

static psp_paramdescr_t pzframe_gw_params[] =
{
    PSP_P_MSTRING("src",    pzframe_gw_privrec_t, src,       "?", 100),
    //PSP_P_STRING("src",    pzframe_gw_privrec_t, src,       "?"),
    PSP_P_INT    ("chan",   pzframe_gw_privrec_t, chan_base, -1, -1, 100000),

    PSP_P_FLAG("b",         pzframe_gw_privrec_t, dataunits, sizeof(int8),  0),
    PSP_P_FLAG("s",         pzframe_gw_privrec_t, dataunits, sizeof(int16), 0),
    PSP_P_FLAG("i",         pzframe_gw_privrec_t, dataunits, sizeof(int32), 1),

    PSP_P_INT ("datacount", pzframe_gw_privrec_t, datacount, 0, 0, 2 << 21 /*=CX_ABSOLUTE_MAX_DATASIZE*/),

    PSP_P_INT ("nargs",     pzframe_gw_privrec_t, nargs,     0, 0, CX_MAX_BIGC_PARAMS),
    
    PSP_P_FLAG("sharable",  pzframe_gw_privrec_t, cachectl,  CX_CACHECTL_SHARABLE,  1),
    PSP_P_FLAG("force",     pzframe_gw_privrec_t, cachectl,  CX_CACHECTL_FORCE,     0),
    PSP_P_FLAG("sniff",     pzframe_gw_privrec_t, cachectl,  CX_CACHECTL_SNIFF,     0),
    PSP_P_FLAG("fromcache", pzframe_gw_privrec_t, cachectl,  CX_CACHECTL_FROMCACHE, 0),

    PSP_P_END()
};

//////////////////////////////////////////////////////////////////////

static void PzframeGwBigcEventProc(cda_serverid_t  sid       __attribute__((unused)),
                                   int             reason,
                                   void           *privptr)
{
  pzframe_gw_privrec_t    *me = (pzframe_gw_privrec_t *) privptr;

  rflags_t                 rflags;
  int32                    info[CX_MAX_BIGC_PARAMS];
  size_t                   l;

    if (reason != CDA_R_MAINDATA)                  return;

    cda_getbigcstats   (me->bigc_handle, NULL, &rflags);
    cda_getbigcparams  (me->bigc_handle, 0, me->nargs, info);
    l = cda_getbigcdata(me->bigc_handle, 0, me->datasize, me->databuf);
    ReturnBigc(me->devid, 0, info, me->nargs, me->databuf, l, me->dataunits, rflags);
}

static void PzframeGwRegcEventProc(cda_serverid_t  sid       __attribute__((unused)),
                                   int             reason,
                                   void           *privptr)
{
  pzframe_gw_privrec_t    *me = (pzframe_gw_privrec_t *) privptr;

  int                      pn;
  int32                    val;
  rflags_t                 rflags;

    if (reason != CDA_R_MAINDATA)                  return;

    for (pn = 0;  pn < me->nargs;  pn++)
    {
        cda_getphyschanraw(me->chan_handles[pn], &val, NULL, &rflags);
        ReturnChanGroup(me->devid, pn, 1, &val, &rflags);
    }
}

static int pzframe_gw_init_d(int devid, void *devptr,
                             int businfocount, int businfo[],
                             const char *auxinfo)
{
  pzframe_gw_privrec_t    *me = (pzframe_gw_privrec_t *) devptr;
  int                      pn;

  char                     srvrspec[200];
  int                      bigc_n;

    me->devid    = devid;
    me->datasize = me->dataunits * me->datacount;

    me->chan_sid = CDA_SERVERID_ERROR;
    for (pn = 0;  pn < countof(me->chan_handles);  pn++)
        me->chan_handles[pn] = CDA_PHYSCHANHANDLE_ERROR;

    me->databuf = malloc(me->datasize);
    if (me->databuf == NULL)
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "malloc(%d) fail",
                    me->datasize);
        return -CXRF_DRV_PROBL;
    }

    if (cx_parse_chanref(me->src,
                         srvrspec, sizeof(srvrspec), &bigc_n) < 0)
    {
        DoDriverLog(devid, 0, "invalid CX big-channel reference \"%s\"",
                    me->src);
        return -CXRF_CFG_PROBL;
    }
    me->bigc_sid = cda_new_server(srvrspec,
                                  PzframeGwBigcEventProc, me,
                                  CDA_BIGC);
    if (me->bigc_sid == CDA_SERVERID_ERROR)
    {
        DoDriverLog(devid, 0, "cda_new_server(server=%s): %s",
                    srvrspec, cx_strerror(errno));
        return -1;
    }
    cda_run_server(me->bigc_sid);

    me->bigc_handle = cda_add_bigc(me->bigc_sid, bigc_n,
                                   me->nargs, me->datasize,
                                   me->cachectl, CX_BIGC_IMMED_YES);

    if (me->base_chan >= 0)
    {
        me->chan_sid = cda_new_server(srvrspec,
                                      PzframeGwRegcEventProc, me,
                                      CDA_REGULAR);
        cda_run_server(me->chan_sid);

        for (pn = 0;  pn < me->nargs;  pn++)
            me->chan_handles[pn] = cda_add_physchan(me->chan_sid,
                                                    me->chan_base + pn);
    }

    return DEVSTATE_OPERATING;
}

static void pzframe_gw_term_d(int devid, void *devptr)
{
  pzframe_gw_privrec_t *me = (pzframe_gw_privrec_t *) devptr;

    cda_del_server(me->bigc_sid); /*!!! Implement it in cda!!!*/
    cda_del_server(me->chan_sid);
    safe_free(me->databuf);
}

static void pzframe_gw_rw_p(int devid, void *devptr,
                            int firstchan, int count, int *values, int action)
{
  pzframe_gw_privrec_t *me = (pzframe_gw_privrec_t *) devptr;
  int              n;     // channel N in values[] (loop ctl var)
  int              x;     // channel indeX
  int32            val;

    if (me->chan_sid == CDA_SERVERID_ERROR) return;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if (x >= countof(me->chan_handles))
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
            return;
        }

        if (action == DRVA_WRITE)
            cda_setphyschanraw(me->chan_handles[x], val);
        /* No need to process DRVA_READ -- all is returned from evproc */
    }
}

static void pzframe_gw_bigc_p(int devid, void *devptr,
                              int bigc,
                              int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
  pzframe_gw_privrec_t *me = (pzframe_gw_privrec_t *) devptr;

    cda_setbigcparams(me->bigc_handle, 0, nargs, args);
}

DEFINE_DRIVER(pzframe_gw,  "Pzframe Gateway",
              NULL, NULL,
              sizeof(pzframe_gw_privrec_t), pzframe_gw_params,
              0, 20, /*!!! CXSD_DB_MAX_BUSINFOCOUNT */
              NULL, 0,
              -1, NULL,
              -1, NULL,
              pzframe_gw_init_d, pzframe_gw_term_d,
              pzframe_gw_rw_p, pzframe_gw_bigc_p);
