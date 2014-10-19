#ifndef __U0632_GUI_H
#define __U0632_GUI_H


#include "pzframe_gui.h"


enum
{
    U0632_GUI_TYPE_XY = 1,
    U0632_GUI_TYPE_ES = 2,
};

struct _u0632_gui_t_struct;
typedef void (*u0632_gui_draw_t)(struct _u0632_gui_t_struct *gui);

typedef struct _u0632_gui_t_struct
{
    pzframe_gui_t      g;
    pzframe_data_t     h;

    int                type;

    Knob               i_k;

    GC                 bkgdGC;
    GC                 axisGC;
    GC                 reprGC;
    GC                 prevGC;
    GC                 outlGC;
    GC                 dat1GC[COLALARM_LAST + 1];
    GC                 dat2GC[COLALARM_LAST + 1];
    GC                 textGC;
    XFontStruct       *text_finfo;
    u0632_gui_draw_t   draw;

    int                wid;
    int                hei;

    Widget             hgrm;

    /* Param-passing-duplets */
    pzframe_gui_dpl_t  M_prm;
    pzframe_gui_dpl_t  P_prm;
    pzframe_gui_dpl_t  x_prm;
} u0632_gui_t;

static inline u0632_gui_t *pzframe2u0632_gui(pzframe_gui_t *pzframe_gui)
{
    return (u0632_gui_t *)(pzframe_gui); /*!!! Should use OFFSETOF() -- pzframe_gui-OFFSETOF(u0632_gui_t, g) */
}

//////////////////////////////////////////////////////////////////////

pzframe_gui_dscr_t *u0632_get_gui_dscr(void);

//////////////////////////////////////////////////////////////////////

void  U0632GuiInit   (u0632_gui_t *gui);
int   U0632GuiRealize(u0632_gui_t *gui, Widget parent,
                      const char *srvrspec, int bigc_n,
                      cda_serverid_t def_regsid, int base_chan);


#endif /* __U0632_GUI_H */
