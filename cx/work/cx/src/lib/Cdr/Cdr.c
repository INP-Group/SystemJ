#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "cx_sysdeps.h"

#include "misc_macros.h"
#include "misclib.h"
#include "memcasecmp.h"
#include "timeval_utils.h"

#include "cxlib.h"  // For cx_strerror()
#include "Cdr.h"
#include "Knobs_typesP.h"

#ifndef NAN
  #warning The NAN macro is undefined, using strtod("NAN") instead
  #define NAN strtod("NAN", NULL)
#endif


static char progname[40] = "";

static void reporterror(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));

static void reporterror(const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
#if 1
    fprintf (stderr, "%s %s%sCdr: ",
             strcurtime(), progname, progname[0] != '\0' ? ": " : "");
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
    va_end(ap);
}

/********************************************************************/

static char _Cdr_lasterr_str[200] = "";
#define CLEAR_ERR() _Cdr_lasterr_str[0] = '\0'

CdrKnobUnhiliter_t cdr_unhilite_knob_hook = NULL;


enum {NUMAVG = 30};

enum {USERINACTIVITYLIMIT = 60};  // Seconds between "input" turns back to autoupdate
enum {RELAX_PERIOD        = 10};  // Seconds for green "relax" bg
enum {OTHEROP_PERIOD      = 5};   // Seconds for orange "other op" bg


static /*inline*/ int astrcpy(char **dst, const char *src)
{
    if (src == NULL) {*dst = NULL;  return 0;}
    return (*dst = strdup(src)) == NULL? -1 : 0;
}


/*!!!INTERNAL ( */

static void SetRelaxing(knobinfo_t *ki, int onoff)
{
    datatree_set_attn(ki, COLALARM_RELAX,   onoff, RELAX_PERIOD);
}

static void SetOtherop(knobinfo_t *ki, int onoff)
{
    datatree_set_attn(ki, COLALARM_OTHEROP, onoff, OTHEROP_PERIOD);
}

static void SetAlarm  (knobinfo_t *ki, int onoff)
{
  eleminfo_t *ei = ki->uplink;
  
    if (ei != NULL                     &&
        ei->emlink            != NULL  &&
        ei->emlink->ShowAlarm != NULL)
        ei->emlink->ShowAlarm(ei, onoff);

    SetRelaxing(ki, !onoff);
}

/*!!!INTERNAL ) */

enum
{
    REALIZE_PASS_1 = 0,
    REALIZE_PASS_2 = 1,
};

static void FillConnsOfElemInfo(cda_serverid_t defsid, int pass_n,
                                uint8 *upper_conns_u,
                                ElemInfo info);

static void FillConnsOfKnobs   (cda_serverid_t defsid, int pass_n,
                                uint8 *upper_conns_u, int numsids,
                                Knob         infos, int count)
{
  knobinfo_t *ki;
  int         n;
  int         r;

    if (infos == NULL  ||  count <= 0) return;

    for (ki = infos, n = 0;  n < count;  n++, ki++)
    {
        /* Isn't it an alive knob? */
        if (ki->type == 0) continue;

        /* A subelement? */
        if (ki->type == LOGT_SUBELEM)
        {
            FillConnsOfElemInfo(defsid, pass_n, upper_conns_u, ki->subelem);
        }
        else if (pass_n == REALIZE_PASS_2)
        {
        }
        else if (ki->kind == LOGK_DIRECT)
        {
            r = cda_cnsof_physchan(defsid, upper_conns_u, numsids, ki->physhandle);
            if (ki->colformula != NULL)
                cda_cnsof_formula (defsid, upper_conns_u, numsids, ki->colformula);
            if (r < 0) reporterror("cnsof(%s)=%d/%s\n", ki->ident, r, strerror(errno));
        }
        else if (ki->kind == LOGK_CALCED)
        {
            r = cda_cnsof_formula (defsid, upper_conns_u, numsids, ki->formula);
            if (ki->colformula != NULL)
                cda_cnsof_formula (defsid, upper_conns_u, numsids, ki->colformula);
            if (r < 0) reporterror("cnsof(%s)=%d/%s\n", ki->ident, r, strerror(errno));
        }
        else /* LOGK_NOP or LOGK_USRTXT... */
        {
            /* Nothing to do here */
        }
    }
}

static void FillConnsOfElemInfo(cda_serverid_t defsid, int pass_n,
                                uint8 *upper_conns_u,
                                ElemInfo info)
{
  int          numsids      = cda_status_lastn(defsid) + 1;
  size_t       conns_u_size = numsids * sizeof(info->conns_u[0]);
  int          x;
    
    /* Allocate array at the start of pass 1 */
    if (pass_n == REALIZE_PASS_1)
    {
        info->conns_u = malloc(conns_u_size);
        if (info->conns_u == NULL) return;
        bzero(info->conns_u, conns_u_size);
    }

    FillConnsOfKnobs(defsid, pass_n,
                     info->conns_u, numsids,
                     info->content, info->count);

    /* Add our used-sids upwards */
    if (upper_conns_u != NULL)
        for (x = 0;  x < numsids;  x++) upper_conns_u[x] |= info->conns_u[x];
    
    /* Perform checks at the end of pass 2 */
    if (pass_n == REALIZE_PASS_2)
    {
        for (x = 0;  x > 0  &&  x < numsids;  x++)
            if (info->conns_u[x]) x = -999; /* Note: NOT -1, since (-1)+1 would result in 0 */

        if (x > 0 /* x>0 means that no sids-used-by this-hierarchy found */
            ||
            (
             info->uplink != NULL           &&
             info->uplink->conns_u != NULL  &&
             memcmp(info->conns_u, info->uplink->conns_u, conns_u_size) == 0
            )
           )
        {
            safe_free(info->conns_u);
            info->conns_u = NULL;
        }
        else if (0)
        {
            fprintf(stderr, "CP: %s/\"%s\"={", info->ident, info->label);
            for (x = 0;  x < numsids;  x++)
                fprintf(stderr, "%s%d",
                        x > 0? ", " : "", info->conns_u[x]);
            fprintf(stderr, "}\n");
        }
    }
}

static void CdrRealizeGrouplist(cda_serverid_t defsid,
                                groupelem_t *list)
{
  int          numsids = cda_status_lastn(defsid) + 1;
  groupelem_t *ge;

    if (numsids <= 1) return;
    
    for (ge = list;  ge->ei != NULL;  ge++)
    {
        FillConnsOfElemInfo(defsid, REALIZE_PASS_1, NULL, ge->ei);
        FillConnsOfElemInfo(defsid, REALIZE_PASS_2, NULL, ge->ei);
    }
}

groupelem_t *CdrCvtGroupunits2Grouplist(cda_serverid_t defsid,
                                        groupunit_t *grouping)
{
  groupelem_t *ret;
  groupunit_t *gp;
  groupelem_t *ge;
  int          count;
  int          n;
    
#if OPTION_HAS_PROGRAM_INVOCATION_NAME /* With GNU libc+ld we can determine the true argv[0] */
    if (progname[0] == '\0') strzcpy(progname, program_invocation_short_name, sizeof(progname));
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */

    CLEAR_ERR();

    if (grouping == NULL)
    {
        errno = EINVAL;
        check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                       "%s(): grouping==NULL", __FUNCTION__ );
        return NULL;
    }
    
    /* Count the number of items first */
    for (gp = grouping, count = 0;  gp->e != NULL;  gp++, count++);
    count++;
    
    /* Allocate space */
    if ((ret = malloc(sizeof(*ret) * count)) == NULL) return NULL;
    bzero(ret, sizeof(*ret) * count);

    /* Walk through the list... */
    for (n = 0, gp = grouping, ge = ret;  n < count - 1;  n++, gp++, ge++)
    {
        if ((ge->ei = CdrCvtElemnet2Eleminfo(defsid, gp->e, ret)) == NULL)
        {
            CdrDestroyGrouplist(ret);
            return NULL;
        }
        ge->fromnewline = gp->fromnewline;
    }

    /* Determine "change points" */
    CdrRealizeGrouplist(defsid, ret);
    
    return ret;
}

ElemInfo     CdrCvtElemnet2Eleminfo    (cda_serverid_t defsid,
                                        elemnet_t *src,
                                        void *grouplist_link)
{
  eleminfo_t *ret;
  
#define DEFSERVER_EQ_S       "defserver="
#define DEFSERVER_SEPARATORS " \t,"
  const char *options;
  const char *ds_b;
  size_t      ds_l;
  char        ds_buf[100];

    CLEAR_ERR();
  
    if (src == NULL)
    {
        errno = EINVAL;
        check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                       "%s(): src==NULL", __FUNCTION__ );
        return NULL;
    }

    if ((ret = malloc(sizeof(*ret))) == NULL) return NULL;
    bzero(ret, sizeof(*ret));

    ret->type  = src->type;
    ret->count = src->count;
    ret->ncols = src->ncols;

    options = src->options;
    if (options != NULL  &&
        cx_memcasecmp(options, DEFSERVER_EQ_S, strlen(DEFSERVER_EQ_S)) == 0)
    {
        options += strlen(DEFSERVER_EQ_S);
        ds_b = options;
        while (*options != '\0'  &&
               strchr(DEFSERVER_SEPARATORS, *options) == NULL)
            options++;
        ds_l = options - ds_b;
        
        while (*options != '\0'  &&
               strchr(DEFSERVER_SEPARATORS, *options) != NULL)
            options++;

        if (ds_l > sizeof(ds_buf) - 1)
        {
            check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                           "%s(): defserver too long in %s/\"%s\"",
                           __FUNCTION__, src->ident, src->label);
            return NULL;
        }

        if (ds_l > 0)
        {
            memcpy(ds_buf, ds_b, ds_l); ds_buf[ds_l] = '\0';
            defsid = cda_add_auxsid(defsid, ds_buf, NULL, NULL);
            if (defsid == CDA_SERVERID_ERROR)
            {
                check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                               "%s/\"%s\".defserver=%s: %s",
                               src->ident, src->label, ds_buf, cx_strerror(errno));
                return NULL;
            }
        }
    }
    
    ret->grouplist_link = grouplist_link;
    if (
        (astrcpy(&(ret->ident),     src->ident))     < 0  ||
        (astrcpy(&(ret->label),     src->label))     < 0  ||
        (astrcpy(&(ret->colnames),  src->colnames))  < 0  ||
        (astrcpy(&(ret->rownames),  src->rownames))  < 0  ||
        (astrcpy(&(ret->options),        options))   < 0  ||
        (astrcpy(&(ret->style),     src->style))     < 0  ||
        (astrcpy(&(ret->defserver), src->defserver)) < 0  ||
        (
         src->count != 0  &&
         (ret->content  = CdrCvtLogchannets2Knobs(defsid, src->channels, src->count, ret)) == NULL
        )
       )
    {
        CdrDestroyEleminfo(ret);
        return NULL;
    }

    return ret;
}

Knob         CdrCvtLogchannets2Knobs   (cda_serverid_t defsid,
                                        logchannet_t *src, int count,
                                        ElemInfo  holder)
{
  knobinfo_t *ret;
  knobinfo_t *ki;
  int         n;

  int         check_dups;
  const char *envv;
  int         j;
  
  double minv, maxv;

  int   flags;
  int   width;
  int   precision;
  char  conv_c;
  
    CLEAR_ERR();
  
    if (src == NULL)
    {
        errno = EINVAL;
        check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                       "%s(): src==NULL", __FUNCTION__ );
        return NULL;
    }
    
    if (count <= 0) {errno = EINVAL; return NULL;}
  
    if ((ret = malloc(sizeof(*ret) * count)) == NULL) return NULL;
    bzero(ret, sizeof(*ret) * count);

    check_dups = (envv = getenv("CDR_DEBUG_DUPS")) != NULL  &&  *envv == '1';
    check_dups = 1;

    for (n = 0, ki = ret;  n < count;  n++, ki++, src++)
    {
        ki->uplink     = holder;
        
        /* General properties */
        ki->type       = src->type;
        ki->is_rw      = (ki->type == LOGT_WRITE1);
        ki->look       = src->look;
        ki->kind       = src->kind;
        ki->color      = src->color;

        /* Behaviour */
        /* (This wizardry is really done by Knobs' widgets, but
           we forestall/duplicate it here for widgetless clients
           (such as cx-starter) */
        switch (ki->look)
        {
            case LOGD_ALARM:
                ki->behaviour |= KNOB_B_IS_ALARM;
                /*fallthrough to lights-common*/
                
            case LOGD_ONOFF:
            case LOGD_GREENLED:
            case LOGD_AMBERLED:
            case LOGD_REDLED:
                ki->behaviour |= KNOB_B_IS_LIGHT;
                break;

            case LOGD_BUTTON:
            case LOGD_ARROW_LT:
            case LOGD_ARROW_RT:
            case LOGD_ARROW_UP:
            case LOGD_ARROW_DN:
                ki->behaviour |= KNOB_B_IS_LIGHT | KNOB_B_IS_BUTTON;
        }
        
        /* Text properties */
        if (
            (astrcpy(&(ki->ident),      src->ident))      < 0  ||
            (astrcpy(&(ki->label),      src->label))      < 0  ||
            (astrcpy(&(ki->units),      src->units))      < 0  ||
            (astrcpy(&(ki->widdepinfo), src->widdepinfo)) < 0  ||
            (astrcpy(&(ki->comment),    src->comment))    < 0  ||
            (astrcpy(&(ki->placement),  src->placement))  < 0  ||
            (astrcpy(&(ki->style),      src->style))      < 0
           )
            goto CLEANUP_ON_ERROR;

        /* Here may check for duplicates */
        if (check_dups                       &&
            IdentifierIsSensible(ki->ident)  &&
            strcmp(ki->ident, ":") != 0)
            for (j = 0;  j < n;  j++)
                if (IdentifierIsSensible(ret[j].ident)  &&
                    strcasecmp(ki->ident, ret[j].ident) == 0)
                {
                    reporterror("%s(holder:%s/\"%s\"): [%d]/\"%s\" identifier \"%s\" collides with [%d]/\"%s\"\n",
                                __FUNCTION__, holder->ident, holder->label,
                                n, ki->label, ki->ident, j, ret[j].label);
                    break;
                }
        
        /* Clever "duplication" of dpyfmt */
        if (ParseDoubleFormat(GetTextFormat(src->dpyfmt),
                              &flags, &width, &precision, &conv_c) < 0)
        {
            reporterror("%s: %s/\"%s\": invalid dpyfmt specification: %s\n",
                        __FUNCTION__, ki->ident, ki->label, printffmt_lasterr());
            ParseDoubleFormat("%1.1f", &flags, &width, &precision, &conv_c);
        }
        CreateDoubleFormat(ki->dpyfmt, sizeof(ki->dpyfmt), 20, 10,
                           flags, width, precision, conv_c);
        
        /* Physical reference */
        if      (src->type == LOGT_SUBELEM)
        {/* Just a guard to escape touching phys/formula/revformula */}
        else if (src->kind == LOGK_DIRECT)
        {
#if PHYS_REC
            ki->physhandle = CDA_PHYSCHANHANDLE_ERROR;
            if      (src->pr.kind != PHYS_KIND_BY_ID)
                reporterror("WARNING: %s.pr.kind=%d, !=PHYS_KIND_BY_ID\n", ki->ident, src->pr.kind);
            else if (src->pr.d.phys_id < 0)
                reporterror("WARNING: %s.pr.d.phys_id=%d\n", ki->ident, src->pr.d.phys_id);
            else
                ki->physhandle = cda_add_physchan(defsid, src->pr.d.phys_id);
#else
            if (src->phys < 0)
                reporterror("WARNING: %s.phys=%d\n", ki->ident, src->phys);
            ki->physhandle = cda_add_physchan(defsid, src->phys);
#endif
            cda_getphyschan_q(ki->physhandle, &(ki->q));
            if (src->formula    != NULL  &&
                (ki->formula    = cda_register_formula(defsid, src->formula,    0)) == NULL)
            {
                check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                               "%s.formula: %s",
                               ki->ident, cda_lasterr());
                goto CLEANUP_ON_ERROR;
            }

            ki->colformula = cda_HACK_findnext_f(ki->formula);
        }
        else if (src->kind == LOGK_CALCED)
        {
            if      (src->formula    == NULL)
            {
                reporterror("WARNING: %s.formula==NULL!\n", ki->ident);
            }
            else if ((ki->formula    = cda_register_formula(defsid, src->formula,    0)) == NULL)
            {
                check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                               "%s.formula: %s",
                               ki->ident, cda_lasterr());
                goto CLEANUP_ON_ERROR;
            }
            if      (src->revformula == NULL)
            {
                if (ki->is_rw) reporterror("WARNING: %s.revformula==NULL!\n", ki->ident);
            }
            else if ((ki->revformula = cda_register_formula(defsid, src->revformula, CDA_OPT_IS_WRITE)) == NULL)
            {
                check_snprintf(_Cdr_lasterr_str, sizeof(_Cdr_lasterr_str),
                               "%s.revformula: %s",
                               ki->ident, cda_lasterr());
                goto CLEANUP_ON_ERROR;
            }

            ki->colformula = cda_HACK_findnext_f(ki->formula);
        }
        /* "else" -- it can be LOGK_NOP or LOGK_USRTXT  */

        /* Informational/display fields */
        ki->rmins[KNOBS_RANGE_NORM]    = src->minnorm;
        ki->rmaxs[KNOBS_RANGE_NORM]    = src->maxnorm;
        ki->defnorm                    = src->defnorm;
        ki->rmins[KNOBS_RANGE_YELW]    = src->minyelw;
        ki->rmaxs[KNOBS_RANGE_YELW]    = src->maxyelw;
        ki->colstate                   = COLALARM_NONE;
        
        ki->rmins[KNOBS_RANGE_DISP]    = src->mindisp;
        ki->rmaxs[KNOBS_RANGE_DISP]    = src->maxdisp;
        if (ki->rmins[KNOBS_RANGE_DISP] >=  ki->rmaxs[KNOBS_RANGE_DISP])
        {
            minv = maxv = 0.0;

            /* May we derive "display range" from broadest of normal/yellow? */
            if (ki->colformula == NULL)
            {
                if (ki->rmins[KNOBS_RANGE_NORM] < ki->rmaxs[KNOBS_RANGE_NORM])
                {
                    minv = ki->rmins[KNOBS_RANGE_NORM];
                    maxv = ki->rmaxs[KNOBS_RANGE_NORM];
                }
                if (ki->rmins[KNOBS_RANGE_YELW] < ki->rmaxs[KNOBS_RANGE_YELW])
                {
                    minv = ki->rmins[KNOBS_RANGE_YELW];
                    maxv = ki->rmaxs[KNOBS_RANGE_YELW];
                }
            }

            /* Still nothing sensible?  Okay, let's use defaults */
            if (minv >= maxv)
            {
                minv = -100.0;
                maxv = +100.0;
            }
            
            ki->rmins[KNOBS_RANGE_DISP] = minv;
            ki->rmaxs[KNOBS_RANGE_DISP] = maxv;
        }

        /* Numeric behaviour */
        ki->incdec_step = src->incdec_step;
        if (ki->incdec_step == 0.0) ki->incdec_step = 1.0;
        
        /*!!! Should do type-dependent operations here (like for LOGT_MINMAX) */
        
        if (ki->type == LOGT_MINMAX)
        {
            ki->minmaxbufcap = 30;
            ki->minmaxbuf    = malloc(ki->minmaxbufcap * sizeof(ki->minmaxbuf[0]));
            if (ki->minmaxbuf == NULL) goto CLEANUP_ON_ERROR;
        }

        if (ki->type == LOGT_SUBELEM)
        {
            ki->subelem = CdrCvtElemnet2Eleminfo(defsid,
                                                 (elemnet_t *)(src->formula),
                                                 holder->grouplist_link);
            if (ki->subelem == NULL) goto CLEANUP_ON_ERROR;
            ki->subelem->uplink = ki->uplink;  /* In fact, holder */
        }
        
        /* Set flags and color state for value=0 */
        ki->currflags = datatree_choose_knob_rflags(ki, ki->curtag, ki->currflags, ki->curv); /*!!!What about colformula? :-)*/
        ki->colstate  = COLALARM_JUSTCREATED;
        
        /*!!! And create appropriate widgets, if "creator" is passed */
    }

    return ret;
  
 CLEANUP_ON_ERROR:
    CdrDestroyKnobs(ret, count);
    return NULL;
}

void         CdrDestroyGrouplist(groupelem_t *list)
{
  groupelem_t *ge;

    for (ge = list;  ge->ei != NULL;  ge++)
        CdrDestroyEleminfo(ge->ei);
  
    free(list);
}

void         CdrDestroyEleminfo (ElemInfo     info)
{
    CdrDestroyKnobs(info->content, info->count);
    safe_free(info->ident);
    safe_free(info->label);
    safe_free(info->colnames);
    safe_free(info->rownames);
    safe_free(info->options);
    safe_free(info->style);
    safe_free(info->defserver);
    safe_free(info->conns_u);
}

void         CdrDestroyKnobs    (Knob         infos, int count)
{
  knobinfo_t *ki;
  int         n;
    
    if (infos == NULL  ||  count <= 0) return;

    for (ki = infos, n = 0;  n < count;  n++, ki++)
    {
        if (ki->type == LOGT_SUBELEM)
            CdrDestroyEleminfo(ki->subelem);
        
        safe_free(ki->ident);
        safe_free(ki->label);
        safe_free(ki->units);
        safe_free(ki->widdepinfo);
        safe_free(ki->comment);
        safe_free(ki->placement);
        safe_free(ki->style);
        
        safe_free(ki->formula);
        safe_free(ki->revformula);
        /* Must NOT do "safe_free(ki->revformula);", since now it is a part of "formula" */

        safe_free(ki->minmaxbuf);
        safe_free(ki->histring);
    }

    free(infos);
}


void         CdrProcessGrouplist(int cause_conn_n, int options, rflags_t *rflags_p,
                                 cda_localreginfo_t *localreginfo,
                                 groupelem_t *list)
{
  groupelem_t *ge;
  rflags_t     rflags;
  rflags_t     cml_rflags = 0;

    for (ge = list;  ge->ei != NULL;  ge++)
    {
        CdrProcessEleminfo(cause_conn_n, options, &rflags, localreginfo, ge->ei);
        cml_rflags |= rflags;
    }
    
    if (rflags_p != NULL)
        *rflags_p = cml_rflags;
}

void         CdrProcessEleminfo (int cause_conn_n, int options, rflags_t *rflags_p,
                                 cda_localreginfo_t *localreginfo,
                                 ElemInfo info)
{
  rflags_t     cml_rflags;

    if (1 /* 1: use feature, 0: don't */  &&
        cause_conn_n >= 0                 &&
        info->conns_u != NULL             &&
        info->conns_u[cause_conn_n] == 0)
    {
        cml_rflags = info->currflags;
        //fprintf(stderr, "skipping %s: [%d]=0\n", info->ident, cause_conn_n);
    }
    else
    {
        CdrProcessKnobs(cause_conn_n, options, &cml_rflags, localreginfo, info->content, info->count);
        info->currflags = cml_rflags;
    
        if (info->emlink          != NULL  &&
            info->emlink->NewData != NULL)
            info->emlink->NewData(info, (options & CDR_OPT_SYNTHETIC) != 0);
    }
    
    if (rflags_p != NULL)
        *rflags_p = cml_rflags;
}

void         CdrProcessKnobs    (int cause_conn_n, int options, rflags_t *rflags_p,
                                 cda_localreginfo_t *localreginfo,
                                 Knob         infos, int count)
{
  knobinfo_t *ki;
  int         n;

  int         cda_opts = (options & CDR_OPT_READONLY)? CDA_OPT_READONLY : 0;

  double      v;
  tag_t       tag;
  rflags_t    rflags;
  int32       rv;
  int         rv_u;
  time_t      timenow   = time(NULL);
  rflags_t    cml_rflags = 0;  // Cumulative rflags

  double      cf_v;
  tag_t       cf_tag;
  rflags_t    cf_rflags;

  /* For LOGT_DEVN */
  double      newA;
  double      newA2;
  double      divider;

  /* For LOGT_MINMAX */
  double      min, max;
  double     *dp;
  int         i;
  
    if (infos == NULL  ||  count <= 0) goto DO_RETURN;

    for (ki = infos, n = 0;  n < count;  n++, ki++)
    {
        /* Isn't it an alive knob? */
        if (ki->type == 0) continue;

        /* A subelement? */
        if (ki->type == LOGT_SUBELEM)
        {
            CdrProcessEleminfo(cause_conn_n, options, &rflags, localreginfo, ki->subelem);
            cml_rflags |= rflags;
            continue;
        }
        
        /* Obtain current value */
        if      (ki->kind == LOGK_DIRECT)
        {
            cda_getphyschanvnr(ki->physhandle, &v, &rv,  &tag, &rflags);
            rv_u = 1;
        }
        else if (ki->kind == LOGK_CALCED  &&  ki->formula != NULL)
        {
            cda_execformula   (ki->formula, cda_opts, NAN, &v, &tag, &rflags, &rv, &rv_u, localreginfo);
        }
        else /* LOGK_NOP or LOGK_USRTXT... */
        {
            v      = NAN;
            rv     = 0;
            tag    = 0;
            rflags = 0;
        }

        rflags &= (CXCF_FLAG_CDA_MASK | CXRF_SERVER_MASK);

        /* Perform type-dependent operations with a value */
        switch (ki->type)
        {
            case LOGT_DEVN:
                /*!!! Should check for synthetic update! */

                /* Initialize average value with current */
                if (ki->notnew == 0)
                {
                    ki->avg  = v;
                    ki->avg2 = v*v;

                    ki->notnew = 1;
                }

                /* Recursively calculate next avg and avg2 */
                newA  = (ki->avg * NUMAVG + v)
                    /
                    (NUMAVG + 1);
                
                newA2 = (ki->avg2 * NUMAVG + v*v)
                    /
                    (NUMAVG + 1);
                
                /* Store them */
                ki->avg  = newA;
                ki->avg2 = newA2; 
                
                /* And substitute the value of 'v' with the deviation */
                divider = fabs(newA2);
                if (divider < 0.00001) divider = 0.00001;
                
                v =  (
                      sqrt(fabs(newA2 - newA*newA))
                      /
                      divider
                     ) * 100;
                
                break;

            case LOGT_MINMAX:
                /* Store current value */
                if ((options & CDR_OPT_SYNTHETIC) == 0) /* On real, non-synthetic updates only */
                {
                    if (ki->minmaxbufused == ki->minmaxbufcap)
                    {
                        ki->minmaxbufused--;
                        memmove(&(ki->minmaxbuf[0]),  &(ki->minmaxbuf[1]),
                                ki->minmaxbufused * sizeof(ki->minmaxbuf[0]));
                    }
                    ki->minmaxbuf[ki->minmaxbufused++] = v;
                }
                else
                {
                    /* Here we handle a special case -- when synthetic update occurs before
                     any real data arrival, so we must use one minmax cell, otherwise we
                     just replace the last value */
                    if (ki->minmaxbufused == 0)
                        ki->minmaxbuf[ki->minmaxbufused++] = v;
                    else
                        ki->minmaxbuf[ki->minmaxbufused-1] = v;
                }
                
                /* Find min & max */
                for (i = 0,  dp = ki->minmaxbuf,  min = max = *dp;
                     i < ki->minmaxbufused;
                     i++, dp++)
                {
                    if (*dp < min) min = *dp;
                    if (*dp > max) max = *dp;
                }
                
                v = fabs(max - min);

                break;

            default:;
        }

        /* A separate-colorization... */
        if (ki->colformula != NULL)
        {
            cda_execformula(ki->colformula, cda_opts, NAN, &cf_v, &cf_tag, &cf_rflags, NULL, NULL, localreginfo);
            if (cf_tag > tag) tag = cf_tag;
            rflags |= cf_rflags;

            ki->cur_cfv = cf_v;
        }
        
        /* Make decisions about behaviour/colorization/etc. */
        if (ki->attn_endtime != 0  &&
            difftime(timenow, ki->attn_endtime) >= 0)
            ki->attn_endtime = 0;
        rflags = datatree_choose_knob_rflags(ki, tag, rflags,
                                             ki->colformula==NULL?v : cf_v);

        if (rflags & CXCF_FLAG_OTHEROP) SetOtherop(ki, 1);
        
        if ((rflags ^ ki->currflags) & CXCF_FLAG_ALARM_ALARM)
            SetAlarm(ki, (rflags & CXCF_FLAG_ALARM_ALARM) != 0);
        
        /* Store new values */
        ki->curv            = v;
        ki->curv_raw        = rv;
        ki->curv_raw_useful = rv_u;
        ki->curtag          = tag;
        ki->currflags       = rflags;

        cml_rflags |= rflags;

        /* Store in history buffer, if present */
        /* (History ignores synthetic updates) */
        if (ki->histring_size != 0  &&  (options & CDR_OPT_SYNTHETIC) == 0)
        {
            ki->histring_ctr++;
            if (ki->histring_ctr >= ki->histring_frqdiv) ki->histring_ctr = 0;

            if (ki->histring_ctr == 0)
            {
                if (ki->histring_used == ki->histring_size)
                    ki->histring_start = (ki->histring_start + 1) % ki->histring_size;
                else
                    ki->histring_used++;

                ki->histring[(ki->histring_start + ki->histring_used - 1) % ki->histring_size] = v;
            }
        }

        /* Display the result */
        if (
            !ki->is_rw  ||
            (
             difftime(timenow, ki->usertime) > USERINACTIVITYLIMIT  &&
             !ki->wasjustset
            )
           )
            datatree_SetControlValue(ki, v, 0);
        
        if ((options & CDR_OPT_SYNTHETIC) == 0) ki->wasjustset = 0;
        
        /* Colorize */
        datatree_set_knobstate(ki, datatree_choose_knobstate(ki, rflags));
    }

 DO_RETURN:
    if (rflags_p != NULL)
        *rflags_p = cml_rflags;
}

int          CdrActivateKnobHistory (Knob k, int histring_size)
{
    if (k->histring != NULL) return 0;

    k->histring       = malloc(histring_size * sizeof(double));
    if (k->histring == NULL) return -1;

    k->histring_size  = histring_size;
    k->histring_start = 0;
    k->histring_used  = 0;
    
    return 0;
}

void         CdrHistorizeKnobsInList(Knob k)
{
    for (;  k != NULL;  k = k->hist_next)
    {
        if (k->histring_used == k->histring_size)
            k->histring_start = (k->histring_start + 1) % k->histring_size;
        else
            k->histring_used++;
        
        k->histring[(k->histring_start + k->histring_used - 1) % k->histring_size] = k->curv;
    }
}

int          CdrSrcOf(Knob k, const char **name_p, int *n_p)
{
    switch (k->kind)
    {
        case LOGK_DIRECT: return cda_srcof_physchan(k->physhandle, name_p, n_p);
        case LOGK_CALCED: return cda_srcof_formula (k->formula,    name_p, n_p);
        default:          return -1;
    }
}


Knob         CdrFindKnob    (groupelem_t *list, const char *name)
{
  knobinfo_t *ki = datatree_FindNode(list, name);

    if (ki != NULL  &&  ki->type == LOGT_SUBELEM)
    {
        errno = EISDIR;
        return NULL;
    }

    return ki;
}

Knob         CdrFindKnobFrom(groupelem_t *list, const char *name,
                             ElemInfo     start_point)
{
  knobinfo_t *ki = datatree_FindNodeFrom(list, name, start_point);

    if (ki != NULL  &&  ki->type == LOGT_SUBELEM)
    {
        errno = EISDIR;
        return NULL;
    }

    return ki;
}


int          CdrSetKnobValue(Knob k, double v, int options,
                             cda_localreginfo_t *localreginfo)
{
  int         cda_opts = (options & CDR_OPT_READONLY)? CDA_OPT_READONLY : 0;

    if (!k->is_rw)
    {
        errno = EINVAL;
        return -1;
    }

    datatree_set_attn     (k, 0, 0, 0);
    datatree_set_knobstate(k, datatree_choose_knobstate(k, k->currflags));

    if      (k->kind == LOGK_DIRECT)
    {
        if (cda_opts & CDA_OPT_READONLY)
            return 0;
        else
            return cda_setphyschanval(k->physhandle, v);
    }
    else if (k->kind == LOGK_CALCED)
        return cda_execformula(k->revformula, cda_opts | CDA_OPT_IS_WRITE, v,
                               NULL, NULL, NULL, NULL, NULL,
                               localreginfo);

    /* Here in case of LOGT_SUBELEM/kind=0 or LOGK_NOP or LOGK_USRTXT */
    errno = EINVAL;
    return -1;
}

static const char *mode_lp_s    = "# Mode";
static const char *subsys_lp_s  = "#!SUBSYSTEM:";
static const char *crtime_lp_s  = "#!CREATE-TIME:";
static const char *comment_lp_s = "#!COMMENT:";

static const char *element_s    = "element";
static const char *channel_s    = "channel";
static const char *usrtxt_s     = "usrtxt";
static const char *range_s[]    = {"norm_range", "yelw_range", "disp_range"};
static const char *grpcoeff_s   = "grpcoeff";
static const char *grouped_s    = "grouped";
static const char *frqdiv_s     = "frqdiv";


static inline void fprintf_nc(FILE *fp, char c, size_t n)
{
    for (;  n > 0;  n--) fprintf(fp, "%c", c);
}

static inline void fprintf_f_rtrim(FILE *fp, const char *dpyfmt, double v)
{
  char  buf[400]; // So that 1e308 fits

    fprintf(fp, "%s", snprintf_dbl_trim(buf, sizeof(buf), dpyfmt, v, 1));
}

static void RecSaveElement(FILE *fp, eleminfo_t *ei, int level)
{
  knobinfo_t  *ki;
  int          y;
  int          r;
  int          is_opaque = level == 0           ||
                           ei->ident == NULL    ||
                           strcmp(ei->ident, ":") != 0;

    if (!is_opaque) level--;

    /* Element title... */
    if (is_opaque)
    {
        fprintf_nc(fp, '\t', level);
        fprintf(fp, ".%s %s {\n", element_s, ei->ident);
    }

    /* ...its content... */
    for (y = 0; y < ei->count; y++)
    {
        ki = ei->content + y;
        if (!(IdentifierIsSensible(ki->ident))) continue;
        if      (ki->type == LOGT_SUBELEM)
        {
            RecSaveElement(fp, ki->subelem, level + 1);
        }
        else if (ki->kind == LOGK_NOP)
        {
            /* Do-nothing */
        }
        else if (ki->kind == LOGK_USRTXT)
        {
          char *ltp;
            
            fprintf_nc(fp, '\t', level + 1);
            fprintf(fp, ".%s %s\t", usrtxt_s, ki->ident);
            for (ltp = ki->label; ltp != NULL  &&  *ltp != '\0';  ltp++)
                fprintf(fp, "%c", !iscntrl(*ltp)? *ltp : ' ');
            fprintf(fp, "\n");
        }
        else
        {
            fprintf_nc(fp, '\t', level + 1);
            fprintf(fp, ".%s %s\t", channel_s, ki->ident);
            fprintf_f_rtrim(fp, ki->dpyfmt, ki->curv);
            fprintf(fp, "\t");
            fprintf_f_rtrim(fp, ki->dpyfmt, ki->incdec_step);
            for (r = 0;  r < 3;  r++)
                if (ki->rchg[r])
                {
                    fprintf(fp, " %s:", range_s[r]);
                    fprintf_f_rtrim(fp, ki->dpyfmt, ki->rmins[r]);
                    fprintf(fp, ",");
                    fprintf_f_rtrim(fp, ki->dpyfmt, ki->rmaxs[r]);
                }
            if (ki->behaviour & KNOB_B_IS_GROUPABLE)
            {
                if (ki->grpcoeffchg)
                {
                    fprintf(fp, " %s:", grpcoeff_s);
                    fprintf_f_rtrim(fp, ki->dpyfmt, ki->grpcoeff);
                }

                fprintf(fp, " %s:%d", grouped_s, ki->is_ingroup);
            }
            if (ki->histring_frqdivchg)
            {
                fprintf(fp, " %s:%d", frqdiv_s, ki->histring_frqdiv);
            }
            fprintf(fp, "\n");
        }
    }

    /* ...and its terminator */
    if (is_opaque)
    {
        fprintf_nc(fp, '\t', level);
        fprintf(fp, "}\n");
    }
    if (level == 0) fprintf(fp, "\n");
}

int          CdrSaveGrouplistMode(groupelem_t *list, const char *filespec,
                                  const char *subsys, const char *comment)
{
  FILE        *fp;
  time_t       timenow = time(NULL);
  const char  *cp;
  
  groupelem_t *ge;
  
    if ((fp = fopen(filespec, "w")) == NULL) return -1;

    fprintf(fp, "##########\n%s %s\n", mode_lp_s, ctime(&timenow));
    fprintf(fp, "%s %s\n",  subsys_lp_s, subsys);
    fprintf(fp, "%s %lu %s", crtime_lp_s, (unsigned long)(timenow), ctime(&timenow)); /*!: No '\n' because ctime includes a trailing one. */
    fprintf(fp, "%s ", comment_lp_s);
    if (comment != NULL)
        for (cp = comment;  *cp != '\0';  cp++)
            fprintf(fp, "%c", !iscntrl(*cp)? *cp : ' ');
    fprintf(fp, "\n\n");
    
    for (ge = list;  ge->ei != NULL;  ge++)
        if (IdentifierIsSensible(ge->ei->ident))
            RecSaveElement(fp, ge->ei, 0);

    fclose(fp);
    
    return 0;
}

static char *SkipWhite(const char *p)
{
    while (*p != '\0'  &&  isspace(*p)) p++;
    
    return p;
}

static char *SkipIdentifier(const char *p)
{
    while (*p != '\0'  &&
           (isalnum(*p)  ||  *p == '-'  ||  *p == '_'  ||  *p == '.')) p++;
    
    return p;
}

static int FindRangeByName(const char *s, size_t len)
{
  int  r;
    
    for (r = 0;  r < 3;  r++)
        if (cx_strmemcasecmp(range_s[r], s, len) == 0) return r;

    return -1;
}

static void ParseChanVals(const char *filespec, int *lineno_p,
                          const char *p, knobinfo_t *ki, int options)
{
  knobinfo_t    old;
  int           changed;

  double      the_v;
  double      the_step;
  int         is_grouped = -1;

  const char *t;
  char       *err;

  double      v1, v2;
  int         r;
  int         intv;

    old = *ki;
    changed = 0;
  
    /* Parse the value */
    the_v = strtod (p, &err);
    if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
    {
        reporterror("\"%s\" line %d: error parsing the value, '%s'->'%s'\n",
                    filespec, *lineno_p, p, err);
        return;
    }
    
    /* Parse "step" (if any) */
    p = SkipWhite(err);
    the_step = strtod(p, &err);
    if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
    {
        the_step = 0.0;
        goto SET_VALUE; /* For "n:v" pairs parsing to be skipped */
    }

    if ((options & CDR_OPT_NOLOAD_VALUES) == 0)
        changed = 1;
    
    /* Parse other "name:val" pairs */
    while (1)
    {
        p = SkipWhite(err);
        t = SkipIdentifier(p);
        if (t == p) goto SET_VALUE;
        if (*t != ':')
        {
            reporterror("\"%s\" line %d: unexpected character instead of ':'\n",
                        filespec, *lineno_p);
            goto SET_VALUE;
        }

        if      ((r = FindRangeByName(p, t - p)) >= 0)
        {
            p = t + 1;
            v1 = strtod(p, &err);
            if (err == p  ||  *err != ',')
                goto SET_VALUE;
            p = err + 1;
            v2 = strtod(p, &err);
            if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
                goto SET_VALUE;

            if ((options & CDR_OPT_NOLOAD_RANGES) == 0)
            {
                ki->rmins[r] = v1;
                ki->rmaxs[r] = v2;
                ki->rchg [r] = 1;

                changed = 1;
            }
        }
        else if (cx_strmemcasecmp(grpcoeff_s, p, t - p) == 0)
        {
            p = t + 1;
            v1 = strtod(p, &err);
            if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
                goto SET_VALUE;

            if ((options & CDR_OPT_NOLOAD_VALUES) == 0  &&
                ki->behaviour & KNOB_B_IS_GROUPABLE)
            {
                ki->grpcoeff    = v1;
                ki->grpcoeffchg = 1;
            }
        }
        else if (cx_strmemcasecmp(grouped_s, p, t - p) == 0)
        {
            p = t + 1;
            intv = strtol(p, &err, 10);
            if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
                goto SET_VALUE;

            if ((options & CDR_OPT_NOLOAD_VALUES) == 0  &&
                ki->behaviour & KNOB_B_IS_GROUPABLE)
            {
                is_grouped = intv;

                changed = 1;
            }
        }
        else if (cx_strmemcasecmp(frqdiv_s, p, t - p) == 0)
        {
            p = t + 1;
            intv = strtol(p, &err, 10);
            if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
                goto SET_VALUE;
            if ((options & CDR_OPT_NOLOAD_VALUES) == 0)
            {
                ki->histring_frqdiv    = intv;
                ki->histring_frqdivchg = 1;
            }
        }
        else
        {
            reporterror("\"%s\" line %d: unknown property \"%s\"\n",
                        filespec, *lineno_p, p);
            goto SET_VALUE;
        }
    }

 SET_VALUE:
    if (0)
        fprintf(stderr, "About to set '%s':=%8.3f (cur=%8.3f)\n",
                ki->ident, the_v, ki->curv);
    else
        if ((options & CDR_OPT_NOLOAD_VALUES) == 0)
            datatree_SetControlValue(ki, the_v, 1);

    if ((options & CDR_OPT_NOLOAD_VALUES) == 0  &&
        the_step != 0.0)
        ki->incdec_step = the_step;

    if ((options & CDR_OPT_NOLOAD_VALUES) == 0  &&
        is_grouped >= 0) ki->is_ingroup = is_grouped;
    
    if (changed  &&
        ki->vmtlink != NULL  &&
        ki->vmtlink->PropsChg != NULL)
        ki->vmtlink->PropsChg(ki, &old);
}

static void ParseUsrTxt  (const char *filespec, int *lineno_p,
                          const char *p, knobinfo_t *ki, int options)
{
  knobinfo_t  old = *ki;

    while (*p != '\0'  &&  isspace(*p)) p++;

    if ((options & CDR_OPT_NOLOAD_VALUES) != 0) return;

    if (ki->label == NULL  ||
        strcmp(p, ki->label) != 0)
    {
        safe_free(ki->label);
        ki->label = strdup(p);
        if (ki->vmtlink != NULL  &&
            ki->vmtlink->PropsChg != NULL)
            ki->vmtlink->PropsChg(ki, &old);
    }
}

/*
 *  Note:
 *      fp==NULL            signifies that it is called as a "global" mode loader
 *      fp!=NULL&&ei==NULL  means that it is called as a parser-only --
 *                          no real actions should be taken, just recursively
 *                          read the file (it is used for cases when a
 *                          subelement specified in a file wasn't found).
 */

static int RecLoadLevel(groupelem_t *list, const char *filespec,
                        FILE *fp, int *lineno_p,
                        eleminfo_t *ei, int options)
{
  int          i_am_root = (fp == NULL);
  /* File-related */
  int          root_lineno;
  char         line[1000];

  /* For walking through tree */
  const char  *knn;    // Knob's normalized name
  groupelem_t *ge;
  knobinfo_t  *ki;

  /* For line parsing */
  char         *p;
  char         *t;
  char          c;
  
  enum
  {
      ETYPE_CHAN = 0,
      ETYPE_ELEM = 1,
      ETYPE_TEXT = 2,
  };
  
  int           line_type;
  int           targ_type;

  static const char **names[3] = {&channel_s, &element_s, &usrtxt_s}; /*!!! Ugly, but bearable. An ideal solution would be a global table of line-types, indexed by a global enum, and being used by all parts instead of direct references to nnn_s */
  
#define BARK_AND_NEXT(format2, args2...)             \
    do {                                             \
        reporterror("\"%s\" line %d: " format2 "\n", \
        filespec, *lineno_p, ##args2);               \
        goto NEXT_LINE;                              \
    }                                                \
    while (0)
  
    if (i_am_root)
    {
        if ((fp = fopen(filespec, "r")) == NULL) return -1;
        root_lineno = 0;
        lineno_p = &root_lineno;
    }

    ////fprintf(stderr, "%s(%s)\n", __FUNCTION__, ei == NULL? "@ROOT" : ei->ident);
    
    while (fgets(line, sizeof(line) - 1, fp) != NULL)
    {
        (*lineno_p)++;

        p = line + strlen(line) - 1;
        while (p > line  &&
               (*p == '\n'  ||  isspace(*p)))
            *p-- = '\0';
        
        p = SkipWhite(line);
        if (*p == '\0') goto NEXT_LINE;
        if (*p == '}')
        {
            if (i_am_root) BARK_AND_NEXT("orphan '}'");
            else           return 0;
        }
        if (*p != '.')  goto NEXT_LINE;

        p++;
        t = SkipIdentifier(p);

        if      (cx_strmemcasecmp(element_s, p, t - p) == 0)
            line_type = ETYPE_ELEM;
        else if (cx_strmemcasecmp(channel_s, p, t - p) == 0)
            line_type = ETYPE_CHAN;
        else if (cx_strmemcasecmp(usrtxt_s,  p, t - p) == 0)
            line_type = ETYPE_TEXT;
        else
            BARK_AND_NEXT("unknown .-statement \"%.*s\"", (int)(t - p), p);

        p = SkipWhite(t);
        t = SkipIdentifier(p);
        
        if (i_am_root*0) /*!!! <- what is this? Obsolete, switched-off code? */
        {
            /* The following check may be modified to check for ".channame"
             to support parentless "global" channels specifications */
            if (line_type != ETYPE_ELEM)
                BARK_AND_NEXT(".channel outside elements is forbidden");
            
            for (ge = list;  ge->ei != NULL;  ge++)
            {
                knn = ge->ei->ident;
                if (knn == NULL) continue;
                if (*knn == DATATREE_DELIM) knn++;
                if (cx_strmemcasecmp(knn, p, t - p) == 0) break;
            }
            if (ge->ei == NULL)
                reporterror("\"%s\" line %d: unknown element \"%.*s\"; skipping it\n",
                            filespec, *lineno_p, (int)(t - p), p);
            RecLoadLevel(list, filespec, fp, lineno_p, ge->ei, options);
        }
        else
        {
            /* Parse-only? */
            if (ei == NULL  &&  !i_am_root)
            {
                if (line_type == ETYPE_ELEM) RecLoadLevel(list, filespec, fp, lineno_p, NULL, options);
                goto NEXT_LINE;
            }
            /* No, do the work */
            else
            {
                /* A "global" name? */
                if (*p == DATATREE_DELIM)
                {
                    c = *t;
                    *t = '\0';
                    ki = datatree_FindNode(list, p/*, t - p*/);
                    *t = c;
                    if (ki == NULL)
                    {
                        reporterror("\"%s\" line %d: global %s \"%.*s\" not found\n",
                                    filespec, *lineno_p, *names[line_type], (int)(t - p), p);
                        if (line_type == ETYPE_ELEM) RecLoadLevel(list, filespec, fp, lineno_p, NULL, options);
                        goto NEXT_LINE;
                    }
                    
                    if      (line_type == ETYPE_ELEM) RecLoadLevel (list, filespec, fp, lineno_p, ki->subelem, options);
                    else if (line_type == ETYPE_TEXT) ParseUsrTxt  (filespec, lineno_p, t, ki, options);
                    else                              ParseChanVals(filespec, lineno_p, t, ki, options);
                }
                /* No, a regular subname */
                else
                {
                    if (i_am_root)
                    {
                        /* The following check may be modified to check for ".channame"
                         to support parentless "global" channels specifications */
                        if (line_type != ETYPE_ELEM)
                            BARK_AND_NEXT(".channel outside elements is forbidden");

                        for (ge = list;  ge->ei != NULL;  ge++)
                        {
                            knn = ge->ei->ident;
                            if (knn == NULL) continue;
                            if (*knn == DATATREE_DELIM) knn++;
                            if (cx_strmemcasecmp(knn, p, t - p) == 0) break;
                        }
                        if (ge->ei == NULL)
                            reporterror("\"%s\" line %d: unknown element \"%.*s\"; skipping it\n",
                                        filespec, *lineno_p, (int)(t - p), p);
                        RecLoadLevel(list, filespec, fp, lineno_p, ge->ei, options);

                        goto NEXT_LINE;
                    }
                    
                    /* Find it in the list... */
                    ki = datatree_FindNodeInside(ei, p, t - p);
                    if (ki == NULL)
                    {
                        reporterror("\"%s\" line %d: %s \"%.*s\" not found inside \"%s\"\n",
                                    filespec, *lineno_p,
                                    *names[line_type], (int)(t - p), p, ei->ident);
                    }

                    /* Check if type of item matches the directive */
                    if (ki != NULL)
                    {
                        if      (ki->type == LOGT_SUBELEM) targ_type = ETYPE_ELEM;
                        else if (ki->kind == LOGK_USRTXT)  targ_type = ETYPE_TEXT;
                        else                               targ_type = ETYPE_CHAN;
                        if (line_type != targ_type)
                        {
                            reporterror("\"%s\" line %d: .%s specified while \"%s\" is %s\n",
                                        filespec, *lineno_p,
                                        *names[line_type], ki->ident, *names[targ_type]);
                            ki = NULL;
                        }
                    }
                    
                    if (ki == NULL)
                    {
                        if (line_type == ETYPE_ELEM) RecLoadLevel(list, filespec, fp, lineno_p, NULL, options);
                        goto NEXT_LINE;
                    }
                    else
                    {
                        if      (line_type == ETYPE_ELEM) RecLoadLevel (list, filespec, fp, lineno_p, ki->subelem, options);
                        else if (line_type == ETYPE_TEXT) ParseUsrTxt  (filespec, lineno_p, t, ki, options);
                        else                              ParseChanVals(filespec, lineno_p, t, ki, options);
                    }
                }
            }
        }
        
 NEXT_LINE:;
    }

    if (i_am_root) fclose(fp);

    return 0;
}

int          CdrLoadGrouplistMode(groupelem_t *list, const char *filespec,
                                  int options)
{
    return RecLoadLevel(list, filespec, NULL, NULL, NULL, options);
}


int          CdrStatGrouplistMode(const char *filespec,
                                  time_t *cr_time,
                                  char   *commentbuf, size_t commentbuf_size)
{
  FILE        *fp;
  char         line[1000];
  int          lineno = 0;
  char        *cp;
  char        *err;
  struct tm    brk_time;
  time_t       sng_time;

    if ((fp = fopen(filespec, "r")) == NULL) return -1;

    if (commentbuf_size > 0) *commentbuf = '\0';

    while (lineno < 10  &&  fgets(line, sizeof(line) - 1, fp) != NULL)
    {
        lineno++;
    
        if      (memcmp(line, mode_lp_s,    strlen(mode_lp_s)) == 0)
        {
            cp = line + strlen(mode_lp_s);
            while (isspace(*cp)) cp++;
            
            bzero(&brk_time, sizeof(brk_time));
            if (strptime(cp, "%a %b %d %H:%M:%S %Y", &brk_time) != NULL)
            {
                sng_time = mktime(&brk_time);
                if (sng_time != (time_t)-1)
                    *cr_time = sng_time;
            }
        }
        else if (memcmp(line, crtime_lp_s,  strlen(crtime_lp_s)) == 0)
        {
            cp = line + strlen(crtime_lp_s);
            while (isspace(*cp)) cp++;

            sng_time = (time_t)strtol(cp, &err, 0);
            if (err != cp  &&  (*err == '\0'  ||  isspace(*err)))
                *cr_time = sng_time;
        }
        else if (memcmp(line, comment_lp_s, strlen(comment_lp_s)) == 0)
        {
            cp = line + strlen(comment_lp_s);
            while (isspace(*cp)) cp++;

            strzcpy(commentbuf, cp, commentbuf_size);
        }
    }

    fclose(fp);

    return 0;
}


static void RecLogElement(FILE *fp, eleminfo_t *ei, int stage_is_data)
{
  knobinfo_t  *ki;
  int          y;
  const char  *units;
  
    for (y = 0, ki = ei->content;  y < ei->count;  y++, ki++)
    {
//        ki = ei->content + y; /*!!! What this line is for?! An artifact from the past? */
        if (!(IdentifierIsSensible(ki->ident))) continue;
        if (ki->type == LOGT_SUBELEM)
        {
            RecLogElement(fp, ki->subelem, stage_is_data);
        }
        else
        {
            if (!stage_is_data)
            {
                units = ki->units;
                if (units == NULL) units = "";
                fprintf(fp, "%s%s%s%s",
                        ki->ident,
                        units[0] != '\0'? "(":"",
                        units,
                        units[0] != '\0'? ")":""
                       );
            }
            else
            {
                fprintf_f_rtrim(fp, ki->dpyfmt, ki->curv);
            }

            fprintf(fp, " ");
        }
    }
}

int          CdrLogGrouplistMode (groupelem_t *list, const char *filespec,
                                  const char *comment,
                                  int headers, int append,
                                  struct timeval *start)
{
  FILE           *fp;
  struct timeval  timenow;
  const char     *cp;
  
  groupelem_t *ge;

  struct timeval  tv_x, tv_y, tv_r;
  
  int          stage;
  
  enum {STAGE_HEADERS = 0, STAGE_DATA = 1};
  
    if ((fp = fopen(filespec, (append? "a" : "w"))) == NULL) return -1;

    gettimeofday(&timenow, NULL);

    if (comment != NULL  &&  comment[0] != '\0')
    {
        fprintf(fp, "# COMMENT: ");

        for (cp = comment; *cp != '\0'; cp++)
        {
            if (*cp == '\n') fprintf(fp, "<CR>");
            else             fwrite(cp, 1, 1, fp);
        }
        
        fprintf(fp, "\n");
    }
    
    for (stage = (headers? STAGE_HEADERS : STAGE_DATA);
         stage <= STAGE_DATA;
         stage++)
    {
        tv_x = timenow;
        tv_y = start != NULL? *start : tv_x;
        timeval_subtract(&tv_r, &tv_x, &tv_y);
        
        if (stage == STAGE_HEADERS)
            fprintf(fp, "#Time(s-01.01.1970) YYYY-MM-DD-HH:MM:SS.mss secs-since0 ");
        else
            fprintf(fp, "%lu.%03d %s.%03d %7lu.%03d ",
                    (long)    timenow.tv_sec,       (int)(timenow.tv_usec / 1000),
                    stroftime(timenow.tv_sec, "-"), (int)(timenow.tv_usec / 1000),
                    (long)    tv_r.tv_sec,          (int)(tv_r.tv_usec    / 1000));
        
        for (ge = list;  ge->ei != NULL;  ge++)
            if (IdentifierIsSensible(ge->ei->ident))
                RecLogElement(fp, ge->ei, stage == STAGE_DATA);
        
        fprintf(fp, "\n");
    }

    fclose(fp);

    return 0;
}

static char *_cdr_colalarmslist[] =
{
    "UNINITIALIZED", "Uninitialized",
    "JustCreated",   "Just created",
    "Normal",        "Normal",
    "Yellow",        "Value in yellow zone",
    "Red",           "Value in red zone",
    "Weird",         "Value is weird",
    "Defunct",       "Defunct channel",
    "HWerr",         "Hardware error",
    "SFerr",         "Software error",
    "NOTFOUND",      "Channel not found",
    "Relax",         "Relaxing after alarm",
    "OtherOp",       "Other operator is active",
    "PrglyChg",      "Programmatically changed",
};

char *CdrStrcolalarmShort(colalarm_t state)
{
  static char buf[100];

    if (state < COLALARM_FIRST  ||  state > COLALARM_LAST)
    {
        sprintf(buf, "?COLALARM_%d?", state);
        return buf;
    }
    
    return _cdr_colalarmslist[state * 2];
}

char *CdrStrcolalarmLong (colalarm_t state)
{
  static char buf[100];

    if (state < COLALARM_FIRST  ||  state > COLALARM_LAST)
    {
        sprintf(buf, "Unknown color state #%d", state);
        return buf;
    }
    
    return _cdr_colalarmslist[state * 2 + 1];
}

colalarm_t CdrName2colalarm(char *name)
{
  int  state;
  
    for (state = COLALARM_FIRST;  state <= COLALARM_LAST;  state++)
        if (strcasecmp(_cdr_colalarmslist[state * 2], name) == 0)
            return state;

    return -1;
}

char *CdrLastErr(void)
{
    return _Cdr_lasterr_str[0] != '\0'? _Cdr_lasterr_str : strerror(errno);
}
