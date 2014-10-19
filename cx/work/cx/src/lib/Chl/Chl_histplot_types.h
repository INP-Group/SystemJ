#ifndef __CHL_HISTPLOT_TYPES_H
#define __CHL_HISTPLOT_TYPES_H


enum {MAX_HISTPLOT_LINES_PER_BOX = XH_NUM_DISTINCT_LINE_COLORS * 2};

typedef struct
{
    int          initialized;
    int          def_histring_size;
    struct timeval timeupd;
    XhWidget     box;

    XhWidget     axis;
    XhWidget     graph;
    XhWidget     horzbar;
    XhWidget     w_l;
    XhWidget     w_r;

    int          fixed;
    
    int          rows_used;
    XhWidget     grid;
    XhWidget     label     [MAX_HISTPLOT_LINES_PER_BOX];
    XhWidget     val_dpy   [MAX_HISTPLOT_LINES_PER_BOX];
    XhWidget     b_up      [MAX_HISTPLOT_LINES_PER_BOX];
    XhWidget     b_dn      [MAX_HISTPLOT_LINES_PER_BOX];
    XhWidget     b_rm      [MAX_HISTPLOT_LINES_PER_BOX];
    Knob         target    [MAX_HISTPLOT_LINES_PER_BOX];
    colalarm_t   colstate_s[MAX_HISTPLOT_LINES_PER_BOX];
    int          show      [MAX_HISTPLOT_LINES_PER_BOX];
    XhPixel      deffg;
    XhPixel      defbg;

    GC           bkgdGC;
    GC           axisGC;
    GC           gridGC;
    GC           chanGC [2][XH_NUM_DISTINCT_LINE_COLORS];
    XFontStruct *axis_finfo;
    XFontStruct *chan_finfo[XH_NUM_DISTINCT_LINE_COLORS];
    int          mode;
    int          x_scale;
    int          prev_cyclesize;
    
    int          m_lft;
    int          m_rgt;
    int          m_top;
    int          m_bot;

    int          ignore_axis_expose;
    int          ignore_graph_expose;

    int          horz_maximum;
    int          horz_offset;
    int          o_horzbar_maximum;
    int          o_horzbar_slidersize;
} histplotdlg_t;


#endif /* __CHL_HISTPLOT_TYPES_H */
