#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cx_sysdeps.h"

#include "cxlib.h"
#include "misc_macros.h"
#include "misclib.h"
#include "findfilein.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "cda_err.h"
#include "cda_internals.h"

#ifndef NAN
  #warning The NAN macro is undefined, using strtod("NAN") instead
  #define NAN strtod("NAN", NULL)
#endif


enum {NUM_TMP_REGS = 100};


static cda_lclregchg_cb_t  lclreg_chg_cb = NULL;

static cda_strlogger_t     strlogger     = NULL;


static int compare_doubles (const void *a, const void *b)
{
  const double *da = (const double *) a;
  const double *db = (const double *) b;
    
    return (*da > *db) - (*da < *db);
}

static excmd_lapprox_rec *lapprox_table_ldr(const void *ptr, int param)
{
  const char        *ref = ptr;
  int                xcol = (param / 65536) & 0xFFFF;
  int                ycol = param  & 0xFFFF;
  
  FILE              *fp;
  char               line[1000];

  int                maxcol;
  char              *p;
  double             v;
  double             a;
  double             b;
  char              *err;
  int                col;
  
  int                tab_size;
  int                tab_used;
  excmd_lapprox_rec *rec;
  excmd_lapprox_rec *nrp;

    if (ref == NULL) return NULL;

    if (xcol < 1) xcol = 1;
    if (ycol < 1) ycol = 2;

    fp = findfilein(ref,
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
                    program_invocation_name,
#else
                    NULL,
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */
                    NULL, ".lst",
                    "./"
                        FINDFILEIN_SEP "../common"
                        FINDFILEIN_SEP "!/"
                        FINDFILEIN_SEP "!/../settings/common"
                        FINDFILEIN_SEP "$PULT_ROOT/settings/common"
                        FINDFILEIN_SEP "~/pult/settings/common");
    if (fp == NULL)
    {
        _cda_reporterror("CMD_LAPPROX_FROM_I: couldn't find a table \"%s.lst\"", ref);
        return NULL;
    }

    tab_size = 0;
    tab_used = 0;
    rec      = malloc(sizeof(*rec));
    bzero(rec, sizeof(*rec));

    maxcol = xcol;
    if (ycol > maxcol) maxcol = ycol;
    
    while (fgets(line, sizeof(line) - 1, fp) != NULL)
    {
        line[sizeof(line) - 1] = '\0';

        /* Skip leading whitespace and ignore empty/comment lines */
        p = line;
        while(*p != '\0'  &&  isspace(*p)) p++;
        if (*p == '\0'  ||  *p == '#')
            goto NEXT_LINE;

        /* Obtain "x" and "y" */
        /* Note: gcc barks
               "warning: `a' might be used uninitialized in this function"
               "warning: `b' might be used uninitialized in this function"
           here, but in fact either a/b are initialized, or we skip their usage.
         */
        for (col = 1;  col <= maxcol;  col++)
        {
            v = strtod(p, &err);
            if (err == p  ||  (*err != '\0'  &&  !isspace(*err)))
                goto NEXT_LINE;
            p = err;

            if (col == xcol) a = v;
            if (col == ycol) b = v;
        }

        /* Grow table if required */
        if (tab_used >= tab_size)
        {
            tab_size += 20;
            nrp = safe_realloc(rec, sizeof(*rec) + sizeof(double)*2*tab_size);
            if (nrp == NULL) goto END_OF_FILE;
            rec = nrp;
        }

        rec->data[tab_used * 2]     = a;
        rec->data[tab_used * 2 + 1] = b;
        tab_used++;
        
 NEXT_LINE:;
    }
 END_OF_FILE:
    
    fclose(fp);

    ////fprintf(stderr, "%d points read from %s\n", tab_used, fname);
    
    rec->numpts = tab_used;
    rec->pts    = rec->data;

    /* Sort the table in ascending order */
    qsort(rec->pts, rec->numpts, sizeof(double)*2, compare_doubles);
    
    return rec;
}


excmd_t *cda_register_formula(cda_serverid_t defsid, excmd_t *formula, int options)
{
  excmd_t              *ret = NULL;
  int                   ncmds;
  excmd_t              *src;
  excmd_t              *dst;
  excmd_lapproxld_func  lf;
  excmd_lapprox_rec    *lrp;
  char                  srvnamebuf[1000];
  chanaddr_t            chan_n;
  cda_serverid_t        auxsid;
  excmd_t              *cp;

  int                   flags;
  int                   width;
  int                   precision;
  char                  conv_c;
  char                  dpyfmt[16];
  
    cda_clear_err();
  
    if (_CdaCheckSid(defsid) != 0) return NULL;

    if (formula == NULL) return NULL;

    if (formula->cmd == OP_COMPILE_I)
    {
        cda_set_err("Textual specification (OP_COMPILE_I) isn't supported yet");
        return NULL;
    }
    
    /* Find out the size of this formula */
    for (ncmds = 0, src = formula;
         (src->cmd & OP_code) != OP_RET;
         ncmds++, src++);
    ncmds++; /* OP_RET */

    /* Allocate space for its clone */
    ret = (excmd_t *)malloc(ncmds * sizeof(excmd_t));

    if (ret == NULL) return NULL;

    /* And perform cloning */
    for (src = formula, dst = ret;
         (src->cmd & OP_code) != OP_RET;
         src++, dst++)
    {
        *dst = *src;
        if      (src->cmd == OP_LAPPROX_BYFUNC_I)
        {
          excmd_lapproxnet_rec   *lsrc;
          excmd_lapprox_info_rec *lparams;
          const void             *ptr_param;
          int                     int_param;

            lf        = NULL;
            lparams   = NULL;
            ptr_param = NULL;
            int_param = 1;
            
            lsrc    = src->arg.lapprox_srcinfo;
            if (lsrc != NULL)
            {
                lf      = lsrc->ldr;
                lparams = lsrc->info;
                if (lparams != NULL)
                {
                    ptr_param = lparams->ptrparam;
                    int_param = lparams->intparam;
                }
            }

            if (lf == NULL) lf = lapprox_table_ldr;

            lrp = lf(ptr_param, int_param);
            if (lrp != NULL)
            {
                dst->cmd            = OP_LAPPROX_I;
                dst->arg.lapprox_rp = lrp;
            }
            else
            {
                dst->cmd           = OP_CALCERR_I;
                dst->arg.number    = 1.0;
                /*!!! Bark somehow...*/
            }
        }
        else if (src->cmd == OP_GETP_I  ||  src->cmd == OP_SETP_I)
        {
            dst->arg.handle = cda_add_physchan(defsid, src->arg.chan_id);
            if (src->cmd == OP_SETP_I  &&
                (options & CDA_OPT_IS_WRITE) == 0)
            {
                /*!!! Bark somehow...*/
            }
        }
        else if (src->cmd == OP_GETP_BYNAME_I  ||  src->cmd == OP_SETP_BYNAME_I  ||
                 src->cmd == OP_ADDSERVER_I)
        {
            /* Split channel name into "SERVER" and "CHAN#" parts */
            if (cx_parse_chanref(src->arg.chanref,
                                 srvnamebuf, sizeof(srvnamebuf), &chan_n) != 0)
            {
                cda_set_err("invalid channel reference \"%s\"", src->arg.chanref);
                goto ERREXIT;
            }

            auxsid = cda_add_auxsid(defsid, srvnamebuf, NULL, NULL);
            if (auxsid == CDA_SERVERID_ERROR)
            {
                cda_set_err("%s_I(\"%s\"): %s",
                            src->cmd == OP_ADDSERVER_I? "ADDSERVER" : "BYNAME",
                            srvnamebuf, cx_strerror(errno));
                goto ERREXIT;
            }

            if (src->cmd != OP_ADDSERVER_I)
            {
                /* And, finally, modify the command */
                dst->cmd = (src->cmd == OP_SETP_BYNAME_I? OP_SETP_I : OP_GETP_I);
                dst->arg.handle = cda_add_physchan(auxsid, chan_n);
                /*!!! Shouldn't we check for "cmd==OP_SETP_I  &&  !is_write" ?*/
            }
            else
            {
                dst->cmd = OP_NOOP;
            }
        }
        else if ((src->cmd & OP_code) == OP_REFRESH  &&
                 (options & CDA_OPT_IS_WRITE) == 0)
        {
            /*!!! Bark somehow...*/
        }
        else if (src->cmd == OP_DEBUGP_I)
        {
            if (src->arg.str != NULL) dst->arg.str = strdup(src->arg.str);
        }
        else if (src->cmd == OP_DEBUGPV_I)
        {
            if (ParseDoubleFormat(GetTextFormat(src->arg.str),
                                  &flags, &width, &precision, &conv_c) < 0)
            {
                _cda_reporterror("%s: invalid dpyfmt specification: %s",
                                 __FUNCTION__, printffmt_lasterr());
                ParseDoubleFormat(GetTextFormat(NULL), &flags, &width, &precision, &conv_c);
            }
            CreateDoubleFormat(dpyfmt, sizeof(dpyfmt), 20, 10,
                               flags, width, precision, conv_c);
            dst->arg.str = strdup(dpyfmt);
        }
        else if (src->cmd == OP_GOTO_I)
        {
            for (cp = src;  ;  cp++)
            {
                if      ((cp->cmd & OP_code) == OP_RET  ||
                         (cp->cmd & OP_code) == OP_RETSEP)
                {
                    dst->cmd = OP_NOOP;
                    _cda_reporterror("%s: label \"%s\" not found",
                                     __FUNCTION__, src->arg.label);
                    goto END_GOTO_LABEL_SEARCH;
                }
                else if (cp->cmd == OP_LABEL_I  &&
                         strcasecmp(cp->arg.label, src->arg.label) == 0)
                {
                    dst->arg.displacement = cp - src;
                    goto END_GOTO_LABEL_SEARCH;
                }
            }
 END_GOTO_LABEL_SEARCH:;
        }
        else if (src->cmd == OP_LABEL_I)
        {
            dst->cmd = OP_NOOP;
        }
    }
    *dst = *src; /* OP_RET */

    return ret;

 ERREXIT:
    /*!!!Should free all malloc()'d/strdup()'d/...'d entities */
    safe_free(ret);
    return NULL;
}

excmd_t *cda_HACK_findnext_f (excmd_t *formula)
{
  excmd_t *cp;
    
    if (formula == NULL) return NULL;

    for (cp = formula;  ;  cp++)
    {
        if ((cp->cmd & OP_code) == OP_RETSEP) return cp + 1;
        if ((cp->cmd & OP_code) == OP_RET)    break;
    }

    return NULL;
}


int  cda_execformula(excmd_t *formula,
                     int options, double userval,
                     double *result_p, tag_t *tag_p, rflags_t *rflags_p,
                     int32 *rv_p, int *rv_useful_p,
                     cda_localreginfo_t *localreginfo)
{
  int                ret    = 0;
  double             result = 0.0;
  tag_t              tag    = 0;
  rflags_t           rflags = 0;
  int32              rawv   = 0;
  int                rawv_c = 0;

  enum               {STKSIZE = 1000};
  excmd_content_t    dblstk[STKSIZE];
  int                dblidx = STKSIZE;

  double             tmp_dblregs      [NUM_TMP_REGS];
  char               tmp_dblregsinited[NUM_TMP_REGS];
  
  excmd_t           *cp;
  unsigned char      cmd;
  double             a1, a2, a3;
  double             cs; // case selector
  double             val;
  const char        *str;
  excmd_content_t    var;
  double             phys_val;
  tag_t              phys_tag;
  rflags_t           phys_rflags = 0; // Initialization for OP_GETOTHEROP
  int                displacement;

  int                order;
  double             x;
  double             sum;

  excmd_lapprox_rec *lapprp;
  int                npts;
  double            *matrix;
  
  int                intv;

#define TEMPLATE_STR "00savevalXXXXXX"
  int                fd;
  char               temp_name[20]; // >strlen(TEMPLATE_STR)+1
  struct
  {
      double         v;
      char           str[100];      // certainly sufficient for most "%8.3f"
  }                  svrec;
  int                nb_rqd;
  int                nbytes;
  
  struct timeval     timenow;
  int                timenow_obtained = 0;
  
  int                regs_op_wr;
  int                regs_cond;
  int                regs_type;
  char              *regs_name;
  int                regs_num;
  double            *regs_p;
  char              *regs_inited_p;

  enum
  {
      FLAG_ALLOW_WRITE  = 1 << 0,
      FLAG_SKIP_COMMAND = 1 << 1,
      FLAG_PRGLY        = 1 << 2,
  };
  int                thisflags;
  int                nextflags = 0;

    if (formula == NULL) return -1;

    bzero(tmp_dblregsinited, sizeof(tmp_dblregsinited));

    cp = formula;

    for (;  ;  cp++)
    {
        thisflags = nextflags;
        nextflags = 0;
        
        cmd = cp->cmd;
        if (cmd & OP_imm)
        {
            dblstk[--dblidx] = cp->arg;
        }

        if (
            (cmd & OP_code) == OP_RET     ||
            (cmd & OP_code) == OP_RETSEP  ||
            (cmd & OP_code) == OP_BREAK
           )
        {
            if (dblidx < STKSIZE)
                result = dblstk[dblidx++].number;
            else
            {
                result = NAN;
                if ((options & CDA_OPT_IS_WRITE) == 0)
                    _cda_reporterror("%s: dblstk UNDERFLOW upon return on cmd='%c'%s",
                                     __FUNCTION__, cmd & OP_code, cmd & OP_imm?"_I":"");
            }
            if ((cmd & OP_code) != OP_BREAK /* Only BREAK is conditional */
                ||
                (thisflags & FLAG_SKIP_COMMAND) == 0)
                break;
        }
        
        switch (cmd & OP_code)
        {
            case OP_ADD:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                dblstk[--dblidx].number = a1 + a2;
                break;
                
            case OP_SUB:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                dblstk[--dblidx].number = a1 - a2;
                break;
                
            case OP_MUL:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                dblstk[--dblidx].number = a1 * a2;
                break;
                
            case OP_DIV:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                if (fabs(a2) < 0.00001) a2 = (a2 >= 0) ?  0.00001 : -0.00001;
                dblstk[--dblidx].number = a1 / a2;
                break;

            case OP_MOD:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                if (fabs(a2) < 0.00001) a2 = (a2 >= 0) ?  0.00001 : -0.00001;
                dblstk[--dblidx].number = fmod(a1, a2);
                break;

            case OP_NEG:
                dblstk[dblidx].number = -dblstk[dblidx].number;
                break;
                
            case OP_ABS:
                dblstk[dblidx].number = fabs(dblstk[dblidx].number);
                break;

            case OP_ROUND:
                dblstk[dblidx].number = round(dblstk[dblidx].number);
                break;

            case OP_TRUNC:
                dblstk[dblidx].number = trunc(dblstk[dblidx].number);
                break;

            case OP_CASE:
                cs = dblstk[dblidx++].number;
                a3 = dblstk[dblidx++].number;
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                if      (cs < 0) val = a1;
                else if (cs > 0) val = a3;
                else             val = a2;
                dblstk[--dblidx].number = val;
                ////fprintf(stderr, "OP_CASE(@%p): %f?%f:%f:%f=>%f\n", cp, cs, a1, a2, a3, val);
                break;

            case OP_CMP_I & OP_code:
                intv = (int)(dblstk[dblidx++].number);
                a2   = dblstk[dblidx++].number;
                a1   = dblstk[dblidx++].number;
                switch (intv & OP_ARG_CMP_COND_MASK)
                {
                    case OP_ARG_CMP_LT: val = a1 <  a2; break;
                    case OP_ARG_CMP_LE: val = a1 <= a2; break;
                    case OP_ARG_CMP_EQ: val = a1 == a2; break;
                    case OP_ARG_CMP_GE: val = a1 >= a2; break;
                    case OP_ARG_CMP_GT: val = a1 >  a2; break;
                    case OP_ARG_CMP_NE: val = a1 != a2; break;
                    default: val = 0;
                }
                ////fprintf(stderr, "CMP: code=%08x val=%1.0f\n", intv, val);
                dblstk[--dblidx].number = val;
                if ((intv & OP_ARG_CMP_TEST) == 0) break;
                /* else fallthrough to TEST */
                
            case OP_TEST:
                val = dblstk[dblidx++].number;
                nextflags = (thisflags &~ FLAG_SKIP_COMMAND) |
                            (val != 0? 0 : FLAG_SKIP_COMMAND);
                break;

            case OP_ALLOW_W:
                nextflags = thisflags | FLAG_ALLOW_WRITE;
                break;

            case OP_GOTO_I & OP_code:
                displacement = dblstk[dblidx++].displacement;
                if ((thisflags & FLAG_SKIP_COMMAND) == 0)
                    cp += displacement;
                break;

            case OP_GETOTHEROP:
                dblstk[--dblidx].number = (phys_rflags & CXCF_FLAG_OTHEROP)? 1.0 : 0.0;
                break;

            case OP_BOOLEANIZE:
                dblstk[dblidx].number = fabs(dblstk[dblidx].number) > 0.00001;
                break;

            case OP_BOOL_OR:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                dblstk[--dblidx].number = fabs(a1) > 0.00001 || fabs(a2) > 0.00001;
                break;

            case OP_BOOL_AND:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                dblstk[--dblidx].number = fabs(a1) > 0.00001 && fabs(a2) > 0.00001;
                break;

            case OP_SQRT:
                dblstk[dblidx].number = sqrt(fabs(dblstk[dblidx].number));
                break;

            case OP_PW2:
                dblstk[dblidx].number = dblstk[dblidx].number * dblstk[dblidx].number;
                break;

            case OP_PWR:
                a2 = dblstk[dblidx++].number;
                a1 = dblstk[dblidx++].number;
                dblstk[--dblidx].number = pow(a1, a2);
                break;

            case OP_EXP:
                dblstk[dblidx].number = exp(dblstk[dblidx].number);
                break;

            case OP_LOG:
                dblstk[dblidx].number = log(dblstk[dblidx].number);
                break;

            case OP_POLY:
                order = (int)(dblstk[dblidx++].number);
                x     = dblstk[dblidx++].number;
                sum   = 0;

                while (order >= 0)
                {
                    sum += dblstk[dblidx++].number * pow(x, order);
                    order--;
                }
                
                dblstk[--dblidx].number = sum;
                break;

            case OP_LAPPROX_I & OP_code:
                lapprp = dblstk[dblidx++].lapprox_rp;
                x      = dblstk[dblidx++].number;
                npts   = lapprp->numpts;
                matrix = lapprp->pts;

                if (cda_do_linear_approximation(&x, x, npts, matrix) != 0)
                    rflags |= CXCF_FLAG_CALCERR;

                dblstk[--dblidx].number = x;
                break;
                
            case OP_NOOP:
                break;

            case OP_POP:
                dblidx++;
                break;

            case OP_DUP:
                var = dblstk[dblidx];
                dblstk[--dblidx] = var;
                break;

            case OP_GETP_I & OP_code:
                intv = dblstk[dblidx].handle;
                
                cda_getphyschanvnr(intv, &phys_val, &rawv, &phys_tag, &phys_rflags);
                dblstk[dblidx].number = phys_val;
                rawv_c++;

                if (phys_tag > tag) tag = phys_tag;
                rflags |= phys_rflags;
                break;

            case OP_SETP_I & OP_code:
                intv = dblstk[dblidx++].handle;
                a1   = dblstk[dblidx++].number;
                if ((thisflags & FLAG_SKIP_COMMAND) == 0)
                {
                    if (_cda_debug_setp)
                    {
                      const char *phys_srvname;
                      int         phys_chan_n;

                        phys_srvname = "UNKNOWN";
                        phys_chan_n  = -1;
                        cda_srcof_physchan(intv, &phys_srvname, &phys_chan_n);
                        fprintf(stderr, "SETP: %s.%d=%f\n",
                                phys_srvname, phys_chan_n, a1);
                    }
                    if ((options & CDA_OPT_READONLY) != 0) break;
                    if ((options   & CDA_OPT_IS_WRITE) != 0  ||
                        (thisflags & FLAG_ALLOW_WRITE) != 0)
                    {
                        (thisflags & FLAG_PRGLY) != 0 ?
                            cda_prgsetphyschanval(intv, a1)
                            :
                            cda_setphyschanval   (intv, a1) ;
                    }
                    else
                    {
                      const char *phys_srvname;
                      int         phys_chan_n;

                        phys_srvname = "UNKNOWN";
                        phys_chan_n  = -1;
                        cda_srcof_physchan(intv, &phys_srvname, &phys_chan_n);
                        _cda_reporterror("%s: SETP command in read-type formula: %s.%d=%f",
                                         __FUNCTION__, phys_srvname, phys_chan_n, a1);
                    }
                }
                break;

            case OP_PRGLY:
                nextflags = thisflags | FLAG_PRGLY;
                break;
                
            case OP_QRYVAL:
                if ((options & CDA_OPT_IS_WRITE) != 0)
                {
                    dblstk[--dblidx].number = userval;
                }
                else
                {
                    dblstk[--dblidx].number = NAN;
                    /*!!! Bark!*/
                }
                break;

            case OP_GETTIME:
                if (!timenow_obtained)
                {
                    gettimeofday(&timenow, NULL);
                    timenow_obtained = 1;
                }
                dblstk[--dblidx].number = timenow.tv_sec + ((double)timenow.tv_usec)/1000000;
                break;
                
            case OP_SETREG:
            case OP_GETREG:
                regs_op_wr = ((cmd & OP_code) == OP_SETREG);

                intv = (int)(dblstk[dblidx++].number);
                regs_type = intv & OP_ARG_REG_TYPE_MASK;
                regs_cond = intv & OP_ARG_REG_COND_MASK;
                intv &= OP_ARG_REG_NUM_MASK;

                if (regs_type == 0) /*!!! Check here if new reg-types appear*/
                {
                    regs_name     = "temporary";
                    regs_num      = NUM_TMP_REGS;
                    regs_p        = tmp_dblregs;
                    regs_inited_p = tmp_dblregsinited;
                }
                else
                {
                    regs_name     = "local";
                    regs_num      = 0;
                    regs_p        = NULL;
                    regs_inited_p = NULL;
                    if (localreginfo != NULL)
                    {
                        regs_num      = localreginfo->count;
                        regs_p        = localreginfo->regs;
                        regs_inited_p = localreginfo->regsinited;
                    }
                }
                
                if (intv < 0  ||  intv >= regs_num)
                {
                    /*!!! Bark "invalid register #"!*/
                    _cda_reporterror("%s: invalid %s register #%d in %s",
                                     __FUNCTION__,
                                     regs_name, intv,
                                     regs_op_wr? "SET":"GET");
                }
                else
                {
                    if (regs_op_wr)
                    {
                        a1 = dblstk[dblidx++].number;
                        if (regs_cond != OP_ARG_REG_COND_INIT  ||  !regs_inited_p[intv])
                        {
                            if ((thisflags & FLAG_SKIP_COMMAND) == 0)
                            {
                                regs_p       [intv] = a1;
                                regs_inited_p[intv] = 1;
                                if (regs_type     != 0     &&
                                    lclreg_chg_cb != NULL  &&
                                    regs_cond     != OP_ARG_REG_COND_INIT) lclreg_chg_cb(intv, a1);
                                if (_cda_debug_setreg)
                                    fprintf(stderr, "SETREG: %s#%d=%f\n",
                                           regs_name, intv, a1);
                            }
                        }
                    }
                    else
                    {
                        if (!regs_inited_p[intv])
                        {
                            /*!!! Bark "Use of uninitialized register"! */
                            a1 = NAN;
                        }
                        else
                        {
                            a1 = regs_p[intv];
                        }

                        dblstk[--dblidx].number = a1;
                    }
                }
                break;

            case OP_SAVEVAL_I & OP_code:
                str = dblstk[dblidx++].str;
                val = dblstk[dblidx++].number;

                if ((options & CDA_OPT_READONLY) != 0) break;

                strcpy(temp_name, TEMPLATE_STR);
                fd = mkstemp(temp_name);
                if (fd < 0)
                {
                    _cda_reporterror("SAVEVAL(\"%s\"): can't mkstemp(): %s",
                                     str, strerror(errno));
                }
                else
                {
                    svrec.v = val;
                    snprintf(svrec.str, sizeof(svrec.str), "\nV=%-8.3f\n", val);
                    svrec.str[sizeof(svrec.str) - 1] = '\0';
                    nb_rqd = sizeof(svrec.v) + strlen(svrec.str) + 1;

                    errno = 0;
                    nbytes = write(fd, &svrec, nb_rqd);
                    if (nbytes != nb_rqd)
                    {
                        _cda_reporterror("SAVEVAL(\"%s\"): write(%d bytes)=%d:%s",
                                         str, nbytes, nbytes, strerror(errno));
                        unlink(temp_name);
                    }
                    else
                    {
                        if (rename(temp_name, str) < 0)
                        {
                            _cda_reporterror("SAVEVAL(\"%s\"): rename(): %s",
                                             str, strerror(errno));
                            unlink(temp_name);
                        }
                    }

                    close(fd);
                }
                break;
                
            case OP_LOADVAL_I & OP_code:
                str = dblstk[dblidx++].str;
                val = dblstk[dblidx++].number;

                fd = open(str, O_RDONLY | O_NOCTTY | O_NONBLOCK);
                if (fd >= 0)
                {
                    nb_rqd = sizeof(a1);
                    errno = 0;
                    nbytes = read(fd, &a1, nb_rqd);
                    if (nbytes == nb_rqd)
                        val = a1;
                    else
                    {
                        _cda_reporterror("LOADVAL(\"%s\"): read(%d bytes)=%d:%s",
                                         str, nb_rqd, nbytes, strerror(errno));
                    }
                    close(fd);
                }
                dblstk[--dblidx].number = val;
                break;
                
            case OP_REFRESH:
                if ((options & CDA_OPT_IS_WRITE) != 0)
                {
                    val = dblstk[dblidx++].number;
                    if (val != 0) ret |= 1;
                }
                else
                    _cda_reporterror("%s: REFRESH command in read-type formula",
                                     __FUNCTION__);
                break;

            case OP_CALCERR:
                val = dblstk[dblidx++].number;
                if (val != 0) rflags |= CXCF_FLAG_CALCERR;
                break;

            case OP_WEIRD:
                val = dblstk[dblidx++].number;
                if (val != 0) rflags |= CXCF_FLAG_COLOR_WEIRD;
                break;

            case OP_SVFLAGS:
                dblstk[--dblidx].number = rflags;
                break;
                
            case OP_LDFLAGS:
                rflags |= ((rflags_t)(dblstk[dblidx++].number));
                break;
                
            case OP_DEBUGP_I & OP_code:
                str = dblstk[dblidx++].str;
                if (str != NULL  &&  (thisflags & FLAG_SKIP_COMMAND) == 0)
                    fprintf(stderr, "%s: %s%sCMD_DEBUGP_I: %s\n", strcurtime(),
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
                            program_invocation_short_name, ": ",
#else
                            "", "",
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */
                            str);
                break;
                
            case OP_DEBUGPV_I & OP_code:
                str = dblstk[dblidx++].str;
                val = dblstk[dblidx++].number;
                if ((thisflags & FLAG_SKIP_COMMAND) == 0)
                {
                    fprintf(stderr, "%s: %s%sCMD_DEBUGPV_I: %s", strcurtime(),
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
                            program_invocation_short_name, ": ",
#else
                            "", "",
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */
                            "" // This is just to allow previous lines end with ','
                           );
                    fprintf(stderr, str, val);
                    fprintf(stderr, "\n");
                }
                break;

            case OP_LOGSTR_I & OP_code:
                str = dblstk[dblidx++].str;
                if ((options & CDA_OPT_READONLY) != 0) break;
                if ((thisflags & FLAG_SKIP_COMMAND) == 0)
                {
                    if (strlogger != NULL) strlogger(str);
                    else
                        fprintf(stderr, "%s: %s%sCMD_LOGEVENT_I: %s\n", strcurtime(),
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
                                program_invocation_short_name, ": ",
#else
                                "", "",
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */
                                str);
                }
                break;

            case OP_BREAK:
                break;
                
            default:
                _cda_reporterror("%s: Unsupported opcode '%c'/chr(%d)",
                            __FUNCTION__, cmd & OP_code, cmd & OP_code);
        }

        if (dblidx > STKSIZE)
            _cda_reporterror("%s: dblstk UNDERFLOW after cmd='%c'%s",
                             __FUNCTION__, cmd & OP_code, cmd & OP_imm?"_I":"");
    }
    
    if (result_p    != NULL) *result_p    = result;
    if (tag_p       != NULL) *tag_p       = tag;
    if (rflags_p    != NULL) *rflags_p    = rflags;
    if (rv_p        != NULL) *rv_p        = rawv;
    if (rv_useful_p != NULL) *rv_useful_p = (rawv_c == 1);
    
    return ret;
}

int  cda_setlclreg(cda_localreginfo_t *localreginfo, int n, double  v)
{
    if (localreginfo == NULL  ||
        n < 0  ||  n >= localreginfo->count)
    {
        errno = EINVAL;
        return -1;
    }

    localreginfo->regs      [n] = v;
    localreginfo->regsinited[n] = 1;

    return 0;
}

int  cda_getlclreg(cda_localreginfo_t *localreginfo, int n, double *vp)
{
    if (localreginfo == NULL  ||
        n < 0  ||  n >= localreginfo->count)
    {
        errno = EINVAL;
        return -1;
    }

    *vp = localreginfo->regs[n];

    return localreginfo->regsinited[n]? 0 : 1;
}

void cda_set_lclregchg_cb(cda_lclregchg_cb_t cb)
{
    lclreg_chg_cb = cb;
}

void cda_set_strlogger(cda_strlogger_t proc)
{
    strlogger = proc;
}



excmd_lapprox_rec *cda_load_lapprox_table(const char *spec, int xcol, int ycol)
{
    return lapprox_table_ldr(spec, xcol * 65536 + ycol);
}

int cda_do_linear_approximation(double *rv_p, double x,
                                int npts, double *matrix)
{
  int     ret = 0;
  int     lb;
  double  x0, y0, x1, y1;

    if (npts == 1)
    {
        *rv_p = matrix[1];
        return 1;
    }
    if (npts <  1)
    {
        *rv_p = x;
        return 1;
    }
  
    if      (x < matrix[0])
    {
        ret = 1;
        lb = 0;
    }
    else if (x > matrix[(npts - 1) * 2])
    {
        ret = 1;
        lb = npts - 2;
    }
    else
    {
        /*!!!Should replace with binary search!*/
        for (lb = 0;  lb < npts - 1/*!!!?*/;  lb++)
            if (matrix[lb*2] <= x  &&  x <= matrix[(lb + 1) * 2])
                break;
    }

    x0 = matrix[lb * 2];
    y0 = matrix[lb * 2 + 1];
    x1 = matrix[lb * 2 + 2];
    y1 = matrix[lb * 2 + 3];
    
    *rv_p = y0 + (x - x0) * (y1 - y0) / (x1 - x0);

    if (_cda_debug_lapprox)
    {
        fprintf(stderr, "LAPPROX: x=%f, npts=%d; lb=%d:[%f,%f] =>%f  r=%d\n",
                                    x,       npts,  lb, x0,x1,  *rv_p, ret);
        //    if (npts == 121) fprintf(stderr, "x=%f: lb=%d v=%f\n", x, lb, *rv_p);
    }

    return ret;
}
