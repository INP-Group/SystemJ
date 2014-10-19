#include "cxsd_includes.h"

#include "cxsd_fe_cxv2.h"
#include "cx_proto.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "fix_arpa_inet.h"


//////////////////////////////////////////////////////////////////////

#include <sys/utsname.h>

/*
 *  HostName
 *      Returns a name of the current processor.
 *          On a first call, does a uname() and copies the .nodename
 *      to a static area, than trims out the domain name (if any),
 *      so that only one-word name remains. If the resulting name is
 *      too long (> 31 chars), it is truncated.
 *          If, for some reason, the uname()d name is empty, sets it
 *      to "?host?".
 *
 *  Result:
 *      Pointer to the name.
 */

static char *HostName(void)
{
  static char     name[32] = "";
  struct utsname  buf;
  int             x;

    /* If it is the first call, must obtain the name */
    if (name[0] == '\0')
    {
        uname(&buf);
        strzcpy(name, buf.nodename, sizeof(name));

        for (x = 0;
             x < (signed)sizeof(name) - 1  &&  name[x] != '\0'  &&  name[x] != '.';
             x++);
        name[x] = '\0';
        if (x == 0) strcpy(name, "?host?");
    }

    return name;
}

//////////////////////////////////////////////////////////////////////
enum { MAX_ALLOWED_FD = FD_SETSIZE - 1 };
enum {
    RCC_SHIFT = MAX_ALLOWED_FD + 1,
    RCC_SIZE  = (1 << 30) / RCC_SHIFT
};

static inline void InitID(int *rcc)
{
    (*rcc) = 12345*0 +
#if OPTION_HAS_RANDOM
             random()
#else
             rand()
#endif /* OPTION_HAS_RANDOM */
    ;
}
static inline uint32 CreateNewID(int s, int *rcc)
{
    (*rcc) = ((*rcc) + 1) % RCC_SIZE;
    return s + (*rcc) * RCC_SHIFT;
}

static inline int ID2fd(uint32 ID)
{
    return ID % RCC_SHIFT;
}
//////////////////////////////////////////////////////////////////////


//// General /////////////////////////////////////////////////////////

enum { MAX_UNCONNECTED_TIME = 60 };

static  int            unix_entry    = -1;    /* Socket to listen AF_UNIX */
static  fdio_handle_t  unix_iohandle = -1;

static  int            inet_entry    = -1;    /* Socket to listen AF_INET */
static  fdio_handle_t  inet_iohandle = -1;

static  int            server_rcc    = 0;


enum
{
    CS_OPEN_FRESH = 0,  // Just allocated
    CS_OPEN_LOGIN,

    CS_USABLE,          // The connection is full-operational if >=
    CS_READY,           // Is ready for I/O
};

enum
{
    CLT_CONS = 1,
    CLT_DATA = 2,
    CLT_BIGC = 3,
};

typedef struct
{
    int            in_use;
    int            state;     // CS_xxx
    int            conntype;  // CLT_xxx

    int            fd;
    fdio_handle_t  fhandle;
    sl_tid_t       clp_tid;

    uint32         ID;
    time_t         when;
    uint32         ip;
    char           progname[40];
    char           username[40];
    uint32         uid;

    int32          lastSeq;

    /* Request/subscription status */
    int            wasrequest;      /* Was DATA request */
    int            hassubscription; /* Had subscribed to smth. */
    int            replyisready;    /* Bigc: Reply packet is ready for sending */
    int            bigc_immediate;  /* bigc_request.immediate value */
    int            bigc_ninfo;      /* bigc_request.ninfo value */
    int            bigc_retbufsize; /* bigc_request.retbufsize value */

    /* Data buffers */
    CxHeader      *replybuf;        /* Reply buffer */
    size_t         replybufsize;    /* Its size */
    CxHeader      *subscrbuf;       /* Subscription buffer */
    size_t         subscrbufsize;   /* Its size */
    int32         *subscrsetbuf;    /* Buffer for GETVSET chans */
    size_t         subscrsetbufsize;/* Its size */

    CxHeader      *lastreqbuf;      /* Buffer to hold last "long" request. Currently used for bigcs */
    size_t         lastreqbufsize;  /* Its size */

    /* Misc. */
    bigc_listitem_t *bigclistitem_p;/* To be added to requested bigc's chain */
} v2clnt_t;

enum
{
    V2_MAX_CONNS = 1000,  // An arbitrary limit
    V2_ALLOC_INC = 2,     // Must be >1 (to provide growth from 0 to 2)
};

static v2clnt_t *cx2clnts_list        = NULL;
static int       cx2clnts_list_allocd = 0;

//////////////////////////////////////////////////////////////////////

// GetV2connSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, V2conn, v2clnt_t,
                                 cx2clnts, in_use, 0, 1,
                                 1, V2_ALLOC_INC, V2_MAX_CONNS,
                                 , , void)

static void RlsV2connSlot(int cd)
{
  v2clnt_t  *cp  = AccessV2connSlot(cd);
  int        err = errno;        // To preserve errno

    if (cp->in_use == 0) return; /*!!! In fact, an internal error */

    cp->in_use = 0;

    ////////////
    if (cp->fhandle >= 0) fdio_deregister(cp->fhandle);
    if (cp->fd      >= 0) close          (cp->fd);
    if (cp->clp_tid >= 0) sl_deq_tout    (cp->clp_tid);

    if (cp->bigclistitem_p != NULL) DelBigcRequest(cp->bigclistitem_p);
    safe_free(cp->replybuf);
    safe_free(cp->subscrbuf);
    safe_free(cp->subscrsetbuf);
    safe_free(cp->lastreqbuf);
    safe_free(cp->bigclistitem_p);

    errno = err;
}

static inline int cp2cd(v2clnt_t *cp)
{
    return cp - cx2clnts_list;
}

enum
{
    ERR_CLOSED = -1,
    ERR_KILLED = -2,
    ERR_IN2BIG = -3,
    ERR_HSHAKE = -4,
    ERR_TIMOUT = -5,
    ERR_INTRNL = -6,
};

static void DisconnectClient(v2clnt_t *cp, int err, uint32 code)
{
  const char *reason_s;
  const char *errdescr_s;
  CxHeader    hdr;
  char        from[100];
    
    /* Log the case... */
    errdescr_s = "";
    if      (err == 0)          reason_s = "disconnect by";
    else if (err == ERR_CLOSED) reason_s = "conn-socket closed by";
    else if (err == ERR_KILLED) reason_s = "killed";
    else if (err == ERR_IN2BIG) reason_s = "overlong packet from";
    else if (err == ERR_HSHAKE) reason_s = "handshake error, dropping";
    else if (err == ERR_TIMOUT) reason_s = "handshake timeout, dropping";
    else if (err == ERR_INTRNL) reason_s = "internal error, disconnecting";
    else                        reason_s = "abnormally disconnected",
                                errdescr_s = strerror(err);

    if (cp->ip == 0)
        strcpy(from, "-");
    else
        sprintf(from, "%d.%d.%d.%d",
                      (cp->ip >> 24) & 255,
                      (cp->ip >> 16) & 255,
                      (cp->ip >>  8) & 255,
                      (cp->ip >>  0) & 255);

    logline(LOGF_ACCESS, 0, "{%d} %s %s@%s:%s%s%s",
            cp2cd(cp),
            reason_s,
            cp->username[0] != '\0'? cp->username : "UNKNOWN",
            from,
            cp->progname[0] != '\0'? cp->progname : "UNKNOWN",
            errdescr_s[0]   != '\0'? ": " : "",
            errdescr_s);

    /* Should try to tell him something? */
    if (code != 0)
    {
        bzero(&hdr, sizeof(hdr));
        hdr.Type = code;

        fdio_send(cp->fhandle, &hdr, sizeof(hdr));
    }

    /* Do "preventive" closes */
    fdio_deregister_flush(cp->fhandle, 10/*!!!Should make an enum-constant*/);
    cp->fhandle = -1;
    cp->fd      = -1;

    RlsV2connSlot(cp2cd(cp));
}


//////////////////////////////////////////////////////////////////////

static int  TellClient(int cd, int32 code)
{
  int       r;
  CxHeader  hdr;
  v2clnt_t *cp = AccessV2connSlot(cd);
    
    bzero(&hdr, sizeof(hdr));
    hdr.ID   = cp->ID;
    hdr.Seq  = cp->lastSeq;
    hdr.Type = code;
    r = fdio_send(cp->fhandle, &hdr, sizeof(hdr));
    if (r < 0) DisconnectClient(cp, errno, 0);
    return r;
}

static int  TellClientVersion(int cd, int32 code)
{
  int       r;
  CxHeader  hdr;
  v2clnt_t *cp = AccessV2connSlot(cd);
    
    bzero(&hdr, sizeof(hdr));
    hdr.ID   = cp->ID;
    hdr.Seq  = cp->lastSeq;
    hdr.Type = code;
    hdr.Res1 = htonl(CX_PROTO_VERSION);
    r = fdio_send(cp->fhandle, &hdr, sizeof(hdr));
    if (r < 0) DisconnectClient(cp, errno, 0);
    return r;
}

static int  TellClientWParam(int cd, int32 code, int param)
{
  int       r;
  CxHeader  hdr;
  v2clnt_t *cp = AccessV2connSlot(cd);
    
    bzero(&hdr, sizeof(hdr));
    hdr.ID   = cp->ID;
    hdr.Seq  = cp->lastSeq;
    hdr.Type = code;
    hdr.Res2 = param;
    r = fdio_send(cp->fhandle, &hdr, sizeof(hdr));
    if (r < 0) DisconnectClient(cp, errno, 0);
    return r;
}

static void InitReplyPacket (v2clnt_t *cp, int32 code, int Seq)
{
    bzero(cp->replybuf, sizeof(CxHeader));
    cp->replybuf->ID   = cp->ID;
    cp->replybuf->Seq  = Seq;
    cp->replybuf->Type = code;
}


static int  GrowReplyPacket (v2clnt_t *cp, size_t datasize)
{
  int r;

    r = GrowBuf((void *) &(cp->replybuf), &(cp->replybufsize),
                sizeof(CxHeader) + datasize);
    if (r < 0) return r;

    cp->replybuf->DataSize = datasize;

    return 0;
}

static int GrowSubscrPacket(v2clnt_t *cp, size_t datasize)
{
  int  r;

    r = GrowBuf((void *) &(cp->subscrbuf), &(cp->subscrbufsize),
                sizeof(CxHeader) + datasize);
    if (r < 0) return r;

    cp->subscrbuf->DataSize = datasize;

    return 0;
}

static int GrowSubscrsetbuf(v2clnt_t *cp, size_t newsize)
{
    return GrowBuf((void *) /*!!!???*/&(cp->subscrsetbuf), &(cp->subscrsetbufsize),
                   newsize);
}

//////////////////////////////////////////////////////////////////////

//// cx-server_channels //////////////////////////////////////////////

/*
 *  CheckFork
 *      Checks fork contents (command code and channel numbers) for validity.
 *
 */

static int CheckFork(CxDataFork *f, int n, const char *function, int cd)
{
  int32  *p;
  int     x;
  size_t  bytesize;
    
    switch (f->OpCode)
    {
        case CXC_SETVGROUP:
        case CXC_GETVGROUP:
            if (f->ChanId                <  0        ||
                f->Count                 <= 0        ||
                f->ChanId + f->Count - 1 >= numchans
               )
            {
                TellClientWParam(cd, CXT_EINVCHAN, f->ChanId + f->Count - 1);
                return -1;
            }
            break;

        case CXC_SETVSET:
        case CXC_GETVSET:
            for (x = 0, p = (int32 *) (f->data);
                 x < f->Count;
                 x++,   p++)
                if (*p < 0  ||  *p >= numchans)
                {
                    TellClientWParam(cd, CXT_EINVCHAN, *p);
                    return -1;
                }
            break;

        default:
            logline(LOGF_SYSTEM, 0, "%s(%d): Unknown DataFork[%d].OpCode %d",
                    function, cd,
                    n, f->OpCode);
            TellClientWParam(cd, CXT_EINVAL, f->OpCode);
            return -1;
    }

    /* Okay, and now we should check the size */
    bytesize = DataForkSize(f->OpCode, f->Count, CXDIR_REQUEST);
    if (f->ByteSize != bytesize)
    {
        logline(LOGF_SYSTEM, 0, "%s(%d): DataFork[%d].ByteSize=%u, !=%zu",
                function, cd,
                n, f->ByteSize, bytesize);
        TellClient(cd, CXT_EINVAL);
        return -1;
    }
    
    return 0;
}


static inline void MarkChanForRead (chanaddr_t chan)
{
    MarkRangeForIO(chan, 1, DRVA_READ,  NULL);
}

static inline void MarkChanForWrite(chanaddr_t chan, int32 value)
{
    MarkRangeForIO(chan, 1, DRVA_WRITE, &value);
}


/*
 *  ServeDataRequest
 *      Handles a CXT_DATA_REQ packet.
 */

static void ServeDataRequest     (v2clnt_t *cp, CxHeader *hdr, size_t inpktsize)
{
  int           cd = cp2cd(cp);
  int           r;
  int           x;
  uint8        *ptr;
  CxDataFork   *src;
  size_t        replyofs;
  size_t        replyinc;
  int           nf;
  CxDataFork   *f;
  int32        *d1, *d2;

    /* Be sure that it is a right connection */
    if (cp->conntype != CLT_DATA)
    {
        TellClient(cd, CXT_EBADRQC);
        return;
    }
  
    /* Was this connection already used in current cycle? */
    if (cp->wasrequest)
    {
        TellClient(cd, CXT_EBUSY);
        return;
    }

    /* Initialize a reply buffer */
    InitReplyPacket(cp, CXT_DATA_RPY, hdr->Seq);
    cp->replybuf->NumData = hdr->NumData;

    /* Now, walk through the request */
    for (nf = 0, ptr = (void *)(hdr->data),
             replyofs = 0;
         nf < hdr->NumData;
         nf++)
    {
        src = (void*)ptr;

        /* Check first... */
        if (CheckFork(src, nf, __FUNCTION__, cd) < 0) return;

        /* Grow the buffer and record fork header */
        replyinc = DataForkSize(CXC_ANY, src->Count, CXDIR_REPLY);
        r = GrowReplyPacket(cp, replyofs + replyinc);
        if (r < 0)
        {
            TellClient(cd, CXT_ENOMEM);
            return;
        }

        /* Record fork header in the reply buffer */
        f = (void *) (cp->replybuf->data + replyofs);
        *f = *src;
        f->OpCode = CXP_CVT_TO_RPY(src->OpCode);

        /* Now, do command-dependent operations */
        d1 = (int32 *)(src->data);
        d2 = d1 + src->Count;  
        
        switch (src->OpCode)
        {
            case CXC_SETVGROUP:
                logline(LOGF_DBG1, 9, "%s: SETVGROUP(start=%d, count=%d), !%d:=%d",
                        cp->progname, src->ChanId, src->Count, src->ChanId, *d1);
            case CXC_GETVGROUP:
                MarkRangeForIO(src->ChanId, src->Count,
                               src->OpCode == CXC_SETVGROUP? DRVA_WRITE : DRVA_READ,
                               d1);
                break;

            case CXC_SETVSET:
            case CXC_GETVSET:
                /* 1. Record a fork in the reply buffer */
                /* In *SET operations the numbers are stored
                   in the data fields */
                memcpy(f->data, src->data, f->Count * sizeof(uint32));

                /* Schedule execution of request */
                if (src->OpCode == CXC_SETVSET)
                {
                    for (x = 0; x < src->Count; x++)
                        MarkChanForWrite(d1[x], d2[x]),
                    logline(LOGF_DBG1, 9, "%s: SETVSET: !%d:=%d",
                            cp->progname, d1[x], d2[x]);
                }
                else
                {
                    for (x = 0; x < src->Count; x++)
                        MarkChanForRead(d1[x]);
                }
                
                break;
        }

        /* Advance pointers */
        replyofs += replyinc;
        ptr      += DataForkSize(src->OpCode, src->Count, CXDIR_REQUEST);
    }

    /* Finally, mark this connection as busy */
    cp->wasrequest = 1;
}

static void RequestSubscription(v2clnt_t *cp)
{
  int           x;
  uint8        *ptr;
  CxDataFork   *frk;
  int           nf;
  int32        *setinfo;

    /* Walk through the request */
    for (nf = 0, ptr = (void *)(cp->subscrbuf->data),
             setinfo = cp->subscrsetbuf;
         nf < cp->subscrbuf->NumData;
         nf++)
    {
        frk = (void*)ptr;

        /* Schedule execution of request */
        switch (frk->OpCode)
        {
            case CXC_GETVGROUP_R:
                MarkRangeForIO(frk->ChanId, frk->Count,
                               DRVA_READ, NULL);
                break;
                
            case CXC_GETVSET_R:
                for (x = 0;  x < frk->Count;  x++)
                    MarkChanForRead(*setinfo++);
                break;

            default:
                logline(LOGF_SYSTEM, 0, "%s::%s(): OpCode=%d - ???",
                        __FILE__, __FUNCTION__, frk->OpCode);
        }
        
        /* Advance pointer */
        ptr += DataForkSize(CXC_ANY, frk->Count, CXDIR_REPLY);
    }
}


/*
 *  ServeSubscribeRequest
 */

static void ServeSubscribeRequest(v2clnt_t *cp, CxHeader *hdr, size_t inpktsize)
{
  int           cd = cp2cd(cp);
  int           r;
  uint8        *ptr;
  CxDataFork   *src;
  size_t        subscrofs;
  size_t        subscrinc;
  int           nf;
  CxDataFork   *f;
  int32        *d1;
  int           setcount;
  
    /* Be sure that it is a right connection */
    if (cp->conntype != CLT_DATA)
    {
        TellClient(cd, CXT_EBADRQC);
        return;
    }

    /* First, forget the previous subscription */
    cp->hassubscription = 0;

    /* Initialize subscription buffer */
    /* We cheat: a packet in the regular reply buffer is init'ed, and is
       copied to subscrbuf */
    InitReplyPacket(cp, CXT_SUBSCRIPTION, hdr->Seq);
    *(cp->subscrbuf) = *(cp->replybuf);

    cp->subscrbuf->NumData = hdr->NumData;

    /* Now, walk through the request and record it */
    for (nf = 0, ptr = (void *)(hdr->data),
             subscrofs = 0, setcount = 0;
         nf < hdr->NumData;
         nf++)
    {
        src = (void*)ptr;

        /* Check first... */
        if (CheckFork(src, nf, __FUNCTION__, cd) < 0) return;

        /* Grow the buffer and record fork header */
        subscrinc = DataForkSize(CXC_ANY, src->Count, CXDIR_REPLY);
        r = GrowSubscrPacket(cp, subscrofs + subscrinc);
        if (r < 0)
        {
            TellClient(cd, CXT_ENOMEM);
            return;
        }

        /* Record a fork header in the reply buffer */
        f = (void *) (cp->subscrbuf->data + subscrofs);
        *f = *src;
        f->OpCode = CXP_CVT_TO_RPY(src->OpCode);

        /* Now, do command-dependent operations */
        d1 = (int32 *)(src->data);
        
        switch (src->OpCode)
        {
            case CXC_GETVGROUP:
                /* Since it is a "group" request, everything
                   is already recorded in the header, so we
                   must do nothing. */
                break;

            case CXC_GETVSET:
                r = GrowSubscrsetbuf(cp, (setcount + src->Count) * sizeof(int32));
                if (r < 0)
                {
                    TellClient(cd, CXT_ENOMEM);
                    return;
                }

                /* Store channel numbers */
                memcpy(cp->subscrsetbuf + setcount, d1,
                       src->Count * sizeof(int32));

                /* Increment the number of "set" addresses */
                setcount += src->Count;
                
                break;
            
            default:
                TellClient(cd, CXT_EINVAL);
                return;
        }

        /* Advance the pointers */
        subscrofs += subscrinc;
        ptr       += DataForkSize(src->OpCode, src->Count, CXDIR_REQUEST);
    }

    /* Finally, mark subscription as existing */
    cp->hassubscription = (cp->subscrbuf->NumData != 0);

    /* Run it */
    RequestSubscription(cp);
    
    /* And tell the client that everything is OK */
    TellClient(cd, CXT_OK);
}

static void FillDataPacket(v2clnt_t *cp, int type)
{
  int           x;
  uint8        *ptr;
  CxDataFork   *dst;
  size_t        bytesize;
  int           nf;
  int32        *vals_p;
  int32        *flgs_p;
  int8         *tags_p;
  int32        *setinfo;
  CxHeader     *buf;
  
    buf = type == CXT_DATA_RPY? cp->replybuf : cp->subscrbuf;

//    logline(LOGF_SYSTEM, 0, "%s: type=%d, buf->NumData=%d",
//            __FUNCTION__, type, buf->NumData);
    
    for (nf = 0, ptr = (void *) (buf->data),
             setinfo = cp->subscrsetbuf;
         nf < buf->NumData;
         nf++)
    {
        dst = (void *)ptr;

        bytesize = DataForkSize(CXC_ANY, dst->Count, CXDIR_REPLY);
        dst->ByteSize = bytesize;
        
        vals_p = (void *)(dst->data);
        flgs_p = (void *)(vals_p + dst->Count);
        tags_p = (void *)(flgs_p + dst->Count);

        if (type == CXT_DATA_RPY)
            setinfo = vals_p;

//        logline(LOGF_SYSTEM, 0, "\tCommand=%x Count=%d", dst->Command, dst->Count);
        
        switch (dst->OpCode)
        {
            case CXC_SETVGROUP_R:
            case CXC_GETVGROUP_R:
                /* Tags */
                for (x = 0;  x < dst->Count;  x++)
                    tags_p[x] = FreshFlag(dst->ChanId + x);

                /* Flags */
                memcpy(flgs_p, c_rflags + dst->ChanId,
                       dst->Count * sizeof(c_rflags[0]));
                
                /* Data values */
                memcpy(vals_p, c_value + dst->ChanId,
                       dst->Count * sizeof(c_value[0]));

                break;

            case CXC_SETVSET_R:
            case CXC_GETVSET_R:
                for (x = 0;  x < dst->Count;  x++)
                {
                    tags_p[x] = FreshFlag(*setinfo);  // Tag
                    flgs_p[x] = c_rflags [*setinfo];  // Flags
                    vals_p[x] = c_value  [*setinfo];  // Value
                    setinfo++;
                }

                break;

            default:
                logline(LOGF_SYSTEM, 0, "%s::%s(): OpCode=%d - ???",
                        __FILE__, __FUNCTION__, dst->OpCode);
        }

//        logline(LOGF_SYSTEM, 0, "\t.");
        
        /* Advance */
        ptr += bytesize;
    }
}


//// cx-server_bigc //////////////////////////////////////////////////

static void bigc_callback(int bigchan, void *closure);


static int CopyReqbufToLastreqbuf(v2clnt_t *cp, CxHeader *hdr)
{
   int     r;
   size_t  newsize = sizeof(CxHeader) + hdr->DataSize;

     r = GrowBuf((void *) &(cp->lastreqbuf), &(cp->lastreqbufsize), newsize);
     if (r < 0) return r;
     memcpy(cp->lastreqbuf, hdr, newsize);

     return 0;
}

static void ServeBigcRequest (v2clnt_t *cp, CxHeader *hdr, size_t inpktsize)
{
  int            cd = cp2cd(cp);
  int            r;
  CxBigcFork    *src;
  CxBigcFork    *copy_src;
  size_t         expected_pkt_datasize;

  bigchaninfo_t *bp;
  
  int            bigchan;
  unsigned int   nparams;
  size_t         datasize;
  size_t         dataunits;
  int            cachectl;
  int            immediate;
  int            ninfo;
  int            retbufsize;

  int32         *args_p;
  void          *data_p;

  size_t         bytesize;

  ////logline(LOGF_DBG1, 0, __FUNCTION__);
  
    /* Be sure that it is a right connection */
    if (cp->conntype != CLT_BIGC)
    {
        TellClient(cd, CXT_EBADRQC);
        return;
    }
  
    /* Was this connection already used? */
    if (cp->wasrequest)
    {
        TellClient(cd, CXT_EBUSY);
        return;
    }

    if (hdr->NumData != 1)
    {
        logline(LOGF_SYSTEM, 0, "%s(%d): NumData=%d instead of 1",
                __FUNCTION__, cd,
                hdr->NumData);
        TellClient(cd, CXT_EINVAL);
        return;
    }
    if (hdr->DataSize < sizeof(CxBigcFork))
    {
        logline(LOGF_SYSTEM, 0, "%s(%d): DataSize=%u, <sizeof(CxBigcFork)=%zu",
                __FUNCTION__, cd,
                hdr->DataSize, sizeof(CxBigcFork));
        TellClient(cd, CXT_EINVAL);
        return;
    }
    
    /* I. Read the request */
    src = (void *)(hdr->data);

    if (src->OpCode != CXB_BIGCREQ)
    {
        logline(LOGF_SYSTEM, 0, "%s(%d): Unknown BigcFork.OpCode code %d",
                __FUNCTION__, cd,
                src->OpCode);
        TellClient(cd, CXT_EINVAL);
        return;
    }
    
    bigchan    = src->ChanId;
    nparams    = src->nparams16x2;
    datasize   = src->datasize;
    dataunits  = src->dataunits;
    cachectl   = EXTRACT_FLAG(src->u.q.ninfo_n_flags, CXBC_FLAGS_CACHECTL_SHIFT, CXBC_FLAGS_CACHECTL_MASK);
    immediate  = EXTRACT_FLAG(src->u.q.ninfo_n_flags, CXBC_FLAGS_IMMED_SHIFT,    CXBC_FLAGS_IMMED_FLAG);
    ninfo      = EXTRACT_FLAG(src->u.q.ninfo_n_flags, CXBC_FLAGS_NINFO_SHIFT,    CXBC_FLAGS_NINFO_MASK);
    retbufsize = src->u.q.retbufsize;

    ////if (cachectl == CX_CACHECTL_FROMCACHE) immediate = 0;

    //logline(LOGF_SYSTEM, 0, "%s: bigchan=%d, nparams=%d, datasize=%d, dataunits=%d",
    //        __FUNCTION__, bigchan, nparams, datasize, dataunits);

    if (ninfo      < 0) ninfo     = 0;
    if (retbufsize < 0) retbufsize = 0;
    
    /* II. Check the request validity */

    /* Channel #? */
    if (bigchan < 0  ||  bigchan >= numbigcs)
    {
        TellClientWParam(cd, CXT_EINVCHAN, bigchan);
        return;
    }

    bp = b_data + bigchan;
    
    /* Counts -- the same checks as in cxlib.c::cx_bigcmsg() */
    if (datasize == 0)  dataunits = 1;
    if (nparams  > CX_MAX_BIGC_PARAMS  ||
        datasize > CX_ABSOLUTE_MAX_DATASIZE/*!!!*/  ||
        (
         (dataunits != 1  &&  dataunits != 2  &&  dataunits != 4) ||
         datasize % dataunits != 0
        )
       )
    {
        logline(LOGF_DBG1, 0, "%s(%d): nparams=%d, datasize=%zu, dataunits=%zu",
                __FUNCTION__, cd, nparams, datasize, dataunits);
        TellClient(cd, CXT_EINVAL);
        return;
    }

    bytesize = BigcForkSize(nparams, datasize);
    if (src->ByteSize != bytesize)
    {
        logline(LOGF_SYSTEM, 0, "%s(%d): BigcFork[%d].ByteSize=%u, !=%zu",
                __FUNCTION__, cd,
                0, src->ByteSize, bytesize);
        TellClient(cd, CXT_EINVAL);
        return;
    }
    
    /* Check the packet size */
    expected_pkt_datasize = BigcForkSize(nparams, datasize);
    if (hdr->DataSize != expected_pkt_datasize)
    {
        logline(LOGF_DBG1, 0, "%s(%d): reqbuf->DataSize=%u != expected_pkt_datasize=%zu",
                __FUNCTION__, cd, hdr->DataSize, expected_pkt_datasize);
        TellClient(cd, CXT_EINVAL);
        return;
    }

    /* Should we correct sizes according to bigc's metric? */
    if (nparams  > (unsigned int)(bp->nargs))
    {
        logline(LOGF_DBG1, 0, "%s(%d=>%s, bigchan=%d): WARNING: nparams=%d > bp->nargs=%d",
                __FUNCTION__, cd, cp->username, bigchan, nparams, bp->nargs);
        nparams  = bp->nargs;
    }
    if (datasize > bp->datasize)
    {
        logline(LOGF_DBG1, 0, "%s(%d=>%s, bigchan=%d): WARNING: datasize=%zu > bp->datasize=%zu",
                __FUNCTION__, cd, cp->username, bigchan, datasize, bp->datasize);
        datasize = bp->datasize;
    }
    
    /* III. Create a copy of request */
    r = CopyReqbufToLastreqbuf(cp, hdr);
    if (r < 0)
    {
        TellClient(cd, CXT_ENOMEM);
        return;
    }
    copy_src = (void *)(cp->lastreqbuf->data);
    
    /* IV. Finally, mark this connection as busy */
    /* Note: since the driver may answer immediately, right from d_dobig[](),
       which can cause SendBigcReply() and wasrequest:=0, we set
       wasrequest:=1 *before* calling AddBigcRequest=>d_dobig[]().
     */
    cp->wasrequest      = 1;
    cp->replyisready    = 0;

    cp->bigc_immediate  = immediate | 0;
    cp->bigc_ninfo      = ninfo;
    cp->bigc_retbufsize = retbufsize;
    
    /* V. Pass the request for execution */
    args_p = (int32 *)(copy_src->Zdata);
    data_p = (void *) (args_p + nparams);

    if (cachectl == CX_CACHECTL_FROMCACHE)
    {
        bigc_callback(bigchan, lint2ptr(cd));
    }
    else
    {
        AddBigcRequest(bigchan, cp->bigclistitem_p,
                       bigc_callback, lint2ptr(cd),
                       args_p, nparams,
                       data_p, datasize, dataunits,
                       cachectl);
    }
}

static int FillBigcReplyPacket(v2clnt_t *cp)
{
  int            r;
  CxBigcFork    *copy_src = (void *)(cp->lastreqbuf->data);
  CxBigcFork    *dst;
  int32         *p;
  int32          ninfo;
  int32          retdatasize;

  int            bigchan  = copy_src->ChanId;
  bigchaninfo_t *bp       = b_data + bigchan;
  
  size_t         bytesize;

    /* -2. Calculate "desired" sizes */
    ninfo = bp->cur_ninfo;
    if (ninfo > cp->bigc_ninfo) ninfo = cp->bigc_ninfo;

    retdatasize = bp->cur_retdatasize;
    if (retdatasize > cp->bigc_retbufsize) retdatasize = cp->bigc_retbufsize;

    bytesize = BigcForkSize(ninfo, retdatasize);
  
    /* -1. Allow really large packets */
    //!!!SetSendParams(s, bytesize, -1); => fdio_set_maxsbuf()
  
    /* 0. Grow the reply packet to accomodate received data */
    InitReplyPacket(cp, CXT_BIGCRESULT, cp->lastreqbuf->Seq);
    cp->replybuf->NumData = cp->lastreqbuf->NumData;
    r = GrowReplyPacket(cp, bytesize);
    if (r < 0)
    {
        cp->wasrequest = 0;
        TellClient(cp2cd(cp), CXT_ENOMEM);
        return -1;
    }
  
    /* 1. Fill the packet with new data */
    dst = (void *)(cp->replybuf->data);
    bzero(dst, sizeof(*dst));

    dst->OpCode      = CXB_BIGCREQ_R;
    dst->ByteSize    = bytesize;
    dst->ChanId      = bigchan;
    dst->nparams16x2 = ninfo;
    dst->datasize    = retdatasize;
    dst->dataunits   = bp->cur_retdataunits;
    dst->u.y.rflags  = bp->cur_rflags;
    dst->u.y.tag     = age_of(bp->cur_time);
    
    p = dst->Zdata;
    if (ninfo != 0)
    {
        fast_memcpy(p, GET_BIGBUF_PTR(bp->cur_info_o,    int32), ninfo * sizeof(int32));
        p += ninfo;
    }
    if (retdatasize != 0)
    {
        fast_memcpy(p, GET_BIGBUF_PTR(bp->cur_retdata_o, void),  retdatasize);
    }

    cp->replyisready = 1;

    return 0;
}


//// cxd_execcmd /////////////////////////////////////////////////////

static char *command;
static char *argstart;
static int   nargs;


static void CmdNONE     (v2clnt_t *cp);
static void CmdClients  (v2clnt_t *cp);
static void CmdClose    (v2clnt_t *cp);
static void CmdEcho     (v2clnt_t *cp);
static void CmdExit     (v2clnt_t *cp);
static void CmdSato     (v2clnt_t *cp);
static void CmdScan     (v2clnt_t *cp);
static void CmdSetdrvlog(v2clnt_t *cp);
static void CmdKill     (v2clnt_t *cp);
static void CmdWho      (v2clnt_t *cp);
static void CmdWall     (v2clnt_t *cp);
static void CmdQ        (v2clnt_t *cp);


static const struct
{
    const char  *cmd;            /* Command name */
    int          priv;           /* Is a privileged command? */
    void       (*func)(v2clnt_t *cp);     /* Function to call */
    const char  *description;    /* One-line description (for "?") */
}
commands[] =
{
    {"clients",   0, CmdClients,   "list the connected clients"},
    {"close",     0, CmdClose,     "terminate some client(s)"},
    {"echo",      0, CmdEcho,      "echoes it's arguments"},
    {"exit",      0, CmdExit,      "exit from the system"},
    {"kill",      0, CmdKill,      "kill another console(s)"},
    {"load",      1, CmdNONE,      "load mode from a file"},
    {"refresh",   1, CmdNONE,      "reload the database"},
    {"run",       1, CmdNONE,      "run a .btl as if on ISERVER"},
    {"save",      1, CmdNONE,      "save current mode to a file"},
    {"sato",      0, CmdSato,      "call Sato"},
    {"scan",      0, CmdScan,      "do a scan"},
    {"setdrvlog", 0, CmdSetdrvlog, "modify driver's logmask"},
    {"shutdown",  1, CmdNONE,      "shut down the system"},
    {"start",     1, CmdNONE,      "start the system"},
    {"stop",      1, CmdNONE,      "stop the system"},
    {"who",       0, CmdWho,       "show who is console-logged on"},
    {"wall",      0, CmdWall,      "write to all consoles"},
    {"?",         0, CmdQ,         "print help information"},
    {NULL,        0, NULL,         NULL}
};


/*
 *  argp
 *      Returns a pointer to n-th argument, thus simulating argv[]
 *      for commands.
 */

static char *argp(int n)
{
  char        *p;

    if (n == 0)                            return command;
    if ((unsigned) n >= (unsigned) nargs)  return NULL;

    p = argstart;
    while (n != 1)
    {
        while (*p != '\0')  p++;        /* Skip the argument */
        p++;                            /* Skip the terminating '\0' */
        while (isspace(*p)) p++;        /* Skip the whitespace */
        n--;
    }

    return p;
}


static int PrintToClient(v2clnt_t *cp, int cmd, int status, const char *format, ...)
                        __attribute__ ((format (printf, 4, 5)));
static int PrintToClient(v2clnt_t *cp, int cmd, int status, const char *format, ...)
{
  va_list   ap;
  char      buf[CX_MAX_DATASIZE];
  CxHeader *replybuf;
  int       r;

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    InitReplyPacket(cp, cmd, cp->lastSeq);
    r = GrowReplyPacket(cp, strlen(buf) + 1);
    if (r < 0)
    {
        TellClient(cp2cd(cp), CXT_ENOMEM);
        return r;
    }
    
    replybuf = cp->replybuf;
    replybuf->Status = CXS_PRINT | status;
    memcpy(replybuf->data, buf, replybuf->DataSize);

    r = fdio_send(cp->fhandle,
                  cp->replybuf,
                  cp->replybuf->DataSize + sizeof(CxHeader));
    if (r < 0) DisconnectClient(cp, errno, 0);
    return r;
}


static void CmdNONE     (v2clnt_t *cp)
{
    PrintToClient(cp, CXT_OK, CXS_ERROR, "\"%s\" is not implemented\n", command);
}


static int ClientsIterator(v2clnt_t *cp, void *privptr)
{
  char **p_p = privptr;
  char  *p   = *p_p;

  char           from[100];

    if (cp->conntype != CLT_DATA  &&  cp->conntype != CLT_BIGC) return 0;

    sprintf(from, "(%d.%d.%d.%d)",
                  (cp->ip >> 24) & 255,
                  (cp->ip >> 16) & 255,
                  (cp->ip >>  8) & 255,
                  (cp->ip >>  0) & 255);

    p += sprintf(p, "%-10s %-5d %-10u %-19s %-17s %s\n",
                 cp->username, cp2cd(cp), cp->ID,
                 stroftime(cp->when, " "),
                 (cp->ip == 0) ?  "-"  :  from,
                 cp->progname
                );
    *p_p = p;

    return 0;
}
static void CmdClients  (v2clnt_t *cp)
{
  char           buf[CX_MAX_DATASIZE];
  char          *p = buf;

    /* The header line */
    p += sprintf(p, "%-10s %-5s %-10s %-19s %-17s %s\n",
                 "USER", "LINE", "ID", "LOGIN-TIME", "FROM", "WHAT");
    /* List them... */
    ForeachV2connSlot(ClientsIterator, &p);
    /* Send */
    PrintToClient(cp, CXT_OK, 0, "%s", buf);
}


static void CmdClose    (v2clnt_t *cp)
{
    CmdKill(cp);
}


static void CmdEcho     (v2clnt_t *cp)
{
  char  buf[CX_MAX_DATASIZE];
  char *p = buf;
  int   n;

    /* Create the string */
    for (n = 1; n < nargs; n++)
            p += sprintf(p, n == 1? "%s" : " %s", argp(n));
    p += sprintf(p, "\n");

    PrintToClient(cp, CXT_OK, 0, "%s", buf);
}


static void CmdExit     (v2clnt_t *cp)
{
    TellClient(cp2cd(cp), CXT_AEXIT);
}


static int FindIDIterator(v2clnt_t *cp, void *privptr)
{
    return (cp->ID == ptr2lint(privptr))? cp2cd(cp) : 0;
}

static void CmdKill     (v2clnt_t *cp)
{
  char    buf[CX_MAX_DATASIZE];
  char   *p = buf;
  int     n;
  char   *arg;
  char   *endptr;
  uint32  ID;
  int     target;
  v2clnt_t *victim;
  int     suicide = 0;

    /* Okay, walk through IDs... */
    for (n = 1; n < nargs; n++)
    {
        arg = argp(n);

        /* Check syntax */
        ID = strtoul(arg, &endptr, 10);
        if (endptr == arg  ||  *endptr != '\0')
        {
            p += sprintf(p, "kill: illegal ID: %s\n", arg);
            continue;
        }

        target = ForeachV2connSlot(FindIDIterator, lint2ptr(ID));
        victim = AccessV2connSlot(target);
        if (target <= 0  /*|| victim->conntype != CLT_CONS*/)
        {
            p += sprintf(p, "kill: no such client/console: %s\n", arg);
            continue;
        }

        /* Is it a "kill(getpid())"? */
        if (target == cp2cd(cp)) suicide = 1;

        /* Kill the victim... */
        DisconnectClient(victim, ERR_KILLED, CXT_DISCONNECT);
    }

    /* Do we still have to answer? ;-) */
    if (!suicide)
    {
        if (p == buf)
            TellClient(cp2cd(cp), CXT_OK);
        else
            PrintToClient(cp, CXT_OK, CXS_ERROR, "%s", buf);
    }
}


static void CmdSato     (v2clnt_t *cp)
{
  char    buf[CX_MAX_DATASIZE];
  char   *p = buf;
  int     n;
  char   *arg;
  char   *endptr;
  int     devid;

    /* Okay, walk through IDs... */
    for (n = 1; n < nargs; n++)
    {
        arg = argp(n);

        /* Check syntax */
        devid = strtoul(arg, &endptr, 10);
        if (endptr == arg  ||  *endptr != '\0')
        {
            p += sprintf(p, "sato: illegal devID: %s\n", arg);
            continue;
        }

        /* Check the id */
        if (devid < 0  ||  devid >= numdevs)
        {
            p += sprintf(p, "sato: devid=%d is out of [0..%d)\n", devid, numdevs);
            continue;
        }

        /* Reset the victim */
        ResetDev(devid);
    }
  
    if (p == buf)
        TellClient(cp2cd(cp), CXT_OK);
    else
        PrintToClient(cp, CXT_OK, CXS_ERROR, "%s", buf);
}


static void CmdScan     (v2clnt_t *cp)
{
  char    buf[CX_MAX_DATASIZE];
  char   *p = buf;
  int     n;
  char   *drvname;
  char   *auxinfo;
  char    statchar;
  int     add_dtls; // "Add details"
  char    flgs_s[100];
  char    time_s[100];
  int     x;
  char    bus_id_str[countof(d_businfo[0]) * (12+2)];
  char   *bis_p;

    add_dtls = (nargs > 1);
    if (add_dtls)
    {
        strcpy(flgs_s, "flgs");                 // 4 spaces
        strcpy(time_s, "@time               "); // 20 spaces
    }
    else
    {
        flgs_s[0] = '\0';
        time_s[0] = '\0';
    }
    
    p += sprintf(p, "%3s %c%s%s %10s %6s %4s %4s %s\n",
                 "ID", '?', flgs_s, time_s, "Name", "BusID", "#reg", "#big", "Auxinfo");
    
    for (n = 0;  n < numdevs;  n++)
    {
        if      (d_state[n] == DEVSTATE_OPERATING) statchar = '+';
        else if (d_state[n] == DEVSTATE_NOTREADY)  statchar = '.';
        else   /*d_state[n] == DEVSTATE_OFFLINE*/  statchar = '-';
        if (add_dtls)
        {
            sprintf(flgs_s, "%4x", d_statflags[n] & CXRF_SERVER_MASK);
            sprintf(time_s, "@%s", stroftime(d_stattime[n].tv_sec, "_"));
        }
        else
        {
            flgs_s[0] = '\0';
            time_s[0] = '\0';
        }
        drvname = GET_DRVINFOBUF_PTR(d_drvname_o[n], char);
        auxinfo = GET_DRVINFOBUF_PTR(d_auxinfo_o[n], char);
        if (d_businfocount[n] == 0)
            sprintf(bus_id_str, "-");
        else
            for (bis_p = bus_id_str, x = 0;  x < d_businfocount[n];  x++)
                bis_p += sprintf(bis_p, "%s%d", x > 0? "," : "", d_businfo[n][x]);
        p += sprintf(p, "%3d %c%s%s %10s %6s %4d %4d %s\n",
                     n,
                     d_loadok[n] < 0? '!' : statchar,
                     flgs_s,
                     time_s,
                     drvname != NULL? drvname : "?",
                     bus_id_str,
                     d_size[n],
                     d_big_count[n],
                     auxinfo != NULL? auxinfo : ""
                    );
    }
    
    PrintToClient(cp, CXT_OK, 0, "%s", buf);
}


static void CmdSetdrvlog(v2clnt_t *cp)
{
  char    buf[CX_MAX_DATASIZE];
  char   *p = buf;
  int            n;
  char          *arg;
  const char    *endptr;
  char          *err;
  int            log_set_mask;
  int            log_clr_mask;
  int            devid;
  int           *mask_p;

    if (nargs < 2)
    {
        p += sprintf(p, "setdrvlog: too few arguments\n");
        goto ERREXIT;
    }

    arg = argp(1);
    err = ParseDrvlogCategories(arg, &endptr,
                                &log_set_mask, &log_clr_mask);
    if (err != NULL)
    {
        p += sprintf(p, "setdrvlog: %s\n", err);
        goto ERREXIT;
    }
    
    /* Okay, walk through IDs... */
    for (n = 2; n < nargs; n++)
    {
        arg = argp(n);

        /* Check syntax */
        devid = strtoul(arg, &endptr, 10);
        if (endptr == arg  ||  *endptr != '\0')
        {
            p += sprintf(p, "setdrvlog: illegal devID: %s\n", arg);
            continue;
        }

        /* Check the id */
        if (devid < -1  ||  devid > numdevs)
        {
            p += sprintf(p, "setdrvlog: illegal devid: %d\n", devid);
            continue;
        }

        /* Perform modification */
        if (devid == -1) mask_p = &option_defdrvlog_mask;
        else             mask_p = d_logmask + devid;
        *mask_p = (*mask_p &~ log_clr_mask) | log_set_mask;
    }

 ERREXIT:
    if (p == buf)
        TellClient(cp2cd(cp), CXT_OK);
    else
        PrintToClient(cp, CXT_OK, CXS_ERROR, "%s", buf);
}


static int WhoIterator(v2clnt_t *cp, void *privptr)
{
  char **p_p = privptr;
  char  *p   = *p_p;

  char           from[100];

    if (cp->conntype != CLT_CONS) return 0;

    sprintf(from, "(%d.%d.%d.%d)",
                  (cp->ip >> 24) & 255,
                  (cp->ip >> 16) & 255,
                  (cp->ip >>  8) & 255,
                  (cp->ip >>  0) & 255);

    p += sprintf(p, "%-10s %-5d %-10u %-19s %s\n",
                 cp->username, cp2cd(cp), cp->ID,
                 stroftime(cp->when, " "),
                 (cp->ip == 0) ?  ""  :  from);
    *p_p = p;

    return 0;
}
static void CmdWho      (v2clnt_t *cp)
{
  char           buf[CX_MAX_DATASIZE];
  char          *p = buf;

    /* The header line */
    p += sprintf(p, "%-10s %-5s %-10s %-19s %s\n",
                 "USER", "LINE", "ID", "LOGIN-TIME", "FROM");
    /* List them... */
    ForeachV2connSlot(WhoIterator, &p);
    /* Send */
    PrintToClient(cp, CXT_OK, 0, "%s", buf);
}


static int WallIterator(v2clnt_t *cp, void *privptr)
{
  const char *buf = privptr;

    if (cp->conntype == CLT_CONS)
        PrintToClient(cp, CXT_ECHO, 0, "%s", buf);

    return 0;
}
static void CmdWall     (v2clnt_t *cp)
{
  char           buf[CX_MAX_DATASIZE];
  char          *p = buf;
  int            n;

    /* Header line */
    p += sprintf(p, "Cx: broadcast message from %s (line %d) %s:\n\n",
                 cp->username, cp2cd(cp), strcurtime());

    /* Append the message */
    for (n = 1; n < nargs; n++)
            p += sprintf(p, n == 1? "%s" : " %s", argp(n));
    p += sprintf(p, "\n");

    /* Send to all logged in */
    ForeachV2connSlot(WallIterator, buf);
    
    /* Confirm */
    TellClient(cp2cd(cp), CXT_OK);
}


static void CmdQ        (v2clnt_t *cp)
{
  char           buf[CX_MAX_DATASIZE];
  char          *p = buf;
  int            n;

    p += sprintf(p, "Available commands:\n\n");

    for (n = 0; commands[n].cmd != NULL; n++)
        p += sprintf(p, "%-10s %s\n",
                     commands[n].cmd, commands[n].description);

    PrintToClient(cp, CXT_OK, 0, "%s", buf);
}


static void ServeConsoleRequest  (v2clnt_t *cp, CxHeader *hdr, size_t inpktsize)
{
  char     *cmdline = hdr->data;
  char     *p;
  int       n;

    /* I. Some preliminary actions */

    /* Check if a command is really '\0'-terminated */
    if (cmdline[hdr->DataSize - 1] != '\0')
    {
        TellClient(cp2cd(cp), CXT_EINVAL);
        return;
    }

    /* First, truncate trailing whitespace */
    for (n = hdr->DataSize - 2;              /* Set n before '\0' */
         n >= 0  &&  isspace(cmdline[n]);    /* Is a space? */
         cmdline[n--] = '\0');               /* Move '\0' backward */
    hdr->DataSize = n + 2;                   /* Adjust DataSize */

    /* Second, skip leading whitespace */
    for (p = cmdline;
         *p != '\0'  &&  isspace(*p);
         p++);
    
    /* Was it an empty command? */
    if (*p == '\0')
    {
        TellClient(cp2cd(cp), CXT_OK);
        return;
    }


    /* II. Now, parse the command line... */

    /* First, extract the command */
    command = p;
    while (*p != '\0'  &&  !isspace(*p))  p++;
    *p++ = '\0';

    /* Second, skip the whitespace between the command and arguments */
    while (*p != '\0'  &&   isspace(*p))  p++;
    argstart = p;

    /* Here we can log the parsed line while it isn't too split :-) */
    /*!!! Currently it is commented-out, but we should select the 'level'
     value appropriately (in fact -- make a number of enums!!! */
    ////logline(LOGF_DBG1, 0, "Command: '%s', args '%s'", command, argstart);
    
    /* Third, split and count the arguments */
    nargs = 1;                                          /* [0] -- cmdname */
    while (*p != '\0')
    {
        ++nargs;
        while (*p != '\0'  &&  !isspace(*p)) p++;
        if (*p == '\0')  break;
        *p++ = '\0';
        while (*p != '\0'  &&   isspace(*p)) p++;
    }


    /* III. Finally, execute the command */
    for (n = 0; ; n++)
    {
        if (commands[n].cmd == NULL)
        {
            PrintToClient(cp, CXT_OK, CXS_ERROR,
                          "Unknown command: \"%s\"\n", command);
            break;
        }

        if (strcasecmp(command, commands[n].cmd) == 0)
        {
            /* Check for operator privileges if necessary */
            if (commands[n].priv && 1)
            {
                PrintToClient(cp, CXT_OK, CXS_ERROR,
                              "Only operator may use \"%s\"\n", command);
                break;
            }

            commands[n].func(cp);
            break;
        }
    }
}



//// cx-porter ///////////////////////////////////////////////////////

/*
 *  CreateSockets
 *          Creates two sockets which are to be listened, binds them
 *      to addresses and marks as "listening".
 *
 *  Effect:
 *      inet_entry  the socket is bound to INADDR_ANY.CX_INET_PORT
 *      unix_entry  the socket is bound to CX_UNIX_ADDR
 *
 *  Note:
 *      `inet_entry' should be bound first, since it will fail if
 *      CX_INET_PORT is busy. Otherwise the following will happen if
 *      the daemon is started twice: it unlink()s CX_UNIX_ADDR, bind()s
 *      this name to its own socket, and then fails to bind() inet_entry.
 *      So, it will just leave a working copy without local entry.
 */

static void Failure(const char *reason)
{
    fprintf(stderr, "cxsd: %s\n", reason);
    normal_exit = 1;
    exit(1);
}

static void CreateSockets(void)
{
  int                 r;		/* Result of system calls */
  struct sockaddr_in  iaddr;		/* Address to bind `inet_entry' to */
  struct sockaddr_un  uaddr;		/* Address to bind `unix_entry' to */
  int                 on     = 1;	/* "1" for REUSE_ADDR */
  char                unix_name[PATH_MAX]; /* Buffer to hold unix socket name */

    /* 1. INET socket */

    /* Create the socket */
    inet_entry = socket(AF_INET, SOCK_STREAM, 0);
    if (inet_entry < 0)
            Failure("socket(inet_entry)");

    /* Bind it to the name */
    setsockopt(inet_entry, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    iaddr.sin_family      = AF_INET;
    iaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    iaddr.sin_port        = htons(CX_INET_PORT + option_instance);
    r = bind(inet_entry, (struct sockaddr *) &iaddr,
             sizeof(iaddr));
    if (r < 0)
            Failure("bind(inet_entry)");

    /* Mark it as listening and non-blocking */
    listen(inet_entry, 5);
    set_fd_flags(inet_entry, O_NONBLOCK, 1);

    
    /* 2. UNIX socket */

    /* Create the socket */
    unix_entry = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_entry < 0)
            Failure("socket(unix_entry)");

    /* Bind it to the name */
    sprintf(unix_name, CX_UNIX_ADDR_FMT, option_instance); /*!!!snprintf()?*/
    unlink(unix_name);
    strcpy(uaddr.sun_path, unix_name);
    uaddr.sun_family = AF_UNIX;
    r = bind(unix_entry, (struct sockaddr *) &uaddr,
             offsetof(struct sockaddr_un, sun_path) + strlen(uaddr.sun_path));
    if (r < 0)
            Failure("bind(unix_entry)");

    /* Mark it as listening and non-blocking */
    listen(unix_entry, 5);
    set_fd_flags(unix_entry, O_NONBLOCK, 1);
    
    /* Set it to be world-accessible (bind() is affected by umask in Linux) */
    chmod(unix_name, 0777);

}

//////////////////////////////////////////////////////////////////////

static void WipeUnconnectedTimed(int uniq, void *unsdptr, sl_tid_t tid, void *privptr)
{
  int       cd = ptr2lint(privptr);
  v2clnt_t *cp = AccessV2connSlot(cd);

    cp->clp_tid = -1;
    DisconnectClient(cp, ERR_TIMOUT, CXT_TIMEOUT);
}

static void HandleClientRequest(v2clnt_t *cp, CxHeader *hdr, size_t inpktsize)
{
    cp->lastSeq = hdr->Seq;

    switch (hdr->Type)
    {
        case CXT_LOGOUT:
            DisconnectClient(cp, 0, 0);
            break;
    
        case CXT_DATA_REQ:
            if (cp->conntype == CLT_DATA)
                ServeDataRequest (cp, hdr, inpktsize);
            else
                ServeBigcRequest (cp, hdr, inpktsize);
            break;

        case CXT_SUBSCRIBE:
            ServeSubscribeRequest(cp, hdr, inpktsize);
            break;

        case CXT_EXEC_CMD:
            ServeConsoleRequest  (cp, hdr, inpktsize);
            break;

        case CXT_PING:
            ////logline(LOGF_SYSTEM, 0, "PING!");
            break;
            
        default:
            logline(LOGF_DBG1, 0, "%s(%d): Invalid request code: %d",
                    __FUNCTION__, cp2cd(cp), hdr->Type);
            TellClient(cp2cd(cp), CXT_EBADRQC);
    }
}

static void HandleClientConnect(v2clnt_t *cp, CxHeader *hdr, size_t inpktsize)
{
  CxLoginRec *lr = (void*)(hdr->data);

  struct      {CxHeader hdr; char messages[256];} resp;
  char                                           *p;

  CxHeader     *replybuf;
  CxHeader     *subscrbuf;
  int32        *subscrsetbuf;
  CxHeader     *lastreqbuf;
  bigc_listitem_t *bclip;

  char               *addr;	                /* IP in dot notation */
  const char         *conntype;
  struct sockaddr_in  inname;
  socklen_t           innamelen = sizeof(inname);

    /* 1. Sanity checking... */
    if (inpktsize != sizeof(CxHeader) + sizeof(*lr))
    {
        logline(LOGF_SYSTEM, 0, "inpktsize=%zd exp=%zd=%zd+%zd",
                inpktsize, sizeof(CxHeader) + sizeof(*lr), sizeof(CxHeader), sizeof(*lr));
        DisconnectClient(cp, ERR_HSHAKE, CXT_ACCESS_DENIED);
        return;
    }

    /*2. What do they want? */
    switch (hdr->Type)
    {
        case CXT_OPENBIGC:
            fdio_set_maxpktsize(cp->fhandle, CX_ABSOLUTE_MAX_DATASIZE);
        case CXT_LOGIN:
            cp->conntype = (hdr->Type == CXT_LOGIN)? CLT_DATA : CLT_BIGC;
            conntype = cp->conntype == CLT_DATA? "connect" : "bigconn";
            strzcpy(cp->progname, lr->password, sizeof(cp->progname));

            /* And allocate server-specific buffers */
            cp->replybufsize     = sizeof(CxHeader);
            cp->subscrbufsize    = sizeof(CxHeader);
            cp->subscrsetbufsize = sizeof(int32);
            cp->lastreqbufsize   = sizeof(CxHeader);
            
            replybuf     = malloc(cp->replybufsize);
            subscrbuf    = malloc(cp->subscrbufsize);
            subscrsetbuf = malloc(cp->subscrsetbufsize);
            lastreqbuf   = malloc(cp->lastreqbufsize);
            bclip        = malloc(sizeof(*bclip));

            if (replybuf == NULL  ||  subscrbuf == NULL  ||  subscrsetbuf == NULL  ||  lastreqbuf == NULL)
            {
                safe_free(replybuf);
                safe_free(subscrbuf);
                safe_free(subscrsetbuf);
                safe_free(lastreqbuf);
                safe_free(bclip);
                cp->subscrbufsize = cp->subscrsetbufsize = cp->lastreqbufsize = 0;
                DisconnectClient(cp, ENOMEM, CXT_ENOMEM);
                return;
            }
            
            cp->replybuf     = replybuf;
            cp->subscrbuf    = subscrbuf;
            cp->subscrsetbuf = subscrsetbuf;
            cp->lastreqbuf   = lastreqbuf;
            cp->bigclistitem_p = bclip;
            bzero(bclip, sizeof(*bclip));

            if (TellClientWParam(cp2cd(cp), CXT_READY, option_basecyclesize) < 0)
                return;
            break;
            
        case CXT_CONSOLE_LOGIN:
            cp->conntype = CLT_CONS;
            conntype = "login";

            cp->replybufsize     = sizeof(CxHeader);
            replybuf     = malloc(cp->replybufsize);
            if (replybuf == NULL)
            {
                safe_free(replybuf);
                DisconnectClient(cp, ENOMEM, CXT_ENOMEM);
                return;
            }
            cp->replybuf     = replybuf;

            bzero(&resp, sizeof(resp));
            resp.hdr.ID       = cp->ID;
            resp.hdr.Type     = CXT_READY;
            resp.hdr.Status   = CXS_PRINT | CXS_PROMPT;
            p  = resp.messages;

            /* Login "issue" */
            p += sprintf(p, "\nCX(r) Server release %d.%d",
                            CX_VERSION_MAJOR, CX_VERSION_MINOR);
            if ((CX_PATCH_OF(CX_VERSION) | CX_SNAP_OF(CX_VERSION)) != 0)
            {
                p += sprintf(p, ".%d", CX_PATCH_OF(CX_VERSION));
                if (CX_SNAP_OF(CX_VERSION) != 0)
                {
                    p += sprintf(p, ".%d", CX_SNAP_OF(CX_VERSION));
                }
            }
            p += sprintf(p, " (%s)\n\n", HostName()) + 1;

            /* Prompt */
            p += sprintf(p, "cx/%s:%d%c ",
                            HostName(), option_instance,
                            0 ?  '#'  :  '%') + 1;

            resp.hdr.DataSize = p - resp.messages;

            if (fdio_send(cp->fhandle, &resp, sizeof(resp.hdr) + resp.hdr.DataSize) < 0)
            {
                DisconnectClient(cp, errno, 0);
                return;
            };
            break;

        default:
            DisconnectClient(cp, ERR_HSHAKE, CXT_EBADRQC);
            return;
    }

    sl_deq_tout(cp->clp_tid);
    cp->clp_tid = -1;

    cp->uid = lr->uid;
    strzcpy(cp->username, lr->username, sizeof(cp->username));

    /* Log it */
    if (cp->ip == 0) addr = "-";
    else
    {
        /*!!! Note: we don't check the return value, since it *must* be 0 */
        getpeername(cp->fd, (struct sockaddr *)(&inname), &innamelen);
        addr = inet_ntoa(inname.sin_addr);
    }
    /* Sanity check... */
    if (addr == NULL)  addr = "KU-KA-RE-KU";
    /**/
    logline(LOGF_ACCESS, 0, "{%d} %s by %s from %s",
            cp2cd(cp),
            conntype,
            cp->username,
            addr);


    cp->state = CS_READY;
}

static void InteractWithClient(int uniq, void *unsdptr,
                               fdio_handle_t handle, int reason,
                               void *inpkt, size_t inpktsize,
                               void *privptr)
{
  int         cd   = ptr2lint(privptr);
  v2clnt_t   *cp   = AccessV2connSlot(cd);
  CxHeader   *hdr  = inpkt;

    switch (reason)
    {
        case FDIO_R_DATA:
            if (cp->state > CS_USABLE) HandleClientRequest(cp, hdr, inpktsize);
            else                       HandleClientConnect(cp, hdr, inpktsize);
            break;
            
        case FDIO_R_CLOSED:
            DisconnectClient(cp, ERR_CLOSED,           0);
            break;

        case FDIO_R_IOERR:
            DisconnectClient(cp, fdio_lasterr(handle), 0);
            break;

        case FDIO_R_PROTOERR:
            /* In fact, as of 10.09.2009, FDIO_R_PROTOERR is unused by fdiolib */
            DisconnectClient(cp, fdio_lasterr(handle), CXT_EBADRQC);
            break;

        case FDIO_R_INPKT2BIG:
            DisconnectClient(cp, ERR_IN2BIG,           CXT_EPKT2BIG);
            break;

        case FDIO_R_ENOMEM:
            DisconnectClient(cp, ENOMEM,               CXT_ENOMEM);
            break;

        default:
            logline(LOGF_ACCESS, 0, "%s::%s: unknown fdiolib reason %d",
                         __FILE__, __FUNCTION__, reason);
            DisconnectClient(cp, ERR_INTRNL,           CXT_EINTERNAL);
    }
}

static void RefuseConnection(int s, int32 cause)
{
  CxHeader  hdr;
    
    bzero(&hdr, sizeof(hdr));
    hdr.Type = cause;
    hdr.Res1 = htonl(CX_PROTO_VERSION);
    write(s, &hdr, sizeof(hdr));
}

static void AcceptCXv2Connection(int uniq, void *unsdptr,
                                 fdio_handle_t handle, int reason,
                                 void *inpkt, size_t inpktsize,
                                 void *privptr)
{
  int                 s;
  struct sockaddr_un  usrc;
  struct sockaddr_in  isrc;
  struct sockaddr    *clnt_addr;
  socklen_t           addrlen;

  int                 cd;
  v2clnt_t           *cp;
  fdio_handle_t       fhandle;
  
    if (reason != FDIO_R_ACCEPT)
    {
        logline(LOGF_SYSTEM, 0, "%s::%s(handle=%d): reason=%d, !=FDIO_R_ACCEPT=%d",
                __FILE__, __FUNCTION__, handle, reason, FDIO_R_ACCEPT);
        return;
    }

    /* First, accept() the connection */
    if (handle == unix_iohandle)
    {
        clnt_addr = (struct sockaddr *)&usrc;
        addrlen   = sizeof(usrc);
    }
    else
    {
        clnt_addr = (struct sockaddr *)&isrc;
        addrlen   = sizeof(isrc);
    }
    
    s = fdio_accept(handle, clnt_addr, &addrlen);

    /* Is anything wrong? */
    if (s < 0)
    {
        logline(LOGF_SYSTEM | LOGF_ERRNO, 0,
                "%s(): fdio_accept(%s)",
                __FUNCTION__,
                handle == unix_iohandle? "unix_iohandle" : "inet_iohandle");
        return;
    }

    if (0)
    {
        close(s);
        logline(LOGF_ACCESS, 0, "%s(%s)", __FUNCTION__, handle == unix_iohandle? "U":"I");
        return;
    }

    /* Instantly switch it to non-blocking mode */
    if (set_fd_flags(s, O_NONBLOCK, 1) < 0)
    {
        logline(LOGF_SYSTEM | LOGF_ERRNO, 0,
                "%s(): set_fd_flags(O_NONBLOCK)",
                __FUNCTION__);
        close(s);
        return;
    }

    /* Is it allowed to log in? */
    if (0)
    {
        RefuseConnection(s, CXT_ACCESS_DENIED);
        close(s);
        return;
    }

    /* Get a connection slot... */
    if ((cd = GetV2connSlot()) < 0)
    {
        RefuseConnection(s, CXT_MANY_CLIENTS);
        close(s);
        return;
    }

    /* ...and fill it with data */
    cp = AccessV2connSlot(cd);
    cp->state = CS_OPEN_LOGIN;
    cp->fd    = s;
    
    /* Okay, let's obtain an fdio slot... */
    cp->fhandle = fdio_register_fd(0, NULL, /*!!!uniq*/
                                   s, FDIO_STREAM,
                                   InteractWithClient, lint2ptr(cd),
                                   CX_MAX_DATASIZE,
                                   sizeof(CxHeader),
                                   FDIO_OFFSET_OF(CxHeader, DataSize),
                                   FDIO_SIZEOF   (CxHeader, DataSize),
                                   FDIO_LEN_LITTLE_ENDIAN,
                                   1, sizeof(CxHeader));
    if (cp->fhandle < 0)
    {
        RefuseConnection(s, CXT_MANY_CLIENTS);
        RlsV2connSlot(cd);
        return;
    }

    cp->clp_tid = sl_enq_tout_after(0, NULL, /*!!!uniq*/
                                    MAX_UNCONNECTED_TIME * 1000000,
                                    WipeUnconnectedTimed, lint2ptr(cd));
    if (cp->clp_tid < 0)
    {
        RefuseConnection(s, CXT_MANY_CLIENTS);
        RlsV2connSlot(cd);
        return;
    }

    cp->ID   = CreateNewID(cp->fd, &server_rcc);
    cp->when = time(NULL);
    cp->ip   = handle == unix_iohandle ?  0
                                       :  ntohl( *( (unsigned*) (&(isrc.sin_addr)) ) );

    TellClientVersion(cd, CXT_ACCESS_GRANTED);
}

//////////////////////////////////////////////////////////////////////

/*
 *  SendDataReply
 *      Sends a DATAREPLY packet to client.
 *      Before sending the packet fills with new data.
 */

static int SendDataReply        (v2clnt_t *cp)
{
  int           r;
  
    /* 1. Fill the packet with new data */
    FillDataPacket(cp, CXT_DATA_RPY);
    
    /* 2. Send the packet to client */
    r = fdio_send(cp->fhandle,
                  cp->replybuf,
                  cp->replybuf->DataSize + sizeof(CxHeader));
    if (r < 0)
    {
        DisconnectClient(cp, errno, 0);
        return r;
    }
    ////logline(LOGF_DBG1, 0, "%s(%d), Seq=%d", __FUNCTION__, s, cp->core.replybuf->Seq);
    
    /* Finally, mark a connection as free */
    cp->wasrequest = 0;

    return 0;
}

static int SendSubscription     (v2clnt_t *cp)
{
  int           r;
  
    /* 1. Fill the packet with new data */
    FillDataPacket(cp, CXT_SUBSCRIPTION);
    
    /* 2. Send the packet to client */
    r = fdio_send(cp->fhandle,
                  cp->subscrbuf,
                  cp->subscrbuf->DataSize + sizeof(CxHeader));
    if (r < 0)
    {
        DisconnectClient(cp, errno, 0);
        return r;
    }

    return 0;
}

static int SendBigcReply        (v2clnt_t *cp)
{
  int            r;

  ////logline(LOGF_DBG1, 0, __FUNCTION__);
  
    /* 2. Send the packet to client */
    r = fdio_send(cp->fhandle,
                  cp->replybuf,
                  cp->replybuf->DataSize + sizeof(CxHeader));
    if (r < 0)
    {
        DisconnectClient(cp, errno, 0);
        return -1;
    }
    
    /* Finally, mark a connection as free */
    cp->wasrequest = 0;

    return 0;
}

static void bigc_callback(int bigchan __attribute__((unused)), void *closure)
{
  int       cd = ptr2lint(closure);
  v2clnt_t *cp = AccessV2connSlot(cd);

    if (FillBigcReplyPacket(cp) < 0) return;
    if (cp->bigc_immediate)
        SendBigcReply(cp);
}


//// Public interface ////////////////////////////////////////////////

int  cxsd_fe_cxv2_activate (void)
{
    InitID(&server_rcc);
    CreateSockets();

    inet_iohandle = fdio_register_fd(0, NULL, /*!!!uniq*/
                                     inet_entry,       FDIO_LISTEN,
                                     AcceptCXv2Connection, NULL,
                                     0, 0, 0, 0, FDIO_LEN_LITTLE_ENDIAN, 0, 0);
    if (inet_iohandle < 0) Failure("fdio_register_fd(inet_entry)");
    unix_iohandle = fdio_register_fd(0, NULL, /*!!!uniq*/
                                     unix_entry,       FDIO_LISTEN,
                                     AcceptCXv2Connection, NULL,
                                     0, 0, 0, 0, FDIO_LEN_LITTLE_ENDIAN, 0, 0);
    if (unix_iohandle < 0) Failure("fdio_register_fd(unix_entry)");

    return 0;
}

static int OnBegOfCycle(v2clnt_t *cp, void *privptr)
{
    if (cp->hassubscription) RequestSubscription(cp);
    return 0;
}

void cxsd_fe_cxv2_cycle_beg(void)
{
    ForeachV2connSlot(OnBegOfCycle, NULL);
}

static int OnEndOfCycle(v2clnt_t *cp, void *privptr)
{
    if      (cp->conntype == CLT_DATA)
    {
        if (cp->wasrequest)      if (SendDataReply(cp)    < 0) return 0;
        if (cp->hassubscription) if (SendSubscription(cp) < 0) return 0;
    }
    else if (cp->conntype == CLT_BIGC)
    {
        if (cp->wasrequest  &&
            cp->replyisready)    if (SendBigcReply(cp)    < 0) return 0;
    }
    return 0;
}

void cxsd_fe_cxv2_cycle_end(void)
{
    ForeachV2connSlot(OnEndOfCycle, NULL);
}
