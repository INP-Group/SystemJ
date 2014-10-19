#ifndef __CXSD_DRVMGR_H
#define __CXSD_DRVMGR_H


#include "cxsd_driver.h"


int  InitDriver(CxsdDriverModRec *metric);

void SetDevRflags(int devid, rflags_t rflags_to_set);
void RstDevRflags(int devid);

void InitDev  (int devid);
void TermDev  (int devid, rflags_t rflags_to_set);
void FreezeDev(int devid, rflags_t rflags_to_set);
void ReviveDev(int devid);
void ResetDev (int devid);


char * ParseDrvlogCategories(const char *str, const char **endptr,
                             int *set_mask_p, int *clr_mask_p);
const char *GetDrvlogCatName(int category);


#endif /* __CXSD_DRVMGR_H */
