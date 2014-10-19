/*******************************************************************************
********************************************************************************
*
*
*  Register structures
*
********************************************************************************
*******************************************************************************/

typedef unsigned int unsigned32;
typedef unsigned short int unsigned16;
typedef unsigned char unsigned8;


typedef volatile struct {
    unsigned8 rsr;
    unsigned8 sypcr;
    unsigned8 swivr;
    unsigned8 swsr;
    unsigned16 par;
    unsigned8 irqpar;
    unsigned8 reserved1;
    unsigned8 pllcr;
    unsigned8 reserved2[3];
    unsigned8 mpark;
    unsigned8 reserved3[3];
    unsigned32 reserved4[12];
    unsigned32 ipr;
    unsigned32 imr;
    unsigned8 reserved5[3];
    unsigned8 avcr;
    unsigned8 icr[12];
    unsigned32 reserved6[10];
} sim_t;

typedef volatile struct {
    unsigned16 csar;
    unsigned16 reserved1;
    unsigned32 csmr;
    unsigned16 reserved2;
    unsigned16 cscr;
} cs_t;

typedef volatile struct {
    unsigned16 dcr;
    unsigned16 reserved1;
    unsigned32 reserved2;
    unsigned32 dacr0;
    unsigned32 dmr0;
    unsigned32 dacr1;
    unsigned32 dmr1;
    unsigned32 reserved3[10];
} dramc_t;

typedef volatile struct {
    unsigned16 tmr;
    unsigned16 reserved1;
    unsigned16 trr;
    unsigned16 reserved2;
    unsigned16 tcr;
    unsigned16 reserved3;
    unsigned16 tcn;
    unsigned16 reserved4;
    unsigned8 reserved5;
    unsigned8 ter;
    unsigned16 reserved6;
    unsigned32 reserved7[11];
} timer_t;

#define ucsr usr
#define utb urb
#define uacr uipcr
#define uimr uisr

typedef volatile struct {
    unsigned8 umr;
    unsigned8 reserved1[3];

    unsigned8 usr;
    unsigned8 reserved2[3];

    unsigned8 ucr;
    unsigned8 reserved3[3];

    unsigned8 urb;
    unsigned8 reserved4[3];

    unsigned8 uipcr;
    unsigned8 reserved5[3];

    unsigned8 uisr;
    unsigned8 reserved6[3];

    unsigned8 ubg1;
    unsigned8 reserved7[3];
    unsigned8 ubg2;
    unsigned8 reserved8[3];

    unsigned32 reserved9[4];

    unsigned8 uivr;
    unsigned8 reserved10[3];

    unsigned8 uip;
    unsigned8 reserved11[3];

    unsigned8 uop1;
    unsigned8 reserved12[3];

    unsigned8 uop0;
    unsigned8 reserved13[3];
} uart_t;

typedef volatile struct {
    unsigned8 madr;
    unsigned8 reserved1[3];
    unsigned8 mfdr;
    unsigned8 reserved2[3];
    unsigned8 mbcr;
    unsigned8 reserved3[3];
    unsigned8 mbsr;
    unsigned8 reserved4[3];
    unsigned8 mbdr;
    unsigned8 reserved5[3];
    unsigned32 reserved6[27];
} mbus_t;

typedef volatile struct {
    unsigned32 sar;
    unsigned32 dar;
    unsigned16 dcr;
    unsigned16 reserved1;
    unsigned16 bcr;
    unsigned16 reserved2;
    unsigned8 dsr;
    unsigned8 reserved3[3];
    unsigned8 divr;
    unsigned8 reserved4[3];
    unsigned32 reserved5[10];
} dma_t;

typedef volatile struct {
   sim_t sim;
   cs_t cs[8];
   unsigned32 reserver0[8];
   dramc_t dramc;
   timer_t timer[2];
   uart_t uart[2];
   unsigned32 reserved1;
   unsigned16 paddr;
   unsigned16 reserved2;
   unsigned16 padat;
   unsigned16 reserved3;
   unsigned32 reserved4[13];
   mbus_t mbus;
   dma_t dma[4];
} imm_t;

extern imm_t imm;
