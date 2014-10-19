#ifndef __REMDRV_PROTO_H
#define __REMDRV_PROTO_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "cx.h"
#include "cx_version.h"


enum {REMDRV_DEFAULT_PORT = 8000};

enum
{
    REMDRV_PROTO_VERSION_MAJOR = 4,
    REMDRV_PROTO_VERSION_MINOR = 0,
    REMDRV_PROTO_VERSION = CX_ENCODE_VERSION(REMDRV_PROTO_VERSION_MAJOR,
                                             REMDRV_PROTO_VERSION_MINOR)
};


enum
{
    REMDRV_C_CONFIG = 0x43666967,  /* "Cfig" */
    REMDRV_C_DEBUGP = 0x44656267,  /* "Debg" */
    REMDRV_C_CRAZY  = 0x43727a79,  /* "Crzy" */
    REMDRV_C_RRUNDP = 0x52726450,  /* "RrdP" -- rrund problem */
    REMDRV_C_CHSTAT = 0x43685374,  /* "ChSt" -- "CHange STate" (SetDevState) */
    REMDRV_C_REREQD = 0x52727144,  /* "RrqD" -- "ReReQuest Data" */
    REMDRV_C_PING_R = 0x50696e67,  /* "Ping" -- application-ping */
    REMDRV_C_PONG_Y = 0x506f6e67,  /* "Pong" (reply) */
};


typedef struct
{
    uint32  pktsize;
    uint32  command;

    union
    {
        struct
        {
            uint32  businfocount;
            uint32  proto_version;
        } config;
        struct
        {
            uint32  level;
        } debugp;
        struct
        {
            int32   status;
            uint32  rflags_to_set;
        } chstat;
    }       var;
    
    int32   data[0];
} remdrv_pkt_header_t;


enum
{
    REMDRV_MAXDATASIZE = 4 * 1048576,
    REMDRV_MAXPKTSIZE  = sizeof(remdrv_pkt_header_t) + REMDRV_MAXDATASIZE,
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __REMDRV_PROTO_H */
