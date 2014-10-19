#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "misc_types.h"
#include "misclib.h"

#include "tsycamlib.h"


#define DPRINTF(level, format_str, args...) \
    fprintf(stderr, "%s::%s: " format_str "\n", __FILE__, __FUNCTION__, ## args)


enum {CAMERA_MAGIC = 0xDEADFACE};

enum
{
    TIMEOUT_INITIAL   =  20*1000 * 1000, INITIAL_COUNT = 2,
    TIMEOUT_REGET     =   100 * 1000,
    TIMEOUT_CONFIRMED = 10000 * 1000
};

enum {PARAM_K = 0, PARAM_T = 1, PARAM_SYNC = 2, PARAM_RRQ = 3, PARAM_MEM = 4,
    PARAM_LAST = PARAM_MEM};

struct {const char *name; int id;} param_info[] =
{
    {"K",    PARAM_K},
    {"T",    PARAM_T},
    {"SYNC", PARAM_SYNC},
    {"RRQ",  PARAM_RRQ},
    {"MEM",  PARAM_MEM},
    {NULL, 0}
};

enum
{
    STATE_READY = 0, STATE_INITIAL = 1, STATE_REGET = 2
};

typedef struct
{
    unsigned int        magic;
    int                 fd;
    struct sockaddr_in  cam_addr;
    int                 w;
    int                 h;
    int                 state;
    int                 init_ctr;
    char               *ispresent;
    int                 rest;
    int                 rrq_rest;
    int                 smth_rcvd;
    void               *framebuffer;
    size_t              fbusize;
    uint8               curframe;
    int                 params[PARAM_LAST + 1];
} cameraobject_t;

typedef struct
{
    uint8  F_FirstLine_L;
    uint8  F_FirstLine_H;
    uint8  F_LastLine_L;
    uint8  F_LastLine_H;
    uint8  Flags;
    uint8  K;
    uint8  L_LastLine_L;
    uint8  L_LastLine_H;
    uint8  L_FirstLine_L;
    uint8  L_FirstLine_H;
    uint8  Time_L;
    uint8  Time_H;
    uint8  SMD;
    uint8  FrameID;
} reqhdr_t;

enum {FLAG_SMDE = 1, FLAG_SYNC = 2, FLAG_PS = 4, FLAG_FSE = 8, FLAG_MEM = 128};
enum {SMD_SMD1 = 2, SMD_SMD2 = 1};

#define CHECK_OBJ(obj, errret) \
    if(obj == NULL  ||  obj->magic != CAMERA_MAGIC) do {errno = EINVAL; return errret;}while (0)

static void ScheduleTimeout(cameraobject_t *obj, int *timeout_p, int usecs)
{
    if (timeout_p != NULL)
        *timeout_p = usecs;
}

static void FillReqHdr(reqhdr_t *hdr,
                       int8 Flags, int8 K, int Time,
                       int FirstLine, int LastLine,
                       uint8 SMD,
                       uint8 FrameID)
{
#define lo(x) (x & 0xFF)
#define hi(x) ((x >> 8) & 0xFF)
    
    bzero(hdr, sizeof(*hdr));

    hdr->Flags   = Flags;
    hdr->K       = K;
    hdr->Time_L  = lo(Time);
    hdr->Time_H  = hi(Time);
    hdr->F_FirstLine_L = hdr->L_FirstLine_L = lo(FirstLine);
    hdr->F_FirstLine_H = hdr->L_FirstLine_H = hi(FirstLine);
    hdr->F_LastLine_L  = hdr->L_LastLine_L  = lo(LastLine);
    hdr->F_LastLine_H  = hdr->L_LastLine_H  = hi(LastLine);
    hdr->SMD     = SMD;
    hdr->FrameID = FrameID;

#undef lo
#undef hi
}

#define TC_DECLARE_DECODELINE(nn)                                   \
static void tc_decodeline##nn(void *src, void *dst, int n_quintets) \
{                                                                   \
  register uint8   *srcp = src;                                     \
  register int##nn *dstp = dst;                                     \
  uint16            b0, b1, b2, b3, b4;                             \
  int               x;                                              \
                                                                    \
    for (x = 0;  x < n_quintets;  x++)                              \
    {                                                               \
        b0 = *srcp++;                                               \
        b1 = *srcp++;                                               \
        b2 = *srcp++;                                               \
        b3 = *srcp++;                                               \
        b4 = *srcp++;                                               \
                                                                    \
        *dstp++ = ((b0     )     ) | ((b1 & 3 ) << 8);              \
        *dstp++ = ((b1 >> 2) & 63) | ((b2 & 15) << 6);              \
        *dstp++ = ((b2 >> 4) & 15) | ((b3 & 63) << 4);              \
        *dstp++ = ((b3 >> 6) & 3)  | ((b4     ) << 2);              \
    }                                                               \
}

TC_DECLARE_DECODELINE(16)
TC_DECLARE_DECODELINE(32)

void *tc_create_camera_object(const char *host, short port,
                              int w, int h,
                              void *framebuffer, size_t fbusize)
{
  cameraobject_t *obj = NULL;

  unsigned long   host_ip;
  struct hostent *hp;

  socklen_t       bsize;                     // Parameter for setsockopt
  socklen_t       bsizelen = sizeof(bsize);  // And its length
  
    if (port == 0) port = DEFAULT_TSYCAM_PORT;

    if (fbusize != sizeof(int16)  &&  fbusize != sizeof(int32))
    {
        errno = EINVAL;
        return NULL;
    }
    
    /* Allocate an object and its data structures */
    if ((obj = malloc(sizeof(cameraobject_t))) == NULL)
        return NULL;
    bzero(obj, sizeof(*obj));
    obj->magic = CAMERA_MAGIC;
    
    obj->w = w;
    obj->h = h;

    /* Create a vector of scanline received=1/missing=0 flags */
    if ((obj->ispresent = malloc(h * sizeof(*(obj->ispresent)))) == NULL)
        goto ERR_EXIT;

    /* Find out IP of the host */

    /* First, is it in dot notation (aaa.bbb.ccc.ddd)? */
    host_ip = inet_addr(host);

    /* No, should do a hostname lookup */
    if (host_ip == INADDR_NONE)
    {
        hp = gethostbyname(host);
        /* No such host?! */
        if (hp == NULL)
        {
            errno = ENOENT;
            goto ERR_EXIT;
        }

        memcpy(&host_ip, hp->h_addr, hp->h_length);
    }

    /* Record camera's ip:port */
    obj->cam_addr.sin_family = AF_INET;
    obj->cam_addr.sin_port   = htons(port);
    memcpy(&(obj->cam_addr.sin_addr), &host_ip, sizeof(host_ip));

    /* Create a UDP socket */
    if ((obj->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        goto ERR_EXIT;
    /* Turn it into nonblocking mode */
    set_fd_flags(obj->fd, O_NONBLOCK, 1);
    /* And set rcv buffer size */
    getsockopt(obj->fd, SOL_SOCKET, SO_RCVBUF, (char *) &bsize, &bsizelen);
//    fprintf(stderr, "RCVBUF=%d\n", bsize);
    bsize=327680*2;
    setsockopt(obj->fd, SOL_SOCKET, SO_RCVBUF, (char *) &bsize, sizeof(bsize));
    getsockopt(obj->fd, SOL_SOCKET, SO_RCVBUF, (char *) &bsize, &bsizelen);
//    fprintf(stderr, "RCVBUF=%d\n", bsize);

    obj->framebuffer = framebuffer;
    obj->fbusize     = fbusize;
    
    return obj;
    
 ERR_EXIT:
    if (obj->ispresent) free(obj->ispresent);
    if (obj)            free(obj);
    return NULL;
}

int   tc_set_parameter    (void *ref, const char *name, int value)
{
  cameraobject_t *obj = ref;
  int             n;
    
    CHECK_OBJ(obj, -1);
  
    for (n = 0;  param_info[n].name != NULL;  n++)
        if (strcasecmp(param_info[n].name, name) == 0)
        {
            obj->params[param_info[n].id] = value;
            ////fprintf(stderr, "'%s':=%d\n", name, value);
            return 0;
        }
    
    return -1;
}

static int SendReq(cameraobject_t *obj, int first, int last, int rrq)
{
  reqhdr_t        hdr;
  int             K    = obj->params[PARAM_K];
  int             T    = obj->params[PARAM_T];
  int             SYNC = obj->params[PARAM_SYNC] != 0;
  int             MEM  = obj->params[PARAM_MEM]  != 0  ||  rrq;

    FillReqHdr(&hdr,
               FLAG_SMDE*1 | FLAG_SYNC*SYNC | FLAG_MEM*MEM,
               K, T,
               first, last,
               SMD_SMD1*0 | SMD_SMD2*1,
               obj->curframe);

    return sendto(obj->fd,
                  &hdr, sizeof(hdr), /*!!! Shouldn't we do connect() and just send()? */
                  0,
                  (struct sockaddr *)(&(obj->cam_addr)), sizeof(obj->cam_addr));
}

static void RequestAFrame      (cameraobject_t *obj, int *timeout_p)
{
    SendReq(obj, 0, obj->h - 1, 0);
    ScheduleTimeout(obj, timeout_p, !obj->params[PARAM_MEM]? TIMEOUT_INITIAL/INITIAL_COUNT : TIMEOUT_REGET);
}

static void RequestMissingLines(cameraobject_t *obj, int *timeout_p)
{
  char *b_p;
  char *e_p;

    b_p = memchr(obj->ispresent, 0, obj->h);
    e_p = memchr(b_p,            1, obj->h - (b_p - obj->ispresent));
    if (e_p == NULL) e_p = obj->ispresent + obj->h - 1;

    SendReq(obj, b_p - obj->ispresent, e_p - obj->ispresent, 1);
    fprintf(stderr, "Requesting missing %d-%d\n", (int)(b_p - obj->ispresent), (int)(e_p - obj->ispresent));
    
    obj->state = STATE_REGET;
    obj->rrq_rest = e_p - b_p + 1;
    obj->smth_rcvd = 0;
    ScheduleTimeout(obj, timeout_p, TIMEOUT_REGET);
}

int   tc_request_new_frame(void *ref, int *timeout_p)
{
  cameraobject_t *obj = ref;
  reqhdr_t        hdr;
  int             r;

    CHECK_OBJ(obj, -1);
    if (obj->state != STATE_READY) return -1;
    bzero(obj->ispresent, obj->h * sizeof(*(obj->ispresent)));
    obj->rest = obj->h;
    bzero(obj->framebuffer, (obj->w * obj->h) * obj->fbusize);
    obj->state = STATE_INITIAL; /*!!! Should it be here or where?  *_CRITICAL()? */
    obj->init_ctr = 0;
    
    obj->curframe++;

    r = SendReq(obj, 0, obj->h - 1, 0);
    if (r != sizeof(hdr)) return -1;

    obj->smth_rcvd = 0;
    ScheduleTimeout(obj, timeout_p, !obj->params[PARAM_MEM]? TIMEOUT_INITIAL/INITIAL_COUNT : TIMEOUT_REGET);
    
    return 0;
}

typedef void (*tc_decoder_t) (void *src, void *dst, int n_quintets);

int   tc_decode_frame     (void *ref)
{
  cameraobject_t *obj = ref;
  int             y;
  char           *srcp;
  char           *dstp;
  size_t          payload_size;
  size_t          scanline_size;
  size_t          src_offset;
  int             n_quintets;
  tc_decoder_t    decodeline;

    CHECK_OBJ(obj, -1);

    payload_size  = (obj->w / 4) * 5;
    scanline_size = obj->w * obj->fbusize;
    src_offset    = scanline_size - payload_size;
    n_quintets    = obj->w / 4;

    decodeline = tc_decodeline32;
    if (obj->fbusize == sizeof(int16)) decodeline = tc_decodeline16;
    
    for (y = 0, srcp = (char *)(obj->framebuffer) + src_offset, dstp = (char *)(obj->framebuffer);
         y < obj->h;
         y++,   srcp += scanline_size,                          dstp += scanline_size)
        decodeline(srcp, (int32 *)dstp, n_quintets);

    return 0;
}

int   tc_missing_count    (void *ref)
{
  cameraobject_t *obj = ref;

    CHECK_OBJ(obj, -1);

    return obj->rest;
}

int   tc_fd_of_object(void *ref)
{
  cameraobject_t *obj = ref;

    CHECK_OBJ(obj, -1);
    return obj->fd;
}

int   tc_fdio_proc   (void *ref, int *timeout_p)
{
  cameraobject_t *obj = ref;
  uint8           packet[2000];
  int             frame_n;
  int             line_n;
  int             r;
  int             repcount = 0;
  size_t          payload_size;
  size_t          scanline_size;
  void           *dstp;
  int             usecs = 0;
    
    CHECK_OBJ(obj, -1);

    payload_size  = (obj->w / 4) * 5; // 5 bytes per 4 pixels
    scanline_size = obj->w * obj->fbusize;
    
    do
    {
        r = read(obj->fd, packet, sizeof(packet));
        if (r == 0)
        {
            /* What can it be? */
            //KillObject(obj);
            return -1;
        }
        if (r < 0) 
        {
            if (errno == EAGAIN  ||  errno == EWOULDBLOCK)
            {
                /* Okay, have just emptied the buffer */

                /* Sure?  Have we read at least one packet? */
                if (repcount == 0)
                {
                    /* O-ops!  The caller had confused something... */
                    DPRINTF(0, "tc_fdio_proc() called w/o data!");
                    return -1;
                }

                /* Okay, everything is fine -- just have to wait for more data */
                goto NORMAL_EXIT;
            }

            /* Ouch, some error... :-( */
            return -1;
        }
//        if (r != sizeof(packet))
//        {
//            DPRINTF(0, "illegal packet size: %d!=%d", r, sizeof(packet));
//            return -1;
//        }

        /* Okay, we've reached a point where we r sure that it is a real packet ;-) */
        frame_n = packet[0] + ((unsigned int)packet[1]) * 256;
        line_n  = packet[2] + ((unsigned int)packet[3]) * 256 - 1;
//            fprintf(stderr, "%d,%d ", r, line_n); fflush(stderr);

        /* Check frame # */
        if (frame_n != obj->curframe)
        {
            //DPRINTF(0, "frame_n=%d != curframe=%d", frame_n, obj->curframe);
            goto CONTINUE_WHILE;
        }

        /* Check line # */
        if (line_n == 12345  && obj->state == STATE_INITIAL)
        {
            usecs = TIMEOUT_CONFIRMED;
            goto CONTINUE_WHILE;
        }
        if (line_n >= obj->h  &&  1)
        {
            DPRINTF(0, "line=%d >= h=%d", line_n, obj->h);
            goto CONTINUE_WHILE;
        }

//        if (obj->state != STATE_REGET  &&  line_n > obj->curframe  &&  line_n < obj->curframe + 40)
//            goto CONTINUE_WHILE;  /* Debugging measure */
        
        if (!obj->ispresent[line_n])
        {
            if (obj->smth_rcvd == 0  &&  obj->state != STATE_REGET) usecs = TIMEOUT_REGET;
            obj->smth_rcvd = 1;
            dstp = ((int8 *)(obj->framebuffer)) +
                   scanline_size * (line_n + 1) -
                   payload_size;
            memcpy(dstp, &(packet[4]), payload_size);
            
            obj->ispresent[line_n] = 1;
            obj->rest--;
            if (obj->rest == 0)
            {
                obj->state = STATE_READY;
                ScheduleTimeout(obj, timeout_p, -1);
                return 0;
            }
            
            if (obj->state == STATE_REGET)
            {
                obj->rrq_rest--;
                if (obj->rrq_rest == 0)
                {
                    RequestMissingLines(obj, timeout_p);
                    return 1;
                }
            }
        }
        else
        {
            //DPRINTF(0, "duplicate scanline %d", line_n);
            goto CONTINUE_WHILE;
        }
 CONTINUE_WHILE:;
    }
    while (++repcount < obj->h);

 NORMAL_EXIT:
    /* Okay, we are here since something was read, but not everything */
    ScheduleTimeout(obj, timeout_p, usecs);
    return 1;
}

int   tc_timeout_proc(void *ref, int *timeout_p)
{
  cameraobject_t *obj = ref;

    CHECK_OBJ(obj, -1);

    ////fprintf(stderr, "TIMEOUT!!!%d!!!\n", obj->rest);
    
    if (obj->state == STATE_READY)
    {
        ScheduleTimeout(obj, timeout_p, 0);
        return 1;
    }

    /**/
    if (obj->state == STATE_INITIAL)
    {
        if (obj->smth_rcvd == 0  &&  obj->init_ctr < INITIAL_COUNT)
        {
            RequestAFrame(obj, timeout_p);
            obj->init_ctr++;
            return 1;
        }
    }
    
    /* Okay, have we received something at all? */
    if (obj->smth_rcvd  &&  obj->params[PARAM_RRQ])
    {
        RequestMissingLines(obj, timeout_p);
        return 1;
    }
    
    obj->state = STATE_READY;
    ScheduleTimeout(obj, timeout_p, 0);
    return 0;
}
