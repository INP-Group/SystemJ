#ifndef __KNOBS_TYPESP_H
#define __KNOBS_TYPESP_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <sys/time.h>

#include "cx.h"
#include "Xh_types.h"
#include "Knobs_types.h"


enum
{
    KNOBS_RANGE_NORM = 0,
    KNOBS_RANGE_YELW,
    KNOBS_RANGE_DISP
};

enum
{
    KNOB_B_IS_BUTTON      = 1 << 0,
    KNOB_B_IGN_OTHEROP    = 1 << 1,
    KNOB_B_IS_LIGHT       = 1 << 2,
    KNOB_B_IS_ALARM       = 1 << 3,

    KNOB_B_IS_GROUPABLE   = 1 << 4,
    
    KNOB_B_NO_WASJUSTSET  = 1 << 5,
    KNOB_B_INCDECSTEP_FXD = 1 << 6, /* Change of "incdec_step" is forbidden */

    KNOB_B_HALIGN_MASK    = 7 << 16,
    KNOB_B_HALIGN_DEFAULT = 0 << 16,
    KNOB_B_HALIGN_LEFT    = 1 << 16,
    KNOB_B_HALIGN_RIGHT   = 2 << 16,
    KNOB_B_HALIGN_CENTER  = 3 << 16,
    KNOB_B_HALIGN_FILL    = 4 << 16,

    KNOB_B_VALIGN_MASK    = 7 << 19,
    KNOB_B_VALIGN_DEFAULT = 0 << 19,
    KNOB_B_VALIGN_TOP     = 1 << 19,
    KNOB_B_VALIGN_BOTTOM  = 2 << 19,
    KNOB_B_VALIGN_CENTER  = 3 << 19,
    KNOB_B_VALIGN_FILL    = 4 << 19,
};


typedef struct _knobinfo_t_struct
{
    /* Object linkage */
    struct _knobs_vmt_t_struct *vmtlink;
    struct _eleminfo_t_struct  *uplink;

    /* Widget-related */
    XhWidget    indicator;
    void       *widget_private;

    struct
    {
        XhPixel  deffg;
        XhPixel  defbg;
    }           wdci;
    
    /* General properties */
    int         type;
    int         is_rw;
    int         look;
    int         kind;
    int         color;

    int         behaviour;
    
    /* Text properties */
    char       *ident;
    char       *label;
    char       *units;
    char        dpyfmt[16];
    char       *widdepinfo;
    char       *comment;
    char       *style;
    char       *placement;

    /* Common data access props */
    int         physhandle;
    void       *formula;
    void       *revformula;
    void       *colformula;
    
    /* Informational/display properties */
    double      defnorm;  /* Default value */
    double      rmins[3]; /* Lower] bounds for                */
    double      rmaxs[3]; /* Upper] normal/poor/display ranges*/
    int8        rchg [3]; /* Which ranges were changed by user */
    
    colalarm_t  colstate; /* Current normal/yellow/red state */
    
    /* Numeric-behaviour properties */
    double      curv;
    double      cur_cfv;
    double      q;
    double      userval;
    int         userval_valid;
    int32       curv_raw;
    int         curv_raw_useful;
    tag_t       curtag;
    rflags_t    currflags;

    double      incdec_step;
    double      grpcoeff;
    int8        grpcoeffchg;
    
    time_t      usertime;      /* time() of the last user activity */
    int         wasjustset;

    time_t      attn_endtime;  /* time() when the relaxation or other "attention" should end, or 0 */
    colalarm_t  attn_colstate; /* Color state which started at attn_time */

    double      undo_val;         // Value before last manual change
    double      undo_val_saved;   // ...was this value ever saved
    
    /* General behaviour... */
    int8        is_ingroup;
    
    /* For LOGT_DEVN only */
    int         notnew;       /* Is used to initialize avg,avg2 */
    double      avg;
    double      avg2;
    
    /* For LOGT_MINMAX only */
    double     *minmaxbuf;
    int         minmaxbufcap;
    int         minmaxbufused;

    /* For LOGT_SUBELEM only */
    struct _eleminfo_t_struct  *subelem;
    
    /* For Simple-knobs */
    void       *privptr1; /*!!! Note: C standard says that function-*s may be size-incompatible with void-*s! */
    void       *privptr2;

    /* History */
    double     *histring;         // History ring buffer
    int         histring_size;    // Ring buffer size
    int         histring_start;   // The oldest point
    int         histring_used;    // Number of points in the buffer
    int         histring_frqdiv;  //
    int         histring_frqdivchg;
    int         histring_ctr;     //
    struct _knobinfo_t_struct *hist_next;
} knobinfo_t;

typedef struct _eleminfo_t_struct
{
    /* Object linkage */
    struct _knobs_emethods_t_struct *emlink;
    struct _eleminfo_t_struct       *uplink;
    void                            *grouplist_link;
    
    /* Widget-related */
    XhWidget    container;
    XhWidget    ttl_cont;
    XhWidget    caption;
    XhWidget    toolbar;
    XhWidget    attitle;
    XhWidget    hline;
    XhWidget    innage;
    XhWidget    folded;
    
    struct
    {
        XhPixel  defbg;
    }           wdci;

    void       *elem_private;
    
    /* Behaviour */
    int         alarm_on;
    int         alarm_acked;
    int         alarm_flipflop;
    
    /* Regular properties */
    char       *ident;
    char       *label;
    char       *colnames;
    char       *rownames;
    int         type;
    int         count;
    int         ncols;
    char       *options;
    char       *style;

    char       *defserver;

    /*  */
    Knob        content;
    
    rflags_t    currflags;

    uint8      *conns_u;
} eleminfo_t;


static inline int IdentifierIsSensible(const char *ident)
{
    return ident != NULL  &&  ident[0] != '\0'  &&  ident[0] != '-';
}


/**** OOP-like functionality... ****/
typedef XhWidget (*_k_create_f)  (knobinfo_t *ki, XhWidget parent);
typedef void     (*_k_destroy_f) (knobinfo_t *ki);
typedef void     (*_k_setvalue_f)(knobinfo_t *ki, double v);
typedef void     (*_k_colorize_f)(knobinfo_t *ki, colalarm_t newstate);
typedef void     (*_k_propschg_f)(knobinfo_t *ki, knobinfo_t *old_ki);

typedef struct _knobs_vmt_t_struct
{
    _k_create_f    Create;
    _k_destroy_f   Destroy;
    _k_setvalue_f  SetValue;
    _k_colorize_f  Colorize;
    _k_propschg_f  PropsChg;
} knobs_vmt_t;


typedef void (*_k_setknob_f)    (knobinfo_t *ki, double v);
typedef void (*_k_esetalarm_f)  (eleminfo_t *ei, int onoff);
typedef void (*_k_eackalarm_f)  (eleminfo_t *ei);
typedef void (*_k_showprops_f)  (knobinfo_t *ki);
typedef void (*_k_showbigval_f) (knobinfo_t *ki);
typedef void (*_k_enewdata_f)   (eleminfo_t *ei, int synthetic);
typedef void (*_k_tohistplot_f) (knobinfo_t *ki);

typedef struct _knobs_emethods_t_struct
{
    _k_setknob_f     SetPhysValue;
    _k_esetalarm_f   ShowAlarm;
    _k_eackalarm_f   AckAlarm;
    _k_showprops_f   ShowProps;
    _k_showbigval_f  ShowBigVal;
    _k_enewdata_f    NewData;
    _k_tohistplot_f  ToHistPlot;
} knobs_emethods_t;

/**** ...and its extendability ****/
typedef struct
{
    int          look_id;
    const char  *name;
    knobs_vmt_t *vmt;
} knobs_widgetinfo_t;

typedef struct _knobs_widgetset_t_struct
{
    knobs_widgetinfo_t               *list;
    struct _knobs_widgetset_t_struct *next;
} knobs_widgetset_t;

void KnobsRegisterWidgetset(knobs_widgetset_t *widgetset);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __KNOBS_TYPESP_H */
