#ifndef __NDBP_H
#define __NDBP_H


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __NDBP_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __NDBP_C */


enum
{
    RUNNONCE_RUN  = 1,
    RUNNONCE_STOP = 2,
    RUNNONCE_ONCE = 3,
};

typedef void (*ndbp_runnonce_proc_t)(int code, void *privptr);

typedef void (*DoLoadPic_t)    (const char *filename);
typedef void (*DoSavePic_t)    (const char *filename, const char *comment, const char *subsys);


D ndbp_runnonce_proc_t   runnonce_proc    V(NULL);
D void                  *runnonce_privptr V(NULL);

D xh_loaddlg_filter_proc LoadPicFilter    V(NULL);
D DoLoadPic_t            DoLoadPic        V(NULL);
D DoSavePic_t            DoSavePic        V(NULL);


#undef D
#undef V


#endif /* __NDBP_H */
