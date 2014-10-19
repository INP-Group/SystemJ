#ifndef __VACCLIENT_DB_H
#define __VACCLIENT_DB_H


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __VACCLIENT_DB_C
  #define D
  #define V(value...) = value
#else
  #define D extern
  #define V(value...)
#endif /* __VACCLIENT_DB_C */


enum
{
    KNOBS_PER_COLUMN = 6,
    KNOB_OFS_IMES = 0,
    KNOB_OFS_UMES = 1,
    KNOB_OFS_ISET = 2,
    KNOB_OFS_USET = 3,
    KNOB_OFS_IALM = 4,
    KNOB_OFS_UALM = 5,
};


D const char        *app_name            V(NULL);
D const char        *app_class           V(NULL);
D const char        *app_title           V(NULL);
D const char       **app_icon            V(NULL);
D const char        *app_defserver       V(NULL);
D const groupunit_t *app_grouping        V(NULL);
D const physprops_t *app_phys_info       V(NULL);
D int                app_phys_info_count V(0);


void OpenSubsysDescription(char *myfilename, char *argv0);


#undef D
#undef V


#endif /* __VACCLIENT_DB_H */
