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

#include "cx_sysdeps.h"

#include "pzframe_drv.h"

#include "misc_macros.h"
#include "misclib.h"
#include "sendqlib.h"

#include "drv_i/ottcamx_drv_i.h"


//////////////////////////////////////////////////////////////////////
// Excerpt from cx/src/lib/cxlib/h2ii2h.h

#if BYTE_ORDER == LITTLE_ENDIAN

static inline uint32 h2i32   (uint32 hostint32)   {return hostint32;}
static inline uint32 h2i16   (uint16 hostint16)   {return hostint16;}
static inline uint32 i2h32   (uint32 intelint32)  {return intelint32;}
static inline uint16 i2h16   (uint16 intelint16)  {return intelint16;}
static inline int32  h2i32s  (int32  hostint32)   {return hostint32;}
static inline int32  h2i16s  (int16  hostint16)   {return hostint16;}
static inline int32  i2h32s  (int32  intelint32)  {return intelint32;}
static inline int16  i2h16s  (int16  intelint16)  {return intelint16;}

static inline void memcpy_h2i32(uint32 *dst, const uint32 *src, int count)
{ memcpy(dst, src, sizeof(int32) * count); }

static inline void memcpy_i2h32(uint32 *dst, const uint32 *src, int count)
{ memcpy(dst, src, sizeof(int32) * count); }

static inline void memcpy_h2i16(uint16 *dst, const uint16 *src, int count)
{ memcpy(dst, src, sizeof(int16) * count); }

static inline void memcpy_i2h16(uint16 *dst, const uint16 *src, int count)
{ memcpy(dst, src, sizeof(int16) * count); }

#elif BYTE_ORDER == BIG_ENDIAN

#define SwapBytes32(x) (			\
            (((x & 0x000000FF) << 24)) |	\
            (((x & 0x0000FF00) <<  8)) |	\
            (((x & 0x00FF0000) >>  8)) |	\
            (((x & 0xFF000000) >> 24) & 0xFF)	\
        )

#define SwapBytes16(x) (			\
            (((x & 0x00FF) << 8)) |		\
            (((x & 0xFF00) >> 8) & 0xFF)	\
        )

static inline uint32 h2i32(uint32 hostint32)  {return SwapBytes32(hostint32);}
static inline uint32 h2i16(uint16 hostint16)  {return SwapBytes16(hostint16);}
static inline uint32 i2h32(uint32 intelint32) {return SwapBytes32(intelint32);}
static inline uint16 i2h16(uint16 intelint16) {return SwapBytes16(intelint16);}
static inline int32  h2i32s(int32 hostint32)  {return SwapBytes32(hostint32);}
static inline int32  h2i16s(int16 hostint16)  {return SwapBytes16(hostint16);}
static inline int32  i2h32s(int32 intelint32) {return SwapBytes32(intelint32);}
static inline int16  i2h16s(int16 intelint16) {return SwapBytes16(intelint16);}

static inline void memcpy_h2i32(uint32 *dst, const uint32 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = h2i32(*src++);
}

static inline void memcpy_i2h32(uint32 *dst, const uint32 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = i2h32(*src++);
}

static inline void memcpy_h2i16(uint16 *dst, const uint16 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = h2i16(*src++);
}

static inline void memcpy_i2h16(uint16 *dst, const uint16 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = i2h16(*src++);
}

#else

#error Sorry, the "BYTE_ORDER" is neither "LITTLE_ENDIAN" nor "BIG_ENDIAN"

#endif

//////////////////////////////////////////////////////////////////////

#define ENCODE_OTTID(a, b, c, d) \
    (((a) << 24) | ((b) << 16) | ((c) << 8) | d)

enum
{
    OTTREQ_COMM = ENCODE_OTTID('c', 'o', 'm', 'm'),
    OTTRPY_EXPO = ENCODE_OTTID('e', 'x', 'p', 'o'),
    OTTRPY_TOUT = ENCODE_OTTID('t', 'o', 'u', 't'),
    OTTRPY_ACKN = ENCODE_OTTID('a', 'c', 'k', 'n'),
    OTTRPY_BUSY = ENCODE_OTTID('b', 'u', 's', 'y'),
    OTTRPY_ERRP = ENCODE_OTTID('e', 'r', 'r', 'P'),
    OTTRPY_ERRM = ENCODE_OTTID('e', 'r', 'r', 'M'),
    OTTRPY_ERRC = ENCODE_OTTID('e', 'r', 'r', 'C'),
    OTTRPY_ERRA = ENCODE_OTTID('e', 'r', 'r', 'A'),
    OTTRPY_ERRS = ENCODE_OTTID('e', 'r', 'r', 'S'),
    OTTRPY_DATA = ENCODE_OTTID('d', 'a', 't', 'a'),
};

enum // OTTREQ_COMM.cmd
{
    OTTCMD_SETMODE   = 0, // .par1=mode
    OTTCMD_NEW_FRAME = 1, //
    OTTCMD_SND_FRAME = 2, // .par1>0 => resend SOME, map in following data[]
    OTTCMD_CONFIGURE = 3, // .par1=func_no, .par2=value
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

enum // OTTREQ_COMM:OTTCMD_CONFIGURE.par1
{
    OTTFUN_EXPO_MS  = 0,
    OTTFUN_AUTOEXPO = 1,
    OTTFUN_AMPL_VAL = 2, // 1...100
    OTTFUN_AUTOAMPL = 3,
    OTTFUN_EXTSTART = 4,
    OTTFUN_ADC_V    = 5,
    OTTFUN_ZZZ      = 6,
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
    uint32  ofs1;
    uint32  ofs2;
    uint16  frm_n;
    uint8   data[0];
} ottcam_data_t;



//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

static int ottcamx_init_d(int devid, void *devptr, 
                          int businfocount, int businfo[],
                          const char *auxinfo)
{
  ottcamx_privrec_t  *me = (ottcamx_privrec_t *) devptr;

  unsigned long       host_ip;
  struct hostent     *hp;
  struct sockaddr_in  cam_addr;
  int                 fd;
  size_t              bsize;                     // Parameter for setsockopt
  int                 r;

    me->devid   = devid;
    me->fd      = -1;
    me->sq_tid  = -1;
    me->rrq_tid = -1;

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
        
        InitParams(&(me->pz));

        me->sim_x  = 300;
        me->sim_y  = 200;
        me->sim_vx = SIMSQ_W/2;
        me->sim_vy = SIMSQ_H/2;

        me->sq_tid = sl_enq_tout_after(devid, devptr, SIM_PERIOD, sim_heartbeat, NULL);

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
        cam_addr.sin_port   = htons(DEFAULT_OTTCAMX_PORT);
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
        bsize = 1500 * OTTCAMX_MAX_H;
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &bsize, sizeof(bsize));
        /* Specify the other endpoint */
        r = connect(fd, &cam_addr, sizeof(cam_addr));
        if (r != 0)
        {
            DoDriverLog(devid, 0 | DRIVERLOG_ERRNO, "%s: connect()", __FUNCTION__);
            return -CXRF_DRV_PROBL;
        }

        /* Init pzframe */
        pzframe_drv_init(&(me->pz), devid,
                         OTTCAMX_NUM_PARAMS,
                         OTTCAMX_PARAM_SHOT, OTTCAMX_PARAM_ISTART,
                         OTTCAMX_PARAM_WAITTIME, OTTCAMX_PARAM_STOP,
                         OTTCAMX_PARAM_ELAPSED,
                         ValidateParam,
                         StartMeasurements, TrggrMeasurements,
                         AbortMeasurements, ReadMeasurements);
        pzframe_set_buf_p(&(me->pz), me->retdata);

        InitParams(&(me->pz));

        /* Init queue */
        r = sq_init(&(me->q), NULL,
                    OTTCAMX_SENDQ_SIZE, sizeof(ottqelem_t),
                    OTTCAMX_SENDQ_TOUT, me,
                    ottcamx_sender,
                    ottcamx_eq_cmp_func,
                    tim_enqueuer,
                    tim_dequeuer);
        if (r < 0)
        {
            DoDriverLog(me->devid, 0 | DRIVERLOG_ERRNO, "%s: sq_init()", __FUNCTION__);
            return -CXRF_DRV_PROBL;
        }

        sl_add_fd(me->devid, devptr, fd, SL_RD, ottcamx_fd_p, NULL);

        return DEVSTATE_OPERATING;
    }
}

static void ottcamx_term_d(int devid, void *devptr)
{
  ottcamx_privrec_t *me = (ottcamx_privrec_t *) devptr;

    if (me->is_sim)
    {
        if (me->sq_tid >= 0) 
        sl_deq_tout(me->sq_tid);
    }
    else
    {
        pzframe_drv_term(&(me->pz));
        sq_fini(&(me->q));
    }
}

static void ottcamx_rw_p  (int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  ottcamx_privrec_t *me = (ottcamx_privrec_t *)devptr;

    if (me->is_sim) return;
    pzframe_drv_rw_p(&(me->pz), firstchan, count, values, action);
}

static void ottcamx_bigc_p(int devid, void *devptr,
                           int chan __attribute__((unused)),
                           int32 *args, int nargs,
                           void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  ottcamx_privrec_t *me = (ottcamx_privrec_t *)devptr;

    if (me->is_sim) return;
    pzframe_drv_bigc_p(&(me->pz), args, nargs);
}


static CxsdMainInfoRec ottcamx_main_info[] =
{
    {OTTCAMX_NUM_PARAMS, 1},
};

static CxsdBigcInfoRec ottcamx_bigc_info[] =
{
    {1, 
        OTTCAMX_NUM_PARAMS, 0,                                             0,
        OTTCAMX_NUM_PARAMS, OTTCAMX_MAX_W*OTTCAMX_MAX_H*OTTCAMX_DATAUNITS, OTTCAMX_DATAUNITS},
};

DEFINE_DRIVER(ottcamx,  "Ottmar's Camera, X-version",
              NULL, NULL,
              sizeof(ottcamx_privrec_t), NULL,
              0, 0,
              NULL, 0,
              countof(ottcamx_main_info), ottcamx_main_info,
              countof(ottcamx_bigc_info), ottcamx_bigc_info,
              ottcamx_init_d, ottcamx_term_d,
              ottcamx_rw_p, ottcamx_bigc_p);
