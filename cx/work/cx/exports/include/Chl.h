#ifndef __CHL_H
#define __CHL_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <unistd.h>
#include <time.h>

#include "cxlib.h"
#include "cxdata.h"
#include "cda.h" /*!!!?*/
#include "Xh.h"
#include "Cdr.h"


#define CHL_MODE_PATTERN_FMT "mode_%s_*.dat"
#define CHL_RNGS_PATTERN_FMT "mode_%s_ranges.dat"
#define CHL_DATA_PATTERN_FMT "data_%s_*.dat"
#define CHL_PLOT_PATTERN_FMT "plot_%s_*.dat"
#define CHL_LOG_PATTERN_FMT  "log_%s.dat"
#define CHL_OPS_PATTERN_FMT  "events_%s.log"


typedef void (*ChlDataCallback_t)(XhWindow window, void *privptr, int synthetic);


/* Emulation of future plugin-based architecture */

typedef int (*_e_create_f) (XhWidget parent, ElemInfo e);

typedef struct
{
    int          look_id;
    const char  *name;
    _e_create_f  create;
} chl_elemdescr_t;

void ChlSetCustomElementsTable(chl_elemdescr_t *table);

void Chl_E_SetPhysValue_m  (Knob ki, double v);
void Chl_E_ShowAlarm_m     (ElemInfo ei, int onoff);
void Chl_E_AckAlarm_m      (ElemInfo ei);
void Chl_E_ShowPropsWindow (Knob ki);
void Chl_E_ShowBigValWindow(Knob ki);
void Chl_E_NewData_m       (ElemInfo ei, int synthetic);
void Chl_E_ToHistPlot      (Knob ki);


enum
{
    CHL_TITLE_SIDE_TOP    = 0,
    CHL_TITLE_SIDE_BOTTOM = 1,
    CHL_TITLE_SIDE_LEFT   = 2,
    CHL_TITLE_SIDE_RIGHT  = 3,
};

enum
{
    CHL_CF_NOSHADOW = 1 << 0,
    CHL_CF_NOTITLE  = 1 << 1,
    CHL_CF_NOHLINE  = 1 << 2,
    CHL_CF_NOFOLD   = 1 << 3,
    CHL_CF_FOLDED   = 1 << 4,
    CHL_CF_FOLD_H   = 1 << 5,
    CHL_CF_SEPFOLD  = 1 << 6,
    CHL_CF_TOOLBAR  = 1 << 7,
    CHL_CF_ATTITLE  = 1 << 8,
    CHL_CF_TITLE_SIDE_shift = 9,
    CHL_CF_TITLE_SIDE_MASK  = 3 << CHL_CF_TITLE_SIDE_shift,
    CHL_CF_TITLE_ATTOP      = CHL_TITLE_SIDE_TOP    << CHL_CF_TITLE_SIDE_shift,
    CHL_CF_TITLE_ATBOTTOM   = CHL_TITLE_SIDE_BOTTOM << CHL_CF_TITLE_SIDE_shift,
    CHL_CF_TITLE_ATLEFT     = CHL_TITLE_SIDE_LEFT   << CHL_CF_TITLE_SIDE_shift,
    CHL_CF_TITLE_ATRIGHT    = CHL_TITLE_SIDE_RIGHT  << CHL_CF_TITLE_SIDE_shift,
};

typedef int (*chl_cpopulator_t)(ElemInfo e, void *privptr);
int  ChlCreateContainer(XhWidget parent, ElemInfo e,
                        int flags,
                        chl_cpopulator_t populator, void *privptr);
ElemInfo  ChlCreateSimpleContainer(XhWidget parent,
                                   const char *spec,
                                   chl_cpopulator_t populator, void *privptr);

void ChlPopulateContainerAttitle(ElemInfo e, int nattl, int title_side);

int  ChlGiveBirthToKnob  (Knob k);
int  ChlGiveBirthToKnobAt(Knob k, XhWidget parent);
int  ChlMakeElement(XhWidget parent, ElemInfo  e);


int  ChlLayOutGrouping(XhWidget parent_form, groupelem_t *grouplist,
                       int is_vertical, int hspc, int vspc);
int  ChlPopulateWorkspace(XhWindow window, groupunit_t *grouping,
                          int is_vertical, int hspc, int vspc);
int  ChlAddLeds(XhWindow window);


/*#### Application-level functionality #############################*/

typedef void (*chl_floader_t)(XhWindow window, const char *filename, void *privptr);

int ChlRunApp(int argc, char *argv[],
              const char *app_name,  const char  *app_class,
              const char *title,     const char **icon,
              actdescr_t *toolslist, XhCommandProc  CommandProc,
              const char *options,   const char **fallbacks,
              const char  *defserver, groupunit_t *grouping,
              physprops_t *phys_info, int  phys_info_count,
              ChlDataCallback_t newdataproc, void *privptr);

int ChlRunMulticlientApp(int argc, char *argv[],
                         const char *main_filename_c,
                         actdescr_t *toolslist, XhCommandProc  CommandProc,
                         const char **fallbacks,
                         chl_floader_t     floadproc,
                         ChlDataCallback_t newdataproc, void *privptr);

int    ChlSaveWindowMode(XhWindow window, const char *path, const char *comment);
int    ChlLoadWindowMode(XhWindow window, const char *path, int options);
int    ChlStatWindowMode(const char *path,
                         time_t *cr_time,
                         char   *commentbuf, size_t commentbuf_size);
int    ChlLogWindowMode (XhWindow window, const char *path,
                         const char *comment,
                         int headers, int append,
                         struct timeval *start);
void   ChlStartLogging  (XhWindow window, const char *filename, int period_ms);
void   ChlStopLogging   (XhWindow window);
void   ChlSwitchLogging (XhWindow window, int period_ms);

void   ChlRecordEvent   (XhWindow window, const char *format, ...)
                        __attribute__((format (printf, 2, 3)));

char  *ChlLastErr(void);

//// Standard commands

enum
{
    cmChlNfLoadMode   = 1000,  cmChlLoad_FIRST = cmChlNfLoadMode,
    cmChlNfLoadOk     = 1001,
    cmChlNfLoadCancel = 1002,
    cmChlLoadMode     = 1003,
    cmChlLoadOk       = 1004,
    cmChlLoadCancel   = 1005,  cmChlLoad_LAST  = cmChlLoadCancel,
    
    cmChlNfSaveMode   = 1010,  cmChlSave_FIRST = cmChlNfSaveMode,
    cmChlNfSaveOk     = 1011,
    cmChlNfSaveCancel = 1012,
    cmChlSaveMode     = 1013,
    cmChlSaveOk       = 1014,
    cmChlSaveCancel   = 1015,  cmChlSave_LAST  = cmChlSaveCancel,

    cmChlSwitchLog    = 1020,
    cmChlHelp         = 1021,
    cmChlQuit         = 1022,
    cmChlFreeze       = 1023,

    cmChlDoElog       = 1030,
    cmChlDoElogOk     = 1031,
    cmChlDoElogCancel = 1032,
};

int ChlHandleStdCommand(XhWindow window, int cmd);
actdescr_t *ChlGetStdToolslist(void);


//// Help

enum
{
    CHL_HELP_COLORS = 1 << 0,
    CHL_HELP_KEYS   = 1 << 1,
    CHL_HELP_MOUSE  = 1 << 2,
    CHL_HELP_ALL    = ~0
};
void   ChlShowHelp      (XhWindow window, int parts);

typedef enum
{
    CHL_HELP_PART_COLORS,
    CHL_HELP_PART_KEYS,
    CHL_HELP_PART_TEXT,
} chl_help_kind_t;

typedef struct
{
    const char      *label;
    chl_help_kind_t  kind;
    void            *data;
} chl_helppart_t;

void   ChlShowHelpWindow(XhWindow window, chl_helppart_t *parts, int nparts);


/*#### Public data API #############################################*/

void ChlSetSysname   (XhWindow window, const char *sysname);
int  ChlSetServer    (XhWindow window, const char *spec);
void ChlSetPhysInfo  (XhWindow window, physprops_t *info, int count);
int  ChlSetGrouplist (XhWindow window, groupelem_t *grouplist);
int  ChlStart        (XhWindow window);
void ChlFreeze       (XhWindow window, int freeze);

int  ChlSetDataArrivedCallback(XhWindow window, ChlDataCallback_t proc, void *privptr);
cda_serverid_t ChlGetMainsid(XhWindow window);
int  ChlGetChanVal(XhWindow window, const char *chanspec, double *v_p);
int  ChlSetChanVal(XhWindow window, const char *chanspec, double  v);
Knob ChlFindKnob  (XhWindow window, const char *chanspec);
Knob ChlFindKnobFrom(XhWindow window, const char *chanspec,
                     ElemInfo start_point);

int  ChlSetLclReg (XhWindow window, int n, double  v);
int  ChlGetLclReg (XhWindow window, int n, double *v_p);

const char  *ChlGetSysname  (XhWindow window);
groupelem_t *ChlGetGrouplist(XhWindow window);

int          ChlIsReadonly  (XhWindow window);


/*#### Leds-related functionality ##################################*/

typedef struct
{
    XhWidget  w;
    int       status;
} ledinfo_t;

int leds_create    (XhWidget parent_grid, int size,
                    ledinfo_t *leds, int count,
                    cda_serverid_t mainsid);
int leds_update    (ledinfo_t *leds, int count,
                    cda_serverid_t mainsid);
int leds_set_status(ledinfo_t *led, int status);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CHL_H */
