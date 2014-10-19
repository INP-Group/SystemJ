#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <math.h>

#include "misc_macros.h"
#include "misclib.h"

#include "datatree.h"
#include "datatreeP.h"
#include "cda.h"
#include "Cdr.h"
#include "CdrP.h"
#include "Cdr_plugmgr.h"

#ifndef NAN
  #warning The NAN macro is undefined, using strtod("NAN") instead
  #define NAN strtod("NAN", NULL)
#endif


enum {NUMAVG = 30};
enum {MMXCAP = 30};


data_section_t *CdrCreateSection(DataSubsys sys,
                                 const char *type,
                                 char       *name, size_t namelen,
                                 void       *data, size_t datasize)
{
  data_section_t *nsp;
  data_section_t *new_sections;

    /* Grow the 'sections' array */
    new_sections = safe_realloc(sys->sections,
                                sizeof(data_section_t)
                                *
                                (sys->num_sections + 1));
    if (new_sections == NULL)
        return NULL;
    sys->sections = new_sections;

    /* Okay -- a new section slot, let's initialize it */
    nsp = new_sections + sys->num_sections;
    bzero(nsp, sizeof(*nsp));
    nsp->type = type;

    /* Allocate and fill the name */
    if (namelen != 0)
    {
        nsp->name = malloc(namelen + 1);
        if (nsp->name == NULL)
            return NULL;
        memcpy(nsp->name, name, namelen);
        nsp->name[namelen] = '\0';
    }

    /* Allocate data, if required */
    if (datasize != 0)
    {
        nsp->data = malloc(datasize);
        if (nsp->data == NULL)
            return NULL;

        if (data != NULL)
            memcpy(nsp->data, data, datasize);
        else
            bzero (nsp->data,       datasize);
    }
    else
        nsp->data = data;

    /* Reflect the existence of new section */
    sys->num_sections += 1;

    return sys->sections + sys->num_sections - 1;
}

DataSubsys  CdrLoadSubsystem(const char *def_scheme, const char *reference)
{
  DataSubsys          ret;
  char                scheme[20];
  const char         *location;
  CdrSubsysLdrModRec *metric;
  
    CdrClearErr();

    if (def_scheme != NULL  &&  *def_scheme == '!')
    {
        strzcpy(scheme, def_scheme + 1, sizeof(scheme));
        location = reference;
    }
    else
        split_url(def_scheme, "::",
                  reference, scheme, sizeof(scheme),
                  &location);

    metric = CdrGetSubsysLdrByScheme(scheme);

    if (metric == NULL)
    {
        CdrSetErr("unknown scheme \"%s\"", scheme);
        return NULL;
    }

    ret = metric->ldr(location);
    if (ret == NULL) return NULL;

    if (CdrFindSection(ret, DSTN_SYSNAME, NULL) == NULL)
        CdrCreateSection(ret, DSTN_SYSNAME,
                         NULL, 0,
                         location, strlen(location) + 1);
    
    ret->sysname       = CdrFindSection(ret, DSTN_SYSNAME,   NULL);
    ret->main_grouping = CdrFindSection(ret, DSTN_GROUPING,  "main");
    ret->defserver     = CdrFindSection(ret, DSTN_DEFSERVER, NULL);
    ret->treeplace     = CdrFindSection(ret, DSTN_TREEPLACE, NULL);
    
    return ret;
}


void     *CdrFindSection    (DataSubsys subsys, const char *type, const char *name)
{
  int  n;
    
    for (n = 0;  n < subsys->num_sections;  n++)
        if (
            (strcasecmp(type, subsys->sections[n].type) == 0)
            &&
            (name == NULL  ||
             strcasecmp(name, subsys->sections[n].name) == 0)
           )
            return subsys->sections[n].data;

    return NULL;
}

DataKnob  CdrGetMainGrouping(DataSubsys subsys)
{
    if (subsys == NULL) return NULL;
    return subsys->main_grouping;
}

void  CdrDestroySubsystem(DataSubsys subsys)
{
  int             n;
  data_section_t *p;
    
    if (subsys == NULL) return;

    for (n = 0, p = subsys->sections;
         n < subsys->num_sections;
         n++,   p++)
    {
        if      (p->destroy != NULL)
            p->destroy(p);
        else if (strcasecmp(p->type, DSTN_GROUPING) == 0)
            CdrDestroyKnobs(p->data, 1), p->data = NULL;

        /* DSTN_DEFSERVER and DSTN_TREEPLACE are just free()'d */
        
        safe_free(p->name);  p->name = NULL;
        safe_free(p->data);  p->data = NULL;
    }

    free(subsys);
}

void  CdrDestroyKnobs(DataKnob list, int count)
{
  int       n;
  DataKnob  k;
  int       pn;
  int       rn;
    
    if (list == NULL  ||  count <= 0) return;

    for (n = 0, k = list;
         n < count;
         n++,   k++)
    {
        /* First, do type-dependent things... */
        switch (k->type)
        {
            case DATAKNOB_GRPG:
                /* Fallthrough to CONT */
            case DATAKNOB_CONT:
                CdrDestroyKnobs(k->u.c.content, k->u.c.count);
                safe_free(k->u.c.content); k->u.c.content = NULL; k->u.c.count = 0;

                safe_free(k->u.c.refbase); k->u.c.refbase = NULL;
                safe_free(k->u.c.str1);    k->u.c.str1    = NULL;
                safe_free(k->u.c.str2);    k->u.c.str2    = NULL;
                safe_free(k->u.c.str3);    k->u.c.str3    = NULL;

                safe_free(k->u.c.at_init_src); k->u.c.at_init_src = NULL;
                safe_free(k->u.c.ap_src);  k->u.c.ap_src  = NULL;
                /*!!! cda_del_chan() references? W/prelim cda_del_chan_evproc()? */
                break;
                
            case DATAKNOB_NOOP:
                /* Nothing to do */
                break;
                
            case DATAKNOB_KNOB:
                /*!!!*/
                safe_free(k->u.k.units);   k->u.k.units   = NULL;
                safe_free(k->u.k.items);   k->u.k.items   = NULL;
                safe_free(k->u.k.rd_src);  k->u.k.rd_src  = NULL;
                safe_free(k->u.k.wr_src);  k->u.k.wr_src  = NULL;
                safe_free(k->u.k.cl_src);  k->u.k.cl_src  = NULL;

                for (pn = 0;  pn < k->u.k.num_params; pn++)
                {
                    safe_free(k->u.k.params[pn].ident);
                    safe_free(k->u.k.params[pn].label);
                }
                safe_free(k->u.k.params);  k->u.k.params  = NULL; k->u.k.num_params = 0;

                /* Do kind-dependent things */
                if      (k->u.k.kind == DATAKNOB_KIND_DEVN)
                {
                    /* Do-nothing */
                }
                else if (k->u.k.kind == DATAKNOB_KIND_MINMAX)
                {
                    safe_free(k->u.k.kind_var.minmax.buf);
                    k->u.k.kind_var.minmax.buf = NULL;
                }
                break;
                
            case DATAKNOB_BIGC:
                /*!!! cda_del_chan() references? W/prelim cda_del_chan_evproc()? */
                break;
                
            case DATAKNOB_USER:
                for (rn = 0;  rn < k->u.u.num_datarecs;  rn++)
                {
                    if (cda_ref_is_sensible(k->u.u.datarecs[n].dr)) /*!!!*/;
                }
                safe_free(k->u.u.datarecs); k->u.u.datarecs = NULL; k->u.u.num_datarecs = 0;
                for (rn = 0;  rn < k->u.u.num_bigcrecs;  rn++)
                {
                    /*!!! cda_del_chan() references? W/prelim cda_del_chan_evproc()? */
                }
                safe_free(k->u.u.bigcrecs); k->u.u.bigcrecs = NULL; k->u.u.num_bigcrecs = 0;
                break;
        }

        /* ...and than common ones */
        safe_free(k->look);    k->look    = NULL;
        safe_free(k->options); k->options = NULL;
        safe_free(k->ident);   k->ident   = NULL;
        safe_free(k->label);   k->label   = NULL;
        safe_free(k->tip);     k->tip     = NULL;
        safe_free(k->comment); k->comment = NULL;
        safe_free(k->style);   k->style   = NULL;
        safe_free(k->layinfo); k->layinfo = NULL;
        safe_free(k->geoinfo); k->geoinfo = NULL;
        safe_free(k->conns_u); k->conns_u = NULL;
    }
}


void  CdrSetSubsystemRO  (DataSubsys  subsys, int ro)
{
    subsys->readonly = (ro != 0);
}


static void ProcessContextEvent(int            uniq,
                                void          *privptr1,
                                cda_context_t  cid,
                                int            reason,
                                int            info_int,
                                void          *privptr2)
{
  DataSubsys      subsys = privptr1;

//fprintf(stderr, "%s()\n", __FUNCTION__);
    switch (reason)
    {
        case CDA_CTX_R_CYCLE:
            CdrProcessSubsystem(subsys, info_int, 0, NULL);
            break;
    }
}

static void CallAtInitOfKnobs(DataKnob list, int count)
{
  int       n;
  DataKnob  k;
    
    if (list == NULL  ||  count <= 0) return;

    for (n = 0, k = list;
         n < count;
         n++,   k++)
    {
        switch (k->type)
        {
            case DATAKNOB_GRPG:
                /* Fallthrough to CONT */
            case DATAKNOB_CONT:
                cda_getrefvnr(k->u.c.at_init_ref, 0.0,
                              CDA_OPT_DO_EXEC,
                              NULL, NULL,
                              NULL, NULL);
                break;
        }
    }
}
int   CdrRealizeSubsystem(DataSubsys subsys, const char *defserver)
{
  int             n;
  data_section_t *p;
  int             r;

  static const char FROM_CMDLINE_STR[] = "from_cmdline";

    CdrClearErr();

    if (subsys == NULL) return -1;

    if (defserver != NULL  &&  *defserver != '\0')
    {
        p = CdrCreateSection(subsys,
                             DSTN_DEFSERVER,
                             FROM_CMDLINE_STR, strlen(FROM_CMDLINE_STR),
                             defserver, strlen(defserver) + 1);
        if (p == NULL) return -1;
        subsys->defserver = p->data;
    }

    /* 0. Create context */
    subsys->cid = cda_new_context(0, subsys,
                                  subsys->defserver, 0,
                                  NULL/*!!!argv[0]*/,
                                  CDA_CTX_EVMASK_CYCLE, ProcessContextEvent, NULL);
    if (subsys->cid < 0)
    {
        CdrSetErr("cda_new_context(): %s", cda_last_err());
        return -1;
    }

    /* 1st stage -- perform binding */
    for (n = 0, p = subsys->sections;
         n < subsys->num_sections;
         n++,   p++)
        if (strcasecmp(p->type, DSTN_GROUPING) == 0)
        {
            r = CdrRealizeKnobs(subsys,
                                "",
                                p->data, 1);
            if (r < 0) return r;
        }
    /* 2nd stage -- trigger all at_init_ref's */
    for (n = 0, p = subsys->sections;
         n < subsys->num_sections;
         n++,   p++)
        if (strcasecmp(p->type, DSTN_GROUPING) == 0)
            CallAtInitOfKnobs(p->data, 1);

    return 0;
}

static cda_dataref_t cvt2ref(DataSubsys     subsys,
                             const char    *curbase,
                             const char    *src,
                             int            rw,
                             CxKnobParam_t *params,
                             int            num_params,
                             int                   evmask,
                             cda_dataref_evproc_t  evproc,
                             void                 *privptr2)
{
  enum
  {
      SRC_IS_CHN = 1,
      SRC_IS_REG = 2,
      SRC_IS_FLA = 3,
  }           srctype;
  const char *p;
  int         is_chanref_char;
  cxdtype_t   dtype = CXDTYPE_DOUBLE;
  const char *chn;
  char        ut_char;

    if (src == NULL  ||  *src == '\0') return CDA_NONE;

    if      (*src == '%')
        srctype = SRC_IS_REG;
    else if (*src == '#')
        srctype = SRC_IS_FLA;
    else
    {
        for (p = src, srctype = SRC_IS_CHN;
             *p != '\0';
             p++)
        {
            is_chanref_char =
                isalnum(*p)  ||  *p == '_'  ||  *p == '-'  ||
                *p == '.'  ||  *p == ':'  ||  *p == '@';
            if (!is_chanref_char)
            {
                srctype = SRC_IS_FLA;
                break;
            }
        }
    }

    if      (srctype == SRC_IS_CHN)
    {
        chn = src;
        if (*chn == '@')
        {
            chn++;
            ut_char = tolower(*chn);
            if      (ut_char == 'b') dtype = CXDTYPE_INT8;
            else if (ut_char == 'h') dtype = CXDTYPE_INT16;
            else if (ut_char == 'i') dtype = CXDTYPE_INT32;
            else if (ut_char == 'q') dtype = CXDTYPE_INT64;
            else if (ut_char == 's') dtype = CXDTYPE_SINGLE;
            else if (ut_char == 'd') dtype = CXDTYPE_DOUBLE;
            else if (ut_char == '\0')
            {
                fprintf(stderr, "%s %s: data-type expected after '@' in \"%s\" spec\n",
                        strcurtime(), __FUNCTION__, src);
                return CDA_NONE;
            }
            else
            {
                fprintf(stderr, "%s %s: invalid channel-data-type after '@' in \"%s\" spec\n",
                        strcurtime(), __FUNCTION__, src);
                return CDA_NONE;
            }
            chn++;
            if (*chn != ':')
            {
                fprintf(stderr, "%s %s: ':' expected after \"@%c\" in \"%s\" spec\n",
                        strcurtime(), __FUNCTION__, ut_char, src);
                return CDA_NONE;
            }
            else
                chn++;
        }

        return cda_add_chan   (subsys->cid, curbase,
                               chn, rw? CDA_OPT_IS_W : CDA_OPT_NONE, dtype, 1,
                               evmask, evproc, privptr2);
    }
    else if (srctype == SRC_IS_REG)
        return cda_add_varchan(subsys->cid, src + 1);
    else if (srctype == SRC_IS_FLA)
        return cda_add_formula(subsys->cid, curbase,
                               src, rw? CDA_OPT_IS_W : CDA_OPT_NONE,
                               params, num_params,
                               0, NULL, NULL);
    else return CDA_NONE;
}

static cda_dataref_t src2ref(DataKnob k,
                             const char    *src_name,
                             DataSubsys     subsys,
                             const char    *curbase,
                             const char    *src,
                             int            rw,
                             CxKnobParam_t *params,
                             int            num_params,
                             int                   evmask,
                             cda_dataref_evproc_t  evproc,
                             void                 *privptr2)
{
  cda_dataref_t  ref = cvt2ref(subsys, curbase,
                               src, rw,
                               params, num_params,
                               evmask, evproc, privptr2);

    if (ref == CDA_ERR)
        fprintf(stderr, "%s %s: %s\n", strcurtime(), src_name, cda_last_err());

    /*!!! No, should also check somehow else for bad results --
     by CXCF_FLAG_NOTFOUND */

    return ref;
}

static inline int isempty(const char *s)
{
    return s == NULL  ||  *s == '\0';
}

static void UpdateStr(char **pp, const char *new_value)
{
  char *new_ptr;

    if (new_value != NULL  &&  *new_value != '\0')
    {
        new_ptr = safe_realloc(*pp, strlen(new_value) + 1);
        if (new_ptr == NULL) return;
        strcpy(new_ptr, new_value);
        *pp = new_ptr;
    }
    else
    {
        /* Non-NULL?  Just set to epmty */
        if (*pp != NULL) *pp = '\0';
        /* Otherwise do-nothing */
    }
}
static void RefPropsChgEvproc(int            uniq,
                              void          *privptr1,
                              cda_dataref_t  ref,
                              int            reason,
                              void          *info_ptr,
                              void          *privptr2)
{
  DataKnob     k     = privptr2;
  data_knob_t  old_k = *k;
  char        *ident;
  char        *label;
  char        *tip;
  char        *comment;
  char        *geoinfo;
  char        *rsrvd6;
  char        *units;
  char        *dpyfmt;

  int     flags;
  int     width;
  int     precision;
  char    conv_c;

//fprintf(stderr, "\a%s()\n", __FUNCTION__);
    /* 1. Get info */
    if (cda_strings_of_ref(ref,
                           &ident, &label,
                           &tip, &comment, &geoinfo, &rsrvd6,
                           &units, &dpyfmt) < 0) return;
    /* 2. Update "unspecified" fields */
    if (k->strsbhvr & DATAKNOB_STRSBHVR_IDENT)   UpdateStr(&(k->ident),     ident);
    if (k->strsbhvr & DATAKNOB_STRSBHVR_LABEL)   UpdateStr(&(k->label),     label);
    if (k->strsbhvr & DATAKNOB_STRSBHVR_TIP)     UpdateStr(&(k->tip),       tip);
    if (k->strsbhvr & DATAKNOB_STRSBHVR_COMMENT) UpdateStr(&(k->comment),   comment);
    if (k->strsbhvr & DATAKNOB_STRSBHVR_GEOINFO) UpdateStr(&(k->geoinfo),   geoinfo);
    if (k->strsbhvr & DATAKNOB_STRSBHVR_UNITS)   UpdateStr(&(k->u.k.units), units);
    if (k->strsbhvr & DATAKNOB_STRSBHVR_DPYFMT)
    {
        /* Clever "duplication" of dpyfmt */
        if (ParseDoubleFormat(GetTextFormat(dpyfmt),
                              &flags, &width, &precision, &conv_c) >= 0)
            CreateDoubleFormat(&(k->u.k.dpyfmt), sizeof(k->u.k.dpyfmt), 20, 10,
                               flags, width, precision, conv_c);
    }
                      
    /* 3. Notify knobplugin... */
    if (k->vmtlink != NULL                 &&
        k->vmtlink->type == DATAKNOB_KNOB  &&
        ((dataknob_knob_vmt_t *)(k->vmtlink))->PropsChg != NULL)
        ((dataknob_knob_vmt_t *)(k->vmtlink))->PropsChg(k, &old_k);
    /* 4. ...and parent */
    if (k->uplink != NULL                     &&
        (k->uplink->type == DATAKNOB_GRPG  ||
         k->uplink->type == DATAKNOB_CONT)    &&
        k->uplink->vmtlink != NULL            &&
        ((dataknob_cont_vmt_t *)(k->uplink->vmtlink))->ChildPropsChg != NULL)
        ((dataknob_cont_vmt_t *)(k->uplink->vmtlink))->ChildPropsChg(k->uplink,
                                                                     k - k->uplink->u.c.content,
                                                                     &old_k);
    
}

int   CdrRealizeKnobs    (DataSubsys  subsys,
                          const char *baseref,
                          DataKnob list, int count)
{
  int       n;
  DataKnob  k;
  char      curbase[CDA_PATH_MAX];
  char     *curbptr;
    
    if (list == NULL  ||  count <= 0) return -1;

    for (n = 0, k = list;
         n < count;
         n++,   k++)
    {
        if (isempty(k->ident))   k->strsbhvr |= DATAKNOB_STRSBHVR_IDENT;
        if (isempty(k->label))   k->strsbhvr |= DATAKNOB_STRSBHVR_LABEL;
        if (isempty(k->tip))     k->strsbhvr |= DATAKNOB_STRSBHVR_TIP;
        if (isempty(k->comment)) k->strsbhvr |= DATAKNOB_STRSBHVR_COMMENT;
        if (isempty(k->geoinfo)) k->strsbhvr |= DATAKNOB_STRSBHVR_GEOINFO;

        /* First, do type-dependent things... */
        switch (k->type)
        {
            case DATAKNOB_GRPG:
                /* Fallthrough to CONT */
            case DATAKNOB_CONT:
                /*!!! baseref+k->u.c.refbase wizardry?*/
#if 1
                if (k->u.c.refbase != NULL  &&  k->u.c.refbase[0] != '\0')
                    curbptr = cda_combine_base_and_spec(subsys->cid,
                                                        baseref, k->u.c.refbase,
                                                        curbase, sizeof(curbase));
                else
                    curbptr = baseref;

                CdrRealizeKnobs(subsys,
                                curbptr,
                                k->u.c.content, k->u.c.count);
#else
                curbase[0] = '\0';
                if      (k->u.c.refbase != NULL  &&  k->u.c.refbase[0] != '\0')
                    strzcpy(curbase, k->u.c.refbase, sizeof(curbase));
                else if (baseref != NULL)
                    strzcpy(curbase, baseref, sizeof(curbase));
                //curbase[0] = '\0';

                CdrRealizeKnobs(subsys,
                                curbase,
                                k->u.c.content, k->u.c.count);
#endif

                k->u.c.at_init_ref = src2ref(k, "at_init",
                                             subsys, baseref,
                                             k->u.c.at_init_src, 0,
                                             NULL, 0,
                                             0, NULL, NULL);
                k->u.c.ap_ref      = src2ref(k, "ap",
                                             subsys, baseref,
                                             k->u.c.ap_src     , 0,
                                             NULL, 0,
                                             0, NULL, NULL);
                break;
                
            case DATAKNOB_NOOP:
                /* Nothing to do */
                break;
                
            case DATAKNOB_KNOB:
                if (isempty(k->u.k.units))    k->strsbhvr |= DATAKNOB_STRSBHVR_UNITS;
                if (k->u.k.dpyfmt[0] == '\0') k->strsbhvr |= DATAKNOB_STRSBHVR_DPYFMT;

                k->u.k.rd_ref      = src2ref(k, "rd",
                                             subsys, baseref,
                                             k->u.k.rd_src    , 1,
                                             k->u.k.params, k->u.k.num_params,
                                             k->strsbhvr == 0? 0 : CDA_REF_EVMASK_PROPSCHG,
                                             RefPropsChgEvproc, k);
                k->u.k.wr_ref      = src2ref(k, "wr",
                                             subsys, baseref,
                                             k->u.k.wr_src    , 1,
                                             k->u.k.params, k->u.k.num_params,
                                             0, NULL, NULL);
                k->u.k.cl_ref      = src2ref(k, "cl",
                                             subsys, baseref,
                                             k->u.k.cl_src    , 0,
                                             k->u.k.params, k->u.k.num_params,
                                             0, NULL, NULL);
                /* Perform kind-dependent preparations */
                if      (k->u.k.kind == DATAKNOB_KIND_DEVN)
                {
                    /* Do-nothing */
                }
                else if (k->u.k.kind == DATAKNOB_KIND_MINMAX)
                {
                    k->u.k.kind_var.minmax.bufcap = MMXCAP;
                    k->u.k.kind_var.minmax.buf    = malloc(k->u.k.kind_var.minmax.bufcap * sizeof(k->u.k.kind_var.minmax.buf[0]));
                    if (k->u.k.kind_var.minmax.buf == NULL) return -1;
                }
                break;
                
            case DATAKNOB_BIGC:
                /*!!!*/
                break;
                
            case DATAKNOB_USER:
                /*!!!*/
                break;
        }
    }

    return 0;
}


void  CdrProcessSubsystem(DataSubsys subsys, int cause_conn_n,  int options,
                                                     rflags_t *rflags_p)
{
  int             n;
  data_section_t *p;

  rflags_t        rflags;
  rflags_t        cml_rflags = 0;

    for (n = 0, p = subsys->sections;
         n < subsys->num_sections;
         n++,   p++)
        if (strcasecmp(p->type, DSTN_GROUPING) == 0)
        {
            CdrProcessKnobs(subsys, cause_conn_n, options,
                            p->data, 1, &rflags);
            cml_rflags   |= rflags;
        }

    if (rflags_p != NULL) *rflags_p = cml_rflags;
}

void  CdrProcessKnobs    (DataSubsys subsys, int cause_conn_n, int options,
                          DataKnob list, int count, rflags_t *rflags_p)
{
  int         n;
  DataKnob    k;

  int         cda_opts = ((options & CDR_OPT_READONLY) ||
                          subsys->readonly)? CDA_OPT_READONLY : 0;

  rflags_t    rflags;
  rflags_t    rflags2;
  rflags_t    cml_rflags = 0;

  double      v;
  double      cl_v;
  CxAnyVal_t  raw;
  cxdtype_t   raw_dtype;
  time_t      timenow = time(NULL);

  int         rn;
  
  /* For LOGT_DEVN */
  double      newA;
  double      newA2;
  double      divider;

  /* For LOGT_MINMAX */
  double      min, max;
  double     *dp;
  int         i;

    for (n = 0, k = list;
         n < count;
         n++,   k++)
    {
        if (cause_conn_n >= 0              &&
            k->conns_u != NULL             &&
            k->conns_u[cause_conn_n] == 0)
        {
            rflags = k->currflags;
            goto ACCUMULATE_RFLAGS;
        }

        rflags = 0;
        switch (k->type)
        {
            case DATAKNOB_GRPG:
                /* Fallthrough to CONT */
            case DATAKNOB_CONT:
                if (cda_ref_is_sensible(k->u.c.ap_ref))
                {
#if 1
                    cda_process_ref (k->u.c.ap_ref, CDA_OPT_DO_EXEC | cda_opts,
                                     NAN,
                                     k->u.k.params, k->u.k.num_params);
                    cda_get_ref_dval(k->u.c.ap_ref,
                                     NULL,
                                     NULL, NULL,
                                     &rflags2, NULL/*!!!timestamp*/);
#else
                    cda_getrefvnr(k->u.c.ap_ref, 0.0,
                                  CDA_OPT_DO_EXEC | cda_opts,
                                  NULL, &rflags2,
                                  NULL, NULL);
#endif
                }
                else
                    rflags2 = 0;

                CdrProcessKnobs(subsys, cause_conn_n, options,
                                k->u.c.content, k->u.c.count, &rflags);
                rflags |= rflags2;

                if (((dataknob_cont_vmt_t*)(k->vmtlink))->NewData != NULL)
                    ((dataknob_cont_vmt_t*)(k->vmtlink))->NewData(k,
                                                                  (options & CDR_OPT_SYNTHETIC) != 0);
                break;
                
            case DATAKNOB_NOOP:
                /* Nothing to do */
                break;
                
            case DATAKNOB_KNOB:
                /* Obtain a value... */
                if (cda_ref_is_sensible(k->u.k.rd_ref))
                {
#if 1
                    cda_process_ref (k->u.k.rd_ref, CDA_OPT_DO_EXEC | cda_opts,
                                     NAN,
                                     k->u.k.params, k->u.k.num_params);
                    cda_get_ref_dval(k->u.k.rd_ref,
                                     &v,
                                     &raw, &raw_dtype,
                                     &rflags, NULL/*!!!timestamp*/);
#else
                    cda_getrefvnr(k->u.k.rd_ref, 0.0,
                                  CDA_OPT_DO_EXEC | cda_opts,
                                  &v, &rflags,
                                  &raw, &raw_dtype);
#endif
                }
                else
                {
                    v         = NAN;
                    rflags    = 0;
                    raw_dtype = CXDTYPE_UNKNOWN;
                }
                rflags &= (CXRF_SERVER_MASK | CXCF_FLAG_CDA_MASK);

                /* Perform kind-dependent operations with a value */
                if      (k->u.k.kind == DATAKNOB_KIND_DEVN)
                {
                    /*!!! Should check for synthetic update! */
    
                    /* Initialize average value with current */
                    if (k->u.k.kind_var.devn.notnew == 0)
                    {
                        k->u.k.kind_var.devn.avg  = v;
                        k->u.k.kind_var.devn.avg2 = v*v;
    
                        k->u.k.kind_var.devn.notnew = 1;
                    }
    
                    /* Recursively calculate next avg and avg2 */
                    newA  = (k->u.k.kind_var.devn.avg * NUMAVG + v)
                        /
                        (NUMAVG + 1);
                    
                    newA2 = (k->u.k.kind_var.devn.avg2 * NUMAVG + v*v)
                        /
                        (NUMAVG + 1);
                    
                    /* Store them */
                    k->u.k.kind_var.devn.avg  = newA;
                    k->u.k.kind_var.devn.avg2 = newA2; 
                    
                    /* And substitute the value of 'v' with the deviation */
                    divider = fabs(newA2);
                    if (divider < 0.00001) divider = 0.00001;
                    
                    v =  (
                          sqrt(fabs(newA2 - newA*newA))
                          /
                          divider
                         ) * 100;
                }
                else if (k->u.k.kind == DATAKNOB_KIND_MINMAX)
                {
                    /* Store current value */
                    if ((options & CDR_OPT_SYNTHETIC) == 0) /* On real, non-synthetic updates only */
                    {
                        if (k->u.k.kind_var.minmax.bufused == k->u.k.kind_var.minmax.bufcap)
                        {
                            k->u.k.kind_var.minmax.bufused--;
                            memmove(&(k->u.k.kind_var.minmax.buf[0]),  &(k->u.k.kind_var.minmax.buf[1]),
                                    k->u.k.kind_var.minmax.bufused * sizeof(k->u.k.kind_var.minmax.buf[0]));
                        }
                        k->u.k.kind_var.minmax.buf[k->u.k.kind_var.minmax.bufused++] = v;
                    }
                    else
                    {
                        /* Here we handle a special case -- when synthetic update occurs before
                         any real data arrival, so we must use one minmax cell, otherwise we
                         just replace the last value */
                        if (k->u.k.kind_var.minmax.bufused == 0)
                            k->u.k.kind_var.minmax.buf[k->u.k.kind_var.minmax.bufused++] = v;
                        else
                            k->u.k.kind_var.minmax.buf[k->u.k.kind_var.minmax.bufused-1] = v;
                    }
                    
                    /* Find min & max */
                    for (i = 0,  dp = k->u.k.kind_var.minmax.buf,  min = max = *dp;
                         i < k->u.k.kind_var.minmax.bufused;
                         i++, dp++)
                    {
                        if (*dp < min) min = *dp;
                        if (*dp > max) max = *dp;
                    }
                    
                    v = fabs(max - min);
                }

                /* A separate colorization? */
                if (cda_ref_is_sensible(k->u.k.cl_ref))
                {
#if 1
                    cda_process_ref (k->u.k.cl_ref, CDA_OPT_DO_EXEC | cda_opts,
                                     NAN,
                                     k->u.k.params, k->u.k.num_params);
                    cda_get_ref_dval(k->u.k.cl_ref,
                                     &cl_v,
                                     NULL, NULL,
                                     &rflags2, NULL);
#else
                    cda_getrefvnr(k->u.k.cl_ref, 0.0,
                                  CDA_OPT_DO_EXEC | cda_opts,
                                  &cl_v, &rflags2,
                                  NULL, NULL);
#endif
                    rflags |= rflags2;

                    k->u.k.curcolv = cl_v;
                }
                
                /* Store new values... */
                k->u.k.curv           = v;
                k->u.k.curv_raw       = raw;
                k->u.k.curv_raw_dtype = raw_dtype;

                /* ...and store in history buffer, if present and if required */

                /*!!! Make decisions about behaviour/colorization/etc.*/
                if (k->u.k.attn_endtime != 0  &&
                    difftime(timenow, k->u.k.attn_endtime) >= 0)
                    k->u.k.attn_endtime = 0;
                rflags = choose_knob_rflags(k, rflags,
                                            cda_ref_is_sensible(k->u.k.cl_ref)? cl_v : v);

                if (rflags & CXCF_FLAG_OTHEROP) set_knob_otherop(k, 1);

                //!!! Here is a place for alarm detection
                if ((rflags ^ k->currflags) & CXCF_FLAG_ALARM_ALARM)
                    /*!!!SetAlarm(k, (rflags & CXCF_FLAG_ALARM_ALARM) != 0)*/;

                k->currflags           = rflags;

                /* Display the result -- ... */
                /* ...colorize... */
                set_knobstate(k, choose_knobstate(k, rflags));
                /* ...and finally show the value */
                if (
                    !k->u.k.is_rw  ||
                    (
                     difftime(timenow, k->u.k.usertime) > KNOBS_USERINACTIVITYLIMIT  &&
                     !k->u.k.wasjustset
                    )
                   )
                    set_knob_controlvalue(k, v, 0);
                
                if ((options & CDR_OPT_SYNTHETIC) == 0) k->u.k.wasjustset = 0;
                
                break;
                
            case DATAKNOB_BIGC:
                /*!!!*/
                break;
                
            case DATAKNOB_USER:
                for (rn = 0;  rn < k->u.u.num_datarecs;  rn++)
                {
#if 1
                    cda_process_ref (k->u.u.datarecs[rn].dr, CDA_OPT_DO_EXEC | cda_opts,
                                     NAN,
                                     NULL, 0);
                    cda_get_ref_dval(k->u.u.datarecs[rn].dr,
                                     &v,
                                     NULL, NULL,
                                     &rflags2, NULL);
#else
                    cda_getrefvnr(k->u.u.datarecs[rn].dr, 0.0,
                                  CDA_OPT_DO_EXEC | cda_opts,
                                  &v, &rflags2,
                                  NULL, NULL);
#endif
                    if (!(k->u.u.datarecs[rn].is_rw))
                        rflags2 &=~ CXCF_FLAG_4WRONLY_MASK;

                    rflags |= rflags2;
                }
                for (rn = 0;  rn < k->u.u.num_bigcrecs;  rn++)
                {
                }
                /*!!!*/
                break;
        }
        k->currflags  = rflags;
        
 ACCUMULATE_RFLAGS:
        cml_rflags   |= rflags;
    }

    if (rflags_p != NULL) *rflags_p = cml_rflags;
}


int   CdrSetKnobValue(DataSubsys subsys, DataKnob  k, double v, int options)
{
  CxDataRef_t  wr;
  int          ret;
  int          cda_opts = ((options & CDR_OPT_READONLY) ||
                           subsys->readonly)? CDA_OPT_READONLY : 0;

    /*!!! Should also check being_modified */
    /*!!! Allow CONTainers to be "writable" -- to set state/subelement
      (see bigfile-0002.html 30.03.2011) */
    if (k->type != DATAKNOB_KNOB  ||  !k->u.k.is_rw)
    {
        errno = EINVAL;
        return CDA_PROCESS_ERR;
    }

    /*!!! some status mangling... */

    /**/
    wr                               = k->u.k.wr_ref;
    if (!cda_ref_is_sensible(wr)) wr = k->u.k.rd_ref;
    if (!cda_ref_is_sensible(wr)) return CDA_PROCESS_DONE;

    /*!!! Should set and later drop being_modified */
    if (k->u.k.being_modified)
    {
        errno = EINPROGRESS;
        return CDA_PROCESS_ERR;
    }
    k->u.k.being_modified = 1;

#if 1
    ret = cda_process_ref(wr, CDA_OPT_IS_W | CDA_OPT_DO_EXEC | cda_opts,
                          v,
                          k->u.k.params, k->u.k.num_params);
    if (ret != CDA_PROCESS_ERR  &&  (ret & CDA_PROCESS_FLAG_REFRESH) != 0)
        CdrProcessSubsystem(subsys, -1, CDR_OPT_SYNTHETIC | cda_opts, NULL);
#else
    ret = cda_getrefvnr(wr, v,
                        CDA_OPT_IS_W | CDA_OPT_DO_EXEC,
                        NULL, NULL,
                        NULL, NULL);
#endif

    k->u.k.being_modified = 0;

    return ret;
}

int   CdrBroadcastCmd(DataKnob  k, const char *cmd)
{
  int  r = 0;
  int  n;
    
    if (k == NULL) return -1;
  
    if (k->vmtlink            != NULL  &&
        k->vmtlink->HandleCmd != NULL)
        r = k->vmtlink->HandleCmd(k, cmd);
    
    if (r != 0) return r;

    if (k->type == DATAKNOB_GRPG  ||  k->type == DATAKNOB_CONT)
        for (n = 0;
             n < k->u.c.count  &&  r == 0;
             n++)
            r = CdrBroadcastCmd(k->u.c.content + n, cmd);

    return r;
}
