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
#define CESRVINTERNAL  (-25)  /* Internal server error */
#define CEWRONGUSAGE   (-26)  /* Wrong usage */
#define CEDIFFPROTO    (-27)  /* Server speaks different CX-protocol version */
#define CESRVSTIMEOUT  (-28)  /* Server-side handshake timeout */

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


char *cx_strerror(int errnum);
void  cx_perror  (const char *s);
void  cx_perror2 (const char *s, const char *argv0);

char *cx_strreason(int reason);

char *cx_strrflag_short(int shift);
char *cx_strrflag_long (int shift);

int   cx_parse_chanref(const char *spec,
                       char *srvnamebuf, size_t srvnamebufsize,
                       chanaddr_t  *chan_n,
                       char **channame_p);

//////////////////////////////////////////////////////////////////////

typedef void (*cx_notifier_t)(int uniq, void *privptr1,
                              int cd, int reason, const void *info,
                              void *privptr2);

int  cx_open  (int            uniq,     void *privptr1,
               const char    *spec,     int flags,
               const char    *argv0,    const char *username,
               cx_notifier_t  notifier, void *privptr2);
int  cx_close (int  cd);


void cx_do_cleanup(int uniq);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CXLIB_H */
