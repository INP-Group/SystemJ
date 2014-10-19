#include <stdlib.h>

#include "cxsd_db.h"


CxsdDb  CxsdDbCreate(void)
{
  CxsdDb db;

    db = malloc(sizeof(*db));
    if (db != NULL) bzero(db, sizeof(*db));
    return db;
}

int     CxsdDbDestroy(CxsdDb db)
{
  int  n;
    
    if (db == NULL) return 0;

    if (db->devlist != NULL)
    {
        for(n = 0;  n < db->numdevs;  n++)
        {
            safe_free(db->devlist[n].options);
            safe_free(db->devlist[n].auxinfo);
        }

        free(db->devlist);
    }

    if (db->liolist != NULL)
    {
        for(n = 0;  n < db->numlios;  n++)
        {
            safe_free(db->liolist[n].info);
        }

        free(db->liolist);
    }
    
    free(db);
    
    return 0;
}

int     CxsdDbAddDev (CxsdDb db, CxsdDbDevLine_t   *dev_p,
                      const char *options, const char *auxinfo)
{
  CxsdDbDevLine_t *new_devlist;
  const char      *new_options;
  const char      *new_auxinfo;
    
    new_options = new_auxinfo = NULL;

    /*!!! Check for duplicate .instname? */

    /* Treat empty strings as unset */
    if (options != NULL  &&  options[0] == '\0') options = NULL;
    if (auxinfo != NULL  &&  auxinfo[0] == '\0') auxinfo = NULL;

    /* Duplicate if specified */
    if (options != NULL  &&  (new_options = strdup(options)) == NULL) goto CLEANUP;
    if (auxinfo != NULL  &&  (new_auxinfo = strdup(auxinfo)) == NULL) goto CLEANUP;

    new_devlist = safe_realloc(db->devlist,
                               sizeof(*new_devlist) * (db->numdevs + 1));
    if (new_devlist == NULL) goto CLEANUP;

    db->devlist = new_devlist;
    db->devlist[db->numdevs] = *dev_p; /*!!! Here should check for duplicate .instname, and do something (error(earlier!)/warning/erase_name) */
    db->devlist[db->numdevs].options = new_options;
    db->devlist[db->numdevs].auxinfo = new_auxinfo;
    db->numdevs++;
    
    return 0;
    
CLEANUP:
    safe_free(new_options);
    safe_free(new_auxinfo);
    return -1;
}

int     CxsdDbAddLyrI(CxsdDb db, CxsdDbLayerinfo_t *lio_p,
                      const char *info)
{
  CxsdDbLayerinfo_t *new_liolist;
  const char        *new_info;

    new_info = NULL;

    if (info != NULL  &&  info[0] == '\0') info = NULL;

    if (info != NULL  &&  (new_info = strdup(info)) == NULL) goto CLEANUP;

    new_liolist = safe_realloc(db->liolist,
                               sizeof(*new_liolist) * (db->numlios + 1));
    if (new_liolist == NULL) goto CLEANUP;

    db->liolist = new_liolist;
    db->liolist[db->numlios] = *lio_p;
    db->liolist[db->numlios].info = new_info;
    db->numlios++;
    
    return 0;
    
CLEANUP:
    safe_free(new_info);
    return -1;
}
