#define __VACCLIENT_DB_C
#include "vacclient_includes.h"

void OpenSubsysDescription(char *myfilename, char *argv0)
{
  char          *p;
  char          *subsys;
  void          *handle;
  subsysdescr_t *info;
  char          *err;
    
    /* Check if we are run "as is" instead of via symlink */
    p = strrchr(myfilename, '.');
    if (p != NULL) *p = '\0';
    p = strrchr(argv0, '/');
    if (p != NULL) p++; else p = argv0;
    if (strcmp(p, myfilename) == 0)
    {
        fprintf(stderr, "%s: this program isn't intended to be run directly\n",
                argv0);
        exit(1);
    }

    subsys = p;
    
    /* Load the description */
    if (CdrOpenDescription(subsys, argv0, &handle, &info, &err) != 0)
    {
        fprintf(stderr, "%s: OpenDescription(): %s\n", subsys, err);
        exit(1);
    }

    /* Fill in "DB" values */
    app_name            = info->app_name;
    app_class           = info->app_class;
    app_title           = info->win_title;
    app_icon            = info->icon;
    app_defserver       = info->defserver;
    app_grouping        = info->grouping;
    app_phys_info       = info->phys_info;
    app_phys_info_count = info->phys_info_count;
}
