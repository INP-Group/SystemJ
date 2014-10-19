#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <math.h>

#include "misc_macros.h"
#include "misclib.h"

#include "cda.h"
#include "cdaP.h"
#include "cda_plugmgr.h"


static char           cda_progname[40] = "";
/////////////////////////////////////////////////////////////////////

enum
{
    REF_TYPE_UNS = 0,
    REF_TYPE_CHN = 1,
    REF_TYPE_FLA = 2,
    REF_TYPE_REG = 3,
};

enum
{
    I_PHYS_COUNT = 2,
};

#define VARPARM_VAL_IN_USE ((void*)1)  /* Can NEVER be a legitimate heap pointer */

enum
{
    REF_STRINGS_NUM = 8,
};


typedef struct
{
    int                   evmask;
    cda_dataref_evproc_t  evproc;
    void                 *privptr2;
} ref_cbrec_t;
typedef struct
{
    int              in_use;
    const char      *reference; // Fully-resolved reference (w/ all prefixes/suffixes added)
                                // ...what to do with PROTOCOL::?
    void            *fla_privptr;
    cda_fla_p_rec_t *fla_vmt;

    cda_context_t    cid;              // Owning context
    cda_srvconn_t    sid;
    int              hwr;              // For varparms

    cxdtype_t        dtype;            // Data type
    int              nelems;           // Max # of units
    size_t           usize;            // Size of 1 unit, =sizeof_cxdtype(dtype)

    char            *strings_buf;
    const char      *strings_ptrs[REF_STRINGS_NUM];

    int              phys_count;
    int              phys_n_allocd;    // in PAIRS -- i.e., malloc(sizeof(double)*phys_n_allocd*2)
    double           imm_phys_rds[I_PHYS_COUNT * 2];
    double          *alc_phys_rds;

    cx_time_t        fresh_age;
    int              fresh_age_specified;
    CxAnyVal_t       q;

    void            *current_val;      // Pointer to current-value-buffer (if !=NULL)...
    int              current_nelems;   // ...and # of units in it
    CxAnyVal_t       valbuf;           // Buffer for small-sized values; current_val=NULL in such cases
    CxAnyVal_t       curraw;           // Raw value
    cxdtype_t        curraw_dtype;     // Its dtype, or UNKNOWN if not useful

    rflags_t         rflags;
    cx_time_t        timestamp;

    ref_cbrec_t     *cb_list;
    int              cb_list_allocd;
} refinfo_t;

typedef struct
{
    int              in_use;
    int              being_destroyed;
    cda_context_t    cid;              // Owning context
    int              nth;              // used-sid # inside context

    char             srvrspec[200];    /*!!! Replace '200' with what?  Or do strdup()? */
    int              state;

    cda_dat_p_rec_t *metric;
    void            *pdt_privptr;

    int             *hwr_arr_buf;      // Buffer for internal use by cda_lock_chans()
    int              hwr_arr_buf_allocd;
} srvinfo_t;

typedef struct
{
    cda_srvconn_t  sid;
} ctx_nthsidrec_t;
typedef struct
{
    int                   evmask;
    cda_context_evproc_t  evproc;
    void                 *privptr2;
} ctx_cbrec_t;
typedef struct
{
    int              in_use;
    cda_context_t    cid;               // Backreference to itself -- for fast ci2cid()

    int              uniq;
    void            *privptr1;
    char             argv0[40];

    char             defpfx[200];       /*!!! Replace '200' with what?  Or do strdup()? */
    int              defpfx_kind;
    int              defpfx_dcln_o;
    cda_dat_p_rec_t *defpfx_metric;

    char             sep_ch;            // '.'
    char             upp_ch;            // ':'
    char             opt_ch;            // '@'

    ctx_nthsidrec_t *sids_list;         /*!!! Should maintain from cda_dat_p_get_server() */
    int              sids_list_allocd;

    CxKnobParam_t   *varparm_list;
    int              varparm_list_allocd;

    ctx_cbrec_t     *cb_list;
    int              cb_list_allocd;
} ctxinfo_t;

//////////////////////////////////////////////////////////////////////

enum
{
    REF_MIN_VAL   = 1,
    REF_MAX_COUNT = 1000000,  // An arbitrary limit
    REF_ALLOC_INC = 100,

    SID_MIN_VAL   = 1,
    SID_MAX_COUNT = 1000,     // An arbitrary limit
    SID_ALLOC_INC = 2,

    VPR_MIN_VAL   = 1,
    VPR_MAX_COUNT = 10000,    // An arbitrary limit
    VPR_ALLOC_INC = 100,

    CTX_MIN_VAL   = 1,
    CTX_MAX_COUNT = 1000,     // An arbitrary limit
    CTX_ALLOC_INC = 2,
};

static refinfo_t *refs_list        = NULL;
static int        refs_list_allocd = 0;

static srvinfo_t *srvs_list        = NULL;
static int        srvs_list_allocd = 0;

static ctxinfo_t *ctxs_list        = NULL;
static int        ctxs_list_allocd = 0;

//--------------------------------------------------------------------

// GetRefCbSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, RefCb, ref_cbrec_t,
                                 cb, evmask, 0, 1,
                                 0, 2, 0,
                                 ri->, ri,
                                 refinfo_t *ri, refinfo_t *ri)

static void RlsRefCbSlot(int id, refinfo_t *ri)
{
  ref_cbrec_t *p = AccessRefCbSlot(id, ri);

    p->evmask = 0;
}

//--------------------------------------------------------------------

// GetRefSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Ref, refinfo_t,
                                 refs, in_use, REF_TYPE_UNS, REF_TYPE_CHN,
                                 REF_MIN_VAL, REF_ALLOC_INC, REF_MAX_COUNT,
                                 , , void)

static void RlsRefSlot(cda_dataref_t ref)
{
  refinfo_t *ri = AccessRefSlot(ref);

    if (ri->in_use == 0) return; /*!!! In fact, an internal error */
    ri->in_use = 0;

    DestroyRefCbSlotArray(ri);

    safe_free(ri->reference);
    safe_free(ri->fla_privptr);
    safe_free(ri->strings_buf);
}

static int CheckRef(cda_dataref_t ref)
{
    if (ref >= REF_MIN_VAL  &&
        ref < (cda_dataref_t)refs_list_allocd  &&  refs_list[ref].in_use != 0)
        return 0;

    errno = EBADF;
    return -1;
}

//--------------------------------------------------------------------

// GetSrvSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Srv, srvinfo_t,
                                 srvs, in_use, 0, 1,
                                 SID_MIN_VAL, SID_ALLOC_INC, SID_MAX_COUNT,
                                 , , void)

static void RlsSrvSlot(cda_srvconn_t sid)
{
  srvinfo_t *si = AccessSrvSlot(sid);

    if (si->in_use == 0) return; /*!!! In fact, an internal error */
    si->in_use = 0;

    if (si->pdt_privptr != NULL) free(si->pdt_privptr);
    if (si->hwr_arr_buf != NULL) free(si->hwr_arr_buf);
}

static int CheckSid(cda_srvconn_t sid)
{
    if (sid >= SID_MIN_VAL  &&
        sid < (cda_srvconn_t)srvs_list_allocd  &&  srvs_list[sid].in_use != 0)
        return 0;

    errno = EBADF;
    return -1;
}

//--------------------------------------------------------------------

// GetVarparmSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Varparm, CxKnobParam_t,
                                 varparm, ident, NULL, VARPARM_VAL_IN_USE,
                                 VPR_MIN_VAL, VPR_ALLOC_INC, VPR_MAX_COUNT,
                                 ci->, ci,
                                 ctxinfo_t *ci, ctxinfo_t *ci)

static void RlsVarparmSlot(int id, ctxinfo_t *ci)
{
  CxKnobParam_t *p = AccessVarparmSlot(id, ci);

    if (p->ident != VARPARM_VAL_IN_USE) safe_free(p->ident);
    p->ident = NULL;
}

//--------------------------------------------------------------------

// GetNthSidSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, NthSid, ctx_nthsidrec_t,
                                 sids, sid, CDA_SRVCONN_ERROR*1/*!!!*/, 1,
                                 0, 10, 0,
                                 ci->, ci,
                                 ctxinfo_t *ci, ctxinfo_t *ci)

static void RlsNthSidSlot(int id, ctxinfo_t *ci)
{
  ctx_nthsidrec_t *p = AccessNthSidSlot(id, ci);

    /* Do-nothing */
}

//--------------------------------------------------------------------

// GetCtxCbSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, CtxCb, ctx_cbrec_t,
                                 cb, evmask, 0, 1,
                                 0, 2, 0,
                                 ci->, ci,
                                 ctxinfo_t *ci, ctxinfo_t *ci)

static void RlsCtxCbSlot(int id, ctxinfo_t *ci)
{
  ctx_cbrec_t *p = AccessCtxCbSlot(id, ci);

    p->evmask = 0;
}

//--------------------------------------------------------------------

// GetCtxSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Ctx, ctxinfo_t,
                                 ctxs, in_use, 0, 1,
                                 CTX_MIN_VAL, CTX_ALLOC_INC, CTX_MAX_COUNT,
                                 , , void)

static void RlsCtxSlot(cda_context_t cid)
{
  ctxinfo_t *ci = AccessCtxSlot(cid);

    if (ci->in_use == 0) return; /*!!! In fact, an internal error */
    ci->in_use = 0;

    DestroyCtxCbSlotArray(ci);
    /*!!! ->sids should be freed too*/
}

static int CheckCid(cda_context_t cid)
{
    if (cid >= CTX_MIN_VAL  &&
        cid < (cda_context_t)ctxs_list_allocd  &&  ctxs_list[cid].in_use != 0)
        return 0;

    errno = EBADF;
    return -1;
}

/*
 *  si2sid
 *      Converts a "context pointer", used internally by the library,
 *      into "context id", accepted by public cda_XXX() calls
 *      and suitable for error/debug messages.
 */

static inline cda_context_t ci2cid(ctxinfo_t *ci)
{
    return ci->cid;
}

//////////////////////////////////////////////////////////////////////

#if 0
{ // Engineering->Operator
    n   = ri->phys_count;
    rdp = ri->alc_phys_rds;
    if (rdp == NULL) rdp = ri->imm_phys_rds;
    while (n > 0)
    {
        v = v / rdp[0] - rdp[1];
        rdp += 2;
        n--;
    }
}
{ // Operator->Engineering
    n   = ri->phys_count;
    rdp = ri->alc_phys_rds;
    if (rdp == NULL) rdp = ri->imm_phys_rds;
    rdp += (n-1)*2;
    while (n > 0)
    {
        v = (v + rdp[1]) * rdp[0]
        rdp -= 2;
        n--;
    }
}
#endif

//////////////////////////////////////////////////////////////////////

#define TIMESTAMP_IS_AFTER(a, b) \
    ((a).sec > (b).sec  ||  ((a).sec == (b).sec  &&  (a).nsec > (b).nsec))

/* Copy of timeval_subtract() */
static int timestamp_subtract  (cx_time_t *result,
                                cx_time_t *x, cx_time_t *y)
{
  cx_time_t yc = *y; // A copy of y -- to preserve y from garbling
    
    /* Perform the carry for the later subtraction by updating Y. */
    if (x->nsec < yc.nsec) {
      int carry = (yc.nsec - x->nsec) / 1000000000 + 1;
        yc.nsec -= 1000000000 * carry;
        yc.sec += carry;
    }
    if (x->nsec - yc.nsec > 1000000000) {
      int carry = (yc.nsec - x->nsec) / 1000000000;
        yc.nsec += 1000000000 * carry;
        yc.sec -= carry;
    }

    /* Compute the time remaining to wait.
     `nsec' is certainly positive. */
    result->sec = x->sec - yc.sec;
    result->nsec = x->nsec - yc.nsec;

    /* Return 1 if result is negative. */
    return x->sec < yc.sec;
}
//////////////////////////////////////////////////////////////////////

/* Note: values are ordered by degree of importance/independence. */
enum
{
    SPEC_IS_LOCAL = 0,
    SPEC_INCL_SRV = 1,
    SPEC_WITH_PRT = 2,
};

/*!!! Note:
          This code supposes that references are CX-rules-conformant
          (i.e., [PROTO::][SERVER:N] prefixes are in effect).
          But that's not true in case of other protocols.
*/
static int kind_of_reference(const char *spec, char **path_p, char **dcln_p)
{
  const char *p = spec;
  const char *s;

    if (path_p != NULL) *path_p = spec;

    // An empty string?
    if (spec == NULL  ||  *p == '\0') return SPEC_IS_LOCAL;

    // Doesn't start with alphanumeric?
    if (!isalnum(*p)) return SPEC_IS_LOCAL;

    // Skip alphanumerics
    while (*p != '\0'  &&  (isalnum(*p)  ||  *p == '.')) p++;

    // Okay, what's then:
    // ...a local...
    if (p[0] != ':') return SPEC_IS_LOCAL;
    // ...or some kind of global?
    else
    {
        // A PROTO:: ?
        if (p[1] == ':')
        {
            p += 2;
            if (dcln_p != NULL) *dcln_p = p;
            // Should also skip [SERVER:N] if any
            if (*p!= '\0'  &&  isalnum(*p))
            {
                s = p;
                while (*s != '\0'  &&  isalnum(*s)) s++;
                if (*s == ':')
                {
                    s++;
                    while (*s != '\0'  &&  isdigit(*s)) s++;
                    // ...and separating '.' if present
                    if (*s == '.') s++;
                    p = s;
                }
            }

            if (path_p != NULL) *path_p = p;
            return SPEC_WITH_PRT;
        }
        // No, just SERVER:N
        else
        {
            // Should skip only remaining [:N]...
            p++;
            while (*p != '\0'  &&  isdigit(*p)) p++;
            // ...and separating '.' if present
            if (*p == '.') p++;

            if (path_p != NULL) *path_p = p;
            return SPEC_INCL_SRV;
        }
    }
}

static char *combine_name_parts(ctxinfo_t *ci,
                                const char           *base,
                                const char           *spec,
                                int                   add_defpfx,
                                char                 *namebuf,
                                size_t                namebuf_size)
{
  int            nx;
  int            len;

  int            base_kind;
  int            spec_kind;

  const char    *dpfx_beg; // Defpfx-begin-pointer

  const char    *base_dcl; // after-DoubleColon-pointer
  const char    *base_beg; // Base-begin-pointer
  const char    *base_pth; // Base-path_begin-pointer (after "::[SRV]"); like root-dir
  const char    *base_atc; // At-p (in base)
  const char    *base_end; // Base-end-p
  const char    *spec_ptr; // Spec-cur-p
  const char    *p;

    base_beg  = base;
    base_kind = kind_of_reference(base, &base_pth, &base_dcl);
    spec_kind = kind_of_reference(spec, NULL, NULL);

    // Find where @-options start
    base_atc = strchr(base_pth, '@');
    if (base_atc == NULL) base_atc = base_pth + strlen(base_pth); // Point to end, resulting in empty opts string

    base_end = base_atc;
    spec_ptr = spec;

    /* b. Decide */
    if      (spec_kind == SPEC_WITH_PRT)
    {
        // Use it as-is, ignoring base
        base_beg = base_end = "";
    }
    else if (spec_kind == SPEC_INCL_SRV)
    {
        // If base-spec includes protocol, use it (by chopping base after "::")
        if (base_kind == SPEC_WITH_PRT)
            base_end = base_dcl;
        // ...otherwise use spec as-is
        else
            base_beg = base_end = "";
    }
    else
    {
        // Relative reference?
        if      (*spec_ptr == ':')
        {
            // Walk upwards as far as required && possible
            while (*spec_ptr == ':'      // Requested?
                   &&
                   base_end > base_pth)  // Possible?
            {
                // Are there more than 1 component left in base?
                p = memrchr(base_pth, '.', base_end - base_pth);
                if (p == NULL)
                    base_end = base_pth; // Chop sole remaining component
                else
                    base_end = p;        // Chop last-from-tail component
    
                spec_ptr++;
            }
    
            // Chop remaining ':'s
            while (*spec_ptr == ':') spec_ptr++;
    
            // Re-check remaining spec
            if (*spec_ptr == '\0') // "*spec_ptr=='@'" should be checked by dat_plugin
            {
                cda_set_err("missing CHANNEL name in spec");
                return NULL;
            }
        }
        // From-root reference?
        else if (*spec_ptr == '.')
        {
            // Skip '.'
            spec_ptr++;
            // Truncate "path" part
            base_end = base_pth;
        }
    }

    /*
        Okay, at this point we have following info:
            spec_ptr  points to CHANNEL_NAME[@OPTIONS]
            base_beg  points to BEGIN of base
            base_end  points to END of base
            base_atc  points to beginning of @OPTIONS in base
     */
    // a. Init
    nx = namebuf_size - 1; namebuf[nx] = '\0';
    // b. spec
    len = strlen(spec_ptr);
    if (len > nx)
    {
        cda_set_err("spec too long");
        return NULL;
    }
    nx -= len;
    memcpy(namebuf + nx, spec_ptr, len);
    // c. base; Note: in case of PRT/SRV specifiedness in spec the {base_beg,base_end} are already chopped earlier
    len = base_end - base_beg;
    if (len > 0)
    {
        // c1. '.' separator
        if (base_beg[len-1] != '.'  &&
            base_beg[len-1] != ':'  &&
            base_beg[len-1] != '@')
        {
            if (nx < 1)
            {
                cda_set_err("spec too long, no room for '.'");
                return NULL;
            }
            namebuf[--nx] = '.';
        }
        // c2. base itself
        if (len > nx)
        {
            cda_set_err("base too long");
            return NULL;
        }
        nx -= len;
        memcpy(namebuf + nx, base_beg, len);
    }
    // d. defpfx
    dpfx_beg = ci->defpfx;
    len      = strlen(dpfx_beg);
    if      (!add_defpfx)
        len = 0;
    else if (spec_kind == SPEC_WITH_PRT  ||  base_kind == SPEC_WITH_PRT)
        len = 0;
    else if (spec_kind == SPEC_INCL_SRV  ||  base_kind == SPEC_INCL_SRV)
    {
        if (ci->defpfx_kind == SPEC_WITH_PRT)
            len = ci->defpfx_dcln_o;
        else
            len = 0;
    }
    if (len > 0)
    {
        // d1. '.' separator
        if (dpfx_beg[len-1] != '.'  &&
            dpfx_beg[len-1] != ':'  &&
            dpfx_beg[len-1] != '@')
        {
            if (nx < 1)
            {
                cda_set_err("base+spec too long, no room for '.'");
                return NULL;
            }
            namebuf[--nx] = '.';
        }
        // d2. base itself
        if (len > nx)
        {
            cda_set_err("defpfx too long");
            return NULL;
        }
        nx -= len;
        memcpy(namebuf + nx, dpfx_beg, len);
    }

    if (1)
    {
        fprintf(stderr, "%s(\"%s\", \"%s\"): [%.*s],(%s) \"%s\"\n",
                __FUNCTION__, base, spec,
                base_end-base_beg, base_beg, spec_ptr,
                namebuf + nx);
    }

    return namebuf + nx;
}

char *cda_combine_base_and_spec(cda_context_t         cid,
                                const char           *base,
                                const char           *spec,
                                char                 *namebuf,
                                size_t                namebuf_size)
{
  ctxinfo_t   *ci = AccessCtxSlot(cid);

    if (CheckCid(cid) != 0) return NULL;

    return combine_name_parts(ci, base, spec, 0, namebuf, namebuf_size);
}
//////////////////////////////////////////////////////////////////////

/* Context management */

cda_context_t  cda_new_context(int                   uniq,   void *privptr1,
                               const char           *defpfx, int   flags,
                               const char           *argv0,
                               int                   evmask,
                               cda_context_evproc_t  evproc,
                               void                 *privptr2)
{
  cda_context_t     cid;
  ctxinfo_t        *ci;
  int               r;

  char             *slash;

  char              scheme[20];
  const char       *target;
  cda_dat_p_rec_t  *pdt;
  char             *dcln_p;

    cda_clear_err();

    if (defpfx == NULL) defpfx = "";
    if (argv0  == NULL) argv0  = "";
    slash = strrchr(argv0, '/');
    if (slash != NULL  &&  slash[1] != '\0') argv0 = slash + 1;

    // ...do "syntax checks"...

    /* Split spec into scheme and target... */
    split_url("cx", "::",
              defpfx, scheme, sizeof(scheme),
              &target);
    /* ...and check if this scheme is supported */
    pdt = cda_get_dat_p_rec_by_scheme(scheme);
    if (pdt == NULL)
    {
        cda_set_err("unknown scheme \"%s\"", scheme);
        return CDA_CONTEXT_ERROR;
    }
    

    /* Allocate a free slot... */
    cid = GetCtxSlot();
    if (cid == CDA_CONTEXT_ERROR) return cid;

    /* ...and fill it with data */
    ci = AccessCtxSlot(cid);
    strzcpy(ci->defpfx, defpfx, sizeof(ci->defpfx));
    strzcpy(ci->argv0,  argv0,  sizeof(ci->argv0));
    ci->uniq     = uniq;
    ci->privptr1 = privptr1;

    dcln_p = NULL;
    ci->defpfx_kind   = kind_of_reference(ci->defpfx, NULL, &dcln_p);
    ci->defpfx_dcln_o = (dcln_p != NULL)? dcln_p - ci->defpfx : 0;
    ci->defpfx_metric = pdt;
    ci->sep_ch        = pdt->sep_ch;
    ci->upp_ch        = pdt->upp_ch;
    ci->opt_ch        = pdt->opt_ch;

    if (cda_progname[0] == '\0'  &&  argv0[0] != '\0')
        strzcpy(cda_progname, argv0, sizeof(cda_progname));

    cda_add_context_evproc(cid, evmask, evproc, privptr2);

    return cid;
}

int            cda_run_context(cda_context_t         cid);
int            cda_hlt_context(cda_context_t         cid);
int            cda_del_context(cda_context_t         cid)
{
  ctxinfo_t   *ci = AccessCtxSlot(cid);

    if (CheckCid(cid) != 0) return -1;

    RlsCtxSlot(cid);

    return 0;
}

static int ctx_evproc_checker(ctx_cbrec_t *p, void *privptr)
{
  ctx_cbrec_t *model = privptr;

    return
        p->evmask   == model->evmask  &&
        p->evproc   == model->evproc  &&
        p->privptr2 == model->privptr2;
}

int            cda_add_context_evproc(cda_context_t         cid,
                                      int                   evmask,
                                      cda_context_evproc_t  evproc,
                                      void                 *privptr2)
{
  ctxinfo_t   *ci = AccessCtxSlot(cid);
  ctx_cbrec_t *p;
  int          n;
  ctx_cbrec_t  rec;

    if (CheckCid(cid) != 0) return -1;
    if (evproc == NULL)     return 0;
    
    /* Check if it is already in the list */
    rec.evmask   = evmask;
    rec.evproc   = evproc;
    rec.privptr2 = privptr2;
    if (ForeachCtxCbSlot(ctx_evproc_checker, &rec, ci) >= 0) return 0;

    n = GetCtxCbSlot(ci);
    if (n < 0) return -1;
    p = AccessCtxCbSlot(n, ci);

    p->evmask   = evmask;
    p->evproc   = evproc;
    p->privptr2 = privptr2;
    
    return 0;
}

int            cda_del_context_evproc(cda_context_t         cid,
                                      int                   evmask,
                                      cda_context_evproc_t  evproc,
                                      void                 *privptr2)
{
  ctxinfo_t   *ci = AccessCtxSlot(cid);
  int          n;
  ctx_cbrec_t  rec;

    if (CheckCid(cid) != 0) return -1;
    if (evproc == NULL)     return 0;

    /* Find requested callback */
    rec.evmask   = evmask;
    rec.evproc   = evproc;
    rec.privptr2 = privptr2;
    n = ForeachCtxCbSlot(ctx_evproc_checker, &rec, ci);
    if (n < 0)
    {
        /* Not found! */
        errno = ENOENT;
        return -1;
    }

    RlsCtxCbSlot(n, ci);

    return 0;
}

//////////////////////////////////////////////////////////////////////

/* Channels management */

static int ctx_ref_checker(refinfo_t *ri, void *privptr)
{
  refinfo_t *model = privptr;

    return
        ri->cid    == model->cid    &&
        ri->in_use == REF_TYPE_CHN  &&
        strcasecmp(ri->reference, model->reference) == 0;
}

cda_dataref_t  cda_add_chan   (cda_context_t         cid,
                               const char           *base,
                               const char           *spec,
                               int                   options,
                               cxdtype_t             dtype,
                               int                   n_items,
                               int                   evmask,
                               cda_dataref_evproc_t  evproc,
                               void                 *privptr2)
{
  ctxinfo_t       *ci = AccessCtxSlot(cid);

  char             namebuf[CDA_PATH_MAX];
  char            *nameptr;

  int              ref;
  refinfo_t       *ri;
  refinfo_t        ref_cmp_info;  // For searching only

  size_t           usize;                // Size of data-units
  size_t           csize;                // Channel data size

  char             scheme[20];
  const char      *target;
  cda_dat_p_rec_t *pdt;
  int              new_chan_r;

    cda_clear_err();

    if (CheckCid(cid) != 0) return -1;

    if (spec == NULL  ||  spec[0] == '\0')
    {
        cda_set_err("empty spec");
        return CDA_DATAREF_ERROR;
    }
    if (base == NULL) base = "";

    /*  */
    if (n_items < 1) n_items = 1;
    usize = sizeof_cxdtype(dtype);
    csize = usize * n_items;

    /* 1. Resolve defpfx,base,spec */
    nameptr = combine_name_parts(ci, base, spec, 1, namebuf, sizeof(namebuf));
    if (nameptr == NULL) return CDA_DATAREF_ERROR;

    /* 2. Check if this channel is already registered */
    ref_cmp_info.cid       = cid;
    ref_cmp_info.reference = nameptr;
    ref = ForeachRefSlot(ctx_ref_checker, &ref_cmp_info);
    if (ref >= 0)
    {
        /*!!! Check {dtype,n_items} compatibility? */
        goto DO_RETURN;
    }

    /* 3. Truly new channel, should allocate new cell */
    ref = GetRefSlot();
    if (ref < 0) return ref;
    ri = AccessRefSlot(ref);
    // Fill data
    ri->in_use = REF_TYPE_CHN;
    if ((ri->reference = strdup(nameptr)) == NULL)
    {
        RlsRefSlot(ref);
        return CDA_DATAREF_ERROR;
    }
    if (csize > sizeof(ri->valbuf))
    {
        if ((ri->current_val = malloc(csize)) == NULL)
        {
            RlsRefSlot(ref);
            return CDA_DATAREF_ERROR;
        }
        bzero(ri->current_val, csize);
    }
    ri->cid    = cid;
    ri->sid    = CDA_SRVCONN_ERROR; // Initially isn't bound to anything

    ri->dtype  = dtype;
    ri->nelems = n_items;
    ri->usize  = usize;

    // A special case: scalar double
    if (dtype == CXDTYPE_DOUBLE  &&  n_items == 1)
    {
        ri->current_nelems = 1;
        ri->valbuf.f64     = NAN;
    }

    /*  */
    /* First, split spec into scheme and target... */
    split_url("cx", "::",
              nameptr, scheme, sizeof(scheme),
              &target);
    /* ...and check if this scheme is supported */
    pdt = cda_get_dat_p_rec_by_scheme(scheme);
    if (pdt == NULL)
    {
        cda_set_err("unknown scheme \"%s\"", scheme);
        RlsRefSlot(ref);
        return CDA_DATAREF_ERROR;
    }

    /* Schedule search/registration */
    cda_add_dataref_evproc(ref, evmask, evproc, privptr2);
    new_chan_r = -1;
    if (pdt->new_chan != NULL) new_chan_r = pdt->new_chan(ref, target, dtype, n_items);
    if (new_chan_r < 0)
    {
        ri->rflags = CXCF_FLAG_NOTFOUND;
    }

 DO_RETURN:
    /*!!! Reference count? */
    cda_add_dataref_evproc(ref, evmask, evproc, privptr2);
    ////fprintf(stderr, "\t->%d\n", ref);
    return ref;
}

int            cda_del_chan   (cda_dataref_t ref)
{
    if      (ref == CDA_NONE) return 0;
    else if (ref < 0)         return -1;

    /*!!!*/

    //safe_free(ri->current_val);

    return 0;
}

static int ref_evproc_checker(ref_cbrec_t *p, void *privptr)
{
  ref_cbrec_t *model = privptr;

    return
        p->evmask   == model->evmask  &&
        p->evproc   == model->evproc  &&
        p->privptr2 == model->privptr2;
}

int            cda_add_dataref_evproc(cda_dataref_t         ref,
                                      int                   evmask,
                                      cda_dataref_evproc_t  evproc,
                                      void                 *privptr2)
{
  refinfo_t   *ri = AccessRefSlot(ref);
  ref_cbrec_t *p;
  int          n;
  ref_cbrec_t  rec;

    if (CheckRef(ref) != 0) return -1;
    if (evmask == 0  ||
        evproc == NULL)     return 0;
    
    /* Check if it is already in the list */
    rec.evmask   = evmask;
    rec.evproc   = evproc;
    rec.privptr2 = privptr2;
    if (ForeachRefCbSlot(ref_evproc_checker, &rec, ri) >= 0) return 0;

    n = GetRefCbSlot(ri);
    if (n < 0) return -1;
    p = AccessRefCbSlot(n, ri);
    
    p->evmask   = evmask;
    p->evproc   = evproc;
    p->privptr2 = privptr2;
    
    return 0;
}

int            cda_del_dataref_evproc(cda_dataref_t         ref,
                                      int                   evmask,
                                      cda_dataref_evproc_t  evproc,
                                      void                 *privptr2)
{
  refinfo_t   *ri = AccessRefSlot(ref);
  int          n;
  ref_cbrec_t  rec;

    if (CheckRef(ref) != 0) return -1;
    if (evproc == NULL)     return 0;

    /* Find requested callback */
    rec.evmask   = evmask;
    rec.evproc   = evproc;
    rec.privptr2 = privptr2;
    n = ForeachRefCbSlot(ref_evproc_checker, &rec, ri);
    if (n < 0)
    {
        /* Not found! */
        errno = ENOENT;
        return -1;
    }

    RlsRefCbSlot(n, ri);

    return 0;
}

static int compare_ref_sids(const void *a, const void *b)
{
  cda_dataref_t  ref_a = *((cda_dataref_t*)a);
  cda_dataref_t  ref_b = *((cda_dataref_t*)b);
  refinfo_t     *ri_a  = AccessRefSlot(ref_a);
  refinfo_t     *ri_b  = AccessRefSlot(ref_b);

    return (ri_a->sid > ri_b->sid) - (ri_a->sid - ri_b->sid);
}
int            cda_lock_chans (int count, cda_dataref_t *refs,
                               int operation)
{
  int            n;
  int            first_n;
  int            after_n;
  int            hwr_count;
  cda_srvconn_t  sid;
  refinfo_t     *ri;
  srvinfo_t     *si;
    
    if (count < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (count == 0) return 0;

    for (n = 0;  n < count;  n++)
    {
        if (CheckRef(refs[n]) != 0) return -1;

        /* Does this ref support locking? */
        ri = AccessRefSlot(refs[n]);
        if (ri->in_use != REF_TYPE_CHN)
        {
            errno = EINVAL;
            return -1;
        }
        si = AccessSrvSlot(ri->sid);
        if (si->metric->do_lock == NULL)
        {
            errno = EINVAL;
            return -1;
        }
    }

    /* Sort ref by sids, for refs from each sid to be in one contiguous chunk */
    qsort(refs, count, sizeof(*refs), compare_ref_sids);

    /* Walk */
    for (first_n = 0;  first_n < count;  first_n = after_n)
    {
        /* Find a bunch of refs belonging to one sid */
        ri  = AccessRefSlot(refs[first_n]);
        sid = ri->sid;
        si  = AccessSrvSlot(sid);
        for (after_n = first_n;  after_n < count;  after_n++)
        {
            ri = AccessRefSlot(refs[after_n]);
            if (ri->sid != sid) break;
        }
        hwr_count = after_n - first_n;

        /* si==-1 in unbound refs */
        if (si < 0) continue;

        /* Make a list and call do_lock() */
        if (GrowUnitsBuf(&(si->hwr_arr_buf), &(si->hwr_arr_buf_allocd),
                         hwr_count, sizeof(*(si->hwr_arr_buf))) < 0) return -1;

        for (n = 0;  n < hwr_count;  n++)
        {
            ri = AccessRefSlot(refs[first_n + n]);
            si->hwr_arr_buf[n] = ri->hwr;
        }
        si->metric->do_lock(si->pdt_privptr,
                            hwr_count, si->hwr_arr_buf,
                            operation, -1);
    }

    return 0;
}


/* Simplified channels API */

cda_dataref_t cda_add_dchan(cda_context_t  cid,
                            const char    *name)
{
    return cda_add_chan(cid, NULL,
                        name, CDA_OPT_NONE, CXDTYPE_DOUBLE, 1,
                        0, NULL, NULL);
}

int           cda_get_dcval(cda_dataref_t  ref, double *v_p)
{
    return cda_get_ref_dval(ref, v_p, NULL, NULL, NULL, NULL);
}

int           cda_set_dcval(cda_dataref_t  ref, double  val)
{
#if 1
    return cda_process_ref(ref, CDA_OPT_IS_W | CDA_OPT_DO_EXEC,
                           val,
                           NULL, 0);
#else
    return cda_snd_ref_data(ref,
                            CXDTYPE_DOUBLE, 1,
                            &val);
#endif
}

//////////////////////////////////////////////////////////////////////
cda_dataref_t  cda_add_formula(cda_context_t         cid,
                               const char           *base,
                               const char           *spec,
                               int                   options,
                               CxKnobParam_t        *params,
                               int                   num_params,
                               int                   evmask,
                               cda_dataref_evproc_t  evproc,
                               void                 *privptr2)
{
  ctxinfo_t       *ci = AccessCtxSlot(cid);

  int              ref;
  refinfo_t       *ri;

  char             scheme[20];
  char            *nlp;
  int              scheme_len;
  cda_fla_p_rec_t *pfl;
  int              new_fla_r;

    cda_clear_err();

    if (CheckCid(cid) != 0) return -1;

    if (spec == NULL  ||  spec[0] == '\0')
    {
        cda_set_err("empty spec");
        return CDA_DATAREF_ERROR;
    }
    if (base == NULL) base = "";

    /*  */
    strcpy(scheme, "fla");
    if (
        spec[0] == '#'  &&  spec[1] == '!'  &&
        (
         (nlp = strchr(spec, '\n')) != NULL  ||
         (nlp = strchr(spec, '\r')) != NULL
        )
       )
    {
        scheme_len = nlp - (spec + 2);
        if (scheme_len > 0)
        {
            if (scheme_len > sizeof(scheme) - 1)
                scheme_len = sizeof(scheme) - 1;
            memcpy(scheme, spec + 2, scheme_len);
            scheme[scheme_len] = '\0';
        }
    }
    /*  */
    pfl = cda_get_fla_p_rec_by_scheme(scheme);
    if (pfl == NULL)
    {
        cda_set_err("unknown scheme \"%s\"", scheme);
        return CDA_DATAREF_ERROR;
    }

    ref = GetRefSlot();
    if (ref < 0) return ref;
    ri = AccessRefSlot(ref);
    // Fill data
    ri->in_use = REF_TYPE_FLA;
    if ((ri->reference = strdup(spec)) == NULL  ||
        (
         pfl->privrecsize != 0  &&
         (ri->fla_privptr  = malloc(pfl->privrecsize)) == NULL
        )
       )
    {
        RlsRefSlot(ref);
        return CDA_DATAREF_ERROR;
    }
    ri->fla_vmt = pfl;
    ri->cid     = cid;
    ri->sid     = CDA_SRVCONN_ERROR; // Formulae aren't bound to any sid

    ri->dtype   = CXDTYPE_DOUBLE;
    ri->nelems  = 1;
    ri->usize   = sizeof_cxdtype(ri->dtype);

    // Initialize "current state"
    ri->current_nelems = 1;
    ri->valbuf.f64     = NAN;
    ri->rflags         = 0;

    /* Perform registration */
    new_fla_r = -1;
    if (pfl->create != NULL)
        new_fla_r = pfl->create(ref,
                                ri->fla_privptr,
                                ci->uniq,
                                base, spec,
                                options,
                                params, num_params);
    if (new_fla_r < 0)
    {
        ri->rflags = CXCF_FLAG_NOTFOUND;
    }

    cda_add_dataref_evproc(ref, evmask, evproc, privptr2);

    return ref;
}

static int varchan_checker(refinfo_t *ri, void *privptr)
{
  refinfo_t     *model = privptr;
  ctxinfo_t     *ci;
  CxKnobParam_t *p;

    if (ri->cid != model->cid  ||
        ri->in_use != REF_TYPE_REG)
        return 0;

    ci = AccessCtxSlot(model->cid);
    p  = AccessVarparmSlot(ri->hwr, ci);
    return strcasecmp(p->ident, model->reference) == 0;
}
cda_dataref_t  cda_add_varchan(cda_context_t         cid,
                               const char           *varname)
{
  ctxinfo_t     *ci = AccessCtxSlot(cid);

  int            ref;
  refinfo_t     *ri;
  refinfo_t      ref_cmp_info;  // For searching only
  cda_varparm_t  vpn;

    cda_clear_err();

    if (CheckCid(cid) != 0) return -1;

    if (varname == NULL  ||  varname[0] == '\0')
    {
        cda_set_err("empty varname");
        return CDA_DATAREF_ERROR;
    }

    /* Check if this variable is already registered */
    ref_cmp_info.cid       = cid;
    ref_cmp_info.reference = varname;
    ref = ForeachRefSlot(varchan_checker, &ref_cmp_info); /*??? '%' at [0]? varchan_checker():strcasecmp(): p->ident+_1_? No, varchan_checker() checks against *KnobParam* names, while varchan's refinfo_t.reference is used by cda_src_of_ref() only */
    if (ref >= 0)
        return ref;
    
    /* Truly new variable, should allocate new cell */
    // Allocate a varparm first (or just get id if already alloc'd)
    vpn = cda_add_varparm(cid, varname);
    if (vpn < 0) return CDA_DATAREF_ERROR;
    // Allocate cell
    ref = GetRefSlot();
    if (ref < 0) return ref;
    ri = AccessRefSlot(ref); /* Note: no reason to del varparm in case of error */
    if ((ri->reference = malloc(1 + strlen(varname))) == NULL)
    {
        RlsRefSlot(ref);
        return CDA_DATAREF_ERROR;
    }
    ri->reference[0] = '%';
    strcpy(ri->reference + 1, varname);
    // Fill data
    ri->in_use = REF_TYPE_REG;
    ri->cid    = cid;
    ri->hwr    = vpn;
    ri->dtype  = CXDTYPE_DOUBLE;
    ri->nelems = 1;
    ri->usize  = sizeof_cxdtype(ri->dtype);

    return ref;
}

static int varparm_checker(CxKnobParam_t *p, void *privptr)
{
    return
        p->ident != VARPARM_VAL_IN_USE  &&
        strcasecmp(p->ident, privptr) == 0;
}
cda_varparm_t  cda_add_varparm(cda_context_t         cid,
                               const char           *varname)
{
  ctxinfo_t     *ci = AccessCtxSlot(cid);
  cda_varparm_t  vpn;
  CxKnobParam_t *p;

    if (CheckCid(cid) != 0)                      return CDA_VARPARM_ERROR;
    if (varname == NULL  ||  varname[0] == '\0') return CDA_VARPARM_ERROR;

    /* Check if it already exists */
    vpn = ForeachVarparmSlot(varparm_checker, varname, ci);
    if (vpn >= 0) return vpn;

    vpn = GetVarparmSlot(ci);
    if (vpn < 0) return vpn;
    p = AccessVarparmSlot(vpn, ci);

    if ((p->ident = strdup(varname)) == NULL)
    {
        RlsVarparmSlot(vpn, ci);
        return CDA_VARPARM_ERROR;
    }

    return vpn;
}

int            cda_src_of_ref           (cda_dataref_t ref, const char **src_p)
{
  refinfo_t      *ri = AccessRefSlot(ref);

    if (CheckRef(ref) != 0) return -1;
    if (ri->reference != NULL)
    {
        *src_p = ri->reference;
        return +1;
    }

    return 0;
}

int            cda_dtype_of_ref         (cda_dataref_t ref)
{
  refinfo_t      *ri = AccessRefSlot(ref);

    if (CheckRef(ref) != 0) return -1;
    return ri->dtype;
}

int            cda_nelems_of_ref        (cda_dataref_t ref)
{
  refinfo_t      *ri = AccessRefSlot(ref);

    if (CheckRef(ref) != 0) return -1;
    return ri->nelems;
}

int            cda_current_nelems_of_ref(cda_dataref_t ref)
{
  refinfo_t      *ri = AccessRefSlot(ref);

    if (CheckRef(ref) != 0) return -1;
    return ri->current_nelems;
}

int            cda_strings_of_ref       (cda_dataref_t  ref,
                                         char    **ident_p,
                                         char    **label_p,
                                         char    **tip_p,
                                         char    **comment_p,
                                         char    **geoinfo_p,
                                         char    **rsrvd6_p,
                                         char    **units_p,
                                         char    **dpyfmt_p)
{
  refinfo_t      *ri = AccessRefSlot(ref);

    if (CheckRef(ref) != 0) return -1;

    if (ident_p != NULL)   *ident_p   = ri->strings_ptrs[0];
    if (label_p != NULL)   *label_p   = ri->strings_ptrs[1];
    if (tip_p != NULL)     *tip_p     = ri->strings_ptrs[2];
    if (comment_p != NULL) *comment_p = ri->strings_ptrs[3];
    if (geoinfo_p != NULL) *geoinfo_p = ri->strings_ptrs[4];
    if (rsrvd6_p != NULL)  *rsrvd6_p  = ri->strings_ptrs[5];
    if (units_p != NULL)   *units_p   = ri->strings_ptrs[6];
    if (dpyfmt_p != NULL)  *dpyfmt_p  = ri->strings_ptrs[7];

    return 0;
}


static int nthsid_count_checker(ctx_nthsidrec_t *p, void *privptr)
{
  int       *lp = privptr;
  srvinfo_t *si = AccessSrvSlot(p->sid);

    if (*lp < si->nth)
        *lp = si->nth;

    return 0;
}
int                 cda_status_srvs_count(cda_context_t  cid)
{
  ctxinfo_t       *ci = AccessCtxSlot(cid);
  int              last = -1;

    if (CheckCid(cid) != 0) return -1;
    ForeachNthSidSlot(nthsid_count_checker, &last, ci);

    return last + 1;
}

cda_serverstatus_t  cda_status_of_srv    (cda_context_t  cid, int nth)
{
  ctxinfo_t       *ci = AccessCtxSlot(cid);
  ctx_nthsidrec_t *p;
  srvinfo_t       *si;

    if (CheckCid(cid) != 0) return CDA_SERVERSTATUS_ERROR;
    if (nth < 0  ||  nth >= ci->sids_list_allocd)
    {
        errno = EINVAL;
        return CDA_SERVERSTATUS_ERROR;
    }
    p = AccessNthSidSlot(nth, ci);
    /* Note: we allow referencing unused cells (which should usually never occur) */
    if (p->sid < SID_MIN_VAL) return CDA_SERVERSTATUS_DISCONNECTED;

    si = AccessSrvSlot(p->sid);
    if      (si->state == CDA_DAT_P_OPERATING) return CDA_SERVERSTATUS_NORMAL;
    /*!!! CDA_SERVERSTATUS_FROZEN? CDA_SERVERSTATUS_ALMOSTREADY? */
    else                                       return CDA_SERVERSTATUS_DISCONNECTED;
}

const char         *cda_status_srv_name  (cda_context_t  cid, int nth)
{
  ctxinfo_t       *ci = AccessCtxSlot(cid);
  ctx_nthsidrec_t *p;
  srvinfo_t       *si;

    if (CheckCid(cid) != 0) return NULL;
    if (nth < 0  ||  nth >= ci->sids_list_allocd)
    {
        errno = EINVAL;
        return NULL;
    }
    p = AccessNthSidSlot(nth, ci);
    /* Note: we allow referencing unused cells (which should usually never occur) */
    if (p->sid < SID_MIN_VAL) return "-UNUSED-";

    si = AccessSrvSlot(p->sid);
    return si->srvrspec;
}


//////////////////////////////////////////////////////////////////////

cda_dataref_t cda_dat_p_get_cid   (cda_dataref_t    source_ref)
{
  refinfo_t      *ri = AccessRefSlot(source_ref);

    if (CheckRef(source_ref) != 0) return CDA_DATAREF_ERROR;
    return ri->cid;
}

typedef struct
{
    cda_context_t    cid;
    cda_dat_p_rec_t *metric;
    const char      *srvrspec;
} srv_cmp_info_t;

static int srv_checker(srvinfo_t *p, void *privptr)
{
  srv_cmp_info_t *model = privptr;

    return
        p->cid    == model->cid     &&
        p->metric == model->metric  &&
        strcasecmp(p->srvrspec, model->srvrspec) == 0;
}

void *cda_dat_p_get_server         (cda_dataref_t    source_ref,
                                    cda_dat_p_rec_t *metric,
                                    const char      *srvrspec)
{
  refinfo_t       *ri = AccessRefSlot(source_ref);
  ctxinfo_t       *ci;
  srv_cmp_info_t   srv_cmp_info;
  cda_srvconn_t    sid;
  srvinfo_t       *si;
  int              r;
  int              nth;
  ctx_nthsidrec_t *nth_p;

    if (CheckRef(source_ref) != 0) return NULL;

    if (srvrspec == NULL) srvrspec = "";

    srv_cmp_info.cid      = ri->cid;
    srv_cmp_info.metric   = metric;
    srv_cmp_info.srvrspec = srvrspec;
    
    sid = ForeachSrvSlot(srv_checker, &srv_cmp_info);
    if (sid > 0)
    {
        si = AccessSrvSlot(sid);
        ri->sid = sid;
        return si->being_destroyed? NULL : si->pdt_privptr;
    }

    /* Okay, allocate a new one... */
    sid = GetSrvSlot();
    if (sid < 0) return NULL;

    si = AccessSrvSlot(sid);
    si->cid    = ri->cid;
    strzcpy(si->srvrspec, srvrspec, sizeof(si->srvrspec));
    si->state = CDA_DAT_P_ERROR;
    si->metric = metric;
    if (si->metric->privrecsize != 0)
    {
        if ((si->pdt_privptr = malloc(si->metric->privrecsize)) == NULL)
        {
            cda_set_err("unable to malloc privrec for \"%s::\"",
                        si->metric->mr.name);
            RlsSrvSlot(sid);
            return NULL;
        }
        bzero(si->pdt_privptr, si->metric->privrecsize);
    }

    ci = AccessCtxSlot(ri->cid);
    r = si->metric->new_srv(sid, si->pdt_privptr,
                            ci->uniq, srvrspec, ci->argv0);
    if (r < 0)
    {
        RlsSrvSlot(sid);
        return NULL;
    }
    si->state = r;

    nth = GetNthSidSlot(ci);
    /* Note: unlikely case nth<0 causes only malfunctioning of "status" calls */
    if (nth >= 0)
    {
        nth_p      = AccessNthSidSlot(nth, ci);
        nth_p->sid = sid;
        si->nth    = nth;
    }

    ri->sid = sid;

    return si->pdt_privptr;
}

void  cda_dat_p_set_server_state   (cda_srvconn_t  sid, int state)
{
  srvinfo_t      *si = AccessSrvSlot(sid);
  
    if (CheckSid(sid) != 0) return;

    si->state = state;
}

typedef struct
{
    int            uniq;
    void          *privptr1;
    cda_context_t  cid;
    int            reason;
    int            evmask;
    int            info_int;
} ctx_call_info_t;

static int ctx_evproc_caller(ctx_cbrec_t *p, void *privptr)
{
  ctx_call_info_t *info = privptr;

    if (p->evmask & info->evmask)
        p->evproc(info->uniq, info->privptr1, info->cid, info->reason, info->info_int, p->privptr2);
    return 0;
}
void  cda_dat_p_update_server_cycle(cda_srvconn_t  sid)
{
  srvinfo_t       *si = AccessSrvSlot(sid);
  ctxinfo_t       *ci;
  ctx_call_info_t  call_info;
  
    if (CheckSid(sid) != 0) return;

    ci = AccessCtxSlot(si->cid);

    call_info.uniq     = ci->uniq;
    call_info.privptr1 = ci->privptr1;
    call_info.cid      = si->cid;
    call_info.reason   = CDA_CTX_R_CYCLE;
    call_info.evmask   = 1 << call_info.reason;
    call_info.info_int = si->nth;
    ForeachCtxCbSlot(ctx_evproc_caller, &call_info, ci);
}

typedef struct
{
    int            uniq;
    void          *privptr1;
    cda_dataref_t  ref;
    int            reason;
    int            evmask;
    void          *info_ptr;
} ref_call_info_t;

static int ref_evproc_caller(ref_cbrec_t *p, void *privptr)
{
  ref_call_info_t *info = privptr;

    if (p->evmask & info->evmask)
        p->evproc(info->uniq, info->privptr1,
                  info->ref, info->reason, info->info_ptr,
                  p->privptr2);
    return 0;
}
void  cda_dat_p_update_dataset     (cda_srvconn_t  sid,
                                    int            count,
                                    cda_dataref_t *refs,
                                    void         **values,
                                    cxdtype_t     *dtypes,
                                    int           *nelems,
                                    rflags_t      *rflags,
                                    cx_time_t     *timestamps)
{
  srvinfo_t       *si = AccessSrvSlot(sid);
  ctxinfo_t       *ci;

  struct timeval   timenow;
  cx_time_t        curtime;
  cx_time_t        delta;

  int              x;
  cda_dataref_t    ref;
  refinfo_t       *ri;
  int              nels;
  uint8           *src;
  uint8           *dst;

  int              srpr;
  size_t           ssiz;
  int              repr;
  size_t           size;

  double           v;
  int              n;
  double          *rdp;

  ref_call_info_t  call_info;

    if (CheckSid(sid) != 0) return;
    ci = AccessCtxSlot(si->cid);

    gettimeofday(&timenow, NULL);
    curtime.sec  = timenow.tv_sec;
    curtime.nsec = timenow.tv_usec * 1000;

    /* I. Update */
    for (x = 0;  x < count;  x++)
    {
        /* Get pointer to the channel in question */
        ref = refs[x];
        if (CheckRef(ref) != 0)
        {
            /*!!!Bark? */
            goto NEXT_TO_UPDATE;
        }
        ri = AccessRefSlot(ref);

        /*!!! Check that ri->cid==si->cid?  Or, better, ri->sid==sid? */

        /* Check nelems */
        nels = nelems[x];
        if      (nels < 0)
        {
            /*!!!Bark? */
            goto NEXT_TO_UPDATE;
        }
        else if (ri->nelems == 1  &&  nels != 1)
        {
            /*!!!Bark? */
            goto NEXT_TO_UPDATE;
        }
        else if (nels > ri->nelems)
        {
            /* Too many?  Okay, just limit */
            /*!!!Bark? */
            nels = ri->nelems;
        }

        /* Store data */
        src = values[x];
        dst = ri->current_val;
        if (dst == NULL) dst = &(ri->valbuf);

        srpr = reprof_cxdtype(dtypes[x]);
        ssiz = sizeof_cxdtype(dtypes[x]);
        repr = reprof_cxdtype(ri->dtype);
        size = sizeof_cxdtype(ri->dtype);

        /* Get raw value if possible */
        if (nels == 1)
        {
            memcpy(&(ri->curraw), src, ssiz);
            ri->curraw_dtype = dtypes[x];
        }
        else
            ri->curraw_dtype = CXDTYPE_UNKNOWN;

        /* a. Incompatible? */
        if      (dtypes[x] != ri->dtype  &&
                 (
                  (srpr != CXDTYPE_REPR_INT  &&  srpr != CXDTYPE_REPR_FLOAT)
                  ||
                  (repr != CXDTYPE_REPR_INT  &&  repr != CXDTYPE_REPR_FLOAT)
                 )
                )
        {
            ri->current_nelems = ri->nelems;
            bzero(dst, ri->nelems * ri->usize);
            fprintf(stderr, "%d != %d\n", dtypes[x], ri->dtype);
        }
        /* b. Simple copy? */
        else if (
                 dtypes[x] == ri->dtype  &&
                 (
                  ri->phys_count == 0
                  ||
                  (repr != CXDTYPE_REPR_INT  &&  repr != CXDTYPE_REPR_FLOAT)
                 )
                )
        {
            if (nels != 0) memcpy(dst, src, size * nels);
            ri->current_nelems = nels;
        }
        /* c. Conversion */
        else
        {
            ri->current_nelems = nels;

            rdp = ri->alc_phys_rds;
            if (rdp == NULL) rdp = ri->imm_phys_rds;
            rdp += ri->phys_count * 2; // Is later compensated with "rdp -= n*2"

//fprintf(stderr, "dtype=%d nels=%d\n", dtypes[x], nels);
            while (nels > 0)
            {
                // Read datum, converting to double
                switch (dtypes[x])
                {
                    case CXDTYPE_INT32:  v = *((  int32*)src);     break;
                    case CXDTYPE_UINT32: v = *(( uint32*)src);     break;
                    case CXDTYPE_INT16:  v = *((  int16*)src);     break;
                    case CXDTYPE_UINT16: v = *(( uint16*)src);     break;
                    case CXDTYPE_DOUBLE: v = *((float64*)src);     break;
                    case CXDTYPE_SINGLE: v = *((float32*)src);     break;
                    case CXDTYPE_INT64:  v = *((  int64*)src);     break;
                    case CXDTYPE_UINT64: v = *(( uint64*)src);     break;
                    case CXDTYPE_INT8:   v = *((  int8 *)src);     break;
                    default:             v = *(( uint8 *)src);     break;
                }
                src += ssiz;

                // Do conversion
                n    = ri->phys_count;
                rdp -= n*2;
                while (n > 0)
                {
//fprintf(stderr, " v= %8.3f / %8.3f - %8.3f\n", v, rdp[0],  rdp[1]);
                    v = v / rdp[0] - rdp[1];
                    rdp += 2;
                    n--;
                }

                // Store datum, converting from double
                switch (ri->dtype)
                {
                    case CXDTYPE_INT32:      *((  int32*)dst) = v; break;
                    case CXDTYPE_UINT32:     *(( uint32*)dst) = v; break;
                    case CXDTYPE_INT16:      *((  int16*)dst) = v; break;
                    case CXDTYPE_UINT16:     *(( uint16*)dst) = v; break;
                    case CXDTYPE_DOUBLE:     *((float64*)dst) = v; break;
                    case CXDTYPE_SINGLE:     *((float32*)dst) = v; break;
                    case CXDTYPE_INT64:      *((  int64*)dst) = v; break;
                    case CXDTYPE_UINT64:     *(( uint64*)dst) = v; break;
                    case CXDTYPE_INT8:       *((  int8 *)dst) = v; break;
                    default:                 *(( uint8 *)dst) = v; break;
                }
                dst += size;

                nels--;
            }
        }

        ri->rflags = rflags[x];
        if (timestamps == NULL  ||  timestamps[x].sec == 0)
        {
            ri->timestamp = curtime;
        }
        else
        {
            ri->timestamp = timestamps[x];

            /* Check oldness only if timestamp is specified (hence in "else")
               and if fresh_age is specified */
            if (ri->fresh_age_specified)
            {
                if (timestamp_subtract(&delta, &curtime, &(ri->timestamp)) == 0  &&
                    TIMESTAMP_IS_AFTER(delta, ri->fresh_age))
                    ri->rflags |= CXCF_FLAG_DEFUNCT;
                //fprintf(stderr, "delta: %d,%d\n", delta.sec, delta.nsec);
            }
        }

 NEXT_TO_UPDATE:;
    }

    /* II. Call who requested */
    call_info.uniq     = ci->uniq;
    call_info.privptr1 = ci->privptr1;
    call_info.reason   = CDA_REF_R_UPDATE;
    call_info.evmask   = 1 << call_info.reason;
    call_info.info_ptr = NULL;

    for (x = 0;  x < count;  x++)
    {
        /* Get pointer to the channel in question */
        ref = refs[x];
        if (CheckRef(ref) != 0)
            goto NEXT_TO_CALL;
        ri = AccessRefSlot(ref);

        call_info.ref = ref;
        ForeachRefCbSlot(ref_evproc_caller, &call_info, ri);

 NEXT_TO_CALL:;
    }
}

void  cda_dat_p_defunct_dataset    (cda_srvconn_t  sid,
                                    int            count,
                                    cda_dataref_t *refs)
{
  srvinfo_t       *si = AccessSrvSlot(sid);
  ctxinfo_t       *ci;

  int              x;
  cda_dataref_t    ref;
  refinfo_t       *ri;

  ref_call_info_t  call_info;

    if (CheckSid(sid) != 0) return;
    ci = AccessCtxSlot(si->cid);

    /* I. Update */
    for (x = 0;  x < count;  x++)
    {
        /* Get pointer to the channel in question */
        ref = refs[x];
        if (CheckRef(ref) != 0)
        {
            /*!!!Bark? */
            goto NEXT_TO_DEFUNCT;
        }
        ri = AccessRefSlot(ref);

        /*!!! Check that ri->cid==si->cid?  Or, better, ri->sid==sid? */

        /* Mark */
        ri->rflags |= CXCF_FLAG_DEFUNCT;
        ri->timestamp.sec  = 0;
        ri->timestamp.nsec = 0;

 NEXT_TO_DEFUNCT:;
    }

    /* II. Call who requested */
    call_info.uniq     = ci->uniq;
    call_info.privptr1 = ci->privptr1;
    call_info.reason   = CDA_REF_R_STATCHG;
    call_info.evmask   = 1 << call_info.reason;
    call_info.info_ptr = NULL;

    for (x = 0;  x < count;  x++)
    {
        /* Get pointer to the channel in question */
        ref = refs[x];
        if (CheckRef(ref) != 0)
        {
            /*!!!Bark? */
            goto NEXT_TO_CALL;
        }
        ri = AccessRefSlot(ref);

        call_info.ref = ref;
        ForeachRefCbSlot(ref_evproc_caller, &call_info, ri);

 NEXT_TO_CALL:;
    }
}

void  cda_fla_p_update_fla_result  (cda_dataref_t  ref,
                                    double         value,
                                    CxAnyVal_t     raw,
                                    int            raw_dtype,
                                    rflags_t       rflags,
                                    cx_time_t      timestamp)
{
  refinfo_t     *ri;
  ctxinfo_t     *ci;

  struct timeval   now;

  ref_call_info_t  call_info;

    if (CheckRef(ref) != 0)
    {
        /*!!!Bark? */
        return;
    }
    ri = AccessRefSlot(ref);

    if (ri->in_use != REF_TYPE_FLA)
    {
        /*!!!Bark? */
        return;
    }

    /* Treat unspecified timestamp as "now" */
    if (timestamp.sec == 0)
    {
        gettimeofday(&now, NULL);
        timestamp.sec  = now.tv_sec;
        timestamp.nsec = now.tv_usec * 1000;
    }

    /* Store data */
    ri->valbuf.f64   = value;
    ri->curraw       = raw;
    ri->curraw_dtype = raw_dtype;
    ri->rflags       = rflags;
    ri->timestamp    = timestamp;

    /* Call who requested */
    ci = AccessCtxSlot(ri->cid);

    call_info.uniq     = ci->uniq;
    call_info.privptr1 = ci->privptr1;
    call_info.reason   = CDA_REF_R_UPDATE;
    call_info.evmask   = 1 << call_info.reason;
    call_info.info_ptr = NULL;
    call_info.ref      = ref;
    ForeachRefCbSlot(ref_evproc_caller, &call_info, ri);
}

void  cda_dat_p_set_hwr            (cda_dataref_t  ref, int hwr)
{
  refinfo_t     *ri;

    if (CheckRef(ref) != 0)
    {
        /*!!!Bark? */
        return;
    }
    ri = AccessRefSlot(ref);

    if (ri->in_use != REF_TYPE_CHN)
    {
        /*!!!Bark? */
        return;
    }

    ri->hwr = hwr;
}

void  cda_dat_p_set_phys_rds       (cda_dataref_t  ref,
                                    int            phys_count,
                                    double        *rds)
{
  refinfo_t     *ri;

    if (CheckRef(ref) != 0)
    {
        /*!!!Bark? */
        return;
    }
    ri = AccessRefSlot(ref);

    if (ri->in_use != REF_TYPE_CHN)
    {
        /*!!!Bark? */
        return;
    }
    /*!!! Are any other checks sensible? */

    if (phys_count < 0)
    {
        /*!!!Bark? */
        return;
    }
    /* Store */
    if (phys_count > 0)
    {
        if (ri->alc_phys_rds != NULL  ||  phys_count > I_PHYS_COUNT)
        {
            if (GrowUnitsBuf(&(ri->alc_phys_rds), &(ri->phys_n_allocd),
                             phys_count, sizeof(double) * 2) < 0)
            {
                /*!!!Bark? */
                return;
            }
        }
        memcpy(ri->alc_phys_rds != NULL? ri->alc_phys_rds : ri->imm_phys_rds,
               rds, phys_count * sizeof(double) * 2);
    }
    ri->phys_count = phys_count;

    /*!!!notify users?*/
}

void  cda_dat_p_set_fresh_age      (cda_dataref_t  ref, int msecs)
{
  refinfo_t     *ri;

    if (CheckRef(ref) != 0)
    {
        /*!!!Bark? */
        return;
    }
    ri = AccessRefSlot(ref);

    if (ri->in_use != REF_TYPE_CHN)
    {
        /*!!!Bark? */
        return;
    }
    /*!!! Are any other checks sensible? */

    if (msecs > 0)
    {
        ri->fresh_age.sec  =  msecs / 1000000;
        ri->fresh_age.nsec = (msecs % 1000000) * 1000;
        ri->fresh_age_specified = 1;
    }
    else
        ri->fresh_age_specified = 0;
}

void  cda_dat_p_set_quant          (cda_dataref_t  ref, CxAnyVal_t q)
{
  refinfo_t     *ri;

    if (CheckRef(ref) != 0)
    {
        /*!!!Bark? */
        return;
    }
    ri = AccessRefSlot(ref);

    if (ri->in_use != REF_TYPE_CHN)
    {
        /*!!!Bark? */
        return;
    }
    /*!!! Are any other checks sensible? */

    ri->q = q;
}

void  cda_dat_p_set_strings        (cda_dataref_t  ref,
                                    const char    *ident,
                                    const char    *label,
                                    const char    *tip,
                                    const char    *comment,
                                    const char    *geoinfo,
                                    const char    *rsrvd6,
                                    const char    *units,
                                    const char    *dpyfmt)
{
  refinfo_t     *ri;
  ctxinfo_t     *ci;
  const char    *sl[REF_STRINGS_NUM];
  int            x;
  size_t         size;
  size_t         len1; // strlen()+1
  char          *new_strings_buf;

  ref_call_info_t  call_info;

    if (CheckRef(ref) != 0)
    {
        /*!!!Bark? */
        return;
    }
    ri = AccessRefSlot(ref);

    if (ri->in_use != REF_TYPE_CHN)
    {
        /*!!!Bark? */
        return;
    }
    /*!!! Are any other checks sensible? */

//fprintf(stderr, "\a%s()\n", __FUNCTION__);
    bzero(sl, sizeof(sl));
    sl[0] = ident;
    sl[1] = label;
    sl[2] = tip;
    sl[3] = comment;
    sl[4] = geoinfo;
    sl[5] = rsrvd6;
    sl[6] = units;
    sl[7] = dpyfmt;
    
    /* Count size */
    for (x = 0, size = 0;  x < countof(sl);  x++)
        if (sl[x] != NULL  &&  sl[x][0] != '\0')
            size += strlen(sl[x]) + 1;

    /* Re-alloc */
    new_strings_buf = safe_realloc(ri->strings_buf, size);
    if (new_strings_buf == NULL) return; // Ouch... Sad, but not fatal

    /* Store */
    ri->strings_buf = new_strings_buf;
    for (x = 0, size = 0;  x < countof(sl);  x++)
        if (sl[x] != NULL  &&  sl[x][0] != '\0')
        {
            ri->strings_ptrs[x] = ri->strings_buf + size;
            len1 = strlen(sl[x]) + 1;
            memcpy(ri->strings_ptrs[x], sl[x], len1);
            size += len1;
        }
        else
            ri->strings_ptrs[x] = NULL;

    /* Notify who requested */
    ci = AccessCtxSlot(ri->cid);

    call_info.uniq     = ci->uniq;
    call_info.privptr1 = ci->privptr1;
    call_info.reason   = CDA_REF_R_PROPSCHG;
    call_info.evmask   = 1 << call_info.reason;
    call_info.info_ptr = NULL;
    call_info.ref      = ref;
    ForeachRefCbSlot(ref_evproc_caller, &call_info, ri);
}

void *cda_dat_p_privp_of(cda_srvconn_t  sid)
{
  srvinfo_t *si = AccessSrvSlot(sid);
  
    if (CheckSid(sid) != 0) return 0;

    return si->pdt_privptr;
}

int   cda_dat_p_suniq_of(cda_srvconn_t  sid)
{
  srvinfo_t *si = AccessSrvSlot(sid);
  ctxinfo_t *ci;
  
    if (CheckSid(sid) != 0) return 0;

    ci = AccessCtxSlot(si->cid);
    return ci->uniq;
}

char *cda_dat_p_argv0_of(cda_srvconn_t  sid)
{
  srvinfo_t *si = AccessSrvSlot(sid);
  ctxinfo_t *ci;

    if (sid == 0)           return cda_progname;
    if (CheckSid(sid) != 0) return "_ARGV0_(ERROR_SID)";

    ci = AccessCtxSlot(si->cid);
    return ci->argv0;
}

char *cda_dat_p_srvrn_of(cda_srvconn_t  sid)
{
  srvinfo_t *si = AccessSrvSlot(sid);

    if (CheckSid(sid) != 0) return "_SRVRN_(ERROR_SID)";

    return si->srvrspec;
}

void cda_dat_p_report(cda_srvconn_t  sid, const char *format, ...)
{
  va_list     ap;
  const char *pn;
  const char *sr;
  char        sid_buf[20];
  
    va_start(ap, format);
#if 1
    pn = sr = NULL;
    if (sid >= 0)
    {
        pn = cda_dat_p_argv0_of(sid);
        sr = cda_dat_p_srvrn_of(sid);
        sprintf(sid_buf, "%d", sid);
    }
    if (pn == NULL) pn = "";
    if (sr == NULL) sr = "";

    fprintf (stderr, "%s ", strcurtime());
    if (*pn != '\0')
        fprintf(stderr, "%s: ", pn);
    fprintf (stderr, "cda");
    if (sid >= 0)
    {
        fprintf (stderr, "[%d", sid);
        if (*sr != '\0')
            fprintf (stderr, ":\"%s\"", sr);
        fprintf (stderr, "]");
    }
    fprintf (stderr, ": ");
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
    va_end(ap);
}

void cda_ref_p_report(cda_dataref_t  ref, const char *format, ...)
{
  va_list     ap;
  refinfo_t  *ri;
  ctxinfo_t  *ci;
  const char *pn;
  const char *rn;
  char        ref_buf[20];
  
    va_start(ap, format);
#if 1
    pn = rn = NULL;
    if (CheckRef(ref) == 0)
    {
        snprintf(ref_buf, sizeof(ref_buf), "%d", ref);
        rn = ref_buf;

        ri = AccessRefSlot(ref);
        if (ri != NULL  &&  CheckCid(ri->cid) == 0)
        {
            ci = AccessCtxSlot(ri->cid);
            pn = ci->argv0;
        }
    }

    if (pn == NULL) pn = "";
    if (rn == NULL) rn = "";

    fprintf (stderr, "%s ", strcurtime());
    if (*pn != '\0')
        fprintf(stderr, "%s: ", pn);
    fprintf (stderr, "cda");
    if (*rn != '\0')
        fprintf (stderr, "[ref=%s]", rn);
    fprintf (stderr, ": ");
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
    va_end(ap);
}

//////////////////////////////////////////////////////////////////////
int cda_getrefvnr(CxDataRef_t ref, double userval,
                  int flags,
                  double *result_p, rflags_t *rflags_p,
                  cx_ival_t *rv_p, int *rv_u_p)
{
}

static int ExecFormula(refinfo_t *ri, int options,
                       double userval,
                       CxKnobParam_t *params, int num_params)
{
  int         r;
  double      value;
  CxAnyVal_t  raw;
  cxdtype_t   raw_dtype;
  rflags_t    rflags;
  cx_time_t   timestamp;

  struct timeval   now;

    if (ri->fla_vmt          != NULL  &&
        ri->fla_vmt->execute != NULL)
    {
        r = ri->fla_vmt->execute(ri->fla_privptr, options,
                                 params, num_params,
                                 userval,
                                 &value,
                                 &raw, &raw_dtype,
                                 &rflags, &timestamp);
        if (r == CDA_PROCESS_DONE)
        {
            /* Treat unspecified timestamp as "now" */
            if (timestamp.sec == 0)
            {
                gettimeofday(&now, NULL);
                timestamp.sec  = now.tv_sec;
                timestamp.nsec = now.tv_usec * 1000;
            }
            
            /* Store data */
            ri->valbuf.f64   = value;
            ri->curraw       = raw;
            ri->curraw_dtype = raw_dtype;
            ri->rflags       = rflags;
            ri->timestamp    = timestamp;
        }
    }
    else
        r = CDA_PROCESS_FLAG_BUSY;

    return r;
}

int cda_process_ref(cda_dataref_t ref, int options,
                    /* Inputs */
                    double userval,
                    CxKnobParam_t *params, int num_params)
{
  int            ret = CDA_PROCESS_DONE;

  refinfo_t     *ri = AccessRefSlot(ref);
  srvinfo_t     *si;
  ctxinfo_t     *ci;
  CxKnobParam_t *p;

  int            repr;
  size_t         size;
  void          *vp;

  double         v;
  int            n;
  double        *rdp;

  float32        f32;
  int64          i64;
  int32          i32;
  int16          i16;
  int8           i8;

//fprintf(stderr, "%s(ref=%d, userval=%f)\n", __FUNCTION__, ref, userval);
    if (CheckRef(ref) != 0) return CDA_PROCESS_ERR;
    repr = reprof_cxdtype(ri->dtype);
    size = sizeof_cxdtype(ri->dtype);
    if (
        ri->nelems != 1
        ||
        (repr != CXDTYPE_REPR_INT  &&
         repr != CXDTYPE_REPR_FLOAT)
       )
    {
        errno = EINVAL;
        return CDA_PROCESS_ERR;
    }

    if (options & CDA_OPT_IS_W)
    {
        if      (ri->in_use == REF_TYPE_CHN)
        {
            /* {r,d} conversion */
            v   = userval;
            n   = ri->phys_count;
            rdp = ri->alc_phys_rds;
            if (rdp == NULL) rdp = ri->imm_phys_rds;
            rdp += (n-1)*2;
            while (n > 0)
            {
                v = (v + rdp[1]) * rdp[0];
                rdp -= 2;
                n--;
            }

            /* Type conversion */
            if    (repr == CXDTYPE_REPR_INT)
            {
                if      (size == sizeof(int32))    {i32 = round(v); vp = &i32;}
                else if (size == sizeof(int16))    {i16 = round(v); vp = &i16;}
                else if (size == sizeof(int64))    {i64 = round(v); vp = &i64;}
                else           /*sizeof(int8)*/    {i8  = round(v); vp = &i8; }
            }
            else /*repr == CXDTYPE_REPR_FLOAT*/
            {
                if      (size == sizeof(float64))            vp = &v;
                else           /*sizeof(float32)*/ {f32 = v; vp = &f32; }
            }
                
            
            /* Send */
            si = AccessSrvSlot(ri->sid);
            if ((options & CDA_OPT_READONLY) == 0)
                if (ri->sid >= 0  &&
                    si->metric->snd_data != NULL)
                    si->metric->snd_data(si->pdt_privptr,
                                         ri->hwr,
                                         ri->dtype, 1, vp);
        }
        else if (ri->in_use == REF_TYPE_FLA)
        {
            if (options & CDA_OPT_DO_EXEC)
                ret = ExecFormula(ri, options, userval, params, num_params);
        }
        else if (ri->in_use == REF_TYPE_REG)
        {
            ci = AccessCtxSlot(ri->cid);
            p  = AccessVarparmSlot(ri->hwr, ci);
            p->value = userval;
        }
    }
    else
    {
        if      (ri->in_use == REF_TYPE_FLA)
        {
            if (options & CDA_OPT_DO_EXEC)
                ret = ExecFormula(ri, options, userval, params, num_params);
        }
    }

    return ret;
}

int cda_get_ref_dval(cda_dataref_t ref,
                     double     *curv_p,
                     CxAnyVal_t *curraw_p, cxdtype_t *curraw_dtype_p,
                     rflags_t *rflags_p, cx_time_t *timestamp_p)
{
  refinfo_t     *ri = AccessRefSlot(ref);
  ctxinfo_t     *ci;
  CxKnobParam_t *p;
  double         v;

    if (CheckRef(ref) != 0) return -1;

    if (ri->nelems != 1)
    {
        errno = EINVAL;
        return -1;
    }
    /* REF_TYPE_REG -- get value from reg */
    if (ri->in_use == REF_TYPE_REG)
    {
        ci = AccessCtxSlot(ri->cid);
        p  = AccessVarparmSlot(ri->hwr, ci);
        v  = p->value;
    }
    else
    {
        /* Note: here we suppose that scalar values ALWAYS
           go from valbuf (because of guard "ri->nelems != 1" above).
           If this changes someday, data shoule be obtained via pointer. */
        if      (ri->dtype == CXDTYPE_DOUBLE) v = ri->valbuf.f64;
        else if (ri->dtype == CXDTYPE_SINGLE) v = ri->valbuf.f32;
        else if (ri->dtype == CXDTYPE_INT32)  v = ri->valbuf.i32;
        else if (ri->dtype == CXDTYPE_UINT32) v = ri->valbuf.u32;
        else if (ri->dtype == CXDTYPE_INT16)  v = ri->valbuf.i16;
        else if (ri->dtype == CXDTYPE_UINT16) v = ri->valbuf.u16;
        else if (ri->dtype == CXDTYPE_INT64)  v = ri->valbuf.i64;
        else if (ri->dtype == CXDTYPE_UINT64) v = ri->valbuf.u64;
        else if (ri->dtype == CXDTYPE_INT8)   v = ri->valbuf.i8;
        else if (ri->dtype == CXDTYPE_UINT8)  v = ri->valbuf.u8;
        else
        {
            errno = EINVAL;
            return -1;
        }
    }

    if (curv_p         != NULL) *curv_p         = v;
    if (curraw_p       != NULL) *curraw_p       = ri->curraw;
    if (curraw_dtype_p != NULL) *curraw_dtype_p = ri->curraw_dtype;
    if (rflags_p       != NULL) *rflags_p       = ri->rflags;
    if (timestamp_p    != NULL) *timestamp_p    = ri->timestamp;

    return 0;
}

int cda_snd_ref_data(cda_dataref_t ref,
                     cxdtype_t dtype, int nelems,
                     void *data)
{
}

int cda_stop_formula(cda_dataref_t ref)
{
  refinfo_t   *ri = AccessRefSlot(ref);

    if (CheckRef(ref) != 0) return -1;
    if (ri->in_use != REF_TYPE_FLA)
    {
        errno = EINVAL;
        return -1;
    }
    if (ri->fla_vmt       != NULL  &&
        ri->fla_vmt->stop != NULL)
        ri->fla_vmt->stop(ri->fla_privptr);

    return 0;
}

//////////////////////////////////////////////////////////////////////

const char *cda_strserverstatus_short(cda_serverstatus_t status)
{
    if      (status == CDA_SERVERSTATUS_NORMAL)       return "NORMAL";
    else if (status == CDA_SERVERSTATUS_ALMOSTREADY)  return "ALMOSTREADY";
    else if (status == CDA_SERVERSTATUS_FROZEN)       return "FROZEN";
    else if (status == CDA_SERVERSTATUS_DISCONNECTED) return "DISCONNECTED";
    else                                              return "UNKNOWN";
}

//////////////////////////////////////////////////////////////////////

static int cleanup_checker(ctxinfo_t *ci, void *privptr)
{
  int  uniq = *((int *)privptr);

    if (ci->uniq == uniq)
        cda_del_context(ci2cid(ci));

    return 0;
}
void  cda_do_cleanup(int uniq)
{
    ForeachCtxSlot(cleanup_checker, &uniq);
}
