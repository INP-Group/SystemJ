#ifndef __CHL_BIGVALS_TYPES_H
#define __CHL_BIGVALS_TYPES_H


enum {MAX_BIGVALS = 10*0+5};

typedef struct
{
    int         initialized;
    XhWidget    box;
    
    int         rows_used;
    XhWidget    grid;
    XhWidget    label     [MAX_BIGVALS];
    XhWidget    val_dpy   [MAX_BIGVALS];
    XhWidget    b_up      [MAX_BIGVALS];
    XhWidget    b_dn      [MAX_BIGVALS];
    XhWidget    b_rm      [MAX_BIGVALS];
    Knob        target    [MAX_BIGVALS];
    colalarm_t  colstate_s[MAX_BIGVALS];
    XhPixel     deffg;
    XhPixel     defbg;
} bigvalsdlg_t;


#endif /* __CHL_BIGVALS_TYPES_H */
