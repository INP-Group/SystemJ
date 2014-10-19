#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "fix_arpa_inet.h"

#include "cx_sysdeps.h"
#include "misc_macros.h"
#include "misclib.h"
#include "cxscheduler.h"
#include "fdiolib.h"

#include "cx.h"
#include "cxlib.h"
#include "cx_proto_v4.h"

#include "cxlib_wait_procs.h"


/*####################################################################
#                                                                    #
#  Connection management                                             #
#                                                                    #
####################################################################*/

enum
{
    MAX_UNCONNECTED_TIME = 60,
    DEF_HEARTBEAT_TIME   = 60 * 5,
};

enum
{
    CS_OPEN_FRESH = 0,  // Just allocated
    CS_OPEN_RESOLVE,    // Resolving server name to address
    CS_OPEN_CONNECT,    // Performing connect()
    CS_OPEN_ENDIANID,   // Have sent CXT4_ENDIANID
    CS_OPEN_LOGIN,      // Have sent CXT4_LOGIN
    CS_OPEN_FAIL,       // ECONNREFUSED, or other error on initial handshake

    CS_CLOSED,          // Closed by server, but not yet by client
    CS_USABLE,          // The connection is full-operational if >=
    CS_READY,           // Is ready for I/O
};

typedef struct
{
    int               in_use;
    int               state;
    int               isconnected;
    int               errcode;

    int               fd;
    fdio_handle_t     fhandle;
    sl_tid_t          clp_tid;
    sl_tid_t          hbt_tid;
    
    char              progname[40];
    char              username[40];
    char              srvrspec[64];       /* Holds a copy of server-spec */
    cx_notifier_t     notifier;
    int               uniq;
    void             *privptr1;
    void             *privptr2;

    uint32            ID;                 /* Identifier */
    uint32            Seq;                /* Sent packets seq. number */
} v4conn_t;

//////////////////////////////////////////////////////////////////////

enum
{
    V4_MAX_CONNS = 1000,  // An arbitrary limit
    V4_ALLOC_INC = 2,     // Must be >1 (to provide growth from 0 to 2)
};

static v4conn_t *cx4conns_list        = NULL;
static int       cx4conns_list_allocd = 0;

// GetV4connSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, V4conn, v4conn_t,
                                 cx4conns, in_use, 0, 1,
                                 1, V4_ALLOC_INC, V4_MAX_CONNS,
                                 , , void)

static void RlsV4connSlot(int cd)
{
  v4conn_t  *cp  = AccessV4connSlot(cd);
  int        err = errno;        // To preserve errno
  
    if (cp->in_use == 0) return; /*!!! In fact, an internal error */
    
    cp->in_use = 0;
    
    ////////////
    if (cp->fhandle >= 0) fdio_deregister(cp->fhandle);
    if (cp->fd      >= 0) close          (cp->fd);
    if (cp->clp_tid >= 0) sl_deq_tout    (cp->clp_tid);
    if (cp->hbt_tid >= 0) sl_deq_tout    (cp->hbt_tid);
    
    errno = err;
}

static int CdIsValid(int cd)
{
    return cd >= 0  &&  cd < cx4conns_list_allocd  &&  cx4conns_list[cd].in_use != 0;
}

static inline int cp2cd(v4conn_t *cp)
{
    return cp - cx4conns_list;
}

static int CXT42CE(uint32 Type)
{
  int  n;
  
  static struct
  {
      uint32  Type;
      int     CE;
  } trn_table[] =
  {
      {CXT4_EBADRQC,        CEBADRQC},
      {CXT4_DISCONNECT,     CEKILLED},
      {CXT4_READY,          0},
      {CXT4_ACCESS_GRANTED, 0},
      {CXT4_ACCESS_DENIED,  CEACCESS},
      {CXT4_DIFFPROTO,      CEDIFFPROTO},
      {CXT4_TIMEOUT,        CESRVSTIMEOUT},
      {CXT4_MANY_CLIENTS,   CEMANYCLIENTS},
      {CXT4_EINVAL,         CEINVAL},
      {CXT4_ENOMEM,         CESRVNOMEM},
      {CXT4_EPKT2BIG,       CEPKT2BIGSRV},
      {CXT4_EINTERNAL,      CESRVINTERNAL},
      {0, 0},
  };

    
    for (n = 0;  ;  n++)
    {
        if (trn_table[n].Type == Type) return trn_table[n].CE;
        if (trn_table[n].Type == 0)    return CEUNKNOWN;
    }
}

//////////////////////////////////////////////////////////////////////

static char cxlib_progname[40] = "";

static void cxlib_report(v4conn_t *cp, const char *format, ...)
    __attribute__ ((format (printf, 2, 3)));
static void cxlib_report(v4conn_t *cp, const char *format, ...)
{
  va_list     ap;
  const char *pn;
  const char *sr;
  int         cd;
  
    va_start(ap, format);
#if 1
    pn = cxlib_progname;
    if (cp != NULL  &&  cp->progname[0] != '\0') pn = cp->progname;
    sr = NULL;
    if (cp != NULL  &&  cp->srvrspec[0] != '\0') sr = cp->srvrspec;
    cd = (cp == NULL)? -1 : cp2cd(cp);
    fprintf (stderr, "%s %s%scxlib",
             strcurtime(), pn, pn[0] != '\0'? ": " : "");
    if (cp != NULL)
        fprintf(stderr, "[%d%s%s%s]", cd,
                sr != NULL? ":\"" : "",
                sr != NULL? sr    : "",
                sr != NULL? "\""  : "");
    fprintf (stderr, ": ");
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
    va_end(ap);
}

//////////////////////////////////////////////////////////////////////

static void async_CS_OPEN_CONNECT(v4conn_t *cp);
static void async_CS_OPEN_ENDIANID(v4conn_t *cp, CxV4Header *rcvd, size_t rcvdsize);
static void async_CS_OPEN_LOGIN   (v4conn_t *cp, CxV4Header *rcvd, size_t rcvdsize);

static inline void CallNotifier(v4conn_t *cp, int reason, void *info)
{
    if (cp->notifier != NULL)
        cp->notifier(cp->uniq, cp->privptr1,
                     cp2cd(cp), reason, info,
                     cp->privptr2);
}

static void MarkAsClosed(v4conn_t *cp, int errcode)
{
  int  reason;
    
    cp->state   = cp->isconnected? CS_CLOSED : CS_OPEN_FAIL;
    cp->errcode = errcode;

    if      (!cp->isconnected)    reason = CAR_CONNFAIL;
    else if (errcode == CEKILLED) reason = CAR_KILLED;
    else                          reason = CAR_ERRCLOSE;

    if (cp->fhandle >= 0) fdio_deregister(cp->fhandle);  cp->fhandle = -1;
    if (cp->fd      >= 0) close          (cp->fd);       cp->fd      = -1;
    if (cp->clp_tid >= 0) sl_deq_tout    (cp->clp_tid);  cp->clp_tid = -1;
    if (cp->hbt_tid >= 0) sl_deq_tout    (cp->hbt_tid);  cp->hbt_tid = -1;

    errno = errcode;
    CallNotifier(cp, reason, NULL);
    _cxlib_break_wait();
}

static void ProcessFdioEvent(int uniq, void *unsdptr,
                             fdio_handle_t handle, int reason,
                             void *inpkt, size_t inpktsize,
                             void *privptr)
{
  int         cd  = ptr2lint(privptr);
  v4conn_t   *cp  = AccessV4connSlot(cd);
  CxV4Header *hdr = inpkt;
  
    switch (reason)
    {
        case FDIO_R_DATA:
            switch (hdr->Type)
            {
                case CXT4_EBADRQC:
                case CXT4_DISCONNECT:
                case CXT4_ACCESS_DENIED:
                case CXT4_DIFFPROTO:
                case CXT4_TIMEOUT:
                case CXT4_MANY_CLIENTS:
                case CXT4_EINVAL:
                case CXT4_ENOMEM:
                case CXT4_EPKT2BIG:
                case CXT4_EINTERNAL:
                    MarkAsClosed(cp, CXT42CE(hdr->Type));
                    break;

                case CXT4_PING:
                    /* Now requires nothing */;
                    break;
                case CXT4_PONG:
                    /* requires no reaction */;
                    /*!!! In fact, never used */
                    break;
                
                default:
                    switch (cp->state)
                    {
                        case CS_OPEN_ENDIANID: async_CS_OPEN_ENDIANID(cp, hdr, inpktsize); break;
                        case CS_OPEN_LOGIN:    async_CS_OPEN_LOGIN   (cp, hdr, inpktsize); break;
                        
                        default:
                            cxlib_report(cp, "%s: unexpected packet of type %d arrived when in state %d",
                                         __FUNCTION__, hdr->Type, cp->state);
                    }
            }
            break;
      
        case FDIO_R_CONNECTED:
            async_CS_OPEN_CONNECT(cp);
            break;

        case FDIO_R_CONNERR:
            MarkAsClosed(cp, fdio_lasterr(handle));
            break;

        case FDIO_R_CLOSED:
            MarkAsClosed(cp, CESOCKCLOSED);
            break;

        case FDIO_R_IOERR:
            MarkAsClosed(cp, fdio_lasterr(handle));
            break;

        case FDIO_R_PROTOERR:
            /* In fact, as of 10.09.2009, FDIO_R_PROTOERR is unused by fdiolib */
            MarkAsClosed(cp, fdio_lasterr(handle)); /*!!! We should introduce CEPROTOERR */
            break;

        case FDIO_R_INPKT2BIG:
            MarkAsClosed(cp, CERCV2BIG);
            break;

        case FDIO_R_ENOMEM:
            MarkAsClosed(cp, ENOMEM);
            break;

        default:
            cxlib_report(cp, "%s: unknown fdiolib reason %d",
                         __FUNCTION__, reason);
            MarkAsClosed(cp, CEINTERNAL);
    }
}


//////////////////////////////////////////////////////////////////////

static int GetSrvSpecParts(const char *spec,
                           char       *hostbuf, size_t hostbufsize,
                           int        *number)
{
  const char *colonp;
  char       *endptr;
  size_t      len;

    /* No host specified?  Try CX_SERVER variable */
    if (spec == NULL  ||  *spec == '\0') spec = getenv("CX_SERVER");
    /* Treat missing spec as localhost */
    if (spec == NULL  ||  *spec == '\0') spec = "localhost";

    colonp = strchr(spec, ':');
    if (colonp != NULL)
    {
        *number = (int)(strtol(colonp + 1, &endptr, 10));
        if (endptr == colonp + 1  ||  *endptr != '\0')
        {
            cxlib_report(NULL, "invalid server specification \"%s\"", spec);
            errno = CEINVAL;
            return -1;
        }
        if (*number > CX_MAX_SERVER)
        {
            cxlib_report(NULL, "server # in specification \"%s\" is out of range [0..%d]",
                         spec, CX_MAX_SERVER);
            errno = CEINVAL;
            return -1;
        }
        len = colonp - spec;
    }
    else
    {
        *number = 0;
        len = strlen(spec);
    }

    if (len > hostbufsize - 1) len = hostbufsize - 1;
    memcpy(hostbuf, spec, len);
    hostbuf[len] = '\0';

    if (hostbuf[0] == '\0') /* Local ":N" spec? */
        strzcpy(hostbuf, "localhost", hostbufsize);

    return 0;
}

int  cx_open  (int            uniq,     void *privptr1,
               const char    *spec,     int flags,
               const char    *argv0,    const char *username,
               cx_notifier_t  notifier, void *privptr2)
{
  int                 cd;
  v4conn_t           *cp;
  int                 s;
  int                 r;

  char                host[256];
  int                 srv_n;
  int                 srv_port;

  int                 fastest;
  struct utsname      uname_buf;
  struct hostent     *hp;

  const char         *shrtname;
  
  in_addr_t           thishost;
  in_addr_t           loclhost;
  in_addr_t           spechost;

  struct sockaddr_un  udst;
  struct sockaddr_in  idst;
  struct sockaddr    *serv_addr;
  int                 addrlen;

  int                 on    = 1;
  size_t              bsize = CX_V4_MAX_PKTSIZE;
  
    /* Parse & check spec first */
    if (GetSrvSpecParts(spec, host, sizeof(host), &srv_n) != 0)
        return -1;
    if (srv_n >= 0)
    {
        srv_port = CX_V4_INET_PORT + srv_n;
        fastest  = 1;
    }
    else
    {
        srv_port = -srv_n;
        fastest  = 0;
    }

    /* Sanitize supplied credentials */
    /* User name... */
    if (username == NULL  ||  username[0] == '\0')
        username = getenv("LOGNAME");
    /* ...and program name */
    shrtname = "???";
    if (argv0 != NULL  &&  argv0[0] != '\0')
    {
        shrtname = strrchr(argv0, '/');
        if (shrtname == NULL) shrtname = argv0;
    }
    /* With GNU libc+ld we can determine the true argv[0] */
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
    shrtname = program_invocation_short_name;
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */
    if (cxlib_progname[0] == '\0')
        strzcpy(cxlib_progname, shrtname, sizeof(cxlib_progname));
    
    /* Allocate a free slot... */
    if ((cd = GetV4connSlot()) < 0) return cd;

    /* ...and fill it with data */
    cp = AccessV4connSlot(cd);
    cp->state    = CS_OPEN_FRESH;
    cp->fd       = -1;
    cp->fhandle  = -1;
    cp->clp_tid  = -1;
    cp->hbt_tid  = -1;
    strzcpy(cp->progname, shrtname, sizeof(cp->progname));
    strzcpy(cp->username, username, sizeof(cp->username));
    cp->notifier = notifier;
    cp->uniq     = uniq;
    cp->privptr1 = privptr1;
    cp->privptr2 = privptr2;

    /* Store server-spec for future debug use */
    if (spec != NULL) strzcpy(cp->srvrspec, spec, sizeof(cp->srvrspec));

    /* Resolve host:N */
    cp->state    = CS_OPEN_RESOLVE;
    /* Note:
     All the code from this point and up to switch to CS_OPEN_CONNECT
     could be moved to asynchronous processing if there was a way to perform
     resolving asynchronously
     */

    /* Find out IP of the specified host */
    /* First, is it in dot notation (aaa.bbb.ccc.ddd)? */
    spechost = inet_addr(host);
    /* No, should do a hostname lookup */
    if (spechost == INADDR_NONE)
    {
        hp = gethostbyname(host);
        /* No such host?! */
        if (hp == NULL)
        {
            errno = CENOHOST;
            goto CLEANUP_ON_ERROR;
        }

        memcpy(&spechost, hp->h_addr, hp->h_length);
    }

    /* Find out IP of this host (if required) */
    thishost = INADDR_NONE;
    loclhost = inet_addr("127.0.0.1");
    if (fastest  &&  spechost != loclhost)
    {
        uname(&uname_buf);
        hp = gethostbyname(uname_buf.nodename);
        if (hp != NULL)
            memcpy(&thishost, hp->h_addr, hp->h_length);
    }

    /* Should connect via UNIX if possible or via INET otherwise */
    if (fastest  &&               /* May use AF_UNIX? */
        (
         spechost == loclhost  || /* Is it a localhost (127.0.0.1)? */
         spechost == thishost     /* Is it the same host? */
        )
       )
    /* Use AF_UNIX */
    {
        /* Create a socket */
        s = cp->fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (s < 0)  goto CLEANUP_ON_ERROR;

        /* Set addrlen::servaddr to the path */
        udst.sun_family = AF_UNIX;
        sprintf(udst.sun_path, CX_V4_UNIX_ADDR_FMT, srv_n);
        serv_addr = (struct sockaddr*) &udst;
        addrlen   = offsetof(struct sockaddr_un, sun_path) + strlen(udst.sun_path);
    }
    else
    /* Use AF_INET */
    {
        /* Create a socket */
        s = cp->fd = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)  goto CLEANUP_ON_ERROR;

        /* Set addrlen::servaddr to host:port */
        idst.sin_family = AF_INET;
        idst.sin_port   = htons(srv_port);
        memcpy(&idst.sin_addr, &spechost, sizeof(spechost));
        serv_addr = (struct sockaddr *) &idst;
        addrlen   = sizeof(idst);

        /* For TCP, turn on keepalives */
        r = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
    }

    /* Set Close-on-exec:=1 */
    fcntl(s, F_SETFD, 1);
    
    /* Set buffers */
    setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &bsize, sizeof(bsize));
    setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &bsize, sizeof(bsize));

    /* Initiate connect() */
    cp->state = CS_OPEN_CONNECT;

    set_fd_flags(s, O_NONBLOCK, 1);
    r = connect(s, serv_addr, addrlen);
    if (r < 0  &&  errno != EINPROGRESS) goto CLEANUP_ON_ERROR;
    
    /* Register with fdiolib */
    cp->fhandle = fdio_register_fd(uniq, NULL,
                                   s, FDIO_CONNECTING,
                                   ProcessFdioEvent, lint2ptr(cd),
                                   CX_V4_MAX_PKTSIZE,
                                   sizeof(CxV4Header),
                                   FDIO_OFFSET_OF(CxV4Header, DataSize),
                                   FDIO_SIZEOF   (CxV4Header, DataSize),
                                   BYTE_ORDER == LITTLE_ENDIAN? FDIO_LEN_LITTLE_ENDIAN
                                                              : FDIO_LEN_BIG_ENDIAN,
                                   1, sizeof(CxV4Header));
    if (cp->fhandle < 0) goto CLEANUP_ON_ERROR;

    /* Block for request completion, if sync mode */
    _cxlib_begin_wait();

    /* Is operation still in progress? */
    if (cp->state != CS_OPEN_FAIL  &&  cp->state != CS_CLOSED) return cd;
    
    /* No, obtain error code and fall through to cleanup */
    errno = cp->errcode;
    
 CLEANUP_ON_ERROR:
    if (errno == ENOENT) errno = ECONNREFUSED;
    RlsV4connSlot(cd);
    return -1;
}

int  cx_close (int  cd)
{
  v4conn_t *cp = AccessV4connSlot(cd);

  struct
  {
      CxV4Header    hdr;
  } pkt;
  
    if (!CdIsValid(cd))
    {
        errno = CEBADC;
        return -1;
    }

    /* Try to send a CXT_LOGOUT packet, if it has sense... */
    if (cp->isconnected  &&  cp->fhandle >= 0)
    {
        bzero(&pkt, sizeof(pkt));
        pkt.hdr.DataSize = sizeof(pkt) - sizeof(pkt.hdr);
        pkt.hdr.Type     = CXT4_LOGOUT;
        fdio_send(cp->fhandle, &pkt, sizeof(pkt));
        ////SleepBySelect(10000000);
        
        /* Do "preventive" closes */
        fdio_deregister_flush(cp->fhandle, 10/*!!!Should make an enum-constant*/);
        cp->fhandle = -1;
        cp->fd      = -1;
    }
    
    RlsV4connSlot(cd);

    return 0;
}


static int cleanup_checker(v4conn_t *cp, void *privptr)
{
  int  uniq = *((int *)privptr);

    if (cp->uniq == uniq)
        cx_close(cp2cd(cp));

    return 0;
}

void cx_do_cleanup(int uniq)
{
    ForeachV4connSlot(cleanup_checker, &uniq);
}

//////////////////////////////////////////////////////////////////////

static void DoHeartbeatPing(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  int         cd  = ptr2lint(privptr);
  v4conn_t   *cp  = AccessV4connSlot(cd);
  CxV4Header  hdr;

    cp->hbt_tid = sl_enq_tout_after(cp->uniq, NULL,
                                    DEF_HEARTBEAT_TIME * 1000000,
                                    DoHeartbeatPing, lint2ptr(cp2cd(cp)));

    bzero(&hdr, sizeof(hdr));
    hdr.Type = CXT4_PING;
    hdr.ID   = cp->ID;

    if (fdio_send(cp->fhandle, &hdr, sizeof(hdr)) < 0)
        MarkAsClosed(cp, errno);
}

static void SetReady(v4conn_t *cp, uint32 ID)
{
    cp->state       = CS_READY;
    cp->isconnected = 1;
    cp->errcode     = 0;
    cp->ID          = ID;
    sl_deq_tout(cp->clp_tid);
    cp->clp_tid     = -1;

    cp->hbt_tid = sl_enq_tout_after(cp->uniq, NULL,
                                    DEF_HEARTBEAT_TIME * 1000000,
                                    DoHeartbeatPing, lint2ptr(cp2cd(cp)));

    CallNotifier(cp, CAR_CONNECT, NULL);
    _cxlib_break_wait();
}

static void WipeUnconnectedTimed(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  int         cd  = ptr2lint(privptr);
  v4conn_t   *cp  = AccessV4connSlot(cd);

    cp->clp_tid = -1;
    MarkAsClosed(cp, CETIMEOUT);
}

static void async_CS_OPEN_CONNECT(v4conn_t *cp)
{
  struct
  {
      CxV4Header    hdr;
  } pkt;
  
    ////SleepBySelect(1000000);
    ////fprintf(stderr, "\nready_w=%d\n", check_fd_state(cp->fd, O_RDWR));
    bzero(&pkt, sizeof(pkt));
    pkt.hdr.DataSize = sizeof(pkt) - sizeof(pkt.hdr);
    pkt.hdr.Type     = CXT4_ENDIANID;
    pkt.hdr.var1     = CXV4_VAR1_ENDIANNESS_SIG;
    pkt.hdr.var2     = CX_V4_PROTO_VERSION;

    if (fdio_send(cp->fhandle, &pkt, sizeof(pkt)) < 0)
        MarkAsClosed(cp, errno);
    else
        cp->state = CS_OPEN_ENDIANID;

    cp->clp_tid = sl_enq_tout_after(cp->uniq, NULL,
                                    MAX_UNCONNECTED_TIME * 1000000,
                                    WipeUnconnectedTimed, lint2ptr(cp2cd(cp)));
}

static void async_CS_OPEN_ENDIANID(v4conn_t *cp, CxV4Header *rcvd, size_t rcvdsize)
{
  struct
  {
      CxV4Header    hdr;
      CxV4LoginRec  lr;
  } pkt;
  
    if      (rcvd->Type != CXT4_LOGIN  &&  rcvd->Type != CXT4_ACCESS_GRANTED)
    {
        MarkAsClosed(cp, CXT42CE(rcvd->Type));
        return;
    }

    /* Note: the versions are "reversed" in check, since server's version
       must be same-or-higher than ours */
    if (!CX_VERSION_IS_COMPATIBLE(CX_V4_PROTO_VERSION, rcvd->var2))
    {
        MarkAsClosed(cp, CEDIFFPROTO);
        return;
    }

    if (rcvd->Type == CXT4_ACCESS_GRANTED)
    {
        SetReady(cp, rcvd->ID);
        return;
    }
    
    bzero(&pkt, sizeof(pkt));
    pkt.hdr.DataSize  = sizeof(pkt) - sizeof(pkt.hdr);
    pkt.hdr.Type      = CXT4_LOGIN;
    pkt.hdr.NumChunks = 1;

    pkt.lr.ck.OpCode   = 0; /*!!!*/
    pkt.lr.ck.ByteSize = sizeof(pkt.lr);
    strzcpy(pkt.lr.progname, cp->progname, sizeof(pkt.lr.progname));
    strzcpy(pkt.lr.username, cp->username, sizeof(pkt.lr.username));

    if (fdio_send(cp->fhandle, &pkt, sizeof(pkt)) < 0)
        MarkAsClosed(cp, errno);
    else
        cp->state = CS_OPEN_LOGIN;
}

static void async_CS_OPEN_LOGIN   (v4conn_t *cp, CxV4Header *rcvd, size_t rcvdsize)
{
    if (rcvd->Type == CXT4_ACCESS_GRANTED)
    {
        SetReady(cp, rcvd->ID);
        return;
    }
    else
    {
        MarkAsClosed(cp, CXT42CE(rcvd->Type));
        return;
    }
}


/*####################################################################
#                                                                    #
#  Transport                                                         #
#                                                                    #
####################################################################*/



/*####################################################################
#                                                                    #
#  Brains                                                            #
#                                                                    #
####################################################################*/
