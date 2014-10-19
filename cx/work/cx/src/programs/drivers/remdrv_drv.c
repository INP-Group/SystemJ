#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "misc_macros.h"
#include "misclib.h"
#include "memcasecmp.h"
#include "timeval_utils.h"

#include "cxsd_driver.h"
#include "remdrv_proto.h"


//// Some definitions ////////////////////////////////////////////////

enum {DEFAULT_RECONNECT_TIME_USECS = 10*1000*1000};

enum {CFG_PROBL_RETRIES = 2};
enum {CONFIG_TO_CHSTAT_SECS = 3};

/*!!!DIRSTRUCT*/
#define DEF_DRIVERPATH_PFX "/mnt/host/oduser/pult/lib/server/drivers"

typedef struct
{
    const char *name;
    int         is_be;
    const char *path_env;
    const char *path_def;
    const char *path_sep;
    const char *suffix;
} arch_info_t;

static arch_info_t architectures[] =
{
    {
        "uclinux",
        1,
        "CM5307_UCLINUX_DRVLETS_PATH",
        DEF_DRIVERPATH_PFX "/cm5307_uclinux_drvlets",
        "/",
        ".drvlet"
    },
    {
        "ppc",
        1,
        "CM5307_PPC_DRVLETS_PATH",
        DEF_DRIVERPATH_PFX "/cm5307_ppc_drvlets",
        "/",
        ".drvlet"
    },
    {
        "cangw",
        1,
        NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        "bivme2",
        1,
        "BIVME2_DRVLETS_PATH",
        DEF_DRIVERPATH_PFX "/bivme2_drvlets",
        "/",
        ".drvlet"
    },
    {
        "moxa",
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },
    {NULL}
};

typedef enum
{
    INRD_HEADER = 0,
    INRD_DATA   = 1
} inbufrdstate_t;

typedef struct
{
    int                  is_ready;
    int                  is_suffering;
    int                  fd;
    sl_fdh_t             rd_fdhandle;
    sl_fdh_t             wr_fdhandle;
    sl_tid_t             rcn_tid;

    int                  fail_count;
    struct timeval       config_timestamp;

    int                  businfocount;
    int32                businfo[20]; /*!!!==CXSD_DB_MAX_BUSINFOCOUNT*/
    unsigned long        hostaddr;
    const arch_info_t   *arch;
    char                 drvname[100];

    int                  set1;
    int                  set2;
    int                  rcvbuf;
    int                  sndbuf;

    int                  last_loglevel;
    int                  last_logmask;

    /* I/O management (in fact -- "I" only :-) */
    remdrv_pkt_header_t *inbuf;
    size_t               inbufsize;
    inbufrdstate_t       inbufstate;
    size_t               inbufused;
    size_t               inpktsize;

    char                 drvletinfo[0];
} privrec_t;


//// Utility functions -- should better be in some lib ///////////////

static inline void memcpy_ntohl(uint32 *dst, uint32 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = ntohl(*src++);
}

static inline void memcpy_htonl(uint32 *dst, uint32 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = htonl(*src++);
}

static inline void memcpy_ntohs(uint16 *dst, uint16 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = ntohs(*src++);
}

static inline void memcpy_htons(uint16 *dst, uint16 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = htons(*src++);
}


//// Auto-reconnect logic ////////////////////////////////////////////

static void remdrv_fd_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr);
static int InitiateStart(int devid, privrec_t *me);

static void TakeCareOfSuffering(const char *function, int devid, privrec_t *me)
{
    if (!me->is_suffering)
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: connect()", function);
    me->is_suffering = 1;
}

static void TryToReconnect_p(int devid, void *devptr,
                             int tid,
                             void *privptr)
{
  privrec_t      *me = (privrec_t *) devptr;

    me->rcn_tid = -1;
    InitiateStart(devid, me);
}

static void ScheduleReconnect(int devid, int fd, privrec_t *me)
{
    if (fd >= 0)
    {
        close(fd);
        if (me->rd_fdhandle >= 0) sl_del_fd(me->rd_fdhandle); me->rd_fdhandle = -1;
        if (me->wr_fdhandle >= 0) sl_del_fd(me->wr_fdhandle); me->wr_fdhandle = -1;
    }

    me->is_ready = 0;
    SetDevState(devid, DEVSTATE_NOTREADY, CXRF_REM_C_PROBL, NULL);

    if (me->rcn_tid >= 0)
    {
        sl_deq_tout(me->rcn_tid);
        me->rcn_tid = -1;
    }
    me->rcn_tid = sl_enq_tout_after(devid, me, DEFAULT_RECONNECT_TIME_USECS, TryToReconnect_p, NULL);
}

static int write_nonempty(int fd, const char *s)
{
    if (s == NULL  ||  *s == '\0') return +1; // Not '0' because of conditions below

    return write(fd, s, strlen(s));
}

static void InitializeDrivelet(int devid, int fd, privrec_t *me,
                               int re_request)
{
  const char         *drvlet_dir;
  int                 r;
  int                 x;
  size_t              pktsize;
  struct {remdrv_pkt_header_t hdr;  int32 data[1000];} pkt;

  static char         nul_byte = '\0';

    set_fd_flags(fd, O_NONBLOCK, 1);

    drvlet_dir = NULL;
    if (me->arch->path_env != NULL) drvlet_dir = getenv(me->arch->path_env);
    if (drvlet_dir == NULL)
        drvlet_dir = me->arch->path_def;

    /* Send the driver name */
    if (
        write_nonempty(fd, drvlet_dir)         <= 0  ||
        write_nonempty(fd, me->arch->path_sep) <= 0  ||
        write_nonempty(fd, me->drvname)        <= 0  ||
        write_nonempty(fd, me->arch->suffix)   <= 0  ||
        write         (fd, &nul_byte, 1)       <= 0
       ) goto FAILURE;

    /* Initialize remote drivelet */
    pktsize =
        sizeof(pkt.hdr)                  +
        me->businfocount * sizeof(int32) +
        strlen(me->drvletinfo) + 1;
    
    bzero(&pkt.hdr, sizeof(pkt.hdr));
    if (me->arch->is_be)
    {
        pkt.hdr.pktsize = htonl(pktsize);
        pkt.hdr.command = htonl(REMDRVP_CONFIG);
        pkt.hdr.var.config.proto_version = htonl(REMDRVP_VERSION);
        pkt.hdr.var.config.businfocount  = htonl(me->businfocount);
    }
    else
    {
        pkt.hdr.pktsize = pktsize;
        pkt.hdr.command = REMDRVP_CONFIG;
        pkt.hdr.var.config.proto_version = REMDRVP_VERSION;
        pkt.hdr.var.config.businfocount  = me->businfocount;
    }
    for (x = 0;  x < me->businfocount;  x++)
        if (me->arch->is_be)
            pkt.hdr.data[x] = htonl(me->businfo[x]);
        else
            pkt.hdr.data[x] = me->businfo[x];
    memcpy(pkt.hdr.data + me->businfocount, me->drvletinfo, strlen(me->drvletinfo) + 1);

    errno = 0;
    r = write(fd, &pkt, pktsize);
    if (r != pktsize) goto FAILURE;
    gettimeofday(&(me->config_timestamp), NULL);

    set_fd_flags(fd, O_NONBLOCK, 1);

    me->last_loglevel = 0;
    me->last_logmask  = DRIVERLOG_m_CHECKMASK_ALL;
    
    /* Proclaim that we are ready */
    me->is_ready    = 1;
    me->fd          = fd;
    me->rd_fdhandle = sl_add_fd(devid, me, fd, SL_RD, remdrv_fd_p, NULL);
    SetDevState(devid, DEVSTATE_OPERATING, 0, NULL);
    if (re_request) ReRequestDevData(devid);

    if (!re_request) DoDriverLog(devid, 0, "Successfully re-started the drivelet");
    
    return;

 FAILURE:
    DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, __FUNCTION__);
    ScheduleReconnect(devid, fd, me);
}

static void FdWrReady_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr)
{
  privrec_t      *me = (privrec_t *) devptr;
  int             sock_err;
  socklen_t       errlen;
  int             was_suffering;
  int             r;
  int             on = 1;

    ////DoDriverLog(devid, 0, "%s()", __FUNCTION__);
  
    errlen = sizeof(sock_err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_err, &errlen);
    errno = sock_err;
    
    if (errno != 0)
    {
        TakeCareOfSuffering(__FUNCTION__, devid, me);
        ScheduleReconnect(devid, fd, me);
        return;
    }

    r = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
    {
      socklen_t           addrlen;
      struct sockaddr_in  my_addr;
    
        addrlen = sizeof(my_addr);
        getsockname(fd, (struct sockaddr *)&(my_addr), &addrlen);

        DoDriverLog(devid, DRIVERLOG_DEBUG,
                    "connect() successful, port=%d",
                    ntohs(my_addr.sin_port));
    }
    
    was_suffering = me->is_suffering;
    me->is_suffering = 0;
    sl_del_fd(me->wr_fdhandle); me->wr_fdhandle = -1;
    InitializeDrivelet(devid, fd, me, !was_suffering);
}

static int InitiateStart(int devid, privrec_t *me)
{
  int                 s;
  struct sockaddr_in  idst;
  socklen_t           addrlen;
  int                 r;

  size_t              bsize;
  socklen_t           bsizelen;
  
    /* Create a socket */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: FATAL: socket()", __FUNCTION__);
        return -CXRF_DRV_PROBL;
    }
    if (s >= FD_SETSIZE)
    {
        DoDriverLog(devid, 0, "%s: FATAL: socket()=%d, >=FD_SETSIZE=%d :-(",
                    __FUNCTION__, s, FD_SETSIZE);
        close(s);
        return -CXRF_DRV_PROBL;
    }

    idst.sin_family = AF_INET;
    idst.sin_port   = htons(REMDRV_DEFAULT_PORT);
    memcpy(&idst.sin_addr, &(me->hostaddr), sizeof(me->hostaddr));
    addrlen = sizeof(idst);

    set_fd_flags(s, O_NONBLOCK, 1);

    if (me->rcvbuf > 0)
    {
        bsizelen = sizeof(bsize);
        r = getsockopt(s, SOL_SOCKET, SO_RCVBUF, &bsize, &bsizelen);
        DoDriverLog(devid, DRIVERLOG_DEBUG, "RCVBUF=%zd/%d", (ssize_t)bsize, r);

        bsize    = me->rcvbuf;
        bsizelen = sizeof(bsize);
        r = setsockopt(s, SOL_SOCKET, SO_RCVBUF, &bsize,  bsizelen);
        DoDriverLog(devid, DRIVERLOG_DEBUG, "   setting %d, r=%d", me->rcvbuf, r);
    }
    if (me->sndbuf > 0)
    {
        bsizelen = sizeof(bsize);
        r = getsockopt(s, SOL_SOCKET, SO_SNDBUF, &bsize, &bsizelen);
        DoDriverLog(devid, DRIVERLOG_DEBUG, "SNDBUF=%zd/%d", (ssize_t)bsize, r);

        bsize    = me->sndbuf;
        bsizelen = sizeof(bsize);
        r = setsockopt(s, SOL_SOCKET, SO_SNDBUF, &bsize,  bsizelen);
        DoDriverLog(devid, DRIVERLOG_DEBUG, "   setting %d, r=%d", me->sndbuf, r);
    }

    r = connect(s, (struct sockaddr *)&idst, addrlen);
    ////DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "connect");
    if (r < 0)
    {
        /* Everything is fine, working... */
        if      (errno == EINPROGRESS)
        {
            me->wr_fdhandle = sl_add_fd(devid, me, s, SL_WR, FdWrReady_p, NULL);
            return 0;
        }
        /* A temporary error condition? */
        else if (IS_A_TEMPORARY_CONNECT_ERROR())
        {
            TakeCareOfSuffering(__FUNCTION__, devid, me);
            close(s);
            ScheduleReconnect(devid, -1, me);
            return 0;
        }
        /* A fatal error... */
        else
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: FATAL: connect()", __FUNCTION__);
            close(s);
            return -CXRF_DRV_PROBL;
        }
    }
    else
    {
        /* Wow! An instant success?! */
        InitializeDrivelet(devid, s, me, 0);
    }

    return 0;
}

static void HeartBeat(int devid, void *devptr,
                      sl_tid_t tid,
                      void *privptr)
{
  privrec_t           *me = (privrec_t *) devptr;
  struct {remdrv_pkt_header_t hdr;  int32 data[0];} pkt;
  int                  pktsize = sizeof(pkt.hdr);
  int                  r;

    if (me->is_ready)
    {
        bzero(&pkt.hdr, sizeof(pkt.hdr));
        if (me->arch->is_be)
        {
            pkt.hdr.pktsize = htonl(pktsize);
            pkt.hdr.command = htonl(REMDRVP_PING);
        }
        else
        {
            pkt.hdr.pktsize = pktsize;
            pkt.hdr.command = REMDRVP_PING;
        }

        set_fd_flags(me->fd, O_NONBLOCK, 0);
        r = write(me->fd, &pkt, pktsize);
        if (r < 0  ||  r != pktsize)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO,
                        "%s: write(fd=%d)=%d,errno=%d; terminating and scheduling restart",
                        __FUNCTION__, me->fd, r, errno);
            ScheduleReconnect(devid, me->fd, me);
            return;
        }
        set_fd_flags(me->fd, O_NONBLOCK, 1);
    }

    sl_enq_tout_after(devid, devptr, 300*1000*1000, HeartBeat, NULL);
}


//// Driver's methods ////////////////////////////////////////////////

typedef struct
{
    int  set1;
    int  set2;
    int  rcvbuf;
    int  sndbuf;
} remdrvopts_t;

static psp_paramdescr_t text2remdrvopts[] =
{
    PSP_P_INT("set1",   remdrvopts_t, set1,   -1, 0, 0),
    PSP_P_INT("set2",   remdrvopts_t, set2,   -1, 0, 0),
    PSP_P_INT("rcvbuf", remdrvopts_t, rcvbuf, -1, 0, 0),
    PSP_P_INT("sndbuf", remdrvopts_t, sndbuf, -1, 0, 0),
    PSP_P_END()
};

static int remdrv_init_d(int devid, void *devptr __attribute__((unused)),
                         int businfocount, int businfo[],
                         const char *auxinfo)
{
  privrec_t          *me;
  remdrvopts_t        opts;
    
  const char         *b;
  const char         *p;
  size_t              archnamelen;
  const arch_info_t  *arch;
  char                hostname[100];
  size_t              hostnamelen;
  const char         *drvname;
  size_t              drvnamelen;
  const char         *drvletinfo;
  size_t              drvletinfolen;
  struct hostent     *hp;
  unsigned long       hostaddr;

  int                 x;

  int                 r;
  
    /* I. Split the auxinfo into type, host, driver and optional drvletinfo */
    /* "(Arch/host:driver[ drvletinfo])" */

    /* 0. Is there anything at all? */
    if (auxinfo == NULL  ||  auxinfo[0] == '\0')
    {
        DoDriverLog(devid, 0, "%s: auxinfo is empty...", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    /* 1. Arch */
    p = strchr(b = auxinfo, '/');
    if (p == NULL  ||  p == b)
    {
        DoDriverLog(devid, 0, "%s: auxinfo should start with 'ARCH/'", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    archnamelen = p - b;
    for (arch = architectures;  arch->name != NULL;  arch++)
        if (cx_memcasecmp(arch->name, b, archnamelen) == 0) break;
    if (arch->name == NULL)
    {
        DoDriverLog(devid, 0, "%s: unknown ARCH", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    /* 2. Host */
    p = strchr(b = p + 1, ':');
    if (p == NULL  ||  p == b)
    {
        DoDriverLog(devid, 0, "%s: <HOST>: is expected after ARCH/", __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }
    hostnamelen = p - b;
    if (hostnamelen > sizeof(hostname) - 1) hostnamelen = sizeof(hostname) - 1;
    memcpy(hostname, b, hostnamelen);
    hostname[hostnamelen] = '\0';
    
    /* 3. Drvname */
    b = p + 1;
    if (*b == '\0')
    {
        DoDriverLog(devid, 0, "%s: <DRIVERNAME> is expected after '%s:'", __FUNCTION__, hostname);
        return -CXRF_CFG_PROBL;
    }
    drvname = b;
    while (*p != '\0'  &&  *p!= ','  &&  !isspace(*p)) p++;  // Find the end of driver-name
    drvnamelen = p - drvname;                  // Remember driver-name length

    /* 4. Optional parameters: */
    /* 4.1. options */
    psp_parse(NULL, NULL, &opts, '=', ",", " \t", text2remdrvopts);
    if (*p == ',')
    {
        p++;
        if (psp_parse(p, &p,
                      &opts,
                      '=', ",", " \t",
                      text2remdrvopts) != PSP_R_OK)
        {
            DoDriverLog(devid, 0, "%s: error in options: %s", __FUNCTION__, psp_error());
            return -CXRF_CFG_PROBL;
        }
    }

    /* 4.2. drvletinfo */
    while (*p != '\0'  &&  isspace(*p))  p++;  // Skip spaces
    drvletinfo = p;                            // Here the drvletinfo starts, if it is present
    drvletinfolen = strlen(drvletinfo) + 1;
    
    /* II. Find out IP of the host */

    /* First, is it in dot notation (aaa.bbb.ccc.ddd)? */
    hostaddr = inet_addr(hostname);

    /* No, should do a hostname lookup */
    if (hostaddr == INADDR_NONE)
    {
        hp = gethostbyname(hostname);
        /* No such host?! */
        if (hp == NULL)
        {
            DoDriverLog(devid, 0, "gethostbyname(\"%s\"): %s",
                        hostname, hstrerror(h_errno));
            return -(CXRF_REM_C_PROBL|CXRF_CFG_PROBL);
        }

        memcpy(&hostaddr, hp->h_addr, hp->h_length);
    }

    /* III. Allocate privrec and store everything there */
    me = malloc(sizeof(privrec_t) + drvletinfolen);
    if (me == NULL) return -CXRF_DRV_PROBL;
    bzero(me, sizeof(*me) + drvletinfolen);

    me->rd_fdhandle = -1;
    me->wr_fdhandle = -1;
    me->rcn_tid     = -1;

    if (businfocount > countof(me->businfo))
        businfocount = countof(me->businfo);
    me->businfocount = businfocount;
    for (x = 0;  x < businfocount; x++) me->businfo[x] = businfo[x];
    me->hostaddr = hostaddr;
    me->arch = arch;
    if (drvnamelen > sizeof(me->drvname) - 1) drvnamelen = sizeof(me->drvname) - 1;
    memcpy(me->drvname,    drvname,    drvnamelen); me->drvname[drvnamelen] = '\0';
    me->set1   = opts.set1;
    me->set2   = opts.set2;
    me->rcvbuf = opts.rcvbuf;
    me->sndbuf = opts.sndbuf;
    memcpy(me->drvletinfo, drvletinfo, drvletinfolen);

    /* ...and don't forget about buffer... */
    me->inbufsize  = sizeof(remdrv_pkt_header_t);
    me->inbuf      = malloc(me->inbufsize);
    me->inbufstate = INRD_HEADER;
    if (me->inbuf == NULL)
    {
        free(me);
        return -1;
    }
    
    RegisterDevPtr(devid, me);

    /* IV. Try to start... */
    r = InitiateStart(devid, me);
    if (r < 0) return r;
    
    HeartBeat(devid, me, -1, NULL);
    
    return DEVSTATE_OPERATING; /*!!! In fact, should be a NOTREADY */
}

static void remdrv_term_d(int devid, void *devptr)
{
    safe_free(devptr);
}

static void remdrv_fd_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr)
{
  privrec_t      *me = (privrec_t *) devptr;

  int             repcount;
  int             r;
  char           *err_reason;

  int32           command;

  /* REMDRVP_{READ,WRITE} */
  int             count;
  int             start;
  int             x;
  enum {MAX_CHANS_PER_RCS = 100};
  int32           ad2ret[MAX_CHANS_PER_RCS];

  /* REMDRVP_BIGC */
  int             bigchan;
  int            *info;
  int             ninfo;
  void           *retdata;
  size_t          retdatasize;
  size_t          retdataunits;
  rflags_t        rflags;

  /* REMDRVP_DEBUGP */
  int32           loglevel;
  int             cur_loglevel;
  int             cur_logmask;
  remdrv_pkt_header_t hdr;
  int             pktsize;

  int32           state;
  int32           rflags_to_set;
  struct timeval  now;

    repcount = 30;

 REPEAT:
    repcount--;

    if (me->inbufstate == INRD_HEADER)
    {
        /* Try to read the whole header */
        count = sizeof(remdrv_pkt_header_t) - me->inbufused;
        r = uintr_read(fd,
                       ((char *)(me->inbuf)) + me->inbufused,
                       count);
        if (CheckReadR(r) != 0) {err_reason = "read@HEADER"; goto ERRRECONNECT;}

        me->inbufused += r;
        if (r < count) return;

        /* Get the packet size */
        if (me->arch->is_be) me->inpktsize = ntohl(me->inbuf->pktsize);
        else                 me->inpktsize =       me->inbuf->pktsize;
        DoDriverLog(devid, 0 | DRIVERLOG_C_REMDRV_PKTDUMP,
                    "FD_P@header: pktsize=%zu, command=0x%08x",
                    me->inpktsize, me->arch->is_be ? ntohl(me->inbuf->command)
                                                   :       me->inbuf->command);

        /* And check it */
        errno = 0;
        if (me->inpktsize < sizeof(remdrv_pkt_header_t))
        {
            DoDriverLog(devid, 0, "inpktsize=%zu, <sizeof(remdrv_pkt_header_t)=%zu (command=0x%08x)",
                        me->inpktsize, sizeof(remdrv_pkt_header_t), /*!!!*/ntohl(me->inbuf->command));
            goto FATAL;
        }
        if (me->inpktsize > REMDRVP_MAXPKTSIZE)
        {
            DoDriverLog(devid, 0, "inpktsize=%zu, >REMDRVP_MAXPKTSIZE=%d (command=0x%08x)",
                        me->inpktsize, REMDRVP_MAXPKTSIZE, /*!!!*/ntohl(me->inbuf->command));
            goto FATAL;
        }

        /* Adjust buffer size */
        r = GrowBuf((void *) &(me->inbuf), &(me->inbufsize), me->inpktsize);
        if (r < 0) goto FATAL;

        /* Change the state */
        me->inbufstate = INRD_DATA;

        /* Should we try to read the following data (if any)? */
        if (me->inpktsize != me->inbufused)
        {
            r = check_fd_state(fd, O_RDONLY);
            if (r < 0) {err_reason = "check_fd_state"; goto ERRRECONNECT;}
            if (r > 0) goto FALLTHROUGH;

            /* No, nothing more yet... */
            return;
        }
        else
        {
            goto FALLTHROUGH;
        }
    }
    else
    {
 FALLTHROUGH:
        /* Try to read the whole remainder of the packet */
        count = me->inpktsize - me->inbufused;

        /* count==0 on fallthrough from header when data size is 0 */
        if (count != 0)
        {
            r = uintr_read(fd,
                           ((char *)(me->inbuf)) + me->inbufused,
                           count);
            if (CheckReadR(r) != 0) {err_reason = "read@DATA"; goto ERRRECONNECT;}
            me->inbufused += r;
            if (r < count) return;
        }

        /**** Okay, the whole packet has arrived ********************/

        /* Reset the read state */
        me->inbufstate = INRD_HEADER;
        me->inbufused  = 0;

        /* Process the packet */
        command = me->inbuf->command;
        if (me->arch->is_be) command = ntohl(command);
        ////DoDriverLog(devid, 0, "command=%d", command);
        switch (command)
        {
            case REMDRVP_READ:
            case REMDRVP_WRITE:
                me->fail_count = 0;

                start = me->inbuf->var.rw.start;
                count = me->inbuf->var.rw.count;
                if (me->arch->is_be)
                {
                    start = ntohl(start);
                    count = ntohl(count);
                }

                if (me->arch->is_be)
                    memcpy_ntohl(me->inbuf->data, me->inbuf->data, count * 2);
                else /*!!! vvv -- what for?! */
                    memcpy      (me->inbuf->data, me->inbuf->data, count * 2 * sizeof(int32));

                ReturnChanGroup(devid, start, count, me->inbuf->data, &(me->inbuf->data[count]));

                break;

            case REMDRVP_RCSET:
                me->fail_count = 0;

                count = me->inbuf->var.rw.count;
                if (me->arch->is_be)
                {
                    count = ntohl(count);
                }

                if (count < 1  ||  count > MAX_CHANS_PER_RCS)
                {
                    DoDriverLog(devid, 0, "REMDRVP_RCSET: count=%d, out_of[1...%d]", count, MAX_CHANS_PER_RCS);
                    SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "REMDRVP_RCSET: 'count' too big");
                }

                if (me->arch->is_be)
                    memcpy_ntohl(me->inbuf->data, me->inbuf->data, count * 3);
                else /*!!! vvv -- what for?! */
                    memcpy      (me->inbuf->data, me->inbuf->data, count * 3 * sizeof(int32));

                /* Note: this copying is made AFTER endian conversion, so that channel numbers are already in host byte order */
                for (x = 0;  x < count;  x++) ad2ret[x] =  me->inbuf->data[x];

                ReturnChanSet(devid, ad2ret, count, &(me->inbuf->data[count]), &(me->inbuf->data[count*2]));

                break;

            case REMDRVP_BIGC:
                me->fail_count = 0;

                bigchan      = me->inbuf->var.bigc.bigchan;
                ninfo        = me->inbuf->var.bigc.nparams;
                retdatasize  = me->inbuf->var.bigc.datasize;
                retdataunits = me->inbuf->var.bigc.dataunits;
                rflags       = me->inbuf->var.bigc.rflags;
                if (me->arch->is_be)
                {
                    bigchan      = ntohl(bigchan);
                    ninfo        = ntohl(ninfo);
                    retdatasize  = ntohl(retdatasize);
                    retdataunits = ntohl(retdataunits);
                    rflags       = ntohl(rflags);
                }
                info         = &(me->inbuf->data[0]);
                retdata      = &(me->inbuf->data[ninfo]);

                if (retdatasize == 0) retdataunits = 1;
                if ((retdataunits != 1  &&  retdataunits != 2  &&  retdataunits != 4) ||
                    retdatasize % retdataunits != 0)
                {
                    DoDriverLog(devid, 0, "retdataunits=%zu, retdatasize=%zu! Setting retdatasize=0",
                                retdataunits, retdatasize);
                    retdatasize = 0; retdataunits = 1;
                }

                /*!!! Should check that everything fits into inpktsize!*/

                if (ninfo != 0)
                {
                    if (me->arch->is_be)
                        memcpy_ntohl(info, info, ninfo);
                    else
                        memcpy      (info, info, ninfo * sizeof(int32));
                }

                if (retdatasize != 0)
                {
                    if (me->arch->is_be)
                        switch (retdataunits)
                        {
                            case 1: memcpy      (retdata, retdata, retdatasize);                break;
                            case 2: memcpy_ntohs(retdata, retdata, retdatasize / retdataunits); break;
                            case 4: memcpy_ntohl(retdata, retdata, retdatasize / retdataunits); break;
                        }
                    else
                        memcpy                  (retdata, retdata, retdatasize);
                }

                ReturnBigc(devid, bigchan,
                           info, ninfo, retdata, retdatasize, retdataunits, rflags);

                break;

            case REMDRVP_DEBUGP:
                loglevel = me->inbuf->var.debugp.level;
                if (me->arch->is_be) loglevel = ntohl(loglevel);
                DoDriverLog(devid, loglevel &~ DRIVERLOG_ERRNO,
                            "DEBUGP: %s", (char *)(me->inbuf->data));

                GetCurrentLogSettings(devid, &cur_loglevel, &cur_logmask);
                if (cur_loglevel != me->last_loglevel  ||
                    cur_logmask  != me->last_logmask)
                {
                    me->last_loglevel = cur_loglevel;
                    me->last_logmask  = cur_logmask;

                    pktsize = sizeof(hdr);
                    bzero(&hdr, sizeof(hdr));
                    if (me->arch->is_be)
                    {
                        hdr.pktsize = htonl(pktsize);
                        hdr.command = htonl(REMDRVP_SETDBG);
                        hdr.var.setdbg.verblevel = htonl(cur_loglevel);
                        hdr.var.setdbg.mask      = htonl(cur_logmask);
                    }
                    else
                    {
                        hdr.pktsize = pktsize;
                        hdr.command = REMDRVP_SETDBG;
                        hdr.var.setdbg.verblevel = cur_loglevel;
                        hdr.var.setdbg.mask      = cur_logmask;
                    }

                    set_fd_flags(fd, O_NONBLOCK, 0);
                    r = write(fd, &hdr, pktsize);
                    if (r < 0  ||  r != pktsize)
                    {
                        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO,
                                    "%s@DEBUGP: write(fd=%d)=%d,errno=%d; terminating and scheduling restart",
                                    __FUNCTION__, me->fd, r, errno);
                        ScheduleReconnect(devid, fd, me);
                        return;
                    }
                    set_fd_flags(fd, O_NONBLOCK, 1);
                }
                break;

            case REMDRVP_RRUNDP:
                DoDriverLog(devid, 0, "RRUNDP: %s",
                            (char *)(me->inbuf->data));
                SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "remote:RRUNDP");
                return;
                
            case REMDRVP_CRAZY:
                DoDriverLog(devid, 0, "CRAZY -- maybe a %s<->drivelet version incompatibility?",
                            __FILE__);
                SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "remote:CRAZY");
                return;

            case REMDRVP_CHSTAT:
                state         = me->inbuf->var.chstat.state;
                rflags_to_set = me->inbuf->var.chstat.rflags_to_set;
                if (me->arch->is_be)
                {
                    state         = ntohl(state);
                    rflags_to_set = ntohl(rflags_to_set);
                }

                gettimeofday(&now, NULL); now.tv_sec -= CONFIG_TO_CHSTAT_SECS;
                if (state == DEVSTATE_OFFLINE           &&
                    rflags_to_set == CXRF_CFG_PROBL     &&
                    me->fail_count < CFG_PROBL_RETRIES  &&
                    !TV_IS_AFTER(now, me->config_timestamp))
                {
                    me->fail_count++;
                    err_reason = "probably a temporary error"; goto ERRRECONNECT;
                }

                me->fail_count = 0;

                SetDevState(devid, state, rflags_to_set, NULL/*!!!*/);
                return;

            case REMDRVP_REREQD:
                ReRequestDevData(devid);
                break;

            case REMDRVP_PING:
                break;
                
            default:;
        }
    }

    if (repcount > 0)
    {
        r = check_fd_state(fd, O_RDONLY);
        if (r < 0) {err_reason = "repeat:check_fd_state"; goto ERRRECONNECT;}
        if (r > 0) goto REPEAT;
    }
    return;

 FATAL:
    DoDriverLog(devid, 0 | (errno == 0? 0 : DRIVERLOG_ERRNO), "%s: FATAL -- terminating", __FUNCTION__);
    SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "FATAL I/O problem");
    return;
    
 ERRRECONNECT:
    DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: ERROR: %s, will restart",
                __FUNCTION__, err_reason);
    ScheduleReconnect(devid, fd, me);
    return;
}

static void remdrv_rw_p(int devid, void *devptr,
                        int firstchan, int count, int32 *values, int action)
{
  privrec_t *me = (privrec_t *) devptr;
  
  struct {remdrv_pkt_header_t hdr;  int32 data[REMDRVP_MAXDATASIZE/sizeof(int32)];} pkt;
  int        pktsize;
  int        command = action == DRVA_READ? REMDRVP_READ : REMDRVP_WRITE;
  int        r;
  
    if (!me->is_ready) return;

    pktsize = sizeof(pkt.hdr);
    if (action == DRVA_WRITE)  pktsize += sizeof(*values) * count;
  
    bzero(&pkt.hdr, sizeof(pkt.hdr));
    if (me->arch->is_be)
    {
        pkt.hdr.pktsize = htonl(pktsize);
        pkt.hdr.command = htonl(command);
        pkt.hdr.cycle   = htonl(GetCurrentCycle());
        
        pkt.hdr.var.rw.start = htonl(firstchan);
        pkt.hdr.var.rw.count = htonl(count);
        pkt.hdr.var.rw.set1  = htonl(me->set1);
        pkt.hdr.var.rw.set2  = htonl(me->set2);
    }
    else
    {
        pkt.hdr.pktsize = pktsize;
        pkt.hdr.command = command;
        pkt.hdr.cycle   = GetCurrentCycle();
        
        pkt.hdr.var.rw.start = firstchan;
        pkt.hdr.var.rw.count = count;
        pkt.hdr.var.rw.set1  = me->set1;
        pkt.hdr.var.rw.set2  = me->set2;
    }

    if (action == DRVA_WRITE)
    {
        if (me->arch->is_be)
            memcpy_htonl(pkt.data, values, count);
        else
            memcpy      (pkt.data, values, count * sizeof(int32));
    }

    set_fd_flags(me->fd, O_NONBLOCK, 0);
    r = write(me->fd, &pkt, pktsize);
    if (r < 0  ||  r != pktsize)
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO,
                    "%s: write(fd=%d)=%d,errno=%d; terminating and scheduling restart",
                    __FUNCTION__, me->fd, r, errno);
        ScheduleReconnect(devid, me->fd, me);
        return;
    }
    set_fd_flags(me->fd, O_NONBLOCK, 1);

    DoDriverLog(devid, 0 | DRIVERLOG_C_REMDRV_PKTINFO,
                "RW_P: pktsize=%d, command=0x%08x", pktsize, command);
}

static void remdrv_bigc_p(int devid, void *devptr,
                          int chan, int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
  privrec_t *me = (privrec_t *) devptr;
  
  struct {remdrv_pkt_header_t hdr;  int32 data[REMDRVP_MAXDATASIZE/sizeof(int32)];} pkt;
  int        pktsize;
  int        r;
  
    if (!me->is_ready) return;

    pktsize = sizeof(pkt.hdr) + sizeof(*args) * nargs + datasize;
    bzero(&pkt.hdr, sizeof(pkt.hdr));
    if (me->arch->is_be)
    {
        pkt.hdr.pktsize = htonl(pktsize);
        pkt.hdr.command = htonl(REMDRVP_BIGC);
        pkt.hdr.cycle   = htonl(GetCurrentCycle());

        pkt.hdr.var.bigc.bigchan   = htonl(chan);
        pkt.hdr.var.bigc.nparams   = htonl(nargs);
        pkt.hdr.var.bigc.datasize  = htonl(datasize);
        pkt.hdr.var.bigc.dataunits = htonl(dataunits);
    }
    else
    {
        pkt.hdr.pktsize = pktsize;
        pkt.hdr.command = REMDRVP_BIGC;
        pkt.hdr.cycle   = GetCurrentCycle();

        pkt.hdr.var.bigc.bigchan   = chan;
        pkt.hdr.var.bigc.nparams   = nargs;
        pkt.hdr.var.bigc.datasize  = datasize;
        pkt.hdr.var.bigc.dataunits = dataunits;
    }

    if (nargs != 0)
    {
        if (me->arch->is_be)
            memcpy_htonl(pkt.data, args, nargs);
        else
            memcpy      (pkt.data, args, nargs * sizeof(int32));
    }

    if (datasize != 0)
    {
        if (me->arch->is_be)
            switch (dataunits)
            {
                case 1: memcpy      (         &(pkt.data[nargs]), data, datasize);             break;
                case 2: memcpy_htons((int16 *)&(pkt.data[nargs]), data, datasize / dataunits); break;
                case 4: memcpy_htonl((int32 *)&(pkt.data[nargs]), data, datasize / dataunits); break;
            }
        else
            memcpy                  (         &(pkt.data[nargs]), data, datasize);
    }

    set_fd_flags(me->fd, O_NONBLOCK, 0);
    r = write(me->fd, &pkt, pktsize);
    if (r < 0  ||  r != pktsize)
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO,
                    "%s: write(fd=%d)=%d,errno=%d; terminating and scheduling restart",
                    __FUNCTION__, me->fd, r, errno);
        ScheduleReconnect(devid, me->fd, me);
        return;
    }
    set_fd_flags(me->fd, O_NONBLOCK, 1);

    DoDriverLog(devid, 0 | DRIVERLOG_C_REMDRV_PKTINFO,
                "BIG_P: pktsize=%d, command=%d", pktsize, REMDRVP_BIGC);
}


//// Driverrec ///////////////////////////////////////////////////////

DEFINE_DRIVER(remdrv, "Remote driver forwarder",
              NULL, NULL,
              0, NULL,
              0, 20, /*!!! CXSD_DB_MAX_BUSINFOCOUNT */
              NULL, 0,
              -1, NULL,
              -1, NULL,
              remdrv_init_d, remdrv_term_d,
              remdrv_rw_p, remdrv_bigc_p);
