#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
/*!!!*/#include <netinet/tcp.h>

#include "cx_sysdeps.h"
#include "misclib.h"

#include "remdrv_proto.h"
#define __REMCXSD_DRIVER_V2_C
#include "remcxsd.h"


#define CHECK_LOG_CORRECT(condition, correction, format, params...)    \
    if (condition)                                                     \
    {                                                                  \
        DoDriverLog(devid, 0, "%s():" format, __FUNCTION__, ##params); \
        correction;                                                    \
    }

#define DO_CHECK_SANITY_OF_DEVID(func_name, errret)                    \
    do {                                                               \
        if (devid < 0  ||  devid >= remcxsd_maxdevs)                   \
        {                                                              \
            DoDriverLog(DEVID_NOT_IN_DRIVER, 0,                        \
                        "%s(): devid=%d(active=%d), out_of[%d...%d]",  \
                        func_name, devid, active_devid, 0, remcxsd_maxdevs - 1);  \
            return errret;                                             \
        }                                                              \
        if (dev->in_use == 0)                                          \
        {                                                              \
            DoDriverLog(DEVID_NOT_IN_DRIVER, 0,                        \
                        "%s(): devid=%d(active=%d) is inactive",       \
                        func_name, devid, active_devid);               \
            return errret;                                             \
        }                                                              \
    } while (0)

#define CHECK_SANITY_OF_DEVID(errret) \
    DO_CHECK_SANITY_OF_DEVID(__FUNCTION__, errret)

//////////////////////////////////////////////////////////////////////

void remcxsd_report_to_fd (int fd,    int code,            const char *format, ...)
{
  struct {remdrv_pkt_header_t hdr;  int32 data[1000];} pkt;
  int      len;
  va_list  ap;

    va_start(ap, format);
    len = vsprintf((char *)(pkt.data), format, ap);
    va_end(ap);

    bzero(&pkt.hdr, sizeof(pkt.hdr));
    pkt.hdr.pktsize = sizeof(pkt.hdr) + len + 1;
    pkt.hdr.command = code;

    write(fd, &pkt, pkt.hdr.pktsize);
}

static int vReportThat    (int devid, int code, int level, const char *format, va_list ap)
{
  remcxsd_dev_t  *dev = remcxsd_devices + devid;
  struct {remdrv_pkt_header_t hdr;  int32 data[1000];} pkt;
  int        len;

    len = vsprintf((char *)(pkt.data), format, ap);

    /*!!! Should support _ERRNO flag too! */
    
    bzero(&pkt.hdr, sizeof(pkt.hdr));
    pkt.hdr.pktsize = sizeof(pkt.hdr) + len + 1;
    pkt.hdr.command = code;
    pkt.hdr.var.debugp.level = level &~ DRIVERLOG_ERRNO;

    ////fprintf(stderr, " vRT: %s\n", (char *)pkt.data);
    return fdio_send(dev->fhandle, &pkt, pkt.hdr.pktsize);
}

int  remcxsd_report_to_dev(int devid, int code, int level, const char *format, ...)
{
  va_list    ap;
  int        r;

    va_start(ap, format);
    r = vReportThat(devid, code, level, format, ap);
    va_end(ap);

    return r;
}

//////////////////////////////////////////////////////////////////////

void DoDriverLog (int devid, int level, const char *format, ...)
{
  va_list        ap;
  
    va_start(ap, format);
    vDoDriverLog(devid, level, format, ap);
    va_end(ap);
}

void vDoDriverLog(int devid, int level, const char *format, va_list ap)
{
  int             category = level & DRIVERLOG_C_mask;
  remcxsd_dev_t  *dev = remcxsd_devices + devid;

  static int  last_good_devid = -1;

    /* Check the devid first... */
    /* a. NOT_IN_DRIVER */
    if (devid == DEVID_NOT_IN_DRIVER)
    {
        devid = last_good_devid;
        dev    = remcxsd_devices + devid;

        if (devid < 0  ||  devid >= remcxsd_maxdevs  ||
            dev->in_use == 0)
//            (dev->in_use == 0 && (fprintf(stderr, "[%d].in_use=0\n", devid), 1)))
        {
            vfprintf(stderr, format, ap);
            fprintf(stderr, "\n");
            return;
        }
    }
    /* b. some strange/unknown one */
    if (devid < 0  ||  devid >= remcxsd_maxdevs  ||
        dev->in_use == 0)
    {
        remcxsd_debug("%s: message from unknown devid=%d: \"%s\"",
                      __FUNCTION__, devid, format);
    }
    /* c. okay */
    else
    {
        last_good_devid = devid;
        
        /* Check current level/mask settings */
        if (category != DRIVERLOG_C_DEFAULT  &&
            !DRIVERLOG_m_C_ISIN_CHECKMASK(category, dev->logmask))
            return;
        if ((level & DRIVERLOG_LEVEL_mask) > dev->loglevel)
            return;
        
        vReportThat(devid, REMDRVP_DEBUGP, level, format, ap);
    }
}

void ReturnChanGroup(int devid, int  start, int count, int32 *values, rflags_t *rflags)
{
  remcxsd_dev_t       *dev = remcxsd_devices + devid;
  size_t               pktsize;
  remdrv_pkt_header_t  hdr;
    
    CHECK_SANITY_OF_DEVID();

    if (count == RETURNCHAN_COUNT_PONG)
    {
        pktsize = sizeof(hdr);
        bzero(&hdr, sizeof(hdr));
        hdr.pktsize      = pktsize;
        hdr.command      = REMDRVP_PING;
        if (fdio_send(dev->fhandle, &hdr, pktsize) < 0)
            FreeDevID(devid);
        return;
    }

    CHECK_LOG_CORRECT(count < 1,
                      return,
                      "count=%d", count);
    
    pktsize = sizeof(remdrv_pkt_header_t) + count * sizeof(int32) * 2;
    /*!!! Any checks? */
  
    bzero(&hdr, sizeof(hdr));
    hdr.pktsize      = pktsize;
    hdr.command      = REMDRVP_READ;
    hdr.var.rw.start = start;
    hdr.var.rw.count = count;

    ////fprintf(stderr, "%s(%d, %d, %d)\n", __FUNCTION__, devid, start,count);
    if (fdio_lock_send  (dev->fhandle)                         < 0  ||
        fdio_send(dev->fhandle, &hdr,   sizeof(hdr))           < 0  ||
        fdio_send(dev->fhandle, values, count * sizeof(int32)) < 0  ||
        fdio_send(dev->fhandle, rflags, count * sizeof(int32)) < 0  ||
        fdio_unlock_send(dev->fhandle)                         < 0)
        FreeDevID(devid);
}

void  ReturnChanSet  (int devid, int *addrs, int count, int32 *values, rflags_t *rflags)
{
  remcxsd_dev_t       *dev = remcxsd_devices + devid;
  size_t               pktsize;
  remdrv_pkt_header_t  hdr;

  int                  x;
  enum {MAX_CHANS_PER_RCS = 100};
  int32                ad2snd[MAX_CHANS_PER_RCS];

    CHECK_SANITY_OF_DEVID();

    CHECK_LOG_CORRECT(count < 1  ||  count > MAX_CHANS_PER_RCS,
                      return,
                      "count=%d, out_of[1...%d]", count, MAX_CHANS_PER_RCS);

    pktsize = sizeof(remdrv_pkt_header_t) + count * sizeof(int32) * 3;
    /*!!! Any checks? */

    for (x = 0;  x < count;  x++) ad2snd[x] = addrs[x];

    bzero(&hdr, sizeof(hdr));
    hdr.pktsize      = pktsize;
    hdr.command      = REMDRVP_RCSET;
    hdr.var.rw.start = 0;
    hdr.var.rw.count = count;

    if (fdio_lock_send  (dev->fhandle)                         < 0  ||
        fdio_send(dev->fhandle, &hdr,   sizeof(hdr))           < 0  ||
        fdio_send(dev->fhandle, ad2snd, count * sizeof(int32)) < 0  ||
        fdio_send(dev->fhandle, values, count * sizeof(int32)) < 0  ||
        fdio_send(dev->fhandle, rflags, count * sizeof(int32)) < 0  ||
        fdio_unlock_send(dev->fhandle)                         < 0)
        FreeDevID(devid);
}

void  ReturnBigc     (int devid, int  bigchan, int32 *info, int ninfo, void *retdata, size_t retdatasize, size_t retdataunits, rflags_t rflags)
{
  remcxsd_dev_t       *dev = remcxsd_devices + devid;
  size_t               pktsize;
  remdrv_pkt_header_t  hdr;

    CHECK_SANITY_OF_DEVID();
  
    CHECK_LOG_CORRECT(ninfo < 0,
                      ninfo = 0,
                      "ninfo=%d, <0", ninfo);
    CHECK_LOG_CORRECT(ninfo > CX_MAX_BIGC_PARAMS,
                      ninfo = CX_MAX_BIGC_PARAMS,
                      "ninfo=%d, >MAXPARAMS=%d", ninfo, CX_MAX_BIGC_PARAMS);
  
    pktsize = sizeof(remdrv_pkt_header_t) +
              sizeof(int32) * ninfo + retdatasize;
    /*!!! Any checks? */
    
    bzero(&hdr, sizeof(hdr));
    hdr.pktsize            = pktsize;
    hdr.command            = REMDRVP_BIGC;
    hdr.var.bigc.bigchan   = bigchan;
    hdr.var.bigc.nparams   = ninfo;
    hdr.var.bigc.datasize  = retdatasize;
    hdr.var.bigc.dataunits = retdataunits;
    hdr.var.bigc.rflags    = rflags;

    if (fdio_lock_send  (dev->fhandle)                        < 0   ||
        (fdio_send(dev->fhandle, &hdr, sizeof(hdr))           < 0)  ||
        (ninfo       != 0  &&
         fdio_send(dev->fhandle, info, sizeof(int32) * ninfo) < 0)  ||
        (retdatasize != 0  &&
         fdio_send(dev->fhandle, retdata, retdatasize)        < 0)  ||
        fdio_unlock_send(dev->fhandle)                        < 0)
        FreeDevID(devid);
}

void  SetDevState     (int devid, int state,
                       rflags_t rflags_to_set, const char *description)
{
  remcxsd_dev_t       *dev = remcxsd_devices + devid;
  remdrv_pkt_header_t  pkt;
  int                  r;
  int                  nagle_off = 1;

    CHECK_SANITY_OF_DEVID();
  
    /* Turn off Nagle algorithm, for packet to be sent immediately,
       instead of being discarded upon subsequent close(). */
    if (state == DEVSTATE_OFFLINE)
        setsockopt(dev->s, SOL_TCP, TCP_NODELAY, &nagle_off, sizeof(nagle_off));

    bzero(&pkt, sizeof(pkt));
    pkt.pktsize = sizeof(pkt);
    pkt.command = REMDRVP_CHSTAT;
    pkt.var.chstat.state         = state;
    pkt.var.chstat.rflags_to_set = rflags_to_set;
    r = fdio_send(dev->fhandle, &pkt, pkt.pktsize);
    if (r < 0) FreeDevID(devid);
  
    if (state == DEVSTATE_OFFLINE) FreeDevID(devid);
}

void  ReRequestDevData(int devid)
{
  remcxsd_dev_t       *dev = remcxsd_devices + devid;
  remdrv_pkt_header_t  pkt;
  int                  r;

    CHECK_SANITY_OF_DEVID();
  
    bzero(&pkt, sizeof(pkt));
    pkt.pktsize = sizeof(pkt);
    pkt.command = REMDRVP_REREQD;
    r = fdio_send(dev->fhandle, &pkt, pkt.pktsize);
    if (r < 0) FreeDevID(devid);
}

//////////////////////////////////////////////////////////////////////

void ProcessPacket(int devid, void *devptr,
                   fdio_handle_t handle, int reason,
                   void *inpkt,  size_t inpktsize,
                   void *privptr)
{
  remcxsd_dev_t       *dev   = remcxsd_devices + devid;
  remdrv_pkt_header_t *hdr   = (remdrv_pkt_header_t *)inpkt;
  remdrv_pkt_header_t  crzbuf;
  int                  s_devid;
  int                  x;
  const char          *auxinfo;
  int                  state;

  char                 bus_id_str[countof(dev->businfo) * (12+2)];
  char                *bis_p;

    ////fprintf(stderr, "%s(%d): %s, p=%p size=%d, code=%08x\n", __FUNCTION__, handle, fdio_strreason(reason), inpkt, inpktsize, reason==FDIO_R_DATA?hdr->command:0);
  
    switch (reason)
    {
        case FDIO_R_DATA:
            break;

        case FDIO_R_INPKT2BIG:
            remcxsd_report_to_dev(devid, REMDRVP_RRUNDP, 0,
                                  "fdiolib: received packet is too big");
            FreeDevID(devid);
            return;

        case FDIO_R_ENOMEM:
            remcxsd_report_to_dev(devid, REMDRVP_RRUNDP, 0,
                                  "fdiolib: out of memory");
            FreeDevID(devid);
            return;
        
        case FDIO_R_CLOSED:
        case FDIO_R_IOERR:
        default:
            remcxsd_debug("%s(handle=%d, dev=%d): %s, errno=%d",
                          __FUNCTION__, handle, devid, fdio_strreason(reason), fdio_lasterr(handle));
            FreeDevID(devid);
            return;
    }

    if (hdr == NULL)
    {
        /*!!! ???*/

        return;
    }

    ////fprintf(stderr, "command=%08x\n", hdev->command);
    switch (hdr->command)
    {
        case REMDRVP_CONFIG:
            /* Check version compatibility */
            if (!CX_VERSION_IS_COMPATIBLE(hdr->var.config.proto_version,
                                          REMDRVP_VERSION))
            {
                bzero(&crzbuf, sizeof(crzbuf));
                crzbuf.pktsize = sizeof(remdrv_pkt_header_t);
                crzbuf.command = REMDRVP_CRAZY;
                n_write(fdio_fd_of(handle), &crzbuf, crzbuf.pktsize);

                FreeDevID(devid);
                return;
            }

            if (dev->inited)
            {
                DoDriverLog(devid, 0, "CONFIG command to an already configured driver");
                return;
            }

            dev->businfocount = hdr->var.config.businfocount;
            if (dev->businfocount > countof(dev->businfo))
                dev->businfocount = countof(dev->businfo);
            for (x = 0;  x < dev->businfocount; x++)
                dev->businfo[x] = hdr->data[x];

            /* Note: PACKET'S provided businfocount is used in calcs */
            auxinfo = hdr->pktsize > sizeof(remdrv_pkt_header_t) + sizeof(int32) * hdr->var.config.businfocount?
                      (char *)(hdr->data + hdr->var.config.businfocount) : NULL;

            if (dev->businfocount < dev->metric->min_businfo_n ||
                dev->businfocount > dev->metric->max_businfo_n)
            {
                DoDriverLog(devid, 0, "businfocount=%d, out_of[%d,%d]",
                            dev->businfocount,
                            dev->metric->min_businfo_n, dev->metric->max_businfo_n);
                SetDevState(devid, DEVSTATE_OFFLINE, CXRF_CFG_PROBL, "businfocount out of range");
                return;
            }

            /* Allocate privrec */
            if (dev->metric->privrecsize != 0)
            {
                if (devptr != NULL) // Static allocation
                    dev->devptr = devptr;
                else
                {
                    dev->devptr = malloc(dev->metric->privrecsize);
                    if (dev->devptr == NULL)
                    {
                        remcxsd_report_to_dev(devid, REMDRVP_RRUNDP, 0,
                                              "out of memory when allocating privrec");
                        FreeDevID(devid);
                        return;
                    }
                }
                bzero(dev->devptr, dev->metric->privrecsize);
                if (dev->metric->paramtable != NULL)
                {
                    if (psp_parse(auxinfo, NULL,
                                  dev->devptr,
                                  '=', " \t", "",
                                  dev->metric->paramtable) != PSP_R_OK)
                    {
                        DoDriverLog(devid, 0, "psp_parse(auxinfo)@remsrv: %s", psp_error());
                        SetDevState(devid, DEVSTATE_OFFLINE, CXRF_CFG_PROBL, "auxinfo parsing error");
                        return;
                    }
                }
            }

            /* Initialize driver */
            {
                if (dev->businfocount == 0)
                    sprintf(bus_id_str, "-");
                else
                    for (bis_p = bus_id_str, x = 0;  x < dev->businfocount;  x++)
                        bis_p += sprintf(bis_p, "%s%d", x > 0? "," : "", dev->businfo[x]);

                remcxsd_debug("[%d] CONFIG: pktsize=%d, businfo=%s, auxinfo=\"%s\"",
                              devid, hdr->pktsize, bus_id_str, auxinfo);
                //fprintf(stderr, "(");
                //for (x = 0; x < hdr->pktsize - sizeof(remdrv_pkt_header_t); x++)
                //    fprintf(stderr, "%s%02X", x == 0? "": " ", ((char *)(hdr->data))[x]);
                //fprintf(stderr, ")\n");
            }
            state = DEVSTATE_OPERATING;
            ENTER_DRIVER_S(devid, s_devid);
            if (dev->metric->init_dev != NULL)
                state = (dev->metric->init_dev)
                    (devid, dev->devptr,
                     dev->businfocount, dev->businfo,
                     auxinfo);
            dev->inited = 1;
            LEAVE_DRIVER_S(s_devid);

            if (state < 0)
            {
                SetDevState(devid, DEVSTATE_OFFLINE,
                            state == -1? 0 : -state, "init_dev failure");
            }
            else
            {
                SetDevState(devid, state, 0, NULL);
            }

            break;

        case REMDRVP_READ:
        case REMDRVP_WRITE:
            ENTER_DRIVER_S(devid, s_devid);
            if (dev->metric->do_rw != NULL)
                dev->metric->do_rw(devid, dev->devptr,
                                  hdr->var.rw.start,
                                  hdr->var.rw.count,
                                  hdr->command == REMDRVP_WRITE? hdr->data  : NULL,
                                  hdr->command == REMDRVP_WRITE? DRVA_WRITE : DRVA_READ);
            LEAVE_DRIVER_S(s_devid);
            break;

        case REMDRVP_BIGC:
            ENTER_DRIVER_S(devid, s_devid);
            if (dev->metric->do_big != NULL)
                dev->metric->do_big(devid, dev->devptr,
                                   hdr->var.bigc.bigchan,
                                   hdr->var.bigc.nparams  != 0? hdr->data + 0 : NULL,
                                   hdr->var.bigc.nparams,
                                   hdr->var.bigc.datasize != 0? hdr->data + hdr->var.bigc.nparams : NULL,
                                   hdr->var.bigc.datasize,
                                   hdr->var.bigc.dataunits);
            LEAVE_DRIVER_S(s_devid);
            break;

        case REMDRVP_SETDBG:
            dev->loglevel = hdr->var.setdbg.verblevel;
            dev->logmask  = hdr->var.setdbg.mask;
            break;

        case REMDRVP_PING:
            if (fdio_send(handle, inpkt, inpktsize) < 0) FreeDevID(devid);
            break;

        default:
            DoDriverLog(devid, 0, "%s: unknown command 0x%x",
                        __FUNCTION__, hdr->command);
            {
              int32 *p;
                for (x = 0, p = inpkt;  x < 8;  x++, p++)
                    fprintf(stderr, "%08x ", *p);
                fprintf(stderr, "\n");
            }
    }
}

int  cxsd_uniq_checker(const char *func_name, int uniq)
{
  int                  devid = uniq;
  remcxsd_dev_t       *dev = remcxsd_devices + devid;

    if (uniq == 0  ||  uniq == DEVID_NOT_IN_DRIVER) return 0;

    DO_CHECK_SANITY_OF_DEVID(func_name, -1);

    return 0;
}
