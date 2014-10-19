#ifndef __PZFRAME_MAIN_H
#define __PZFRAME_MAIN_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "pzframe_gui.h"


enum
{
    cmLoop = 50,
    cmOnce = 52,

    cmSetGood = 60,
    cmRstGood = 61,
    cmSetBkgd = 62,
    cmRstBkgd = 63,
};


int  PzframeMain(int argc, char *argv[],
                 const char *def_app_name, const char *def_app_class,
                 pzframe_gui_t *gui, pzframe_gui_dscr_t *gkd,
                 psp_paramdescr_t *table2, void *rec2,
                 psp_paramdescr_t *table4, void *rec4);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PZFRAME_MAIN_H */
