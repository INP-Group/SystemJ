#ifndef __XH_TYPESP_H
#define __XH_TYPESP_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "Xh.h"


typedef struct _WindowInfo {
    struct _WindowInfo *next;

    Widget  shell;        /* Application shell of window */
    Widget  mainForm;     /* A form, containing everything in a window */
    Widget  workSpace;    /* Container for elements */
    
    Widget  menuBar;
    
    Widget  toolBar;      /* Toolbar form */
    Widget  toolHolder;   /* parent of tool-buttons */
    Widget  alarmLeds;    /*  */
    
    Widget  statsForm;    /* Statusline form */
    Widget  statsLine;    /* an XmLabel itself */
    char    mainmessage[256];
    char    tempmessage[256];

    Widget  clue;
    
    Widget  hilites[4];
    int     hilite_shown;
    
    void   *commandproc;
    int     sendcmdonclose;

    void   *higher_private;
    void   *higher_priv2;
} zzz_unused_WindowInfo;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __XH_TYPESP_H */
