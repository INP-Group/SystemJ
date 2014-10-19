#ifndef __FASTADC_DATA_H
#define __FASTADC_DATA_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "pzframe_data.h"
#include "plotdata.h"


enum
{
    FASTADC_MAX_LINES = 16,
};


/********************************************************************/
struct _fastadc_data_t_struct;
struct _fastadc_mes_t_struct;
//--------------------------------------------------------------------
typedef void   (*fastadc_info2mes_t)   (int32 *info, struct _fastadc_mes_t_struct *mes_p);
typedef double (*fastadc_raw2pvl_t)    (int32 *info, int nl, int raw); /* Is used for integer data only */
typedef int    (*fastadc_x2xs_t)       (int32 *info, int x);
/********************************************************************/

typedef struct
{
    pzframe_type_dscr_t    ftd;

    int                    max_numpts;
    int                    num_lines;
    const char            *line_name_prefix;
    const char           **line_name_list;
    const char           **line_unit_list;
    int                    param_numpts;
    plotdata_range_t       range;
    const char            *dpyfmt;

    fastadc_info2mes_t     info2mes;
    fastadc_raw2pvl_t      raw2pvl;
    fastadc_x2xs_t         x2xs;
    const char            *xs;
} fastadc_type_dscr_t;

static inline fastadc_type_dscr_t *pzframe2fastadc_type_dscr(pzframe_type_dscr_t *ftd)
{
    return (ftd == NULL)? NULL
        : (fastadc_type_dscr_t *)(ftd); /*!!! Should use OFFSETOF() -- (uint8*)ftd-OFFSETOF(fastadc_type_dscr_t, ftd) */
}

/********************************************************************/

typedef struct
{
    int     show;
    int     magn;
    int     invp;
    int     _pad_;
    double  coeff;
    double  zero;
    char    units [10];
    char    descr [40];
    char    dpyfmt[20];
} fastadc_dcnv_one_t;

typedef struct
{
    fastadc_dcnv_one_t  lines[FASTADC_MAX_LINES];
    int                 cmpr;
} fastadc_dcnv_t;

/*!!! Bad... Should better live in _gui, but must be accessible to _data somehow... */
extern int               fastadc_data_magn_factors[7];
extern int               fastadc_data_cmpr_factors[7];

/********************************************************************/

typedef struct _fastadc_mes_t_struct
{
    pzframe_mes_t  *inhrt; /*!!! Maybe FORCE its use and eliminate fastadc_info2mes_t.info? */
    /* Derived info */     
    int             exttim;
    /* ?"Disassembled"? data */
    plotdata_t      plots[FASTADC_MAX_LINES];
} fastadc_mes_t;

typedef struct _fastadc_data_t_struct
{
    pzframe_data_t         pfr;
    fastadc_type_dscr_t   *atd;  // Should be =.pfr.ftd
    fastadc_dcnv_t         dcnv;

    fastadc_mes_t          mes;

    int                    use_svd;
    fastadc_mes_t          svd;
    pzframe_mes_t          inhrt_svd;
} fastadc_data_t;

static inline fastadc_data_t *pzframe2fastadc_data(pzframe_data_t *pzframe_data)
{
    return (pzframe_data == NULL)? NULL
        : (fastadc_data_t *)(pzframe_data); /*!!! Should use OFFSETOF() -- (uint8*)pzframe_data-OFFSETOF(fastadc_data_t, pfr) */
}

/********************************************************************/

static inline void FastadcSymmMinMaxInt(int    *min_val, int    *max_val)
{
  int     abs_max;

    if (*min_val < 0  &&  *max_val > 0)
    {
        abs_max = -*min_val;
        if (abs_max < *max_val)
            abs_max = *max_val;

        *min_val = -abs_max;
        *max_val =  abs_max;
    }
}

static inline void FastadcSymmMinMaxDbl(double *min_val, double *max_val)
{
  double  abs_max;

    if (*min_val < 0  &&  *max_val > 0)
    {
        abs_max = -*min_val;
        if (abs_max < *max_val)
            abs_max = *max_val;

        *min_val = -abs_max;
        *max_val =  abs_max;
    }
}

/********************************************************************/

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
                       fastadc_x2xs_t x2xs, const char *xs);

void FastadcDataInit       (fastadc_data_t *adc, fastadc_type_dscr_t *atd);

int  FastadcDataRealize    (fastadc_data_t *adc,
                            const char *srvrspec, int bigc_n,
                            cda_serverid_t def_regsid, int base_chan);

int  FastadcDataCalcDpyFmts(fastadc_data_t *adc);

psp_paramdescr_t *FastadcDataCreateText2DcnvTable(fastadc_type_dscr_t *atd,
                                                  char **mallocd_buf_p);

//--------------------------------------------------------------------
//--------------------------------------------------------------------

static inline double FastadcDataRaw2Pvl(fastadc_data_t *adc,
                                        fastadc_mes_t  *mes_p,
                                        int nl, int raw)
{
    return adc->atd->raw2pvl(mes_p->inhrt->info, nl, raw);
}

static inline double FastadcDataPvl2Dsp(fastadc_data_t *adc,
                                        fastadc_mes_t  *mes_p,
                                        int nl, double pvl)
{
    return pvl * adc->dcnv.lines[nl].coeff + adc->dcnv.lines[nl].zero;
}

static inline double FastadcDataGetPvl (fastadc_data_t *adc,
                                        fastadc_mes_t  *mes_p,
                                        int nl, int x)
{
    return
        (
         (reprof_cxdtype(mes_p->plots[nl].x_buf_dtype) == CXDTYPE_REPR_INT)
         ?
         adc->atd->raw2pvl(mes_p->inhrt->info, nl,
                           plotdata_get_int(mes_p->plots + nl, x))
         :
                           plotdata_get_dbl(mes_p->plots + nl, x)
        );
}

static inline double FastadcDataGetDsp (fastadc_data_t *adc,
                                        fastadc_mes_t  *mes_p,
                                        int nl, int x)
{
    return FastadcDataPvl2Dsp(adc, mes_p, nl,
                              FastadcDataGetPvl(adc, mes_p, nl, x));
}

int  FastadcDataSave       (fastadc_data_t *adc, const char *filespec,
                            const char *subsys, const char *comment);
int  FastadcDataLoad       (fastadc_data_t *adc, const char *filespec);
int  FastadcDataFilter     (pzframe_data_t *pfr,
                            const char *filespec,
                            time_t *cr_time,
                            char *commentbuf, size_t commentbuf_size);


#endif /* __FASTADC_DATA_H */
