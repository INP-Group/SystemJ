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

#include "cxsd_driver.h"

#include "drv_i/tsycamv_drv_i.h"


enum {PIX_W = 660, PIX_H = 500};
enum {PAYLOAD_SIZE = (PIX_W * 5 / 4)};
enum {DEFAULT_TSYCAM_PORT = 4001};
  

enum
{
    SIMSQ_W = 60,
    SIMSQ_H = 50,
    SIM_PERIOD = 200*1000 // 20ms
};

typedef struct
{
    int         is_sim;
    int         req_sent;
    
    int         devid;
    int         fd;

    sl_tid_t    act_tid;
    
    int         requested[TSYCAMV_NUM_BIGCS];
    void       *databuf  [TSYCAMV_NUM_BIGCS];
    
    int32       cur_args [TSYCAMV_NUM_PARAMS];
    int32       nxt_args [TSYCAMV_NUM_PARAMS];

    uint8       curframe;
    int         rest;
    uint8       ispresent[PIX_H];

    int         sim_x;
    int         sim_y;
    int         sim_vx;
    int         sim_vy;
} privrec_t;

#define ACCESS_RAW_LINE_N(me, n) \
    ((uint8 *)(me->databuf[TSYCAMV_BIGC_RAW]) + PAYLOAD_SIZE * n)

static struct
{
    size_t  datasize;
    size_t  dataunits;
}
bigcprops[TSYCAMV_NUM_BIGCS] =
{
    [TSYCAMV_BIGC_RAW]   = {PIX_W * PIX_H * 10 / 8,         sizeof(uint8)},
    [TSYCAMV_BIGC_16BIT] = {PIX_W * PIX_H * sizeof(uint16), sizeof(uint16)},
    [TSYCAMV_BIGC_32BIT] = {PIX_W * PIX_H * sizeof(uint32), sizeof(uint32)},
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

    memcpy(me->cur_args, me->nxt_args, sizeof(me->cur_args));
  
    me->cur_args[TSYCAMV_PARAM_MISS] = 0;
    me->cur_args[TSYCAMV_PARAM_W]    = PIX_W;
    me->cur_args[TSYCAMV_PARAM_H]    = PIX_H;

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
    
    for (bigc = 0;  bigc < TSYCAMV_NUM_BIGCS;  bigc++)
        if (me->requested[bigc])
        {
            me->requested[bigc] = 0;
            bzero(me->databuf[bigc], bigcprops[bigc].datasize);
            
            if (bigc == TSYCAMV_BIGC_16BIT)
            {
                buf16 = me->databuf[bigc];

                for (y = 0;  y < SIMSQ_H;  y++)
                    for (x = 0, p16 = buf16 + (y + me->sim_y) * PIX_W + me->sim_x;
                         x < SIMSQ_W;
                         x++,   p16++)
                        *p16 = (me->cur_args[TSYCAMV_PARAM_T] + me->cur_args[TSYCAMV_PARAM_K] + x*4 + y*4) & 1023;
            }

            ReturnBigc(devid, bigc, me->cur_args, TSYCAMV_NUM_PARAMS,
                       me->databuf[bigc], bigcprops[bigc].datasize, bigcprops[bigc].dataunits,
                       0);
        }

    me->act_tid = sl_enq_tout_after(devid, devptr, SIM_PERIOD, sim_heartbeat, NULL);
}

//////////////////////////////////////////////////////////////////////

enum
{
    TIMEOUT_INITIAL = 1000*1000,
    TIMEOUT_REGET   =  100*1000,
};

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

enum {FLAG_SMDE = 1, FLAG_SYNC = 2, FLAG_PS = 4, FLAG_FSE = 8};
enum {SMD_SMD1 = 2, SMD_SMD2 = 1};

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

#define TC_DECLARE_DECODE(nn)                                   \
static void tc_decode##nn(void *src, void *dst, int n_quintets) \
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

TC_DECLARE_DECODE(16)
TC_DECLARE_DECODE(32)

static void RawTo16(privrec_t *me)
{
    tc_decode16(me->databuf[TSYCAMV_BIGC_RAW],
                me->databuf[TSYCAMV_BIGC_16BIT],
                (PIX_W / 4) * PIX_H);
}

static void RawTo32(privrec_t *me)
{
    tc_decode32(me->databuf[TSYCAMV_BIGC_RAW],
                me->databuf[TSYCAMV_BIGC_32BIT],
                (PIX_W / 4) * PIX_H);
}

static void tsy_return_em(int devid, privrec_t *me)
{
  int        bigc;
  int        line_n;
  rflags_t   flags[TSYCAMV_NUMCHANS];
  int        n;
    
    me->req_sent = 0;

    me->cur_args[TSYCAMV_PARAM_MISS] = me->rest;
    me->cur_args[TSYCAMV_PARAM_W]    = PIX_W;
    me->cur_args[TSYCAMV_PARAM_H]    = PIX_H;

    flags[0] = me->rest == 0? 0 : CXRF_IO_TIMEOUT;
    for (n = 1;  n < countof(flags);  n++)
        flags[n] = flags[0];
    
    /* Wipe-out data in absent lines */
    if (me->rest != 0)
        for (line_n = 0;  line_n < PIX_H;  line_n++)
            if (!me->ispresent[line_n])
                bzero(ACCESS_RAW_LINE_N(me, line_n), PAYLOAD_SIZE);

    for (bigc = 0;  bigc < TSYCAMV_NUM_BIGCS;  bigc++)
        if (me->requested[bigc])
        {
            me->requested[bigc] = 0;

            if      (bigc == TSYCAMV_BIGC_16BIT)
                RawTo16(me);
            else if (bigc == TSYCAMV_BIGC_32BIT)
                RawTo32(me);
            
            ReturnBigc(devid, bigc, me->cur_args, TSYCAMV_NUM_PARAMS,
                       me->databuf[bigc], bigcprops[bigc].datasize, bigcprops[bigc].dataunits,
                       flags[0]);
        }

    ReturnChanGroup(devid, 0, TSYCAMV_NUMCHANS, me->cur_args, flags);
}

static void act_timeout(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr)
{
  privrec_t *me = (privrec_t *) devptr;
  
    tsy_return_em(devid, me);
}

static void tsy_request_new_frame(int devid, privrec_t *me)
{
  reqhdr_t  hdr;
  int       r;
  
    memcpy(me->cur_args, me->nxt_args, sizeof(me->cur_args));
    me->curframe++;

    FillReqHdr(&hdr,
               FLAG_SMDE*1 | FLAG_SYNC*me->cur_args[TSYCAMV_PARAM_SYNC],
               me->cur_args[TSYCAMV_PARAM_K], me->cur_args[TSYCAMV_PARAM_T],
               0, PIX_H - 1,
               SMD_SMD1*0 | SMD_SMD2*1,
               me->curframe);
    
    r = send(me->fd, &hdr, sizeof(hdr), 0);
    
    if (r != sizeof(hdr))
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: send()=%d", __FUNCTION__, r);
        SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "send() error");
        return;
    }

    me->req_sent = 1;
    me->rest     = PIX_H;
    bzero(me->ispresent, sizeof(me->ispresent));

    me->act_tid = sl_enq_tout_after(devid, me, me->cur_args[TSYCAMV_PARAM_WAITTIME]*1000, act_timeout, NULL);
    
    return;
}


//////////////////////////////////////////////////////////////////////

static void tsycamv_fd_p(int devid, void *devptr,
                         sl_fdh_t fdhandle, int fd, int mask,
                         void *privptr);

static int tsycamv_init_d(int devid, void *devptr, 
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

    me->nxt_args[TSYCAMV_PARAM_K]             = 128;
    me->nxt_args[TSYCAMV_PARAM_T]             = 200;
    me->nxt_args[TSYCAMV_PARAM_SYNC]          = 0;
    me->nxt_args[TSYCAMV_PARAM_WAITTIME]      =  1*1000;
    me->nxt_args[TSYCAMV_PARAM_WAITTIME_ACKD] = 10*1000;
  
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
        cam_addr.sin_port   = htons(DEFAULT_TSYCAM_PORT);
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
        
        if ((me->databuf[TSYCAMV_BIGC_RAW] = malloc(bigcprops[TSYCAMV_BIGC_RAW].datasize)) == NULL)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: malloc(%zu) for bigc=%d failed",
                        __FUNCTION__, bigcprops[TSYCAMV_BIGC_RAW].datasize, TSYCAMV_BIGC_RAW);
            return -1;
        }

        sl_add_fd(me->devid, devptr, fd, SL_RD, tsycamv_fd_p, NULL);

        return DEVSTATE_OPERATING;
    }
}

static void tsycamv_term_d(int devid __attribute__((unused)), void *devptr)
{
  privrec_t *me = (privrec_t *) devptr;
  int        bigc;
  
    for (bigc = 0;  bigc < TSYCAMV_NUM_BIGCS;  bigc++)
        safe_free(me->databuf[bigc]);
}

static void tsycamv_fd_p(int devid, void *devptr,
                         sl_fdh_t fdhandle, int fd, int mask,
                         void *privptr)
{
  privrec_t *me = (privrec_t *) devptr;
  uint8      packet[2000];
  int        frame_n;
  int        line_n;
  int        r;
  int        repcount;

    for (repcount = me->rest;  repcount > 0;  repcount--)
    {
        errno = 0;
        r = recv(fd, packet, sizeof(packet), 0);
        if (r <= 0)
        {
            if (ERRNO_WOULDBLOCK()) return;
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: recv()=%d", __FUNCTION__, r);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "recv() error");
            return;
        }

        /* Should we also check that the packet is of right size? */
        
        /* Okay, we've reached a point where we r sure that it is a real packet ;-) */
        frame_n = packet[0] + ((unsigned int)packet[1]) * 256;
        line_n  = packet[2] + ((unsigned int)packet[3]) * 256 - 1;

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
            memcpy(ACCESS_RAW_LINE_N(me, line_n),
                   packet + 4,
                   PAYLOAD_SIZE);
            
            me->ispresent[line_n] = 1;
            me->rest--;
            if (me->rest == 0)
            {
                sl_deq_tout(me->act_tid);
                me->act_tid = -1;
                tsy_return_em(devid, me);
                return;
            }
        }
        
 CONTINUE_FOR:;
    }
}

static void SanitizeArgs(privrec_t *me)
{
    if (me->nxt_args[TSYCAMV_PARAM_K] < 0)   me->nxt_args[TSYCAMV_PARAM_K] = 0;
    if (me->nxt_args[TSYCAMV_PARAM_K] > 255) me->nxt_args[TSYCAMV_PARAM_K] = 255;
    if (me->nxt_args[TSYCAMV_PARAM_T] < 0)   me->nxt_args[TSYCAMV_PARAM_T] = 0;
    if (me->nxt_args[TSYCAMV_PARAM_T] > 511) me->nxt_args[TSYCAMV_PARAM_T] = 511;
    me->nxt_args[TSYCAMV_PARAM_SYNC] = (me->nxt_args[TSYCAMV_PARAM_SYNC] != 0);
    if (me->nxt_args[TSYCAMV_PARAM_WAITTIME]      < 10) me->nxt_args[TSYCAMV_PARAM_WAITTIME]      = 10;
    if (me->nxt_args[TSYCAMV_PARAM_WAITTIME_ACKD] < 10) me->nxt_args[TSYCAMV_PARAM_WAITTIME_ACKD] = 10;
    me->nxt_args[TSYCAMV_PARAM_STOP] = 0;
}

static void tsycamv_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t *me = (privrec_t *) devptr;
  int        n;
  int        x;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE  &&  x < TSYCAMV_NUM_PARAMS)
        {
            me->nxt_args[x] = values[n];
            SanitizeArgs(me);
            if (x == TSYCAMV_PARAM_STOP  &&  values[n] == CX_VALUE_COMMAND  &&
                !me->is_sim  &&  me->req_sent)
                tsy_return_em(devid, me);
        }
    }
}

static void tsycamv_bigc_p(int devid, void *devptr, int bigc, int32 *args, int nargs, void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  privrec_t *me = (privrec_t *) devptr;
  int        n;

    if (me->databuf[bigc] == NULL)
        if ((me->databuf[bigc] = malloc(bigcprops[bigc].datasize)) == NULL)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: malloc(%zu) for bigc=%d failed",
                        __FUNCTION__, bigcprops[bigc].datasize, bigc);
            SetDevState(devid, DEVSTATE_OFFLINE, 0, "malloc() fail");
            return;
        }
    me->requested[bigc] = 1;
    
    for (n = 0;  n <= TSYCAMV_PARAM_SYNC  &&  n < nargs;  n++)
        me->nxt_args[n] = args[n]/*, DoDriverLog(devid, 0, "param[%d]:=%d", n, me->nxt_args[n])*/;

    SanitizeArgs(me);
    
    if (!me->is_sim  &&  !me->req_sent)
        tsy_request_new_frame(devid, me);
}

DEFINE_DRIVER(tsycamv, "Tsyganov's Camera, V-driver",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              0, 0,
              NULL, 0,
              -1,  NULL,
              -1, NULL,
              tsycamv_init_d, tsycamv_term_d,
              tsycamv_rw_p, tsycamv_bigc_p);
