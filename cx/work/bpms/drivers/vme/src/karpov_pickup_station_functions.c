#include <stdio.h>

#include "misclib.h"
#include "bivme2_io.h"

#include "karpov_pickup_station_functions.h"

typedef struct
{
    int     iohandle;
    uint8   irq_n;
    uint8   irq_vect;
} karpov_privrec_t;

static karpov_privrec_t  global_karpov_privrec;
static karpov_privrec_t *global_karpov_privrec_p = NULL;

static uint16 IRQ_VECT_REG_PRG(uint8 irq_n, uint8 irq_on, uint8 irq_vect)
{
    irq_n    &= 0x7;
    irq_on   &= 0x1;
    irq_vect &= 0xff;

    uint16 w = ( (irq_vect << 9) | (irq_on << 3) | (irq_n << 0) );
#if 1
    fprintf(stderr, "%s) irq = 0x%04x\n", __FUNCTION__, w);
#endif
    return w;
}

static uint16 PS_CMD(uint8 cmd, uint8 modifier)
{
    return (uint16)( (cmd << 5) | (modifier & 0x1f) );
}

/* === */

int read_code (uint32 ofset, uint16 *data)
{
    karpov_privrec_t  *me = global_karpov_privrec_p;

    if ( me && data )
	   return bivme2_io_a16rd16(me->iohandle, ofset, data);
    return 0;
}

int write_code(uint32 ofset, uint16  data)
{
    karpov_privrec_t  *me = global_karpov_privrec_p;

    if ( me )
	   return bivme2_io_a16wr16(me->iohandle, ofset, data);
    return 0;
}

/* === */

int LinkD_TransmOutbuf(void)
{
    return write_code(strt_trnsm_addr, DATA_FAKE);
}

int LinkD_PrepareInbuf(void)
{
    return write_code(prep_inbuf_addr, DATA_FAKE);
}

/* === */

int LinkD_RegRead (int cell, uint16 *value)
{
    int r = 0;

    if ( 0 <= cell && cell <= 200 && value != NULL )
    {
        r = read_code (regs_cell_addr(cell), value);
    }
    else
    {
#if 1
        fprintf(stderr, "%s) Error!\n", __FUNCTION__);
#endif    
    }
    
    return r;
}

int LinkD_RegWrite(int cell, uint16  value)
{
    int r = 0;

    if ( 0 <= cell && cell <= 200 )
    {
	   r = write_code(regs_cell_addr(cell), value);
    }
    else
    {
        fprintf(stderr, "%s) Error!\n", __FUNCTION__);
    }

    return r;
}

/* === */

int LinkD_InbufCellRead  (int cell, uint16 *value)
{
    int r = 0;
    if ( value != NULL )
        r = read_code (inbuf_cell_addr(cell), value);
    return r;
}

int LinkD_InbufCellWrite (int cell, uint16  value)
{
    int r = 0;
    if ( 1 )
        r = write_code(inbuf_cell_addr(cell), value);
    return r;
}

int LinkD_OutbufCellRead (int cell, uint16 *value)
{
    int r = 0;
    if ( value != NULL )
        r = read_code (outbuf_cell_addr(cell), value);
    return r;
}

int LinkD_OutbufCellWrite(int cell, uint16  value)
{
    int r = 0;
    if ( 1 )
        r = write_code(outbuf_cell_addr(cell), value);
    return r;
}

/* === */

int LinkD_EnableIRQ(int on)
{
    karpov_privrec_t  *me = global_karpov_privrec_p;

    if (me)
        return LinkD_RegWrite(LD_REG_IRQ_VECT, IRQ_VECT_REG_PRG(me->irq_n, on, me->irq_vect));
    return 0;
}

int DropIRQ(void)
{
    return LinkD_RegWrite(LD_REG_DROP_CLR, DATA_FAKE);
}

/* === */

int LinkD_CheckLink2PS(void)
{
  uint16 w;
  int    r = 0;

    LinkD_RegRead(LD_REG_LNKSTATE, &w);
#if 0
    DoDriverLog(global_hack_devid, 0, "LD_REG_LNKSTATE = 0x%08x", w);
#endif
#if 0
    if ( w & LD_REG_LNKSTATE_FLAG_ENBL300MS )
        DoDriverLog(global_hack_devid, 0, "Link was present at least for last 300 ms");

    if ( w & LD_REG_LNKSTATE_FLAG_ETH100MBS )
        DoDriverLog(global_hack_devid, 0, "Link speed = 100 Mb/s");

    if ( w & LD_REG_LNKSTATE_FLAG_ENABLENOW )
        DoDriverLog(global_hack_devid, 0, "Link is  present just now");
#endif
    return r;
}

/* === */

static int TryToSend(int do_wait)
{
    int w = 0;
    /* 1. Configure transmisson procedure */
    /* 1.a. Drop ... */
    w &= LinkD_RegWrite(LD_REG_DROP_CLR, DATA_FAKE);

    /* 1.b. Set transsmission bufer size */
    w &= LinkD_RegWrite(LD_REG_WORD2OUT, 0x200 + 9);

    /* 1.c. Prepare inbuf for data receiving */
    w &= LinkD_PrepareInbuf();

    /* 1.d. Check link before send */
    LinkD_CheckLink2PS();

    /* 1.e. Real Start transmission */
    w &= LinkD_TransmOutbuf();

    /* 2. Wait */
    if (do_wait) SleepBySelect( 2 * 1000 );

    return w;
}

static int EnqueuePkt(uint16 regnum, uint16 value, int do_wait)
{
    int          it;
    uint16       pkt[9];

    /* 1. Fill and write pkt into transmission bufer */
    /* 1.a. Fill pkt header */
    for (it = 0; it < 9; it++)
    {
    	/* 1.b. Fill pkt data */
    	switch (it)
    	{
    	    case 0:  pkt[it] = 0x0000; break;
    	    case 1:  pkt[it] = 0x0000; break;
    	    case 2:  pkt[it] = 0x5555; break;
    	    case 3:  pkt[it] = 0x5555; break;
    	    case 4:  pkt[it] = 0x2412; break;
    	    case 5:  pkt[it] = regnum; break;
    	    case 6:  pkt[it] = value;  break;
    	    default: pkt[it] = 0x0000; break;
    	}

	   /* 2. Write pkt into memory */
        LinkD_OutbufCellWrite(it, pkt[it]);
#if 0
        DoDriverLog(global_hack_devid, 0, "%s) >OUTBUF [%d] = 0x%04x%s",
                    __FUNCTION__, it, pkt[it], it == 8 ? "\n++++++++++" : "");
#endif
    }

    /* Try to send pkt */
    return TryToSend(do_wait);
}

/* === */

void PS_RegWrite (uint16 ps_reg, uint16  value)
{
    EnqueuePkt(PS_CMD(PS_CMD_REG_WR, ps_reg), value,    SEND_NOT_WAIT);
}

void PS_RegRead  (uint16 ps_reg, uint16 *value)
{
    uint16 w1, w2;
#if 0 /* in future we need to read register after IRQ */
    LinkD_EnableIRQ(0);
    LinkD_RegWrite(LD_REG_DROP_CLR, DATA_FAKE);
    LinkD_PrepareInbuf();
    LinkD_EnableIRQ(1);
#endif
    EnqueuePkt(PS_CMD(PS_CMD_REG_RD, ps_reg), DATA_FAKE, SEND_AND_WAIT);

    /* real read here */
    LinkD_RegWrite(LD_REG_PAGE_SEL, 0);
    LinkD_InbufCellRead(1, &w1);
    LinkD_InbufCellRead(2, &w2);

    if ( value != NULL ) *value = w2;
}

/* === */

int PS_MeasStart(void)
{
    return EnqueuePkt(PS_CMD(PS_CMD_CYCLESTRT, PS_REG_FAKE), DATA_FAKE, SEND_NOT_WAIT);
}

int PS_MeasStop (void)
{
    return EnqueuePkt(PS_CMD(PS_CMD_CYCLESTOP, PS_REG_FAKE), DATA_FAKE, SEND_NOT_WAIT);
}

int PS_MeasAskTBT(uint8 line)
{
    return EnqueuePkt(PS_CMD(PS_CMD_TBT_RD,    line & 0x3 ), DATA_FAKE, SEND_AND_WAIT);
}

int PS_MeasAskSLW(void)
{
    return EnqueuePkt(PS_CMD(PS_CMD_SLW_RD,    PS_REG_FAKE), DATA_FAKE, SEND_AND_WAIT);
}

/* === */

void PS_InitParams  (void)
{
    /* Initialise Parameters */
}

/* === */

enum
{
    DIGITAL_QUANT = 5810, /* 5,811ns = 5811ps = separatrix duration */
    ANALOG_QUANT  = 10,   /* 0.010ns = 10ps   = delayunit quant     */

    MIN_PS        = 0,
    MAX_PS        = 13 * DIGITAL_QUANT + 581 * ANALOG_QUANT,
};

/* http://www.c.happycodings.com/Miscellaneous/code28.html */
static uint8 reverse(uint8 x)
{
    uint8 h = 0;
    uint8 i = 0;

    for (h = i = 0; i < 8; i++)
    {
        h = (h << 1) + (x & 1); 
        x >>= 1; 
    }

    return h;
}

static uint16 conv(uint16 val)
{
    uint16 w = 0;

    if( val & 128) w |= 1;
    if( val & 64 ) w |= 2;
    if( val & 32 ) w |= 4;
    if( val & 16 ) w |= 8;
    if( val & 8  ) w |= 16;
    if( val & 4  ) w |= 32;
    if( val & 2  ) w |= 64;
    if( val & 1  ) w |= 128;

    return (w & 0xff);
}

static uint32 DelaySanityCheck(uint32 value)
{
    if (value < MIN_PS) value = MIN_PS;
    if (value > MAX_PS) value = MAX_PS;

    value -= ( ( value % DIGITAL_QUANT ) % ANALOG_QUANT );
    return value;
}

static uint32 DelayStepSanityCheck(uint32 value)
{
    if (value < MIN_PS) value = MIN_PS;
    if (value > MAX_PS) value = MAX_PS;

    value -= ( value % ANALOG_QUANT );
    return (value < ANALOG_QUANT ? ANALOG_QUANT : value);
}

uint32 PS_DelayCalc(uint32 picos, uint16 *sep, uint16 *del)
{
    int    analog_picos;
    int    digital_part;
    int    analog_part;
    uint32 code;

    picos = DelaySanityCheck(picos);

    digital_part = picos / DIGITAL_QUANT;
    if (digital_part > 13)  digital_part = 13;

    analog_picos = picos - digital_part * DIGITAL_QUANT;

    analog_part  = analog_picos / ANALOG_QUANT;
    if (analog_part  > 581) analog_part  = 581;

    /* in future we will use only code instead of sep/del pair */
    code = digital_part * DIGITAL_QUANT + analog_part * ANALOG_QUANT;

    *sep = ( sep != NULL ? digital_part : 0);
    *del = ( del != NULL ?  analog_part : 0);

    return code;
}

static int code2delay(int code)
{
    int picos = code * 0;
    return picos;
}

uint32 PS_DelayWr(uint32  picos, int delay_set)
{
    uint16 w1, w2, w, sep, del;
    int    reg = 0;
    uint32 code;

    code = PS_DelayCalc(picos, &sep, &del);

    /* write digital part of delay */
    w2 = 255 - sep;
    w1 = conv(w2);
    w  = ( w1 << 8 ) | ( w1 << 0 );

    if ( 0 < delay_set && delay_set < 3 ) reg = PS_REG_SEP_DEL_12;
    if ( 2 < delay_set && delay_set < 5 ) reg = PS_REG_SEP_DEL_34;

    PS_RegWrite(reg, w);

    /* write analog  part of delay */
    w = del;
    if      ( delay_set == 1 ) reg = PS_REG_FINE_DEL_1;
    else if ( delay_set == 2 ) reg = PS_REG_FINE_DEL_2;
    else if ( delay_set == 3 ) reg = PS_REG_FINE_DEL_3;
    else if ( delay_set == 4 ) reg = PS_REG_FINE_DEL_4;
    else                       reg = PS_REG_FINE_DEL_1;

    PS_RegWrite(reg, w);

    return code;
}

uint32 PS_DelayRd(uint32 *picos __attribute__((unused)), int delay_set __attribute__((unused)))
{
    return 1;
}

int PS_Pts2AvrWr(int  value)
{
  uint16 w1, w2;
  int32  w;

    w = value;
    w1 = ( w >> 0 ) & 0xffff;
    w2 = ( w >> 16) & 0x00ff;
    PS_RegWrite(PS_REG_AVERAGE_LO, w1);
    PS_RegWrite(PS_REG_AVERAGE_HI, w2);

    return 1;
}

int PS_Pts2AvrRd(int *value __attribute__((unused)))
{
    return 1;
}

void InitKarpovLibrary(int iohandle, uint8 irq_n, uint8 irq_vect)
{
    global_karpov_privrec_p = &(global_karpov_privrec);

    global_karpov_privrec_p->iohandle = iohandle;
    global_karpov_privrec_p->irq_n    = irq_n;
    global_karpov_privrec_p->irq_vect = irq_vect;
}
