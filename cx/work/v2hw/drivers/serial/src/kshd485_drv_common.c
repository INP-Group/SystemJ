#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h> // For PATH_MAX, at least on ARM@MOXA

#include <termios.h>

#include "misc_macros.h"
#include "misclib.h"
#include "cxsd_driver.h"

#include "drv_i/kshd485_drv_i.h"

#ifndef SERIALHAL_FILE_H
  #error The "SERIALHAL_FILE_H" macro is undefined
#else
  #include SERIALHAL_FILE_H
#endif


/**** KSHD485 instruction set description ***************************/

/* PIV485 basic codes */
enum
{
    PIV485_START = 0xAA,
    PIV485_STOP  = 0xAB,
    PIV485_SHIFT = 0xAC
};

/* KSHD485 commands */
enum
{
    KCMD_GET_INFO      = 1,
    KCMD_REPEAT_REPLY  = 2,
    KCMD_GET_STATUS    = 3,
    KCMD_GO            = 4,
    KCMD_GO_WO_ACCEL   = 5,
    KCMD_SET_CONFIG    = 6,
    KCMD_SET_SPEEDS    = 7,
    KCMD_STOP          = 8,
    KCMD_SWITCHOFF     = 9,
    KCMD_SAVE_CONFIG   = 10,
    KCMD_PULSE         = 11,
    KCMD_GET_REMSTEPS  = 12,
    KCMD_GET_CONFIG    = 13,
    KCMD_GET_SPEEDS    = 14,
    KCMD_TEST_FREQ     = 15,
    KCMD_CALIBR_PREC   = 16,
    KCMD_GO_PREC       = 17,
    KCMD_OUT_IMPULSE   = 18,
    KCMD_SET_ABSCOORD  = 19,
    KCMD_GET_ABSCOORD  = 20,
    KCMD_SYNC_MULTIPLE = 21,
    KCMD_SAVE_USERDATA = 22,
    KCMD_RETR_USERDATA = 23,

    /* Not a real command */
    KCMD_0xFF = 0xFF
};

static int piv485_send(int devid, int fd, uint8 addr, uint8 *buf, size_t bufsize)
{
  int           x;
  uint8         csum;
  uint8        *p;
  ssize_t       nb;
  ssize_t       r;

  char          bufimage[200];
  char         *bip;
  const char   *pfx;

  static uint8  b_start = PIV485_START;
  static uint8  b_stop  = PIV485_STOP;
  static uint8  shifts[3][2] =
  {
      {PIV485_SHIFT, PIV485_START - PIV485_START},
      {PIV485_SHIFT, PIV485_STOP  - PIV485_START},
      {PIV485_SHIFT, PIV485_SHIFT - PIV485_START},
  };
  
    if (write(fd, &b_start, sizeof(b_start)) != sizeof(b_start)) return -1;

    bip = bufimage;
    *bip = '\0';
    for (x = 0;  x < (int)bufsize;  x++)
        bip += sprintf(bip, "%s%02x",
                       x == 0? "":",", buf[x]);
    bip += sprintf(bip, " -> ");

    for (x = -1, csum = 0;  x <= (int)bufsize;  x++)
    {
        if      (x < 0)
        {
            p = &addr;
            csum ^= *p;
        }
        else if (x < (int)bufsize)
        {
            p = buf + x;
            csum ^= *p;
        }
        else
            p = &csum;
        
        if (*p >= PIV485_START  &&  *p <= PIV485_SHIFT)
        {
            p = &(shifts[*p - PIV485_START][0]);
            nb = 2;
        }
        else
        {
            nb = 1;
        }

        pfx             = ",";
        if (x <  0) pfx = "";
        if (x == 0) pfx = ":";
        if (nb == 1)
            bip += sprintf(bip,
                           "%s%02x", pfx, *p);
        else
            bip += sprintf(bip,
                           "%s%02x-%02x", pfx, *p, p[1]);

        r = write(fd, p, nb);
        //DoDriverLog(devid, 0, "\tr=%d", r);
        if (r != nb) return -2;
    }
  
    if (write(fd, &b_stop,  sizeof(b_stop))  != sizeof(b_stop))  return -3;

    DoDriverLog(devid, 0 | DRIVERLOG_C_PKTDUMP, "SEND %s", bufimage);
  
    return 0;
}

/********************************************************************/


/*********************************************************************
  CHANNELS LAYOUT:
      0r  device version
      1r  device serial #
      2-  unused
      3r  current status (all bits in 1 byte)
      4-11r status-by-bits
      12w tok rabochij
      13w tok uderzhaniya
      14w zaderzhka
      15w min.velocity
      16w max.velocity
      17w acceleration
      18w configbyte
      19w # steps
      20c go
      21c go w/o acceleration
      22c stop
      23c switch off current

*********************************************************************/

enum {KSHD485_NUMCHANS = 24};

enum {MAXPKTBYTES  = 15};
enum {OUTQUEUESIZE = 10};

typedef struct
{
    uint8  len;
    uint8  data[MAXPKTBYTES];
} qelem_t;

typedef struct
{
    int32  vals[KSHD485_NUMCHANS];
} initvals_t;

typedef struct
{
    int         devid;
    int         baudrate;
    int         port_id;
    int         unit_id;
    int         fd;
    sl_fdh_t    fdhandle;

    int         dev_identified;
    int         status_cycle;
    int         is_suffering;
    
    initvals_t  opts;
    int32       cache[KSHD485_NUMCHANS];
    int32       stopcond;

    uint8       inbuf[MAXPKTBYTES * 2];
    size_t      inbufused;

    uint8       pkt_expected;
    int         is_ping;
    
    qelem_t     outqueue[OUTQUEUESIZE];
    int         outqueueused;
    sl_tid_t    tid;
} privrec_t;

static void MaybePreset(privrec_t *me, int cn)
{
    if (me->opts.vals[cn] >= 0)
    {
        me->cache    [cn] = me->opts.vals[cn];
        //me->opts.vals[cn] = -1;
    }
}

static void DoReset(int devid, privrec_t *me)
{
    /* Reset queue */
    me->inbufused    = 0;
    me->is_suffering = 0;
    me->pkt_expected = 0;
    me->is_ping      = 1;
    me->outqueueused = 0;

    /* ...and notify server */
    SetDevState(devid, DEVSTATE_NOTREADY,  CXRF_REM_C_PROBL, NULL);
    SetDevState(devid, DEVSTATE_OPERATING, 0,                NULL);
}


/********************************************************************/

static void errreport(void *opaqueptr,
                      const char *format, ...)
{
  int            devid = ptr2lint(opaqueptr);

  va_list        ap;
  
    va_start(ap, format);
    vDoDriverLog(devid, 0 | DRIVERLOG_ERRNO, format, ap);
    va_end(ap);
}


/********************************************************************/

static void kshd485_tout_p(int devid, void *devptr,
                           sl_tid_t tid,
                           void *privptr);
static void fd_p(int devid, void *devptr,
                 sl_fdh_t fdhandle, int fd, int mask,
                 void *privptr);

static int TryToSend(privrec_t *me)
{
  int  r;
    
    if (me->pkt_expected != 0) return 1;

    if (me->outqueueused == 0) return 0;

    me->pkt_expected = me->outqueue[0].data[0];
    r = piv485_send(me->devid, me->fd, me->unit_id,
                    me->outqueue[0].data, me->outqueue[0].len);
    me->is_suffering = (r != 0);

    if (me->tid >= 0)
    {
        sl_deq_tout(me->tid);
        me->tid = -1;
    }
    me->tid = sl_enq_tout_after(me->devid, me, 1000000, kshd485_tout_p, NULL);

    me->outqueueused--;
    if (me->outqueueused != 0)
        memmove(&(me->outqueue[0]), &(me->outqueue[1]),
                sizeof(qelem_t) * me->outqueueused);

    return r;
}

static int EnqueuePkt(privrec_t *me, uint8 code, int nbytes, ...)
{
  va_list  ap;
  uint8    data[MAXPKTBYTES];
  int      x;
    
    if (nbytes > MAXPKTBYTES - 1)
    {
        DoDriverLog(me->devid, 0, "%s: EXTRALONG PACKET REQUEST -- %d BYTES, WHILE LIMIT IS %d!!!",
                    __FUNCTION__, nbytes, MAXPKTBYTES - 1);
        return -1;
    }

    if (me->outqueueused >= OUTQUEUESIZE)
    {
        DoDriverLog(me->devid, 0, "%s: output queue overflow",
                    __FUNCTION__);
        return -1;
    }
    
    data[0] = code;

    if (nbytes > 0)
    {
        va_start(ap, nbytes);
        
        for (x = 0;  x < nbytes;  x++)
        {
            data[x + 1] = va_arg(ap, int);
        }

        va_end(ap);
    }

    me->outqueue[me->outqueueused].len = nbytes + 1;
    memcpy(me->outqueue[me->outqueueused].data, data, nbytes + 1);
    me->outqueueused++;
  
    return TryToSend(me);
}

static int RequestKshdParams(privrec_t *me)
{
    if (
        EnqueuePkt(me, KCMD_GET_INFO,   0) < 0  ||
        EnqueuePkt(me, KCMD_GET_STATUS, 0) < 0  ||
        EnqueuePkt(me, KCMD_GET_CONFIG, 0) < 0  ||
        EnqueuePkt(me, KCMD_GET_SPEEDS, 0) < 0
       ) return -1;

    return 0;
}

static void kshd485_tout_p(int devid, void *devptr,
                           sl_tid_t tid,
                           void *privptr)
{
  privrec_t *me       = (privrec_t *) devptr;
  uint8      pingcode = KCMD_GET_STATUS;
  int        r;
  static uint8  b_stop  = PIV485_STOP;

    me->tid = -1;

    /* Are we still waiting for any reply? */
    if (me->pkt_expected == 0) return;

    /* ...is it a scheduled re-open? */
    if (me->pkt_expected == KCMD_0xFF)
    {
#if 0
        ResetDev(devid);
#else
        /* Try to re-open the port */
        me->fd = serialhal_opendev(me->port_id, me->baudrate,
                                   errreport, lint2ptr(devid));
        if (me->fd >= 0)
        {
            write(me->fd, &b_stop, sizeof(b_stop));
            me->fdhandle = sl_add_fd(devid, devptr, me->fd, SL_RD, fd_p, NULL);
            DoDriverLog(devid, 0, "ZZZ:pre");
            DoReset(devid, me);
            DoDriverLog(devid, 0, "ZZZ:post");
            return;
        }
        SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "re-open()");
#endif
        return;
    }
    
    /*  */
    if (!me->is_ping)
    {
        DoDriverLog(devid, 0, "%s: TIMEOUT EXPIRED waiting for response %d; %d requests in outqueue",
                    __FUNCTION__, me->pkt_expected, me->outqueueused);
    }

    r = piv485_send(devid, me->fd, me->unit_id, &pingcode, 1);
    if (r < 0)
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: piv485_send: r=%d, errno=%d", __FUNCTION__, r, errno);
    me->is_suffering = (r != 0);
    me->pkt_expected = pingcode;
    me->is_ping      = 1;
    me->outqueueused = 0;
    
    if (me->tid >= 0)
    {
        sl_deq_tout(me->tid);
        me->tid = -1;
    }
    me->tid = sl_enq_tout_after(me->devid, devptr, 1000000, kshd485_tout_p, NULL);
}

static void PingReplyArrived(privrec_t *me)
{
    me->is_ping = 0;

    RequestKshdParams(me);
}

                      
/********************************************************************/

static inline int IsStatusReq(uint8 cmd)
{
    return cmd >= KCMD_GET_STATUS  &&  cmd <= KCMD_PULSE;
}

static int StatusReqInQueue(privrec_t *me)
{
  int  x;
    
    if (IsStatusReq(me->pkt_expected)) return 1;

    for (x = 0;  x < me->outqueueused;  x++)
        if (IsStatusReq(me->outqueue[x].data[0])) return 1;
    
    return 0;
}

static psp_paramdescr_t text2initvals[] =
{
    PSP_P_INT("work_current", initvals_t, vals[KSHD485_C_WORK_CURRENT], -1, 0, 7),
    PSP_P_INT("hold_current", initvals_t, vals[KSHD485_C_HOLD_CURRENT], -1, 0, 7),
    PSP_P_INT("hold_delay",   initvals_t, vals[KSHD485_C_HOLD_DELAY],   -1, 0, 255),
    PSP_P_INT("config_byte",  initvals_t, vals[KSHD485_C_CONFIG_BYTE],  -1, 0, 255),
    PSP_P_INT("min_velocity", initvals_t, vals[KSHD485_C_MIN_VELOCITY], -1, 32, 12000),
    PSP_P_INT("max_velocity", initvals_t, vals[KSHD485_C_MAX_VELOCITY], -1, 32, 12000),
    PSP_P_INT("acceleration", initvals_t, vals[KSHD485_C_ACCELERATION], -1, 32, 65535),
    PSP_P_END()
};

static int baudrate_table[] =
{
    B9600,
    B1200,
    B2400,
    B4800,
    B9600,
    B19200,
    B38400,
    B57600,
};


//#include <asm/ioctls.h>
#include <sys/ioctl.h>
static void fd_check_p(int devid, void *devptr,
                       sl_tid_t tid,
                       void *privptr)
{
  privrec_t  *me    = (privrec_t *) devptr;
  int         r1;
  int         r2;

    sl_enq_tout_after(devid, devptr, 1000000, fd_check_p, NULL);
    if (me->fd < 0) return;
    r1 = check_fd_state(me->fd, O_RDONLY);
    ioctl(me->fd, FIONREAD, &r2);
    //DoDriverLog(devid, 0, "%s(): r1=%d r2=%d", __FUNCTION__, r1, r2);
}

static int init_d(int devid, void *devptr, 
                  int businfocount, int businfo[],
                  const char *auxinfo)
{
  privrec_t  *me      = (privrec_t *) devptr;
  int         baud_id = businfo[2];
  int         port_id = businfo[1];
  int         unit_id = businfo[0];
  rflags_t    zero_rflags = 0;

  int         x;

  static uint8  b_stop  = PIV485_STOP;

    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    if (baud_id < 0  ||  baud_id >= countof(baudrate_table))
    {
        DoDriverLog(devid, 0, "unsupported baudrate_n=%d", baud_id);
        return -CXRF_CFG_PROBL;
    }

    me->stopcond = me->cache[KSHD485_C_GO_WO_ACCEL] = KSHD485_STOPCOND_S1;

    for (x = 0;  x < countof(me->opts.vals); x++) me->opts.vals[x] = -1;
    if (psp_parse(auxinfo, NULL,
                  &(me->opts),
                  '=', " \t", "",
                  text2initvals) != PSP_R_OK)
    {
        DoDriverLog(devid, 0, "psp_parse(auxinfo): %s", psp_error());
        return -CXRF_CFG_PROBL;
    }
    for (x = 0; x < countof(me->opts.vals); x++) MaybePreset(me, x);

    /* Fill in the privrec */
    me->devid    = devid;
    me->baudrate = baudrate_table[baud_id];
    me->port_id  = port_id;
    me->unit_id  = unit_id;

    me->fd = serialhal_opendev(me->port_id, me->baudrate,
                               errreport, lint2ptr(devid));
    if (me->fd < 0) return me->fd;
    write(me->fd, &b_stop, sizeof(b_stop));
    me->fdhandle = sl_add_fd(devid, devptr, me->fd, SL_RD, fd_p, NULL);

    me->tid      = -1;

    ReturnChanGroup(devid,
                    KSHD485_C_NUM_STEPS, 1,
                    me->cache + KSHD485_C_NUM_STEPS,
                    &zero_rflags);
    RequestKshdParams(me);

#if 0
    sl_enq_tout_after(devid, devptr, 1000000, fd_check_p, NULL);
#endif

    return DEVSTATE_OPERATING;
}

static void term_d(int devid, void *devptr)
{
  privrec_t *me    = (privrec_t *) devptr;

    if (me->fd >= 0)
    {
        sl_del_fd(me->fdhandle); me->fdhandle = -1;
        close(me->fd);
        me->fd = -1;
    }
}

static void fd_p(int devid, void *devptr,
                 sl_fdh_t fdhandle, int fd, int mask,
                 void *privptr)
{
  privrec_t *me    = (privrec_t *) devptr;
  int        r;
  uint8     *endp;
  size_t     inpktlen;
  uint8     *src;
  uint8     *dst;
  uint8      pkt[20];
  uint8      csum;

  uint8      cmd;

  rflags_t   emptyflags[countof(me->cache)];
  int        x;

  char       bufimage[200];
  char      *bip;

    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s", __FUNCTION__);

    /* Do we have any space left? */
    if (me->inbufused == sizeof(me->inbuf))
    {
        DoDriverLog(devid, 0, "%s: input buffer overflow; resetting",
                    __FUNCTION__);
        DoReset(devid, me);
        return;
    }

    /* Read what is available */
    r = read(fd,
             me->inbuf + me->inbufused,
             sizeof(me->inbuf) - me->inbufused);

    if (r <= 0)
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: read=%d",
                    __FUNCTION__, r);
        /*!!! Note: this code, together with matching code in _tout_p(),
              is probably never reached.
              At least, unless a true EOF arrives somewhen somehow */
        if (r == 0)
        {
            /* Okay, let's close old fd... */
            sl_del_fd(fdhandle); me->fdhandle = -1;
            close(fd);
            me->fd = -1;
            /* ...and schedule re-open */
            me->pkt_expected = KCMD_0xFF;
            if (me->tid >= 0)
            {
                sl_deq_tout(me->tid);
                me->tid = -1;
            }
            me->tid = sl_enq_tout_after(me->devid, devptr, 5000000, kshd485_tout_p, NULL);
            return;
        }
        /* else -- r<0 is definitely an error */
        SetDevState(devid, DEVSTATE_OFFLINE, CXRF_DRV_PROBL, "read() problem");
        return;
    }

    DoDriverLog(devid, DRIVERLOG_DEBUG | DRIVERLOG_C_PKTDUMP,
                "r=%d, [%zu]=0x%02x",
                r, me->inbufused, me->inbuf[me->inbufused]);

    me->inbufused += r;

    /* Did we receive the STOP byte? */
    endp = memchr(me->inbuf, PIV485_STOP, me->inbufused);
    if (endp == NULL)
    {
        /*!!! And this code is probably unreachable too, since buffer overflow
              is detected at the beginning.
              And here it was because of poor design decision in 2003... */
        if (me->inbufused == sizeof(me->inbuf))
        {
            DoDriverLog(devid, 0, "%s: input buffer overflow",
                        __FUNCTION__);
            DoReset(devid, me);
        }
        
        return;
    }
    inpktlen = (endp - me->inbuf) + 1;

    for (x = 0, bip = bufimage;  x < inpktlen;  x++)
        bip += sprintf(bip, "%s%02x",
                       x == 0? "":",", me->inbuf[x]);
    DoDriverLog(devid, DRIVERLOG_NOTICE | DRIVERLOG_C_PKTDUMP,
                "RECV [%zu], type=%d=0x%02x %s",
                inpktlen, me->pkt_expected, me->pkt_expected, bufimage);

    /* Okay, let's decode and check the packet... */
    for (src = me->inbuf, dst = pkt, csum = 0;
         src < endp;
         src++, dst++)
    {
        if (*src == PIV485_SHIFT)
        {
            src++;
            if (src == endp)
            {
                DoDriverLog(devid, 0, "%s: malformed packet (SHIFT,STOP)",
                            __FUNCTION__);
                DoReset(devid, me);
                return;
            }
            *dst = *src + PIV485_START;
        }
        else
            *dst = *src;

        csum ^= *dst;
    }

    if (csum != 0)
    {
        DoDriverLog(devid, 0, "%s: malformed packet (csum!=0)",
                    __FUNCTION__);
        DoReset(devid, me);
        return;
    }

    cmd = me->pkt_expected;
    me->pkt_expected = 0;
    if (me->is_ping) PingReplyArrived(me);
    TryToSend(me);
    
    /* Oka-a-a-a-y... */
    bzero(emptyflags, sizeof(emptyflags));
    if      (cmd == KCMD_GET_INFO)
    {
        me->cache[KSHD485_C_DEV_VERSION] = pkt[3] * 1000 + pkt[4];
        me->cache[KSHD485_C_DEV_SERIAL]  = pkt[5] * 256  + pkt[6];
        me->dev_identified = 1;
        ReturnChanGroup(devid,
                        KSHD485_C_DEV_VERSION, 2,
                        me->cache + KSHD485_C_DEV_VERSION,
                        emptyflags);
    }
    else if (cmd >= KCMD_GET_STATUS  &&  cmd <= KCMD_PULSE)
    {
        me->cache[KSHD485_C_STATUS_BYTE] = pkt[1];
        for (x = 0;  x < 8;  x++)
            me->cache[KSHD485_C_STATUS_BYTE + 1 + x] = ((pkt[1] & (1 << x)) != 0);
        /*me->status_cycle = GetCurrentCycle();*/ // In fact, was unused
        
        ReturnChanGroup(devid,
                        KSHD485_C_STATUS_BYTE, 1 + 8,
                        me->cache + KSHD485_C_STATUS_BYTE,
                        emptyflags);
    }
    else if (cmd == KCMD_GET_CONFIG)
    {
        me->cache[KSHD485_C_WORK_CURRENT] = pkt[1]; MaybePreset(me, KSHD485_C_WORK_CURRENT);
        me->cache[KSHD485_C_HOLD_CURRENT] = pkt[2]; MaybePreset(me, KSHD485_C_HOLD_CURRENT);
        me->cache[KSHD485_C_HOLD_DELAY  ] = pkt[3]; MaybePreset(me, KSHD485_C_HOLD_DELAY);
        me->cache[KSHD485_C_CONFIG_BYTE ] = pkt[4]; MaybePreset(me, KSHD485_C_CONFIG_BYTE);

        ReturnChanGroup(devid,
                        KSHD485_C_WORK_CURRENT, 4,
                        me->cache + KSHD485_C_WORK_CURRENT,
                        emptyflags);
    }
    else if (cmd == KCMD_GET_SPEEDS)
    {
        me->cache[KSHD485_C_MIN_VELOCITY] = (pkt[1] << 8) + pkt[2]; MaybePreset(me, KSHD485_C_MIN_VELOCITY);
        me->cache[KSHD485_C_MAX_VELOCITY] = (pkt[3] << 8) + pkt[4]; MaybePreset(me, KSHD485_C_MAX_VELOCITY);
        me->cache[KSHD485_C_ACCELERATION] = (pkt[5] << 8) + pkt[6]; MaybePreset(me, KSHD485_C_ACCELERATION);

        ReturnChanGroup(devid,
                        KSHD485_C_MIN_VELOCITY, 3,
                        me->cache + KSHD485_C_MIN_VELOCITY,
                        emptyflags);
    }
    else
    {
        DoDriverLog(devid, 0, "%s: unknown packet type %d, length=%zu",
                    __FUNCTION__, cmd, inpktlen);
    }

    /* Remove the packet from input buffer */
    if (inpktlen < me->inbufused)
        memmove(me->inbuf, me->inbuf + inpktlen, me->inbufused - inpktlen);
    me->inbufused -= inpktlen;
}

static void rw_p(int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  privrec_t *me     = (privrec_t *) devptr;
  int        n;
  int        x;
  rflags_t   flags;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

        if      (x == KSHD485_C_DEV_VERSION  ||  x == KSHD485_C_DEV_SERIAL)
        {
            flags = me->dev_identified? 0 : CXRF_IO_TIMEOUT; /*!!! What should we really return if we hadn't heard from him yet? */
            ReturnChanGroup(devid, x, 1, me->cache + x, &flags);
        }
        else if (x >= KSHD485_C_STATUS_BYTE  &&  x <= KSHD485_C_STATUS_BASE + 7)
        {
            if (!StatusReqInQueue(me))
                EnqueuePkt(me,
                           KCMD_GET_STATUS,
                           0);
        }
        else if (x >= KSHD485_C_WORK_CURRENT  &&  x <= KSHD485_C_NUM_STEPS)
        {
            if (action == DRVA_WRITE)
                me->cache[x] = me->opts.vals[x] = values[n];
            flags = 0;
            ReturnChanGroup(devid, x, 1, me->cache + x, &flags);
        }
#if 1
        else if (x == KSHD485_C_GO_WO_ACCEL)
        {
            if (action == DRVA_WRITE)
            {
                me->stopcond = me->cache[x] = values[n] & 0x0F;
            }

            flags = 0;
            ReturnChanGroup(devid, x, 1, me->cache + x, &flags);
        }
#endif
        else if (x == KSHD485_C_GO  ||  x == KSHD485_C_GO_WO_ACCEL)
        {
            if (action == DRVA_WRITE  &&  values[n] == CX_VALUE_COMMAND)
            {
                EnqueuePkt(me,
                           KCMD_SET_CONFIG,
                           4,
                           me->cache[KSHD485_C_WORK_CURRENT],
                           me->cache[KSHD485_C_HOLD_CURRENT],
                           me->cache[KSHD485_C_HOLD_DELAY],
                           me->cache[KSHD485_C_CONFIG_BYTE]);
                EnqueuePkt(me,
                           KCMD_SET_SPEEDS,
                           6,
                           (me->cache[KSHD485_C_MIN_VELOCITY] >> 8) & 0xFF,
                           (me->cache[KSHD485_C_MIN_VELOCITY]     ) & 0xFF,
                           (me->cache[KSHD485_C_MAX_VELOCITY] >> 8) & 0xFF,
                           (me->cache[KSHD485_C_MAX_VELOCITY]     ) & 0xFF,
                           (me->cache[KSHD485_C_ACCELERATION] >> 8) & 0xFF,
                           (me->cache[KSHD485_C_ACCELERATION]     ) & 0xFF);
                EnqueuePkt(me,
                           x == KSHD485_C_GO? KCMD_GO : KCMD_GO_WO_ACCEL,
                           5,
                           (me->cache[KSHD485_C_NUM_STEPS] >> 24) & 0xFF,
                           (me->cache[KSHD485_C_NUM_STEPS] >> 16) & 0xFF,
                           (me->cache[KSHD485_C_NUM_STEPS] >>  8) & 0xFF,
                           (me->cache[KSHD485_C_NUM_STEPS]      ) & 0xFF,
                           KSHD485_STOPCOND_S1);
            }

            flags = 0;
            ReturnChanGroup(devid, x, 1, me->cache + x, &flags);
        }
        else if (x == KSHD485_C_STOP  ||  x == KSHD485_C_SWITCHOFF)
        {
            if (action == DRVA_WRITE  &&  values[n] == CX_VALUE_COMMAND)
                EnqueuePkt(me,
                           x == KSHD485_C_STOP? KCMD_STOP : KCMD_SWITCHOFF,
                           0);

            flags = 0;
            ReturnChanGroup(devid, x, 1, me->cache + x, &flags);
        }
    }
}


DEFINE_DRIVER(__CX_CONCATENATE(SERIALDRV_PFX, kshd485), "KShD485, "__CX_STRINGIZE(SERIALDRV_PFX)"-version",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              2, 3,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_d, term_d,
              rw_p, NULL);
