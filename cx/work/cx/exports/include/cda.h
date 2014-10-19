#ifndef __CDA_H
#define __CDA_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <unistd.h>
#include <sys/time.h>

#include "cx.h"
#include "cxdata.h"


typedef unsigned int     cda_serverid_t;        // Server ID inside lib
typedef unsigned int     cda_objhandle_t;       // Phys.- and big-channel handle -- combines server_id and chan #; Generic "server-specific" object
typedef cda_objhandle_t  cda_physchanhandle_t;  // Phys. channel handle
typedef cda_objhandle_t  cda_bigchandle_t;      // Big-channel handle

enum {CDA_SERVERID_ERROR       = (cda_serverid_t)      -1};
enum {CDA_PHYSCHANHANDLE_ERROR = (cda_physchanhandle_t)-1};
enum {CDA_BIGCHANHANDLE_ERROR  = (cda_bigchandle_t)    -1};

enum
{
    CDA_OPT_IS_WRITE = 1 << 0,
    CDA_OPT_READONLY = 1 << 1,
};


typedef void * (*cda_signal_setter_t)  (void *info);
typedef void   (*cda_signal_raiser_t)  (void *id);
typedef void   (*cda_signal_remover_t) (void *id);
typedef void * (*cda_timeout_setter_t) (int  usecs, void *info);
typedef void   (*cda_timeout_remover_t)(void *id);

typedef void * (*cda_malloc_func_t)(size_t size);

typedef void   (*cda_eventp_t)(cda_serverid_t sid, int reason, void *privptr);

typedef void   (*cda_lclregchg_cb_t)(int lreg_n, double v);

typedef void   (*cda_strlogger_t)(const char *str);

typedef enum
{
    CDA_REGULAR = 0x67526144, // Little-endian 'DaRg'
    CDA_BIGC    = 0x63426144, // Little-endian 'DaBc'
} cda_conntype_t;

typedef enum /* Note: the statuses are ordered by the level of severity, so that a "most severe of several" status can be easily chosen */
{
    CDA_SERVERSTATUS_NORMAL,
    CDA_SERVERSTATUS_FROZEN,
    CDA_SERVERSTATUS_ALMOSTREADY,
    CDA_SERVERSTATUS_DISCONNECTED,
} cda_serverstatus_t;

enum
{
    CDA_R_MIN_SID_N = 0,
    CDA_R_MAINDATA  = 0,
};

typedef struct
{
    int     count;
    double *regs;
    char   *regsinited;
} cda_localreginfo_t;


/* Temp global-database staff */
int             cda_TMP_register_physinfo_dbase(physinfodb_rec_t *db);


/* Initialization stuff */
cda_serverid_t  cda_new_server(const char *spec,
                               cda_eventp_t event_processer, void *privptr,
                               cda_conntype_t conntype);
cda_serverid_t  cda_add_auxsid(cda_serverid_t  sid, const char *auxspec,
                               cda_eventp_t event_processer, void *privptr);
int             cda_run_server(cda_serverid_t  sid);
int             cda_hlt_server(cda_serverid_t  sid);
int             cda_del_server(cda_serverid_t  sid);
int             cda_continue  (cda_serverid_t  sid);

int             cda_add_evproc(cda_serverid_t  sid, cda_eventp_t event_processer, void *privptr);
int             cda_del_evproc(cda_serverid_t  sid, cda_eventp_t event_processer, void *privptr);

int             cda_get_reqtimestamp(cda_serverid_t  sid, struct timeval *timestamp);

int             cda_set_physinfo(cda_serverid_t sid, physprops_t *info, int count);

int             cda_cyclesize     (cda_serverid_t  sid);

int             cda_status_lastn  (cda_serverid_t  sid);
int             cda_status_of     (cda_serverid_t  sid, int n);
const char     *cda_status_srvname(cda_serverid_t  sid, int n);

int             cda_srcof_physchan(cda_physchanhandle_t  chanh,   const char **name_p, int *n_p);
int             cda_srcof_formula (excmd_t              *formula, const char **name_p, int *n_p);

cda_serverid_t  cda_sidof_physchan(cda_physchanhandle_t  chanh);

int             cda_cnsof_physchan(cda_serverid_t  sid,
                                   uint8 *conns_u, int conns_u_size,
                                   cda_physchanhandle_t  chanh);
int             cda_cnsof_formula (cda_serverid_t  sid,
                                   uint8 *conns_u, int conns_u_size,
                                   excmd_t              *formula);

/* Data-access initialization stuff */
cda_physchanhandle_t cda_add_physchan(cda_serverid_t sid, int physchan);

excmd_t *cda_register_formula(cda_serverid_t defsid, excmd_t *formula, int options);
excmd_t *cda_HACK_findnext_f (excmd_t *formula);

cda_bigchandle_t cda_add_bigc(cda_serverid_t sid, int bigc,
                              int nargs, int retbufsize,
                              int cachectl, int immediate);
int              cda_del_bigc(cda_bigchandle_t  bigch);


/* Data r/w stuff */
int  cda_setphyschanval(cda_physchanhandle_t  chanh, double  v);
int  cda_setphyschanraw(cda_physchanhandle_t  chanh, int32   rv);
int  cda_prgsetphyschanval(cda_physchanhandle_t  chanh, double  v);
int  cda_prgsetphyschanraw(cda_physchanhandle_t  chanh, int32   rv);
int  cda_getphyschanval(cda_physchanhandle_t  chanh, double *vp, tag_t *tag_p, rflags_t *rflags_p);
int  cda_getphyschanraw(cda_physchanhandle_t  chanh, int32  *rp, tag_t *tag_p, rflags_t *rflags_p);
int  cda_getphyschanvnr(cda_physchanhandle_t  chanh, double *vp, int32  *rp, tag_t *tag_p, rflags_t *rflags_p);
int  cda_getphyschan_rd(cda_physchanhandle_t  chanh, double *rp, double *dp);
int  cda_getphyschan_q (cda_physchanhandle_t  chanh, double *qp);

int  cda_execformula(excmd_t *formula,
                     int options, double userval,
                     double *result_p, tag_t *tag_p, rflags_t *rflags_p,
                     int32 *rv_p, int *rv_useful_p,
                     cda_localreginfo_t *localreginfo);

int  cda_setlclreg(cda_localreginfo_t *localreginfo, int n, double  v);
int  cda_getlclreg(cda_localreginfo_t *localreginfo, int n, double *vp);
void cda_set_lclregchg_cb(cda_lclregchg_cb_t cb);
void cda_set_strlogger(cda_strlogger_t proc);

int cda_getbigcdata   (cda_bigchandle_t bigch, size_t ofs, size_t size, void *buf);
int cda_setbigcdata   (cda_bigchandle_t bigch, size_t ofs, size_t size, void *buf, size_t bufunits);
int cda_getbigcstats  (cda_bigchandle_t bigch, tag_t *tag_p, rflags_t *rflags_p);
int cda_getbigcparams (cda_bigchandle_t bigch, int start, int count, int32 *params);
int cda_setbigcparams (cda_bigchandle_t bigch, int start, int count, int32 *params);
int cda_setbigcparam_p(cda_bigchandle_t bigch, int param, int is_persistent);
int cda_setbigc_cachectl(cda_bigchandle_t  bigch, int cachectl);


char *cda_strserverstatus_short(int status);

char *cda_lasterr(void);


excmd_lapprox_rec *cda_load_lapprox_table(const char *spec, int xcol, int ycol);
int cda_do_linear_approximation(double *rv_p, double x,
                                int npts, double *matrix);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CDA_H */
