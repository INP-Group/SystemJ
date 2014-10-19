#ifndef __VCAMIMG_GUI_H
#define __VCAMIMG_GUI_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "plotdata.h"
#include "Xh_monoimg.h"

#include "vcamimg_data.h"
#include "pzframe_gui.h"


enum
{
    VCAMIMG_GUI_2_REPERS = 2,
};


typedef enum
{
    VCAMIMG_GUI_CTL_COMMONS,
    VCAMIMG_GUI_CTL_DPYMODE,
    VCAMIMG_GUI_CTL_UNDEFECT,
    VCAMIMG_GUI_CTL_NORMALIZE,
    VCAMIMG_GUI_CTL_MAX_RED,
    VCAMIMG_GUI_CTL_0_VIOLET,
} vcamimg_gui_ctl_kind_t;

enum
{
    VCAMIMG_REASON_
};


/********************************************************************/
struct _vcamimg_gui_t_struct;
/********************************************************************/

typedef Widget (*vcamimg_gui_mkctls_t)   (struct _pzframe_gui_t_struct *gui,
                                          vcamimg_type_dscr_t     *vtd,
                                          Widget                   parent,
                                          pzframe_gui_mkstdctl_t   mkstdctl,
                                          pzframe_gui_mkparknob_t  mkparknob);

/********************************************************************/

typedef struct
{
    pzframe_gui_dscr_t    gkd;
    vcamimg_type_dscr_t  *vtd;

    int                   img_w;
    int                   img_h;

    int                   cpanel_loc;
    vcamimg_gui_mkctls_t  mkctls;
} vcamimg_gui_dscr_t;

static inline vcamimg_gui_dscr_t *pzframe2vcamimg_gui_dscr(pzframe_gui_dscr_t *gkd)
{
    return (gkd == NULL)? NULL
        : (vcamimg_gui_dscr_t *)(gkd); /*!!! Should use OFFSETOF() -- (uint8*)gkd-OFFSETOF(vcamimg_gui_dscr_t, gkd) */
}

/********************************************************************/

typedef struct
{
    int     dpymode;
    int     normalize;
    int     maxred;
    int     violet0;
} vcamimg_gui_look_t;

typedef struct _vcamimg_gui_t_struct
{
    pzframe_gui_t           g;
    vcamimg_data_t          v;
    vcamimg_gui_dscr_t     *d;

    vcamimg_gui_look_t      look;

    XhMonoimg               view;

    Widget                  container;
    Widget                  cpanel;

    /* Param-passing-duplets */
} vcamimg_gui_t;

static inline vcamimg_gui_t *pzframe2vcamimg_gui(pzframe_gui_t *pzframe_gui)
{
    return (pzframe_gui == NULL)? NULL
        : (vcamimg_gui_t *)(pzframe_gui); /*!!! Should use OFFSETOF() -- (uint8*)pzframe_gui-OFFSETOF(vcamimg_gui_t, g) */
}

//////////////////////////////////////////////////////////////////////    

void  VcamimgGuiFillStdDscr(vcamimg_gui_dscr_t *gkd, vcamimg_type_dscr_t *vtd);

void  VcamimgGuiInit       (vcamimg_gui_t *gui, vcamimg_gui_dscr_t *gkd);
int   VcamimgGuiRealize    (vcamimg_gui_t *gui, Widget parent,
                            const char *srvrspec, int bigc_n,
                            cda_serverid_t def_regsid, int base_chan);

void  VcamimgGuiRenewView  (vcamimg_gui_t *gui, int info_changed);

psp_paramdescr_t *VcamimgGuiCreateText2LookTable(vcamimg_gui_dscr_t *gkd);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __VCAMIMG_GUI_H */
