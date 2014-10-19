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

#include "misc_macros.h"
#include "misclib.h"
#include "sendqlib.h"

#include "cxsd_driver.h"

#include "drv_i/ottcamv_drv_i.h"


//////////////////////////////////////////////////////////////////////
enum
{
    OTTREQ_COMM = 0x636f6d6d, // "comm"
    OTTRPY_EXPO = 0x6578706f, // "expo"
    OTTRPY_NEXP = 0x6e657870, // "nexp"
    OTTRPY_ACKN = 0x61636b6e, // "ackn"
    OTTRPY_NACK = 0x6e61636b, // "nack"
    OTTRPY_DATA = 0x64617461, // "data"
};

enum // OTTREQ_COMM.cmd
{
    OTTCMD_SETMODE   = 0, // .par1=mode
    OTTCMD_NEW_FRAME = 1, //
    OTTCMD_SND_FRAME = 2, // .par1,.par2 - line range
    OTTCMD_SET_FSIZE = 3, // .par1=width, .par2=height
    OTTCMD_RESERVED4 = 4,
    OTTCMD_WRITEREG  = 5, // .par1=addr, .par2=value
    OTTCMD_READREG   = 6, // .par1=addr
    OTTCMD_RESET     = 7,
};

enum // OTTREQ_COMM:OTTCMD_SETMODE.par1
{
    OTTMODE_INACTIVITY  = 0,
    OTTMODE_SINGLE_SHOT = 1,
    OTTMODE_MONITOR     = 2, // Not implemented
    OTTMODE_SERIAL      = 3, // Not implemented
};


typedef struct
{
    uint32  id;
    uint16  seq;
    uint16  cmd;
    uint16  par1;
    uint16  par2;
} ottcam_hdr_t;

typedef struct
{
    uint32  id;
    uint16  seq;
    uint16  line_n;
    uint8   data[0];
} ottcam_line_t;

//////////////////////////////////////////////////////////////////////

enum {PIX_W = OTTCAMV_WIDTH, PIX_H = OTTCAMV_HEIGHT}; // 752!!!
enum {PAYLOAD_SIZE = (PIX_W * 4 / 3)};
enum {DEFAULT_OTTCAM_PORT = 4001};

enum
{
    STATE_READY = 0, STATE_INITIAL = 1, STATE_REGET = 2
};

enum
{
    SIMSQ_W = 60,
    SIMSQ_H = 50,
    SIM_PERIOD = 200*1000 // 20ms
};


enum
{
    OTTCAMV_SENDQ_SIZE = 100,
    OTTCAMV_SENDQ_TOUT = 100000, // 100ms
};
typedef struct
{
    sq_eprops_t       props;
    ottcam_hdr_t      hdr;
} ottqelem_t;

typedef struct
{
    int         is_sim;
    int         req_sent;

    int         devid;
    int         fd;

    sl_tid_t    act_tid;

    int         state;
    
    int         requested[OTTCAMV_NUM_BIGCS];
    void       *databuf  [OTTCAMV_NUM_BIGCS];
    
    int32       cur_args [OTTCAMV_NUM_PARAMS];
    int32       nxt_args [OTTCAMV_NUM_PARAMS];

    uint16      curframe;
    int         rest;
    int         rrq_rest;
    int         smth_rcvd;
    uint8       ispresent[PIX_H];

    int         sim_x;
    int         sim_y;
    int         sim_vx;
    int         sim_vy;

    ////////
    sq_q_t      q;
    sl_tid_t    sq_tid;
} privrec_t;

#define ACCESS_RAW_LINE_N(me, n) \
    ((uint8 *)(me->databuf[OTTCAMV_BIGC_RAW]) + PAYLOAD_SIZE * n)

static struct
{
    size_t  datasize;
    size_t  dataunits;
}
bigcprops[OTTCAMV_NUM_BIGCS] =
{
    [OTTCAMV_BIGC_RAW]   = {PIX_W * PIX_H * 4 / 3,          sizeof(uint8)},
    [OTTCAMV_BIGC_16BIT] = {PIX_W * PIX_H * sizeof(uint16), sizeof(uint16)},
    [OTTCAMV_BIGC_32BIT] = {PIX_W * PIX_H * sizeof(uint32), sizeof(uint32)},
};

//////////////////////////////////////////////////////////////////////

static void sim_heartbeat(int devid, void *devptr,
                          sl_tid_t tid,
                          void *privptr)
{
  privrec_t *me = (privrec_t *) devptr;
  int        bigc;
  
  int        x;
  int        y;
  int16     *buf16;
  int16     *p16;

    fprintf(stderr, "%s [%d] %s()\n", strcurtime(), devid, __FUNCTION__);

    me->act_tid = -1;

    memcpy(me->cur_args, me->nxt_args, sizeof(me->cur_args));
  
    me->cur_args[OTTCAMV_PARAM_MISS] = 0;
    me->cur_args[OTTCAMV_PARAM_W]    = PIX_W;
    me->cur_args[OTTCAMV_PARAM_H]    = PIX_H;

    me->sim_x += me->sim_vx;
    me->sim_y += me->sim_vy;

    if (me->sim_x < 0)
    {
        me->sim_x  = -me->sim_x;
        me->sim_vx = -me->sim_vx;
    }
    if (me->sim_y < 0)
    {
        me->sim_y  = -me->sim_y;
        me->sim_vy = -me->sim_vy;
    }
    if (me->sim_x > PIX_W-SIMSQ_W)
    {
        me->sim_x  = (PIX_W-SIMSQ_W) - (me->sim_x - (PIX_W-SIMSQ_W));
        me->sim_vx = -me->sim_vx;
    }
    if (me->sim_y > PIX_H-SIMSQ_H)
    {
        me->sim_y  = (PIX_H-SIMSQ_H) - (me->sim_y - (PIX_H-SIMSQ_H));
        me->sim_vy = -me->sim_vy;
    }
    
    for (bigc = 0;  bigc < OTTCAMV_NUM_BIGCS;  bigc++)
        if (me->requested[bigc])
        {
            me->requested[bigc] = 0;
            bzero(me->databuf[bigc], bigcprops[bigc].datasize);
            
            if (bigc == OTTCAMV_BIGC_16BIT)
            {
                buf16 = me->databuf[bigc];

                for (y = 0;  y < SIMSQ_H;  y++)
                    for (x = 0, p16 = buf16 + (y + me->sim_y) * PIX_W + me->sim_x;
                         x < SIMSQ_W;
                         x++,   p16++)
                        *p16 = (me->cur_args[OTTCAMV_PARAM_T] + me->cur_args[OTTCAMV_PARAM_K] + x*4 + y*4) & 1023;
            }

            ReturnBigc(devid, bigc, me->cur_args, OTTCAMV_NUM_PARAMS,
                       me->databuf[bigc], bigcprops[bigc].datasize, bigcprops[bigc].dataunits,
                       0);
        }

    me->act_tid = sl_enq_tout_after(devid, devptr, SIM_PERIOD, sim_heartbeat, NULL);
}

//////////////////////////////////////////////////////////////////////

static int  ottcamv_sender     (void *elem, void *devptr)
{
  privrec_t    *me = devptr;
  ottqelem_t   *qe = elem;
  ottcam_hdr_t  hdr;
  int           r;

    hdr.id   = htonl(qe->hdr.id);
    hdr.seq  = htons(qe->hdr.seq);
    hdr.cmd  = htons(qe->hdr.cmd);
    hdr.par1 = htons(qe->hdr.par1);
    hdr.par2 = htons(qe->hdr.par2);
    r = send(me->fd, &hdr, sizeof(hdr), 0);
    ////fprintf(stderr, "send=%d\n", r);
    return r > 0? 0 : -1;
}

static int  ottcamv_eq_cmp_func(void *elem, void *ptr)
{
  ottcam_hdr_t *a = &(((ottqelem_t *)elem)->hdr);
  ottcam_hdr_t *b = &(((ottqelem_t *)ptr )->hdr);

    // We DON'T compare id, since only OTTREQ_COMM can be in queue
    if (a->seq != b->seq  ||  a->cmd != b->cmd) return 0;

    if (a->cmd  == OTTCMD_SETMODE    &&
        a->par1 == b->par1)                         return 1;
    if (a->cmd  == OTTCMD_NEW_FRAME)                return 1;
    if (a->cmd  == OTTCMD_SND_FRAME  &&
        a->par1 == b->par1  &&  a->par2 == b->par2) return 1;
    if (a->cmd  == OTTCMD_SET_FSIZE  &&
        a->par1 == b->par1  &&  a->par2 == b->par2) return 1;
    if (a->cmd  == OTTCMD_WRITEREG   &&
        a->par1 == b->par1  &&  a->par2 == b->par2) return 1;
    if (a->cmd  == OTTCMD_READREG    &&
        a->par1 == b->par1)                         return 1;

    return 0;
}

static void tout_proc   (int devid, void *devptr,
                         sl_tid_t tid,
                         void *privptr)
{
  privrec_t  *me = devptr;
  
    me->sq_tid = -1;
    sq_timeout(&(me->q));
}

static int  tim_enqueuer(void *devptr, int usecs)
{
  privrec_t  *me = devptr;

////fprintf(stderr, "%s(%d)\n", __FUNCTION__, usecs);
    if (me->sq_tid >= 0)
    {
        sl_deq_tout(me->sq_tid);
        me->sq_tid = -1;
    }
    me->sq_tid = sl_enq_tout_after(me->devid, devptr, usecs, tout_proc, NULL);
    return me->sq_tid >= 0? 0 : -1;
}

static void tim_dequeuer(void *devptr)
{
  privrec_t  *me = devptr;
  
    if (me->sq_tid >= 0)
    {
        sl_deq_tout(me->sq_tid);
        me->sq_tid = -1;
    }
}

static sq_status_t enq_pkt(privrec_t *me, 
                           uint16     cmd,
                           uint16     par1,
                           uint16     par2)
{
  ottqelem_t  item;
  sq_status_t r;

    bzero(&item, sizeof(item));
    
    item.props.tries      = SQ_TRIES_INF;
    item.props.timeout_us = OTTCAMV_SENDQ_TOUT;

    item.hdr.id   = OTTREQ_COMM;
    item.hdr.seq  = me->curframe;
    item.hdr.cmd  = cmd;
    item.hdr.par1 = par1;
    item.hdr.par2 = par2;

    r = sq_enq(&(me->q), &(item.props), SQ_ALWAYS, NULL);
////fprintf(stderr, "%s()=%d %d %d\n", __FUNCTION__, r, me->q.ring_used, me->q.tout_set);
    return r;
}

static sq_status_t esn_pkt(privrec_t *me,
                           uint16     cmd,
                           uint16     par1,
                           uint16     par2)
{
  ottqelem_t  item;
  void       *fe;

    bzero(&item, sizeof(item));
    
    item.hdr.id   = OTTREQ_COMM;
    item.hdr.seq  = me->curframe;
    item.hdr.cmd  = cmd;
    item.hdr.par1 = par1;
    item.hdr.par2 = par2;

    /* Check "whether we should really erase and send next packet" */
    fe = sq_access_e(&(me->q), 0);
    if (fe != NULL  &&  ottcamv_eq_cmp_func(fe, &item) == 0)
        return SQ_NOTFOUND;

    /* Erase... */
    sq_foreach(&(me->q), SQ_ELEM_IS_EQUAL, &item, SQ_ERASE_FIRST,  NULL);
    /* ...and send next */
    sq_sendnext(&(me->q));

    return SQ_FOUND;
}

//--------------------------------------------------------------------

enum
{
    TIMEOUT_INITIAL = 1000*1000,
    TIMEOUT_REGET   =  100*1000,
};

#define TC_DECLARE_DECODE(nn)                                   \
static void tc_decode##nn(void *src, void *dst, int n_uint32s) \
{                                                                   \
  register uint8   *srcp = src;                                     \
  register int##nn *dstp = dst;                                     \
  uint16            b0, b1, b2, b3;                                 \
  int               x;                                              \
                                                                    \
    for (x = 0;  x < n_uint32s;  x++)                               \
    {                                                               \
        b3 = *srcp++;                                               \
        b2 = *srcp++;                                               \
        b1 = *srcp++;                                               \
        b0 = *srcp++;                                               \
                                                                    \
        *dstp++ = ((b0     )     ) | ((b1 & 3 ) << 8);              \
        *dstp++ = ((b1 >> 2) & 63) | ((b2 & 15) << 6);              \
        *dstp++ = ((b2 >> 4) & 15) | ((b3 & 63) << 4);              \
    }                                                               \
}

TC_DECLARE_DECODE(16)
TC_DECLARE_DECODE(32)

static void RawTo16(privrec_t *me)
{
    tc_decode16(me->databuf[OTTCAMV_BIGC_RAW],
                me->databuf[OTTCAMV_BIGC_16BIT],
                (PIX_W / 3) * PIX_H);
}

static void RawTo32(privrec_t *me)
{
    tc_decode32(me->databuf[OTTCAMV_BIGC_RAW],
                me->databuf[OTTCAMV_BIGC_32BIT],
                (PIX_W / 3) * PIX_H);
}

static void ott_return_em(int devid, privrec_t *me)
{
  int        bigc;
  int        line_n;
  rflags_t   flags[OTTCAMV_NUMCHANS];
  int        n;
    
    me->req_sent = 0;
    me->state    = STATE_READY;
    sq_clear(&(me->q));
    if (me->act_tid >= 0)
    {
        sl_deq_tout(me->act_tid);
        me->act_tid = -1;
    }

    me->cur_args[OTTCAMV_PARAM_MISS] = me->rest;
    me->cur_args[OTTCAMV_PARAM_W]    = PIX_W;
    me->cur_args[OTTCAMV_PARAM_H]    = PIX_H;

    flags[0] = me->rest == 0? 0 : CXRF_IO_TIMEOUT;
    for (n = 1;  n < countof(flags);  n++)
        flags[n] = flags[0];
    
    /* Wipe-out data in absent lines */
    if (me->rest != 0)
        for (line_n = 0;  line_n < PIX_H;  line_n++)
            if (!me->ispresent[line_n])
                bzero(ACCESS_RAW_LINE_N(me, line_n), PAYLOAD_SIZE);

    for (bigc = 0;  bigc < OTTCAMV_NUM_BIGCS;  bigc++)
        if (me->requested[bigc])
        {
            me->requested[bigc] = 0;

            if      (bigc == OTTCAMV_BIGC_16BIT)
                RawTo16(me);
            else if (bigc == OTTCAMV_BIGC_32BIT)
                RawTo32(me);
            
            ReturnBigc(devid, bigc, me->cur_args, OTTCAMV_NUM_PARAMS,
                       me->databuf[bigc], bigcprops[bigc].datasize, bigcprops[bigc].dataunits,
                       flags[0]);
        }

    ReturnChanGroup(devid, 0, OTTCAMV_NUMCHANS, me->cur_args, flags);
}


static void act_timeout(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr);

static void RequestMissingLines(privrec_t *me)
{
  uint8 *b_p;
  uint8 *e_p;

    b_p = memchr(me->ispresent, 0, PIX_H);
    e_p = memchr(b_p,           1, PIX_H - (b_p - me->ispresent));
    if (e_p == NULL) e_p = me->ispresent + PIX_H - 1;

    sq_clear(&(me->q));
    enq_pkt(me, OTTCMD_SND_FRAME, b_p - me->ispresent, e_p - me->ispresent);
    fprintf(stderr, "%s [%d] %s(): %d,%d\n", strcurtime(), me->devid, __FUNCTION__, (int)(b_p - me->ispresent), (int)(e_p - me->ispresent));

    me->state     = STATE_REGET;
    me->rrq_rest  = e_p - b_p + 1;
    me->smth_rcvd = 0;

    if (me->act_tid >= 0)
        sl_deq_tout(me->act_tid);
    me->act_tid = sl_enq_tout_after(me->devid, me, me->cur_args[OTTCAMV_PARAM_WAITTIME_ACKD]*1000, act_timeout, NULL);
}


static void act_timeout(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr)
{
  privrec_t *me = (privrec_t *) devptr;

    me->act_tid = -1;

////fprintf(stderr, "%s(state=%d)\n", __FUNCTION__, me->state);
    if (me->state == STATE_READY)
    {
        /*!!!?*/
        return;
    }

    /* Okay, have we received something at all? */
    if (/*me->state == STATE_REGET  &&*/  me->smth_rcvd)
    {
        RequestMissingLines(me);
        return;
    }

    ott_return_em(devid, me);
}

static void ott_request_new_frame(int devid, privrec_t *me)
{
  int       r;

    ////if (devid == 1) fprintf(stderr, "%s [%d] %s()\n", strcurtime(), devid, __FUNCTION__);

    memcpy(me->cur_args, me->nxt_args, sizeof(me->cur_args));
    me->curframe++;

    sq_clear(&(me->q));

    /////////////////!!!!!!!!!!!!!!!!! request
    enq_pkt(me, OTTCMD_SETMODE,   OTTMODE_SINGLE_SHOT, 0);
    enq_pkt(me, OTTCMD_WRITEREG,  0x02,  0 + 4);
    enq_pkt(me, OTTCMD_WRITEREG,  0x01,  0 + 1);
    enq_pkt(me, OTTCMD_WRITEREG,  0x07,  0x8 |
                                         ((me->cur_args[OTTCAMV_PARAM_SYNC]) << 4) |
                                         (0xC << 5));
    enq_pkt(me, OTTCMD_WRITEREG,  0x0B,  me->cur_args[OTTCAMV_PARAM_T]);
    enq_pkt(me, OTTCMD_WRITEREG,  0x0D,  0);
    enq_pkt(me, OTTCMD_WRITEREG,  0x35,  me->cur_args[OTTCAMV_PARAM_K]);
    enq_pkt(me, OTTCMD_WRITEREG,  0xAF,  0);
    enq_pkt(me, OTTCMD_WRITEREG,  0x47,  0x808);
    enq_pkt(me, OTTCMD_WRITEREG,  0x48,  0);
    enq_pkt(me, OTTCMD_WRITEREG,  0x70,  0x14);
    enq_pkt(me, OTTCMD_SET_FSIZE, PIX_W, PIX_H);
    enq_pkt(me, OTTCMD_NEW_FRAME, 0,     0);

    me->req_sent = 1;
    me->rest     = PIX_H;
    bzero(me->ispresent, sizeof(me->ispresent));
    me->state = STATE_INITIAL;
    me->smth_rcvd = 0;

    me->act_tid = sl_enq_tout_after(devid, me, me->cur_args[OTTCAMV_PARAM_WAITTIME]*1000, act_timeout, NULL);
    
    return;
}

//////////////////////////////////////////////////////////////////////

static void ottcamv_fd_p(int devid, void *devptr,
                         sl_fdh_t fdhandle, int fd, int mask,
                         void *privptr);

static int ottcamv_init_d(int devid, void *devptr, 
                          int businfocount, int businfo[],
                          const char *auxinfo)
{
  privrec_t          *me = (privrec_t *) devptr;

  unsigned long       host_ip;
  struct hostent     *hp;
  struct sockaddr_in  cam_addr;
  int                 fd;
  size_t              bsize;                     // Parameter for setsockopt
  int                 r;

    me->devid   = devid;
    me->act_tid = -1;

    me->nxt_args[OTTCAMV_PARAM_K]             = 16;
    me->nxt_args[OTTCAMV_PARAM_T]             = 30*100;
    me->nxt_args[OTTCAMV_PARAM_SYNC]          = 0;
    me->nxt_args[OTTCAMV_PARAM_WAITTIME]      = 10*1000;
    me->nxt_args[OTTCAMV_PARAM_WAITTIME_ACKD] =     100;
  
    if (auxinfo == NULL  ||  *auxinfo == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s: either CCD-camera hostname/IP, or '-' should be specified in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

    if (auxinfo[0] == '-')
    {
        me->is_sim = 1;
        
        me->sim_x  = 300;
        me->sim_y  = 200;
        me->sim_vx = SIMSQ_W/2;
        me->sim_vy = SIMSQ_H/2;

        me->act_tid = sl_enq_tout_after(devid, devptr, SIM_PERIOD, sim_heartbeat, NULL);

        return DEVSTATE_OPERATING;
    }
    else
    {
        /* Find out IP of the host */
        
        /* First, is it in dot notation (aaa.bbb.ccc.ddd)? */
        host_ip = inet_addr(auxinfo);
        
        /* No, should do a hostname lookup */
        if (host_ip == INADDR_NONE)
        {
            hp = gethostbyname(auxinfo);
            /* No such host?! */
            if (hp == NULL)
            {
                DoDriverLog(devid, 0, "%s: unable to resolve name \"%s\"",
                            __FUNCTION__, auxinfo);
                return -CXRF_CFG_PROBL;
            }
            
            memcpy(&host_ip, hp->h_addr, hp->h_length);
        }
        
        /* Record camera's ip:port */
        cam_addr.sin_family = AF_INET;
        cam_addr.sin_port   = htons(DEFAULT_OTTCAM_PORT);
        memcpy(&(cam_addr.sin_addr), &host_ip, sizeof(host_ip));

        /* Create a UDP socket */
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: socket()", __FUNCTION__);
            return -CXRF_DRV_PROBL;
        }
        me->fd = fd;
        /* Turn it into nonblocking mode */
        set_fd_flags(fd, O_NONBLOCK, 1);
        /* And set RCV buffer size */
        bsize = 1500 * PIX_H;
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &bsize, sizeof(bsize));
        /* Specify the other endpoint */
        r = connect(fd, &cam_addr, sizeof(cam_addr));
        if (r != 0)
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: connect()", __FUNCTION__);
        
        if ((me->databuf[OTTCAMV_BIGC_RAW] = malloc(bigcprops[OTTCAMV_BIGC_RAW].datasize)) == NULL)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: malloc(%zu) for bigc=%d failed",
                        __FUNCTION__, bigcprops[OTTCAMV_BIGC_RAW].datasize, OTTCAMV_BIGC_RAW);
            return -1;
        }

        /*  */
        r = sq_init(&(me->q), NULL,
                    OTTCAMV_SENDQ_SIZE, sizeof(ottqelem_t),
                    OTTCAMV_SENDQ_TOUT, me,
                    ottcamv_sender,
                    ottcamv_eq_cmp_func,
                    tim_enqueuer,
                    tim_dequeuer);
        if (r < 0)
        {
            DoDriverLog(me->devid, 0 | DRIVERLOG_ERRNO,
                        "%s: sq_init()", __FUNCTION__);
            return -CXRF_DRV_PROBL;
        }
        me->sq_tid = -1;

        sl_add_fd(me->devid, devptr, fd, SL_RD, ottcamv_fd_p, NULL);

        return DEVSTATE_OPERATING;
    }
}

static void ottcamv_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t *me = (privrec_t *) devptr;
  int        bigc;
  
    for (bigc = 0;  bigc < OTTCAMV_NUM_BIGCS;  bigc++)
        safe_free(me->databuf[bigc]);

    sq_fini(&(me->q));
}

static void ottcamv_fd_p(int devid, void *devptr,
                         sl_fdh_t fdhandle, int fd, int mask,
                         void *privptr)
{
  privrec_t    *me = (privrec_t *) devptr;
  uint8         packet[2000];
  ottcam_hdr_t  hdr;
  int           frame_n;
  int           line_n;
  int           r;
  int           repcount;
  int           usecs = 0;

////fprintf(stderr, "%s(privptr=%p)\n", __FUNCTION__, privptr);
    for (repcount = me->rest + 100;  repcount > 0;  repcount--)
    {
        errno = 0;
        r = recv(fd, packet, sizeof(packet), 0);
        if (r <= 0)
        {
            if (ERRNO_WOULDBLOCK())
            {
                /* Okay, have just emptied the buffer */
                /* Sure?  Have we read at least one packet? */
                if (repcount == 0)
                {
                    DoDriverLog(devid, DRIVERLOG_ERR,
                                "%s() called w/o data!", __FUNCTION__);
                    SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "fd_p() no data");
                    return;
                }
                goto NORMAL_EXIT;
            }
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: recv()=%d", __FUNCTION__, r);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "recv() error");
            return;
        }

        if (me->req_sent == 0) continue;

        /* Should we also check that the packet is of right size? */

        memcpy(&hdr, packet, sizeof(hdr));
        hdr.id   = ntohl(hdr.id);
        hdr.seq  = ntohs(hdr.seq);
        hdr.cmd  = ntohs(hdr.cmd);
        hdr.par1 = ntohs(hdr.par1);
        hdr.par2 = ntohs(hdr.par2);

        ////fprintf(stderr, "hdr.id=%08x seq=%d cmd=%d par1=%d par2=%d\n", hdr.id, hdr.seq, hdr.cmd, hdr.par1, hdr.par2);
        switch (hdr.id)
        {
            case OTTRPY_EXPO:
                enq_pkt(me, OTTCMD_SND_FRAME, 0, PIX_H - 1);
                break;
            
            case OTTRPY_NEXP:
                fprintf(stderr, "%s [%d] NEXP(seq=%d,cur=%d): cmd=%d par1=%d par2=%d\n",
                        strcurtime(), devid, hdr.seq, me->curframe, hdr.cmd, hdr.par1, hdr.par2);
                ott_return_em(devid, me);
                break;
            
            case OTTRPY_ACKN:
                esn_pkt(me, hdr.cmd, hdr.par1, hdr.par2);
                break;
            
            case OTTRPY_NACK:
                DoDriverLog(devid, 0, "NACK(seq=%d,cur=%d): cmd=%d par1=%d par2=%d",
                            hdr.seq, me->curframe, hdr.cmd, hdr.par1, hdr.par2);
                esn_pkt(me, hdr.cmd, hdr.par1, hdr.par2);
                break;
            
            case OTTRPY_DATA:
                frame_n = packet[5] + ((unsigned int)packet[4]) * 256;
                line_n  = packet[7] + ((unsigned int)packet[6]) * 256;
                ////fprintf(stderr, "DATA frame=%d line=%d rest=%d\n", frame_n, line_n, me->rest);

                /* Check frame # */
                if (frame_n != me->curframe)
                    goto CONTINUE_FOR;
                
                /* Check line # */
                if (line_n >= PIX_H)
                {
                    DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO, "line=%d >= h=%d", line_n, PIX_H);
                    goto CONTINUE_FOR;
                }
                
                if (!me->ispresent[line_n])
                {
                    if (me->smth_rcvd == 0  &&  me->state != STATE_REGET)
                        usecs = me->cur_args[OTTCAMV_PARAM_WAITTIME_ACKD]*1000;
                    me->smth_rcvd = 1;

                    memcpy(ACCESS_RAW_LINE_N(me, line_n),
                           packet + 8,
                           PAYLOAD_SIZE);
                    
                    me->ispresent[line_n] = 1;
                    me->rest--;
                    if (me->rest == 0)
                    {
                        ////if (devid == 1) fprintf(stderr, "%s [%d] rest=0\n", strcurtime(), devid);
                        ott_return_em(devid, me);
                        return;
                    }

                    if (me->state == STATE_REGET)
                    {
                        me->rrq_rest--;
                        if (me->rrq_rest == 0)
                        {
                            RequestMissingLines(me);
                            return;
                        }
                    }
                }
                break;
            
            default:
                ;
        }
        
 CONTINUE_FOR:;
    }

 NORMAL_EXIT:;
    if (usecs > 0)
    {
        if (me->act_tid >= 0)
            sl_deq_tout(me->act_tid);
        me->act_tid = sl_enq_tout_after(devid, devptr, usecs, act_timeout, NULL);
    }
}

static void SanitizeArgs(privrec_t *me)
{
    if (me->nxt_args[OTTCAMV_PARAM_K] < 16)    me->nxt_args[OTTCAMV_PARAM_K] = 16;
    if (me->nxt_args[OTTCAMV_PARAM_K] > 64)    me->nxt_args[OTTCAMV_PARAM_K] = 64;
    if (me->nxt_args[OTTCAMV_PARAM_T] < 0)     me->nxt_args[OTTCAMV_PARAM_T] = 0;
    if (me->nxt_args[OTTCAMV_PARAM_T] > 65535) me->nxt_args[OTTCAMV_PARAM_T] = 65535;
    me->nxt_args[OTTCAMV_PARAM_SYNC] = (me->nxt_args[OTTCAMV_PARAM_SYNC] != 0);
    if (me->nxt_args[OTTCAMV_PARAM_WAITTIME]      < 10) me->nxt_args[OTTCAMV_PARAM_WAITTIME]      = 10;
    if (me->nxt_args[OTTCAMV_PARAM_WAITTIME_ACKD] < 10) me->nxt_args[OTTCAMV_PARAM_WAITTIME_ACKD] = 10;
    me->nxt_args[OTTCAMV_PARAM_STOP] = 0;
}

static void ottcamv_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t *me = (privrec_t *) devptr;
  int        n;
  int        x;
  rflags_t   zero_rflags = 0;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE  &&  x < OTTCAMV_NUM_PARAMS)
        {
            me->nxt_args[x] = values[n];
            if (x == OTTCAMV_PARAM_SYNC) fprintf(stderr, "[%d]SYNC:=%d\n", devid, me->nxt_args[x]);
            SanitizeArgs(me);
            if (x == OTTCAMV_PARAM_STOP  &&  values[n] == CX_VALUE_COMMAND  &&
                !me->is_sim  &&  me->req_sent)
                ott_return_em(devid, me);
        }
        if (x < OTTCAMV_NUM_PARAMS)
            ReturnChanGroup(devid, x, 1, me->nxt_args + x, &zero_rflags);
    }
}

static void ottcamv_bigc_p(int devid, void *devptr, int bigc, int32 *args, int nargs, void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  privrec_t *me = (privrec_t *) devptr;
  int        n;

  ////fprintf(stderr, "%s [%d] %s(), sim=%d, sent=%d\n", strcurtime(), devid, __FUNCTION__, me->is_sim, me->req_sent);

    if (me->databuf[bigc] == NULL)
        if ((me->databuf[bigc] = malloc(bigcprops[bigc].datasize)) == NULL)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: malloc(%zu) for bigc=%d failed",
                        __FUNCTION__, bigcprops[bigc].datasize, bigc);
            SetDevState(devid, DEVSTATE_OFFLINE, 0, "malloc() fail");
            return;
        }
    me->requested[bigc] = 1;
    
    for (n = 0;  n <= OTTCAMV_NUM_PARAMS  &&  n < nargs;  n++)
        me->nxt_args[n] = args[n]/*, DoDriverLog(devid, 0, "param[%d]:=%d", n, me->nxt_args[n])*/;

    SanitizeArgs(me);
    
    if (!me->is_sim  &&  !me->req_sent)
        ott_request_new_frame(devid, me);
}

DEFINE_DRIVER(ottcamv,  "Ottmar's Camera, V-driver",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              0, 0+1,
              NULL, 0,
              -1,  NULL,
              -1, NULL,
              ottcamv_init_d, ottcamv_term_d,
              ottcamv_rw_p, ottcamv_bigc_p);
