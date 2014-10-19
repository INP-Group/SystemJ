#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "misc_macros.h"
#include "cxscheduler.h"

#include "cxlib.h"
#include "cda.h"
#include "cdaP.h"

#include "cda_d_cx.h"


static int IsATemporaryCxError(void)
{
    return
        IS_A_TEMPORARY_CONNECT_ERROR()  ||
        errno == CENOHOST    ||
        errno == CETIMEOUT   ||  errno == CEMANYCLIENTS  ||
        errno == CESRVNOMEM  ||  errno == CEMCONN;
}


typedef struct
{
    cda_srvconn_t  sid;
    int            cd;
    int            is_suffering;
    sl_tid_t       rcn_tid;
} cda_d_cx_privrec_t;


//////////////////////////////////////////////////////////////////////


static int determine_name_type(const char *name,
                               char srvrspec[], size_t srvrspec_size,
                               char **channame_p)
{
  int         srv_len;
  const char *colon_p;
  const char *p;
  char       *vp;

    /* Defaults */
    srv_len = 0;
    *channame_p = name;

    /* Doesn't start with alphanumeric? */
    if (!isalnum(*name)) goto DO_RET;

    /* Is there a colon at all? */
    colon_p = strchr(name, ':');
    if (colon_p == NULL) goto DO_RET;

    /* Check than EVERYTHING preceding ':' can constitute a hostname */
    for (p = name;  p < colon_p;  p++)
        if (*p == '\0'  ||  (!isalnum(*p)  &&  *p != '.')) goto DO_RET;
    /* Okay, skip ':' */
    p++;
    /* ':' should be followed by a digit */
    if (*p == '\0'  ||  !isdigit(*p)) goto DO_RET;
    /* Okay, skip digits... */
    while (*p != '\0'  &&  isdigit(*p)) p++;
    //
    if (*p != '.') goto DO_RET;

    ////////////////////
    srv_len = p - name;
    *channame_p = p + 1;

 DO_RET:
    if (srv_len > 0)
    {
        if (srv_len > srvrspec_size - 1)
            srv_len = srvrspec_size - 1;
        memcpy(srvrspec, name, srv_len);
    }
    srvrspec[srv_len] = '\0';
    for (vp = srvrspec;  *vp != '\0';  vp++) *vp = tolower(*vp);

    return srv_len != 0;
}

static int  cda_d_cx_new_chan(cda_dataref_t ref, const char *name,
                              cxdtype_t dtype, int nelems)
{
  cda_d_cx_privrec_t *me;

  char                srvrspec[200];
  const char         *channame;
  int                 w_srv;
  const char         *at_p;
  size_t              channamelen;
  const char         *params;

    w_srv = determine_name_type(name, srvrspec, sizeof(srvrspec), &channame);
    ////fprintf(stderr, "\t%s(%d, \"%s\")\n\t\t[%s]%s\n", __FUNCTION__, ref, name, srvrspec, channame);

    if (strcasecmp(srvrspec, "unknown") == 0)
    {
        srvrspec[0] = '\0';
        w_srv = 0;
    }

    at_p = strchr(channame, '@');
    if (at_p != NULL)
    {
        channamelen = at_p - channame;
        params      = at_p + 1;
    }
    else
    {
        channamelen = strlen(channame);
        params      = "";
    }

    if (channamelen < 1)
    {
        cda_set_err("empty CHANNEL name");
        return CDA_DAT_P_ERROR;
    }

    if (w_srv)
    {
        me = cda_dat_p_get_server(ref, &CDA_DAT_P_MODREC_NAME(cx), srvrspec);
        if (me == NULL) return CDA_DAT_P_ERROR;
    }
    else
    {
    }

    return CDA_DAT_P_NOTREADY;
}

//////////////////////////////////////////////////////////////////////

static void ReconnectProc(int uniq, void *unsdptr,
                          sl_tid_t tid, void *privptr);

static void SuccessProc(cda_d_cx_privrec_t *me, int from_new)
{
    if (me->is_suffering)
    {
        cda_dat_p_report(me->sid, "connected.");
        me->is_suffering = 0;
    }
    
    sl_deq_tout(me->rcn_tid); me->rcn_tid = -1;

    cda_dat_p_set_server_state(me->sid, CDA_DAT_P_OPERATING);
}

static void FailureProc(cda_d_cx_privrec_t *me, int from_new, int reason)
{
  int                   ec = errno;
  int                   us;
  enum {MAX_US_X10 = 2*1000*1000*1000};

    us = 1*1000*1000;
    if (ec == CENOHOST)
    {
        if (us > MAX_US_X10 / 10)
            us = MAX_US_X10;
        else
            us *= 10;
    }

    if (!me->is_suffering)
    {
        cda_dat_p_report(me->sid, "%s: %s; will reconnect.",
                   cx_strreason(reason), cx_strerror(ec));
        me->is_suffering = 1;
    }

    /* Notify cda */
    cda_dat_p_set_server_state(me->sid, CDA_DAT_P_NOTREADY);
    
    /* Forget old connection */
    if (me->cd >= 0) cx_close(me->cd);
    me->cd = -1;

    /* And organize a timeout in a second... */
    me->rcn_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                    us, ReconnectProc, me);
}

static void ProcessCxlibEvent(int uniq, void *unsdptr,
                              int cd, int reason, const void *info,
                              void *privptr)
{
  cda_d_cx_privrec_t *me = (cda_d_cx_privrec_t *)privptr;
  int                 ec = errno;

    ////fprintf(stderr, "%s: reason=%d\n", __FUNCTION__, reason);
    switch (reason)
    {
        case CAR_ANSWER:
            break;
        
        case CAR_CONNECT:
            SuccessProc(me, 0);
            break;

        case CAR_CONNFAIL:
        case CAR_ERRCLOSE:
        case CAR_KILLED:
            FailureProc(me, 0, reason);
            break;
    }
}

static void ReconnectProc(int uniq, void *unsdptr,
                          sl_tid_t tid, void *privptr)
{
  cda_d_cx_privrec_t *me = privptr;

    me->rcn_tid = -1;

    me->cd  = cx_open(cda_dat_p_suniq_of(me->sid), NULL,
                      cda_dat_p_srvrn_of(me->sid), 0,
                      cda_dat_p_argv0_of(me->sid), NULL,
                      ProcessCxlibEvent,           me);
    if (me->cd < 0)
        FailureProc(me, 0, CAR_CONNFAIL);
}

static int  cda_d_cx_new_srv (cda_srvconn_t  sid, void *pdt_privptr,
                              int            uniq,
                              const char    *srvrspec,
                              const char    *argv0)
{
  cda_d_cx_privrec_t *me = pdt_privptr;
  int                 ec;

fprintf(stderr, "ZZZ %s(%s)\n", __FUNCTION__, srvrspec);
    me->sid = sid;
    me->cd  = cx_open(cda_dat_p_suniq_of(me->sid), NULL,
                      cda_dat_p_srvrn_of(me->sid), 0,
                      cda_dat_p_argv0_of(me->sid), NULL,
                      ProcessCxlibEvent,           me);
    me->rcn_tid = -1;

    if (me->cd < 0)
    {
        if (!IsATemporaryCxError())
        {
            ec = errno;
            cda_set_err("cx_open(\"%s\"): %s",
                        cda_dat_p_srvrn_of(me->sid), cx_strerror(ec));
            return CDA_DAT_P_ERROR;
        }
        
        FailureProc(me, 1, CAR_CONNFAIL);
    }
    else
        SuccessProc(me, 1);

    return CDA_DAT_P_NOTREADY;
}

static void cda_d_cx_del_srv (cda_srvconn_t  sid, void *pdt_privptr)
{
  cda_d_cx_privrec_t *me = pdt_privptr;
  
    if (me->cd >= 0)
        cx_close(me->cd);
    sl_deq_tout(me->rcn_tid); me->rcn_tid = -1;
}

//////////////////////////////////////////////////////////////////////

CDA_DEFINE_DAT_PLUGIN(cx, "CX data-access plugin",
                      NULL, NULL,
                      sizeof(cda_d_cx_privrec_t),
                      '.', ':', '@',
                      cda_d_cx_new_chan, NULL,
                      NULL,              NULL,
                      cda_d_cx_new_srv,  cda_d_cx_del_srv);
