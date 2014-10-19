/****************************************************************************/

/*
 *	elia.h -- Moreton Bay eLIA platform support.
 *
 *	(C) Copyright 1999-2000, Moreton Bay (www.moreton.com.au)
 */

/****************************************************************************/
#ifndef	elia_h
#define	elia_h
/****************************************************************************/

#include <linux/config.h>
#include <asm/coldfire.h>

#ifdef CONFIG_eLIA

/*
 *	The eLIA board has 2 programmable LEDs. We use one as the
 *	alive indicator, the other is user configurable.
 *	The serial port DTR and DCD lines are also on the Parallel I/O
 *	as well, so define those too.
 */
#define	eLIA_LEDDIAG		0x2000
#define	eLIA_USER		0x1000

#define	eLIA_DCD1		0x0001
#define	eLIA_DCD0		0x0002
#define	eLIA_DTR1		0x0004
#define	eLIA_DTR0		0x0008


/*
 *	User space routines for back door access to the LEDs :-)
 *	This is a complete and utter bastard hack... But quick...
 *	The "on" pattern turns on some LEDs permanently. The "off"
 *	pattern turns some LEDs off permantly. The "flash" pattern
 *	will flash a particular LED pattern.
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
extern unsigned short	ppdata;
#endif /* __ASSEMBLY__ */

/*
 *	The following macros set and unset the LEDS.
 */
#define	setledpp(x)							\
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_PADAT)) =	\
		(ppdata &= ~(x))

#define	unsetledpp(x)							\
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_PADAT)) =	\
		(ppdata |= (x))


/*
 *	Panic setup for eLIA. There may be no console so it may be
 *	difficult to tell if the kernel has paniced or not. So this little
 *	macro will turn on all the LEDS - for a visual indicator of panic.
 */
#define	nettel_panic()		{ cli(); setledpp(0x3000); }

#endif	/* CONFIG_eLIA */

/****************************************************************************/
#endif	/* elia_h */
