#ifndef __CHL_MULTICOL_H
#define __CHL_MULTICOL_H


typedef struct
{
    _Chl_containeropts_t cont;

    int   one;
    int   nocoltitles;
    int   norowtitles;
    int   transposed;
    int   hspc;
    int   vspc;
    int   colspc;
} _Chl_multicolopts_t;
extern psp_paramdescr_t text2_Chl_multicolopts[];

int ELEM_MULTICOL_Create_m   (XhWidget parent, ElemInfo e);
int ELEM_MULTICOL_Create_m_as(XhWidget parent, ElemInfo e,
                              _Chl_multicolopts_t *opts_p);


#endif /* __CHL_MULTICOL_H */
