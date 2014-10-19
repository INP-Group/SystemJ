#ifndef __REMDRV_PROTO_H
#define __REMDRV_PROTO_H


#include "cx.h"
#include "cx_version.h"


enum {REMDRV_DEFAULT_PORT = 8000};


enum
{
    REMDRVP_VERSION_MAJOR = 3,
    REMDRVP_VERSION_MINOR = 0,
    REMDRVP_VERSION = CX_ENCODE_VERSION(REMDRVP_VERSION_MAJOR, REMDRVP_VERSION_MINOR)
};


/*
 *  REMDRVP_xxx -- possible values of remdrv_pkt_header_t.command
 */

enum
{
    REMDRVP_CONFIG = 0x43666967, /* "Cfig" */
    REMDRVP_READ   = 0x52656164, /* "Read" */
    REMDRVP_RCSET  = 0x52435374, /* "RCSt" -- ReturnChanSet */
    REMDRVP_WRITE  = 0x46656564, /* "Feed" */
    REMDRVP_BIGC   = 0x42636f70, /* "Bcop" -- "Big Channel OPeration" */
    REMDRVP_DEBUGP = 0x44656267, /* "Debg" */
    REMDRVP_CRAZY  = 0x43727a79, /* "Crzy" */
    REMDRVP_RRUNDP = 0x52726450, /* "RrdP" -- rrund problem */
    REMDRVP_SETDBG = 0x53646267, /* "Sdbg" -- "Set DeBuG parameters" */
    REMDRVP_CHSTAT = 0x43685374, /* "ChSt" -- "CHange STate" (SetDevState) */
    REMDRVP_REREQD = 0x52727144, /* "RrqD" -- "ReReQuest Data" */
    REMDRVP_PING   = 0x50696e67, /* "Ping" -- application-ping */
};


typedef struct
{
    uint32  pktsize;
    uint32  command;
    uint32  cycle;
    union
    {
        struct {
            uint32  businfocount;
            uint32  proto_version;
        } config;
        struct {
            int32   start;
            int32   count;
            int32   set1;
            int32   set2;
        } rw;
        struct {
            int32   bigchan;
            int32   nparams;
            uint32  datasize;
            uint32  dataunits;
            uint32  rflags;
        } bigc;
        struct {
            uint32  level;
        } debugp;
        struct {
            int32   verblevel;
            uint32  mask;
        } setdbg;
        struct
        {
            int32   state;
            uint32  rflags_to_set;
        } chstat;
    } var;
    int32  data[0];
} remdrv_pkt_header_t;


enum {
    REMDRVP_MAXDATASIZE = (131071+CX_MAX_BIGC_PARAMS)*4,
    REMDRVP_MAXPKTSIZE  = sizeof(remdrv_pkt_header_t) + REMDRVP_MAXDATASIZE
};


#endif /* __REMDRV_PROTO_H */
