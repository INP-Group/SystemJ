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

#include "cxsd_driver.h"

#include "misc_macros.h"
#include "misclib.h"
#include "sendqlib.h"

#include "drv_i/slbpm_drv_i.h"


enum
{
    SLBCMD_WRREG = 0x00,
    SLBCMD_RDOSC = 0x01,
    SLBCMD_START = 0x02,
    SLBCMD_RDREG = 0x04,
    SLBCMD_RDMMX = 0x06,  // ReaD Min/MaX
#if 0
    SLBCMD_RDZER = 0x07,  // Read ZERo
#endif
    SLBCMD_WR_RD = 0x08,  // WRite and ReaD reg
    SLBCMD_DOCLB = 0x09,
    SLBCMD_SRBUF = 0x0A,
    SLBCMD_IRBUF = 0x0B,
    SLBCMD_SRSUM = 0x0C,
    SLBCMD_IRSUM = 0x0D,
    SLBCMD_AUTOX = 0x0E,
    SLBCMD_AUTOB = 0x0F,
};

enum
{
    SLBREG_USRVAL = 0,
    SLBREG_DELAYA = 1,
    SLBREG_DELAY0 = 2,
    SLBREG_DELAY1 = 3,
    SLBREG_DELAY2 = 4,
    SLBREG_DELAY3 = 5,
    SLBREG_MODE   = 6,
    SLBREG_NAV    = 7,
    SLBREG_BORDER = 8,
};

enum
{
    SLBMODE_AMPL      = 1 << 0,
    SLBMODE_ELECTRONS = 1 << 1,
    SLBMODE_AUTO_MMX  = 1 << 2,
    SLBMODE_DIS_EXT   = 1 << 3,
    SLBMODE_INT_START = 1 << 4,
    SLBMODE_AUTO_BUF  = 1 << 5,
};

enum {MAX_NAV = 63};

enum
{
//    BPMPING_CMD = OTTCMD_READREG,
//    BPMPING_REG = 0x7F,
//    BPMPING_VAL = 0x2AA,          // == 0b0000001010101010
    
    SLBPM_PING_REG = SLBREG_USRVAL,
    SLBPM_PING_VAL = 54321,

    ALIVE_USECS = 10*1000000,     // 10s keepalive ping period
};

typedef struct
{
    uint8   id_hi;
    uint8   id_lo;
    uint8   cmd;
    uint8   addr;
    uint8   data_hi;
    uint8   data_lo;
} slbpm_hdr_t;

static inline void FillHdr(slbpm_hdr_t *hdr, uint8 cmd, uint8 addr, uint16 data)
{
    hdr->id_hi   = 0x24;
    hdr->id_lo   = 0x13;
    hdr->cmd     = cmd;
    hdr->addr    = addr;
    hdr->data_hi = data >> 8;
    hdr->data_lo = data & 0xFF;
}

enum {DEFAULT_SLBPM_PORT = 2195};

//////////////////////////////////////////////////////////////////////

enum
{
    SLBPM_SENDQ_SIZE = 100,
    SLBPM_SENDQ_TOUT = 100000, // 100ms
};
typedef struct
{
    sq_eprops_t       props;
    slbpm_hdr_t       hdr;
} bpmqelem_t;

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int               devid;
    int               fd;

    int32             amplon;
    int32             eminus;

    ////
    sq_q_t            q;
    sl_tid_t          sq_tid;

    ////
    sl_tid_t          alv_tid;

} slbpm_privrec_t;

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

//////////////////////////////////////////////////////////////////////

static int  slbpm_sender     (void *elem, void *devptr)
{
  slbpm_privrec_t  *me = devptr;
  bpmqelem_t       *qe = elem;
  int               r;

    ////DoDriverLog(me->devid, 0, "%s(), %d/%d, %d", __FUNCTION__, qe->props.tries_done, qe->props.tries, qe->props.timeout_us);
    r = send(me->fd, &(qe->hdr), sizeof(qe->hdr), 0);
    ////fprintf(stderr, "send=%d\n", r);
    return r > 0? 0 : -1;
}

static int  slbpm_eq_cmp_func(void *elem, void *ptr)
{
  slbpm_hdr_t *a = &(((bpmqelem_t *)elem)->hdr);
  slbpm_hdr_t *b = &(((bpmqelem_t *)ptr )->hdr);

    if (a->cmd != b->cmd) return 0;
    /*!!!*/return 1;
    if (a->cmd == SLBCMD_WRREG  ||
        a->cmd == SLBCMD_RDREG  ||
#if 0
        a->cmd == SLBCMD_RDZER  ||
#endif
        a->cmd == SLBCMD_WR_RD)
        return a->addr == b->addr;

    return 0; /*!!!??? Why not 1?*/
}

static void tout_proc   (int devid, void *devptr,
                         sl_tid_t tid,
                         void *privptr)
{
  slbpm_privrec_t *me = devptr;
  
    me->sq_tid = -1;
    sq_timeout(&(me->q));
}

static int  tim_enqueuer(void *devptr, int usecs)
{
  slbpm_privrec_t *me = devptr;

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
  slbpm_privrec_t  *me = devptr;
  
    if (me->sq_tid >= 0)
    {
        sl_deq_tout(me->sq_tid);
        me->sq_tid = -1;
    }
}

static sq_status_t do_enqx(slbpm_privrec_t *me,
                           sq_enqcond_t     how,
                           int              tries,
                           int              timeout_us,
                           int8             cmd,
                           int8             addr,
                           uint16           data)
{
  bpmqelem_t  item;
  sq_status_t r;

    bzero(&item, sizeof(item));
    
    item.props.tries      = tries;
    item.props.timeout_us = timeout_us;

    FillHdr(&(item.hdr), cmd, addr, data);

    r = sq_enq(&(me->q), &(item.props), how, NULL);
////fprintf(stderr, "%s()=%d %d %d\n", __FUNCTION__, r, me->q.ring_used, me->q.tout_set);
    return r;
}

static sq_status_t enq_pkt(slbpm_privrec_t *me, 
                           int8             cmd,
                           int8             addr,
                           uint16           data)
{
    return do_enqx(me, SQ_ALWAYS,    SQ_TRIES_INF, SLBPM_SENDQ_TOUT,
                   cmd, addr, data);
}

static sq_status_t enq_ons(slbpm_privrec_t *me,
                           int8             cmd,
                           int8             addr,
                           uint16           data)
{
    ////DoDriverLog(me->devid, 0, "%s()", __FUNCTION__);
    return do_enqx(me, SQ_IF_ABSENT, SQ_TRIES_ONS, 0,
                   cmd, addr, data);
}

static sq_status_t esn_pkt(slbpm_privrec_t *me,
                           int8             cmd,
                           int8             addr)
{
  bpmqelem_t  item;
  void       *fe;

    bzero(&item, sizeof(item));
    
    item.hdr.cmd     = cmd;
    item.hdr.addr    = addr;

    /* Check "whether we should really erase and send next packet" */
    fe = sq_access_e(&(me->q), 0);
    if (fe != NULL  &&  slbpm_eq_cmp_func(fe, &item) == 0)
        return SQ_NOTFOUND;

    /* Erase... */
    sq_foreach(&(me->q), SQ_ELEM_IS_EQUAL, &item, SQ_ERASE_FIRST,  NULL);
    /* ...and send next */
    sq_sendnext(&(me->q));

    return SQ_FOUND;
}

//////////////////////////////////////////////////////////////////////

static void SendInit(slbpm_privrec_t *me)
{
    enq_pkt(me, SLBCMD_WRREG, SLBREG_MODE,
            (me->eminus? SLBMODE_ELECTRONS : 0) |
            (me->amplon? SLBMODE_AMPL      : 0) |
            SLBMODE_AUTO_MMX*1 | SLBMODE_AUTO_BUF*1);

    enq_pkt(me, SLBCMD_WRREG, SLBPM_PING_REG, SLBPM_PING_VAL);
}

static void SendTerm(slbpm_privrec_t *me)
{
  slbpm_hdr_t       hdr;

    FillHdr(&hdr, SLBCMD_WRREG, SLBREG_MODE,   0 | SLBMODE_DIS_EXT);
    send(me->fd, &hdr, sizeof(hdr), 0);
    FillHdr(&hdr, SLBCMD_WRREG, SLBPM_PING_REG, 0);
    send(me->fd, &hdr, sizeof(hdr), 0);
}

//////////////////////////////////////////////////////////////////////

static void slbpm_fd_p(int devid, void *devptr,
                       sl_fdh_t fdhandle, int fd, int mask,
                       void *privptr)
{
  slbpm_privrec_t     *me = (slbpm_privrec_t *) devptr;
  uint8                data[2000];
  int                  r;
  int                  repcount;
  int                  was_good;

  int                  sock_err;
  socklen_t            errlen;

  uint8                cmd;
  uint8                addr;
  uint16               val16;
  int32                val32;

  int                  nl;
  int32                mmxs     [4];
  rflags_t             mmxs_flgs[4];

  int                  ofs;
  int32                param0;
  int32                bufs[4 * MAX_NAV]; /*!!! Note: bufs[] also used for oscillograms, but their size is smaller than of buffers */
  int                  x;

    for (repcount = 100, was_good = 0;
         repcount > 0;
         repcount--,     was_good = 1)
    {
        errno = 0;
        r = recv(fd, data, sizeof(data), 0);
        if (r <= 0)
        {
            if      (ERRNO_WOULDBLOCK())
            {
                /* Okay, have just emptied the buffer */
                /* Sure?  Have we read at least one packet? */
                if (was_good == 0)
                {
                    DoDriverLog(devid, DRIVERLOG_ERR,
                                "%s() called w/o data!", __FUNCTION__);
                    SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "fd_p() no data");
                    return;
                }
                goto NORMAL_EXIT;
            }
            else if (errno == ECONNREFUSED)
            {
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_err, &errlen);
                goto NORMAL_EXIT;
            }
            /*!!! No-no-no!!!  Should treat ECONNREFUSED somehow differently */
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: recv()=%d", __FUNCTION__, r);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "recv() error");
            return;
        }

        /* Check size */
        if (r < 4)
        {
            DoDriverLog(devid, DRIVERLOG_INFO | DRIVERLOG_C_PKTINFO,
                        "pktsize=%d, <2+2", r);
            goto CONTINUE_TO_NEXT_PACKET;
        }
        r -= 2;

        cmd  = data[0];
        addr = data[1];

        /* An acknowledgement or an error? */
        if (r == 2)
        {
            if      (data[1] == 0x11)
            {
                DoDriverLog(devid, DRIVERLOG_INFO | DRIVERLOG_C_PKTINFO,
                            "0x0001: wrong format");
            }
            else if (data[1] == 0x12)
            {
                DoDriverLog(devid, DRIVERLOG_INFO | DRIVERLOG_C_PKTINFO,
                            "0xCC02: wrong identification, cmd=0x%02x", cmd);
            }
            else if (data[1] == 0x14)
            {
                DoDriverLog(devid, DRIVERLOG_INFO | DRIVERLOG_C_PKTINFO,
                            "0xCC04: unknown command 0x%02x", cmd);
            }
            else
            {
                /* An acknowledgement */
                // What to erase?
                /*!!! Maybe use convention "addr==0xFF means ANY"? */
                esn_pkt(me, cmd, addr);
            }

            goto CONTINUE_TO_NEXT_PACKET;
        }

        /* A complete response packet */
        switch (cmd)
        {
            //case 0x00:
            case SLBCMD_RDMMX:
            case SLBCMD_AUTOX:
                if (r < 18)
                {
                    DoDriverLog(devid, DRIVERLOG_INFO | DRIVERLOG_C_PKTINFO,
                                "0x00/ALL: pktsize=%d, <18", r);
                    goto CONTINUE_TO_NEXT_PACKET;
                }

                if (cmd == SLBCMD_RDMMX) esn_pkt(me, cmd, 0);

                for (nl = 0;  nl < 4;  nl++)
                {
                    mmxs     [nl] =
                        (
                         ((uint32)(data[2 + nl * 4 + 0]) << 24) |
                         ((uint32)(data[2 + nl * 4 + 1]) << 16) |
                         ((uint32)(data[2 + nl * 4 + 2]) <<  8) |
                         ((uint32)(data[2 + nl * 4 + 3])      )
                        ) >> 10;
                    mmxs_flgs[nl] = 0;
                    ReturnBigc(devid, SLBPM_BIGC_MMX0,
                               NULL, 0,
                               mmxs + nl, sizeof(mmxs[nl]), sizeof(mmxs[nl]),
                               mmxs_flgs[nl]);
                }
                ReturnBigc(devid, SLBPM_BIGC_MMXA,
                           NULL, 0,
                           mmxs, sizeof(mmxs), sizeof(mmxs[0]),
                           mmxs_flgs[0]);
                ReturnChanGroup(devid, SLBPM_CHAN_MMX0, 4, mmxs, mmxs_flgs);

                break;

            case SLBCMD_RDREG:
            case SLBCMD_WR_RD:
                esn_pkt(me, cmd, addr);

                val32 = val16 = ((uint32)(data[2]) << 8) | data[3];

                /* Is it a PING reply?  Then check value */
                if      (addr == SLBPM_PING_REG)
                {
                    if (val16 != SLBPM_PING_VAL)
                    {
                        sq_clear(&(me->q));
                        SendInit(me);
                        DoDriverLog(devid, 0, "PING val differ");

                        SetDevState(devid, DEVSTATE_NOTREADY,  0, NULL);
                        SetDevState(devid, DEVSTATE_OPERATING, 0, NULL);
                    }
                }
                else if (addr == SLBREG_DELAYA)
                    ReturnChanGroup(devid, SLBPM_CHAN_DELAYA,
                                    1, &val32, &zero_rflags);
                else if (addr >= SLBREG_DELAY0  &&  addr <= SLBREG_DELAY3)
                    ReturnChanGroup(devid, SLBPM_CHAN_DELAY0 + addr - SLBREG_DELAY0,
                                    1, &val32, &zero_rflags);
                else if (addr == SLBREG_NAV)
                    ReturnChanGroup(devid, SLBPM_CHAN_NAV,
                                    1, &val32, &zero_rflags);
                else if (addr == SLBREG_BORDER)
                    ReturnChanGroup(devid, SLBPM_CHAN_BORDER,
                                    1, &val32, &zero_rflags);

                break;

#if 0
            case SLBCMD_RDZER:
                esn_pkt(me, cmd, addr);
                val32 = ((uint32)(data[2 + 0]) << 24) |
                        ((uint32)(data[2 + 1]) << 16) |
                        ((uint32)(data[2 + 2]) <<  8) |
                        ((uint32)(data[2 + 3])      );
                ReturnChanGroup(devid, SLBPM_CHAN_ZERO0 + addr,
                                1, &val32, &zero_rflags);

                break;
#endif

            case SLBCMD_AUTOB:
            case SLBCMD_IRBUF:
            case SLBCMD_SRBUF:
                esn_pkt(me, cmd, addr);
                if (cmd == SLBCMD_AUTOB)
                {
                    param0 = data[5];
                    ofs    = 6;
                }
                else
                {
                    param0 = data[1];
                    ofs    = 2;
                }

                if (param0 > MAX_NAV) param0 = MAX_NAV;
                for (x = 0;  x < param0 * 4;  x++)
                    bufs[x] =
                        ((uint32)(data[ofs + x * 4 + 0]) << 24) |
                        ((uint32)(data[ofs + x * 4 + 1]) << 16) |
                        ((uint32)(data[ofs + x * 4 + 2]) <<  8) |
                        ((uint32)(data[ofs + x * 4 + 3])      );

                ReturnBigc(devid, SLBPM_BIGC_BUFA,
                           &param0, 1,
                           bufs,
                           sizeof(bufs[0]) * 4 * param0,
                           sizeof(bufs[0]),
                           zero_rflags);
                for (nl = 0;  nl < 4;  nl++)
                    ReturnBigc(devid, SLBPM_BIGC_BUF0 + nl,
                               &param0, 1,
                               bufs + param0 * nl,
                               sizeof(bufs[0]) * param0,
                               sizeof(bufs[0]),
                               zero_rflags);
                break;

            case SLBCMD_RDOSC:
                param0 = 32;
                for (x = 0;  x < 4 * 32;  x++)
                    bufs[x] =
                        ((uint32)(data[2 + x * 4 + 0]) << 24) |
                        ((uint32)(data[2 + x * 4 + 1]) << 16) |
                        ((uint32)(data[2 + x * 4 + 2]) <<  8) |
                        ((uint32)(data[2 + x * 4 + 3])      );

                ReturnBigc(devid, SLBPM_BIGC_OSCA,
                           &param0, 1,
                           bufs,
                           sizeof(bufs[0]) * 4 * param0,
                           sizeof(bufs[0]),
                           zero_rflags);
                for (nl = 0;  nl < 4;  nl++)
                    ReturnBigc(devid, SLBPM_BIGC_OSC0 + nl,
                               &param0, 1,
                               bufs + param0 * nl,
                               sizeof(bufs[0]) * param0,
                               sizeof(bufs[0]),
                               zero_rflags);
                break;

            default:
                DoDriverLog(devid, DRIVERLOG_INFO | DRIVERLOG_C_PKTINFO,
                            "packet with unknown command %d, len=%d",
                            cmd, r);
        }

 CONTINUE_TO_NEXT_PACKET:;
    }
 NORMAL_EXIT:;
}

static void slbpm_alv(int devid, void *devptr,
                      sl_tid_t tid,
                      void *privptr)
{
  slbpm_privrec_t    *me = (slbpm_privrec_t *) devptr;
  slbpm_hdr_t         hdr;
  int                 r;

    me->alv_tid = sl_enq_tout_after(me->devid, devptr, ALIVE_USECS, slbpm_alv, NULL);

    if (0)
    {
        FillHdr(&hdr, SLBCMD_RDREG, SLBPM_PING_REG, 0);
        r = send(me->fd, &hdr, sizeof(hdr), 0);
    }
}

//////////////////////////////////////////////////////////////////////

static int slbpm_init_d(int devid, void *devptr,
                          int businfocount, int businfo[],
                          const char *auxinfo)
{
  slbpm_privrec_t    *me = (slbpm_privrec_t *) devptr;

  unsigned long       host_ip;
  struct hostent     *hp;
  struct sockaddr_in  bpm_addr;
  int                 fd;
  size_t              bsize;                     // Parameter for setsockopt
  int                 r;

    me->devid   = devid;
    me->fd      = -1;
    me->sq_tid  = -1;

    if (auxinfo == NULL  ||  *auxinfo == '\0')
    {
        DoDriverLog(devid, 0,
                    "%s: either BPM hostname or IP should be specified in auxinfo",
                    __FUNCTION__);
        return -CXRF_CFG_PROBL;
    }

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
    
    /* Record bpm's ip:port */
    bpm_addr.sin_family = AF_INET;
    bpm_addr.sin_port   = htons(DEFAULT_SLBPM_PORT);
    memcpy(&(bpm_addr.sin_addr), &host_ip, sizeof(host_ip));

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
    bsize = 1500 * 1000; // An arbitrary value
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &bsize, sizeof(bsize));
    /* Specify the other endpoint */
    r = connect(fd, &bpm_addr, sizeof(bpm_addr));
    if (r != 0)
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: connect()", __FUNCTION__);
        return -CXRF_DRV_PROBL;
    }

    /* Init queue */
    r = sq_init(&(me->q), NULL,
                SLBPM_SENDQ_SIZE, sizeof(bpmqelem_t),
                SLBPM_SENDQ_TOUT, me,
                slbpm_sender,
                slbpm_eq_cmp_func,
                tim_enqueuer,
                tim_dequeuer);
    if (r < 0)
    {
        ////DoDriverLog(me->devid, 0 | DRIVERLOG_ERRNO, "%s: sq_init()", __FUNCTION__);
        return -CXRF_DRV_PROBL;
    }

    sl_add_fd(me->devid, devptr, fd, SL_RD, slbpm_fd_p, NULL);

    me->alv_tid = sl_enq_tout_after(me->devid, devptr, ALIVE_USECS, slbpm_alv, NULL);

    SendInit(me);

    return DEVSTATE_OPERATING;
}

static void slbpm_term_d(int devid, void *devptr)
{
  slbpm_privrec_t *me = (slbpm_privrec_t *) devptr;

    SendTerm(me);
    if (me->alv_tid >= 0)
        sl_deq_tout(me->alv_tid);
    me->alv_tid = -1;
    sq_fini(&(me->q));
    /*!!! fd, fdio?*/
}

static void slbpm_rw_p(int devid, void *devptr,
                       int firstchan, int count, int *values, int action)
{
  slbpm_privrec_t *me = (slbpm_privrec_t *) devptr;
  int              n;     // channel N in values[] (loop ctl var)
  int              x;     // channel indeX
  int32            val;   // Value
  int              addr;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x >= SLBPM_CHAN_MMX0  &&  x <= SLBPM_CHAN_MMX3)
        {
            /* Do-nothing, those are returned automatically */
        }
#if 0
        else if (0  &&  x >= SLBPM_CHAN_ZERO0  &&  x <= SLBPM_CHAN_ZERO3)
        {
            addr = x - SLBPM_CHAN_ZERO0;
            enq_pkt(me, SLBCMD_RDZER, addr, 0);
        }
#endif
        else if ((x == SLBPM_CHAN_DELAYA)  ||
                 (x >= SLBPM_CHAN_DELAY0  &&  x <= SLBPM_CHAN_DELAY3))
        {
            addr = x == SLBPM_CHAN_DELAYA? SLBREG_DELAYA
                                         : SLBREG_DELAY0 + (x - SLBPM_CHAN_DELAY0);

            if (action == DRVA_WRITE)
            {
                if (val < 0)    val = 0;
                if (val > 1023) val = 1023;

                enq_pkt(me, SLBCMD_WRREG, addr, val);
            }
            enq_pkt(me, SLBCMD_RDREG, addr, 0);
        }
        else if (x == SLBPM_CHAN_EMINUS)
        {
            if (action == DRVA_WRITE)
            {
                me->eminus = (val != 0);
                SendInit(me);
            }
            ReturnChanGroup(devid, x, 1, &(me->eminus), &zero_rflags);
        }
        else if (x == SLBPM_CHAN_AMPLON)
        {
            if (action == DRVA_WRITE)
            {
                me->amplon = (val != 0);
                SendInit(me);
            }
            ReturnChanGroup(devid, x, 1, &(me->amplon), &zero_rflags);
        }
        else if (x == SLBPM_CHAN_NAV)
        {
            if (action == DRVA_WRITE)
            {
                if (val < 0)  val = 0;
                if (val > 63) val = 63;

                enq_pkt(me, SLBCMD_WRREG, SLBREG_NAV, val);
            }
            enq_pkt(me, SLBCMD_RDREG, SLBREG_NAV, 0);
        }
        else if (x == SLBPM_CHAN_BORDER)
        {
            if (action == DRVA_WRITE)
            {
                if (val < 0)  val = 0;
                if (val > 31) val = 31;

                enq_pkt(me, SLBCMD_WRREG, SLBREG_BORDER, val);
            }
            enq_pkt(me, SLBCMD_RDREG, SLBREG_BORDER, 0);
        }
        else if (x == SLBPM_CHAN_SHOT)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                enq_pkt(me, SLBCMD_START, 0, 0);
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == SLBPM_CHAN_CALB_0)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                enq_pkt(me, SLBCMD_DOCLB, 0, 0);
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}

static void slbpm_bigc_p(int devid, void *devptr,
                         int bigc,
                         int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
  slbpm_privrec_t *me = (slbpm_privrec_t *) devptr;

#if 1
    ////DoDriverLog(devid, 0, "%s(%d)", __FUNCTION__, bigc);
    if (bigc == SLBPM_BIGC_OSCA  ||
        (bigc >= SLBPM_BIGC_OSC0  &&  bigc <= SLBPM_BIGC_OSC3))
        enq_ons(me, SLBCMD_RDOSC, 0, 0);
#endif
}

static CxsdMainInfoRec slbpm_main_info[] =
{
    {SLBPM_CONFIG_CHAN_count, 1},
    {SLBPM_READ_CHAN_count,   0},
};

static CxsdBigcInfoRec slbpm_bigc_info[] =
{
};

DEFINE_DRIVER(slbpm,  "Styuf's Linac BPM",
              NULL, NULL,
              sizeof(slbpm_privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(slbpm_main_info), slbpm_main_info,
              countof(slbpm_bigc_info)*0-1, slbpm_bigc_info,
              slbpm_init_d, slbpm_term_d,
              slbpm_rw_p, slbpm_bigc_p);
