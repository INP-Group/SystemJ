#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "misclib.h"
#include "timeval_utils.h"

#include "fastadc_data.h"


int               fastadc_data_magn_factors[7] = {1, 2, 4, 10, 20, 50, 100};
int               fastadc_data_cmpr_factors[7] = {1, 2, 4, 10, 20, 50, 100};


//////////////////////////////////////////////////////////////////////

static void ProcessAdcInfo(fastadc_data_t *adc)
{
  fastadc_type_dscr_t *atd = adc->atd;

  int                  nl;

    /* Ensure acceptable NUMPTS value */
    if (adc->pfr.mes.info[atd->param_numpts] < 0)
        adc->pfr.mes.info[atd->param_numpts] = 0;
    if (adc->pfr.mes.info[atd->param_numpts] > atd->max_numpts)
        adc->pfr.mes.info[atd->param_numpts] = atd->max_numpts;

    /*  */
    adc->mes.inhrt = &(adc->pfr.mes);

    /* Call ADC-type-specific processing */
    if (atd->info2mes != NULL)
        atd->info2mes(adc->pfr.mes.info, &(adc->mes));
}

//////////////////////////////////////////////////////////////////////

static void DoEvproc(pzframe_data_t *pfr,
                     int             reason,
                     int             info_changed,
                     void           *privptr)
{
  fastadc_data_t *adc = pzframe2fastadc_data(pfr);

    if (reason == PZFRAME_REASON_BIGC_DATA) ProcessAdcInfo(adc);
}

static int  DoSave  (pzframe_data_t *pfr,
                     const char     *filespec,
                     const char     *subsys, const char *comment)
{
    return FastadcDataSave(pzframe2fastadc_data(pfr), filespec,
                           subsys, comment);
}

static int  DoLoad  (pzframe_data_t *pfr,
                     const char     *filespec)
{
    return FastadcDataLoad(pzframe2fastadc_data(pfr), filespec);
}

static int  DoFilter(pzframe_data_t *pfr,
                     const char     *filespec,
                     time_t         *cr_time,
                     char           *commentbuf, size_t commentbuf_size)
{
    return FastadcDataFilter(pfr, filespec, cr_time,
                             commentbuf, commentbuf_size);
}

pzframe_data_vmt_t fastadc_data_std_pzframe_vmt =
{
    .evproc = DoEvproc,
    .save   = DoSave,
    .load   = DoLoad,
    .filter = DoFilter,
};
void
FastadcDataFillStdDscr(fastadc_type_dscr_t *atd, const char *type_name,
                       int behaviour,
                       /* Bigc-related ADC characteristics */
                       cxdtype_t dtype,
                       int num_params,
                       int max_numpts, int num_lines,
                       int param_numpts,
                       fastadc_info2mes_t info2mes,
                       /* Description of bigc-parameters */
                       psp_paramdescr_t *specific_params,
                       pzframe_paramdscr_t *param_dscrs,
                       /* Data specifics */
                       const char *line_name_prefix, const char **line_name_list,
                       const char **line_unit_list,
                       plotdata_range_t range,
                       const char *dpyfmt, fastadc_raw2pvl_t raw2pvl,
                       fastadc_x2xs_t x2xs, const char *xs)
{
    bzero(atd, sizeof(*atd));
    PzframeFillDscr(&(atd->ftd), type_name,
                    behaviour,
                    dtype,
                    num_params,
                    max_numpts * num_lines,
                    specific_params,
                    param_dscrs);
    atd->ftd.vmt = fastadc_data_std_pzframe_vmt;

    if      (reprof_cxdtype(dtype) == CXDTYPE_REPR_INT)
    {
        FastadcSymmMinMaxInt(range.int_r + 0, range.int_r + 1);
    }
    else if (reprof_cxdtype(dtype) == CXDTYPE_REPR_FLOAT)
    {
        FastadcSymmMinMaxDbl(range.dbl_r + 0, range.dbl_r + 1);
    }
    else
    {
        fprintf(stderr,
                "%s(type_name=\"%s\"): unsupported data-representation %d\n",
                __FUNCTION__, type_name, reprof_cxdtype(dtype));
    }

    atd->max_numpts       = max_numpts;
    atd->num_lines        = num_lines;

    atd->param_numpts     = param_numpts;
    atd->info2mes         = info2mes;

    atd->line_name_prefix = line_name_prefix;
    atd->line_name_list   = line_name_list;
    atd->line_unit_list   = line_unit_list;
    atd->range            = range;
    atd->dpyfmt           = dpyfmt;
    atd->raw2pvl          = raw2pvl;
    atd->x2xs             = x2xs;
    atd->xs               = xs;
}

//////////////////////////////////////////////////////////////////////

void FastadcDataInit       (fastadc_data_t *adc, fastadc_type_dscr_t *atd)
{
  int  nl;

    bzero(adc, sizeof(*adc));
    adc->atd = atd;
    PzframeDataInit(&(adc->pfr), &(atd->ftd));

    adc->mes.inhrt = &(adc->pfr.mes);
    adc->svd.inhrt = &(adc->inhrt_svd);

    for (nl = 0;  nl < atd->num_lines;  nl++)
    {
        /* Set default ranges */
        adc->mes.plots[nl].all_range   = atd->range;
        adc->mes.plots[nl].cur_range   = atd->range;
        adc->mes.plots[nl].x_buf_dtype = atd->ftd.dtype;
    }
}

static inline int notempty(const char *s)
{
    return s != NULL  &&  *s != '\0';
}

int  FastadcDataRealize    (fastadc_data_t *adc,
                            const char *srvrspec, int bigc_n,
                            cda_serverid_t def_regsid, int base_chan)
{
  fastadc_type_dscr_t *atd = adc->atd;

  int                  r;
  int                  nl;
  char                *p;

    /* 0. Call parent */
    r = PzframeDataRealize(&(adc->pfr),
                           srvrspec, bigc_n,
                           def_regsid, base_chan);
    if (r != 0) return r;

    /* 1. Prepare descriptions */
    for (nl = 0;  nl < adc->atd->num_lines;  nl++)
    {
        /* Create descriptions, if not specified by user... */
        if (        adc->dcnv.lines[nl].descr[0] == '\0')
            sprintf(adc->dcnv.lines[nl].descr, "%s%s%s",
                    notempty(atd->line_name_prefix)? atd->line_name_prefix : "",
                    notempty(atd->line_name_prefix)? " "                   : "",
                    atd->line_name_list[nl]);

        /* ...and sanitize 'em */
        for (p = adc->dcnv.lines[nl].descr;  *p != '\0';  p++)
            if (*p == '\''  ||  *p == '\"'  ||  *p == '\\'  ||  !isprint(*p))
                *p = '?';
    }            

    /* 2. DpyFmts */
    if ((r = FastadcDataCalcDpyFmts(adc)) < 0) return r;

    return 0;
}

int  FastadcDataCalcDpyFmts(fastadc_data_t *adc)
{
  int          nl;
    
  int          dpy_flags;
  int          dpy_width;
  int          dpy_precision;
  char         dpy_conv_c;
  int          dpy_shift;

    if (ParseDoubleFormat(adc->atd->dpyfmt,
                          &dpy_flags, &dpy_width, &dpy_precision, &dpy_conv_c) < 0)
    {
        fprintf(stderr, "ParseDoubleFormat(\"%s\"): %s\n",
                adc->atd->dpyfmt, printffmt_lasterr());
        return -1;
    }

    for (nl = 0;  nl < adc->atd->num_lines;  nl++)
    {
        dpy_shift = round(log10(fabs(adc->dcnv.lines[nl].coeff)) + 0.4);
        ////printf("shift=%d\n", dpy_shift);
        CreateDoubleFormat(adc->dcnv.lines[nl].dpyfmt, sizeof(adc->dcnv.lines[nl].dpyfmt), 20, 10,
                           dpy_flags,
                           dpy_width,
                           dpy_precision >= dpy_shift? dpy_precision - dpy_shift : 0,
                           dpy_conv_c);
    }

    return 0;
}

psp_paramdescr_t *FastadcDataCreateText2DcnvTable(fastadc_type_dscr_t *atd,
                                                  char **mallocd_buf_p)
{
  psp_paramdescr_t *ret           = NULL;
  int               ret_count;
  size_t            ret_size;

  int               x;
  int               nl;
  psp_paramdescr_t *srcp;
  psp_paramdescr_t *dstp;
  size_t            pcp_names_size;
  char             *pcp_names_buf = NULL;
  char             *pcp_names_p;

  static char      magn_lkp_labels[countof(fastadc_data_magn_factors)][10];
  static psp_lkp_t magn_lkp       [countof(fastadc_data_magn_factors) + 1];
  static char      cmpr_lkp_labels[countof(fastadc_data_cmpr_factors)][10];
  static psp_lkp_t cmpr_lkp       [countof(fastadc_data_cmpr_factors) + 1];
  static int       nnnn_lkps_inited = 0;

  static psp_paramdescr_t perchan_params[] =
  {
      PSP_P_FLAG   ("chan",     fastadc_dcnv_t, lines[0].show,  1, 1),
      PSP_P_FLAG   ("nochan",   fastadc_dcnv_t, lines[0].show,  0, 0),
      PSP_P_LOOKUP ("magn",     fastadc_dcnv_t, lines[0].magn,  1, magn_lkp),
      PSP_P_FLAG   ("straight", fastadc_dcnv_t, lines[0].invp,  0, 1),
      PSP_P_FLAG   ("inverse",  fastadc_dcnv_t, lines[0].invp,  1, 0),
      PSP_P_REAL   ("coeff",    fastadc_dcnv_t, lines[0].coeff, 1.0, 0.0, 0.0),
      PSP_P_REAL   ("zero",     fastadc_dcnv_t, lines[0].zero,  0.0, 0.0, 0.0),
      PSP_P_STRING ("units",    fastadc_dcnv_t, lines[0].units, "V"),
      PSP_P_STRING ("descr",    fastadc_dcnv_t, lines[0].descr, ""),
  };

  static psp_paramdescr_t end_params[] =
  {
      PSP_P_LOOKUP ("cmpr",   fastadc_dcnv_t, cmpr,           1, cmpr_lkp),
      PSP_P_END()
  };

    if (!nnnn_lkps_inited)
    {
        for (x = 0;  x < countof(fastadc_data_magn_factors);  x++)
        {
            sprintf(magn_lkp_labels[x], "%d", fastadc_data_magn_factors[x]);
            magn_lkp[x].label = magn_lkp_labels          [x];
            magn_lkp[x].val   = fastadc_data_magn_factors[x];
        }
        for (x = 0;  x < countof(fastadc_data_cmpr_factors);  x++)
        {
            sprintf(cmpr_lkp_labels[x], "%d", fastadc_data_cmpr_factors[x]);
            cmpr_lkp[x].label = cmpr_lkp_labels          [x];
            cmpr_lkp[x].val   = fastadc_data_cmpr_factors[x];
        }
        nnnn_lkps_inited = 1;
    }

    /* Allocate a table */
    ret_count = countof(perchan_params) * atd->num_lines + countof(end_params);
    ret_size  = ret_count * sizeof(*ret);
    ret       = malloc(ret_size);
    if (ret == NULL)
    {
        fprintf(stderr, "%s %s(type=%s): unable to allocate %zu bytes for PSP-table of parameters\n",
                strcurtime(), __FUNCTION__, atd->ftd.type_name, ret_size);
        goto ERREXIT;
    }
    dstp = ret;

    /* Allocate a names-buffer */
    /* Calc size... */
    for (x = 0,  pcp_names_size = 0;
         x < countof(perchan_params);
         x++)
        for (nl = 0; nl < atd->num_lines;  nl++)
            pcp_names_size +=
                strlen(perchan_params[x].name)  +
                strlen(atd->line_name_list[nl]) +
                1;
    /* ...do allocate */
    pcp_names_buf   = malloc(pcp_names_size);
    if (pcp_names_buf == NULL)
    {
        fprintf(stderr, "%s %s(type=%s): unable to allocate %zu bytes for per-channel parameter names\n",
                strcurtime(), __FUNCTION__, atd->ftd.type_name, pcp_names_size);
        goto ERREXIT;
    }

    /* Fill in data */
    for (x = 0,  srcp = perchan_params,  pcp_names_p = pcp_names_buf;
         x < countof(perchan_params);
         x++,    srcp++ /*NO "dstp++" -- that is done in inner loop*/)
        for (nl = 0;
             nl < atd->num_lines;
             nl++, dstp++)
        {
            *dstp = *srcp;                                   // Copy main properties
            dstp->offset += sizeof(fastadc_dcnv_one_t) * nl; // Tweak offset to point to line'th element of respective array
            sprintf(pcp_names_p, "%s%s",                     // Store name...
                    srcp->name, atd->line_name_list[nl]);    // ...appending line-name
            dstp->name = pcp_names_p;                        // And remember it
            pcp_names_p += strlen(dstp->name) + 1;           // Advance buffer pointer

            /* Special case -- units */
            if (srcp->offset == PSP_OFFSET_OF(fastadc_dcnv_t, lines[0].units))
            {
                if (atd->line_unit_list != NULL)
                {
                    dstp->var.p_string.defval = atd->line_unit_list[nl];
                }
            }
        }

    /* END */
    for (x = 0,  srcp = end_params;
         x < countof(end_params);
         x++,    srcp++,  dstp++)
        *dstp = *srcp;

    if (mallocd_buf_p != NULL) *mallocd_buf_p = pcp_names_buf;
    
    return ret;

 ERREXIT:
    safe_free(pcp_names_buf);
    safe_free(ret);
    return NULL;
}

//////////////////////////////////////////////////////////////////////

static void fprintf_s_printable(FILE *fp, const char *s)
{
    if (s == NULL) return;
    while (*s != '\0')
    {
        fprintf(fp, "%c", isprint(*s)? *s : '?');
        s++;
    }
}

static void rtrim_line(char *line)
{
  char *p = line + strlen(line) - 1;

    while (p > line  &&
           (*p == '\n'  ||  isspace(*p)))
        *p-- = '\0';
}

static const char *sign_lp_s    = "# Data:";
static const char *subsys_lp_s  = "#!SUBSYSTEM:";
static const char *crtime_lp_s  = "#!CREATE-TIME:";
static const char *comment_lp_s = "#!COMMENT:";
static const char *param_lp_s   = "#.PARAM";


int  FastadcDataSave       (fastadc_data_t *adc, const char *filespec,
                            const char *subsys, const char *comment)
{
  fastadc_type_dscr_t *atd = adc->atd;

  FILE        *fp;
  time_t       timenow = time(NULL);

  int          i;
  int          np;
  int          nl;
  int          x;
  const char  *param_name;

    if ((fp = fopen(filespec, "w")) == NULL) return -1;

    /* Standard header */
    fprintf(fp, "%s%s\n", sign_lp_s, atd->ftd.type_name);
    if (subsys != NULL)
    {
        fprintf(fp, "%s ", subsys_lp_s);
        fprintf_s_printable(fp, subsys);
        fprintf(fp, "\n");
    }
    fprintf(fp, "%s %lu %s", crtime_lp_s, (unsigned long)(timenow), ctime(&timenow)); /*!: No '\n' because ctime includes a trailing one. */
    if (comment != NULL)
    {
        fprintf(fp, "%s ", comment_lp_s);
        fprintf_s_printable(fp, comment);
        fprintf(fp, "\n");
    }

    /* Parameters */
    for (np = 0;  np < atd->ftd.num_params;  np++)
    {
        for (i = 0,                                    param_name = NULL;
             atd->ftd.param_dscrs[i].name != NULL  &&  param_name == NULL;
             i++)
            if (atd->ftd.param_dscrs[i].n == np)
                param_name = atd->ftd.param_dscrs[i].name;
        if (param_name == NULL) param_name = "-";

        fprintf(fp, "%s %d %-10s %d\n",
                param_lp_s, np, param_name, adc->pfr.mes.info[np]);
    }

    /* Data */
    /* Header... */
    fprintf(fp, "\n# %7s %8s", "N", adc->mes.exttim? "n" : atd->xs);
    for (nl = 0;  nl < atd->num_lines;  nl++)
        fprintf(fp, " c%d", nl);
    for (nl = 0;  nl < atd->num_lines;  nl++)
        fprintf(fp, " v%d", nl);
    fprintf(fp, "\n");
    /* ...data itself */
    for (x = 0;  x < adc->pfr.mes.info[atd->param_numpts];  x++)
    {
        fprintf(fp, " %6d %d",
                x,
                adc->mes.exttim? x : atd->x2xs(adc->pfr.mes.info, x));
        for (nl = 0;  nl < atd->num_lines;  nl++)
        {
            fprintf(fp, " ");
            if (adc->mes.plots[nl].on  &&  adc->mes.plots[nl].numpts > x)
            {
                if (reprof_cxdtype(adc->mes.plots[nl].x_buf_dtype) == CXDTYPE_REPR_INT)
                    fprintf(fp, "%9d",
                            plotdata_get_int(adc->mes.plots + nl, x));
                else
                    fprintf(fp, adc->dcnv.lines[nl].dpyfmt,
                            plotdata_get_dbl(adc->mes.plots + nl, x));
            }
            else
                fprintf(fp, "0");
        }
        for (nl = 0;  nl < atd->num_lines;  nl++)
        {
            fprintf(fp, " ");
            if (adc->mes.plots[nl].on  &&  adc->mes.plots[nl].numpts > x)
                fprintf(fp, adc->dcnv.lines[nl].dpyfmt,
                        FastadcDataGetDsp(adc, &adc->mes, nl, x));
            else
                fprintf(fp, "0");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    return 0;
}

int  FastadcDataLoad       (fastadc_data_t *adc, const char *filespec)
{
  fastadc_type_dscr_t *atd = adc->atd;

  FILE        *fp;
  char         line[1000];
  int          lineno;
  int          data_started;

  char        *p;
  char        *errp;

  int          np;
  int          ival;
  double       fval;
  int          x;
  int          nl;

    /* Try to open file and read the 1st line */
    if ((fp = fopen(filespec, "r")) == NULL  ||
        fgets(line, sizeof(line) - 1, fp)  == NULL)
    {
        if (fp != NULL) fclose(fp);
        return -1;
    }

    /* Check signature */
    rtrim_line(line);
    if (memcmp(line, sign_lp_s,   strlen(sign_lp_s))   != 0    ||
        strcasecmp(line + strlen(sign_lp_s), atd->ftd.type_name) != 0)
    {
    fprintf(stderr, "sig mismatch\n");
        errno = 0;
        return -1;
    }

    /* Initialize everything with zeros... */
    bzero(adc->pfr.mes.info,       sizeof(adc->pfr.mes.info));
    bzero(adc->mes.inhrt->databuf, atd->ftd.max_datasize);

    lineno = 1;
    data_started = 0;
    x            = 0;
    while (fgets(line, sizeof(line) - 1, fp) != NULL)
    {
        lineno++;
        rtrim_line(line);
        p = line; while (*p != '\0'  &&  isspace(*p)) p++;

        if (*p == '\0') goto NEXT_LINE;

        if (memcmp(line, param_lp_s, strlen(param_lp_s)) == 0)
        {
            if (data_started)
            {
                fprintf(stderr, "%s(\"%s\") line %d: parameter AFTER data\n",
                        __FUNCTION__, filespec, lineno);
                goto PARSE_ERROR;
            }

            /* Get np */
            p += strlen(param_lp_s);
            while (*p != '\0'  &&  isspace(*p)) p++;
            np = strtol(p, &errp, 10);
            if (errp == p  ||  !isspace(*errp))
            {
                fprintf(stderr, "%s(\"%s\") line %d: error parsing parameter-number\n",
                        __FUNCTION__, filespec, lineno);
                goto PARSE_ERROR;
            }
            p = errp;

            if (np < 0  ||  np >= atd->ftd.num_params)
            {
                fprintf(stderr, "%s(\"%s\") line %d: parameter-number %d out_of [0..%d)\n",
                        __FUNCTION__, filespec, lineno, np, atd->ftd.num_params);
                goto PARSE_ERROR;
            }

            /* Skip name */
            while (*p != '\0'  &&  isspace(*p))  p++; // leading spaces
            while (*p != '\0'  &&  !isspace(*p)) p++; // name itself

            /* Get value */
            while (*p != '\0'  &&  isspace(*p)) p++;
            ival = strtol(p, &errp, 0);
            if (errp == p  ||  (*errp != '\0'  &&  !isspace(*errp)))
            {
                fprintf(stderr, "%s(\"%s\") line %d: error parsing parameter-value\n",
                        __FUNCTION__, filespec, lineno);
                goto PARSE_ERROR;
            }

            adc->pfr.mes.info[np] = ival;
        }
        else if (*p == '#') goto NEXT_LINE;
        else if (!isdigit(*p))
        {
            fprintf(stderr, "%s(\"%s\") line %d: garbage\n",
                    __FUNCTION__, filespec, lineno);
            goto PARSE_ERROR;
        }
        else
        {
            /* Okay -- that's data */

            if (!data_started)
            {
                ProcessAdcInfo(adc);
                data_started = 1;
            }

            if (x > adc->pfr.mes.info[atd->param_numpts])
            {
                fprintf(stderr, "%s(\"%s\") line %d: too many measurements, exceeding numpts=%d\n",
                        __FUNCTION__, filespec, lineno, adc->pfr.mes.info[atd->param_numpts]);
                break;
            }

            /* Skip N... */
            while (*p != '\0'  &&  !isspace(*p)) p++;
            /* ...and xs */
            while (*p != '\0'  &&  isspace(*p))  p++;
            while (*p != '\0'  &&  !isspace(*p)) p++;

            for (nl = 0;  nl < atd->num_lines;  nl++)
            {
                while (*p != '\0'  &&  isspace(*p))  p++;
                if (reprof_cxdtype(adc->mes.plots[nl].x_buf_dtype) == CXDTYPE_REPR_INT)
                    ival = strtol(p, &errp, 0);
                else
                    fval = strtod(p, &errp);
                if (errp == p  ||  (*errp != '\0'  &&  !isspace(*errp)))
                {
                    fprintf(stderr, "%s(\"%s\") line %d: error parsing line#%d@x=%d\n",
                            __FUNCTION__, filespec, lineno, nl, x);
                    goto PARSE_ERROR;
                }
                p = errp;
                if (adc->mes.plots[nl].on  &&  adc->mes.plots[nl].numpts > x)
                {
                    if (reprof_cxdtype(adc->mes.plots[nl].x_buf_dtype) == CXDTYPE_REPR_INT)
                        plotdata_put_int(adc->mes.plots + nl, x, ival);
                    else
                        plotdata_put_dbl(adc->mes.plots + nl, x, fval);
                }
            }
            x++;
        }

 NEXT_LINE:;
    }

    fclose(fp);

    return 0;

 PARSE_ERROR:;
    bzero(adc->pfr.mes.info,       sizeof(adc->pfr.mes.info));
    bzero(adc->mes.inhrt->databuf, atd->ftd.max_datasize);
    ProcessAdcInfo(adc);
    errno = 0;
    return -1;
}

int  FastadcDataFilter     (pzframe_data_t *pfr,
                            const char *filespec,
                            time_t *cr_time,
                            char *commentbuf, size_t commentbuf_size)
{
  fastadc_data_t      *adc = pzframe2fastadc_data(pfr);
  fastadc_type_dscr_t *atd = adc->atd;

  FILE        *fp;
  char         line[1000];
  int          lineno;
  int          was_sign = 0;

  char        *cp;
  char        *err;
  time_t       sng_time;

    if ((fp = fopen(filespec, "r")) == NULL) return -1;

    if (commentbuf_size > 0) *commentbuf = '\0';

    lineno = 0;
    while (lineno < 10  &&  fgets(line, sizeof(line) - 1, fp) != NULL)
    {
        lineno++;
        rtrim_line(line);

        if      (memcmp(line, sign_lp_s,   strlen(sign_lp_s))   == 0    &&
                 strcasecmp(line + strlen(sign_lp_s), atd->ftd.type_name) == 0)
            was_sign = 1;
        else if (memcmp(line, crtime_lp_s, strlen(crtime_lp_s)) == 0)
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

    return was_sign? 0 : 1;
}
