#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "fix_arpa_inet.h"

#include "misclib.h"
#include "cxscheduler.h"
#include "fdiolib.h"

#include "cxsd_frontend.h"
#include "cxsd_logger.h"

#include "cx_proto_v4.h"
#include "endianconv.h"


enum
{
    MAX_UNCONNECTED_TIME = 60,
};


static  int            my_server_instance;
static  int            my_rcc;

static  int            unix_entry    = -1;    /* Socket to listen AF_UNIX */
static  fdio_handle_t  unix_iohandle = -1;

static  int            inet_entry    = -1;    /* Socket to listen AF_INET */
static  fdio_handle_t  inet_iohandle = -1;


//////////////////////////////////////////////////////////////////////
enum { MAX_ALLOWED_FD = FD_SETSIZE - 1 };
enum {
    RCC_SHIFT = MAX_ALLOWED_FD + 1,
    RCC_SIZE  = (1 << 30) / RCC_SHIFT
};

static inline void InitID(int *rcc)
{
    (*rcc) = 12345*0 +
#if OPTION_HAS_RANDOM
             random()
#else
             rand()
#endif /* OPTION_HAS_RANDOM */
    ;
}
static inline uint32 CreateNewID(int s, int *rcc)
{
    (*rcc) = ((*rcc) + 1) % RCC_SIZE;
    return s + (*rcc) * RCC_SHIFT;
}
//////////////////////////////////////////////////////////////////////

enum
{
    CS_OPEN_FRESH = 0,  // Just allocated
    CS_OPEN_ENDIANID,   // Waiting for CXT4_ENDIANID
    CS_OPEN_LOGIN,      // Waiting for CXT4_LOGIN

    CS_USABLE,          // The connection is full-operational if >=
    CS_READY,           // Is ready for I/O
};

typedef struct
{
    int            in_use;
    int            state;

    int            fd;
    fdio_handle_t  fhandle;
    sl_tid_t       clp_tid;

    uint32         ID;
    time_t         when;
    uint32         ip;
    struct sockaddr addr;

    int            endianness;
    char           progname[40];
    char           username[40];
} v4clnt_t;


enum
{
    V4_MAX_CONNS = 1000,  // An arbitrary limit
    V4_ALLOC_INC = 2,     // Must be >1 (to provide growth from 0 to 2)
};

static v4clnt_t *cx4clnts_list        = NULL;
static int       cx4clnts_list_allocd = 0;

//////////////////////////////////////////////////////////////////////

// GetV4connSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, V4conn, v4clnt_t,
                                 cx4clnts, in_use, 0, 1,
                                 1, V4_ALLOC_INC, V4_MAX_CONNS,
                                 , , void)

static void RlsV4connSlot(int cd)
{
  v4clnt_t  *cp  = AccessV4connSlot(cd);
  int        err = errno;        // To preserve errno
  
    if (cp->in_use == 0) return; /*!!! In fact, an internal error */
    
    cp->in_use = 0;
    
    ////////////
    if (cp->fhandle >= 0) fdio_deregister(cp->fhandle);
    if (cp->fd      >= 0) close          (cp->fd);
    if (cp->clp_tid >= 0) sl_deq_tout    (cp->clp_tid);
    
    errno = err;
}

static inline int cp2cd(v4clnt_t *cp)
{
    return cp - cx4clnts_list;
}

//////////////////////////////////////////////////////////////////////

static uint32 host_i32(v4clnt_t *cp, uint32 clnt_int32)
{
    if      (cp->endianness == FDIO_LEN_LITTLE_ENDIAN) return l2h_i32(clnt_int32);
    else if (cp->endianness == FDIO_LEN_BIG_ENDIAN)    return b2h_i32(clnt_int32);
    else                                               return         clnt_int32;
}

static uint32 clnt_i32(v4clnt_t *cp, uint32 host_int32)
{
    if      (cp->endianness == FDIO_LEN_LITTLE_ENDIAN) return h2l_i32(host_int32);
    else if (cp->endianness == FDIO_LEN_BIG_ENDIAN)    return h2b_i32(host_int32);
    else                                               return         host_int32;
}

static int sanitize_strzcpy(char *dst, size_t dst_size,
                            char *src, size_t src_size)
{
  int  ret = 0;
  int  x;
  
    for (x = 0;
         x < dst_size  &&  x < src_size;
         x++)
    {
        dst[x] = src[x];
        if (dst[x] == '\0') break;
        if (!isprint(dst[x]))
        {
            dst[x] = '?';
            ret = 1;
        }
    }
    dst[dst_size - 1] = '\0';

    return ret;
}

//////////////////////////////////////////////////////////////////////

/*
 *  CreateSockets
 *          Creates two sockets which are to be listened, binds them
 *      to addresses and marks as "listening".
 *
 *  Effect:
 *      inet_entry  the socket is bound to INADDR_ANY.CX_V4_INET_PORT+instance
 *      unix_entry  the socket is bound to CX_V4_UNIX_ADDR_FMT(instance)
 *
 *  Note:
 *      `inet_entry' should be bound first, since it will fail if
 *      INET port is busy. Otherwise the following will happen if
 *      the daemon is started twice: it unlink()s CX_UNIX_ADDR, bind()s
 *      this name to its own socket, and then fails to bind() inet_entry.
 *      So, it will just leave a working copy without local entry.
 */

#define Failure(msg) \
    do {             \
        logline(LOGF_SYSTEM | LOGF_ERRNO, LOGL_EMERG, "%s::%s: %s", __FILE__, __FUNCTION__, msg); return -1; \
        return -1;                                                                                           \
    } while (0)

static int CreateSockets(void)
{
  int                 r;                /* Result of system calls */
  struct sockaddr_in  iaddr;            /* Address to bind `inet_entry' to */
  struct sockaddr_un  uaddr;            /* Address to bind `unix_entry' to */
  int                 on     = 1;       /* "1" for REUSE_ADDR */
  char                unix_name[PATH_MAX]; /* Buffer to hold unix socket name */

    /* 1. INET socket */

    /* Create the socket */
    inet_entry = socket(AF_INET, SOCK_STREAM, 0);
    if (inet_entry < 0)
        Failure("socket(inet_entry)");

    /* Bind it to the name */
    setsockopt(inet_entry, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    iaddr.sin_family      = AF_INET;
    iaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    iaddr.sin_port        = htons(CX_V4_INET_PORT + my_server_instance);
    r = bind(inet_entry, (struct sockaddr *) &iaddr,
             sizeof(iaddr));
    if (r < 0)
            Failure("bind(inet_entry)");

    /* Mark it as listening and non-blocking */
    listen(inet_entry, 5);
    set_fd_flags(inet_entry, O_NONBLOCK, 1);

    
    /* 2. UNIX socket */

    /* Create the socket */
    unix_entry = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_entry < 0)
            Failure("socket(unix_entry)");

    /* Bind it to the name */
    sprintf(unix_name, CX_V4_UNIX_ADDR_FMT, my_server_instance); /*!!!snprintf()?*/
    unlink (unix_name);
    strcpy(uaddr.sun_path, unix_name);
    uaddr.sun_family = AF_UNIX;
    r = bind(unix_entry, (struct sockaddr *) &uaddr,
             offsetof(struct sockaddr_un, sun_path) + strlen(uaddr.sun_path));
    if (r < 0)
            Failure("bind(unix_entry)");

    /* Mark it as listening and non-blocking */
    listen(unix_entry, 5);
    set_fd_flags(unix_entry, O_NONBLOCK, 1);
    
    /* Set it to be world-accessible (bind() is affected by umask in Linux) */
    chmod(unix_name, 0777);

    return 0;
}

static void DestroySockets(int and_deregister)
{
  char                unix_name[PATH_MAX]; /* Buffer to hold unix socket name */
  
    if (inet_entry >= 0)
    {
        if (and_deregister) fdio_deregister(inet_iohandle), inet_iohandle = -1;
        close(inet_entry);
    }
    if (unix_entry >= 0)
    {
        if (and_deregister) fdio_deregister(unix_iohandle), unix_iohandle = -1;
        close(unix_entry);

        sprintf(unix_name, CX_V4_UNIX_ADDR_FMT, my_server_instance); /*!!!snprintf()?*/
        unlink (unix_name);
    }
    
    inet_entry = unix_entry = -1;
    
}

//////////////////////////////////////////////////////////////////////

enum
{
    ERR_CLOSED = -1,
    ERR_KILLED = -2,
    ERR_IN2BIG = -3,
    ERR_HSHAKE = -4,
    ERR_TIMOUT = -5,
    ERR_INTRNL = -6,
};

static void DisconnectClient(v4clnt_t *cp, int err, uint32 code)
{
  const char *reason_s;
  const char *errdescr_s;
  CxV4Header  hdr;
  char        from[100];
    
    /* Log the case... */
    errdescr_s = "";
    if      (err == 0)          reason_s = "disconnect by";
    else if (err == ERR_CLOSED) reason_s = "conn-socket closed by";
    else if (err == ERR_KILLED) reason_s = "killed";
    else if (err == ERR_IN2BIG) reason_s = "overlong packet from";
    else if (err == ERR_HSHAKE) reason_s = "handshake error, dropping";
    else if (err == ERR_TIMOUT) reason_s = "handshake timeout, dropping";
    else if (err == ERR_INTRNL) reason_s = "internal error, disconnecting";
    else                        reason_s = "abnormally disconnected",
                                errdescr_s = strerror(err);
  
    if (cp->ip == 0)
        strcpy(from, "-");
    else
        sprintf(from, "%d.%d.%d.%d",
                      (cp->ip >> 24) & 255,
                      (cp->ip >> 16) & 255,
                      (cp->ip >>  8) & 255,
                      (cp->ip >>  0) & 255);

    logline(LOGF_ACCESS, LOGL_NOTICE, "[%d] %s %s@%s:%s%s%s",
            cp2cd(cp),
            reason_s,
            cp->username[0] != '\0'? cp->username : "UNKNOWN",
            0                      ? ""           : from,
            cp->progname[0] != '\0'? cp->progname : "UNKNOWN",
            errdescr_s[0]   != '\0'? ": " : "",
            errdescr_s);

    /* Should try to tell him something? */
    if (code != 0)
    {
        bzero(&hdr, sizeof(hdr));
        if      (cp->endianness == FDIO_LEN_LITTLE_ENDIAN)
            hdr.Type = h2l_i32(code);
        else if (cp->endianness == FDIO_LEN_BIG_ENDIAN)
            hdr.Type = h2b_i32(code);
        else
            hdr.Type = code;

        fdio_send(cp->fhandle, &hdr, sizeof(hdr));
    }

    /* Do "preventive" closes */
    fdio_deregister_flush(cp->fhandle, 10/*!!!Should make an enum-constant*/);
    cp->fhandle = -1;
    cp->fd      = -1;

    RlsV4connSlot(cp2cd(cp));
}

static void HandleClientRequest(v4clnt_t *cp, CxV4Header *hdr, size_t inpktsize)
{
  struct
  {
      CxV4Header    hdr;
  } pkt;

    switch (host_i32(cp, hdr->Type))
    {
        case CXT4_LOGOUT:
            DisconnectClient(cp, 0, 0);
            break;

        case CXT4_PING:
            break;

        case CXT4_PONG:
            break;

        default:
            bzero(&pkt, sizeof(pkt));
            pkt.hdr.Type = clnt_i32(cp, CXT4_EBADRQC);
            pkt.hdr.ID   = clnt_i32(cp, cp->ID);
            pkt.hdr.Seq  = hdr->Seq;  /* Direct copy -- no need for (double) conversion */
            pkt.hdr.var1 = hdr->Type; /* The same */
            if (fdio_send(cp->fhandle, &pkt, sizeof(pkt)) < 0)
            {
                DisconnectClient(cp, fdio_lasterr(cp->fhandle), 0);
                return;
            }
    }
}

static void HandleClientConnect(v4clnt_t *cp, CxV4Header *hdr, size_t inpktsize)
{
  uint32        Type;
  uint32        exp_Type     = 0;
  const char   *exp_TypeName = NULL;
  uint32        cln_ver;
  CxV4LoginRec *lr = (CxV4LoginRec *)hdr->data;
  const char   *from;
  
  struct
  {
      CxV4Header    hdr;
  } pkt;
  
    Type = hdr->Type;
    if      (cp->endianness == FDIO_LEN_LITTLE_ENDIAN) Type = l2h_i32(Type);
    else if (cp->endianness == FDIO_LEN_BIG_ENDIAN)    Type = b2h_i32(Type);

    switch (cp->state)
    {
        case CS_OPEN_ENDIANID:
            exp_Type     =                CXT4_ENDIANID;
            exp_TypeName = __CX_STRINGIZE(CXT4_ENDIANID);
            break;

        case CS_OPEN_LOGIN:
            exp_Type     =                CXT4_LOGIN;
            exp_TypeName = __CX_STRINGIZE(CXT4_LOGIN);
            break;
    }

    if (Type != exp_Type)
    {
        logline(LOGF_ACCESS, LOGL_ERR, "%s::%s: packet.Type mismatch: received 0x%08x instead of %s=0x%08x",
                __FILE__, __FUNCTION__, Type, exp_TypeName, exp_Type);
        DisconnectClient(cp, ERR_HSHAKE, CXT4_EINVAL);
        return;
    }
    
    switch (cp->state)
    {
        case CS_OPEN_ENDIANID:
            /* First, determine endianness... */
            if      (l2h_i32(hdr->var1) == CXV4_VAR1_ENDIANNESS_SIG)
                cp->endianness = FDIO_LEN_LITTLE_ENDIAN;
            else if (b2h_i32(hdr->var1) == CXV4_VAR1_ENDIANNESS_SIG)
                cp->endianness = FDIO_LEN_BIG_ENDIAN;
            else
            {
                logline(LOGF_ACCESS, LOGL_ERR, "%s::%s: unrecognizable endianness-signature 0x%08x",
                        __FILE__, __FUNCTION__, hdr->var1);
                DisconnectClient(cp, ERR_HSHAKE, CXT4_EINVAL);
                return;
            }

            /* ...and update fdiolib's info */
            fdio_set_len_endian(cp->fhandle, cp->endianness);
            fdio_set_maxpktsize(cp->fhandle, CX_V4_MAX_PKTSIZE);

            /* Next, check version */
            cln_ver = host_i32(cp, hdr->var2);
            if (!CX_VERSION_IS_COMPATIBLE(cln_ver, CX_V4_PROTO_VERSION))
            {
                logline(LOGF_ACCESS, LOGL_ERR, "%s::%s: client's protocol version %d.%d(%d) is incompatible with our %d.%d",
                        __FILE__, __FUNCTION__,
                        CX_MAJOR_OF(cln_ver), CX_MINOR_OF(cln_ver), cln_ver,
                        CX_V4_PROTO_VERSION_MAJOR, CX_V4_PROTO_VERSION_MINOR);
                DisconnectClient(cp, ERR_HSHAKE, CXT4_DIFFPROTO);
                return;
            }

            /* Okay, let's command it to send login info... */
            bzero(&pkt, sizeof(pkt));
            pkt.hdr.Type = clnt_i32(cp, CXT4_LOGIN);
            pkt.hdr.ID   = clnt_i32(cp, cp->ID);
            pkt.hdr.Seq  = hdr->Seq; /* Direct copy -- no need for (double) conversion */
            pkt.hdr.var2 = clnt_i32(cp, CX_V4_PROTO_VERSION);
            if (fdio_send(cp->fhandle, &pkt, sizeof(pkt)) < 0)
            {
                DisconnectClient(cp, fdio_lasterr(cp->fhandle), 0);
                return;
            }

            /* ...and promote state */
            cp->state = CS_OPEN_LOGIN;
            break;

        case CS_OPEN_LOGIN:
            /* Is packet size right? */
            if (inpktsize != sizeof(CxV4Header) + sizeof(CxV4LoginRec))
            {
                logline(LOGF_ACCESS, LOGL_ERR, "%s::%s: inpktsize=%zu, while %zu expected",
                        __FILE__, __FUNCTION__,
                        inpktsize,
                        sizeof(CxV4Header) + sizeof(CxV4LoginRec));
                DisconnectClient(cp, ERR_HSHAKE, CXT4_EINVAL);
                return;
            }

            sl_deq_tout(cp->clp_tid);
            cp->clp_tid = -1;

            /* Remember login info */
            sanitize_strzcpy(cp->progname, sizeof(cp->progname),
                             lr->progname, sizeof(lr->progname));
            sanitize_strzcpy(cp->username, sizeof(cp->username),
                             lr->username, sizeof(lr->username));

            /* Make client "logged-in" */
            bzero(&pkt, sizeof(pkt));
            pkt.hdr.Type = clnt_i32(cp, CXT4_ACCESS_GRANTED);
            pkt.hdr.ID   = clnt_i32(cp, cp->ID);
            pkt.hdr.Seq  = hdr->Seq; /* Direct copy -- no need for (double) conversion */
            pkt.hdr.var2 = clnt_i32(cp, CX_V4_PROTO_VERSION);
            if (fdio_send(cp->fhandle, &pkt, sizeof(pkt)) < 0)
            {
                DisconnectClient(cp, fdio_lasterr(cp->fhandle), 0);
                return;
            }


            if (cp->ip == 0) from = "-";
            else
            {
                from = inet_ntoa(((struct sockaddr_in *)&(cp->addr))->sin_addr);
            }

            logline(LOGF_ACCESS, LOGL_NOTICE, "%s [%d] %s@%s:%s",
                    "connect",
                    cp2cd(cp),
                    cp->username[0] != '\0'? cp->username : "UNKNOWN",
                    0                      ? ""           : from,
                    cp->progname[0] != '\0'? cp->progname : "UNKNOWN");

            cp->state = CS_READY;
            break;
    }
}

static void InteractWithClient(int uniq, void *unsdptr,
                               fdio_handle_t handle, int reason,
                               void *inpkt, size_t inpktsize,
                               void *privptr)
{
  int         cd   = ptr2lint(privptr);
  v4clnt_t   *cp   = AccessV4connSlot(cd);
  CxV4Header *hdr  = inpkt;

    switch (reason)
    {
        case FDIO_R_DATA:
            if (cp->state > CS_USABLE) HandleClientRequest(cp, hdr, inpktsize);
            else                       HandleClientConnect(cp, hdr, inpktsize);
            break;
            
        case FDIO_R_CLOSED:
            DisconnectClient(cp, ERR_CLOSED,           0);
            break;

        case FDIO_R_IOERR:
            DisconnectClient(cp, fdio_lasterr(handle), 0);
            break;

        case FDIO_R_PROTOERR:
            /* In fact, as of 10.09.2009, FDIO_R_PROTOERR is unused by fdiolib */
            DisconnectClient(cp, fdio_lasterr(handle), CXT4_EBADRQC);
            break;

        case FDIO_R_INPKT2BIG:
            DisconnectClient(cp, ERR_IN2BIG,           CXT4_EPKT2BIG);
            break;

        case FDIO_R_ENOMEM:
            DisconnectClient(cp, ENOMEM,               CXT4_ENOMEM);
            break;

        default:
            logline(LOGF_ACCESS, LOGL_ERR, "%s::%s: unknown fdiolib reason %d",
                         __FILE__, __FUNCTION__, reason);
            DisconnectClient(cp, ERR_INTRNL,           CXT4_EINTERNAL);
    }
}

//////////////////////////////////////////////////////////////////////

static void WipeUnconnectedTimed(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  int       cd = ptr2lint(privptr);
  v4clnt_t *cp = AccessV4connSlot(cd);

    cp->clp_tid = -1;
    DisconnectClient(cp, ERR_TIMOUT, CXT4_TIMEOUT);
}

static void RefuseConnection(int s, int cause)
{
  CxV4Header  hdr;
    
    bzero(&hdr, sizeof(hdr));
    hdr.Type = cause;
    hdr.var2 = CX_V4_PROTO_VERSION;
    write(s, &hdr, sizeof(hdr));
}

static void AcceptCXv4Connection(int uniq, void *unsdptr,
                                 fdio_handle_t handle, int reason,
                                 void *inpkt, size_t inpktsize,
                                 void *privptr)
{
  int                 s;
  struct sockaddr     addr;
  socklen_t           addrlen;

  int                 cd;
  v4clnt_t           *cp;
  fdio_handle_t       fhandle;
  
    if (reason != FDIO_R_ACCEPT)
    {
        logline(LOGF_SYSTEM, LOGL_ERR, "%s::%s(handle=%d): reason=%d, !=FDIO_R_ACCEPT=%d",
                __FILE__, __FUNCTION__, handle, reason, FDIO_R_ACCEPT);
        return;
    }

    /* First, accept() the connection */
    addrlen = sizeof(addr);
    s = fdio_accept(handle, &addr, &addrlen);

    /* Is anything wrong? */
    if (s < 0)
    {
        logline(LOGF_SYSTEM | LOGF_ERRNO, LOGL_ERR,
                "%s(): fdio_accept(%s)",
                __FUNCTION__,
                handle == unix_iohandle? "unix_iohandle" : "inet_iohandle");
        return;
    }

    /* Instantly switch it to non-blocking mode */
    if (set_fd_flags(s, O_NONBLOCK, 1) < 0)
    {
        logline(LOGF_SYSTEM | LOGF_ERRNO, LOGL_ERR,
                "%s(): set_fd_flags(O_NONBLOCK)",
                __FUNCTION__);
        close(s);
        return;
    }

    /* Is it allowed to log in? */
    if (0)
    {
        RefuseConnection(s, CXT4_ACCESS_DENIED);
        close(s);
        return;
    }

    /* Get a connection slot... */
    if ((cd = GetV4connSlot()) < 0)
    {
        RefuseConnection(s, CXT4_MANY_CLIENTS);
        close(s);
        return;
    }

    /* ...and fill it with data */
    cp = AccessV4connSlot(cd);
    cp->state = CS_OPEN_ENDIANID;
    cp->fd    = s;
    cp->ID    = CreateNewID(cp->fd, &my_rcc);
    cp->when  = time(NULL);
    cp->ip    = handle == unix_iohandle ?  0
                                        :  ntohl(inet_addr(inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr)));
    cp->addr  = addr;
    
    /* Okay, let's obtain an fdio slot... */
    cp->fhandle = fdio_register_fd(0, NULL, /*!!!uniq*/
                                   s, FDIO_STREAM,
                                   InteractWithClient, lint2ptr(cd),
                                   sizeof(CxV4Header),
                                   sizeof(CxV4Header),
                                   FDIO_OFFSET_OF(CxV4Header, DataSize),
                                   FDIO_SIZEOF   (CxV4Header, DataSize),
                                   FDIO_LEN_LITTLE_ENDIAN,
                                   1, sizeof(CxV4Header));
    if (cp->fhandle < 0)
    {
        RefuseConnection(s, CXT4_MANY_CLIENTS);
        RlsV4connSlot(cd);
        return;
    }

    cp->clp_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                    MAX_UNCONNECTED_TIME * 1000000,
                                    WipeUnconnectedTimed, lint2ptr(cd));
    if (cp->clp_tid < 0)
    {
        RefuseConnection(s, CXT4_MANY_CLIENTS);
        RlsV4connSlot(cd);
        return;
    }
}

//////////////////////////////////////////////////////////////////////

#define CHECK_R(msg)                                                  \
    do {                                                              \
        if (r != 0)                                                   \
        {                                                             \
            logline(LOGF_SYSTEM | LOGF_ERRNO, LOGL_EMERG, "%s::%s: %s", \
                    __FILE__, __FUNCTION__, msg);                     \
            return -1;                                                \
        }                                                             \
    } while (0)

static int  cx_init_f (int server_instance)
{
  int  r;
    
    my_server_instance = server_instance;
    InitID(&my_rcc);
  
    if (CreateSockets() != 0)
    {
        DestroySockets(0);
        return -1;
    }

    inet_iohandle = fdio_register_fd(0, NULL, /*!!!uniq*/
                                     inet_entry,       FDIO_LISTEN,
                                     AcceptCXv4Connection, NULL,
                                     0, 0, 0, 0, FDIO_LEN_LITTLE_ENDIAN, 0, 0);
    r = inet_iohandle < 0;
    CHECK_R("fdio_register_fd(inet_entry)");
    unix_iohandle = fdio_register_fd(0, NULL, /*!!!uniq*/
                                     unix_entry,       FDIO_LISTEN,
                                     AcceptCXv4Connection, NULL,
                                     0, 0, 0, 0, FDIO_LEN_LITTLE_ENDIAN, 0, 0);
    r = unix_iohandle < 0;
    CHECK_R("fdio_register_fd(unix_entry)");
    
    return 0;
}

static int TermIterator(v4clnt_t *cp, void *privptr __attribute__((unused)))
{
    DisconnectClient(cp, ERR_KILLED, CXT4_DISCONNECT);

    return 0;
}

static void cx_term_f (void)
{
    DestroySockets(1);
    ForeachV4connSlot(TermIterator, NULL);
    DestroyV4connSlotArray();
}

static void cx_begin_c(void)
{
}

static void cx_end_c  (void)
{
}


static int  cx_init_m(void);
static void cx_term_m(void);

DEFINE_CXSD_FRONTEND(cx, "CX frontend",
                     cx_init_m, cx_term_m,
                     cx_init_f, cx_term_f,
                     cx_begin_c, cx_end_c);

static int  cx_init_m(void)
{
    return RegisterFrontend(&(CXSD_FRONTEND_MODREC_NAME(cx)));
}

static void cx_term_m(void)
{
    DeregisterFrontend(&(CXSD_FRONTEND_MODREC_NAME(cx)));
}
