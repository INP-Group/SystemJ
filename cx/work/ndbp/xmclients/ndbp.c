#define GEESE 0

#if GEESE
  #define XH_FONT_PROPORTIONAL_FMLY "new century schoolbook"
  #define XH_FONT_FIXED_FMLY        "new century schoolbook"
  #define XH_FONT_MEDIUM_WGHT       "bold"
  #define XH_FONT_BASESIZE "14"
  #define XH_FONT_TINYSIZE "12"
#endif

#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>

#include "misclib.h"

#include "Xh.h"
#include "Chl.h"

#include "pixmaps/btn_open.xpm"
#include "pixmaps/btn_save.xpm"
#include "pixmaps/btn_start.xpm"
#include "pixmaps/btn_once.xpm"
#include "pixmaps/btn_quit.xpm"

#include "pixmaps/bobbin.xpm"
#include "pixmaps/camera.xpm"
#include "pixmaps/film.xpm"

#include "ndbp_elemlist.h"
#include "ndbp_image_elemplugin.h"
#include "ndbp_nadc200_elemplugin.h"


//////////////////////////////////////////////////////////////////////

#define __NDBP_C
#include "ndbp.h"

typedef struct
{
    int   running;
    int   oneshot;
} show_t;

static show_t show;

//////////////////////////////////////////////////////////////////////

#define CDM_PIC_PATTERN_FMT "cdmpic_%s_*.dat"

chl_elemdescr_t custom_table[] =
{
    {ELEM_IMAGE,   "ndbp_image",   ELEM_IMAGE_Create_m},
    {ELEM_NADC200, "ndbp_nadc200", ELEM_NADC200_Create_m},
    {0},
};


/*####  ####*/

enum
{
    cmNfLoadPic       = 10,
    cmNfLoadPicOk     = 11,
    cmNfLoadPicCancel = 12,

    cmNfSavePic       = 20,
    cmNfSavePicOk     = 21,
    cmNfSavePicCancel = 22,
    
    cmLoop = 50,
    cmOnce = 52,
};

static actdescr_t toolslist[] =
{
    XhXXX_TOOLCMD   (cmNfLoadPic,     "Load picture",            film_xpm),
    XhXXX_TOOLCMD   (cmNfSavePic,     "Save picture",            camera_xpm),
    XhXXX_TOOLCHK   (cmLoop,          "Periodic measurement",    btn_start_xpm),
    XhXXX_TOOLCMD   (cmOnce,          "One measurement",         btn_once_xpm),
    XhXXX_TOOLLEDS  (),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD   (cmChlNfLoadMode, "Read settings from file", btn_open_xpm),
    XhXXX_TOOLCMD   (cmChlNfSaveMode, "Save settings to file",   btn_save_xpm),
    XhXXX_SEPARATOR (),
    XhXXX_TOOLCMD   (cmChlQuit,       "Quit the program",        btn_quit_xpm),
    XhXXX_END()
};

static void SetRunMode(XhWindow win, int is_loop)
{
    show.running = is_loop;
    show.oneshot = 0;
    XhSetCommandOnOff  (win, cmLoop, show.running);
    XhSetCommandEnabled(win, cmOnce, !show.running);
}

static void CommandProc(XhWindow win, int cmd)
{
  char             filename[PATH_MAX];
  char             comment[400];
    
  static XhWidget  pic_load_nfndlg;
  static XhWidget  pic_save_nfndlg;
    
    switch (cmd)
    {
        case cmNfLoadPic:
            if (LoadPicFilter == NULL) return;
            check_snprintf(filename, sizeof(filename), CDM_PIC_PATTERN_FMT, ChlGetSysname(win));
            if (pic_load_nfndlg == NULL)
                pic_load_nfndlg = XhCreateLoadDlg(win, "loadModeDialog", NULL, NULL, LoadPicFilter, NULL, filename, cmNfLoadPicOk, -1);
            XhLoadDlgShow(pic_load_nfndlg);
            break;

        case cmNfLoadPicOk:
        case cmNfLoadPicCancel:
            XhLoadDlgHide(pic_load_nfndlg);
            if (cmd == cmNfLoadPicOk  &&  DoLoadPic != NULL)
            {
                XhLoadDlgGetFilename(pic_load_nfndlg, filename, sizeof(filename));
                if (filename[0] != '\0')
                    DoLoadPic(filename);
                if (show.running) CommandProc(win, cmLoop);
            }
            break;
            
        case cmNfSavePic:
            if (pic_save_nfndlg == NULL)
                pic_save_nfndlg = XhCreateSaveDlg(win, "saveModeDialog", NULL, NULL, cmNfSavePicOk);
            XhSaveDlgShow(pic_save_nfndlg);
            break;

        case cmNfSavePicOk:
        case cmNfSavePicCancel:
            XhSaveDlgHide(pic_save_nfndlg);
            if (cmd == cmNfSavePicOk  &&  DoSavePic != NULL)
            {
                XhSaveDlgGetComment(pic_save_nfndlg, comment, sizeof(comment));
                check_snprintf(filename, sizeof(filename), CDM_PIC_PATTERN_FMT, ChlGetSysname(win));
                XhFindFilename(filename, filename, sizeof(filename));
                DoSavePic(filename, comment, ChlGetSysname(win));
            }
            break;
            
        case cmLoop:
            SetRunMode(win, !show.running);
            if (runnonce_proc != NULL)
                runnonce_proc(show.running? RUNNONCE_RUN : RUNNONCE_STOP, runnonce_privptr);
            break;
        
        case cmOnce:
            show.oneshot = 1;
            if (runnonce_proc != NULL)
                runnonce_proc(RUNNONCE_ONCE, runnonce_privptr);
            break;
        
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


static void NewDataProc(XhWindow  win     __attribute__((unused)),
                        void     *privptr __attribute__((unused)),
                        int       synthetic)
{
    if (synthetic) return;

}


int main(int argc, char *argv[])
{
  int            r;

    ChlSetCustomElementsTable(custom_table);

#if GEESE
    XhSetColorBinding("BG_DEFAULT",     "#5f9ba0");
    XhSetColorBinding("TS_DEFAULT",     "#b7d2d5");
    XhSetColorBinding("BS_DEFAULT",     "#2f4d50");
    XhSetColorBinding("BG_ARMED",       "#518488");
    XhSetColorBinding("BG_TOOL_ARMED",  "#f0cc80");
    XhSetColorBinding("BG_DATA_INPUT",  "#bfbfbf");
    XhSetColorBinding("CTL3D_BG",       "#5f9ba0");
    XhSetColorBinding("CTL3D_TS",       "#b7d2d5");
    XhSetColorBinding("CTL3D_BS",       "#2f4d50");
    XhSetColorBinding("CTL3D_BG_ARMED", "#518488");
    XhSetColorBinding("GRAPH_BG",       "#b3b3b3");
    XhSetColorBinding("GRAPH_AXIS",     "#000000");
    XhSetColorBinding("GRAPH_GRID",     "#000000");
    XhSetColorBinding("GRAPH_LINE1",    "#0000cd");
    XhSetColorBinding("GRAPH_LINE2",    "#d02090");
#endif
    XhSetColorBinding("GRAPH_REPERS", "#00FF00");
//    XhSetColorBinding("GRAPH_REPERS", "#87ceff");
    XhSetColorBinding("GRAPH_LINE3",  "#87ceff");
    XhSetColorBinding("GRAPH_LINE4",  "#FFC000");

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             toolslist, CommandProc,
                             NULL,
                             NULL,
                             NewDataProc, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
