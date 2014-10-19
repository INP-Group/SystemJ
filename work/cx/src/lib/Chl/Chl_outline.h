#ifndef __CHL_OUTLINE_H
#define __CHL_OUTLINE_H


#include <Xm/Xm.h>
#if (XmVERSION*1000+XmREVISION) >= 2002
  #define XM_OUTLINE_AVAILABLE 1
#else
  #define XM_OUTLINE_AVAILABLE 0
#endif


#if XM_OUTLINE_AVAILABLE
int ELEM_OUTLINE_Create_m(XhWidget parent, ElemInfo e);
#endif


#endif /* __CHL_OUTLINE_H */
