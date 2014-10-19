#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "misc_macros.h"
#include "misclib.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/cankoz_common_drv_i.h"
#include "cankoz_strdb.h"


enum {NUMLINES    = 8};
enum {DEVSPERLINE = 64};

typedef struct
{
    int              devid;
    void            *devptr;
    int              devcode;
    CanKozOnIdProc   rst;
    CanKozPktinProc  cb;

    int              hw_ver;
    int              sw_ver;
    int              hw_code;

    sl_tid_t         q_tid;

    canqelem_t      *ring;
    int              ring_size;
    int              ring_start;
    int              ring_used;

    struct
    {
        int    base;
        int    rcvd;
        int    pend;
        
        uint8  cur_val;
        uint8  req_val;
        uint8  req_msk;
        uint8  cur_inp;
    }                ioregs;
} kozdevinfo_t;

typedef struct
{
    int           fd;
    sl_fdh_t      fdhandle;
    int           last_rd_r;
    int           last_wr_r;
    int           last_ex_id;
    kozdevinfo_t  devs[DEVSPERLINE];
} lineinfo_t;

static lineinfo_t lines[NUMLINES];
static int layer_initialized = 0;

static int log_frame_allowed = 1;


static inline int  encode_handle(int line, int kid)
{
    return (line << 16)  |  kid;
}

static inline void decode_handle(int handle, int *line_p, int *kid_p)
{
    *line_p = handle >> 16;
    *kid_p  = handle & 63;
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

#define DECODE_AND_CHECK(errret)                                        \
    do {                                                                \
        decode_handle(handle, &line, &kid);                             \
        if (line > countof(lines)  ||  lines[line].fd < 0)              \
        {                                                               \
            DoDriverLog(DEVID_NOT_IN_DRIVER, 0,                         \
                        "%s: invalid handle %d (=>%d:%d)",              \
                        __FUNCTION__, handle, line, kid);               \
            return errret;                                              \
        }                                                               \
        dp = lines[line].devs + kid;                                    \
        if (dp->devid < 0)                                              \
        {                                                               \
            DoDriverLog(DEVID_NOT_IN_DRIVER, 0,                         \
                        "%s: attempt to use unregistered dev=%d:%d",    \
                        __FUNCTION__, line, kid);                       \
            return errret;                                              \
        }                                                               \
    } while (0)


static inline int  encode_frameid(int kid, int prio)
{
    return (kid << 2) | (prio << 8);
}

static inline void decode_frameid(int frameid, int *kid_p, int *prio_p)
{
    *kid_p  = (frameid >> 2) & 63;
    *prio_p = (frameid >> 8) & 7;
}

static void log_frame(int devid, const char *pfx,
                      int id, int len, uint8 *data)
{
  int   x;
  int   kid;
  int   prio;
  int   desc;
  char  data_s[100];
  char *p;

    if (!log_frame_allowed) return;

    decode_frameid(id, &kid, &prio);
    desc = len > 0? data[0] : -1;
    
    for (x = 0, p = data_s;  x < len;  x++)
        p += sprintf(p, "%s%02X", x == 0? "" : ",", data[x]);
  
    DoDriverLog(devid, 0 | DRIVERLOG_C_PKTDUMP,
                "%s id=%u, kid=%d, prio=%d, desc=0x%02X data[%d]={%s}",
                pfx, id, kid, prio, desc, len, data_s);
}

//////////////////////////////////////////////////////////////////////

#ifndef CANHAL_FILE_H
  #error The "CANHAL_FILE_H" macro is undefined
#else
  #include CANHAL_FILE_H
#endif

static int SendFrame(int devid, lineinfo_t *lp, int kid,
                     int prio, int dlc, uint8 *data)
{
  int   r;
  int   errflg;
  char *errstr;

    r = canhal_send_frame(lp->fd, encode_frameid(kid, prio), dlc, data);
    log_frame(DEVID_NOT_IN_DRIVER*0/*06.08.2013*/+devid, "SNDFRAME", encode_frameid(kid, prio), dlc, data);
    
    if (r != CANHAL_OK  &&  r != lp->last_wr_r)
    {
        errflg = 0;
        errstr = "";
        if      (r == CANHAL_ZERO)   errstr = ": ZERO";
        else if (r == CANHAL_BUSOFF) errstr = ": BUSOFF";
        else                         errflg = DRIVERLOG_ERRNO;
        
        DoDriverLog(devid, 0 | errflg, "%s(%d:k%d/%d, dlc=%d, cmd=0x%02x)%s",
                    __FUNCTION__, (int)(lp-lines), kid, prio, dlc,
                    dlc > 0? data[0] : 0xFFFFFFFF,
                    errstr);
    }

    lp->last_wr_r = r;

    return r;
}

//////////////////////////////////////////////////////////////////////

/* Forward declarations */

static void cankoz_fd_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr);

static void perform_sendnext(int line, int kid, kozdevinfo_t *dp);
static void         cankoz_q_clear   (int handle);
static qfe_status_t cankoz_q_enqueue (int handle,
                                      canqelem_t *item, qfe_enqcond_t how);
static qfe_status_t cankoz_q_foreach (int handle,
                                      queue_checker_t checker, void *ptr,
                                      qfe_action_t action);
static qfe_status_t cankoz_q_erase_and_send_next(int handle,
                                                 int datasize, ...);


/* Initialization staff */

static int init_l(void)
{
  int  l;
  int  d;

    if (!layer_initialized)
    {
        bzero(lines, sizeof(lines));
        for (l = 0;  l < countof(lines);  l++)
        {
            lines[l].fd = -1;
            for (d = 0;  d < countof(lines[l].devs);  d++)
                lines[l].devs[d].devid = -1;
        }
    }

    layer_initialized = 1;
    
    return 0;
}

static int  cankoz_add(int devid, void *devptr,
                       int businfocount, int businfo[],
                       int devcode,
                       CanKozOnIdProc  rstproc,
                       CanKozPktinProc inproc,
                       int queue_size, int regs_base)
{
  int           line    = businfo[0];
  int           kid     = businfo[1];
  int           speed_n = businfo[2];
  
  lineinfo_t   *lp;
  kozdevinfo_t *dp;
  int           handle;

  uint8         data[8];
  canqelem_t    item;
  char         *err;

    init_l();
  
    if (line < 0  ||  line >= NUMLINES)
    {
        DoDriverLog(devid, 0, "%s: line=%d, out_of[%d..%d)",
                    __FUNCTION__, line, 0, NUMLINES);
        return -CXRF_CFG_PROBL;
    }
    if (kid < 0  ||  kid >= DEVSPERLINE)
    {
        DoDriverLog(devid, 0, "%s: kid=%d, out_of[%d..%d)",
                    __FUNCTION__, kid,  0, DEVSPERLINE);
        return -CXRF_CFG_PROBL;
    }

    lp     = lines    + line;
    dp     = lp->devs + kid;
    handle = encode_handle(line, kid);

    if (lp->fd < 0)
    {
        err = NULL;
        lp->fd = canhal_open_and_setup_line(line, speed_n, &err);
        if (lp->fd < 0)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO,
                        "%s: canhal_open_and_setup_line(): %s",
                        __FUNCTION__, err);
            return -CXRF_DRV_PROBL;
        }
        lp->last_rd_r = lp->last_wr_r = CANHAL_OK;
        lp->last_ex_id = 0;
        lp->fdhandle = sl_add_fd(DEVID_NOT_IN_DRIVER/*!!!*/, NULL,
                                 lp->fd, SL_RD,
                                 cankoz_fd_p, lint2ptr(line));
        
        data[0] = CANKOZ_DESC_GETDEVATTRS;
        SendFrame(devid, lp, 0, CANKOZ_PRIO_BROADCAST, 1, data);
    }

    if (dp->devid >= 0)
    {
        DoDriverLog(devid, 0, "%s: device (%d,%d) is already in use",
                   __FUNCTION__, line, kid);
        ReturnChanGroup(dp->devid, 0, RETURNCHAN_COUNT_PONG, NULL, NULL);
        return -CXRF_CFG_PROBL;
    }

    dp->q_tid = -1;

    dp->ring = NULL;
    if (queue_size != 0)
        if ((dp->ring = malloc(sizeof(canqelem_t) * queue_size)) == NULL) return -1;
    dp->ring_size = queue_size;
    dp->ring_used = 0;

    dp->devid   = devid;
    dp->devptr  = devptr;
    dp->devcode = devcode;
    dp->rst     = rstproc;
    dp->cb      = inproc;
    dp->ioregs.base = regs_base;

    dp->hw_ver = dp->sw_ver = dp->hw_code = 0;

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 1;
    item.data[0]  = CANKOZ_DESC_GETDEVATTRS;
    cankoz_q_enqueue(handle, &item, QFE_IF_ABSENT);

    return handle;
}

static void cankoz_disconnect(int devid)
{
  int  l;
  int  d;

    if (devid < 0) return;
    
    for (l = 0;  l < countof(lines);  l++)
        for (d = 0;  d < countof(lines[l].devs);  d++)
        {
            if (lines[l].devs[d].devid == devid)
            {
                safe_free(lines[l].devs[d].ring);
                bzero(&(lines[l].devs[d]), sizeof(lines[l].devs[d]));
                lines[l].devs[d].devid = -1;

                /* Check if some other devices are active on this line */
                for (d = 0;  d < countof(lines[l].devs);  d++)
                    if (lines[l].devs[d].devid >= 0)
                        goto NEXT_KID;

                /* ...or was last "client" of this line? */
                /* Then release the line! */
                sl_del_fd(lines[l].fdhandle);
                canhal_close_line(lines[l].fd);
                lines[l].fd = -1;
            }
        NEXT_KID:;
        }
}

static int cankoz_get_dev_ver(int handle, int *hw_ver_p, int *sw_ver_p, int *hw_code_p)
{
  int           line;
  int           kid;
  kozdevinfo_t *dp;

    DECODE_AND_CHECK(-1);
    if (hw_ver_p  != NULL) *hw_ver_p  = dp->hw_ver;
    if (sw_ver_p  != NULL) *sw_ver_p  = dp->sw_ver;
    if (hw_code_p != NULL) *hw_code_p = dp->hw_code;
    return 0;
}

static void send_wrreg_cmd(kozdevinfo_t *dp, int line, int kid)
{
  uint8  data[8];
    
    data[0] = CANKOZ_DESC_WRITEOUTREG;
    data[1] = (dp->ioregs.cur_val &~ dp->ioregs.req_msk) |
              (dp->ioregs.req_val &  dp->ioregs.req_msk);
    SendFrame(dp->devid, lines + line, kid,
              CANKOZ_PRIO_UNICAST, 2, data);

    //dp->ioregs.req_msk = 0;
    dp->ioregs.pend    = 0;
}


static void cankoz_fd_p(int devid, void *devptr,
                        sl_fdh_t fdhandle, int fd, int mask,
                        void *privptr)
{
  int           line = ptr2lint(privptr);
  lineinfo_t   *lp   = lines + line;
  int           handle;

  int           repcount;

  int           r;
  int           last_rd_r;
  int           can_id;
  int           can_dlc;
  uint8         can_data[8];

  int           errflg;
  char         *errstr;

  int           kid;
  int           prio;
  int           desc;
  kozdevinfo_t *dp;

  int           id_devcode;
  int           id_hwver;
  int           id_swver;
  int           id_reason;
  int           is_a_reset;

  int32         regsvals  [18];
  rflags_t      regsrflags[18];
  int           x;
  canqelem_t    item;

    repcount = 30;

 REPEAT:
    repcount--;

    /* Obtain a frame */
    r = canhal_recv_frame(fd, &can_id, &can_dlc, can_data);
    last_rd_r = lp->last_rd_r;
    lp->last_rd_r = r;
    handle = -1;

    /* Check result */
    if (r != CANHAL_OK)
    {
        if (r != last_rd_r)
        {
            errflg = 0;
            errstr = "";
            if      (r == CANHAL_ZERO)   errstr = ": ZERO";
            else if (r == CANHAL_BUSOFF) errstr = ": BUSOFF";
            else                         errflg = DRIVERLOG_ERRNO;
            
            DoDriverLog(DEVID_NOT_IN_DRIVER, 0 | errflg,
                        "%s()%s", __FUNCTION__, errstr);
        }
        return;
    }

    log_frame(DEVID_NOT_IN_DRIVER, "RCVFRAME", can_id, can_dlc, can_data);

    /* An extended-id/RTR/other-non-kozak frame? */
    if (can_id != (can_id & 0x7FF))
    {
        if (can_id != lp->last_ex_id)
        {
            lp->last_ex_id = can_id;
            /*!!! DriverLog() it somehow? */
        }

        return;
    }

    /* Okay, that's a valid frame */
    decode_frameid(can_id, &kid, &prio);
    desc = can_dlc > 0? can_data[0] : -1;

    /* We don't want to deal with not-from-devices frames */
    if (prio != CANKOZ_PRIO_REPLY) return;
    
    dp     = lp->devs + kid;
    handle = encode_handle(line, kid);

    /* Perform additional (common) actions for GETDEVATTRS */
    if (desc == CANKOZ_DESC_GETDEVATTRS)
    {
        /* Log information */
        id_devcode = can_dlc > 1? can_data[1] : -1;
        id_hwver   = can_dlc > 2? can_data[2] : -1;
        id_swver   = can_dlc > 3? can_data[3] : -1;
        id_reason  = can_dlc > 4? can_data[4] : -1;
        ////DoDriverLog(dp->devid >= 0? dp->devid : DEVID_NOT_IN_DRIVER, 0,
        ////            "pkt: len=%d data={%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x", can_dlc, can_data[0], can_data[1], can_data[2], can_data[3], can_data[4], can_data[5], can_data[6], can_data[7]);
        log_frame(dp->devid >= 0? dp->devid : DEVID_NOT_IN_DRIVER, "GDA", can_id, can_dlc, can_data);
        DoDriverLog(dp->devid >= 0? dp->devid : DEVID_NOT_IN_DRIVER, 0,
                    "GETDEVATTRS: ID=%d,%d, DevCode=%d:%s, HWver=%d, SWver=%d, Reason=%d:%s",
                    line, kid,
                    id_devcode,
                    id_devcode >= 0  &&  id_devcode < countof(cankoz_devtypes)  &&  cankoz_devtypes[id_devcode] != NULL?
                        cankoz_devtypes[id_devcode] : "???",
                    id_hwver,
                    id_swver,
                    id_reason,
                    id_reason  >= 0  &&  id_reason  < countof(cankoz_GETDEVATTRS_reasons)?
                        cankoz_GETDEVATTRS_reasons[id_reason] : "???");

        dp->hw_ver  = id_hwver;
        dp->sw_ver  = id_swver;
        dp->hw_code = id_devcode;
        
        /* Is it our device? */
        if (dp->devid >= 0)
        {
            cankoz_q_erase_and_send_next(handle, 1, CANKOZ_DESC_GETDEVATTRS);

            /* Is code correct? */
            if(dp->devcode >= 0  &&  dp->devcode != id_devcode)
            {
                DoDriverLog(DEVID_NOT_IN_DRIVER, 0,
                            "%s: DevCode=%d differs from registered %d. Terminating device.",
                            __FUNCTION__, id_devcode, dp->devcode);
                SetDevState(dp->devid, DEVSTATE_OFFLINE, CXRF_WRONG_DEV, "wrong device");
                return;
            }

            is_a_reset =
                id_reason == CANKOZ_IAMR_POWERON   ||
                id_reason == CANKOZ_IAMR_RESET     ||
                id_reason == CANKOZ_IAMR_WATCHDOG  ||
                id_reason == CANKOZ_IAMR_BUSOFF;
            
            if (is_a_reset)
            {
                cankoz_q_clear(handle);
            }

            if (dp->rst != NULL  &&  id_reason != CANKOZ_IAMR_WHOAREHERE)
                dp->rst(dp->devid, dp->devptr, is_a_reset);

            /* Should we perform re-request? */
            if (is_a_reset)
            {
                dp->ioregs.rcvd       = 0;
                dp->ioregs.pend       = 0;
                dp->ioregs.req_msk = 0;
                SetDevState(dp->devid, DEVSTATE_NOTREADY,  0, NULL);
                SetDevState(dp->devid, DEVSTATE_OPERATING, 0, NULL);
            }

            /* Do we need to send a write request? */
        }
    }

    /* If this device isn't registered, we aren't interested in further processing */
    if (dp->devid < 0) return;

    /* Ioregs operation? */
    if (desc == CANKOZ_DESC_READREGS  &&  dp->ioregs.base > 0)
    {
        if (can_dlc < 3)
        {
            DoDriverLog(dp->devid, 0,
                        "DESC_READREGS: packet is too short - %d bytes",
                        can_dlc);
            return;
        }

        /* Erase the requestor */
        cankoz_q_erase_and_send_next(handle, 1, CANKOZ_DESC_READREGS);
        
        /* Extract data into a ready-for-ReturnChanGroup format */
        regsvals[CANKOZ_REGS_WR1_OFS] = dp->ioregs.cur_val = can_data[1];
        regsvals[CANKOZ_REGS_RD1_OFS] = dp->ioregs.cur_inp = can_data[2];
        for (x = 0;  x < 8;  x++)
        {
            regsvals[CANKOZ_REGS_WR8_BOFS + x] = (regsvals[CANKOZ_REGS_WR1_OFS] & (1 << x)) != 0;
            regsvals[CANKOZ_REGS_RD8_BOFS + x] = (regsvals[CANKOZ_REGS_RD1_OFS] & (1 << x)) != 0;
        }
        dp->ioregs.rcvd = 1;

        /* Do we have a pending write request? */
        if (dp->ioregs.pend != 0)
        {
            send_wrreg_cmd(dp, line, kid);
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = CANKOZ_DESC_READREGS;
            cankoz_q_enqueue(handle, &item, QFE_IF_NONEORFIRST);
        }

        /* And return requested data */
        bzero(regsrflags, sizeof(regsrflags));
        ReturnChanGroup(dp->devid, dp->ioregs.base, 18, regsvals, regsrflags);
    }
    
    /* Finally, call the processer... */
    if (dp->cb != NULL)
        dp->cb(dp->devid, dp->devptr,
               desc, can_dlc, can_data);

    if (repcount > 0)
    {
        r = check_fd_state(fd, O_RDONLY);
        if (r > 0) goto REPEAT;
    }
}


static int elem_is_equal(canqelem_t *elem, void *ptr)
{
  canqelem_t *model = (canqelem_t *)ptr;
  
    if (model->datasize >= 0)
        return
            elem->prio     == model->prio      &&
            elem->datasize == model->datasize  &&
            (elem->datasize == 0  ||
             memcmp(elem->data, model->data, elem->datasize) == 0);
    else
        return
            elem->prio     ==  model->prio      &&
            elem->datasize >= -model->datasize  &&
            memcmp(elem->data, model->data, -model->datasize) == 0;
}

static int cankoz_sendframe(int handle, int prio,
                            int datasize, uint8 *data)
{
  int           line;
  int           kid;
  kozdevinfo_t *dp;
  
    DECODE_AND_CHECK(-1);

    SendFrame(dp->devid, lines + line, kid,
              prio, datasize, data);

    return 0;
}

static void cankoz_regs_rw(int handle, int action, int chan, int32 *wvp)
{
  int           line;
  int           kid;
  kozdevinfo_t *dp;

  canqelem_t    item;
  uint8         mask;

    DECODE_AND_CHECK();

//    return;
    
    chan -= dp->ioregs.base;

    /* Prepare the READREGS packet -- it will be required in all cases */
    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 1;
    item.data[0]  = CANKOZ_DESC_READREGS;
    
    /* Read? */
    if (action == DRVA_READ   ||
        chan == CANKOZ_REGS_RD1_OFS  ||
        (chan >= CANKOZ_REGS_RD8_BOFS  &&  chan <= CANKOZ_REGS_RD8_BOFS + 7))
    {
        cankoz_q_enqueue(handle, &item, QFE_IF_ABSENT);
    }
    /* No, some form of write */
    else
    {
        /* Decide, what to write... */
        if (chan == CANKOZ_REGS_WR1_OFS)
        {
            dp->ioregs.req_val = *wvp;
            dp->ioregs.req_msk = 0xFF;
        }
        else
        {
            mask = 1 << (chan - CANKOZ_REGS_WR8_BOFS);
            if (*wvp != 0) dp->ioregs.req_val |=  mask;
            else           dp->ioregs.req_val &=~ mask;
            dp->ioregs.req_msk |= mask;
        }

        dp->ioregs.pend = 1;
        /* May we perform write right now? */
        if (dp->ioregs.rcvd)
        {
            send_wrreg_cmd(dp, line, kid);
            cankoz_q_enqueue(handle, &item, QFE_IF_NONEORFIRST);
        }
        /* No, we should request read first... */
        else
        {
            cankoz_q_enqueue(handle, &item, QFE_IF_ABSENT);
        }
    }
}

static void cankoz_q_clear(int handle)
{
  int           line;
  int           kid;
  kozdevinfo_t *dp;

    DECODE_AND_CHECK();

    if (dp->q_tid >= 0)
    {
        sl_deq_tout(dp->q_tid);
        dp->q_tid = -1;
    }
    dp->ring_used = 0;
}

static qfe_status_t _cankoz_q_enqueue_s(int handle,
                                        canqelem_t *item, qfe_enqcond_t how,
                                        int oneshot)
{
  int           line;
  int           kid;
  kozdevinfo_t *dp;
  canqelem_t   *place;
  
    DECODE_AND_CHECK(QFE_ERROR);
    
    if (
        (how == QFE_IF_ABSENT       &&
         cankoz_q_foreach(handle, elem_is_equal, item,
                          QFE_FIND_FIRST) != QFE_NOTFOUND)
        ||
        (how == QFE_IF_NONEORFIRST  &&
         cankoz_q_foreach(handle, elem_is_equal, item,
                          QFE_FIND_LAST)  == QFE_ISFIRST*0+QFE_FOUND) /*!!! Maybe QFE_FOUND? */
       ) return QFE_FOUND;
    
    if (dp->ring_used >= dp->ring_size)
    {
        DoDriverLog(dp->devid, 0, "%s(%d:%d): attempt to add to full queue (prio=%d, [0]=0x%02x)",
                    __FUNCTION__, line, kid, item->prio, item->data[0]);
        return QFE_ERROR;
    }
    
    place = dp->ring + ((dp->ring_start + dp->ring_used) % dp->ring_size);
    *place = *item;
    place->oneshot = oneshot;
    dp->ring_used++;

    if (dp->ring_used == 1) perform_sendnext(line, kid, dp);

    return QFE_NOTFOUND;
}

static qfe_status_t cankoz_q_enqueue(int handle,
                                     canqelem_t *item, qfe_enqcond_t how)
{
    return _cankoz_q_enqueue_s(handle, item, how, 0);
}

static qfe_status_t cankoz_q_enq_ons(int handle,
                                     canqelem_t *item, qfe_enqcond_t how)
{
    return _cankoz_q_enqueue_s(handle, item, how, 1);
}

static void handle_timeout(int devid, void *devptr,
                           sl_tid_t tid,
                           void *privptr);

static void perform_sendnext(int line, int kid, kozdevinfo_t *dp)
{
  int           r;
  canqelem_t   *item;

    while (dp->ring_used != 0)
    {
        item = dp->ring + dp->ring_start;

        /* Try to send the frame */
        r = SendFrame(dp->devid, lines + line, kid,
                      item->prio, item->datasize, item->data);

        /* Set timeout if (a) error or (b) a reply-expected frame */
        if (r != CANHAL_OK  ||  !(item->oneshot))
        {
            if (dp->q_tid >= 0)
            {
                sl_deq_tout(dp->q_tid);
                dp->q_tid = -1;
            }
            dp->q_tid = sl_enq_tout_after(dp->devid, NULL,
                                          100*1000 /*!!!*/,
                                          handle_timeout,
                                          lint2ptr(encode_handle(line, kid)));
        }

        /* In case of error -- stop sending, leave it for future */
        if (r != CANHAL_OK) return;

        /* If oneshot -- dequeue frame and continue, otherwise -- stop for now */
        if (item->oneshot)
        {
            dp->ring_start = (dp->ring_start + 1) % dp->ring_size;
            dp->ring_used--;
        }
        else
            break;
    }
}

static void handle_timeout(int devid, void *devptr,
                           sl_tid_t tid,
                           void *privptr)
{
  int           handle = ptr2lint(privptr);
  int           line;
  int           kid;
  kozdevinfo_t *dp;

    DECODE_AND_CHECK();

    dp->q_tid = -1;

    if (dp->ring_used == 0) return;

    perform_sendnext(line, kid, dp);
}

static qfe_status_t cankoz_q_foreach (int handle,
                                      queue_checker_t checker, void *ptr,
                                      qfe_action_t action)
{
  int           line;
  int           kid;
  kozdevinfo_t *dp;
  int           i;
  int           j;
  int           last_found = -1;
  
    DECODE_AND_CHECK(QFE_ERROR);

    if (checker == QFE_ELEM_IS_EQUAL) checker = elem_is_equal;
    
    for (i = 0;  i < dp->ring_used;  i++)
    {
        if (checker(dp->ring + ((dp->ring_start + i) % dp->ring_size), ptr))
        {
            last_found = i;
            if (action == QFE_FIND_FIRST) return i == 0? QFE_ISFIRST : QFE_FOUND;
            if (action == QFE_FIND_LAST)  goto CONTINUE_SEARCH;

            /* Should we erase at begin of queue? */
            if (i == 0)
            {
                if (dp->q_tid >= 0)
                {
                    sl_deq_tout(dp->q_tid);
                    dp->q_tid = -1;
                }
                dp->ring_start = (dp->ring_start + 1) % dp->ring_size;
            }
            /* No, we must do a clever "ringular" squeeze */
            else
                for (j = i;  j < dp->ring_used - 1;  j++)
                    memcpy(dp->ring + ((dp->ring_start + j)     % dp->ring_size),
                           dp->ring + ((dp->ring_start + j + 1) % dp->ring_size),
                           sizeof(canqelem_t));

            dp->ring_used--;

            if (action == QFE_ERASE_FIRST) return i == 0? QFE_ISFIRST : QFE_FOUND;
            i--; // To neutralize loop's "i++" in order to stay at the same position
        }
 CONTINUE_SEARCH:;
    }

    if (last_found == 0) return QFE_ISFIRST;
    if (last_found >  0) return QFE_FOUND;
    return QFE_NOTFOUND;
}

static qfe_status_t cankoz_q_erase_and_send_next(int handle,
                                                 int datasize, ...)
{
  int           line;
  int           kid;
  kozdevinfo_t *dp;
  int           abs_datasize = abs(datasize);
  canqelem_t    item;
  int           x;
  va_list       ap;
  qfe_status_t  r;

    DECODE_AND_CHECK(QFE_ERROR);

    if (abs_datasize < 1  ||  abs_datasize > 8) return QFE_ERROR;
    
    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = datasize;
    va_start(ap, datasize);
    for (x = 0;  x < abs_datasize;  x++) item.data[x] = va_arg(ap, int);
    va_end(ap);
#if 1 /* New-style (20.04.2011) check "whether we should really erase and send next packet" */
    if (dp->ring_used != 0  &&
        elem_is_equal(dp->ring + dp->ring_start, &item) == 0)
        return QFE_NOTFOUND;
#endif
    r = cankoz_q_foreach(handle, QFE_ELEM_IS_EQUAL, &item, QFE_ERASE_FIRST);
    if (r != QFE_ISFIRST)
    {
        /* Okay -- and what to do? :-) */
    }
    
    perform_sendnext(line, kid, dp);

    return QFE_FOUND;
}


/* Interface staff */

static cankoz_liface_t iface =
{
    cankoz_disconnect,
    cankoz_add,
    cankoz_get_dev_ver,
    cankoz_sendframe,
    cankoz_regs_rw,
    
    cankoz_q_clear,
    cankoz_q_enqueue,
    cankoz_q_enq_ons,
    cankoz_q_foreach,
    cankoz_q_erase_and_send_next,
};

cankoz_liface_t *CanKozGetIface(void)
{
    return &iface;
}

void             CanKozSetLogTo(int onoff)
{
    log_frame_allowed = (onoff != 0);
}

void             CanKozSend0xFF(void)
{
  int           line;
  uint8         data[8];
  
    init_l();

    for (line = 0;  line < NUMLINES;  line++)
        if (lines[line].fd >= 0)
        {
            data[0] = CANKOZ_DESC_GETDEVATTRS;
            SendFrame(DEVID_NOT_IN_DRIVER, lines + line, 0,
                      CANKOZ_PRIO_BROADCAST, 1, data);
        }
}

int              CanKozGetDevInfo(int devid,
                                  int *line_p, int *kid_p, int *devcode_p)
{
  int           handle;
  int           line;
  int           kid;
  kozdevinfo_t *dp;
  
    handle = devid2handle(devid);
    if (handle < 0) return handle;
    DECODE_AND_CHECK(-1);
    if (line_p    != NULL) *line_p    = line;
    if (kid_p     != NULL) *kid_p     = kid;
    if (devcode_p != NULL) *devcode_p = dp->devcode;

    return 0;
}
