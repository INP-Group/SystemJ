#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fix_arpa_inet.h>

#include "cx_sysdeps.h"

#include "cxsd_driver.h"
#include "remdrv_proto.h"


typedef enum
{
    DRVLET_LITTLE_ENDIAN = 0,
    DRVLET_BIG_ENDIAN    = 1,
} drvlet_endianness_t;

typedef enum
{
    DRVLET_FORKED = 1,
    DRVLET_REMOTE = 2,
} drvlet_kind_t;

enum
{
    MAX_BUSINFOCOUNT = 20,           /* ==cxsd_db.h::CXSD_DB_MAX_BUSINFOCOUNT */
    RECONNECT_DELAY  = 10*1000*1000  /* 10 seconds */
};


//// Type definitions ////////////////////////////////////////////////

typedef int16 (*l2r_i16_t)       (int16 v);
typedef int32 (*l2r_i32_t)       (int32 v);
typedef void  (*memcpy_l2r_i16_t)(int16 *dst, int16 *src, int count);
typedef void  (*memcpy_l2r_i32_t)(int32 *dst, int32 *src, int count);

typedef int16 (*r2l_i16_t)       (int16 v);
typedef int32 (*r2l_i32_t)       (int32 v);
typedef void  (*memcpy_r2l_i16_t)(int16 *dst, int16 *src, int count);
typedef void  (*memcpy_r2l_i32_t)(int32 *dst, int32 *src, int count);

typedef struct
{
    drvlet_kind_t        kind;
    int                  is_ready;
    int                  is_reconnecting;
    int                  was_success;

    int                  fd;
    fdio_handle_t        iohandle;

    sl_tid_t             reconnect_tid;

    int                  businfocount;
    int32                businfo[MAX_BUSINFOCOUNT];
    char                 typename[100];
    drvlet_endianness_t  e;
    unsigned long        hostaddr;

    l2r_i16_t            l2r_i16;
    l2r_i32_t            l2r_i32;
    memcpy_l2r_i16_t     memcpy_l2r_i16;
    memcpy_l2r_i32_t     memcpy_l2r_i32;
    
    r2l_i16_t            r2l_i16;
    r2l_i32_t            r2l_i32;
    memcpy_r2l_i16_t     memcpy_r2l_i16;
    memcpy_r2l_i32_t     memcpy_r2l_i32;

    char                 drvletinfo[0];
} privrec_t;

//// Endian conversion routines //////////////////////////////////////

static int16 noop_i16(int16 v) {return v;}
static int32 noop_i32(int32 v) {return v;}
static void  noop_memcpy_i16(int16 *dst, int16 *src, int count)
{
    memcpy(dst, src, count * sizeof(*dst));
}
static void  noop_memcpy_i32(int32 *dst, int32 *src, int count)
{
    memcpy(dst, src, count * sizeof(*dst));
}

static inline int16 SwapBytes16(int16 x)
{
    return
        (((x & 0x00FF) << 8)) |
        (((x & 0xFF00) >> 8) & 0xFF);
}

 /*!!! BUGGY-BUGGY!!! Side-effects */
static inline int32 SwapBytes32(int32 x)
{
    return
        (((x & 0x000000FF) << 24)) |
        (((x & 0x0000FF00) <<  8)) |
        (((x & 0x00FF0000) >>  8)) |
        (((x & 0xFF000000) >> 24) & 0xFF);
}

static int16 swab_i16(int16 v) {return SwapBytes16(v);}
static int32 swab_i32(int32 v) {return SwapBytes32(v);}
static void  swab_memcpy_i16(int16 *dst, int16 *src, int count)
{
    while (count > 0) *dst++ = SwapBytes16(*src++);
}
static void  swab_memcpy_i32(int32 *dst, int32 *src, int count)
{
    while (count > 0) *dst++ = SwapBytes32(*src++);
}


//// ... /////////////////////////////////////////////////////////////

static void CleanupFDIO(int devid, privrec_t *me)
{
    if (me->iohandle >= 0)
    {
        fdio_deregister(me->iohandle);
        me->iohandle = -1;
    }
    if (me->fd     >= 0)
    {
        close(me->fd);
        me->fd       = -1;
    }
}

static void CleanupEVRT(int devid, privrec_t *me)
{
    CleanupFDIO(devid, me);
    if (me->reconnect_tid >= 0)
    {
        sl_deq_tout(me->reconnect_tid);
        me->reconnect_tid = -1;
    }
    RegisterDevPtr(devid, NULL); // In fact, an unneeded step
    safe_free(me);
}


static void CommitSuicide(int devid, privrec_t *me,
                          const char *function, const char *reason)
{
    DoDriverLog(devid, DRIVERLOG_CRIT, "%s: FATAL: %s", function, reason);
    CleanupEVRT(devid, me);
    SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, reason);
}

static void ReportConnfail(int devid, privrec_t *me, const char *function)
{
    if (!me->is_reconnecting)
        DoDriverLog(devid, DRIVERLOG_NOTICE | DRIVERLOG_ERRNO,
                    "%s: connect()", function);
}

static void ReportConnect (int devid, privrec_t *me)
{
    if (me->is_reconnecting)
        DoDriverLog(devid, DRIVERLOG_NOTICE,
                    "Successfully %sstarted the drivelet",
                    me->was_success? "re-" : "");
}

//////////////////////////////////////////////////////////////////////
static int  InitiateStart     (int devid, privrec_t *me);
static void InitializeDrivelet(int devid, privrec_t *me);
static void ProcessInData     (int devid, privrec_t *me,
                               remdrv_pkt_header_t *pkt, size_t inpktsize);
//////////////////////////////////////////////////////////////////////

static void TryToReconnect(int devid, void *devptr,
                           sl_tid_t tid,
                           void *privptr)
{
  privrec_t  *me  = (privrec_t *)devptr;
  int         r;
  
    me->reconnect_tid = -1;
    r = InitiateStart(devid, me);
    if (r < 0)
        CommitSuicide(devid, me, NULL, NULL);
}
static void ScheduleReconnect(int devid, privrec_t *me, const char *reason)
{
    /* Is it a forked-drivelet?  Then no way... */
    if (me->kind == DRVLET_FORKED)
    {
        CommitSuicide(devid, me, __FUNCTION__, "reconnect is impossible for local fork()-drivelets");
        return;
    }

    /* Perform cleanup and notify server of our problem */
    CleanupFDIO(devid, me);
    SetDevState(devid, DEVSTATE_NOTREADY, CXRF_REM_C_PROBL, reason);

    /* And do schedule reconnect */
    me->is_ready        = 0;
    me->is_reconnecting = 1;
    me->reconnect_tid   = sl_enq_tout_after(devid, me, RECONNECT_DELAY,
                                            TryToReconnect, NULL);
}


static void ProcessIO(int devid, void *devptr,
                      fdio_handle_t iohandle __attribute__((unused)), int reason,
                      void *inpkt, size_t inpktsize,
                      void *privptr __attribute__((unused)))
{
  privrec_t           *me  = (privrec_t *)devptr;

    switch (reason)
    {
        case FDIO_R_DATA:
            ProcessInData     (devid, me, inpkt, inpktsize);
            break;
            
        case FDIO_R_CONNECTED:
            InitializeDrivelet(devid, me);
            break;

        case FDIO_R_CONNERR:
            ReportConnfail(devid, me, __FUNCTION__);
            ScheduleReconnect(devid, me, "connection failure");
            break;

        case FDIO_R_CLOSED:
        case FDIO_R_IOERR:
            DoDriverLog(devid, DRIVERLOG_NOTICE | DRIVERLOG_ERRNO,
                        "%s: ERROR", __FUNCTION__);
            ScheduleReconnect(devid, me,
                              reason == FDIO_R_CLOSED? "connection closed"
                                                     : "I/O error");
            break;

        case FDIO_R_PROTOERR:
            CommitSuicide(devid, me, __FUNCTION__, "protocol error");
            break;

        case FDIO_R_INPKT2BIG:
            CommitSuicide(devid, me, __FUNCTION__, "received packet is too big");
            break;

        case FDIO_R_ENOMEM:
            CommitSuicide(devid, me, __FUNCTION__, "out of memory");
            break;

        default:
            DoDriverLog(devid, DRIVERLOG_CRIT,
                        "%s: unknown fdiolib reason %d",
                         __FUNCTION__, reason);
            CommitSuicide(devid, me, __FUNCTION__, "unknown fdiolib reason");
    }
}


static int  RegisterWithFDIO  (int devid, privrec_t *me, int fdtype)
{
    me->iohandle = fdio_register_fd(devid, me, me->fd, fdtype,
                                    ProcessIO, NULL,
                                    REMDRV_MAXPKTSIZE,
                                    sizeof(remdrv_pkt_header_t),
                                    FDIO_OFFSET_OF(remdrv_pkt_header_t, pktsize),
                                    FDIO_SIZEOF   (remdrv_pkt_header_t, pktsize),
                                    me->e == DRVLET_LITTLE_ENDIAN? FDIO_LEN_LITTLE_ENDIAN
                                                                 : FDIO_LEN_BIG_ENDIAN,
                                    1,
                                    0);
    if (me->iohandle < 0)
    {
        DoDriverLog(devid, DRIVERLOG_CRIT | DRIVERLOG_ERRNO,
                    "%s: FATAL: fdio_register_fd()", __FUNCTION__);
        return -CXRF_DRV_PROBL;
    }

    return 0;
}

/* Our local copy of remdrvlet_report() -- for failed-to-exec, with shrunk 'data' buffer */
static void remdrvlet_report(int fd, int code, const char *format, ...)
{
  struct {remdrv_pkt_header_t hdr;  int32 data[100];} pkt;
  int      len;
  va_list  ap;

    va_start(ap, format);
    len = vsnprintf((char *)(pkt.data), sizeof(pkt.data), format, ap);
    va_end(ap);

    bzero(&pkt.hdr, sizeof(pkt.hdr));
    pkt.hdr.pktsize = sizeof(pkt.hdr) + len + 1;
    pkt.hdr.command = code;

    write(fd, &pkt, pkt.hdr.pktsize);
}

static int  InitiateStart     (int devid, privrec_t *me)
{
  int                 fdtype;

  int                 pair[2];
  const char         *drvletdir;
  char                path[PATH_MAX];
  char               *args[2];

  struct sockaddr_in  idst;
  socklen_t           addrlen;
  int                 r;
  int                 on    = 1;

    if (me->kind == DRVLET_FORKED)
    {
        fdtype = FDIO_STREAM;

        /* a. Create a pair of interconnected sockets */
        /* Note: [0] is master's, [1] is drivelet's */
        r = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
        if (r != 0)
        {
            DoDriverLog(devid, DRIVERLOG_CRIT | DRIVERLOG_ERRNO, "socketpair()");
            return -CXRF_DRV_PROBL;
        }

        /* b. Prepare the name for exec */
        drvletdir = GetLayerInfo("remdrv_fork_drvlets_dir", -1);
        if (drvletdir == NULL)
            drvletdir = "lib/server/drivers"; /*!!!DIRSTRUCT*/
        check_snprintf(path, sizeof(path), "%s/%s.drvlet",
                       drvletdir, me->typename);
        
        /* c. Fork into separate process */
        r = fork();
        switch (r)
        {
            case  0:  /* Child */
                /* Clone connection fd into stdin/stdout. !!!Don't touch stderr!!! */
                dup2 (pair[1], 0);  fcntl(0, F_SETFD, 0);
                dup2 (pair[1], 1);  fcntl(1, F_SETFD, 0);
                close(pair[1]); // Close original descriptor...
                close(pair[0]); // ...and our copy of master's one

                /* Exec the required program */
                args[0] = path;
                args[1] = NULL;
                r = execv(path, args);

                /* Failed to exec? */
                remdrvlet_report(1, REMDRV_C_RRUNDP, "can't execv(\"%s\"): %s",
                                 path, strerror(errno));
                _exit(0);
                /* No need for "break" here */

            case -1:  /* Failed to fork */
                DoDriverLog(devid, DRIVERLOG_CRIT | DRIVERLOG_ERRNO, "fork()");
                close(pair[0]);
                close(pair[1]);
                return -CXRF_DRV_PROBL;

            default:  /* Parent */
                me->fd = pair[0];
                close(pair[1]); // Close drivelet's end
        }
    }
    else
    {
        fdtype = FDIO_CONNECTING;

        /* Create a socket */
        me->fd = socket(AF_INET, SOCK_STREAM, 0);
        if (me->fd < 0)
        {
            DoDriverLog(devid, DRIVERLOG_CRIT | DRIVERLOG_ERRNO,
                        "%s: FATAL: socket()", __FUNCTION__);
            return -CXRF_DRV_PROBL;
        }
    
        idst.sin_family = AF_INET;
        idst.sin_port   = htons(REMDRV_DEFAULT_PORT);
        memcpy(&idst.sin_addr, &(me->hostaddr), sizeof(me->hostaddr));
        addrlen = sizeof(idst);
    
        /* Turn on TCP keepalives, ... */
        r = setsockopt(me->fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
        /* ...switch to non-blocking mode... */
        r = set_fd_flags(me->fd, O_NONBLOCK, 1);
        /* ...and initiate connection */
        r = connect(me->fd, (struct sockaddr *)&idst, addrlen);
        if (r < 0)
        {
            if (errno == EINPROGRESS)
                ;
            else if (IS_A_TEMPORARY_CONNECT_ERROR())
            {
                ReportConnfail(devid, me, __FUNCTION__);
                ScheduleReconnect(devid, me, "connection failure");
                return 0;
            }
            else
            {
                DoDriverLog(devid, DRIVERLOG_CRIT | DRIVERLOG_ERRNO,
                            "%s: FATAL: connect()", __FUNCTION__);
                close(me->fd); me->fd = -1;
                return -CXRF_DRV_PROBL;
            }
        }
    }

    /* Register with fdio */
    r = RegisterWithFDIO(devid, me, fdtype);
    if (r < 0)
    {
        close(me->fd); me->fd = -1;
        return r;
    }

    /* Okay -- the connection will be completed later */
    return 0;
}

static void InitializeDrivelet(int devid, privrec_t *me)
{
  struct
  {
      remdrv_pkt_header_t  hdr;
      int32                businfo[MAX_BUSINFOCOUNT];
  }            pkt;
  
  size_t       pktsize;

  static char  newline[] = "\n";

    /* Prepare the CONFIG packet */
    pktsize = sizeof(pkt.hdr)                  +
              sizeof(int32) * me->businfocount +
              strlen(me->drvletinfo) + 1;

    pkt.hdr.pktsize                  = me->l2r_i32(pktsize);
    pkt.hdr.command                  = me->l2r_i32(REMDRV_C_CONFIG);
    pkt.hdr.var.config.businfocount  = me->l2r_i32(me->businfocount);
    pkt.hdr.var.config.proto_version = me->l2r_i32(REMDRV_PROTO_VERSION);
    if (me->businfocount > 0)
        me->memcpy_l2r_i32(pkt.businfo, me->businfo, me->businfocount);

    /* Send driver name and CONFIG */
    if (fdio_send(me->iohandle, me->typename,   strlen(me->typename)) < 0
        ||
        fdio_send(me->iohandle, newline,        strlen(newline))      < 0
        ||
        fdio_send(me->iohandle, &pkt,           sizeof(pkt.hdr) + sizeof(int32) * me->businfocount) < 0
        ||
        fdio_send(me->iohandle, me->drvletinfo, strlen(me->drvletinfo) + 1) < 0)
        goto FAILURE;

    /* Proclaim that we are ready */
    ReportConnect(devid, me);
    me->is_ready        = 1;
    me->is_reconnecting = 0;
    me->was_success     = 1;
    SetDevState(devid, DEVSTATE_OPERATING, 0, NULL);

    return;
    
 FAILURE:
    DoDriverLog(devid, DRIVERLOG_CRIT | DRIVERLOG_ERRNO, __FUNCTION__);
    ScheduleReconnect(devid, me, "drivelet initialization failure");
}


//// Driver's methods ////////////////////////////////////////////////

static int  remdrv_init_d(int devid, void *devptr,
                          int businfocount, int businfo[],
                          const char *auxinfo)
{
  privrec_t          *me;
  int                 r;
  int                 x;
  
  const char         *typename;

  const char         *p   = auxinfo;
  const char         *endp;

  char                ec;
  char                hostname[100];
  size_t              hostnamelen;
  const char         *drvletinfo;
  size_t              drvletinfolen;
  struct hostent     *hp;
  unsigned long       hostaddr;

  static drvlet_endianness_t local_e =
#if   BYTE_ORDER == LITTLE_ENDIAN
      DRVLET_LITTLE_ENDIAN
#elif BYTE_ORDER == BIG_ENDIAN
      DRVLET_BIG_ENDIAN
#else
      -1
#error Sorry, the "BYTE_ORDER" is neither "LITTLE_ENDIAN" nor "BIG_ENDIAN"
#endif
      ;

    typename = GetDevTypename(devid);

    /* Check if we aren't run directly */
    if (typename == NULL  ||  *typename == '\0')
    {
        DoDriverLog(devid, DRIVERLOG_ERR, "%s: this driver isn't intended to be run directly, only as TYPENAME/remdrv",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }


    /* I. Split auxinfo into endianness:host and optional drvletinfo */
    /* ("e:host[ drvletinfo]") */

    /* 0. Is there anything at all? */
    if (auxinfo == NULL  ||  auxinfo[0] == '\0')
    {
        DoDriverLog(devid, DRIVERLOG_ERR, "%s: auxinfo is empty...", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    /* 1. Endianness */
    ec = toupper(*p++);
    if (ec != 'L'  ||  ec != 'B'  ||  *p++ != ':')
    {
        DoDriverLog(devid, DRIVERLOG_ERR, "%s: auxinfo should start with either 'l:' or 'b:'",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    /* 2. Host */
    endp = p;
    while (*endp != '\0'  &&  !isspace(*endp)) endp++;
    hostnamelen = endp - p;
    if (hostnamelen == 0)
    {
        DoDriverLog(devid, DRIVERLOG_ERR, "%s: HOST is expected after '%c:'",
                    __FUNCTION__, ec);
        return -CXRF_CFG_PROBL;
    }
    if (hostnamelen > sizeof(hostname) - 1)
    {
        DoDriverLog(devid, DRIVERLOG_WARNING,
                    "%s: HOST is %zu characters long, >%zu",
                    __FUNCTION__, hostnamelen, sizeof(hostname) - 1);
        hostnamelen = sizeof(hostname) - 1;
    }
    memcpy(hostname, p, hostnamelen);
    hostname[hostnamelen] = '\0';

    /* 3. Drvletinfo */
    p = endp;
    while (*p != '\0'  &&  isspace(*p)) p++; // Skip whitespace
    drvletinfo = p;
    drvletinfolen = strlen(drvletinfo + 1);

    /* (4). Businfo */
    if (businfocount > countof(me->businfo))
    {
        DoDriverLog(devid, DRIVERLOG_WARNING,
                    "%s: BUSINFO is too long, will use first %d elements",
                    __FUNCTION__, countof(me->businfo));

        businfocount = countof(me->businfo);
    }
    
    /* (5). Typename */
    if (strlen(typename) > sizeof(me->typename) - 1)
        DoDriverLog(devid, DRIVERLOG_WARNING,
                    "%s: TYPENAME is too long, will use first %zu characters",
                    __FUNCTION__, sizeof(me->typename) - 1);

    /* II. Allocate privrec and store everything there */
    me = malloc(sizeof(privrec_t) + drvletinfolen);
    if (me == NULL) return -CXRF_DRV_PROBL;
    bzero(me, sizeof(*me) + drvletinfolen);
    me->fd            = -1;
    me->iohandle      = -1;
    me->reconnect_tid = -1;

    me->businfocount = businfocount;
    for (x = 0;  x < businfocount;  x++) me->businfo[x] = businfo[x];
    strzcpy(me->typename,    typename,   sizeof(me->typename));
    me->e            = (ec == 'L')? DRVLET_LITTLE_ENDIAN : DRVLET_BIG_ENDIAN;
    memcpy (me->drvletinfo,  drvletinfo, drvletinfolen);

    if (me->e == local_e)
    {
        me->l2r_i16        = noop_i16;
        me->l2r_i32        = noop_i32;
        me->memcpy_l2r_i16 = noop_memcpy_i16;
        me->memcpy_l2r_i32 = noop_memcpy_i32;
        
        me->r2l_i16        = noop_i16;
        me->r2l_i32        = noop_i32;
        me->memcpy_r2l_i16 = noop_memcpy_i16;
        me->memcpy_r2l_i32 = noop_memcpy_i32;
    }
    else
    {
        me->l2r_i16        = swab_i16;
        me->l2r_i32        = swab_i32;
        me->memcpy_l2r_i16 = swab_memcpy_i16;
        me->memcpy_l2r_i32 = swab_memcpy_i32;
        
        me->r2l_i16        = swab_i16;
        me->r2l_i32        = swab_i32;
        me->memcpy_r2l_i16 = swab_memcpy_i16;
        me->memcpy_r2l_i32 = swab_memcpy_i32;
    }
    
    RegisterDevPtr(devid, me);

    /* III. Find out IP of the host */

    /* What kind of driver should we start? */
    if (strcmp(hostname, ".") == 0)
    {
        /* A local forked-out drivelet -- just remember this fact */
        me->kind = DRVLET_FORKED;
    }
    else
    {
        /* A host */
        me->kind = DRVLET_REMOTE;

        /*First, is it in dot notation (aaa.bbb.ccc.ddd)? */
        hostaddr = inet_addr(hostname);
    
        /* No, should do a hostname lookup */
        if (hostaddr == INADDR_NONE)
        {
            hp = gethostbyname(hostname);
            /* No such host?! */
            if (hp == NULL)
            {
                DoDriverLog(devid, DRIVERLOG_ERR, "gethostbyname(\"%s\"): %s",
                            hostname, hstrerror(h_errno));
                CleanupEVRT(devid, me);
                return -(CXRF_REM_C_PROBL|CXRF_CFG_PROBL);
            }
    
            memcpy(&(me->hostaddr), hp->h_addr, hp->h_length);
        }
    }

    /* IV. Try to start */
    r = InitiateStart(devid, me);
    if (r < 0)
    {
        CleanupEVRT(devid, me);
        return r;
    }
    /*!!! HeartBeat(devid, me, -1, NULL); */

    return DEVSTATE_NOTREADY;
}

static void remdrv_term_d(int devid, void *devptr)
{
  privrec_t  *me = (privrec_t *)devptr;

    CleanupEVRT(devid, me);
}

static void ProcessInData     (int devid, privrec_t *me,
                               remdrv_pkt_header_t *pkt, size_t inpktsize)
{
  uint32  command = me->r2l_i32(pkt->command);

    switch (command)
    {
        case REMDRV_C_DEBUGP:
            DoDriverLog (devid, me->r2l_i32(pkt->var.debugp.level) &~ DRIVERLOG_ERRNO,
                         "DEBUGP: %s", (char *)(pkt->data));
            break;

        case REMDRV_C_CRAZY:
            DoDriverLog(devid, DRIVERLOG_CRIT, "CRAZY -- maybe %s<->drivelet version incompatibility",
                        __FILE__);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "CRAZY");
            break;
        
        case REMDRV_C_RRUNDP:
            DoDriverLog(devid, DRIVERLOG_CRIT, "RRUNDP: %s", (char *)(pkt->data));
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "RRUNDP");
            break;
        
        case REMDRV_C_CHSTAT:
            SetDevState(devid,
                        me->r2l_i32(pkt->var.chstat.status),
                        me->r2l_i32(pkt->var.chstat.rflags_to_set),
                        (char *)(pkt->data));
            break;

        case REMDRV_C_REREQD:
            ReRequestDevData(devid);
            break;

        case REMDRV_C_PONG_Y:
            /* We don't need to do anything upon PONG */
            break;
        
        default:;
    }
}

static void remdrv_rw_p(int devid, void *devptr,
                        int action,
                        int count, int *addrs,
                        cxdtype_t *dtypes, int *nelems, void **values)
{
  privrec_t  *me = (privrec_t *)devptr;
}


DEFINE_CXSD_DRIVER(remdrv, "Remote driver adapter",
                   NULL, NULL,
                   0, NULL,
                   0, 1000 /* Seems to be a fair limit */,
                   NULL, 0,
                   NULL,
                   -1, NULL, NULL,
                   remdrv_init_d, remdrv_term_d, remdrv_rw_p);
