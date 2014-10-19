#ifndef __CX_PROTO_V4_H
#define __CX_PROTO_V4_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "cx.h"
#include "cx_version.h"


enum {  CX_V4_INET_PORT    = 8012 };
#define CX_V4_UNIX_ADDR_FMT "/tmp/cxv4-%d-socket"

enum
{
    CX_V4_PROTO_VERSION_MAJOR = 4,
    CX_V4_PROTO_VERSION_MINOR = 0,
    CX_V4_PROTO_VERSION = CX_ENCODE_VERSION(CX_V4_PROTO_VERSION_MAJOR,
                                            CX_V4_PROTO_VERSION_MINOR)
};


/* ==== Packet header stuff ======================================= */

typedef struct
{
    uint32  DataSize;   // Size of data (w/o header!) in bytes
    uint32  Type;       // Packet type
    uint32  ID;         // Connection identifier (assigned by server)
    uint32  Seq;        // Packet seq. number (client-side)
    int32   NumChunks;  // # of data chunks
    uint32  Status;     // Status of operation (in response packets)
    uint32  var1;       // Type-dependent #1; =0x12345678 in CONNECT packets
    uint32  var2;       // Type-dependent #2; =CX_V4_PROTO_VERSION in CONNECT packets
    uint8   data[0];    // Begin of the data
} CxV4Header;

enum
{
    CX_V4_MAX_DATASIZE = 4 * 1048576,
    CX_V4_MAX_PKTSIZE  = CX_V4_MAX_DATASIZE + sizeof(CxV4Header),
};

/*** CXT4_nnn -- possible values of CxV4Header.Type *****************/
enum
{
    /* Special endian-independent codes */
    CXT4_EBADRQC        =  1 * 0x01010101,
    CXT4_DISCONNECT     =  2 * 0x01010101,
    
    CXT4_ENDIANID       = 10 * 0x01010101,
    CXT4_LOGIN          = 11 * 0x01010101,
    CXT4_LOGOUT         = 12 * 0x01010101,

    CXT4_READY          = 20 * 0x01010101,
    CXT4_ACCESS_GRANTED = 21 * 0x01010101,
    CXT4_ACCESS_DENIED  = 22 * 0x01010101,
    CXT4_DIFFPROTO      = 23 * 0x01010101,
    CXT4_TIMEOUT        = 24 * 0x01010101,
    CXT4_MANY_CLIENTS   = 25 * 0x01010101,
    CXT4_EINVAL         = 26 * 0x01010101,
    CXT4_ENOMEM         = 27 * 0x01010101,
    CXT4_EPKT2BIG       = 28 * 0x01010101,
    CXT4_EINTERNAL      = 29 * 0x01010101,
    CXT4_PING           = 30 * 0x01010101,  /* "Request" */
    CXT4_PONG           = 31 * 0x01010101,  /* "Reply" */

    /* Regular error codes */
    CXT4_err            = 1000,
    CXT4_OK             = CXT4_err + 0,
};

enum {CXV4_VAR1_ENDIANNESS_SIG = 0x12345678};


/**/

typedef struct
{
    uint32  OpCode;
    uint32  ByteSize;
    int32   param1;
    int32   param2;
    uint8   data[0];
} CxV4Chunk;



/**/

typedef struct
{
    CxV4Chunk ck;
    char      progname[40];
    char      username[40];
} CxV4LoginRec;


/**/
typedef struct
{
    CxV4Chunk  ck;
    int32      cpid;   // ControlPoint ID -- the "key"
    int32      hwid;   // HWchan ID -- what to use for read/write ops
    //??? Maybe pack rw+dtype somehow?
    int32      rw;     // 0 -- readonly, 1 -- read/write
    int32      dtype;  // cxdtype_t, with 3 high bytes of 0s
    int32      nelems; // Max # of units; ==1 for scalar channels
    //??? color and other properties?  Or use other requests for individual parameters?
    int32      zzz;
} ___CxV4CpointPropsRec___;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CX_PROTO_V4_H */
