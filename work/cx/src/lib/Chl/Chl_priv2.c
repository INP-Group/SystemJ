#include "Chl_includes.h"


datainfo_t *DataInfoOf(XhWindow window)
{
  datainfo_t *di;
  
    di = XhGetHigherPrivate(window);
    if (di == NULL)
    {
        XhSetHigherPrivate(window, (di = (void *)(XtCalloc(sizeof(*di), 1))));
        di->mainsid = CDA_SERVERID_ERROR;
    }

    return di;
}

priv2rec_t *Priv2Of(XhWindow win)
{
  priv2rec_t *pp = XhGetHigherPrivate2(win);
  
    if (pp == NULL)
    {
        pp = (void *)XtMalloc(sizeof(*pp));
        bzero(pp, sizeof(*pp));
        XhSetHigherPrivate2(win, pp);
    }

    return pp;
}
