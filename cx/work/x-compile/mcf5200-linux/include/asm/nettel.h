/****************************************************************************/

/*
 *	nettel.h -- Moreton Bay NETtel support.
 *
 *	(C) Copyright 1999, Moreton Bay (www.moreton.com.au)
 */

/****************************************************************************/
#ifndef	nettel_h
#define	nettel_h
/****************************************************************************/

#include <linux/config.h>
#include <asm/coldfire.h>

#ifdef CONFIG_NETtel

/*
 *	Define LED bank address and flags...
 *	Different NETtel modems are configured a little differently.
 */

#ifdef CONFIG_M5307
#define	NETtel_LEDADDR		0x30400000

#define	NETtel_LEDVPN		0x01
#define	NETtel_LEDUSB2		0x02
#define	NETtel_LEDTX1		0x04
#define	NETtel_LEDRX1		0x08
#define	NETtel_LEDTX2		0x10
#define	NETtel_LEDRX2		0x20
#define	NETtel_LEDUSB1		0x40
#define	NETtel_LEDETH		0x80

/*
 *	The POWER and DIAG LEDs are on parallel I/O pins of 5307.
 *	The serial port DTR and DCD lines are also on the Parallel I/O.
 */
#define	NETtel_LEDPWR		0x0040
#define	NETtel_LEDDIAG		0x0020

#define	NETtel_DCD1		0x0001
#define	NETtel_DCD0		0x0002
#define	NETtel_DTR1		0x0004
#define	NETtel_DTR0		0x0008
#endif /* CONFIG_M5307 */


#ifdef CONFIG_M5206e
/*
 *	NETtel 1500 only has 4 LEDs.
 */
#define	NETtel_LEDADDR		0x50000000

#define	NETtel_LEDDIAG		0x01
#define	NETtel_LEDDCD		0x02
#define	NETtel_LEDOH		0x04
#define	NETtel_LEDDATA		0x08
#endif /* CONFIG_M5206e */


/*
 *	User space routines for back door access to the LEDs :-)
 *	This is a complete and utter bastard hack... But quick...
 *	The "on" pattern turns on some LEDs permanently. The "off"
 *	pattern turns some LEDs off permantly. The "flash" pattern
 *	will flash a particular led pattern.
 */
#define	LEDON(x)	*((unsigned char *) 0x3fe) |= (x)
#define	LEDOFF(x)	*((unsigned char *) 0x3fe) &= ~(x)
#define	LEDFLASH(x)	*((unsigned char *) 0x3ff) = (x)

#define	GETLED()	*((unsigned char *) 0x3fe)
#define	GETFLASH()	*((unsigned char *) 0x3ff)


/*
 *	Kernel macros to set and unset the LEDs.
 */
#ifndef __ASSEMBLY__
extern unsigned char	ledbank;
#ifdef CONFIG_M5307
extern unsigned short	ppdata;
#endif /* CONFIG_M5307 */
#endif /* __ASSEMBLY__ */

/*
 *	The following macros apply to the basic latched LEDs.
 *	(Not any other LEDs hanging of PP bits)
 */
#define	setled(x)							\
	*((volatile unsigned char *) NETtel_LEDADDR) = (ledbank &= ~(x))

#define	unsetled(x)							\
	*((volatile unsigned char *) NETtel_LEDADDR) = (ledbank |= (x))

#define	writeled(x)							\
	*((volatile unsigned char *) NETtel_LEDADDR) = (ledbank & ~(x))

#define	rewriteled()							\
	*((volatile unsigned char *) NETtel_LEDADDR) = ledbank


/*
 *	The following macros set and unset the POWER and DIAG LEDS.
 *	They are treated separate since they are on ColdFire PP lines
 *	on the NETtels > 2500.
 */
#ifdef CONFIG_M5206e
#define	setledpp(x)	setled(x)
#define	unsetledpp(x)	unsetled(x)
#else

#define	setledpp(x)							\
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_PADAT)) =	\
		(ppdata &= ~(x))

#define	unsetledpp(x)							\
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_PADAT)) =	\
		(ppdata |= (x))

#endif


/*
 *	Panic setup for NETtel. No console on the NETtel so it may be
 *	difficult to tell if the unit has paniced or not. So this little
 *	macro will turn on all the LEDS - for a visual indicator of panic.
 */
#define	nettel_panic()		{ cli(); setled(0xff); setledpp(0x60); }

#endif	/* CONFIG_NETtel */

/****************************************************************************/
#endif	/* nettel_h */
