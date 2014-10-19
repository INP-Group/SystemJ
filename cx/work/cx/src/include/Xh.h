#ifndef __XH_H
#define __XH_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "Xh_types.h"


/*
 *  Utilities
 */

CxWidget XhTopmost(CxWidget w);

int  XhAssignPixmap        (CxWidget w, char **pix);
int  XhAssignPixmapFromFile(CxWidget w, char *filename);
void XhInvertButton        (CxWidget w);

void XhAssignVertLabel(CxWidget w, const char *text, int alignment);

void XhSwitchContentFoldedness(CxWidget parent);

void XhENTER_FAILABLE(void);
int  XhLEAVE_FAILABLE(void);

void XhProcessPendingXEvents(void);
int  XhCompressConfigureEvents  (CxWidget w);
void XhRemoveExposeEvents       (CxWidget w);
void XhAdjustPreferredSizeInForm(CxWidget w);

    
enum {
    XhACT_END = 0,
    XhACT_COMMAND,
    XhACT_CHECKBOX,
    XhACT_SUBMENU,
    XhACT_SEPARATOR,
    XhACT_FILLER,
    XhACT_LEDS,
    XhACT_LABEL,
    XhACT_NOP,
};


typedef struct {
    int          type;    /* XhACT_XXX */
    int          cmd;     /* Command to pass to handler */
    const char  *label;   /* Text for menu */
    const char  *tip;     /* Short help string */
    char       **pixmap;  /* Image */
    const char  *hotkey;  /* "<Key><osfRight> */
} actdescr_t;

#define XhXXX_TOOLCMD(cmd, tip, pixmap) {XhACT_COMMAND,   cmd, NULL, tip,  pixmap, NULL}
#define XhXXX_TOOLCHK(cmd, tip, pixmap) {XhACT_CHECKBOX,  cmd, NULL, tip,  pixmap, NULL}
#define XhXXX_TOOLCKY(cmd, tip, pixmap, key) {XhACT_COMMAND,   cmd, NULL, tip,  pixmap, key}
#define XhXXX_SEPARATOR()               {XhACT_SEPARATOR, 0,   NULL, NULL, NULL,   NULL}
#define XhXXX_TOOLFILLER()              {XhACT_FILLER,    0,   NULL, NULL, NULL,   NULL}
#define XhXXX_TOOLLEDS()                {XhACT_LEDS,      0,   NULL, NULL, NULL,   NULL}
#define XhXXX_TOOLLABEL(lab)            {XhACT_LABEL,     0,   lab,  NULL, NULL,   NULL}
#define XhXXX_END()                     {XhACT_END,       0,   NULL, NULL, NULL,   NULL}


/*
 *  Windows
 */

enum {
    XhWindowMenuMask          = 1 << 0,
    XhWindowToolbarMask       = 1 << 1,
    XhWindowStatuslineMask    = 1 << 2,
    XhWindowCompactMask       = 1 << 3,

    XhWindowWorkspaceTypeMask = 7 << 4,
    XhWindowFormMask          = 0 << 4,
    XhWindowRowColumnMask     = 1 << 4,
    XhWindowGridMask          = 2 << 4,
    
    XhWindowUnresizableMask   = 1 << 8,
    XhWindowOnTopMask         = 1 << 9,   /*!!! <- Currently not implemented  */
    XhWindowSecondaryMask     = 1 << 10,
};

typedef void (*XhCommandProc)(XhWindow window, int cmd);


XhWindow  XhCreateWindow(const char *title,
                         const char *app_name, const char *app_class,
                         int         contents,
                         actdescr_t *menus,
                         actdescr_t *tools);
void        XhDeleteWindow(XhWindow window);
void        XhShowWindow  (XhWindow window);
void        XhHideWindow  (XhWindow window);

int         XhSetWindowIcon   (XhWindow window, char **pix);
int         XhSetWindowCmdProc(XhWindow window, XhCommandProc proc);
int         XhSetWindowCloseBh(XhWindow window, int sendcmd);
int         XhGetWindowSize   (XhWindow window, int *wid, int *hei);
int         XhSetWindowLimits (XhWindow window,
                               int  min_w, int min_h,
                               int  max_w, int max_h);

XhWindow XhWindowOf(XhWidget w);

void *XhGetHigherPrivate (XhWindow window);
void  XhSetHigherPrivate (XhWindow window, void *ptr);

void *XhGetHigherPrivate2(XhWindow window);
void  XhSetHigherPrivate2(XhWindow window, void *ptr);

XhWidget    XhGetWindowWorkspace(XhWindow window);
XhWidget    XhGetWindowAlarmleds(XhWindow window);
XhWidget    XhGetWindowShell    (XhWindow window);


/* Menus/Toolbars */

void XhCreateToolbar(XhWindow window, actdescr_t  buttons[]);
void XhSetCommandOnOff  (XhWindow window, int cmd, int is_pushed);
void XhSetCommandEnabled(XhWindow window, int cmd, int is_enabled);


/* Statusline */
void XhCreateStatusline(XhWindow window);

void XhMakeMessage     (XhWindow window, const char *format, ...)
                       __attribute__((format (printf, 2, 3)));
void XhVMakeMessage    (XhWindow window, const char *format, va_list ap);
void XhMakeTempMessage (XhWindow window, const char *format, ...)
                       __attribute__((format (printf, 2, 3)));
void XhVMakeTempMessage(XhWindow window, const char *format, va_list ap);


/* Balloon */
void XhSetBalloon(XhWidget w, const char *text);


/* Hilite rectangle */
void XhCreateHilite(XhWindow window);

void XhShowHilite(CxWidget w);
void XhHideHilite(CxWidget w);


/* Grid widget */
XhWidget  XhCreateGrid(XhWidget parent, const char *name);
void      XhGridSetPadding  (XhWidget w, int hpad, int vpad);
void      XhGridSetSpacing  (XhWidget w, int hspc, int vspc);
void      XhGridSetSize     (XhWidget w, int cols, int rows);
void      XhGridSetGrid     (XhWidget w, int horz, int vert);
void      XhGridSetChildPosition (XhWidget w, int  x,   int  y);
void      XhGridGetChildPosition (XhWidget w, int *x_p, int *y_p);
void      XhGridSetChildAlignment(XhWidget w, int  h_a, int  v_a);
void      XhGridSetChildFilling  (XhWidget w, int  h_f, int  v_f);


/* File dialogs */
XhWidget XhCreateFdlg(XhWindow window, const char *name,
                      const char *title, const char *ok_text,
                      const char *pattern, int cmd);
void     XhFdlgShow       (XhWidget  fdlg);
void     XhFdlgHide       (XhWidget  fdlg);
void     XhFdlgGetFilename(XhWidget  fdlg, char *buf, size_t bufsize);


/* Simplified-file-interface dialogs */
XhWidget XhCreateSaveDlg     (XhWindow window, const char *name,
                              const char *title, const char *ok_text,
                              int cmd);
void     XhSaveDlgShow       (XhWidget dlg);
void     XhSaveDlgHide       (XhWidget dlg);
void     XhSaveDlgGetComment (XhWidget dlg, char *buf, size_t bufsize);
void     XhFindFilename      (const char *pattern,
                              char *buf, size_t bufsize);

typedef int (*xh_loaddlg_filter_proc)(const char *fname,
                                      time_t *cr_time,
                                      char *labelbuf, size_t labelbufsize,
                                      void *privptr);

XhWidget XhCreateLoadDlg     (XhWindow window, const char *name,
                              const char *title, const char *ok_text,
                              xh_loaddlg_filter_proc filter, void *privptr,
                              const char *pattern, int cmd, int flst_cmd);
void     XhLoadDlgShow       (XhWidget dlg);
void     XhLoadDlgHide       (XhWidget dlg);
void     XhLoadDlgGetFilename(XhWidget dlg, char *buf, size_t bufsize);


/* "Standard" (messagebox-based) dialogs */
CxWidget XhCreateStdDlg    (XhWindow window, const char *name,
                            const char *title, const char *ok_text,
                            const char *msg, int flags);
void     XhStdDlgSetMessage(CxWidget dlg, const char *msg);
void     XhStdDlgShow      (CxWidget dlg);
void     XhStdDlgHide      (CxWidget dlg);

enum
{
    XhStdDlgFOk          = 1 << 0,
    //XhStdDlgFApply       = 1 << 1,
    XhStdDlgFCancel      = 1 << 2,
    XhStdDlgFHelp        = 1 << 3,
    XhStdDlgFNothing     = 1 << 4,
    XhStdDlgFCenterMsg   = 1 << 5,
    XhStdDlgFNoMargins   = 1 << 6,

    XhStdDlgFModal       = 1 << 8,
    XhStdDlgFNoAutoUnmng = 1 << 9,
    XhStdDlgFNoDefButton = 1 << 10,
    
    XhStdDlgFResizable   = 1 << 16,
    XhStdDlgFZoomable    = 1 << 17,
};


/* Colors */

enum
{
    XH_COLOR_FG_DEFAULT,
    XH_COLOR_BG_DEFAULT,
    XH_COLOR_TS_DEFAULT,
    XH_COLOR_BS_DEFAULT,
    XH_COLOR_BG_ARMED,
    XH_COLOR_BG_TOOL_ARMED,
    XH_COLOR_HIGHLIGHT,
    XH_COLOR_BORDER,

    XH_COLOR_FG_BALLOON,
    XH_COLOR_BG_BALLOON,
    
    XH_COLOR_BG_FDLG_INPUTS,
    
    XH_COLOR_BG_DATA_INPUT,

    XH_COLOR_THERMOMETER,

    XH_COLOR_INGROUP,

    XH_COLOR_CTL3D_BG,
    XH_COLOR_CTL3D_TS,
    XH_COLOR_CTL3D_BS,
    XH_COLOR_CTL3D_BG_ARMED,
    
    XH_COLOR_GRAPH_BG,
    XH_COLOR_GRAPH_AXIS,
    XH_COLOR_GRAPH_GRID,
    XH_COLOR_GRAPH_TITLES,
    XH_COLOR_GRAPH_REPERS,
    XH_COLOR_GRAPH_LINE1,
    XH_COLOR_GRAPH_LINE2,
    XH_COLOR_GRAPH_LINE3,
    XH_COLOR_GRAPH_LINE4,
    XH_COLOR_GRAPH_LINE5,
    XH_COLOR_GRAPH_LINE6,
    XH_COLOR_GRAPH_LINE7,
    XH_COLOR_GRAPH_LINE8,
    XH_COLOR_GRAPH_BAR1,
    XH_COLOR_GRAPH_BAR2,
    XH_COLOR_GRAPH_BAR3,
    XH_COLOR_GRAPH_PREV,
    XH_COLOR_GRAPH_OUTL,
    
    XH_COLOR_BGRAPH_BG,
    XH_COLOR_BGRAPH_AXIS,
    XH_COLOR_BGRAPH_GRID,
    XH_COLOR_BGRAPH_TITLES,
    XH_COLOR_BGRAPH_REPERS,
    XH_COLOR_BGRAPH_LINE1,
    XH_COLOR_BGRAPH_LINE2,
    XH_COLOR_BGRAPH_LINE3,
    XH_COLOR_BGRAPH_LINE4,
    XH_COLOR_BGRAPH_LINE5,
    XH_COLOR_BGRAPH_LINE6,
    XH_COLOR_BGRAPH_LINE7,
    XH_COLOR_BGRAPH_LINE8,
    XH_COLOR_BGRAPH_BAR1,
    XH_COLOR_BGRAPH_BAR2,
    XH_COLOR_BGRAPH_BAR3,
    XH_COLOR_BGRAPH_PREV,
    XH_COLOR_BGRAPH_OUTL,

    XH_COLOR_FG_HILITED,
    XH_COLOR_FG_IMPORTANT,
    XH_COLOR_FG_DIM,
    XH_COLOR_FG_WEIRD,
    XH_COLOR_BG_VIC,
    XH_COLOR_FG_HEADING,
    XH_COLOR_BG_HEADING,
    XH_COLOR_BG_NORM_YELLOW,
    XH_COLOR_BG_NORM_RED,
    XH_COLOR_BG_IMP_YELLOW,
    XH_COLOR_BG_IMP_RED,
    XH_COLOR_BG_WEIRD,
    XH_COLOR_BG_JUSTCREATED,
    XH_COLOR_BG_DEFUNCT,
    XH_COLOR_BG_HWERR,
    XH_COLOR_BG_SFERR,
    XH_COLOR_BG_OTHEROP,
    XH_COLOR_BG_PRGLYCHG,
    XH_COLOR_BG_NOTFOUND,

    XH_COLOR_JUST_RED,
    XH_COLOR_JUST_ORANGE,
    XH_COLOR_JUST_YELLOW,
    XH_COLOR_JUST_GREEN,
    XH_COLOR_JUST_BLUE,
    XH_COLOR_JUST_INDIGO,
    XH_COLOR_JUST_VIOLET,
    
    XH_COLOR_JUST_BLACK,
    XH_COLOR_JUST_WHITE,
    XH_COLOR_JUST_GRAY,

    XH_COLOR_JUST_AMBER,
    XH_COLOR_JUST_BROWN,
};

enum
{
    XH_NUM_DISTINCT_LINE_COLORS = XH_COLOR_GRAPH_LINE8 - XH_COLOR_GRAPH_LINE1 + 1,
    XH_NUM_DISTINCT_BAR_COLORS  = XH_COLOR_GRAPH_BAR3  - XH_COLOR_GRAPH_BAR1  + 1,

    XH_FIRST_RAINBOW_COLOR = XH_COLOR_JUST_RED,
    XH_NUM_RAINBOW_COLORS  = XH_COLOR_JUST_VIOLET - XH_COLOR_JUST_RED + 1,
};

int     XhSetColorBinding(const char *name, const char *defn);
XhPixel XhGetColor       (int n);
XhPixel XhGetColorByName (const char *name);
const char *XhStrColindex(int n);
int     XhColorCount     (void);


/* Application */
void  XhInitApplication(int         *argc,     char **argv,
                        const char  *app_name,
                        const char  *app_class,
                        const char **fallbacks);

void  XhRunApplication(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __XH_H */
