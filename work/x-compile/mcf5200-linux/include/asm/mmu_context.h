#ifndef __68K_MMU_CONTEXT_H
#define __68K_MMU_CONTEXT_H

#include <linux/config.h>

/*
 * get a new mmu context.. do we need this on the m68k?
 */

#ifdef CONFIG_NETtel
#define get_mmu_context(x) do { 	\
    *(volatile unsigned short *)0xffe00248 &= 0xfffe; /* turn off the LED */ \
} while (0)
#else
#define get_mmu_context(x) do { } while (0)
#endif

#endif
