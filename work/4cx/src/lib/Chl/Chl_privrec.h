#ifndef __CHL_PRIVREC_H
#define __CHL_PRIVREC_H


#include "datatree.h"
#include "Xh.h"

#include "Chl_knobprops_types.h"


typedef struct
{
    DataSubsys       subsys;
    
    knobprops_rec_t  kp;
} chl_privrec_t;

chl_privrec_t *_ChlPrivOf(XhWindow win);


#endif /* __CHL_PRIVREC_H */
