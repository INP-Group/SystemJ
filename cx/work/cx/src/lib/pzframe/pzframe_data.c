#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "misclib.h"
#include "timeval_utils.h"

#include "cxlib.h"  // For cx_strerror()
#include "cda.h"

#include "pzframe_data.h"


psp_paramdescr_t  pzframe_data_text2cfg[] =
{
    PSP_P_FLAG   ("readonly", pzframe_cfg_t, readonly, 1, 0),
    PSP_P_INT    ("maxfrq",   pzframe_cfg_t, maxfrq,   0, 0, 100),
    PSP_P_FLAG   ("loop",     pzframe_cfg_t, running,  1, 1),
    PSP_P_FLAG   ("pause",    pzframe_cfg_t, running,  0, 0),
    PSP_P_END()
};

//////////////////////////////////////////////////////////////////////

// GetCblistSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Cblist, pzframe_data_cbinfo_t,
                                 cb, cb, NULL, (void*)1,
                                 0, 2, 0,
                                 pfr->, pfr,
                                 pzframe_data_t *pfr, pzframe_data_t *pfr)

void PzframeDataCallCBs(pzframe_data_t *pfr,
                        int             reason,
                        int             info_changed)
{
  int                  x;

    if (pfr->ftd->vmt.evproc != NULL)
        pfr->ftd->vmt.evproc(pfr, reason, info_changed, NULL);

    for (x = 0;  x < pfr->cb_list_allocd;  x++)
        if (pfr->cb_list[x].cb != NULL)
            pfr->cb_list[x].cb(pfr, reason,
                               info_changed, pfr->cb_list[x].privptr);
}

//////////////////////////////////////////////////////////////////////

static void PzframeBigcEventProc(cda_serverid_t  sid       __attribute__((unused)),
                                 int             reason,
                                 void           *privptr)
{
  pzframe_data_t      *pfr = privptr;
  pzframe_type_dscr_t *ftd = pfr->ftd;

  struct timeval       timenow;
  struct timeval       timediff;

  int                  pn;
  int                  info_changed;

    if (reason != CDA_R_MAINDATA)                  return;
    if (!(pfr->cfg.running  ||  pfr->cfg.oneshot)) return;
    gettimeofday(&timenow, NULL);
    if (pfr->cfg.oneshot == 0  &&  pfr->cfg.maxfrq != 0)
    {
        timeval_subtract(&timediff, &timenow, &(pfr->timeupd));
        if (timediff.tv_sec  == 0  &&
            timediff.tv_usec <  1000000 / pfr->cfg.maxfrq)
            return;
    }
    pfr->timeupd     = timenow;
    PzframeDataSetRunMode(pfr, -1, 0);

    cda_getbigcstats (pfr->bigc_handle, &(pfr->mes.tag), &(pfr->mes.rflags));
    cda_getbigcparams(pfr->bigc_handle, 0, ftd->num_params, pfr->mes.info);

    /* Get measured data -- request maximum */
    cda_getbigcdata(pfr->bigc_handle, 0, ftd->max_datasize, pfr->mes.databuf);

    /* Find if info had changed */
    for (pn = 0, info_changed = 0;  pn < ftd->num_params;  pn++)
        if (pfr->cfg.param_info[pn].change_important  &&
            pfr->mes.info[pn] != pfr->prv_info[pn]) info_changed = 1;
    memcpy(pfr->prv_info, pfr->mes.info, sizeof(pfr->mes.info));

    /* Call upstream */
    PzframeDataCallCBs(pfr, PZFRAME_REASON_BIGC_DATA, info_changed);
}

static void PzframeRegcEventProc(cda_serverid_t  sid       __attribute__((unused)),
                                 int             reason,
                                 void           *privptr)
{
  pzframe_data_t      *pfr = privptr;
  pzframe_type_dscr_t *ftd = pfr->ftd;

  int                  pn;

    if (reason != CDA_R_MAINDATA)                  return;

    for (pn = 0;  pn < ftd->num_params;  pn++)
        if (pfr->chan_handles[pn] != CDA_PHYSCHANHANDLE_ERROR)
            cda_getphyschanraw(pfr->chan_handles[pn],
                               pfr->chan_vals  + pn,
                               NULL, NULL);

    /* Call upstream */
    PzframeDataCallCBs(pfr, PZFRAME_REASON_REG_CHANS, 0);
}


static int DoConnect      (pzframe_data_t *pfr,
                           const char *srvrspec, int bigc_n,
                           cda_serverid_t def_regsid, int base_chan)
{
  pzframe_type_dscr_t *ftd = pfr->ftd;

  int                  pn;
  int32                param_val;

    pfr->bigc_sid = cda_new_server(srvrspec,
                                   PzframeBigcEventProc, pfr,
                                   CDA_BIGC);
    if (pfr->bigc_sid == CDA_SERVERID_ERROR)
    {
        fprintf(stderr, "%s %s: cda_new_server(server=%s): %s\n",
                strcurtime(), __FUNCTION__, srvrspec, cx_strerror(errno));
        return -1;
    }
    pfr->bigc_srvcount = cda_status_lastn(pfr->bigc_sid) + 1;

    pfr->bigc_handle = cda_add_bigc(pfr->bigc_sid, bigc_n,
                                    ftd->num_params, ftd->max_datasize,
                                    pfr->cfg.readonly? CX_CACHECTL_SNIFF
                                                     : CX_CACHECTL_SHARABLE,
                                    CX_BIGC_IMMED_YES);

    if (!pfr->cfg.readonly)
        for (pn = 0;  pn < ftd->num_params;  pn++)
            if ((param_val = pfr->cfg.param_iv.p[pn]) >= 0)
            {
                param_val = (
                             pfr->cfg.param_iv.p[pn] + 
                             pfr->cfg.param_info[pn].phys_d
                            ) * pfr->cfg.param_info[pn].phys_r;
                cda_setbigcparams(pfr->bigc_handle, pn, 1, &param_val);
                ////fprintf(stderr, "[%x][%d]=%d\n", pfr->bigc_handle, pn, param_val);
            }

    if (pfr->cfg.running  ||  pfr->cfg.oneshot)
        cda_run_server(pfr->bigc_sid);

    if (base_chan >= 0)
    {
        pfr->chan_base = base_chan;
        if (def_regsid == CDA_SERVERID_ERROR)
        {
            pfr->chan_sid = cda_new_server(srvrspec,
                                           PzframeRegcEventProc, pfr,
                                           CDA_REGULAR);
            cda_run_server(pfr->chan_sid);
        }
        else
        {
            pfr->chan_sid = def_regsid;
            cda_add_evproc(pfr->chan_sid, PzframeRegcEventProc, pfr);
        }
        for (pn = 0;  pn < ftd->num_params;  pn++)
            if ((pfr->cfg.param_info[pn].type  & 0xFFFF) != PZFRAME_PARAM_BIGC)
                pfr->chan_handles[pn] = cda_add_physchan(pfr->chan_sid,
                                                         pfr->chan_base + pn);
            else
                pfr->chan_handles[pn] = CDA_PHYSCHANHANDLE_ERROR;
    }
    else
    {
        pfr->chan_sid = CDA_SERVERID_ERROR;
        pfr->chan_base = -1;
        for (pn = 0;  pn < ftd->num_params;  pn++)
            pfr->chan_handles[pn] = CDA_PHYSCHANHANDLE_ERROR;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

void PzframeDataInit      (pzframe_data_t *pfr, pzframe_type_dscr_t *ftd)
{
  int  pn;
  int  x;

    bzero(pfr, sizeof(*pfr));
    pfr->ftd = ftd;
    pfr->bigc_sid = pfr->chan_sid = CDA_SERVERID_ERROR;
    
    for (pn = 0;  pn < ftd->num_params;  pn++)
    {
        /* Set default phys_r=1 */
        pfr->cfg.param_info[pn].phys_r = 1;

        /* Pre-set init_params to "not specified" */
        pfr->cfg.param_iv.p[pn]        = -1;
        /* ...and previous-info to "insane" */
        pfr->prv_info      [pn]        = -1;
    }

    if (ftd->param_dscrs != NULL)
        for (x = 0;
             ftd->param_dscrs[x].name != NULL;
             x++)
        {
            pn = ftd->param_dscrs[x].n;
            if (pn >= 0  &&  pn < ftd->num_params)
            {
                if (ftd->param_dscrs[x].phys_r != 0)
                    pfr->cfg.param_info[pn].phys_r = ftd->param_dscrs[x].phys_r;
                pfr->    cfg.param_info[pn].phys_d = ftd->param_dscrs[x].phys_d;
                pfr->    cfg.param_info[pn].change_important =
                                                     ftd->param_dscrs[x].change_important;
                pfr->    cfg.param_info[pn].type   = ftd->param_dscrs[x].param_type;
            }
        }
}

int  PzframeDataRealize   (pzframe_data_t *pfr,
                           const char *srvrspec, int bigc_n,
                           cda_serverid_t def_regsid, int base_chan)
{
  pzframe_type_dscr_t *ftd = pfr->ftd;

  int                  r;

    /* Allocate buffers */
    if ((ftd->behaviour & PZFRAME_B_NO_ALLOC) == 0)
    {
        pfr->mes.databuf = malloc(ftd->max_datasize);
        if (pfr->mes.databuf == NULL)
        {
            fprintf(stderr, "%s %s(type=%s): unable to allocate %zu*%d=%zu bytes for (mes)databuf",
                    strcurtime(), __FUNCTION__, ftd->type_name,
                    ftd->dataunits, ftd->max_datacount, ftd->max_datasize);
            return -1;
        }

        if ((ftd->behaviour & PZFRAME_B_NO_SVD) == 0)
        {
#if 0
            adc->svd.databuf = malloc(atd->max_datasize);
            if (adc->svd.databuf == NULL)
            {
                safe_free(adc->mes.databuf); adc->mes.databuf = NULL;
                fprintf(stderr, "%s %s(type=%s): unable to allocate %d*%d*%zu=%zu bytes for svd.databuf",
                        strcurtime(), __FUNCTION__, atd->type_name,
                        atd->max_numpts, atd->num_lines, atd->dataunits, atd->max_datasize);
                return -1;
            }
#endif
        }
    }

    if ((pfr->ftd->behaviour & PZFRAME_B_ARTIFICIAL) == 0)
    {
        if ((r = DoConnect(pfr,
                           srvrspec, bigc_n,
                           def_regsid, base_chan)) < 0) return r;
    }

    return 0;
}

int  PzframeDataAddEvproc (pzframe_data_t *pfr,
                           pzframe_data_evproc_t cb, void *privptr)
{
  int                    id;
  pzframe_data_cbinfo_t *slot;

    if (cb == NULL) return 0;

    id = GetCblistSlot(pfr);
    if (id < 0) return -1;
    slot = AccessCblistSlot(id, pfr);

    slot->cb      = cb;
    slot->privptr = privptr;

    return 0;
}

void PzframeDataSetRunMode(pzframe_data_t *pfr,
                           int running, int oneshot)
{
    /* Default to current values */
    if (running < 0) running = pfr->cfg.running;
    if (oneshot < 0) oneshot = pfr->cfg.oneshot;

    /* Sanitize */
    running = running != 0;
    oneshot = oneshot != 0;

    /* Had anything changed? */
    if (running == pfr->cfg.running  &&
        oneshot == pfr->cfg.oneshot)
        return;

    /* Store */
    pfr->cfg.running = running;
    pfr->cfg.oneshot = oneshot;

    /* Reflect state */
    if (pfr->bigc_sid == CDA_SERVERID_ERROR) return;
    if (running  ||  oneshot)
        cda_run_server(pfr->bigc_sid);
    else
        cda_hlt_server(pfr->bigc_sid);
}

//////////////////////////////////////////////////////////////////////

int  PzframeDataFdlgFilter(const char *filespec,
                           time_t *cr_time,
                           char *commentbuf, size_t commentbuf_size,
                           void *privptr)
{
  pzframe_data_t      *pfr = privptr;

    if (pfr->ftd->vmt.filter != NULL  &&
        pfr->ftd->vmt.filter(pfr, filespec,
                             cr_time,
                             commentbuf, commentbuf_size) == 0)
        return 0;
    else
        return 1;
}
