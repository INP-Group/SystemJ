/*********************************************************************
*                                                                    *
*  cx_proto.h                                                        *
*      CX client-server protocol definition                          *
*                                                                    *
*********************************************************************/

#ifndef __CX_PROTO_H
#define __CX_PROTO_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <sys/types.h>

#include "cx.h"
#include "cx_version.h"


enum {  CX_INET_PORT = 8012 }; /* 8008-8011 -- itus at bviii0-bviii3 */
enum {  CX_MAX_SERVER = 60  }; /* Max N of CX_INET_PORT+N */
#define CX_UNIX_ADDR_FMT   "/tmp/cx-%d-socket"

#define CX_ANONYMOUS "anonymous"

enum
{
    CX_PROTO_VERSION_MAJOR = 3,
    CX_PROTO_VERSION_MINOR = 0,
    CX_PROTO_VERSION_PATCH = 1,
    CX_PROTO_VERSION_SNAP  = 0,
    CX_PROTO_VERSION       = CX_ENCODE_VERSION_MmPs(CX_PROTO_VERSION_MAJOR,
                                                    CX_PROTO_VERSION_MINOR,
                                                    CX_PROTO_VERSION_PATCH,
                                                    CX_PROTO_VERSION_SNAP),
    
    CX_PROTO_VERSION_PINGABLE = CX_ENCODE_VERSION_MmPs(3, 0, 1, 0)
};


/* ==== Packet header stuff ======================================= */

typedef char cx_username_t[17];
typedef char cx_password_t[33];

typedef struct {
            uint32  ID;        /* Connection identifier */
            uint32  Seq;       /* Packet seq. number (client-side) */
            uint32  Type;      /* Packet type */
            uint32  Res1;      /* Reserved for future use #1; =Version in pkts from porter (in NETWORK byte order) */
            uint32  Res2;      /* Reserved for future use #2 */
            uint32  Status;    /* Status of the last _control_ operation */
            int32   NumData;   /* # of data forks */
            uint32  DataSize;  /* Size of all data forks */
            uint8   data[0];   /* Begin of the data */
        } CxHeader;

/* Max data size is 24K */
enum {
         CX_MAX_DATASIZE          = 1024 * 24,
         CX_MAX_PKTSIZE           = CX_MAX_DATASIZE + sizeof(CxHeader),
         CX_ABSOLUTE_MAX_DATASIZE = 1 << 24, /* 16M should be enough for all CS applications */
         CX_ABSOLUTE_MAX_PKTSIZE  = CX_ABSOLUTE_MAX_DATASIZE + sizeof(CxHeader)
     };

typedef struct {
            uint32           uid;
            cx_username_t    username;
            cx_password_t    password;
        } CxLoginRec;

/*
 *  CXT_xxx -- possible values of CxHeader.Type
 */
enum {
         /* Special endian-independent codes */
         CXT_LOGIN          =  1 * 0x01010101,  /* Request */
         CXT_OPENBIGC       =  2 * 0x01010101,  /* Request */
         CXT_CONSOLE_LOGIN  =  3 * 0x01010101,  /* Request */
         CXT_LOGOUT         = 10 * 0x01010101,  /* Request */
         CXT_ACCESS_GRANTED = 11 * 0x01010101,  /* Reply */
         CXT_ACCESS_DENIED  = 12 * 0x01010101,  /* Reply */
         CXT_TIMEOUT        = 13 * 0x01010101,  /* Reply */
         CXT_MANY_CLIENTS   = 14 * 0x01010101,  /* Reply */
         CXT_READY          = 15 * 0x01010101,  /* Reply */

         CXT_EBADRQC        = 20 * 0x01010101,  /* Reply & error code */
    
         CXT_PING           = 30 * 0x01010101,  /* "Request" */
         CXT_PONG           = 31 * 0x01010101,  /* "Reply" */
         
         /* Error codes */
         CXT_err            = 1000,
         CXT_OK             = CXT_err + 0,
         CXT_EINVAL         = CXT_err + 1,
         CXT_EPKT2BIG       = CXT_err + 3,
         CXT_EBUSY          = CXT_err + 4,
         CXT_ENOMEM         = CXT_err + 5,
         CXT_EINVCHAN       = CXT_err + 6,
         CXT_EINTERNAL      = CXT_err + 7, /* New code! */

         /* Responses/advices */
         CXT_resp           = 2000,
         CXT_AEXIT          = CXT_resp + 1,

         /* System management */
         CXT_mgmt           = 4000,
         CXT_EXEC_CMD       = CXT_mgmt + 1,
         CXT_ECHO           = CXT_mgmt + 2,

         /* Main services */
         CXT_serv           = 5000,

         CXT_chnserv        = CXT_serv + 100,  /* Channel services */
         CXT_DATA_REQ       = CXT_chnserv + 1,
         CXT_SUBSCRIBE      = CXT_chnserv + 2,
         CXT_DATA_RPY       = CXT_chnserv + 3,
         CXT_SUBSCRIPTION   = CXT_chnserv + 4,

         CXT_bigserv        = CXT_serv + 200,  /* Bigchan services */
         CXT_BIGC           = /*CXT_bigserv + 1*/CXT_DATA_REQ,
         CXT_BIGCRESULT     = /*CXT_bigserv + 2*/CXT_DATA_RPY,

         CXT_dbserv         = CXT_serv + 300,  /* Database services */
         CXT_DBREQUEST      = CXT_dbserv + 1,
         CXT_DBRESULT       = CXT_dbserv + 2,

         CXT_uncserv        = CXT_serv + 400,  /* Uncategorized services */
//         CXT_NAF            = CXT_uncserv + 1,
//         CXT_NAFRESULT      = CXT_uncserv + 2,
         
         /* Server-to-client commands */
         CXT_cmds           = 6000,
         CXT_DISCONNECT     = CXT_cmds + 1,
     };


/*
 *  CXS_xxx -- flags in CxHeader.Status
 */
enum {
         CXS_ENDIANID  = 0x12345678,    /* A word for initial client's endianness identification */
    
         CXS_NONE      = 0,
         CXS_ERROR     = 1 << 31,	/* Request wasn't successful */

         /* Flags for console requests */
         CXS_PRINT     = 1 << 0,	/* Printable is appended */
         CXS_PROMPT    = 1 << 1,        /* Prompt is appended */
     };


/* ==== Forks ===================================================== */

enum
{
    CXP_REQ_CHAR = '>',
    CXP_RPY_CHAR = '<',
};

#define CXP_CVT_TO_RPY(opcode)  (((opcode) & 0xFFFFFF00) | CXP_RPY_CHAR)
#define CXP_REQ_CMD(c2, c3, c4) ((uint32)(CXP_REQ_CHAR | (c2 << 8) | (c3 << 16) | (c4 << 24)))
#define CXP_RPY_CMD(c2, c3, c4) ((uint32)(CXP_RPY_CHAR | (c2 << 8) | (c3 << 16) | (c4 << 24)))

#define CXP_REQ_VAL(    c3, c4) (CXP_REQ_CMD('V', c3, c4))
#define CXP_REQ_BIG(    c3, c4) (CXP_REQ_CMD('B', c3, c4))

enum
{
    /* CXC_xxx -- possible values of CxDataFork.OpCode */
    CXC_GETVGROUP = CXP_REQ_VAL('g', 'G'), // ">VgG" Get sequental group of values
                                           CXC_GETVGROUP_R = CXP_CVT_TO_RPY(CXC_GETVGROUP),
    CXC_GETVSET   = CXP_REQ_VAL('g', 'S'), // ">VgS" Get random set of values
                                           CXC_GETVSET_R   = CXP_CVT_TO_RPY(CXC_GETVSET),
    CXC_SETVGROUP = CXP_REQ_VAL('s', 'G'), // ">VsG" Set sequental group of values
                                           CXC_SETVGROUP_R = CXP_CVT_TO_RPY(CXC_SETVGROUP),
    CXC_SETVSET   = CXP_REQ_VAL('s', 'S'), // ">VsS" Set random set of values
                                           CXC_SETVSET_R   = CXP_CVT_TO_RPY(CXC_SETVSET),
    CXC_ANY       = (unsigned) -1,         // "NotImportant" for DataForkSize()

    /* CXB_xxx -- possible values of CxBigcFork.OpCode */
    CXB_BIGCREQ   = CXP_REQ_BIG('r', 'q'), // ">Brq" Bigc request -- execute big command
                                           CXB_BIGCREQ_R   = CXP_CVT_TO_RPY(CXB_BIGCREQ),
};

typedef struct
{
    uint32  OpCode;            // Command code
    uint32  ByteSize;          // Total size of fork in bytes
    int32   ChanId;            // Regular/Big channel ID
    /*----*/
    int32   Count;

    int32   _padding_[4];

    uint8   data[0];
} CxDataFork;


/* Max chans in one fork -- in fact, just for sanity checks */
enum {
         CX_MAX_CHANS_IN_FORK = 8192
     };


/*
 *  CXDIR_xxx -- packet directions for DataForkSize()
 */
 
enum {
         CXDIR_REQUEST = 0,
         CXDIR_REPLY   = 1
     };


/*
 *  DataForkSize
 */

static inline size_t DataForkSize(uint32 Command, int32 Count, int direction)
{
    if      (direction == CXDIR_REPLY)
        return sizeof(CxDataFork) +
               (((
                  sizeof(uint32) + sizeof(uint32) + sizeof(tag_t)
                 ) * Count + 3) &~ 3U);
    else if (direction == CXDIR_REQUEST)
    {
        if      (Command == CXC_SETVGROUP)
            return sizeof(CxDataFork) +
                   sizeof(uint32) * Count;
        else if (Command == CXC_SETVSET)
            return sizeof(CxDataFork) +
                   sizeof(uint32) * Count * 2;
        else if (Command == CXC_GETVGROUP)
            return sizeof(CxDataFork);
        else if (Command == CXC_GETVSET)
            return sizeof(CxDataFork) +
                   sizeof(uint32) * Count;
        else return 1/0;
    }
    else return 2/0;
}

/*
 *  CxDataFork reQuest "accessors"
 */

static inline int32 *CDFQ_A1(CxDataFork *f) {return (int32 *)(f->data);}
static inline int32 *CDFQ_A2(CxDataFork *f) {return (int32 *)(f->data + f->Count * sizeof(int32));}

/*
 *  CxDataFork replY "accessors"
 */

static inline int32 *CDFY_VALUES(CxDataFork *f) {return (int32 *)(f->data);}
static inline int32 *CDFY_RFLAGS(CxDataFork *f) {return (int32 *)(f->data + f->Count * sizeof(int32));}
static inline tag_t *CDFY_TAGS  (CxDataFork *f) {return (tag_t *)(f->data + f->Count * sizeof(int32) * 2);}


/* ==== Big channels ============================================== */

typedef struct
{
    uint32  OpCode;
    uint32  ByteSize;
    int32   ChanId;      // Bigc #
    /*----*/
    uint32  nparams16x2; // # of int32 parameters; 2bytes+2bytes?
    uint32  datasize;    // Size (in bytes) of data
    uint32  dataunits;   // Units of that data (=sizeof(unit), may be 0 if datasize==0)
    union {
        struct {
            uint32  ninfo_n_flags; // `ninfo' from client and request flags -- {cachectl, immediate, ...}
            uint32  retbufsize;    // `retbufsize' from client
        } q;
        struct {
            uint32  rflags;
            uint32  tag;
        } y;
    } u;
    int32   Zdata[0];
} CxBigcFork;


enum
{
    CXBC_FLAGS_NINFO_SHIFT    = 0,  CXBC_FLAGS_NINFO_MASK     = 0x0000FFFF,
    CXBC_FLAGS_CACHECTL_SHIFT = 16, CXBC_FLAGS_CACHECTL_MASK  = 0x00FF0000,
    CXBC_FLAGS_IMMED_SHIFT    = 24, CXBC_FLAGS_IMMED_FLAG     = 1 << CXBC_FLAGS_IMMED_SHIFT,
};


static inline size_t BigcForkSize(int nparams, size_t datasize)
{
    return sizeof(CxBigcFork)  +
           nparams * sizeof(int32) +
           ((datasize + 3) &~ 3U);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CX_PROTO_H */
