/****************************************************************************/

/*
 *	coldfire.h -- Motorola ColdFire CPU sepecific defines
 *
 *	(C) Copyright 1999-2000, Greg Ungerer (gerg@moreton.com.au)
 */

/****************************************************************************/
#ifndef	coldfire_h
#define	coldfire_h
/****************************************************************************/

#include <linux/config.h>

/*
 *	Define the processor support peripherals base address.
 *	This is generally setup by the boards start up code.
 */
#define	MCF_MBAR	0xffe00000

/*
 *	Define master clock frequency.
 */
#define	MCF_CLK		33333333

/****************************************************************************/
#endif	/* coldfire_h */
