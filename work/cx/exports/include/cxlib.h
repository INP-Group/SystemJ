/*********************************************************************
*                                                                    *
*  cxlib.h                                                           *
*                                                                    *
*********************************************************************/

#ifndef __CXLIB_H
#define __CXLIB_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <unistd.h>
#include "cx.h"


/*==== Some additional error codes -- all are negative =============*/
#define CEINTERNAL     (-1)   /* Internal cxlib problem or data corrupt */
#define CENOHOST       (-2)   /* Unknown host */
#define CEUNKNOWN      (-3)   /* Unknown response error code */
#define CETIMEOUT      (-4)   /* Handshake timed out */
#define CEMANYCLIENTS  (-5)   /* Too many clients */
#define CEACCESS       (-6)   /* Access to server denied */
#define CEBADRQC       (-7)   /* Bad request code */
#define CEPKT2BIGSRV   (-8)   /* Packet is too big for server */
#define CEINVCHAN      (-9)   /* Invalid channel number */
#define CESRVNOMEM     (-10)  /* Server ran out of memory */
#define CEMCONN        (-11)  /* Too many connections */
#define CERUNNING      (-12)  /* Request was successfully sent */
#define CECLOSED       (-13)  /* Connection is closed */
#define CEBADC         (-14)  /* Bad connection number */
#define CEBUSY         (-15)  /* Connection is busy */
#define CESOCKCLOSED   (-16)  /* Connection socket is closed by server */
#define CEKILLED       (-17)  /* Connection is killed by server */
#define CERCV2BIG      (-18)  /* Received packet is too big */
#define CESUBSSET      (-19)  /* Set request in subscription */
#define CEINVAL        (-20)  /* Invalid argument */
#define CEREQ2BIG      (-21)  /* Request packet will be too big */
#define CEFORKERR      (-22)  /* One of the forks has error code set */
#define CESMALLBUF     (-23)  /* Client-supplied buffer is too small */
#define CEINVCONN      (-24)  /* Invalid connection type */
#define CESYNCINSELECT (-25)  /* Synchronous connections are forbidden in select()-mode */
#define CEWRONGUSAGE   (-26)  /* Wrong usage */
#define CEDIFFPROTO    (-27)  /* Server speaks different CX-protocol version */


/*==== Cx Asynchronous call Reasons ================================*/
enum {
         CAR_CONNECT = 0,     /* Connection succeeded */
         CAR_CONNFAIL,        /* Connection failed */
         CAR_ERRCLOSE,        /* Connection was closed on error */
         CAR_ANSWER,          /* Synchronous answer had arrived */
         
         CAR_SUBSCRIPTION,    /* Subscription had arrived */
         CAR_ECHO,            /* Echo packet */
         CAR_KILLED           /* Connection was killed by server */
     };

typedef void (*cx_notifier_t)(int cd, int reason,
                              void *privptr, const void *info);


/*==== Connection/disconnection ====================================*/
int  cx_connect       (const char *host, const char *argv0);
int  cx_connect_n     (const char *host, const char *argv0,
                       cx_notifier_t notifier, void *privptr);

int  cx_openbigc      (const char *host, const char *argv0);
int  cx_openbigc_n    (const char *host, const char *argv0,
                       cx_notifier_t notifier, void *privptr);

int  cx_consolelogin  (const char *host, const char *user,
                       const char **banner);
int  cx_consolelogin_n(const char *host, const char *user,
                       const char **banner,
                       cx_notifier_t notifier, void *privptr);

int  cx_close         (int cd);

int  cx_ping          (int cd);

/*==== Non-blocking ================================================*/
int  cx_nonblock(int cd, int state);
int  cx_isready (int cd);
int  cx_result  (int cd);

int  cx_setnotifier(int cd, cx_notifier_t notifier, void *privptr);

int  cx_getcyclesize(int cd);

/*==== Console management ==========================================*/
int   cx_execcmd  (int cd, const char *cmdline, const char **reply);
char *cx_getprompt(int cd);

/*==== Packet management ===========================================*/
int  cx_begin    (int cd);
int  cx_run      (int cd);
int  cx_subscribe(int cd);

/*==== Writing the data ============================================*/
int  cx_setvalue (int cd, chanaddr_t  addr,
                          int32       value,
                          int32      *result,    tag_t *tag,
                          rflags_t   *rflags);

int  cx_setvgroup(int cd, chanaddr_t  start,     int    count,
                          int32       values [],
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags []);

int  cx_setvset  (int cd, chanaddr_t  addrs  [], int    count,
                          int32       values [],
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags []);

/*==== Reading the data ============================================*/
int  cx_getvalue (int cd, chanaddr_t  addr,
                          int32      *result,    tag_t *tag,
                          rflags_t   *rflags);

int  cx_getvgroup(int cd, chanaddr_t  start,     int    count,
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags []);

int  cx_getvset  (int cd, chanaddr_t  addrs  [], int    count,
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags []);

/*==== Big channels ================================================*/
int  cx_bigcmsg (int cd, int bigchan_id,
                 int32  args[],   int       nargs,
                 void  *data,     size_t    datasize,   size_t  dataunits,
                 int32  info[],   int       ninfo,      int    *rninfo,
                 void  *retbuf,   size_t    retbufsize, size_t *retbufused, size_t *retbufunits,
                 tag_t *tag,      rflags_t *rflags,
                 int    cachectl, int       immediate);
int  cx_bigcreq (int cd, int bigchan_id,
                 int32  args[],   int       nargs,
                 void  *data,     size_t    datasize,   size_t  dataunits,
                 int32  info[],   int       ninfo,      int    *rninfo,
                 void  *retbuf,   size_t    retbufsize, size_t *retbufused, size_t *retbufunits,
                 tag_t *tag,      rflags_t *rflags,
                 int    cachectl, int       immediate);
                 
/*==== Support services ============================================*/
char *cx_strerror(int errnum);
void  cx_perror  (const char *s);
void  cx_perror2 (const char *s, const char *argv0);

char *cx_strreason(int reason);

char *cx_strrflag_short(int shift);
char *cx_strrflag_long (int shift);

int   cx_parse_chanref(const char *spec,
                       char *srvnamebuf, size_t srvnamebufsize,
                       chanaddr_t  *chan_n);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CXLIB_H */
