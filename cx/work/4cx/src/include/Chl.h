#ifndef __CHL_H
#define __CHL_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "datatree.h"


int  ChlRunSubsystem(DataSubsys subsys);

/* Error descriptions */
char *ChlLastErr(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CHL_H */
