#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "misclib.h"
#include "cxscheduler.h"
#include "fdiolib.h"

#include "remdrvlet.h"
#include "remdrv_proto.h"


enum
{
    MAX_UNCONNECTED_TIME = 10, // In seconds
};


static int    lsock         = -1;
static int    csock         = -1;


static void ErrnoFail(const char *argv0, const char *format, ...)
            __attribute__ ((format (printf, 2, 3)));
static void ErrnoFail(const char *argv0, const char *format, ...)
{
  va_list  ap;
  int      errcode = errno;
    
    fprintf(stderr, "%s: ", argv0);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errcode));
    exit(1);
}

//////////////////////////////////////////////////////////////////////

static int CreateSocket(const char *argv0, const char *which, int port)
{
  int                 s;
  struct sockaddr_in  iaddr;		/* Address to bind `inet_entry' to */
  int                 on     = 1;	/* "1" for REUSE_ADDR */
  int                 r;

    /* Create a socket */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        ErrnoFail(argv0, "unable to create %s listening socket", which);

    /* Bind it to a name */
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    iaddr.sin_family      = AF_INET;
    iaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    iaddr.sin_port        = htons(port);
    r = bind(s, (struct sockaddr *) &iaddr,
             sizeof(iaddr));
    if (r < 0)
        ErrnoFail(argv0, "unable to bind() %s listening socket to port %d", which, port);

    /* Mark it as listening */
    r = listen(s, 20);
    if (r < 0)
        ErrnoFail(argv0, "unable to listen() %s listening socket", which);

    return s;
}

static void Daemonize(const char *argv0)
{
  int   fd;

    /* Substitute with a child to release the caller */
    switch (fork())
    {
        case  0:  /* Child */
            fprintf(stderr, "Child -- operating\n");
            break;                                

        case -1:  /* Failed to fork */
            ErrnoFail(argv0, "can't fork()");

        default:  /* Parent */
            fprintf(stderr, "Parent -- exiting\n");
            _exit(0);
    }

    /* Switch to a new session */
    if (setsid() < 0)
    {
        ErrnoFail(argv0, "can't setsid()");
        exit(1);
    }

    /* Get rid of stdin, stdout and stderr files and terminal */
    /* We don't need to disassociate from tty explicitly, thanks to setsid() */
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1) {
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        if (fd > 2) close(fd);
    }

    errno = 0;
}

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int                 in_use;
    int                 s;            // Host communication socket
    sl_fdh_t            s_fdh;        // ...and its cxscheduler handle
    struct sockaddr     cln_addr;
    remdrvlet_newdev_t  newdev_p;
    char                drvname[32];  // Name of this driver
    int                 drvnamelen;   // ...and its length
    sl_tid_t            tid;
} reqinfo_t;

#ifndef MAX_REM_DEVS
  #define MAX_REM_DEVS 100 // An arbitrary limit
#endif /* MAX_REM_DEVS */

static reqinfo_t  reqs[MAX_REM_DEVS];

static reqinfo_t *AllocReq(void)
{
  int        id;
  reqinfo_t *rp;

   for (id = 0;  id < countof(reqs);  id++)
        if (reqs[id].in_use == 0)
        {
            rp = reqs + id;
            bzero(rp, sizeof(*rp));
            rp->in_use   = 1;
            return rp;
        }

   return NULL;
}

static void      FreeReq(reqinfo_t *rp, int do_close)
{
    if (rp->in_use == 0)
    {
        remdrvlet_debug("%s: attempt to free an unused req @0x%p/#%d!",
                        __FUNCTION__, rp, (int)(rp-reqs));
        return;
    }

    if (do_close)
    {
        sl_del_fd(rp->s_fdh);
        close    (rp->s);
    }
    sl_deq_tout(rp->tid);
    
    rp->in_use = 0;
}

static void ReadDriverName(int uniq, void *unsdptr,
                           sl_fdh_t fdh, int fd, int mask, void *privptr)
{
  reqinfo_t *rp = privptr;
  int        r;

    if ((mask & SL_RD) == 0) return;
  
    errno = 0;
    r = read(fd, rp->drvname + rp->drvnamelen, 1);
    if (r <= 0)
    {
        remdrvlet_debug("%s: read=%d: %s", __FUNCTION__, r, strerror(errno));
        FreeReq(rp, 1);
        return;
    }

    if (rp->drvname[rp->drvnamelen] != '\0')
    {
        rp->drvnamelen++;
        if (rp->drvnamelen < sizeof(rp->drvname)) return;

        remdrvlet_report(fd, REMDRV_C_RRUNDP,
                         "Driver name too long - limit is %zu chars",
                         sizeof(rp->drvname) - 1);
        FreeReq(rp, 1);
        return;
    }

    /* Okay, the whole name has arrived */
    sl_deq_tout(rp->tid); rp->tid = -1;
    sl_del_fd(fdh);

    rp->newdev_p(fd, &(rp->cln_addr), rp->drvname);
    
    FreeReq(rp, 0);
}

static void CleanupUnfinishedReq(int uniq, void *unsdptr,
                                 sl_tid_t tid, void *privptr)
{
  reqinfo_t *rp = privptr;

    remdrvlet_debug ("%s: timed out waiting for driver name", __FUNCTION__);
    remdrvlet_report(rp->s, REMDRV_C_RRUNDP,
                     "Timed out waiting for driver name");
    FreeReq(rp, 1);
}


static void AcceptHostConnection(int uniq, void *unsdptr,
                                 fdio_handle_t handle, int reason,
                                 void *inpkt __attribute((unused)), size_t inpktsize __attribute((unused)),
                                 void *privptr)
{
  int                 s;
  struct  sockaddr    cln_addr;
  socklen_t           addrlen;
  int                 on = 1;

  reqinfo_t          *rp;

    if (reason != FDIO_R_ACCEPT) return; /*?!*/

    addrlen = sizeof(cln_addr);
    s = fdio_accept(handle, &cln_addr, &addrlen);
    if (s < 0)
    {
        remdrvlet_debug("%s: fdio_accept=%d, %s",
                        __FUNCTION__, s, strerror(errno));
        return;
    }

    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));

    rp = AllocReq();
    if (rp == NULL)
    {
        remdrvlet_report(s, REMDRV_C_RRUNDP,
                         "%s: unable to allocate req (all used)",
                         __FUNCTION__);
        close(s);
    }
    
    rp->s        = s;
    rp->cln_addr = cln_addr;
    rp->newdev_p = privptr;
    
    if ((rp->s_fdh = sl_add_fd(0, NULL, /*!!!uniq*/ s, 1, ReadDriverName, rp)) < 0)
    {
        remdrvlet_report(s, REMDRV_C_RRUNDP,
                         "%s: unable to sl_add_fd(s=%d)",
                         __FUNCTION__, s);
        close(s);
        return;
    }
    
    rp->tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                MAX_UNCONNECTED_TIME*1000*1000,
                                CleanupUnfinishedReq, rp);
}


static void AcceptConsConnection(int uniq, void *unsdptr,
                                 fdio_handle_t handle, int reason,
                                 void *inpkt __attribute((unused)), size_t inpktsize __attribute((unused)),
                                 void *privptr)
{
  remdrvlet_newcon_t  newcon_p = privptr;
  int                 s;
  struct  sockaddr    cln_addr;
  socklen_t           addrlen;
  int                 on = 1;

    if (reason != FDIO_R_ACCEPT) return; /*?!*/

    addrlen = sizeof(cln_addr);
    s = fdio_accept(handle, &cln_addr, &addrlen);
    if (s < 0)
    {
        remdrvlet_debug("%s: fdio_accept=%d, %s",
                        __FUNCTION__, s, strerror(errno));
        return;
    }

    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
    
    newcon_p(s, &cln_addr);
}


int  remdrvlet_listener(const char *argv0,
                        int port, int dontdaemonize,
                        remdrvlet_newdev_t newdev_p,
                        remdrvlet_newcon_t newcon_p)
{
    if (1)
    {
        lsock = CreateSocket(argv0, "main", port);
        fdio_register_fd(0, NULL, /*!!!uniq*/
                         lsock, FDIO_LISTEN, AcceptHostConnection, newdev_p,
                         0, 0, 0, 0, FDIO_LEN_LITTLE_ENDIAN, 0, 0);
    }
    if (newcon_p != NULL)
    {
        csock = CreateSocket(argv0, "cons", port + 1);
        fdio_register_fd(0, NULL, /*!!!uniq*/
                         csock, FDIO_LISTEN, AcceptConsConnection, newcon_p,
                         0, 0, 0, 0, FDIO_LEN_LITTLE_ENDIAN, 0, 0);
    }

    if (!dontdaemonize) Daemonize(argv0);

    return sl_main_loop() == 0? 0 : 1;
}
