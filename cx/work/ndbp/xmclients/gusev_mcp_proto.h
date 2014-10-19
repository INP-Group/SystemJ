#ifndef __GUSEV_MCP_PROTO_H
#define __GUSEV_MCP_PROTO_H


typedef struct
{
    uint32  len;
    uint32  magic;
    int8    ID[10];
    uint16  SqN;
    uint16  OpC;
    uint16  gusStatus;
} __attribute__((packed)) gusev_pkt_header;

enum {GUS_MAGIC = 0x55555555};

enum
{
    GUS_CMD_SETVALUE       = 1,
    GUS_CMD_GETALLDATA     = 2,
    GUS_CMD_SETOSCILLPARS  = 3,
    GUS_CMD_GETOSCILL      = 4,
    GUS_CMD_ALFO           = 5,
    GUS_CMD_STARTCAMERA    = 10,
    GUS_CMD_SETIMGPARS     = 11,
    GUS_CMD_GETIMG         = 12,
};

typedef struct
{
    gusev_pkt_header  hdr;
    uint16            n;
    double            value __attribute__((packed));
} __attribute__((packed)) gusev_pkt_SETVALUE_t;

typedef struct
{
    gusev_pkt_header  hdr;
    uint16            brightness;
    uint16            exposition;
} __attribute__((packed)) gusev_pkt_SETIMGPARS_t;


#endif /* __GUSEV_MCP_PROTO_H */
