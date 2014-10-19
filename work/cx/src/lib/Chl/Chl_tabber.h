#ifndef __CHL_TABBER_H
#define __CHL_TABBER_H


#include <Xm/Xm.h>
#if (XmVERSION*1000+XmREVISION) >= 2002
  #define XM_TABBER_AVAILABLE 1
#else
  #define XM_TABBER_AVAILABLE 0
#endif


#if XM_TABBER_AVAILABLE
int ELEM_TABBER_Create_m(XhWidget parent, ElemInfo e);
#endif


#endif /* __CHL_MULTICOL_H */
