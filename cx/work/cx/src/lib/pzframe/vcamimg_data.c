#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "misclib.h"
#include "timeval_utils.h"

#include "vcamimg_data.h"


//////////////////////////////////////////////////////////////////////

static void ProcessVciInfo(vcamimg_data_t *vci)
{
  vcamimg_type_dscr_t *vtd = vci->vtd;

    /*  */
    vci->mes.inhrt = &(vci->pfr.mes);

    /* Call VCI-type-specific processing */
    if (vtd->info2mes != NULL)
        vtd->info2mes(vci->pfr.mes.info, &(vci->mes));
}

//////////////////////////////////////////////////////////////////////

static void DoEvproc(pzframe_data_t *pfr,
                     int             reason,
                     int             info_changed,
                     void           *privptr)
{
  vcamimg_data_t *vci = pzframe2vcamimg_data(pfr);

    if (reason == CDA_R_MAINDATA) ProcessVciInfo(vci);
}

static int  DoSave  (pzframe_data_t *pfr,
                     const char     *filespec,
                     const char     *subsys, const char *comment)
{
    return VcamimgDataSave(pzframe2vcamimg_data(pfr), filespec,
                           subsys, comment);
}

static int  DoLoad  (pzframe_data_t *pfr,
                     const char     *filespec)
{
    return VcamimgDataLoad(pzframe2vcamimg_data(pfr), filespec);
}

static int  DoFilter(pzframe_data_t *pfr,
                     const char     *filespec,
                     time_t         *cr_time,
                     char           *commentbuf, size_t commentbuf_size)
{
    return VcamimgDataFilter(pfr, filespec, cr_time,
                             commentbuf, commentbuf_size);
}

pzframe_data_vmt_t vcamimg_data_std_pzframe_vmt =
{
    .evproc = DoEvproc,
    .save   = DoSave,
    .load   = DoLoad,
    .filter = DoFilter,
};
void
VcamimgDataFillStdDscr(vcamimg_type_dscr_t *vtd, const char *type_name,
                       int behaviour,
                       /* Bigc-related ADC characteristics */
                       cxdtype_t dtype,
                       int num_params,
                       int data_w, int data_h,
                       int show_w, int show_h,
                       int sofs_x, int sofs_y,
                       int bpp,
                       int srcmaxval,
                       vcamimg_info2mes_t info2mes,
                       /* Description of bigc-parameters */
                       psp_paramdescr_t *specific_params,
                       pzframe_paramdscr_t *param_dscrs)
{
    bzero(vtd, sizeof(*vtd));
    PzframeFillDscr(&(vtd->ftd), type_name,
                    behaviour,
                    dtype,
                    num_params,
                    data_w * data_h,
                    specific_params,
                    param_dscrs);
    vtd->ftd.vmt = vcamimg_data_std_pzframe_vmt;

    if      (reprof_cxdtype(dtype) != CXDTYPE_REPR_INT)
    {
        fprintf(stderr,
                "%s(type_name=\"%s\"): unsupported data-representation %d\n",
                __FUNCTION__, type_name, reprof_cxdtype(dtype));
    }

    vtd->data_w    = data_w;
    vtd->data_h    = data_h;
    vtd->show_w    = show_w;
    vtd->show_h    = show_h;
    vtd->sofs_x    = sofs_x;
    vtd->sofs_y    = sofs_y;

    vtd->bpp       = bpp;
    vtd->srcmaxval = srcmaxval;

    vtd->info2mes         = info2mes;
}

//////////////////////////////////////////////////////////////////////

void VcamimgDataInit       (vcamimg_data_t *vci, vcamimg_type_dscr_t *vtd)
{
  int  nl;

    bzero(vci, sizeof(*vci));
    vci->vtd = vtd;
    PzframeDataInit(&(vci->pfr), &(vtd->ftd));

    vci->mes.inhrt = &(vci->pfr.mes);
    vci->svd.inhrt = &(vci->inhrt_svd);

}

int  VcamimgDataRealize    (vcamimg_data_t *vci,
                            const char *srvrspec, int bigc_n,
                            cda_serverid_t def_regsid, int base_chan)
{
  vcamimg_type_dscr_t *vtd = vci->vtd;

  int                  r;
  int                  nl;
  char                *p;

    /* 0. Call parent */
    r = PzframeDataRealize(&(vci->pfr),
                           srvrspec, bigc_n,
                           def_regsid, base_chan);
    if (r != 0) return r;

    return 0;
}

psp_paramdescr_t *VcamimgDataCreateText2DcnvTable(vcamimg_type_dscr_t *vtd,
                                                  char **mallocd_buf_p)
{
  psp_paramdescr_t *ret           = NULL;
  int               ret_count;
  size_t            ret_size;

  int               x;
  psp_paramdescr_t *srcp;
  psp_paramdescr_t *dstp;

  static psp_paramdescr_t end_params[] =
  {
      PSP_P_END()
  };

    /* Allocate a table */
    ret_count = countof(end_params);
    ret_size  = ret_count * sizeof(*ret);
    ret       = malloc(ret_size);
    if (ret == NULL)
    {
        fprintf(stderr, "%s %s(type=%s): unable to allocate %zu bytes for PSP-table of parameters\n",
                strcurtime(), __FUNCTION__, vtd->ftd.type_name, ret_size);
        goto ERREXIT;
    }
    dstp = ret;

    /* END */
    for (x = 0,  srcp = end_params;
         x < countof(end_params);
         x++,    srcp++,  dstp++)
        *dstp = *srcp;

    if (mallocd_buf_p != NULL) *mallocd_buf_p = NULL;
    
    return ret;

 ERREXIT:
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


int  VcamimgDataSave       (vcamimg_data_t *vci, const char *filespec,
                            const char *subsys, const char *comment)
{
  vcamimg_type_dscr_t *vtd = vci->vtd;

  FILE        *fp;
  time_t       timenow = time(NULL);

  int          i;
  int          np;
  int          nl;
  int          x;
  const char  *param_name;

    if ((fp = fopen(filespec, "w")) == NULL) return -1;

    /* Standard header */
    fprintf(fp, "%s%s\n", sign_lp_s, vtd->ftd.type_name);
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
    for (np = 0;  np < vtd->ftd.num_params;  np++)
    {
        for (i = 0,                                    param_name = NULL;
             vtd->ftd.param_dscrs[i].name != NULL  &&  param_name == NULL;
             i++)
            if (vtd->ftd.param_dscrs[i].n == np)
                param_name = vtd->ftd.param_dscrs[i].name;
        if (param_name == NULL) param_name = "-";

        fprintf(fp, "%s %d %-10s %d\n",
                param_lp_s, np, param_name, vci->pfr.mes.info[np]);
    }

    /* Data */

    fclose(fp);

    return 0;
}

int  VcamimgDataLoad       (vcamimg_data_t *vci, const char *filespec)
{
  vcamimg_type_dscr_t *vtd = vci->vtd;

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
        strcasecmp(line + strlen(sign_lp_s), vtd->ftd.type_name) != 0)
    {
    fprintf(stderr, "sig mismatch\n");
        errno = 0;
        return -1;
    }

    /* Initialize everything with zeros... */
    bzero(vci->pfr.mes.info,       sizeof(vci->pfr.mes.info));
    bzero(vci->mes.inhrt->databuf, vtd->ftd.max_datasize);

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

            if (np < 0  ||  np >= vtd->ftd.num_params)
            {
                fprintf(stderr, "%s(\"%s\") line %d: parameter-number %d out_of [0..%d)\n",
                        __FUNCTION__, filespec, lineno, np, vtd->ftd.num_params);
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

            vci->pfr.mes.info[np] = ival;
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
                ProcessVciInfo(vci);
                data_started = 1;
            }

            x++;
        }

 NEXT_LINE:;
    }

    fclose(fp);

    return 0;

 PARSE_ERROR:;
    bzero(vci->pfr.mes.info,       sizeof(vci->pfr.mes.info));
    bzero(vci->mes.inhrt->databuf, vtd->ftd.max_datasize);
    ProcessVciInfo(vci);
    errno = 0;
    return -1;
}

int  VcamimgDataFilter     (pzframe_data_t *pfr,
                            const char *filespec,
                            time_t *cr_time,
                            char *commentbuf, size_t commentbuf_size)
{
  vcamimg_data_t      *vci = pzframe2vcamimg_data(pfr);
  vcamimg_type_dscr_t *vtd = vci->vtd;

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
                 strcasecmp(line + strlen(sign_lp_s), vtd->ftd.type_name) == 0)
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
