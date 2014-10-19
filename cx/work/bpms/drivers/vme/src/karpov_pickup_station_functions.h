#ifndef __KARPOV_PICKUP_STATION_FUNCTIONS_H
#define __KARPOV_PICKUP_STATION_FUNCTIONS_H

/* === !!! DO NOT EDIT !!! === */

enum {ADDRESS_MODIFIER = 0xd};

enum
{
    REGS_PTR       = 0x000,
    OUTBUF_PTR     = 0x200,
    INBUF_PTR      = 0x400,
    STRT_TRNSM_PTR = 0x600,
    PREP_INBUF_PTR = 0x800,
};

#define     regs_cell_addr(m)   (  REGS_PTR     + 2*(m))
#define   outbuf_cell_addr(m)   (OUTBUF_PTR     + 2*(m))
#define    inbuf_cell_addr(m)   ( INBUF_PTR     + 2*(m))
#define     strt_trnsm_addr     (STRT_TRNSM_PTR + 2*(0))
#define     prep_inbuf_addr     (PREP_INBUF_PTR + 2*(0))

enum
{
    DATA_FAKE     = 0,

    SEND_NOT_WAIT = 0,
    SEND_AND_WAIT = 1,
};

enum
{
    LD_REG_PAGE_SEL = 0,
    LD_REG_WORD2OUT = 1,
    LD_REG_IRQ_VECT = 2,
    LD_REG_LNKSTATE = 14,
    LD_REG_DROP_CLR = 18,
};

enum
{
    LD_REG_LNKSTATE_FLAG_ENBL300MS = 1 << 0,
    LD_REG_LNKSTATE_FLAG_ETH100MBS = 1 << 1,
    LD_REG_LNKSTATE_FLAG_ENABLENOW = 1 << 2,
};

enum
{
    PS_REG_STAT_CONFG = 0, PS_REG_FAKE = 0,

    PS_REG_AVERAGE_LO = 1,
    PS_REG_AVERAGE_HI = 2,

    PS_REG_DECIMATION = 3,

    PS_REG_SYNC_DELAY = 4,

    PS_REG_DEL_CH_SEL = 5,

    PS_REG_SEP_DEL_12 = 6,
    PS_REG_SEP_DEL_34 = 7,

    PS_REG_FINE_DEL_1 = 8,
    PS_REG_FINE_DEL_2 = 9,
    PS_REG_FINE_DEL_3 = 10,
    PS_REG_FINE_DEL_4 = 11,

    PS_REG_INDV_DEL_1 = 12,
    PS_REG_INDV_DEL_2 = 13,
    PS_REG_INDV_DEL_3 = 14,
    PS_REG_INDV_DEL_4 = 15,
};

enum
{
    PS_REG_STAT_FLAG_ATTENN_ON = 1 << 0,
    PS_REG_STAT_FLAG_CALIBR_ON = 1 << 1,
    PS_REG_STAT_FLAG_EXT_START = 1 << 2,
};

enum
{
    PS_CMD_REG_WR    = 0,
    PS_CMD_REG_RD    = 4,

    PS_CMD_CYCLESTRT = 3,
    PS_CMD_CYCLESTOP = 5,

    PS_CMD_SLW_RD    = 2,
    PS_CMD_TBT_RD    = 1,
};

/* Link Device API functions */

int    LinkD_TransmOutbuf(void);
int    LinkD_PrepareInbuf(void);

int    LinkD_RegRead (int cell, uint16 *value);
int    LinkD_RegWrite(int cell, uint16  value);

int    LinkD_InbufCellRead  (int cell, uint16 *value);
int    LinkD_InbufCellWrite (int cell, uint16  value);

int    LinkD_OutbufCellRead (int cell, uint16 *value);
int    LinkD_OutbufCellWrite(int cell, uint16  value);

int    LinkD_EnableIRQ(int on);
int    DropIRQ(void);

int    LinkD_CheckLink2PS(void);

/* Pickup Station API functions */

void   PS_RegWrite (uint16 ps_reg, uint16  value);
void   PS_RegRead  (uint16 ps_reg, uint16 *value);

int    PS_MeasStart(void);
int    PS_MeasStop (void);
int    PS_MeasAskTBT(uint8 line);
int    PS_MeasAskSLW(void);

uint32 DelaySanityCheck(uint32 value);
uint32 DelayStepSanityCheck(uint32 value);

uint32 PS_DelayWr(uint32  picos, int delay_set);
uint32 PS_DelayRd(uint32 *picos, int delay_set);

int    PS_Pts2AvrWr(int  value);
int    PS_Pts2AvrRd(int *value);

/* Auxilary Library functions */

void   InitKarpovLibrary(int iohandle, uint8 irq_n, uint8 irq_vect);

#endif /* __KARPOV_PICKUP_STATION_FUNCTIONS_H */
