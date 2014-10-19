#include "cxsd_includes.h"


static CxsdFrontendModRec *first_frontend_metric = NULL;

void ActivateFrontends(void)
{
  CxsdFrontendModRec *mp;

    for (mp = first_frontend_metric;  mp != NULL;  mp = mp->next)
        if (mp->init_fe != NULL  &&
            mp->init_fe(option_instance) != 0)
        {
            normal_exit = 1;
            exit(1);
        }
}

int RegisterFrontend  (CxsdFrontendModRec *metric)
{
  CxsdFrontendModRec *mp;

    if (metric == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    /* Check if it is already registered */
    for (mp = first_frontend_metric;  mp != NULL;  mp = mp->next)
        if (mp == metric) return +1;
    
    /* Insert at beginning of the list */
    metric->next = first_frontend_metric;
    first_frontend_metric = metric;

    return 0;
}

int DeregisterFrontend(CxsdFrontendModRec *metric)
{
}
