#ifndef _M68K_DMA_H
#define _M68K_DMA_H 1
 
#include <linux/config.h>

#ifdef CONFIG_COLDFIRE
/*
 * ColdFire DMA Model:
 *   ColdFire DMA supports two forms of DMA: Single and Dual address. Single
 * address mode emits a source address, and expects that the device will either
 * pick up the data (DMA READ) or source data (DMA WRITE). This implies that
 * the device will place data on the correct byte(s) of the data bus, as the
 * memory transactions are always 32 bits. This implies that only 32 bit
 * devices will find single mode transfers useful. Dual address DMA mode
 * performs two cycles: source read and destination write. ColdFire will
 * align the data so that the device will always get the correct bytes, thus
 * is useful for 8 and 16 bit devices. This is the mode that is supported
 * below.
 */

#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#include <asm/mcfdma.h>

/*
 * Set number of channels of DMA on ColdFire for different implementations
 */
#if defined(CONFIG_M5307)
#define MAX_DMA_CHANNELS 4
#else
#define MAX_DMA_CHANNELS 2
#endif

extern unsigned int dma_base_addr[];

/* Storage for where to write/read DMA data to/from */
unsigned int dma_device_address[MAX_DMA_CHANNELS];

#define DMA_MODE_WRITE_BIT 0x01  /* Memory/IO to IO/Memory select */
#define DMA_MODE_WORD_BIT  0x02  /* 8 or 16 bit transfers */

/* I/O to memory, 8 bits, mode */
#define DMA_MODE_READ	         0
/* memory to I/O, 8 bits, mode */
#define DMA_MODE_WRITE	         1
/* I/O to memory, 16 bits, mode */
#define DMA_MODE_READ_WORD       2
/* memory to I/O, 16 bits, mode */
#define DMA_MODE_WRITE_WORD      3

/* enable/disable a specific DMA channel */
static __inline__ void enable_dma(unsigned int dmanr)
{
  volatile unsigned short *dmawp;

  dmawp = (unsigned short *) dma_base_addr[dmanr];
  dmawp[MCFDMA_DCR] |= MCFDMA_DCR_EEXT;

}

static __inline__ void disable_dma(unsigned int dmanr)
{
  volatile unsigned short *dmawp;
  volatile unsigned char  *dmapb;

  dmawp = (unsigned short *) dma_base_addr[dmanr];
  dmapb = (unsigned char *) dma_base_addr[dmanr];

  /* Turn off external requests, and stop any DMA in progress */
  dmawp[MCFDMA_DCR] &= ~MCFDMA_DCR_EEXT;
  dmapb[MCFDMA_DSR] = MCFDMA_DSR_DONE;
}

/* Clear the 'DMA Pointer Flip Flop'.
 * Write 0 for LSB/MSB, 1 for MSB/LSB access.
 * Use this once to initialize the FF to a known state.
 * After that, keep track of it. :-)
 * --- In order to do that, the DMA routines below should ---
 * --- only be used while interrupts are disabled! ---
 *
 * This is a NOP for ColdFire. Provide a stub for compatibility.
 */

static __inline__ void clear_dma_ff(unsigned int dmanr)
{
}

/* set mode (above) for a specific DMA channel */
static __inline__ void set_dma_mode(unsigned int dmanr, char mode)
{
  volatile unsigned char  *dmabp;
  volatile unsigned short *dmawp;

  dmabp = (unsigned char *) dma_base_addr[dmanr];
  dmawp = (unsigned short *) dma_base_addr[dmanr];

  // Clear config errors
  dmabp[MCFDMA_DSR] = MCFDMA_DSR_DONE; 

  // Set command register
  dmawp[MCFDMA_DCR] =
    MCFDMA_DCR_INT |         // Enable completion irq
    MCFDMA_DCR_CS |          // Force one xfer per request
    MCFDMA_DCR_AA |          // Enable auto alignment
    // Memory to I/O or I/O to Memory
    ((mode & DMA_MODE_WRITE_BIT) ? MCFDMA_DCR_SINC : MCFDMA_DCR_DINC) |
    // 16 bit or 8 bit transfers
    ((mode & DMA_MODE_WORD_BIT)  ? MCFDMA_DCR_SSIZE_WORD :
                                   MCFDMA_DCR_SSIZE_BYTE) |
    ((mode & DMA_MODE_WORD_BIT)  ? MCFDMA_DCR_DSIZE_WORD : 
                                   MCFDMA_DCR_DSIZE_BYTE) ;

#ifdef DMA_DEBUG
  printk("%s: Setting stat %x: %x ctrl %x: %x regs for chan %d\n",
         __FUNCTION__, 
	 &dmabp[MCFDMA_DSR], dmabp[MCFDMA_DSR],
	 &dmawp[MCFDMA_DCR], dmawp[MCFDMA_DCR],
	 dmanr);
#endif
}
/* Set transfer address for specific DMA channel */
static __inline__ void set_dma_addr(unsigned int dmanr, unsigned int a) {
  volatile unsigned short *dmawp;
  volatile unsigned int   *dmalp;

  dmawp = (unsigned short *) dma_base_addr[dmanr];
  dmalp = (unsigned int *) dma_base_addr[dmanr];

  // Determine which address registers are used for memory/device accesses
  if (dmawp[MCFDMA_DCR] & MCFDMA_DCR_SINC) {
    // Source incrementing, must be memory
    dmalp[MCFDMA_SAR] = a;
    // Set dest address, must be device
    dmalp[MCFDMA_DAR] = dma_device_address[dmanr];
  }
  else {
    // Destination incrementing, must be memory
    dmalp[MCFDMA_DAR] = a;
    // Set source address, must be device
    dmalp[MCFDMA_SAR] = dma_device_address[dmanr];
  }

#ifdef DMA_DEBUG
  printk("%s: Setting src %x dest %x addr for chan %d\n",
         __FUNCTION__, dmalp[MCFDMA_SAR], dmalp[MCFDMA_DAR], dmanr);
#endif
}

/*
 * Specific for Coldfire - sets device address.
 * Should be called after the mode set call, and before set DMA address.
 */
static __inline__ void set_dma_device_addr(unsigned int dmanr, unsigned int a) {
  dma_device_address[dmanr] = a;
#ifdef DMA_DEBUG
  printk("%s: Setting device addr %x for chan %d\n",
         __FUNCTION__, dma_device_address[dmanr], dmanr);
#endif DMA_DEBUG
}

/*
 * NOTE 2: "count" represents _bytes_.
 */
static __inline__ void set_dma_count(unsigned int dmanr, unsigned int count)
{
  volatile unsigned short *dmawp;

  dmawp = (unsigned short *) dma_base_addr[dmanr];
  dmawp[MCFDMA_BCR] = (unsigned short)count;

#ifdef DMA_DEBUG
  printk("%s: Setting count %x for chan %d\n",
         __FUNCTION__, (unsigned short)count , dmanr);
#endif DMA_DEBUG

}

/* Get DMA residue count. After a DMA transfer, this
 * should return zero. Reading this while a DMA transfer is
 * still in progress will return unpredictable results.
 * Otherwise, it returns the number of _bytes_ left to transfer.
 *
 */
static __inline__ int get_dma_residue(unsigned int dmanr)
{
  volatile unsigned short *dmawp;
  unsigned short count;

  dmawp = (unsigned short *) dma_base_addr[dmanr];
  count = dmawp[MCFDMA_BCR];
  return((int) count);
}

#else
 
 #define MAX_DMA_CHANNELS 8
 
#endif /* CONFIG_COLDFIRE */

/* Don't define MAX_DMA_ADDRESS; it's useless on the m68k/coldfire and any
   occurrence should be flagged as an error.  */

/* These are in kernel/dma.c: */
 extern int request_dma(unsigned int dmanr, const char * device_id);	/* reserve a DMA channel */
 extern void free_dma(unsigned int dmanr);	/* release it again */
 
#endif /* _M68K_DMA_H */
