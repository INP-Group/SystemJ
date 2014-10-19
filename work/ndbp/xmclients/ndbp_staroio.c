#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <math.h>

#include <X11/Intrinsic.h>

#include "cx_sysdeps.h"
#include "misc_macros.h"
#include "misclib.h"
#include "fdiolib.h"

#include "Xh.h"
#include "Chl.h"
#include "Knobs_typesP.h"
#include "KnobsI.h"

#define __NDBP_STAROIO_C
#include "ndbp_staroio.h"

#include "gusev_mcp_proto.h"


typedef struct
{
    char   name[100];
    Knob   ki;
} gusev_chaninfo_t;

enum {FD_UNUSED = -1};
typedef struct
{
    int             s;
    struct in_addr  addr;
    int             is_camera;
    fdio_handle_t   fhandle;
} gusev_client_t;

static XhWindow          ourwin              = NULL;

static gusev_chaninfo_t *watched_chans       = NULL;
static int               watched_chans_count = 0;

static gusev_client_t   *clients             = NULL;
static int               clients_allocd      = 0;


static int               senddata_after      = 0;
static char              senddata_changer[10];


static char progname[40] = "";

static void reporterror(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));

static void reporterror(const char *format, ...)
{
  va_list ap;

    va_start(ap, format);
#if 1
    fprintf (stderr, "%s: %s%s",
             strcurtime(), progname, progname[0] != '\0' ? ": " : "");
    vfprintf(stderr, format, ap);
    fprintf (stderr, "\n");
#endif
    va_end(ap);
}


static int FindClient(int fd)
{
  int  n;

    for (n = 0;  n < clients_allocd;  n++)
        if (clients[n].s == fd) return n;

    return -1;
}

static int GrowClients(void)
{
  enum           {GROW_INCREMENT = 1};
  gusev_client_t *new_clients;
  int             n;

    new_clients = safe_realloc(clients, sizeof(*clients) * (clients_allocd + GROW_INCREMENT));
    if (new_clients == NULL) return -1;
    clients = new_clients;
    
    for (n = 0;  n < GROW_INCREMENT;  n++)
        clients[clients_allocd + n].s = FD_UNUSED;
    clients_allocd += GROW_INCREMENT;

    return 0;
}

static void CloseClient(gusev_client_t *cp)
{
    fdio_deregister(cp->fhandle);
    close          (cp->s);
    cp->s = -1;
}


static void SendDataTo(int cn, int SqN)
{
  gusev_client_t         *cp  = clients + cn;
  char                   *addr_s;

  struct
  {
      gusev_pkt_header  hdr;
      double            values[1000];
  } pkt;
  int                     x;

    bzero(&pkt.hdr, sizeof(pkt.hdr));
    pkt.hdr.len   = sizeof(pkt.hdr) + sizeof(pkt.values[0]) * watched_chans_count;
    pkt.hdr.magic = GUS_MAGIC;
    strzcpy(pkt.hdr.ID, senddata_changer, sizeof(pkt.hdr.ID));
    pkt.hdr.SqN   = SqN;
    pkt.hdr.OpC   = GUS_CMD_GETALLDATA;
    
    for (x = 0;  x < watched_chans_count;  x++)
        pkt.values[x] = (watched_chans[x].ki != NULL)? watched_chans[x].ki->curv : NAN;
  
    if (fdio_send(cp->fhandle, &pkt, pkt.hdr.len) < 0)
    {
        addr_s = inet_ntoa(cp->addr);
        reporterror("%s(%s): %s", __FUNCTION__, addr_s, strerror(errno));
        CloseClient(cp);
    }
}


static void ProcessPacket(int uniq, void *unsdptr,
                          fdio_handle_t handle, int reason,
                          void *inpkt,  size_t inpktsize,
                          void *privptr)
{
  int                     cn  = ptr2lint(privptr);
  gusev_client_t         *cp  = clients + cn;
  gusev_pkt_header       *hdr = (gusev_pkt_header *)inpkt;
  char                   *addr_s;

  gusev_pkt_SETVALUE_t   *pkt_SETVALUE   = (gusev_pkt_SETVALUE_t *)   hdr;
  gusev_pkt_SETIMGPARS_t *pkt_SETIMGPARS = (gusev_pkt_SETIMGPARS_t *) hdr;

  int                   n;
  
    addr_s = inet_ntoa(cp->addr);
  
    switch (reason)
    {
        case FDIO_R_DATA:
            break;

        case FDIO_R_INPKT2BIG:
            reporterror("%s: fdiolib: received packet is too big", addr_s);
            CloseClient(cp);
            return;

        case FDIO_R_ENOMEM:
            reporterror("%s: fdiolib: out of memory", addr_s);
            CloseClient(cp);
            return;

        case FDIO_R_CLOSED:
        case FDIO_R_IOERR:
        default:
            reporterror("%s: fdiolib: %s", addr_s, fdio_strreason(reason));
            CloseClient(cp);
            return;
    }

    if (hdr == NULL)
    {
        /*!!! ???*/

        return;
    }

    if (hdr->magic != GUS_MAGIC)
    {
        reporterror("%s: hdr.magic==%08x, !=%d", addr_s, hdr->magic, GUS_MAGIC);
        return;
    }
    
    fprintf(stderr, "\t<%s: %d\n", addr_s, hdr->OpC);
    
    switch (hdr->OpC)
    {
        case GUS_CMD_SETVALUE:
            if (inpktsize != sizeof(gusev_pkt_SETVALUE_t))
            {
                reporterror("%s: SETVALUE: inpktsize=%zu, !=%zu",
                            addr_s, inpktsize, sizeof(gusev_pkt_SETVALUE_t));
                return;
            }
            n = pkt_SETVALUE->n;
            if (n < watched_chans_count)
            {
                if (watched_chans[n].ki != NULL)
                {
                    SetControlValue(watched_chans[n].ki, pkt_SETVALUE->value, 1);

                    senddata_after = 2;
                    hdr->ID[sizeof(hdr->ID)-1] = 0;
                    strzcpy(senddata_changer, hdr->ID, sizeof(senddata_changer));
                }
            }
            else
                reporterror("%s: SETVALUE: n=%d, >%d", addr_s, n, watched_chans_count-1);
            break;
            
        case GUS_CMD_GETALLDATA:
            SendDataTo(cn, hdr->SqN);
            break;
            
        case GUS_CMD_SETOSCILLPARS:
            break;
            
        case GUS_CMD_GETOSCILL:
            break;
            
        case GUS_CMD_ALFO:
            break;
            
        case GUS_CMD_STARTCAMERA:
            break;
            
        case GUS_CMD_SETIMGPARS:
            if (inpktsize != sizeof(gusev_pkt_SETIMGPARS_t))
            {
                reporterror("%s: SETVALUE: inpktsize=%zu, !=%zu",
                            addr_s, inpktsize, sizeof(gusev_pkt_SETIMGPARS_t));
                return;
            }
            if (set_kt_proc != NULL)
                set_kt_proc(set_kt_privptr,
                            pkt_SETIMGPARS->brightness,
                            pkt_SETIMGPARS->exposition);
            break;
            
        case GUS_CMD_GETIMG:
            break;

        default:
            reporterror("%s: unknown OpC=%d", addr_s, hdr->OpC);
    }
}

static void AcceptConnection(int uniq, void *unsdptr,
                             fdio_handle_t handle, int reason,
                             void *inpkt __attribute((unused)), size_t inpktsize __attribute((unused)),
                             void *privptr)
{
  int                 is_camera  = ptr2lint(privptr);
  int                 s;
  struct sockaddr_in  clnaddr;
  socklen_t           clnaddrlen = sizeof(clnaddr);
  int                 on = 1;

  char               *addr_s;
  int                 n;
  gusev_client_t     *cp;
    
    if (reason != FDIO_R_ACCEPT) return; /*?!*/

    s = fdio_accept(handle, (struct sockaddr *)&clnaddr, &clnaddrlen);
    if (s < 0)
    {
        reporterror("%s: fdio_accept=%d, %s", __FUNCTION__, s, strerror(errno));
        return;
    }

    addr_s = inet_ntoa(clnaddr.sin_addr);
    /* Sanity check... */
    if (addr_s == NULL)  addr_s = "KU-KA-RE-KU";
    reporterror("%s: connection from %s",
                __FUNCTION__, addr_s);
    
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));

    n = FindClient(FD_UNUSED);
    if (n < 0)
    {
        if (GrowClients() < 0  ||  (n = FindClient(FD_UNUSED)) < 0)
        {
            reporterror("unable to find a slot for client...");
            close(s);
            return;
        }
    }

    cp = clients + n;
    cp->s         = s;
    cp->addr      = clnaddr.sin_addr;
    cp->is_camera = is_camera;
    cp->fhandle   = fdio_register_fd(0, NULL, /*!!!uniq*/
                                     s, FDIO_STREAM, ProcessPacket, lint2ptr(n),
                                     1000000,
                                     sizeof(gusev_pkt_header),
                                     FDIO_OFFSET_OF(gusev_pkt_header, len),
                                     FDIO_SIZEOF   (gusev_pkt_header, len),
                                     FDIO_LEN_LITTLE_ENDIAN,
                                     1,
                                     0);
}

static int create_lsock(const char *funcname, int port_n, int is_camera)
{
  int                 lsock;
  int                 r;                /* Result of system calls */
  struct sockaddr_in  iaddr;            /* Address to bind `inet_entry' to */
  int                 on     = 1;       /* "1" for REUSE_ADDR */
  
    if (port_n <= 0) return 0;
  
    lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock < 0)
    {
        reporterror("%s: unable to create listening socket: %s",
                    strerror(errno), funcname);
        return -1;
    }

    /* Bind it to the name */
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    iaddr.sin_family      = AF_INET;
    iaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    iaddr.sin_port        = htons(port_n);
    r = bind(lsock, (struct sockaddr *) &iaddr, sizeof(iaddr));
    if (r < 0)
    {
        reporterror("%s: unable to bind to port %d: %s",
                    funcname, port_n, strerror(errno));
        return -1;
    }

    /* Mark it as listening */
    listen(lsock, 5);
    
    r = fdio_register_fd(0, NULL, /*!!!uniq*/
                         lsock, FDIO_LISTEN, AcceptConnection, lint2ptr(is_camera),
                         0, 0, 0, 0, FDIO_LEN_LITTLE_ENDIAN, 0, 0);

    return 0;
}

void ndbp_staroio_Listen(XhWindow win, int port1, int port2, const char *chanlist)
{
  int                 n;
  gusev_chaninfo_t   *wp;

    fprintf(stderr, "%zu %zu .n=%zu .v=%zu\n", sizeof(gusev_pkt_header), sizeof(gusev_pkt_SETVALUE_t), offsetof(gusev_pkt_SETVALUE_t, n), offsetof(gusev_pkt_SETVALUE_t, value));
#if OPTION_HAS_PROGRAM_INVOCATION_NAME /* With GNU libc+ld we can determine the true argv[0] */
    if (progname[0] == '\0') strzcpy(progname, program_invocation_short_name, sizeof(progname));
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */

    create_lsock(__FUNCTION__, port1, 0);
    create_lsock(__FUNCTION__, port2, 1);
    
    /* Handle list of channels, if any */
    if (chanlist != NULL  &&  chanlist[0] != '\0')
    {
        ourwin              = win;
        
        watched_chans_count = countstrings(chanlist);
        watched_chans       = (void *)XtMalloc(sizeof(watched_chans[0]) * watched_chans_count);

        for (n = 0, wp = watched_chans;  n < watched_chans_count;  n++, wp++)
        {
            bzero(wp, sizeof(*wp));
            extractstring(chanlist, n, wp->name, sizeof(wp->name));
            if ((wp->ki =
                 ChlFindKnob(ourwin, wp->name)) == NULL)
            {
                reporterror("channel #%d \"%s\" is unaccessible: %s",
                            n, wp->name, strerror(errno));
            }
        }
    }
}

void ndbp_staroio_NewData(void)
{
  int  n;
    
    if (senddata_after == 0) return;
    senddata_after--;
    if (senddata_after != 0) return;

    for (n = 0;  n < clients_allocd;  n++)
        if (clients[n].s != FD_UNUSED  &&  !(clients[n].is_camera))
            SendDataTo(n, 0);
}
