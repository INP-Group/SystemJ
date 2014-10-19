#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include "cda.h"
#include "Cdr.h"
#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Xh_globals.h"
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


#define def_app_name  "cdmclient"
#define def_app_class "CDMclient"
#define def_app_title "Charge Distribution Monitor"

static groupunit_t cdmclient_grouping[] =
{
    {
        &(elemnet_t){
            "image", "Image",
            NULL, NULL,
            ELEM_IMAGE, 0, 0,
            (logchannet_t []){
            },
            ""
        }, 0
    },
    {NULL}
};


DEFINE_SUBSYS_DESCR(cdmclient, "",
                    def_app_name, def_app_class,
                    def_app_title, NULL,
                    NULL, 0, cdmclient_grouping,
                    "cdmclient", "");

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

static actdescr_t minitoolslist[] =
{
    XhXXX_TOOLCMD   (cmNfLoadPic,     "Load picture",            film_xpm),
    XhXXX_TOOLCMD   (cmNfSavePic,     "Save picture",            camera_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD   (cmChlQuit,       "Quit the program",        btn_quit_xpm),
    XhXXX_END()
};


static void CommandProc(XhWindow win, int cmd)
{
    switch (cmd)
    {

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



char myfilename[] = __FILE__;

static char *fallbacks[] = {XH_STANDARD_FALLBACKS};

int main(int argc, char *argv[])
{
  char          *p;
  char          *subsys = NULL;
  char          *srvref = NULL;
  char          *picref = NULL;
  int            argn;

  void          *handle;
  subsysdescr_t *info;
  char          *err;

    /* Find out how we are called */
    p = strrchr(myfilename, '.');
    if (p != NULL) *p = '\0';
    p = strrchr(argv[0], '/');
    if (p != NULL) p++; else p = argv[0];

    if (strcmp(p, myfilename) == 0)
    {
        /* Directly! */
        
        XhInitApplication(&argc, argv, def_app_name, def_app_class, fallbacks);
        if (argc < 2)
        {
            fprintf(stderr, "Usage: %s FILENAME\n", argv[0]);
            exit(1);
        }
        picref = argv[1];

        info = &cdmclient_subsysdescr;
    }
    else
    {
        /* Via symlink */
        
        subsys = p;
        if (CdrOpenDescription(subsys, argv[0], &handle, &info, &err) != 0)
        {
            fprintf(stderr, "%s: OpenDescription(): %s\n", subsys, err);
            exit(1);
        }

        XhInitApplication(&argc, argv, info->app_name, info->app_class, fallbacks);
        
        for (argn = 1;  argn < argc;  argn++)
        {
            if (strchr(argv[argn], ':') != NULL)
            {
                /* A server-reference */
                if (srvref != NULL)
                {
                    fprintf(stderr, "%s: duplicate server-reference (\"%s\" after \"%s\")\n",
                            argv[0], argv[argn], srvref);
                    exit(1);
                }
                srvref = argv[argn];
            }
            else
            {
                /* A file-reference */
                if (picref != NULL)
                {
                    fprintf(stderr, "%s: duplicate file-request (\"%s\" after \"%s\")\n",
                            argv[0], argv[argn], picref);
                    exit(1);
                }
                picref = argv[argn];
            }
        }
    }

    if (picref != NULL)
    {
        ///
    }

    {
      XhWindow      win;
      int           toolbar_mask    = XhWindowToolbarMask;
      int           statusline_mask = XhWindowStatuslineMask;
      int           compact_mask    = 0;
      
        win = XhCreateWindow(info->win_title, info->app_name, info->app_class,
                             toolbar_mask | statusline_mask | compact_mask | XhWindowUnresizableMask*1,
                             NULL,
                             subsys != NULL? toolslist : minitoolslist);
        XhSetWindowCmdProc(win, CommandProc);

        ChlSetSysname(win, info->app_name);

        if (srvref == NULL) srvref = info->defserver;
        if (info->phys_info_count < 0)
            cda_TMP_register_physinfo_dbase((physinfodb_rec_t *)(info->phys_info));
        if (ChlSetServer(win, srvref) < 0)
        {
            fprintf(stderr,
                    "%s: ChlSetServer(%s): %s\n", argv[0], srvref, cx_strerror(errno));
            exit(1);
        }
        if (info->phys_info_count > 0)
            ChlSetPhysInfo  (win, info->phys_info, info->phys_info_count);

        if (ChlPopulateWorkspace(win, info->grouping, 0, -1, -1))
        {
            fprintf(stderr, "%s: %s\n", argv[0], ChlLastErr());
            exit(1);
        }

        ChlSetDataArrivedCallback(win, NewDataProc, NULL);

        XhShowWindow(win);
        XhRunApplication();
    }
    
    return 0;
}
