#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>

#include "misclib.h"

#include "cxlib.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Xh_fallbacks.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Chl.h"
#include "ChlI.h"

#include "pzframe_data.h"
#include "pzframe_gui.h"
#include "pzframe_main.h"

#include "pixmaps/btn_open.xpm"
#include "pixmaps/btn_save.xpm"
#include "pixmaps/btn_start.xpm"
#include "pixmaps/btn_once.xpm"
#include "pixmaps/btn_help.xpm"
#include "pixmaps/btn_quit.xpm"


//// ctype extensions ////////////////////////////////////////////////

static inline int isletnum(int c)
{
    return isalnum(c)  ||  c == '_'  ||  c == '-';
}

static int only_letters(const char *s, int count)
{
    for (;  count > 0;  s++, count--)
        if (!isletnum(*s)) return 0;

    return 1;
}

static int only_digits(const char *s)
{
    for (;  *s != '\0';  s++)
        if (!isdigit(*s)) return 0;

    return 1;
}

//////////////////////////////////////////////////////////////////////

static pzframe_gui_t       *global_gui = NULL;
static XhWindow             pz_win;


static void ShowRunMode(void)
{
    XhSetCommandOnOff  (pz_win, cmLoop,  global_gui->pfr->cfg.running);
    //XhSetCommandOnOff  (pz_win, cmOnce,  global_gui->pfr->cfg.oneshot);
    XhSetCommandEnabled(pz_win, cmOnce, !global_gui->pfr->cfg.running);
}

static void SetRunMode (int is_loop)
{
    PzframeDataSetRunMode(global_gui->pfr, is_loop, 0);
    ShowRunMode();
}

//////////////////////////////////////////////////////////////////////

typedef struct
{
    char    app_name [32];
    char    app_class[32];
    char    title    [128];

    int     notoolbar;
    int     nostatusline;
    int     compact;
    int     noresize;
} pzframe_main_opts_t;

static psp_paramdescr_t text2pzframe_main_opts[] =
{
    PSP_P_STRING("app_name",      pzframe_main_opts_t, app_name,     NULL),
    PSP_P_STRING("app_class",     pzframe_main_opts_t, app_class,    NULL),
    PSP_P_STRING("title",         pzframe_main_opts_t, title,        NULL),
  
    PSP_P_FLAG  ("notoolbar",    pzframe_main_opts_t, notoolbar,    1, 0),
    PSP_P_FLAG  ("nostatusline", pzframe_main_opts_t, nostatusline, 1, 0),
    PSP_P_FLAG  ("compact",      pzframe_main_opts_t, compact,      1, 0),
    PSP_P_FLAG  ("noresize",     pzframe_main_opts_t, noresize,     1, 0),

    PSP_P_END()
};

static void EventProc(pzframe_gui_t *gui,
                      int            reason,
                      int            info_changed,
                      void          *privptr)
{
    ShowRunMode();
}


static void PzframeMainSaveData(const char *filename, const char *comment)
{
    if (global_gui->pfr->ftd->vmt.save != NULL  &&
        global_gui->pfr->ftd->vmt.save(global_gui->pfr, filename, NULL, comment) == 0)
        XhMakeMessage(pz_win, "Data saved to \"%s\"", filename);
    else
        XhMakeMessage(pz_win, "Error saving to file: %s", strerror(errno));
}

static void PzframeMainLoadData(const char *filename)
{
    if (global_gui->pfr->ftd->vmt.load != NULL  &&
        global_gui->pfr->ftd->vmt.load(global_gui->pfr, filename) == 0)
        XhMakeMessage(pz_win, "Data read from \"%s\"", filename);
    else
        XhMakeMessage(pz_win, "Error reading from \"%s\": %s", filename, strerror(errno));

    /* Renew displayed data */
    PzframeGuiUpdate(global_gui, 1);

    SetRunMode(0);
}

static actdescr_t toolslist[] =
{
    XhXXX_TOOLCMD   (cmChlNfLoadMode, "Read measurement from file", btn_open_xpm),
    XhXXX_TOOLCMD   (cmChlNfSaveMode, "Save measurement to file",   btn_save_xpm),
    XhXXX_TOOLLABEL ("ZZZ"),
    //XhXXX_SEPARATOR (),
    XhXXX_TOOLCHK   (cmLoop,          "Periodic measurement",       btn_start_xpm),
    XhXXX_TOOLCMD   (cmOnce,          "One measurement",            btn_once_xpm),
    XhXXX_SEPARATOR (),
    //XhXXX_TOOLCMD   (cmSetGood,       "Save current data as good",  btn_setgood_xpm),
    //XhXXX_TOOLCMD   (cmRstGood,       "Forget saved data",          btn_rstgood_xpm),
    XhXXX_TOOLLEDS  (),
    XhXXX_TOOLCMD   (cmChlHelp,       "Краткая справка",            btn_help_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD   (cmChlQuit,       "Quit the program",           btn_quit_xpm),
    XhXXX_END()
};


static XhWidget save_nfndlg = NULL;
static XhWidget load_nfndlg = NULL;

static void CommandProc(XhWindow win, int cmd)
{
  char          filename[PATH_MAX];
  char          comment[400];
  
    switch (cmd)
    {
        case cmChlNfLoadMode:
            check_snprintf(filename, sizeof(filename), CHL_DATA_PATTERN_FMT,
                           global_gui->pfr->ftd->type_name);
            if (load_nfndlg == NULL)
                load_nfndlg = XhCreateLoadDlg(win, "loadDataDialog",
                                              NULL, NULL,
                                              PzframeDataFdlgFilter, global_gui->pfr,
                                              filename, cmChlNfLoadOk, 0);
            XhLoadDlgShow(load_nfndlg);
            break;

        case cmChlNfLoadOk:
        case cmChlNfLoadCancel:
            XhLoadDlgHide(load_nfndlg);
            if (cmd == cmChlNfLoadOk)
            {
                XhLoadDlgGetFilename(load_nfndlg, filename, sizeof(filename));
                if (filename[0] == '\0') return;
                PzframeMainLoadData(filename);
            }
            break;

        case cmChlNfSaveMode:
            if (save_nfndlg != NULL) XhSaveDlgHide(save_nfndlg);
            if (save_nfndlg == NULL)
            save_nfndlg = XhCreateSaveDlg(win, "saveDataDialog", 
                                          NULL, NULL,
                                          cmChlNfSaveOk);
            XhSaveDlgShow(save_nfndlg);
            break;

        case cmChlNfSaveOk:
        case cmChlNfSaveCancel:
            XhSaveDlgHide(save_nfndlg);
            if (cmd == cmChlNfSaveOk)
            {
                XhSaveDlgGetComment(save_nfndlg, comment, sizeof(comment));
                check_snprintf(filename, sizeof(filename), CHL_DATA_PATTERN_FMT,
                               global_gui->pfr->ftd->type_name);
                XhFindFilename(filename, filename, sizeof(filename));
                PzframeMainSaveData(filename, comment);
            }
            break;

        case cmLoop:
            SetRunMode(!global_gui->pfr->cfg.running);
            break;
        
        case cmOnce:
            PzframeDataSetRunMode(global_gui->pfr, -1, 1);
            ShowRunMode();
            break;

        case cmChlHelp:
            ChlShowHelp(win, CHL_HELP_ALL &~ CHL_HELP_MOUSE);
            break;

        case cmChlQuit:
            exit(0);
    }
}
        

static void ShowHelp(const char *argv0, void *zzz /*!!! M-m-m??? Pass tables[],count? */)
{
    printf("Usage: %s SERVER.BIGC [OPTIONS]\n", argv0);
    
    exit(1);
}

static const char *fallbacks[] =
{
    XH_STANDARD_FALLBACKS,
};

int  PzframeMain(int argc, char *argv[],
                 const char *def_app_name, const char *def_app_class,
                 pzframe_gui_t *gui, pzframe_gui_dscr_t *gkd,
                 psp_paramdescr_t *table2, void *rec2,
                 psp_paramdescr_t *table4, void *rec4)
{
  pzframe_main_opts_t  opts;

  const char          *file_to_load = NULL;
  const char          *bigc_to_use  = NULL;
  int                  chan_base    = -1;
  char                 chan_base_str[20];

  char                 srvrspec[200];
  int                  bigc_n;

  int                  n;

  Widget               w;

  enum {PK_PARAM, PK_BIGC, PK_CHANBASE, PK_FILE};
  int          pkind;
  const char  *p_eq; // '='
  const char  *p_sl; // '/'
  const char  *p_cl; // ':'
  const char  *p_xc; // '!'

  void             *recs  [] =
  {
      &(gui->pfr->cfg.param_iv),
      rec2,
      &(gui->pfr->cfg),
      rec4,
      &(gui->look),
      &(opts),
  };
  psp_paramdescr_t *tables[] =
  {
      gkd->ftd->specific_params,
      table2,
      pzframe_data_text2cfg,
      table4,
      pzframe_gui_text2look,
      text2pzframe_main_opts,
  };

    global_gui = gui;

    /**** Initialize toolkits ***************************************/
    XhSetColorBinding("BG_DEFAULT",   "#f9eaa0");
    XhSetColorBinding("TS_DEFAULT",   "#fdf8e3");
    XhSetColorBinding("BS_DEFAULT",   "#bbb078");
    XhSetColorBinding("BG_ARMED",     "#fcf4d0");
    XhSetColorBinding("GRAPH_REPERS", "#00FF00");
  
    /* Initialize and parse command line */
    XhInitApplication(&argc, argv, def_app_name, def_app_class, fallbacks);

    /**** Prepare PSP-tables ****************************************/
    /*!!!*/
    /* main */
    {
      psp_paramdescr_t  *item;

        for (item = text2pzframe_main_opts;  item->name != NULL;  item++)
        {
            if (strcasecmp(item->name, "app_name")  == 0)
                item->var.p_string.defval = def_app_name;
            if (strcasecmp(item->name, "app_class") == 0)
                item->var.p_string.defval = def_app_class;
        }
    }

    /**** Parse command line parameters *****************************/
    /* Initialize everything to defaults */
    bzero(&opts, sizeof(opts));
    psp_parse_v(NULL, NULL,
                recs, tables, countof(tables),
                '=', "", "", 0);
    /* Walk through command line */
    for (n = 1;  n < argc;  n++)
    {
        if (strcmp(argv[n], "-h")     == 0  ||
            strcmp(argv[n], "-help")  == 0  ||
            strcmp(argv[n], "--help") == 0)
            ShowHelp(argv[0], NULL);

        if (strcmp(argv[n], "-readonly") == 0) argv[n]++;


        /* Paramkind determination heuristics */
        pkind = PK_PARAM;
        p_eq = strchr(argv[n], '=');
        p_sl = strchr(argv[n], '/');
        p_cl = strchr(argv[n], ':');
        p_xc = strchr(argv[n], '!');

        // =~ /[a-zA-Z]=/
        if (p_eq != NULL  &&  only_letters(argv[n], p_eq - argv[n]))
            pkind = PK_PARAM;

        // 
        if (argv[n][0] == '/'  ||  memcmp(argv[n], "./", 2) == 0  ||
            (p_sl != NULL  &&  (p_eq == NULL  ||  p_sl < p_eq)))
            pkind = PK_FILE;

        if (p_eq == NULL  &&  p_sl == NULL  &&
            (p_cl != NULL  ||  p_xc != NULL))
            pkind = PK_BIGC;

        if (only_digits(argv[n]))
            pkind = PK_CHANBASE;
        
        if      (pkind == PK_FILE)
            file_to_load = argv[n];
        else if (pkind == PK_BIGC)
            bigc_to_use  = argv[n];
        else if (pkind == PK_CHANBASE)
            chan_base    = atoi(argv[n]);
        else
        {
            if (psp_parse_v(argv[n], NULL,
                            recs, tables, countof(tables),
                            '=', "", "",
                            PSP_MF_NOINIT) != PSP_R_OK)
                fprintf(stderr, "%s: %s\n", argv[0], psp_error());
        }
    }

    /**** Use data from command line ********************************/
    if (bigc_to_use == NULL) bigc_to_use = "::0";

    if (cx_parse_chanref(bigc_to_use,
                         srvrspec, sizeof(srvrspec), &bigc_n) < 0)
    {
        fprintf(stderr, "%s: invalid CX big-channel reference \"%s\"\n",
                argv[0], bigc_to_use);
        return 1;
    }

    /**** Create window *********************************************/
    for (n = 0;  n < countof(toolslist);  n++)
        if (toolslist[n].type == XhACT_LABEL)
        {
            toolslist[n].label = global_gui->pfr->ftd->type_name;
            break;
        }
    if (opts.title[0] == '\0')
    {
        check_snprintf(chan_base_str, sizeof(chan_base_str), "%d", chan_base);
        check_snprintf(opts.title, sizeof(opts.title),
                       "%s: %s %s%s",
                       global_gui->pfr->ftd->type_name, bigc_to_use,
                       chan_base < 0? "" : " ", chan_base < 0? "" : chan_base_str);
    }

    pz_win = XhCreateWindow(opts.title, opts.app_name, opts.app_class,
                            (opts.notoolbar     ?  0 : XhWindowToolbarMask)     |
                            (opts.nostatusline  ?  0 : XhWindowStatuslineMask)  |
                            (opts.noresize == 0  &&
                             (global_gui->d->bhv & PZFRAME_GUI_BHV_NORESIZE) == 0?
                                                   0 : XhWindowUnresizableMask) |
                            (opts.compact  == 0 ?  0 : XhWindowCompactMask),
                            NULL,
                            toolslist);
    XhSetWindowCmdProc (pz_win, CommandProc);

    global_gui->look.noleds = !opts.notoolbar;
    if (global_gui->d->vmt.realize != NULL  &&
        global_gui->d->vmt.realize(global_gui,
                                   CNCRTZE(XhGetWindowWorkspace(pz_win)),
                                   srvrspec, bigc_n,
                                   CDA_SERVERID_ERROR, chan_base) < 0) return 1;

    w = global_gui->top;
    attachleft  (w, NULL, 0);
    attachright (w, NULL, 0);
    attachtop   (w, NULL, 0);
    attachbottom(w, NULL, 0);

    /**** Run *******************************************************/
    SetRunMode(global_gui->pfr->cfg.running);
    XhShowWindow(pz_win);

    if (file_to_load != NULL)
        PzframeMainLoadData(file_to_load);

    XhRunApplication();

    return 0;
}
