#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/param.h> // For PATH_MAX, at least on ARM@MOXA

#include "cxsd_driver.h"
#include "drv_i/nkshd485_drv_i.h"


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;


//// piv485_pre_lyr.h ////////////////////////////////////////////////
#include "sendqlib.h"


enum {PIV485_MAXPKTBYTES = 15};


/* Driver-provided callbacks */

typedef void (*Piv485PktinProc) (int devid, void *devptr,
                                 int cmd, int dlc, uint8 *data);
typedef void (*Piv485OnSendProc)(int devid, void *devptr,
                                 sq_try_n_t try_n, void *privptr);
typedef void (*Piv485SndMdProc) (int devid, void *devptr,
                                 int *dlc_p, uint8 *data);
typedef int  (*Piv485PktCmpFunc)(int dlc, uint8 *data, void *privptr);

/* Layer API for drivers */

typedef void (*DisconnectDeviceFromLayer)(int devid);

typedef int  (*Piv485AddDevice)(int devid, void *devptr,
                                int businfocount, int businfo[],
                                Piv485PktinProc inproc,
                                Piv485SndMdProc mdproc,
                                int queue_size);

typedef void        (*Piv485QClear)     (int handle);

typedef sq_status_t (*Piv485QEnqTotal)  (int handle, sq_enqcond_t how,
                                         int tries, int timeout_us,
                                         Piv485OnSendProc on_send, void *privptr,
                                         int model_dlc, int dlc, uint8 *data);
typedef sq_status_t (*Piv485QEnqTotal_v)(int handle, sq_enqcond_t how,
                                         int tries, int timeout_us,
                                         Piv485OnSendProc on_send, void *privptr,
                                         int model_dlc, int dlc, ...);
typedef sq_status_t (*Piv485QEnq)       (int handle, sq_enqcond_t how,
                                         int dlc, uint8 *data);
typedef sq_status_t (*Piv485QEnq_v)     (int handle, sq_enqcond_t how,
                                         int dlc, ...);

typedef sq_status_t (*Piv485QErAndSNx)  (int handle,
                                         int dlc, uint8 *data);
typedef sq_status_t (*Piv485QErAndSNx_v)(int handle,
                                         int dlc, ...);

typedef sq_status_t (*Piv485Foreach)    (int handle,
                                         Piv485PktCmpFunc do_cmp, void *privptr,
                                         sq_action_t action);

/* Piv485 interface itself */

typedef struct
{
    /* General device management */
    DisconnectDeviceFromLayer  disconnect;
    Piv485AddDevice            open;

    /* Queue management */
    Piv485QClear               q_clear;

    Piv485QEnqTotal            q_enqueue;
    Piv485QEnqTotal_v          q_enqueue_v;
    Piv485QEnq                 q_enq;
    Piv485QEnq_v               q_enq_v;

    Piv485QErAndSNx            q_erase_and_send_next;
    Piv485QErAndSNx_v          q_erase_and_send_next_v;

    Piv485Foreach              q_foreach;
} piv485_lvmt_t;

piv485_lvmt_t *Piv485GetLvmt(void);


//// piv485_pre_lyr.c ////////////////////////////////////////////////
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "misclib.h"

#ifndef SERIALHAL_FILE_H
  #error The "SERIALHAL_FILE_H" macro is undefined
#else
  #include SERIALHAL_FILE_H
#endif


enum
{
    PIV485_START = 0xAA,
    PIV485_STOP  = 0xAB,
    PIV485_SHIFT = 0xAC
};

enum {PIV485_PKTBUFSIZE = 1 + (PIV485_MAXPKTBYTES + 1) * 2 + 1};

enum
{
    PIV485_DEFAULT_TIMEOUT =  1 *  100 * 1000, // 100 ms
};

typedef struct
{
    sq_eprops_t       props;

    Piv485OnSendProc  on_send;
    void             *privptr;

    int               dlc;  /* 'dlc' MUST be signed (instead of size_t) in order to permit partial-comparison (triggered by negative dlc) */
    uint8             data[PIV485_MAXPKTBYTES];
} pivqelem_t;

enum
{
    NUMLINES    = 8,
    DEVSPERLINE = 256,
};

typedef struct
{
    /* Driver properties */
    int              line;    // For back-referencing when passed just dp
    int              kid;     // The same
    int              devid;
    void            *devptr;
    Piv485PktinProc  inproc;
    Piv485SndMdProc  mdproc;

    /* Life state */
    int              last_cmd;

    /* Support */
    sq_q_t           q;
    sl_tid_t         tid;
} pivdevinfo_t;

typedef struct
{
    int           fd;
    sl_fdh_t      fdhandle;
    sl_tid_t      tid;
    sq_port_t     port;

    int           last_kid;

    uint8         inbuf[PIV485_PKTBUFSIZE];
    size_t        inbufused;

    pivdevinfo_t  devs[DEVSPERLINE];
} lineinfo_t;


static int         my_lyrid;

static lineinfo_t  lines[NUMLINES];

static inline int  encode_handle(int line, int kid)
{
    return (line << 16)  |  kid;
}

static inline void decode_handle(int handle, int *line_p, int *kid_p)
{
    *line_p = handle >> 16;
    *kid_p  = handle & 255;
}

static int devid2handle(int devid)
{
  int  l;
  int  d;

    for (l = 0;  l < countof(lines);  l++)
        for (d = 0;  d < countof(lines[l].devs);  d++)
            if (lines[l].devs[d].devid == devid)
                return encode_handle(l, d);

    return -1;
}

#define DECODE_AND_CHECK(errret)                                             \
    do {                                                                     \
        decode_handle(handle, &line, &kid);                                  \
        if (line < 0  ||  line >= countof(lines)  ||  lines[line].fd < 0  || \
            kid  < 0  ||  kid  >= countof(lines[line].devs))                 \
        {                                                                    \
            DoDriverLog(my_lyrid, 0,                                         \
                        "%s: invalid handle %d (=>%d:%d)",                   \
                        __FUNCTION__, handle, line, kid);                    \
            return errret;                                                   \
        }                                                                    \
        dp = lines[line].devs + kid;                                         \
        if (dp->devid < 0)                                                   \
        {                                                                    \
            DoDriverLog(my_lyrid, 0,                                         \
                        "%s: attempt to use unregistered dev=%d:%d",         \
                        __FUNCTION__, line, kid);                            \
            return errret;                                                   \
        }                                                                    \
    } while (0)

//--------------------------------------------------------------------

static void errreport(void *opaqueptr,
                      const char *format, ...)
{
  int            devid = ptr2lint(opaqueptr);

  va_list        ap;
  
    va_start(ap, format);
    vDoDriverLog(devid, 0 | DRIVERLOG_ERRNO, format, ap);
    va_end(ap);
}

static int  piv485_sender     (void *elem, void *privptr)
{
  pivdevinfo_t *dp = privptr;
  pivqelem_t   *qe = elem;
  lineinfo_t   *lp = lines + dp->line;

  uint8         databuf[PIV485_PKTBUFSIZE];
  int           len;
  uint8         csum;
  int           x;
  uint8         c;

  int           r;

  char          bufimage[PIV485_PKTBUFSIZE*3];
  char         *bip;

    /* 0. Do we suffer from yet-unopened serial-device? */
    if (lp->fd < 0) return -1;

    /**/
    if (dp->mdproc != NULL)
        dp->mdproc(dp->devid, dp->devptr, &(qe->dlc), qe->data);

    /**/
    bip = bufimage;
    *bip = '\0';
    for (x = 0;  x < qe->dlc;  x++)
        bip += sprintf(bip, "%s%02x",
                       x == 0? "":",", qe->data[x]);

    /* I. Prepare data in "PIV485"-encoded representation */
    len  = 0;
    csum = 0;

    /* 1. The START byte */
    databuf[len++] = PIV485_START;
    /* 2. The address */
    csum ^= (databuf[len++] = dp->kid);
    /* 3. Packet data, including trailing checksum */
    for (x = 0;  x <= qe->dlc /* '==' - csum */;  x++)
    {
        if (x < qe->dlc)
        {
            c = qe->data[x];
            csum ^= c;
        }
        else
            c = csum;

        if (c >= PIV485_START  &&  c <= PIV485_SHIFT)
        {
            databuf[len++] = PIV485_SHIFT;
            databuf[len++] = c - PIV485_START;
        }
        else
            databuf[len++] = c;
    }
    /* 4. The STOP byte */
    databuf[len++] = PIV485_STOP;

    /**/
    bip += sprintf(bip, " -> ");
    for (x = 0;  x < len;  x++)
    {
        bip += sprintf(bip, "%s%02x", x == 0? "" : ",", databuf[x]);
    }
    DoDriverLog(dp->devid, 0 | DRIVERLOG_C_PKTDUMP, "SEND %d:%s",
                dp->kid, bufimage);

    /* II. Send data to port altogether */
    r = n_write(lp->fd, databuf, len);

    /* III. Could we send the whole packet? */
    if (r == len)
    {
        lp->last_kid = dp->kid;
        dp->last_cmd = qe->data[0];
//if (1 && dp->last_cmd == 1) fprintf(stderr, "%s   sender: kid=%d %s\n", strcurtime_msc(), lp->last_kid, bufimage);
//fprintf(stderr, "%s sender: kid=%d %s\n", strcurtime_msc(), lp->last_kid, bufimage);
        return 0;
    }

    /* Unfortunately, no */
    /*!!! Re-open?*/
    lp->last_kid = -1;
    dp->last_cmd = -1;

    return -1;
}

static int  piv485_eq_cmp_func(void *elem, void *ptr)
{
  pivqelem_t *a = elem;
  pivqelem_t *b = ptr;
  
    if (b->dlc >= 0)
        return
            a->dlc  ==  b->dlc   &&
            (a->dlc ==  0  ||
             memcmp(a->data, b->data, a->dlc) == 0);
    else
        return
            a->dlc  >= -b->dlc   &&
             memcmp(a->data, b->data, -b->dlc) == 0;
}


static void port_tout_proc(int devid, void *devptr,
                           sl_tid_t tid,
                           void *privptr)
{
  int           line = ptr2lint(privptr);
  lineinfo_t   *lp   = lines + line;

    lp->tid = -1;
    sq_port_tout(&(lp->port));
}

static int  port_tim_enqueuer(void *privptr, int usecs)
{
  int           line = ptr2lint(privptr);
  lineinfo_t   *lp   = lines + line;

    if (lp->tid >= 0)
    {
        sl_deq_tout(lp->tid);
        lp->tid = -1;
    }
    lp->tid = sl_enq_tout_after(my_lyrid, NULL, usecs, port_tout_proc, lint2ptr(line));
    return lp->tid >= 0? 0 : -1;
}

static void port_tim_dequeuer(void *privptr)
{
  int           line = ptr2lint(privptr);
  lineinfo_t   *lp   = lines + line;

    if (lp->tid >= 0)
    {
        sl_deq_tout(lp->tid);
        lp->tid = -1;
    }
}

static void piv485_tout_proc(int devid, void *devptr,
                             sl_tid_t tid,
                             void *privptr)
{
  int           handle = ptr2lint(privptr);
  int           line;
  int           kid;
  pivdevinfo_t *dp;

    DECODE_AND_CHECK();

    dp->tid = -1;
    sq_timeout(&(dp->q));
}

static int  tim_enqueuer(void *privptr, int usecs)
{
  pivdevinfo_t *dp = privptr;

    if (dp->tid >= 0)
    {
        sl_deq_tout(dp->tid);
        dp->tid = -1;
    }
    dp->tid = sl_enq_tout_after(dp->devid, dp->devptr, usecs, piv485_tout_proc,
                                lint2ptr(encode_handle(dp->line, dp->kid)));
    return dp->tid >= 0? 0 : -1;
}

static void tim_dequeuer(void *privptr)
{
  pivdevinfo_t *dp = privptr;

    if (dp->tid >= 0)
    {
        sl_deq_tout(dp->tid);
        dp->tid = -1;
    }
}

static void piv485_send_callback(void        *q_privptr,
                                 sq_eprops_t *e,
                                 sq_try_n_t   try_n)
{
  pivdevinfo_t *dp = q_privptr;
  pivqelem_t   *qe = (pivqelem_t *)e;

    qe->on_send(dp->devid, dp->devptr, try_n, qe->privptr);
}

//--------------------------------------------------------------------

static void piv485_disconnect(int devid)
{
  int           l;
  int           d;
  pivdevinfo_t *dp;

    for (l = 0;  l < countof(lines);  l++)
        for (d = 0;  d < countof(lines[l].devs);  d++)
            if (lines[l].devs[d].devid == devid)
            {
                dp = lines[l].devs + d;

                /* Release slot */
                sq_fini(&(dp->q));
                bzero(dp, sizeof(*dp));
                dp->devid = -1;

                /* Check if it is the current line's "owner" */
                if (lines[l].last_kid == d)
                    lines[l].last_kid = -1;

                /* Check if some other devices are active on this line */
                for (d = 0;  d < countof(lines[l].devs);  d++)
                    if (lines[l].devs[d].devid >= 0)
                        return;

                /* ...or was last "client" of this line? */
                /* Then release the line! */
                sl_del_fd(lines[l].fdhandle);
                lines[l].fdhandle = -1;
                close(lines[l].fd);
                lines[l].fd = -1;
                /* Note: we do NOT dequeue the port timeout, for it to remain active in case of subsequent port re-use */
                //if (lines[l].tid >= 0) sl_deq_tout(lines[l].tid);
                //lines[l].tid = -1;
                return;
            }

    DoDriverLog(devid, DRIVERLOG_WARNING,
                "%s: request to disconnect unknown devid=%d",
                __FUNCTION__, devid);
}

static void piv485_fd_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr);

static int piv485_init_lyr(int lyrid);

static int  piv485_open(int devid, void *devptr,
                        int businfocount, int businfo[],
                        Piv485PktinProc inproc,
                        Piv485SndMdProc mdproc,
                        int queue_size)
{
  int           line = businfo[0];
  int           kid  = businfo[1];

  lineinfo_t   *lp;
  pivdevinfo_t *dp;
  int           handle;
  int           r;

    piv485_init_lyr(DEVID_NOT_IN_DRIVER);

    if (line < 0  ||  line >= NUMLINES)
    {
        DoDriverLog(devid, 0, "%s: line=%d, out_of[%d..%d]",
                    __FUNCTION__, line, 0, NUMLINES);
        return -CXRF_CFG_PROBL;
    }
    if (kid < 0  ||  kid >= DEVSPERLINE)
    {
        DoDriverLog(devid, 0, "%s: kid=%d, out_of[%d..%d]",
                    __FUNCTION__, line, 0, DEVSPERLINE);
        return -CXRF_CFG_PROBL;
    }

    lp     = lines    + line;
    dp     = lp->devs + kid;
    handle = encode_handle(line, kid);

    if (lp->fd >= 0  &&  dp->devid >= 0)
    {
        DoDriverLog(devid, 0, "%s: device (%d,%d) is already in use",
                   __FUNCTION__, line, kid);
        ReturnChanGroup(dp->devid, 0, RETURNCHAN_COUNT_PONG, NULL, NULL);
        return -CXRF_CFG_PROBL;
    }

    dp->line     = line;
    dp->kid      = kid;
    dp->devid    = devid;
    dp->devptr   = devptr;
    dp->inproc   = inproc;
    dp->mdproc   = mdproc;
    dp->last_cmd = -1;

    r = sq_init(&(dp->q), &(lp->port),
                queue_size, sizeof(pivqelem_t),
                PIV485_DEFAULT_TIMEOUT, dp,
                piv485_sender,
                piv485_eq_cmp_func,
                tim_enqueuer,
                tim_dequeuer);
    if (r < 0)
    {
        DoDriverLog(devid, 0 | DRIVERLOG_ERRNO,
                    "%s: sq_init()", __FUNCTION__);
        return -CXRF_DRV_PROBL;
    }
    dp->tid = -1;

    if (lp->fd < 0)
    {
        lp->fd = serialhal_opendev(line, B9600,
                                   errreport, lint2ptr(devid));
        if (lp->fd < 0)
        {
            sq_fini(&(dp->q));
            return -CXRF_DRV_PROBL;
        }
        lp->tid = -1;

        lp->fdhandle = sl_add_fd(my_lyrid, NULL, lp->fd, SL_RD, piv485_fd_p, lint2ptr(line));
        lp->last_kid = -1;
    }

    return handle;
}

static void DoReset(lineinfo_t *lp)
{
  pivdevinfo_t *dp;

    if (lp->last_kid >= 0) lp->devs[lp->last_kid].last_cmd = -1;

    lp->last_kid  = -1;
    lp->inbufused = 0;
}

static sq_status_t piv485_q_erase_and_send_next  (int handle,
                                                  int dlc, uint8 *data);

static void piv485_fd_p(int unused_devid, void *unused_devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr)
{
  int           line = ptr2lint(privptr);
  lineinfo_t   *lp   = lines + line;
  pivdevinfo_t *dp;
  int           devid;

  int           r;

  uint8         data[PIV485_PKTBUFSIZE];
  int           x;
  int           dlc;
  uint8         csum;
  uint8         cmd;

  char          bufimage[PIV485_PKTBUFSIZE*3];
  char         *bip;

    /*!!! The device-address is in the packet, 0th byte!!! */
    if (lp->last_kid >= 0)
    {
        dp    = lines[line].devs + lp->last_kid;
        devid = dp->devid;
    }
    else
    {
        dp    = NULL;
        devid = my_lyrid;
    }

    /* Read 1 byte */
    r = uintr_read(fd,
                   lp->inbuf + lp->inbufused,
                   1);
    if (r <= 0) /*!!! Should we REALLY close/re-open?  */
    {
        /*!!! Should also (schedule?) re-open */
        sl_del_fd(lp->fdhandle); lp->fdhandle = -1;
        close(fd);
        lp->fd = -1;
        return;
    }

    /* Isn't it a STOP byte? */
    if (lp->inbuf[lp->inbufused] != PIV485_STOP)
    {
        lp->inbufused++;
        /* Is there still space in input buffer? */
        if (lp->inbufused < sizeof(lp->inbuf)) return;

        DoDriverLog(my_lyrid, DRIVERLOG_ALERT,
                    "%s: input buffer overflow; resetting",
                    __FUNCTION__);
        DoReset(lp);

        return;
    }

    /* Okay, the whole packet has arrived */
    /* Let's decode and check it */
    for (x = 0, dlc = 0, csum = 0;
         x < lp->inbufused;
         x++,   dlc++)
    {
        if (lp->inbuf[x] == PIV485_SHIFT)
        {
            x++;
            if (x == lp->inbufused)
            {
                DoDriverLog(devid, DRIVERLOG_WARNING,
                            "%s: malformed packet (SHIFT,STOP); last_kid=%d, devid=%d",
                            __FUNCTION__, lp->last_kid, devid);
                DoReset(lp);
                return;
            }
            data[dlc] = lp->inbuf[x] + PIV485_START;
        }
        else
            data[dlc] = lp->inbuf[x];
    }

    if (csum != 0)
    {
        DoDriverLog(devid, DRIVERLOG_WARNING,
                    "%s: malformed packet (csum!=0); last_kid=%d, devid=%d",
                    __FUNCTION__, lp->last_kid, devid);
        DoReset(lp);
        return;
    }

    /* Select target */
    dp    = lines[line].devs + data[0];
    devid = dp->devid;
    if (devid < 0)
    {
        dp    = NULL;
        devid = my_lyrid;
    }

    /* Log */
    bip = bufimage;
    *bip = '\0';
    for (x = 1;  x < dlc - 1;  x++)
        bip += sprintf(bip, "%s%02x",
                       x == 1? "":",", data[x]);
    bip += sprintf(bip, " <- ");
    for (x = 0;  x < lp->inbufused + 1;  x++)
    {
        bip += sprintf(bip, "%s%02x", x == 0? "" : ",", lp->inbuf[x]);
    }
    DoDriverLog(devid, 0 | DRIVERLOG_C_PKTDUMP, "RECV %d:%02x:%s",
                data[0], dp != NULL? dp->last_cmd : -1, bufimage);

//if (1  &&  dp != NULL  &&  dp->last_cmd == 1) fprintf(stderr, "%s GET_INFO: last_kid=%d;%d %s\n", strcurtime_msc(), lp->last_kid, data[0], bufimage);
//fprintf(stderr, "%s   fd_p: last_kid=%d;%d %s\n", strcurtime_msc(), lp->last_kid, data[0], bufimage);

    /* Reset input buffer to readiness */
    lp->last_kid  = -1;
    lp->inbufused = 0;

    if (dp != NULL)
    {
        cmd = dp->last_cmd;
        dp->last_cmd  = -1;

        /* (send next, if present) */
        piv485_q_erase_and_send_next(encode_handle(dp->line, dp->kid), -1, &cmd);
        /* Finally, dispatch the packet */
        if (dp->inproc != NULL)
            dp->inproc(dp->devid, dp->devptr,
                       cmd, dlc - 2, data + 1);
    }
}


static void        piv485_q_clear    (int handle)
{
  int           line;
  int           kid;
  pivdevinfo_t *dp;
  lineinfo_t   *lp;

    DECODE_AND_CHECK();
    sq_clear(&(dp->q));
    /*!!! any other actions -- like clearing "last-sent-cmd"? And/or "schedule next"? */
    lp = lines + line;
    if (lp->last_kid == kid)
        lp->last_kid = -1;
}

static sq_status_t piv485_q_enqueue  (int handle, sq_enqcond_t how,
                                      int tries, int timeout_us,
                                      Piv485OnSendProc on_send, void *privptr,
                                      int model_dlc, int dlc, uint8 *data)
{
  int           line;
  int           kid;
  pivdevinfo_t *dp;
  pivqelem_t    item;
  pivqelem_t    model;

    DECODE_AND_CHECK(SQ_ERROR);

    if (dlc < 1  ||  dlc > PIV485_MAXPKTBYTES) return SQ_ERROR;
    if (model_dlc > PIV485_MAXPKTBYTES  ||
        model_dlc > dlc)                       return SQ_ERROR;

    /* Prepare item... */
    bzero(&item, sizeof(item));

    item.props.tries      = tries;
    item.props.timeout_us = timeout_us;
    item.props.callback   = (on_send == NULL)? NULL : piv485_send_callback;

    item.on_send = on_send;
    item.privptr = privptr;
    item.dlc     = dlc;
    memcpy(item.data, data, dlc);

    /* ...and model, if required */
    if (model_dlc > 0)
    {
        model     = item;
        model.dlc = -model_dlc;
    }

    return sq_enq(&(dp->q), &(item.props), how, model_dlc > 0? &model : NULL);
}
static sq_status_t piv485_q_enqueue_v(int handle, sq_enqcond_t how,
                                      int tries, int timeout_us,
                                      Piv485OnSendProc on_send, void *privptr,
                                      int model_dlc, int dlc, ...)
{
  va_list  ap;
  uint8    data[PIV485_MAXPKTBYTES];
  int      x;

    if (dlc < 1  ||  dlc > PIV485_MAXPKTBYTES) return SQ_ERROR;
    va_start(ap, dlc);
    for (x = 0;  x < dlc;  x++) data[x] = va_arg(ap, int);
    va_end(ap);
    return piv485_q_enqueue(handle, how,
                            tries, timeout_us,
                            on_send, privptr,
                            model_dlc, dlc, data);
}
static sq_status_t piv485_q_enq      (int handle, sq_enqcond_t how,
                                      int dlc, uint8 *data)
{
    return piv485_q_enqueue(handle, how,
                            SQ_TRIES_INF, 0,
                            NULL, NULL,
                            0, dlc, data);
}
static sq_status_t piv485_q_enq_v    (int handle, sq_enqcond_t how,
                                      int dlc, ...)
{
  va_list  ap;
  uint8    data[PIV485_MAXPKTBYTES];
  int      x;

    if (dlc < 1  ||  dlc > PIV485_MAXPKTBYTES) return SQ_ERROR;
    va_start(ap, dlc);
    for (x = 0;  x < dlc;  x++) data[x] = va_arg(ap, int);
    va_end(ap);
    return piv485_q_enqueue(handle, how,
                            SQ_TRIES_INF, 0,
                            NULL, NULL,
                            0, dlc, data);
}

static sq_status_t piv485_q_erase_and_send_next  (int handle,
                                                  int dlc, uint8 *data)
{
  int           line;
  int           kid;
  pivdevinfo_t *dp;
  pivqelem_t    item;
  int           abs_dlc = abs(dlc);
  void         *fe;

    DECODE_AND_CHECK(SQ_ERROR);

    if (abs_dlc < 1  ||  abs_dlc > PIV485_MAXPKTBYTES) return SQ_ERROR;

    /* Make a model for comparison */
    item.dlc  = dlc;
    memcpy(item.data, data, abs_dlc);

    /* Check "whether we should really erase and send next packet" */
    fe = sq_access_e(&(dp->q), 0);
    if (fe != NULL  &&  piv485_eq_cmp_func(fe, &item) == 0)
        return SQ_NOTFOUND;

    /* Erase... */
    sq_foreach(&(dp->q), SQ_ELEM_IS_EQUAL, &item, SQ_ERASE_FIRST,  NULL);
    /* ...and send next */
    sq_sendnext(&(dp->q));

    return SQ_FOUND;
}
static sq_status_t piv485_q_erase_and_send_next_v(int handle,
                                                  int dlc, ...)
{
  va_list  ap;
  int      abs_dlc = abs(dlc);
  uint8    data[PIV485_MAXPKTBYTES];
  int      x;
  
    if (abs_dlc < 1  ||  abs_dlc > PIV485_MAXPKTBYTES) return SQ_ERROR;
    va_start(ap, dlc);
    for (x = 0;  x < abs_dlc;  x++) data[x] = va_arg(ap, int);
    va_end(ap);
    return piv485_q_erase_and_send_next(handle, dlc, data);
}

typedef struct
{
    Piv485PktCmpFunc  do_cmp;
    void             *privptr;
} cmp_rec_t;
static int piv485_foreach_cmp_func(void *elem, void *ptr)
{
  pivqelem_t *a  = elem;
  cmp_rec_t  *rp = ptr;

    return rp->do_cmp(a->dlc, a->data, rp->privptr);
}
static sq_status_t piv485_q_foreach(int handle,
                                    Piv485PktCmpFunc do_cmp, void *privptr,
                                    sq_action_t action)
{
  int           line;
  int           kid;
  pivdevinfo_t *dp;
  cmp_rec_t     rec;

    DECODE_AND_CHECK(SQ_ERROR);
    if (do_cmp == NULL) return SQ_ERROR;

    rec.do_cmp  = do_cmp;
    rec.privptr = privptr;
    return sq_foreach(&(dp->q),
                      piv485_foreach_cmp_func, &rec, action, NULL);
}

/*##################################################################*/

static int piv485_init_lyr(int lyrid)
{
  int  l;
  int  d;

  static int layer_initialized = 0;

    if (layer_initialized) return 0;

    my_lyrid = lyrid;
    DoDriverLog(my_lyrid, 0, "%s(%d)!", __FUNCTION__, lyrid);
    bzero(lines, sizeof(lines));
    for (l = 0;  l < countof(lines);  l++)
    {
        lines[l].fd  = -1;
        lines[l].tid = -1;
        sq_port_init(&(lines[l].port),
                     lint2ptr(l), port_tim_enqueuer, port_tim_dequeuer);

        for (d = 0;  d < countof(lines[l].devs);  d++)
            lines[l].devs[d].devid = -1;
    }
    layer_initialized = 1;
    DoDriverLog(my_lyrid, 0, "END %s", __FUNCTION__);

    return 0;
}

static piv485_lvmt_t piv485_lvmt =
{
    piv485_disconnect,
    piv485_open,

    piv485_q_clear,
    piv485_q_enqueue,
    piv485_q_enqueue_v,
    piv485_q_enq,
    piv485_q_enq_v,
    piv485_q_erase_and_send_next,
    piv485_q_erase_and_send_next_v,
    piv485_q_foreach,
};

piv485_lvmt_t *Piv485GetLvmt(void)
{
    return &piv485_lvmt;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
#include "cxsd_driver.h"


/**** KSHD485 instruction set description ***************************/

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

/* STOPCOND bits (for GO and GO_WO_ACCEL commands */
enum
{
    KSHD485_STOPCOND_KP = 1 << 2,
    KSHD485_STOPCOND_KM = 1 << 3,
    KSHD485_STOPCOND_S0 = 1 << 4,
    KSHD485_STOPCOND_S1 = 1 << 5
};

/**/
enum
{
    KSHD485_CONFIG_HALF     = 1 << 0,
    KSHD485_CONFIG_KM       = 1 << 2,
    KSHD485_CONFIG_KP       = 1 << 3,
    KSHD485_CONFIG_SENSOR   = 1 << 4,
    KSHD485_CONFIG_SOFTK    = 1 << 5,
    KSHD485_CONFIG_LEAVEK   = 1 << 6,
    KSHD485_CONFIG_ACCLEAVE = 1 << 7
};

/********************************************************************/

enum {NKSHD485_OUTQUEUESIZE = 15};


typedef struct
{
    piv485_lvmt_t *lvmt;
    int            handle;
    int            devid;

    struct
    {
        int  info;
        int  config;
        int  speeds;
    }              got;

    int32          cache[NKSHD485_NUMCHANS];
} privrec_t;

static inline int IsReady(privrec_t *me)
{
    return (me->cache[NKSHD485_CHAN_STATUS_BYTE] & (1 << 0/*!!!READY*/)) != 0;
}

static inline int DevVersion(privrec_t *me)
{
    return me->cache[NKSHD485_CHAN_DEV_VERSION];
}

//////////////////////////////////////////////////////////////////////

static int RequestKshdParams(privrec_t *me)
{
    me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 1, KCMD_GET_INFO);
    me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 1, KCMD_GET_STATUS);
    me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 1, KCMD_GET_CONFIG);
    me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 1, KCMD_GET_SPEEDS);

    return 0;
}

static void UploadConfig(privrec_t *me)
{
    me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 5,
                      KCMD_SET_CONFIG,
                      me->cache[NKSHD485_CHAN_WORK_CURRENT],
                      me->cache[NKSHD485_CHAN_HOLD_CURRENT],
                      me->cache[NKSHD485_CHAN_HOLD_DELAY],
                      me->cache[NKSHD485_CHAN_CONFIG_BYTE]);
}

static void SetConfigByte(privrec_t *me, uint32 val, uint32 mask, int immed)
{
  int  n;

    me->cache[NKSHD485_CHAN_CONFIG_BYTE] =
        (me->cache[NKSHD485_CHAN_CONFIG_BYTE] &~ mask) |
        (val                                  &  mask);
    ReturnChanGroup(me->devid, NKSHD485_CHAN_CONFIG_BYTE, 1,
                    me->cache + NKSHD485_CHAN_CONFIG_BYTE, &zero_rflags);

    for (n = 0;  n <= 7;  n++)
        if ((mask & (1 << n)) != 0)
        {
            me->cache[NKSHD485_CHAN_CONFIG_BIT_base + n] =
                (val >> n) & 1;
            ReturnChanGroup(me->devid, NKSHD485_CHAN_CONFIG_BIT_base + n, 1,
                            me->cache + NKSHD485_CHAN_CONFIG_BIT_base + n, &zero_rflags);
        }

    if (immed  &&  me->got.config) UploadConfig(me);
}

static void SetStopCond(privrec_t *me, uint32 val, uint32 mask)
{
  int  n;

    me->cache[NKSHD485_CHAN_STOPCOND_BYTE] =
        (me->cache[NKSHD485_CHAN_STOPCOND_BYTE] &~ mask) |
        (val                                    &  mask);
    ReturnChanGroup(me->devid, NKSHD485_CHAN_STOPCOND_BYTE, 1,
                    me->cache + NKSHD485_CHAN_STOPCOND_BYTE, &zero_rflags);

    for (n = 0;  n <= 7;  n++)
        if ((mask & (1 << n)) != 0)
        {
            me->cache[NKSHD485_CHAN_STOPCOND_BIT_base + n] =
                (val >> n) & 1;
            ReturnChanGroup(me->devid, NKSHD485_CHAN_STOPCOND_BIT_base + n, 1,
                            me->cache + NKSHD485_CHAN_STOPCOND_BIT_base + n, &zero_rflags);
        }
}

static int SendGo(privrec_t *me, int cmd, int32 steps, uint8 stopcond)
{
    if (1)
    {
        me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 5,
                          KCMD_SET_CONFIG,
                          me->cache[NKSHD485_CHAN_WORK_CURRENT],
                          me->cache[NKSHD485_CHAN_HOLD_CURRENT],
                          me->cache[NKSHD485_CHAN_HOLD_DELAY],
                          me->cache[NKSHD485_CHAN_CONFIG_BYTE]);
        me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 7,
                          KCMD_SET_SPEEDS,
                          (me->cache[NKSHD485_CHAN_MIN_VELOCITY] >> 8) & 0xFF,
                          (me->cache[NKSHD485_CHAN_MIN_VELOCITY]     ) & 0xFF,
                          (me->cache[NKSHD485_CHAN_MAX_VELOCITY] >> 8) & 0xFF,
                          (me->cache[NKSHD485_CHAN_MAX_VELOCITY]     ) & 0xFF,
                          (me->cache[NKSHD485_CHAN_ACCELERATION] >> 8) & 0xFF,
                          (me->cache[NKSHD485_CHAN_ACCELERATION]     ) & 0xFF);
        me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 6,
                          cmd,
                          (steps >> 24) & 0xFF,
                          (steps >> 16) & 0xFF,
                          (steps >>  8) & 0xFF,
                           steps        & 0xFF,
                           stopcond);

        return 1;
    }

    return 0;
}

static int nkshd485_is_status_req_cmp_func(int dlc, uint8 *data,
                                           void *privptr __attribute__((unused)))
{
    return data[0] >= KCMD_GET_STATUS  &&  data[0] <= KCMD_PULSE;
}
static int StatusReqInQueue(privrec_t *me)
{
    return me->lvmt->q_foreach(me->handle,
                               nkshd485_is_status_req_cmp_func, NULL,
                               SQ_FIND_FIRST) != SQ_NOTFOUND;
}

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int32  steps;
    int32  stopcond;
} kshd485_initvals_t;

static psp_paramdescr_t text2initvals[] =
{
    PSP_P_INT("steps",    kshd485_initvals_t, steps,    1000, 0, 0),
    PSP_P_INT("stopcond", kshd485_initvals_t, stopcond, 0,    0, 255),
    PSP_P_END()
};

static void nkshd485_in(int devid, void *devptr,
                        int cmd, int dlc, uint8 *data);
static void nkshd485_md(int devid, void *devptr,
                        int *dlc_p, uint8 *data);

static int nkshd485_init_d(int devid, void *devptr, 
                           int businfocount, int businfo[],
                           const char *auxinfo)
{
  privrec_t          *me = (privrec_t *)devptr;

  kshd485_initvals_t  opts;
  int                 x;

    /*!!! Parse init-vals */
    if (psp_parse(auxinfo, NULL,
                  &(opts),
                  '=', " \t", "",
                  text2initvals) != PSP_R_OK)
    {
        DoDriverLog(devid, 0, "psp_parse(auxinfo): %s", psp_error());
        return -CXRF_CFG_PROBL;
    }
    me->cache[NKSHD485_CHAN_STOPCOND_BYTE] = opts.stopcond;
    me->cache[NKSHD485_CHAN_NUM_STEPS]     = opts.steps;

    me->lvmt   = Piv485GetLvmt();
    me->handle = me->lvmt->open(devid, me, 
                                businfocount, businfo,
                                nkshd485_in, nkshd485_md,
                                NKSHD485_OUTQUEUESIZE);
    if (me->handle < 0) return me->handle;
    me->devid  = devid;

    RequestKshdParams(me);

    return DEVSTATE_OPERATING;
}

static void nkshd485_term_d(int devid, void *devptr)
{
  privrec_t      *me = (privrec_t *)devptr;

    me->lvmt->disconnect(devid);
}

static void nkshd485_rw_p(int devid, void *devptr __attribute__((unused)), int firstchan, int count, int32 *values, int action)
{
  privrec_t      *me = (privrec_t *)devptr;
  int             n;        // channel N in values[] (loop ctl var)
  int             x;        // channel indeX
  int32           val;      // Value
  rflags_t        flags;
  int32           nsteps;
  int32           stopcond;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        flags = 0;

        if      (x == NKSHD485_CHAN_DEV_VERSION  ||  x == NKSHD485_CHAN_DEV_SERIAL)
        {
            if (me->got.info)
            {
                val   = me->cache[x];
                flags = 0;
            }
            else
            {
                val   = -1;
                flags = CXRF_IO_TIMEOUT;
            }
            ReturnChanGroup(devid, x, 1, &val, &flags);
        }
        else if (x == NKSHD485_CHAN_STEPS_LEFT)
        {
            if (0)
                me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 1,
                                  KCMD_GET_REMSTEPS);
        }
        else if (x == NKSHD485_CHAN_ABSCOORD  &&  DevVersion(me) >= 21)
        {
            if (action == DRVA_WRITE)
            {
                if (me->lvmt->q_enqueue_v(me->handle, SQ_REPLACE_NOTFIRST,
                                          SQ_TRIES_INF, 0,
                                          NULL, NULL,
                                          1, 5,
                                          KCMD_SET_ABSCOORD,
                                          (val >> 24) & 0xFF,
                                          (val >> 16) & 0xFF,
                                          (val >>  8) & 0xFF,
                                          val        & 0xFF) == SQ_NOTFOUND)
                    me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 1,
                                      KCMD_GET_ABSCOORD);
            }
            else
                me->lvmt->q_enq_v(me->handle, SQ_IF_ABSENT, 1,
                                  KCMD_GET_ABSCOORD);
        }
        else if (x == NKSHD485_CHAN_STATUS_BYTE   ||
                 (x >= NKSHD485_CHAN_STATUS_base  &&
                  x <= NKSHD485_CHAN_STATUS_base + 7))
        {
            if (!StatusReqInQueue(me))
                me->lvmt->q_enq_v(me->handle, SQ_ALWAYS, 1,
                                  KCMD_GET_STATUS);
        }
        else if (x == NKSHD485_CHAN_WORK_CURRENT  ||
                 x == NKSHD485_CHAN_HOLD_CURRENT  ||
                 x == NKSHD485_CHAN_HOLD_DELAY)
        {
            /*!!!MIN/MAX!!!*/
            if (action == DRVA_WRITE)
                me->cache[x] = val;
            if (me->got.config)
            {
                ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
                UploadConfig(me);
            }
        }
        else if (x == NKSHD485_CHAN_MIN_VELOCITY  ||
                 x == NKSHD485_CHAN_MAX_VELOCITY  ||
                 x == NKSHD485_CHAN_ACCELERATION)
        {
            /*!!!MIN/MAX!!!*/
            if (action == DRVA_WRITE)
                me->cache[x] = val;
            if (me->got.speeds)
                ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else if (x == NKSHD485_CHAN_CONFIG_BYTE  ||
                 (x >= NKSHD485_CHAN_CONFIG_BIT_base  &&  x <= NKSHD485_CHAN_CONFIG_BIT_base + 7))
        {
            if (action == DRVA_WRITE)
            {
                if (x == NKSHD485_CHAN_CONFIG_BYTE)
                    SetConfigByte(me, val, 0xFF, 1);
                else
                    SetConfigByte(me,
                                  (val != 0) << (x - NKSHD485_CHAN_CONFIG_BIT_base),
                                  (1)        << (x - NKSHD485_CHAN_CONFIG_BIT_base),
                                  1);
            }
            else
                if (me->got.config)
                    ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else if (x == NKSHD485_CHAN_STOPCOND_BYTE  ||
                 (x >= NKSHD485_CHAN_STOPCOND_BIT_base  &&  x <= NKSHD485_CHAN_STOPCOND_BIT_base + 7))
        {
            if (action == DRVA_WRITE)
            {
                if (x == NKSHD485_CHAN_STOPCOND_BYTE)
                    SetStopCond(me, val, 0xFF);
                else
                    SetStopCond(me,
                                (val != 0) << (x - NKSHD485_CHAN_STOPCOND_BIT_base),
                                (1)        << (x - NKSHD485_CHAN_STOPCOND_BIT_base));
            }
            else
                ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else if (x == NKSHD485_CHAN_NUM_STEPS)
        {
            if (action == DRVA_WRITE)
                me->cache[x] = val;
            ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else if (x == NKSHD485_CHAN_GO    ||  x == NKSHD485_CHAN_GO_WO_ACCEL)
        {
            if (action == DRVA_WRITE  &&  values[n] == CX_VALUE_COMMAND)
                SendGo(me,
                       x == NKSHD485_CHAN_GO? KCMD_GO : KCMD_GO_WO_ACCEL,
                       me->cache[NKSHD485_CHAN_NUM_STEPS],
                       me->cache[NKSHD485_CHAN_STOPCOND_BYTE]);
            ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else if (x == NKSHD485_CHAN_STOP  ||  x == NKSHD485_CHAN_SWITCHOFF)
        {
            if (action == DRVA_WRITE  &&  values[n] == CX_VALUE_COMMAND)
                me->lvmt->q_enq_v(me->handle, SQ_IF_NONEORFIRST, 1, 
                                  x == NKSHD485_CHAN_STOP? KCMD_STOP : KCMD_SWITCHOFF);
            ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else if (x == NKSHD485_CHAN_GO_N_STEPS  ||  x == NKSHD485_CHAN_GO_WOA_N_STEPS)
        {
            if (action == DRVA_WRITE  &&  val != 0)
            {
                nsteps = val & 0x00FFFFFF;
                if (nsteps & 0x00800000) nsteps |= 0xFF000000; // Sign-extend 24->32 bits
                stopcond = (val >> 24) & 0xFF;
                if (stopcond == 0) stopcond = me->cache[NKSHD485_CHAN_STOPCOND_BYTE];
                SendGo(me,
                       x == NKSHD485_CHAN_GO_N_STEPS? KCMD_GO : KCMD_GO_WO_ACCEL,
                       nsteps, stopcond);
            }
            ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else if (x == NKSHD485_CHAN_DO_RESET)
        {
            ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
            if (action == DRVA_WRITE  &&  values[n] == CX_VALUE_COMMAND)
            {
                me->lvmt->q_clear(me->handle);
                SetDevState(devid, DEVSTATE_NOTREADY,  0, NULL);
                SetDevState(devid, DEVSTATE_OPERATING, 0, NULL);
            }
        }
        else if (x == NKSHD485_CHAN_SAVE_CONFIG)
        {
            if (action == DRVA_WRITE  &&  values[n] == CX_VALUE_COMMAND)
                me->lvmt->q_enq_v(me->handle, SQ_IF_NONEORFIRST, 1, 
                                  KCMD_SAVE_CONFIG);
            ReturnChanGroup(devid, x, 1, me->cache + x, &zero_rflags);
        }
        else
        {
            val   = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}

static void nkshd485_in(int devid, void *devptr,
                        int cmd, int dlc, uint8 *data)
{
  privrec_t      *me = (privrec_t *)devptr;

  int             prev_ready;

  int             x;

    if      (cmd == KCMD_GET_INFO)
    {
        me->got.info = 1;
        me->cache[NKSHD485_CHAN_DEV_VERSION] = data[2] * 10  + data[3];
        me->cache[NKSHD485_CHAN_DEV_SERIAL]  = data[4] * 256 + data[5];
        ReturnChanGroup(devid,      NKSHD485_CHAN_DEV_VERSION, 1,
                        me->cache + NKSHD485_CHAN_DEV_VERSION, &zero_rflags);
        ReturnChanGroup(devid,      NKSHD485_CHAN_DEV_SERIAL,  1,
                        me->cache + NKSHD485_CHAN_DEV_SERIAL,  &zero_rflags);
    }
    else if (cmd >= KCMD_GET_STATUS  &&  cmd <= KCMD_PULSE)
    {
        prev_ready = IsReady(me);
        me->cache[NKSHD485_CHAN_STATUS_BYTE] = data[0];
        ReturnChanGroup(devid,      NKSHD485_CHAN_STATUS_BYTE, 1,
                        me->cache + NKSHD485_CHAN_STATUS_BYTE, &zero_rflags);
        for (x = 0;  x < 8;  x++)
        {
            me->cache[NKSHD485_CHAN_STATUS_base + x] = (data[0] >> x) & 1;
            ReturnChanGroup(devid,      NKSHD485_CHAN_STATUS_base + x, 1,
                            me->cache + NKSHD485_CHAN_STATUS_base + x, &zero_rflags);
        }

        if (prev_ready == 0  &&  IsReady(me))
        {
            if (DevVersion(me) >= 20)
                me->lvmt->q_enq_v(me->handle, SQ_IF_NONEORFIRST, 1,
                                  KCMD_GET_REMSTEPS);
            if (DevVersion(me) >= 21)
                me->lvmt->q_enq_v(me->handle, SQ_IF_NONEORFIRST, 1,
                                  KCMD_GET_ABSCOORD);
        }
    }
    else if (cmd == KCMD_GET_REMSTEPS)
    {
        me->cache[NKSHD485_CHAN_STEPS_LEFT] =
            (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
        ReturnChanGroup(devid,      NKSHD485_CHAN_STEPS_LEFT, 1,
                        me->cache + NKSHD485_CHAN_STEPS_LEFT, &zero_rflags);
    }
    else if (cmd == KCMD_GET_ABSCOORD)
    {
        me->cache[NKSHD485_CHAN_ABSCOORD] =
            (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
        ReturnChanGroup(devid,      NKSHD485_CHAN_ABSCOORD, 1,
                        me->cache + NKSHD485_CHAN_ABSCOORD, &zero_rflags);
    }
    else if (cmd == KCMD_GET_CONFIG)
    {
        me->got.config = 1;
        for (x = 0;  x < 3;  x++)
        {
            me->cache                  [NKSHD485_CONFIG_CHAN_base + x] = data[x];
            ReturnChanGroup(devid,      NKSHD485_CONFIG_CHAN_base + x, 1,
                            me->cache + NKSHD485_CONFIG_CHAN_base + x, &zero_rflags);
        }
        SetConfigByte(me, data[3], 0xFF, 0);
    }
    else if (cmd == KCMD_GET_SPEEDS)
    {
        me->got.speeds = 1;
        for (x = 0;  x < 3;  x++)
        {
            me->cache[NKSHD485_SPEEDS_CHAN_base + x] =
                (data[x*2] << 8) + data[x*2 + 1];
            ReturnChanGroup(devid,      NKSHD485_SPEEDS_CHAN_base + x, 1,
                            me->cache + NKSHD485_SPEEDS_CHAN_base + x, &zero_rflags);
        }
    }
    else
    {
        /* How can this happen? 'cmd' is the code of last-sent packet */
        DoDriverLog(devid, DRIVERLOG_ALERT,
                    "%s: !!! unknown packet type %d, length=%d",
                    __FUNCTION__, cmd, dlc);
    }
}

static void nkshd485_md(int devid, void *devptr,
                        int *dlc_p, uint8 *data)
{
  privrec_t      *me = (privrec_t *)devptr;

    if (!IsReady(me)  &&
        (
         *data == KCMD_GO          ||  *data == KCMD_GO_WO_ACCEL  ||
         *data == KCMD_SET_CONFIG  ||  *data == KCMD_SET_SPEEDS   ||
         *data == KCMD_SWITCHOFF   ||  *data == KCMD_SAVE_CONFIG  ||
         *data == KCMD_GO_PREC
         ||
         *data == KCMD_GET_REMSTEPS  ||
         *data == KCMD_SET_ABSCOORD  ||  *data == KCMD_GET_ABSCOORD
        ))
    {
        *dlc_p = 1; data[0] = KCMD_GET_STATUS;
    }
}

/* Metric */

static int  nkshd485_init_m(void)
{
    return piv485_init_lyr(DEVID_NOT_IN_DRIVER);
}

static CxsdMainInfoRec nkshd485_main_info[] =
{
    {NKSHD485_RD_CHAN_count, 0},
    {NKSHD485_WR_CHAN_count, 1},
};
DEFINE_DRIVER(__CX_CONCATENATE(SERIALDRV_PFX, nkshd485), "n-KShD485, "__CX_STRINGIZE(SERIALDRV_PFX)"-version",
              nkshd485_init_m, NULL,
              sizeof(privrec_t), NULL,
              2, 3,
              NULL, 0,
              countof(nkshd485_main_info), nkshd485_main_info,
              -1, NULL,
              nkshd485_init_d, nkshd485_term_d,
              nkshd485_rw_p, NULL);
