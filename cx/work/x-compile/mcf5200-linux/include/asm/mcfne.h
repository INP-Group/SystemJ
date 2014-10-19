/****************************************************************************/

/*
 *	mcfne.h -- NE2000 in ColdFire eval boards.
 *
 *	(C) Copyright 1999, Greg Ungerer (gerg@moreton.com.au)
 *
 *      19990409 David W. Miller  Converted from m5206ne.h for 5307 eval board
 *
 *      Hacked support for m5206e Cadre III evaluation board
 *      Fred Stevens (fred.stevens@pemstar.com) 13 April 1999
 */

/****************************************************************************/
#ifndef	mcfne_h
#define	mcfne_h
/****************************************************************************/

#include <linux/config.h>

/*
 *	Everything in here is to remap the standard PC style
 *	ISA bus access to eval board style ISA bus access.
 */

/*
 *	Define the basic hardware resources of NE2000 board.
 *	The ISA bus is memory mapped on the eval board, with a
 *	really funky method for accessing odd addresses.
 */

#if defined(CONFIG_M5206)
/*
 *	Defines specific to the 5206 eval boards.
 */
#define NE2000_ADDR		0x40000000
#define NE2000_ODDOFFSET	0x00010000
#define	NE2000_IRQ		4
#define	NE2000_IRQ_VECTOR	0xf0
#define	NE2000_IRQ_PRIORITY	2
#define	NE2000_IRQ_LEVEL	4
#endif


#if defined(CONFIG_CADRE3) && defined(CONFIG_M5307)
#define NE2000_ADDR             0x40000000
#define NE2000_ODDOFFSET        0x00008000
#define NE2000_IRQ_VECTOR       0x1b

#elif defined(CONFIG_CADRE3)
#define	NE2000_ADDR		0x40000000
#define	NE2000_ODDOFFSET	0x00010000
#define	NE2000_IRQ_VECTOR	0x1c
#define	NE2000_IRQ		4
#define	NE2000_IRQ_PRIORITY	2
#define	NE2000_IRQ_LEVEL	4

#elif defined(CONFIG_M5307)
/*
 *	Defines for addressing setup of the 5307 eval board.
 */
#define NE2000_ADDR             0xfe600000
#define NE2000_ODDOFFSET        0x00010000
#define NE2000_IRQ              3
#define NE2000_IRQ_VECTOR       0x1b
#define NE2000_IRQ_PRIORITY     2
#define NE2000_IRQ_LEVEL        3
#endif

/*
 *	Define the PC type resources that the NE2000 board thinks
 *	it is using.
 */
#define NE2000_ISA_IO           0x300
#if defined(CONFIG_CADRE3) && defined(CONFIG_M5206e)
#define NE2000_ISA_IRQ          4
#else
#define NE2000_ISA_IRQ          3
#endif

/*
 *	Re-defines for ColdFire environment... The ne2000 board is
 *	mapped in sorta strangely, make adjustments for it.
 */
#undef outb
#undef outb_p
#undef inb
#undef inb_p

#define	outb	ne2000_outb
#define	inb	ne2000_inb
#define	outb_p	ne2000_outb
#define	inb_p	ne2000_inb
#define	outsb	ne2000_outsb
#define	outsw	ne2000_outsw
#define	insb	ne2000_insb
#define	insw	ne2000_insw


#ifndef COLDFIRE_NE2000_FUNCS

void ne2000_outb(unsigned int val, unsigned int addr);
int  ne2000_inb(unsigned int addr);
void ne2000_insb(unsigned int addr, void *vbuf, int unsigned long len);
void ne2000_insw(unsigned int addr, void *vbuf, unsigned long len);
void ne2000_outsb(unsigned int addr, void *vbuf, unsigned long len);
void ne2000_outsw(unsigned int addr, void *vbuf, unsigned long len);

#else

/*
 *	This macro converts a conventional register address into the
 *	real memory pointer on the SBC5206 eval board...
 *	This is only needed if the address can be odd!
 */
#define	NE2000_PTR(addr)					\
	((volatile unsigned short *) ((addr & 0x1) ?		\
		(NE2000_ADDR + NE2000_ODDOFFSET + addr - 1) :	\
		(NE2000_ADDR + addr)))


void ne2000_outb(unsigned int val, unsigned int addr)
{
	volatile unsigned short	*rp;

	rp = NE2000_PTR(addr);
	*rp = val;
}

int ne2000_inb(unsigned int addr)
{
	volatile unsigned short	*rp;

	rp = NE2000_PTR(addr);
	return(*rp);
}

void ne2000_insb(unsigned int addr, void *vbuf, int unsigned long len)
{
	volatile unsigned short	*rp;
	unsigned char		*buf;

	buf = (unsigned char *) vbuf;
	rp = NE2000_PTR(addr);
	for (; (len > 0); len--)
		*buf++ = *rp;
}

void ne2000_insw(unsigned int addr, void *vbuf, unsigned long len)
{
	volatile unsigned short	*rp;
	unsigned short		w, *buf;

	buf = (unsigned short *) vbuf;
	rp = (volatile unsigned short *) (NE2000_ADDR + addr);
	for (; (len > 0); len--) {
		w = *rp;
		*buf++ = ((w & 0xff) << 8) | ((w >> 8) & 0xff);
	}
}

void ne2000_outsb(unsigned int addr, const void *vbuf, unsigned long len)
{
	volatile unsigned short	*rp;
	unsigned char		*buf;

	buf = (unsigned char *) vbuf;
	rp = NE2000_PTR(addr);
	for (; (len > 0); len--)
		*rp = *buf++;
}

void ne2000_outsw(unsigned int addr, const void *vbuf, unsigned long len)
{
	volatile unsigned short	*rp;
	unsigned short		w, *buf;

	buf = (unsigned short *) vbuf;
	/*rp = NE2000_PTR(addr);*/
	rp = (volatile unsigned short *) (NE2000_ADDR + addr);
	for (; (len > 0); len--) {
		w = *buf++;
		*rp = ((w & 0xff) << 8) | ((w >> 8) & 0xff);
	}
}


/*
 *	Lastly the interrupt set up code...
 *	Minor diferences between the different board types.
 */

#if defined(CONFIG_M5206)
void ne2000_irqsetup(void)
{
	volatile unsigned char  *icrp;

	icrp = (volatile unsigned char *) (MCF_MBAR + MCFSIM_ICR4);
	*icrp = MCFSIM_ICR_LEVEL4 | MCFSIM_ICR_PRI2;
	mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_EINT4);
}
#endif

#if defined(CONFIG_CADRE3) && !defined(CONFIG_M5307)
void ne2000_irqsetup(void)
{
	volatile unsigned char  *icrp;

	icrp = (volatile unsigned char *) (MCF_MBAR + MCFSIM_ICR4);
	*icrp = MCFSIM_ICR_LEVEL4 | MCFSIM_ICR_PRI2 | MCFSIM_ICR_AUTOVEC;
	mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_EINT4);
}
#endif

#if defined(CONFIG_M5307)
void ne2000_irqsetup(void)
{
	mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_EINT3);
	printk("\nSIM: %08lx\n", mcf_getimr());
}
#endif

#endif /* COLDFIRE_NE2000_FUNCS */

/****************************************************************************/
#endif	/* mcfne_h */
