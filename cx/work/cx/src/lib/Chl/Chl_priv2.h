#ifndef __CHL_PRIV2_H
#define __CHL_PRIV2_H


#include "Chl_knobprops_types.h"
#include "Chl_bigvals_types.h"
#include "Chl_histplot_types.h"
#include "Chl_help_types.h"
#include "Chl_elog_types.h"


/**** higher_private/datainfo stuff *********************************/

enum {NUMLOCALREGS = 1000};

typedef struct
{
    cda_serverid_t  mainsid;

    int             srvcount;
    ledinfo_t      *leds;

    groupelem_t    *grouplist;
    
    double          localregs      [NUMLOCALREGS];
    char            localregsinited[NUMLOCALREGS];

    int             is_freezed;

    int             is_readonly;
    int             is_protected;

    ChlDataCallback_t  user_cb;
    void              *user_ptr;
} datainfo_t;

datainfo_t *DataInfoOf(XhWindow window);


/**** Priv2 stuff ***************************************************/

typedef struct
{
    char           *filename;
    XtIntervalId    timeout_id;
    int             period_ms; /* Note: we use negative value on to signal 1st logging */
    int             defperiod_ms;
    struct timeval  start_tv;
} winlogrec_t;

typedef struct
{
    XhWidget load_dialog;
    XhWidget save_dialog;
    XhWidget load_nfndlg;
    XhWidget save_nfndlg;
} stddlgsrec_t;

typedef struct
{
    char          *sysname;
    winlogrec_t    li;
    kpropsdlg_t    kp;
    bigvalsdlg_t   bv;
    histplotdlg_t  hp;
    helprec_t      hr;
    elogrec_t      el;
    stddlgsrec_t   sd;
} priv2rec_t;

priv2rec_t *Priv2Of(XhWindow win);


#endif /* __CHL_PRIV2_H */
