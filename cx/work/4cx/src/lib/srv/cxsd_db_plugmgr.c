#include "cxsd_db.h"


CxsdDbLdrModRec *first_dbldr_metric = NULL;


CxsdDb  CxsdDbLoadDb(const char *argv0,
                     const char *def_scheme, const char *reference)
{
  char             scheme[20];
  const char      *location;
  CxsdDbLdrModRec *mp;

    /* Decide with scheme */
    if (def_scheme != NULL  &&  *def_scheme == '!')
    {
        strzcpy(scheme, def_scheme + 1, sizeof(scheme));
        location = reference;
    }
    else
        split_url(def_scheme, "::",
                  reference, scheme, sizeof(scheme),
                  &location);

    /* Find appropriate loader */
    for (mp = first_dbldr_metric;  mp != NULL;  mp = mp->next)
        if (strcasecmp(scheme, mp->mr.name) == 0)
            break;
    if (mp == NULL) return NULL;

    /* Do! */
    return mp->ldr(argv0, location);
}


int  CxsdDbRegisterDbLdr  (CxsdDbLdrModRec *metric)
{
  CxsdDbLdrModRec *mp;

    if (metric == NULL)
        return -1;

    /* Check if it is already registered */
    for (mp = first_dbldr_metric;  mp != NULL;  mp = mp->next)
        if (mp == metric) return +1;

    /* Insert at beginning of the list */
    metric->next = first_dbldr_metric;
    first_dbldr_metric = metric;

    return 0;
}

int  CxsdDbDeregisterDbLdr(CxsdDbLdrModRec *metric)
{
}
