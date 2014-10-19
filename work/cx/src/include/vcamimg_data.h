#ifndef __VCAMIMG_DATA_H
#define __VCAMIMG_DATA_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "pzframe_data.h"
#include "plotdata.h"


enum
{
    VCAMIMG_MAX_LINES = 16,
};


/********************************************************************/
struct _vcamimg_data_t_struct;
struct _vcamimg_mes_t_struct;
//--------------------------------------------------------------------
typedef void   (*vcamimg_info2mes_t)   (int32 *info, struct _vcamimg_mes_t_struct *mes_p);
/********************************************************************/

typedef struct
{
    pzframe_type_dscr_t    ftd;

    int                    data_w;
    int                    data_h;
    int                    show_w;
    int                    show_h;
    int                    sofs_x;
    int                    sofs_y;

    int                    bpp;
    int                    srcmaxval;

    vcamimg_info2mes_t     info2mes;
} vcamimg_type_dscr_t;

static inline vcamimg_type_dscr_t *pzframe2vcamimg_type_dscr(pzframe_type_dscr_t *ftd)
{
    return (ftd == NULL)? NULL
        :(vcamimg_type_dscr_t *)(ftd); /*!!! Should use OFFSETOF() -- (uint8*)ftd-OFFSETOF(vcamimg_type_dscr_t, ftd) */
}

/********************************************************************/

typedef struct
{
    // Maybe place "display scaling" (mindisp,maxdisp) here?
} vcamimg_dcnv_t;

/********************************************************************/

typedef struct _vcamimg_mes_t_struct
{
    pzframe_mes_t  *inhrt; /*!!! Maybe FORCE its use and eliminate vcamimg_info2mes_t.info? */
    /* ?"Disassembled"? data */
    // None for now...
} vcamimg_mes_t;

typedef struct _vcamimg_data_t_struct
{
    pzframe_data_t         pfr;
    vcamimg_type_dscr_t   *vtd;  // Should be =.pfr.ftd
    vcamimg_dcnv_t         dcnv;

    vcamimg_mes_t          mes;

    int                    use_svd;
    vcamimg_mes_t          svd;
    pzframe_mes_t          inhrt_svd;
} vcamimg_data_t;

static inline vcamimg_data_t *pzframe2vcamimg_data(pzframe_data_t *pzframe_data)
{
    return (pzframe_data == NULL)? NULL
        : (vcamimg_data_t *)(pzframe_data); /*!!! Should use OFFSETOF() -- (uint8*)pzframe_data-OFFSETOF(vcamimg_data_t, pfr) */
}

/********************************************************************/

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
                       pzframe_paramdscr_t *param_dscrs);

void VcamimgDataInit       (vcamimg_data_t *vci, vcamimg_type_dscr_t *vtd);

int  VcamimgDataRealize    (vcamimg_data_t *vci,
                            const char *srvrspec, int bigc_n,
                            cda_serverid_t def_regsid, int base_chan);

int  VcamimgDataCalcDpyFmts(vcamimg_data_t *vci);

psp_paramdescr_t *VcamimgDataCreateText2DcnvTable(vcamimg_type_dscr_t *vtd,
                                                  char **mallocd_buf_p);

//--------------------------------------------------------------------
//--------------------------------------------------------------------

int  VcamimgDataSave       (vcamimg_data_t *vci, const char *filespec,
                            const char *subsys, const char *comment);
int  VcamimgDataLoad       (vcamimg_data_t *vci, const char *filespec);
int  VcamimgDataFilter     (pzframe_data_t *pfr,
                            const char *filespec,
                            time_t *cr_time,
                            char *commentbuf, size_t commentbuf_size);


#endif /* __VCAMIMG_DATA_H */
