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
#include "cx_proto.h"

#include "cxlib_wait_procs.h"

#include "h2ii2h.h"


/*####################################################################
#                                                                    #
#  Connection management                                             #
#                                                                    #
####################################################################*/

typedef struct {
    uint32    command;  /* CXC_xxx */
    int32    *results;  /* Array to store the results */
    tag_t    *tags;     /* Array to store the tags */
    rflags_t *rflags;   /* Array to store the flags */
    int32     count;    /* Number of channels */
} DataForkInfo;

typedef struct {
    int32    *info;
    int       ninfo;
    int      *rninfo;
    void     *retbuf;
    size_t    retbufsize;
    size_t   *retbufused;
    size_t   *retbufunits;
    tag_t    *tag;
    rflags_t *rflags;
} BigcForkInfo;

enum { MAX_UNCONNECTED_TIME = 60 };

typedef enum {
    CT_FREE = 0,        /* Isn't connected */
    CT_ANY  = 1,        /* Any type -- for CheckCd() only */
    CT_DATA = 2,        /* Data connection */
    CT_BIGC = 3,        /* Big channel */
    CT_CONS = 4         /* Console connection */
} conntype_t;

typedef enum {
    CS_OPEN_FRESH = 0,  /* Just created, not even connect()ed */
    CS_OPEN_RESOLVE,    // Resolving server name to address
    CS_OPEN_CONNECT,    // Performing connect()
    CS_OPEN_ACCESS,     /* Waiting for CXT_ACCESS_{GRANTED|DENIED} */
    CS_OPEN_LOGIN,      /* Waiting for CXT_READY or ... */
    CS_OPEN_FAIL,       // ECONNREFUSED, or other error on initial handshake

    CS_CLOSED,          /* Closed by server, but not yet by client */
    CS_USABLE,          /* The connection is full-operational if >= */
    CS_READY,           /* Is ready for I/O */
        
    CS_DATA_PREP,       /* type=CT_DATA_REQ was cx_begin()ed */
    CS_DATA_SENT,       /* CXT_DATA_REQ request was sent */
    CS_SUBS_SENT,       /* CXT_SUBSCRIBE request was sent */

    CS_CONS_SENT,       /* CXT_EXECCMD request was sent */

    CS_BIGC_PREP,       /* type=CT_BIGC was cx_begin()ed */
    CS_BIGC_SENT,       /* CXT_BIGC request was sent */
} connstate_t;

typedef struct
{
    int               in_use;
    connstate_t       state;
    int               isconnected;
    int               errcode;

    int               fd;
    fdio_handle_t     fhandle;
    sl_tid_t          clp_tid;
    
    char              progname[40];
    char              username[40];
    char              srvrspec[64];       /* Holds a copy of server-spec */
    cx_notifier_t     notifier;
    void             *privptr;

    ////////////////// Candidates for inclusion in v4
    uint32            ID;                 /* Identifier */
    uint32            Seq;                /* Sent packets seq. number */
    uint32            syncSeq;            /* Seq of last synchronous pkt */
    uint32            subsSeq;            /* Seq of last subscription */
    ////////////////// V2-specific, mainly obsolete
    conntype_t        type;
    int               logincmd;
    void             *auxptr;
    int               retval;
    int               pingable;
    uint32            BaseCycleSize;      /* Server's BaseCycleSize */

    CxHeader         *sendbuf;            /* Buffer to send data from */
    size_t            sendbufsize;        /* Its size */
    CxHeader         *lastbuf;            /* Buf. for last sync.reply */
    size_t            lastbufsize;        /* Its size */

    char              mainprompt[32];     /* Main prompt to use */
    char              tempprompt[32];     /* Aux prompt */

    union {
        void         *thebuf;
        DataForkInfo *data;
        BigcForkInfo *bigc;
    } fib;                             /* Buffer to store fork info */
    size_t         fibsize;            /* Its size */
    int            numforks;           /* # of forks requested */
    size_t         expreplydatasize;   /* Expected reply packet's `DataSize' */
    DataForkInfo  *subsbuf;            /* Subscription info buf */
    size_t         subsbufsize;        /* Its size */
    int            numsubscribed;      /* # of forks subscribed */

    union {
        struct {
            const char    **replyptr;  /* `reply' from cx_execcmd() */
        } execcmd;
    } aux;
} v2conn_t;

//////////////////////////////////////////////////////////////////////

enum
{
    V2_MAX_CONNS = 1000,  // An arbitrary limit
    V2_ALLOC_INC = 2,     // Must be >1 (to provide growth from 0 to 2)
};

static v2conn_t *cx2conns_list        = NULL;
static int       cx2conns_list_allocd = 0;

// GetV2connSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, V2conn, v2conn_t,
                                 cx2conns, in_use, 0, 1,
                                 1, V2_ALLOC_INC, V2_MAX_CONNS,
                                 , , void)

static void RlsV2connSlot(int cd)
{
  v2conn_t  *cp  = AccessV2connSlot(cd);
  int        err = errno;        // To preserve errno
  
    if (cp->in_use == 0) return; /*!!! In fact, an internal error */
    
    cp->in_use = 0;
    
    ////////////
    if (cp->fhandle >= 0) fdio_deregister(cp->fhandle);
    if (cp->fd      >= 0) close          (cp->fd);
    if (cp->clp_tid >= 0) sl_deq_tout    (cp->clp_tid);

    ////////////
    safe_free(cp->sendbuf);
    safe_free(cp->lastbuf);
    
    errno = err;
}

static int CdIsValid(int cd)
{
    return cd >= 0  &&  cd < cx2conns_list_allocd  &&  cx2conns_list[cd].in_use != 0;
}

static inline int cp2cd(v2conn_t *cp)
{
    return cp - cx2conns_list;
}

static int CXT2CE(uint32 Type)
{
  int  n;
  
  static struct
  {
      uint32  Type;
      int     CE;
  } trn_table[] =
  {
    {CXT_EINVCHAN,       CEINVCHAN},
    {CXT_OK,             0},
////
      {CXT_EBADRQC,        CEBADRQC},
      {CXT_DISCONNECT,     CEKILLED},
      {CXT_READY,          0},
      {CXT_ACCESS_GRANTED, 0},
      {CXT_ACCESS_DENIED,  CEACCESS},
//      {CXT_DIFFPROTO,      CEDIFFPROTO},
      {CXT_TIMEOUT,        CETIMEOUT},
      {CXT_MANY_CLIENTS,   CEMANYCLIENTS},
      {CXT_EINVAL,         CEINVAL},
      {CXT_ENOMEM,         CESRVNOMEM},
      {CXT_EPKT2BIG,       CEPKT2BIGSRV},
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

static void cxlib_report(v2conn_t *cp, const char *format, ...)
    __attribute__ ((format (printf, 2, 3)));
static void cxlib_report(v2conn_t *cp, const char *format, ...)
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

/*
 *  async_CXT_xxx, async_CS_xxx
 *      forward declarations of asynchronous packet processors (async_CXT_XXX)
 *      and state changers (async_CS_XXX).
 */

static void async_CXT_SUBSCRIPTION (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);
static void async_CXT_ECHO         (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);

static void async_CS_OPEN_CONNECT  (v2conn_t *cp);
static void async_CS_OPEN_ACCESS   (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);
static void async_CS_OPEN_LOGIN    (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);

static void async_CS_DATA_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);
static void async_CS_SUBS_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);
static void async_CS_BIGC_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);
static void async_CS_CONS_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize);

static inline void CallNotifier(v2conn_t *cp, int reason, void *info)
{
    if (cp->notifier != NULL)
        cp->notifier(cp2cd(cp), reason, cp->privptr, info); /* Note: privptr<->info are reversed in v2/v4 */
}

static void MarkAsClosed(v2conn_t *cp, int errcode)
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
  v2conn_t   *cp  = AccessV2connSlot(cd);
  CxHeader   *hdr = inpkt;
  
    switch (reason)
    {
        case FDIO_R_DATA:
            switch (hdr->Type)
            {
                case CXT_EBADRQC:
                case CXT_DISCONNECT:
                case CXT_ACCESS_DENIED:
                //case CXT_DIFFPROTO:
                case CXT_TIMEOUT:
                case CXT_MANY_CLIENTS:
                case CXT_EINVAL:
                case CXT_ENOMEM:
                case CXT_EPKT2BIG:
                case CXT_EINTERNAL:
                    MarkAsClosed(cp, CXT2CE(hdr->Type));
                    break;

                case CXT_SUBSCRIPTION: async_CXT_SUBSCRIPTION (cp, hdr, inpktsize); break;
                case CXT_ECHO:         async_CXT_ECHO         (cp, hdr, inpktsize); break;
                case CXT_PING:         /* Now requires nothing */;  break;
                case CXT_PONG:         /* requires no reaction */;  break; /*!!! In fact, never used */
                
                default:
                    switch (cp->state)
                    {
                        //case CS_OPEN_ENDIANID: async_CS_OPEN_ENDIANID(cp, hdr, inpktsize); break;
                        case CS_OPEN_LOGIN:  async_CS_OPEN_LOGIN  (cp, hdr, inpktsize); break;
                        ////
                        case CS_OPEN_ACCESS: async_CS_OPEN_ACCESS (cp, hdr, inpktsize); break;
                        case CS_DATA_SENT:   async_CS_DATA_SENT   (cp, hdr, inpktsize); break;
                        case CS_SUBS_SENT:   async_CS_SUBS_SENT   (cp, hdr, inpktsize); break;
                        case CS_CONS_SENT:   async_CS_CONS_SENT   (cp, hdr, inpktsize); break;
                        case CS_BIGC_SENT:   async_CS_BIGC_SENT   (cp, hdr, inpktsize); break;

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

//////////////////////////////////////////////////////////////////////

static int  do_open  (const char    *spec,     int flags,
                      const char    *argv0,    const char *username,
                      cx_notifier_t  notifier, void *privptr,
                      int            type,     int   logincmd, void *auxptr)
{
  int                 cd;
  v2conn_t           *cp;
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
  size_t              bsize = CX_MAX_PKTSIZE;

  enum {INIT_ANYBUFSIZE  = sizeof(CxHeader) + 1024 /* arbitrary value */,
        INIT_SENDBUFSIZE = INIT_ANYBUFSIZE,
        INIT_LASTBUFSIZE = INIT_ANYBUFSIZE};
  
    /* Parse & check spec first */
    if (GetSrvSpecParts(spec, host, sizeof(host), &srv_n) != 0)
        return -1;
    if (srv_n >= 0)
    {
        srv_port = CX_INET_PORT + srv_n;
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
    if ((cd = GetV2connSlot()) < 0) return cd;

    /* ...and fill it with data */
    cp = AccessV2connSlot(cd);
    cp->state   = CS_OPEN_FRESH;
    cp->fd      = -1;
    cp->fhandle = -1;
    cp->clp_tid = -1;
    strzcpy(cp->progname, shrtname, sizeof(cp->progname));
    strzcpy(cp->username, username, sizeof(cp->username));
    cp->notifier = notifier;
    cp->privptr  = privptr;
    cp->type     = type;
    cp->logincmd = logincmd;
    cp->auxptr   = auxptr;

    cp->sendbufsize = INIT_SENDBUFSIZE;
    cp->lastbufsize = INIT_LASTBUFSIZE;
    cp->sendbuf     = malloc(cp->sendbufsize);
    cp->lastbuf     = malloc(cp->lastbufsize);
    /* Any of the allocations failed? */
    if (cp->sendbuf == NULL  ||  cp->lastbuf == NULL)
        goto CLEANUP_ON_ERROR;

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
        sprintf(udst.sun_path, CX_UNIX_ADDR_FMT, srv_n);
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
    cp->fhandle = fdio_register_fd(0, NULL, /*!!!uniq*/s, FDIO_CONNECTING,
                                   ProcessFdioEvent, lint2ptr(cd),
                                   CX_ABSOLUTE_MAX_PKTSIZE,
                                   sizeof(CxHeader),
                                   FDIO_OFFSET_OF(CxHeader, DataSize),
                                   FDIO_SIZEOF   (CxHeader, DataSize),
                                   FDIO_LEN_LITTLE_ENDIAN,
                                   1, sizeof(CxHeader));
    if (cp->fhandle < 0) goto CLEANUP_ON_ERROR;

    /* Block for request completion, if sync mode */
    _cxlib_begin_wait();

    /* Is operation still in progress? */
    if (cp->state != CS_OPEN_FAIL  &&  cp->state != CS_CLOSED) return cd;
    
    /* No, obtain error code and fall through to cleanup */
    errno = cp->errcode;
    
 CLEANUP_ON_ERROR:
    if (errno == ENOENT) errno = ECONNREFUSED;
    RlsV2connSlot(cd);
    return -1;
}

int  cx_close (int  cd)
{
  v2conn_t *cp = AccessV2connSlot(cd);

  struct
  {
      CxHeader    hdr;
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
        pkt.hdr.Type     = CXT_LOGOUT;
        fdio_send(cp->fhandle, &pkt, sizeof(pkt));
        ////SleepBySelect(10000000);
        
        /* Do "preventive" closes */
        fdio_deregister_flush(cp->fhandle, 10/*!!!Should make an enum-constant*/);
        cp->fhandle = -1;
        cp->fd      = -1;
    }
    
    RlsV2connSlot(cd);

    return 0;
}

//////////////////////////////////////////////////////////////////////

int  cx_connect     (const char *host, const char *argv0)
{
    return cx_connect_n(host, argv0, NULL, NULL);
}

int  cx_connect_n   (const char *host, const char *argv0,
                     cx_notifier_t notifier, void *privptr)
{
    return do_open(host, 0,
                   argv0, NULL,
                   notifier, privptr,
                   CT_DATA, CXT_LOGIN, NULL);
}

int  cx_openbigc      (const char *host, const char *argv0)
{
    return cx_openbigc_n(host, argv0, NULL, NULL);
}

int  cx_openbigc_n    (const char *host, const char *argv0,
                       cx_notifier_t notifier, void *privptr)
{
    return do_open(host, 0,
                   argv0, NULL,
                   notifier, privptr,
                   CT_BIGC, CXT_OPENBIGC, NULL);
}

int cx_consolelogin  (const char *host, const char *user,
                      const char **banner)
{
    return cx_consolelogin_n(host, user, banner, NULL, NULL);
}

int cx_consolelogin_n(const char *host, const char *user,
                      const char **banner,
                      cx_notifier_t notifier, void *privptr)
{
    return do_open(host, 0,
                   NULL, user,
                   notifier, privptr,
                   CT_CONS, CXT_CONSOLE_LOGIN, banner);
}

//////////////////////////////////////////////////////////////////////

/*
 *  CheckCd
 *      Checks if a connection is of given type and in a given state
 *
 *  Args:
 *      cd     connection descriptor to check
 *      type   required type; if ==CT_ANY, type check isn't performed
 *      state  required state; if cp->state == CS_CLOSED, tate' is ignored
 *
 *  Result:
 *      0   connection is ok
 *      -1  something is wrong, rrno' contains a CExxx code
 */

static inline int CheckCd(int cd, conntype_t type, connstate_t state)
{
  v2conn_t *cp = AccessV2connSlot(cd);
    
    if (!CdIsValid(cd))
    {
        errno = CEBADC;
        return -1;
    }
    if (type != CT_ANY  &&  cp->type != type)
    {
        errno = CEINVCONN;
        return -1;
    }
    if (cp->state == CS_CLOSED)
    {
        errno = CECLOSED;
        return -1;
    }
    if (cp->state != state)
    {
        errno = CEBUSY;
        return -1;
    }
    return 0;
}

/*
 *  CheckSyncSeq
 *      Checks if the `Seq' field in a received packet is what it should be,
 *      and prints an error message if not so.
 *
 *  Args:
 *      cp        connection pointer to check
 *      function  calling function name; usually `__FUNCTION__'
 */

static void CheckSyncSeq(v2conn_t *cp, const char *function, CxHeader *rcvd)
{
    if (rcvd->Seq != cp->syncSeq)
        cxlib_report(cp, "%s: reply Seq/%d/ != syncSeq/%d/",
                     function, rcvd->Seq, cp->syncSeq);
}

/*
 *  CheckSyncReply
 *      Checks the validity of a synchronous reply packet:
 *      the `Type' and `Seq' fields and, if required, the `NumData'
 *      field.
 *
 *  Args:
 *      cp        connection pointer to check
 *      function  calling function name; usually `__FUNCTION__'
 *      etype     expected value of the `Type' field
 *      checknum  true/false -- whether to check the `NumData' field
 *
 *  Result:
 *      0     connection is ok
 *      else  connection is MarkAsClosed()'d, and the caller should
 *            immediately return
 */

static int CheckSyncReply(v2conn_t *cp,    const char *function, CxHeader *rcvd,
                          uint32    etype, int         checknum)
{
    /* Okay, what is there? */
    if (rcvd->Type != etype)
    {
        cxlib_report(cp, "%s: reply is %d while %d expected",
                     function, rcvd->Type, etype);
        MarkAsClosed(cp, CXT2CE(rcvd->Type));
        return 1;
    }
    CheckSyncSeq(cp, function, rcvd);

    /* Just a sanity check... */
    if (checknum  &&  rcvd->NumData != cp->numforks)
        cxlib_report(cp, "%s: rcvd->NumData/%d/ != cp->numforks/%d/",
                     function, rcvd->NumData, cp->numforks);

    return 0;
}

//////////////////////////////////////////////////////////////////////

/*
 *  StartNewPacket
 *      Initializes a packet
 *
 *  Args:
 *      cp    connection pointer
 *      type  one of CXT_xxx constants to assign a packet type to
 */

static inline void StartNewPacket(v2conn_t *cp, int type)
{
    bzero(cp->sendbuf, sizeof(CxHeader));
    cp->sendbuf->Type = type;

    /* Must also assign values to Seq and ID here */
    cp->sendbuf->ID  = cp->ID;
    cp->sendbuf->Seq = ++(cp->Seq); /* Increment first, than assign,
                                       so that cp->sendbuf->Seq == cp->Seq */
}

/*
 *  GrowSendBuf
 *      Calls a GrowBuf() to grow the cp->sendbuf up to
 *      sizeof(CxHeader)+datasize.
 */

static inline int GrowSendBuf(v2conn_t *cp, size_t datasize)
{
    return GrowBuf((void *) &(cp->sendbuf),
                   &(cp->sendbufsize),
                   sizeof(CxHeader) + datasize);
}

/*
 *  SendRequest
 *      Sends a request prepared in [cd]->sendbuf to server.
 *      Converts the header from host to intel byteorder prior to
 *      sending it, and back to host after sending.
 *
 *  Args:
 *      cp  connection pointer
 *
 *  Result:
 *      0   success
 *      -1  failure; `errno' contains error code
 */

static int SendRequest(v2conn_t *cp)
{
  int             r;
  size_t          DataSize; /* To hold DataSize after h2ihdr() */

    DataSize = cp->sendbuf->DataSize;
    h2ihdr(cp->sendbuf);
    r = fdio_send(cp->fhandle, cp->sendbuf, sizeof(CxHeader) + DataSize);
    i2hhdr(cp->sendbuf);
    if (r < 0)  return -1;

    return 0;
}

/*
 * StoreReply
 *     New incarnation of CopyRecvbufToLastbuf()
 */

static int StoreReply(v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
  int     r;
    
    r = GrowBuf((void *) &(cp->lastbuf),
                &(cp->lastbufsize),
                rcvdsize);
    if (r < 0) return r;
    memcpy(cp->lastbuf, rcvd, rcvdsize);

    return 0;
}

/*
 *  ExtractPrintable
 *      Extracts message and prompt from the packet if they are
 *      present.
 *
 *  Args:
 *      buf         buffer to extract data from;
 *                  usually cp->lastbuf
 *      *message    where to put the pointer to the message (may be NULL)
 *      prompt      where to put the prompt (may be NULL)
 *      promptsize  max. length of the prompt, including '\0'
 *      defprompt   default prompt; is used if there is no
 *                  prompt in the packet
 *
 *  Note:
 *      The message is *not* copied from the packet, but is only
 *      pointed to by *message', while the prompt is copied into
 *      `prompt'.
 */

static void ExtractPrintable(CxHeader *buf,
                             const char **message,
                             char *prompt, size_t promptsize,
                             const char *defprompt)
{
  char       *p      = buf->data;

  const char empty[] = "";

    /* Any printable message? */
    if (buf->Status & CXS_PRINT)
    {
        if (message != NULL)  *message = p;
        p += strlen(p) + 1;
    }
    else if (message != NULL) *message = empty;

    /* Any prompt? */
    if (buf->Status & CXS_PROMPT)
    {
        if  (prompt != NULL) strzcpy(prompt, p, promptsize);
        p += strlen(p) + 1;
    }
    else if (prompt != NULL) strzcpy(prompt, defprompt, promptsize);
}

//////////////////////////////////////////////////////////////////////

static void WipeUnconnectedTimed(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  int         cd  = ptr2lint(privptr);
  v2conn_t   *cp  = AccessV2connSlot(cd);

    cp->clp_tid = -1;
    MarkAsClosed(cp, CETIMEOUT);
}

static void async_CS_OPEN_CONNECT  (v2conn_t *cp)
{
    cp->state = CS_OPEN_ACCESS;

    cp->clp_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                    MAX_UNCONNECTED_TIME * 1000000,
                                    WipeUnconnectedTimed, lint2ptr(cp2cd(cp)));
}

static void async_CS_OPEN_ACCESS   (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
  int         r;
  CxLoginRec  info;

    /* Interpret the reply */
    if (rcvd->Type != CXT_ACCESS_GRANTED)
    {
        errno = CXT2CE(rcvd->Type);
        goto ERROR;
    }

    /* Check protocol version */
    if (!CX_VERSION_IS_COMPATIBLE(ntohl(rcvd->Res1), CX_PROTO_VERSION))
    {
        errno = CEDIFFPROTO;
        goto ERROR;
    }

    /* Remember server's pingability */
    cp->pingable = (ntohl(rcvd->Res1) >= CX_PROTO_VERSION_PINGABLE);
    ////fprintf(stderr, "cd=%d: pingable=%d: min=%d, his=%d\n", cp2cd(cp), cp->pingable, CX_PROTO_VERSION_PINGABLE, ntohl(cp->recvbuf->Res1));

    /* Now, send a [CONSOLE_]LOGIN request */
    StartNewPacket(cp, cp->logincmd);
    cp->sendbuf->Status = CXS_ENDIANID;
    r = GrowSendBuf(cp, sizeof(info));
    if (r < 0) goto ERROR;
    bzero(&info, sizeof(info));

    /* User & program info */
    info.uid = h2i32(getuid());
    strzcpy(info.username, cp->username, sizeof(info.username));
    strzcpy(info.password, cp->progname, sizeof(info.password));

    memcpy(cp->sendbuf->data, &info, sizeof(info));
    cp->sendbuf->DataSize = sizeof(info);

    r = SendRequest(cp);
    if (r < 0)  goto ERROR;

    cp->state = CS_OPEN_LOGIN;
    return;

 ERROR:
    MarkAsClosed(cp, errno);
}

static void async_CS_OPEN_LOGIN    (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
    /* Finally, look what does the server reply */
    if (rcvd->Type != CXT_READY)
    {
        MarkAsClosed(cp, CXT2CE(rcvd->Type));
        return;
    }

    /* This packet contains the ID assigned to this connection */
    cp->ID = rcvd->ID;

    /* ...and (probably) BaseCycleSize */
    cp->BaseCycleSize = rcvd->Res2;
    
    /* Preserve the reply */
    if (StoreReply(cp, rcvd, rcvdsize) < 0)
    {
        MarkAsClosed(cp, errno);
        return;
    }
    
    /* Notify the protocol */
    if (cp->type == CT_CONS)
        ExtractPrintable(cp->lastbuf, cp->auxptr,
                         cp->mainprompt, sizeof(cp->mainprompt), "(cx):");
    
    /* Mark connection as ready */
    cp->state       = CS_READY;
    cp->isconnected = 1;
    cp->errcode     = 0;

    sl_deq_tout(cp->clp_tid); cp->clp_tid = -1;

    /* Notify the user. */
    CallNotifier(cp, CAR_CONNECT, NULL);
    _cxlib_break_wait();
}


//////////////////////////////////////////////////////////////////////

/*#### "PACKETING BRACES" ##########################################*/

/*
 *  cx_begin
 *      Initializes a new data/subscription packet.
 *
 *  Args:
 *      cd  connection descriptor
 *
 *  Result:
 *      0   success
 *      -1  error, `errno' contains the code
 */

int cx_begin(int cd)
{
  v2conn_t       *cp = AccessV2connSlot(cd);
  connstate_t     next_state;

    /* Check the connection first */
    if      (CheckCd(cd, CT_DATA, CS_READY) >= 0)
        next_state = CS_DATA_PREP;
    else if (CheckCd(cd, CT_BIGC, CS_READY) >= 0)
        next_state = CS_BIGC_PREP;
    else
    {
        errno = CEINVCONN;
        return -1;
    }

    /* Do the initialization */
    StartNewPacket(cp, 0);
    cp->state            = next_state;
    cp->numforks         = 0;
    cp->expreplydatasize = 0;

    return 0;
}

static int RequestResult(v2conn_t *cp)
{
    cp->errcode = CERUNNING;
    _cxlib_begin_wait();

    if (cp->state == CS_READY)
    {
        errno = 0;
        return 0;
    }
    else
    {
        errno = cp->errcode;
        return -1;
    }
}

/*
 *  cx_run
 *      Sends a data request to the server.
 *
 *  Args:
 *      cd  connection descriptor
 *
 *  Result:
 *      0   success
 *      -1  error
 */

int cx_run(int cd)
{
  v2conn_t       *cp = AccessV2connSlot(cd);
  int             r;
  uint32          pkt_type;
  connstate_t     next_state;

    /* Check the connection first */
    if      (CheckCd(cd, CT_DATA, CS_DATA_PREP) >= 0)
    {
        pkt_type   = CXT_DATA_REQ;
        next_state = CS_DATA_SENT;
    }
    else if (CheckCd(cd, CT_BIGC, CS_BIGC_PREP) >= 0)
    {
        pkt_type   = CXT_BIGC;
        next_state = CS_BIGC_SENT;
    }
    else
    {
        errno = CEINVCONN;
        return -1;
    }

    /* Finish the preparation process */
    cp->sendbuf->Type    = pkt_type;
    cp->sendbuf->NumData = cp->numforks;

    /* Send the packet... */
    cp->state = next_state;
    cp->syncSeq = cp->sendbuf->Seq;
    r = SendRequest(cp);
    if (r < 0)
    {
        MarkAsClosed(cp, errno);
        return -1;
    }

    return RequestResult(cp);
}

/*
 *  cx_subscribe
 *      Sends a subscription request to the server.
 *
 *  Args:
 *      cd  connection descriptor
 *
 *  Result:
 *      0   success
 *      -1  error
 */

int cx_subscribe(int cd)
{
  v2conn_t     *cp = AccessV2connSlot(cd);
  int           r;
  uint8        *ptr;
  CxDataFork   *f;
  int           nf;

    /* Check the connection first */
    if (CheckCd(cd, CT_DATA, CS_DATA_PREP) < 0) return -1;

    /* Than, must check if there are any SET commands... */
    ptr = (void *) (cp->sendbuf->data);
    for (nf = 0; nf < cp->numforks; nf++)
    {
        f = (void*)ptr;
        switch (i2h32(f->OpCode))
        {
            case CXC_SETVGROUP:
            case CXC_SETVSET:
                errno = CESUBSSET;
                return -1;
        }
        ptr += DataForkSize(i2h32(f->OpCode), i2h32s(f->Count), CXDIR_REQUEST);
    }

    /* Store subscription info */
    r = GrowBuf((void *) &(cp->subsbuf),
                &(cp->subsbufsize),
                (cp->numforks) * sizeof(DataForkInfo));
    if (r < 0) return -1;
    cp->subsSeq = cp->Seq;
    memcpy(cp->subsbuf, cp->fib.data,
           (cp->numforks) * sizeof(DataForkInfo));
    cp->numsubscribed = cp->numforks;
    
    /* Finish the preparation process */
    cp->sendbuf->Type    = CXT_SUBSCRIBE;
    cp->sendbuf->NumData = cp->numforks;

    /* Send the packet... */
    cp->state   = CS_SUBS_SENT;
    cp->syncSeq = cp->sendbuf->Seq;
    r = SendRequest(cp);
    if (r < 0)
    {
        MarkAsClosed(cp, errno);
        return -1;
    }

    return RequestResult(cp);
}


/*#### DATA ########################################################*/

static void CopyReceivedForks(v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize,
                              DataForkInfo *fi, int number)
{
  uint8        *ptr;
  CxDataFork   *src;
  int           nf;
  size_t        bytesize;

    for (nf = 0,  ptr = (void *) (rcvd->data);
         nf < number;
         nf++,  fi++)
    {
        src = (void*)ptr;
        i2hfork(src);

        /* Some sanity checks... */
        if (src->OpCode   != fi->command)
            cxlib_report(cp, "%s: fork #%d: src->Command/0x%08x/ != fi->command/0x%08x/",
                         __FUNCTION__, nf, src->OpCode,   fi->command);
        if (src->Count    != fi->count)
            cxlib_report(cp, "%s: fork #%d: src->Count/%d/ != fi->count/%d/",
                         __FUNCTION__, nf, src->Count,    fi->count);
        bytesize = DataForkSize(CXC_ANY, fi->count, CXDIR_REPLY);
        if (src->ByteSize != bytesize)
            cxlib_report(cp, "%s: fork #%d: src->ByteSize/%d/ != DataForkSize()/%zu/",
                         __FUNCTION__, nf, src->ByteSize, bytesize);

        /* Copy results, tags and rflags */
        if (fi->results != NULL)
                memcpy_i2h32(fi->results, CDFY_VALUES(src), fi->count);
        if (fi->rflags  != NULL)
                memcpy_i2h32(fi->rflags,  CDFY_RFLAGS(src), fi->count);
        if (fi->tags    != NULL)
                memcpy      (fi->tags,    CDFY_TAGS  (src), fi->count * sizeof(tag_t));  /* sizeof(tag_t) == 1 */

        /* Advance */
        ptr += bytesize;
    }
}


static void async_CS_DATA_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
    /* Okay, what is there? */
    if (rcvd->Type != CXT_DATA_RPY)
    {
        cxlib_report(cp, "%s: reply is %d while %d expected; param=%d",
                     __FUNCTION__, rcvd->Type, CXT_DATA_RPY, rcvd->Res2);
        MarkAsClosed(cp, CXT2CE(rcvd->Type));
        return;
    }
    CheckSyncSeq(cp, __FUNCTION__, rcvd);

    /* Just a sanity check... */
    if (rcvd->NumData != cp->numforks)
        cxlib_report(cp, "%s: rcvd->NumData/%d/ != cp->numforks/%d/",
                     __FUNCTION__, rcvd->NumData, cp->numforks);

    /* Now, must interpret the results... */
    CopyReceivedForks(cp, rcvd, rcvdsize, cp->fib.data, cp->numforks);
    
    /* Finish */
    cp->retval = 0;
    cp->state  = CS_READY;
    CallNotifier(cp, CAR_ANSWER, NULL);
    _cxlib_break_wait();
}

static void async_CS_SUBS_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
    /* Okay, what is there? */
    if (rcvd->Type != CXT_OK)
    {
        cxlib_report(cp, "%s: reply is %d while %d expected; param=%d",
                     __FUNCTION__, rcvd->Type, CXT_OK, rcvd->Res2);
        MarkAsClosed(cp, CXT2CE(rcvd->Type));
        return;
    }
    CheckSyncSeq(cp, __FUNCTION__, rcvd);

    /* Finish */
    cp->retval = 0;
    cp->state  = CS_READY;
    CallNotifier(cp, CAR_ANSWER, NULL);
    _cxlib_break_wait();
}

static void async_CXT_SUBSCRIPTION (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
    /* Well, is it a DATA connection? */
    if (cp->type != CT_DATA)
    {
        cxlib_report(cp, "%s: CXT_SUBSCRIPTION/%d/ packet a %d type connection",
                     __FUNCTION__, (int)CXT_SUBSCRIPTION, (int)cp->type);
        return;
    }

    /* Check if the subscription matches current info */
    if (rcvd->Seq != cp->subsSeq) return;

    /* Just a sanity check... */
    if (rcvd->NumData != cp->numsubscribed)
        cxlib_report(cp, "%s: rcvd->NumData/%d/ != cp->numsubscribed/%d/",
                     __FUNCTION__, rcvd->NumData, cp->numsubscribed);

    /* Now, must interpret the results... */
    CopyReceivedForks(cp, rcvd, rcvdsize, cp->subsbuf, cp->numsubscribed);

    /* And notify the client */
    CallNotifier(cp, CAR_SUBSCRIPTION, NULL);
}

/*
 *  AddDataFork
 *      Generalized cx_{get,set}{value,vgroup,vset}(), which actually
 *      does the work. Is called by all of these funcs.
 *
 *  Args:
 *      cd       connection descriptor
 *      command  CXC_xxx -- operation requested
 *      addrs    for *VSET:   contains channel numbers
 *               for *VGROUP: [0] contains start channel
 *      count    number of channels in request, for *VALUE -- =1
 *      values   for SETV*: contains new values
 *               for GETV*: unused, may be NULL
 *      results  where to place returned values, may be NULL
 *      tags     where to place tags ("age" flags), may be NULL
 *      rflags   where to place "result flags", may be NULL
 *
 *  Result:
 *      0   success
 *      -1  error, `errno' contains code.
 */

static int AddDataFork(int cd, uint32     command,
                               chanaddr_t addrs [], int   count,
                               int32      values[], int32 results[], tag_t tags[],
                               rflags_t   rflags[])
{
  v2conn_t     *cp = AccessV2connSlot(cd);
  int           r;
  size_t        bytesize;
  CxDataFork   *p;

    /* Check the connection first */
    if (CheckCd(cd, CT_DATA, CS_DATA_PREP) < 0) return -1;
    
    /* Is `count' senseless? */
    if (count <= 0  ||  count > CX_MAX_CHANS_IN_FORK)
    {
        errno = CEINVAL;
        return -1;
    }

    /* Calc size of data... */
    bytesize = DataForkSize(command, count, CXDIR_REQUEST);
    if (cp->sendbuf->DataSize + bytesize > CX_MAX_DATASIZE)
    {
        errno = CEREQ2BIG;
        return -1;
    }

    /* Store fork info */
    r = GrowBuf((void *) &(cp->fib.thebuf),
                &(cp->fibsize),
                (cp->numforks + 1) * sizeof(DataForkInfo));
    if (r < 0)  return -1;
    cp->fib.data[cp->numforks].command = CXP_CVT_TO_RPY(command);
    cp->fib.data[cp->numforks].results = results;
    cp->fib.data[cp->numforks].tags    = tags;
    cp->fib.data[cp->numforks].rflags  = rflags;
    cp->fib.data[cp->numforks].count   = count;

    /* Now, append the data... */
    r = GrowSendBuf(cp, cp->sendbuf->DataSize + bytesize);
    if (r < 0)  return -1;

    p = (void *) (cp->sendbuf->data + cp->sendbuf->DataSize);
    bzero(p, sizeof(CxDataFork));
    p->OpCode   = command;
    p->ByteSize = bytesize;
    p->Count    = count;

    /* Command-code-dependent operations... */
    switch (command)
    {
        case CXC_SETVGROUP:
            p->ChanId = addrs[0];
            memcpy_h2i32((void *) (p->data), values, count);
            break;

        case CXC_SETVSET:
            memcpy_h2i32((void *) (p->data),                          addrs,  count);
            memcpy_h2i32((void *) (p->data + sizeof(uint32) * count), values, count);
            break;
        
        case CXC_GETVGROUP:
            p->ChanId = addrs[0];
            break;
            
        case CXC_GETVSET:
            memcpy_h2i32((void *) (p->data),                          addrs,  count);
            break;
    }

    h2ifork(p);

    /* Finally, modify packet header */
    cp->numforks++;
    cp->sendbuf->DataSize += bytesize;

    return 0;
}


/*
 *  cx_{set,get}{value,vgroup,vset}
 *      Data functions. In fact, just call AddFork().
 */

int  cx_setvalue (int cd, chanaddr_t  addr,
                          int32       value,
                          int32      *result,    tag_t *tag,
                          rflags_t   *rflags)
{ return AddDataFork(cd, CXC_SETVGROUP, &addr,  1,     &value, result,  tag,  rflags); }

int  cx_setvgroup(int cd, chanaddr_t  start,     int    count,
                          int32       values [],
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags [])
{ return AddDataFork(cd, CXC_SETVGROUP, &start, count, values, results, tags, rflags); }

int  cx_setvset  (int cd, chanaddr_t  addrs  [], int    count,
                          int32       values [],
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags [])
{ return AddDataFork(cd, CXC_SETVSET,   addrs,  count, values, results, tags, rflags); }

int  cx_getvalue (int cd, chanaddr_t  addr,
                          int32      *result,    tag_t *tag,
                          rflags_t   *rflags)
{ return AddDataFork(cd, CXC_GETVGROUP, &addr,  1,     NULL,   result,  tag,  rflags); }

int  cx_getvgroup(int cd, chanaddr_t  start,     int    count,
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags [])
{ return AddDataFork(cd, CXC_GETVGROUP, &start, count, NULL,   results, tags, rflags); }

int  cx_getvset  (int cd, chanaddr_t  addrs  [], int    count,
                          int32       results[], tag_t  tags[],
                          rflags_t    rflags [])
{ return AddDataFork(cd, CXC_GETVSET,   addrs,  count, NULL,   results, tags, rflags); }


/*#### BIGC ########################################################*/

int  cx_bigcmsg (int cd, int bigchan_id,
                 int32  args[],   int       nargs,
                 void  *data,     size_t    datasize,   size_t  dataunits,
                 int32  info[],   int       ninfo,      int    *rninfo,
                 void  *retbuf,   size_t    retbufsize, size_t *retbufused,  size_t *retbufunits,
                 tag_t *tag,      rflags_t *rflags,
                 int    cachectl, int       immediate)
{
    if (cx_begin(cd) < 0) return -1;
    if (cx_bigcreq(cd, bigchan_id,
                   args, nargs,
                   data, datasize, dataunits,
                   info, ninfo, rninfo,
                   retbuf, retbufsize, retbufused, retbufunits,
                   tag, rflags,
                   cachectl, immediate) < 0) return -1;
    return cx_run(cd);
}

int  cx_bigcreq (int cd, int bigchan_id,
                 int32  args[],   int       nargs,
                 void  *data,     size_t    datasize,   size_t  dataunits,
                 int32  info[],   int       ninfo,      int    *rninfo,
                 void  *retbuf,   size_t    retbufsize, size_t *retbufused, size_t *retbufunits,
                 tag_t *tag,      rflags_t *rflags,
                 int    cachectl, int       immediate)
{
  v2conn_t       *cp = AccessV2connSlot(cd);
  int             r;
  size_t          bytesize;
  size_t          recvsize;
  CxBigcFork     *cmd;
  int8           *p;
  
    /* Check the connection first */
    if (CheckCd(cd, CT_BIGC, CS_BIGC_PREP) < 0) return -1;

    /* Check parameters */
    if (datasize == 0) dataunits = 1;
    if (nargs < 0  ||  nargs > CX_MAX_BIGC_PARAMS  ||
        (
          (dataunits != 1  &&  dataunits != 2  &&  dataunits != 4)  ||
          datasize % dataunits != 0
        )  ||
        ninfo < 0
       )
    {
        errno = CEINVAL;
        return -1;
    }

    /* Calc size of data... */
    bytesize = BigcForkSize(nargs, datasize);
    if (cp->sendbuf->DataSize + bytesize > CX_ABSOLUTE_MAX_DATASIZE)
    {
        errno = CEREQ2BIG;
        return -1;
    }

    /* Store fork info */ 
    r = GrowBuf((void *) &(cp->fib.thebuf),
                &(cp->fibsize),
                (cp->numforks + 1) * sizeof(BigcForkInfo));
    if (r < 0)  return -1;
    cp->fib.bigc[cp->numforks].info        = info;
    cp->fib.bigc[cp->numforks].ninfo       = ninfo;
    cp->fib.bigc[cp->numforks].rninfo      = rninfo;
    cp->fib.bigc[cp->numforks].retbuf      = retbuf;
    cp->fib.bigc[cp->numforks].retbufsize  = retbufsize;
    cp->fib.bigc[cp->numforks].retbufused  = retbufused;
    cp->fib.bigc[cp->numforks].retbufunits = retbufunits;
    cp->fib.bigc[cp->numforks].tag         = tag;
    cp->fib.bigc[cp->numforks].rflags      = rflags;

    /* Now, append the data... */
    r = GrowSendBuf(cp, cp->sendbuf->DataSize + bytesize);
    if (r < 0)  return -1;

    cmd = (CxBigcFork *) (cp->sendbuf->data + cp->sendbuf->DataSize);
    bzero(cmd, sizeof(*cmd));
    
    cmd->OpCode            = h2i32(CXB_BIGCREQ);
    cmd->ByteSize          = h2i32(bytesize);
    cmd->ChanId            = h2i32(bigchan_id);
    cmd->nparams16x2       = h2i32(nargs);
    cmd->datasize          = h2i32(datasize);
    cmd->dataunits         = h2i32(dataunits);
    cmd->u.q.ninfo_n_flags = h2i32(ENCODE_FLAG(ninfo,     CXBC_FLAGS_NINFO_SHIFT,    CXBC_FLAGS_NINFO_MASK)    |
                                   ENCODE_FLAG(cachectl,  CXBC_FLAGS_CACHECTL_SHIFT, CXBC_FLAGS_CACHECTL_MASK) |
                                   ENCODE_FLAG(immediate, CXBC_FLAGS_IMMED_SHIFT,    CXBC_FLAGS_IMMED_FLAG));
    cmd->u.q.retbufsize    = h2i32(retbufsize);

    p = (int8 *)cmd->Zdata;
    if (nargs != 0)
    {
        memcpy_h2i32((uint32 *)p, args, nargs);
        p += nargs * sizeof(int32);
    }
    if (datasize != 0)
    {
        switch (dataunits)
        {
            case 1: memcpy      (          p, data, datasize);             break;
            case 2: memcpy_h2i16((uint16 *)p, data, datasize / dataunits); break;
            case 4: memcpy_h2i32((uint32 *)p, data, datasize / dataunits); break;
        }
    }
    
    recvsize = BigcForkSize(ninfo + 100/*!*/, retbufsize + 1024/*!*/);

    /* Finally, modify packet header */
    cp->numforks++;
    cp->expreplydatasize  += recvsize;
    cp->sendbuf->DataSize += bytesize;
    
    return 0;
}

static void CopyReceivedBigcs(v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize,
                              BigcForkInfo *fi, int number)
{
  uint8        *ptr;
  CxBigcFork   *src;
  int           nf;
  size_t        bytesize;

  int8           *p;

  void           *retbuf;
  int             count;

  uint32          _opcode_;
  int             nparams;
  size_t          datasize;
  size_t          dataunits;
  
    for (nf = 0,  ptr = (void *) (rcvd->data);
         nf < number;
         nf++,  fi++)
    {
        src = (void*)ptr;

        /* 1. Decode the header */
        _opcode_  = i2h32(src->OpCode);
        nparams   = i2h32(src->nparams16x2);
        datasize  = i2h32(src->datasize);
        dataunits = i2h32(src->dataunits);

        if (_opcode_ != CXB_BIGCREQ_R)
            cxlib_report(cp, "%s: fork #%d: src->Command/0x%08x/ != CXB_BIGCREQ_R/0x%08x/",
                         __FUNCTION__, nf, _opcode_,  CXB_BIGCREQ_R);

        if (nparams  > CX_MAX_BIGC_PARAMS  ||
            datasize > CX_ABSOLUTE_MAX_DATASIZE/*!!!*/  ||
            (
             (dataunits != 1  &&  dataunits != 2  &&  dataunits != 4) ||
             datasize % dataunits != 0
            )
           )
            cxlib_report(cp, "%s: fork #%d: nparams=%d, datasize=%zu, dataunits=%zu",
                         __FUNCTION__, nf, nparams, datasize, dataunits);
        
        bytesize = BigcForkSize(nparams, datasize);

        if (i2h32(src->ByteSize) != bytesize)
            cxlib_report(cp, "%s: fork #%d: src->ByteSize/%d/ != BigcForkSize()/%zu/",
                         __FUNCTION__, nf, i2h32(src->ByteSize), bytesize);
        
        p = (int8 *)(src->Zdata);
        
        /* 2. Copy params (if any) */
        count = fi->ninfo;
        if (nparams < count)  count = nparams;
        if (count != 0  &&  fi->info != NULL)
            memcpy_i2h32(fi->info, (uint32 *)p, count);
        p += nparams * sizeof(int32);
        
        if (fi->rninfo != NULL) /* This must be done here, while we know `count' (<=ninfo), not later */
            *(fi->rninfo) = count;
        
        /* 3. Copy data (if any) */
        if (fi->retbufsize < datasize) datasize = fi->retbufsize;
        if (datasize == 0) dataunits = 1;
        if ((dataunits != 1  &&  dataunits != 2  &&  dataunits != 4) ||
            datasize % dataunits != 0)
        {
            cxlib_report(cp, "%s: dataunits=%zu, datasize=%zu! Setting datasize=0",
                         __FUNCTION__, dataunits, datasize);
            datasize = 0; dataunits = 1;
        }
        count = datasize / dataunits;
        retbuf = fi->retbuf;
        if (count != 0  &&  retbuf != NULL)
        {
            switch(dataunits)
                {
                    case 1: memcpy      (retbuf,           p, datasize); break;
                    case 2: memcpy_i2h16(retbuf, (uint16 *)p, count);    break;
                    case 4: memcpy_i2h32(retbuf, (uint32 *)p, count);    break;
                }
        }
        
        if (fi->retbufused    != NULL)
            *(fi->retbufused)  = datasize;
        
        if (fi->retbufunits   != NULL)
            *(fi->retbufunits) = dataunits;

        if (fi->tag           != NULL)
            *(fi->tag)         = i2h32(src->u.y.tag);

        if (fi->rflags        != NULL)
            *(fi->rflags)      = i2h32(src->u.y.rflags);
        
        /* Advance */
        ptr += BigcForkSize(nparams, datasize);
    }
}

static void async_CS_BIGC_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
    /* Okay, what is there? */
    if (rcvd->Type != CXT_BIGCRESULT)
    {
        cxlib_report(cp, "%s: reply is %d while %d expected; param=%d",
                     __FUNCTION__, rcvd->Type, CXT_BIGCRESULT, rcvd->Res2);
        MarkAsClosed(cp, CXT2CE(rcvd->Type));
        return;
    }
    CheckSyncSeq(cp, __FUNCTION__, rcvd);

    /* Just a sanity check... */
    if (rcvd->NumData != cp->numforks)
        cxlib_report(cp, "%s: rcvd->NumData/%d/ != cp->numforks/%d/",
                     __FUNCTION__, rcvd->NumData, cp->numforks);

    /* Now, must interpret the results... */
    CopyReceivedBigcs(cp, rcvd, rcvdsize, cp->fib.bigc, cp->numforks);
    
    /* Finish */
    cp->retval = 0;
    cp->state  = CS_READY;
    CallNotifier(cp, CAR_ANSWER, NULL);
    _cxlib_break_wait();
}

/*#### CONSOLE #####################################################*/

/*
 *  cx_execcmd
 *      Sends a command to the CX manager for execution.
 *
 *  Args:
 *      cd       connection descriptor
 *      cmdline  command line
 *      *reply   where to place the pointer to returned message;
 *               if no message is returned, is set to NULL
 *
 *  Result:
 *       0  ok, just print the reply
 *       1  the EXIT command arrived
 *      -1  error, `errno' contains the code
 *
 *  Note:
 *      `*reply' is guaranteed to point to the returned message only
 *      until the next call to a synchronous command (in this case,
 *      cx_execcmd()).
 */

int   cx_execcmd(int cd, const char *cmdline, const char **reply)
{
  v2conn_t       *cp = AccessV2connSlot(cd);
  int             r;
  size_t          cmdsize = strlen(cmdline) + 1;

    /* Check the descriptor */
    if (CheckCd(cd, CT_CONS, CS_READY) < 0) return -1;

    /* Adjust send buffer size */
    r = GrowSendBuf(cp, cmdsize);
    if (r != 0)  return -1;

    cp->aux.execcmd.replyptr = reply;
    /* Prepare the packet */
    strcpy(cp->sendbuf->data, cmdline);
    StartNewPacket(cp, CXT_EXEC_CMD);
    cp->sendbuf->DataSize = cmdsize;

    /* Send it */
    cp->state = CS_CONS_SENT;
    cp->syncSeq = cp->sendbuf->Seq;
    r = SendRequest(cp);
    if (r < 0)
    {
        MarkAsClosed(cp, errno);
        return -1;
    }

    return RequestResult(cp);
}


static void async_CS_CONS_SENT     (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
    /* Finally, look what does the server reply */
    if (rcvd->Type != CXT_OK  &&  rcvd->Type != CXT_AEXIT)
    {
        cxlib_report(cp, "%s: reply is %d while %d or %d expected",
                     __FUNCTION__, rcvd->Type, CXT_OK, CXT_AEXIT);
        MarkAsClosed(cp, CXT2CE(rcvd->Type));
        return;
    }
    CheckSyncSeq(cp, __FUNCTION__, rcvd);
    
    /* Preserve the reply */
    if (StoreReply(cp, rcvd, rcvdsize) < 0)
    {
        MarkAsClosed(cp, errno);
        return;
    }
    
    /* Well, is there any additional info with the reply? */
    ExtractPrintable(cp->lastbuf, cp->aux.execcmd.replyptr,
                     cp->tempprompt, sizeof(cp->tempprompt), "");

    /* Finish */
    cp->retval = rcvd->Type == CXT_AEXIT ?  1  :  0;
    cp->state = CS_READY;
    CallNotifier(cp, CAR_ANSWER, NULL);
    _cxlib_break_wait();
}

static void async_CXT_ECHO         (v2conn_t *cp, CxHeader *rcvd, size_t rcvdsize)
{
  const char *message;

cxlib_report(cp, "CXT_ECHO");
    /* Well, is it a CONS connection? */
    if (cp->type != CT_CONS)
    {
        cxlib_report(cp, "%s: CXT_ECHO/%d/ packet on a %d type connection",
                     __FUNCTION__, (int)CXT_ECHO, (int)cp->type);
        return;
    }

    /* Extract the message */
    ExtractPrintable(rcvd, &message,
                     NULL, 0, NULL);
    
    CallNotifier(cp, CAR_ECHO, message);
}

/*
 *  cx_getprompt
 *      Returns the prompt to be printed this moment
 *      for a given connection.
 *
 *  Args:
 *      cd  connection descriptor
 *
 *  Result:
 *      NULL       `cd' is invalid or connection is busy
 *      otherwise  pointer to the prompt
 */

char *cx_getprompt(int cd)
{
  v2conn_t *cp = AccessV2connSlot(cd);

    if (CheckCd(cd, CT_CONS, CS_READY) < 0) return NULL;

    if (cp->tempprompt[0] != '\0')  return cp->tempprompt;
    else                            return cp->mainprompt;
}

//////////////////////////////////////////////////////////////////////

int  cx_nonblock(int cd, int state)
{
    return 0;
}

int  cx_result  (int cd)
{
  v2conn_t *cp = AccessV2connSlot(cd);

    if (!CdIsValid(cd))
    {
        errno = CEBADC;
        return -1;
    }

    errno = cp->errcode;
    return errno == 0? 0 : -1;
}

int  cx_setnotifier(int cd, cx_notifier_t notifier, void *privptr)
{
  v2conn_t *cp = AccessV2connSlot(cd);

    if (!CdIsValid(cd))
    {
        errno = CEBADC;
        return -1;
    }

    cp->notifier = notifier;
    cp->privptr  = privptr;

    return 0;
}

int  cx_getcyclesize(int cd)
{
  v2conn_t  *cp = AccessV2connSlot(cd);
  int        ret;

    if (CheckCd(cd, CT_DATA, CS_READY) < 0  &&
        CheckCd(cd, CT_BIGC, CS_READY) < 0) return -1;

    ret = cp->BaseCycleSize;
    if (ret <= 0) ret = 1000000; /* Use 1s default */

    return ret;
}

int  cx_ping          (int cd)
{
  v2conn_t *cp = AccessV2connSlot(cd);
  int       r;

    if (!CdIsValid(cd)  ||  cp->type == CT_FREE)
    {
        errno = CEBADC;
        return -1;
    }

    if (cp->state == CS_CLOSED)
    {
        errno = CECLOSED;
        return -1;
    }
    ////fprintf(stderr, "%s PING(cd=%d): pingable=%d busy=%d\n", strcurtime(), cd, cp->pingable, 0);
    if (cp->state == CS_DATA_PREP  ||
        cp->state == CS_BIGC_PREP)
    {
        errno = CEBUSY;
        return -1;
    }
    
    if (!cp->isconnected  ||  !cp->pingable)
        return 0;

    ////fprintf(stderr, "%s %s(%d): Seq=%d, syncSeq=%d\n", strcurtime(), __FUNCTION__, cd, cp->Seq, cp->syncSeq);
    StartNewPacket(cp, CXT_PING);
    r = SendRequest(cp);
    if (r < 0)
    {
        MarkAsClosed(cp, errno);
        return -1;
    }

    return 0;
}
