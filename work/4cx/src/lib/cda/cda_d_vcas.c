#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "misc_macros.h"
#include "cxscheduler.h"

#include "cxlib.h"
#include "cda.h"
#include "cdaP.h"

#include "cda_d_vcas.h"


typedef int             cda_hwcnref_t;
enum {CDA_HWCNREF_ERROR = -1};

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int            in_use;

    cda_dataref_t  dataref; // "Backreference" to corr.entry in the global table

    char          *name;
} hwrinfo_t;

typedef struct
{
    cda_hwcnref_t  hwr;
    const char    *name;
} hwrname_t;

typedef struct
{
    cda_srvconn_t  sid;
    int            fd;
    int            is_suffering;
    sl_tid_t       rcn_tid;

    hwrinfo_t     *hwrs_list;
    int            hwrs_list_allocd;
} cda_d_vcas_privrec_t;

enum
{
    HWR_MIN_VAL   = 1,
    HWR_MAX_COUNT = 1000000,  // An arbitrary limit
    HWR_ALLOC_INC = 100,
};


//--------------------------------------------------------------------

// GetHwrSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Hwr, hwrinfo_t,
                                 hwrs, in_use, 0, 1,
                                 HWR_MIN_VAL, HWR_ALLOC_INC, HWR_MAX_COUNT,
                                 me->, me,
                                 cda_d_vcas_privrec_t *me, cda_d_vcas_privrec_t *me)

static void RlsHwrSlot(cda_hwcnref_t hwr, cda_d_vcas_privrec_t *me)
{
  hwrinfo_t *hi = AccessHwrSlot(hwr, me);

    if (hi->in_use == 0) return; /*!!! In fact, an internal error */
    hi->in_use = 0;

    safe_free(hi->name);
}

//////////////////////////////////////////////////////////////////////


static int  cda_d_vcas_new_chan(cda_dataref_t ref, const char *name,
                                cxdtype_t dtype, int nelems)
{
  cda_d_vcas_privrec_t *me;

  cda_hwcnref_t         hwr;
  hwrinfo_t            *hi;

  char                 *p;

    me = cda_dat_p_get_server(ref, &CDA_DAT_P_MODREC_NAME(vcas), NULL);
    if (me == NULL) return CDA_DAT_P_ERROR;

    hwr = GetHwrSlot(me);
    if (hwr < 0) return CDA_DAT_P_ERROR;
    hi = AccessHwrSlot(hwr, me);
    if ((hi->name = strdup(name)) == NULL)
    {
        RlsHwrSlot(hwr, me);
        return CDA_DAT_P_ERROR;
    }
    for (p = hi->name; *p != '0';  p++)
        if (*p == '.') *p = '/';

    return CDA_DAT_P_NOTREADY;
}

//////////////////////////////////////////////////////////////////////

static int  cda_d_vcas_new_srv (cda_srvconn_t  sid, void *pdt_privptr,
                                int            uniq,
                                const char    *srvrspec,
                                const char    *argv0)
{
  cda_d_vcas_privrec_t *me = pdt_privptr;
  int                   ec;

fprintf(stderr, "ZZZ %s(%s)\n", __FUNCTION__, srvrspec);
    me->sid = sid;

    return CDA_DAT_P_NOTREADY;
}

static void cda_d_vcas_del_srv (cda_srvconn_t  sid, void *pdt_privptr)
{
  cda_d_vcas_privrec_t *me = pdt_privptr;
  
    if (me->fd >= 0)
        close(me->fd);
    sl_deq_tout(me->rcn_tid); me->rcn_tid = -1;
}

//////////////////////////////////////////////////////////////////////

CDA_DEFINE_DAT_PLUGIN(vcas, "VCAS data-access plugin",
                      NULL, NULL,
                      sizeof(cda_d_vcas_privrec_t),
                      '.', ':', '@',
                      cda_d_vcas_new_chan, NULL,
                      NULL,                NULL,
                      cda_d_vcas_new_srv,  cda_d_vcas_del_srv);