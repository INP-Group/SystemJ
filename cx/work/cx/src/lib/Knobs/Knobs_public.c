#include "Knobs_includes.h"

XhWidget  KnobsGetElemContainer(ElemInfo e)
{
    return e->container;
}

XhWidget *KnobsGetElemInnageP  (ElemInfo e)
{
    return &(e->innage);
}
