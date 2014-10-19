#ifndef __CHL_INTERNALS_H
#define __CHL_INTERNALS_H


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __CHL_INTERNALS_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __CHL_INTERNALS_C */


/* "Abstraction" (copied from Xh_utils.h) */
#define CNCRTZE(w) ((Widget)w)
#define ABSTRZE(w) ((XhWidget)w)


/* One more Xh "reference" -- attachments */

void attachleft  (XhWidget what, XhWidget where, int offset);
void attachright (XhWidget what, XhWidget where, int offset);
void attachtop   (XhWidget what, XhWidget where, int offset);
void attachbottom(XhWidget what, XhWidget where, int offset);


/* Common/standard elements' options */

typedef struct
{
    int   title_side;
    int   notitle;   
    int   nohline;   
    int   noshadow;  
    int   nofold;    
    int   folded;    
    int   fold_h;    
    int   sepfold;   
} _Chl_containeropts_t;

extern psp_paramdescr_t text2_Chl_containeropts[];


D char _Chl_lasterr_str[240] V("");
#define CLEAR_ERR() _Chl_lasterr_str[0] = '\0'


#undef D
#undef V


#endif /* __CHL_INTERNALS_H */
