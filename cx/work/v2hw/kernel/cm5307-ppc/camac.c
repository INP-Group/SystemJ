#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/8xx_immap.h>
#include <asm/mpc8xx.h>

#include "camac_chip.h"
#include "camac.h"

#define CAMAC_MAJOR 21

#undef DEBUG

#ifdef DEBUG
#define DP(x) x
#else
#define DP(x)
#endif

static volatile unsigned 
    *cr, *sr, *cam_chip, *cam_bus;

typedef struct {
    int                 used;
    struct task_struct *task;
    LAMHANDLER          lamhandler;
    wait_queue_head_t   queue;
} lam_owner_t;

static lam_owner_t lam_owner[24];

static void 
lam_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    unsigned lam_active,mask,i;
    unsigned long flags;

    save_flags(flags); cli();

    lam_active = *cr & *sr & 0x00ffffff;

    for (mask = 1, i=0 ; mask != 0x01000000; mask <<= 1, i++) {
	if ((lam_active & mask) && (lam_owner[i].used)) {
            *cr &= ~mask;
            wake_up(&(lam_owner[i].queue));
	    if (lam_owner[i].task) {
		DP(printk("signal sent (LAM %d)\n",i));
		send_sig(SIGUSR1, lam_owner[i].task, 1);
	    }
	    else if (lam_owner[i].lamhandler) {
		lam_owner[i].lamhandler(i);
	    }
	}
    }
    restore_flags(flags);
}

int cam_bus_read (char *buf, int len) 
{
    cam_header_t *ch = (cam_header_t *)buf;
    unsigned i, offset, status;
    unsigned long flags;

    if (len > 256) return -1;

    offset = (ch->n << 9) | (ch->a << 5) | ch->f;

    save_flags(flags); cli();

    for ( i = 0; i < len; i++) 
	ch->buf[i] = cam_bus[offset];

    status = *sr;
    restore_flags(flags);
    ch->status = (status & (CAM_SR_X | CAM_SR_Q)) >> 25;

    return len;
}

static ssize_t cam_read(struct file *file, char *buf,
                           size_t len, loff_t *ppos)
{
    cam_header_t *ch = (cam_header_t *)buf;

    if (verify_area(VERIFY_READ, ch, sizeof(cam_header_t))) return -1;
    if (verify_area(VERIFY_READ, ch->buf, len * sizeof(int))) return -1;

    return(cam_bus_read( buf, len ));
}

int cam_bus_write (const char *buf, int len) 
{
    cam_header_t *ch = (cam_header_t *)buf;
    unsigned i, offset, status;
    unsigned long flags;

    if (len > 256) return -1;

    offset = (ch->n << 9) | (ch->a << 5) | ch->f;
    
    save_flags(flags); cli();

    for ( i = 0; i < len; i++) 
	cam_bus[offset] = ch->buf[i];

    status = *sr;
    restore_flags(flags);
    ch->status = (status & (CAM_SR_X | CAM_SR_Q)) >> 25;

    return len;
}

static ssize_t cam_write(struct file * file, const char * buf, 
			 size_t len, loff_t *ppos)
{
    cam_header_t *ch = (cam_header_t *)buf;
    
    if (verify_area(VERIFY_READ, ch, sizeof(cam_header_t))) return -1;
    if (verify_area(VERIFY_WRITE, ch->buf, len * sizeof(int))) return -1;

    return(cam_bus_write(buf, len));
}

static int cam_open (struct inode *inode, struct file *file)
{
    DP(printk("camac open\n"));

    file->private_data = NULL;

    MOD_INC_USE_COUNT;
    return 0;
}

static int cam_release(struct inode *inode, struct file *file)
{
    int i, mask;
    unsigned long flags;
    
    for ( i = 0; i < 24; i++)
	if (lam_owner[i].used && (lam_owner[i].task == current)) {
	    lam_owner[i].used = 0;
	    lam_owner[i].task = NULL;
	    mask = 1 << i;
	    save_flags(flags);cli();
	    *cr &= ~mask;
	    restore_flags(flags);
	}

    DP(printk("camac release\n"));

    MOD_DEC_USE_COUNT;
    return 0;
}
void cam_execute_C(void)
{
    unsigned long flags;

    save_flags(flags);cli();
    *cr |= CAM_CR_C;
    restore_flags(flags);
    *cam_bus = 0;
}
void cam_execute_Z(void)
{
    unsigned long flags;
    
    save_flags(flags);cli();
    *cr |= CAM_CR_Z;
    restore_flags(flags);
    *cam_bus = 0;
}
void cam_set_I(int x)
{
    unsigned long flags;
    
    save_flags(flags);cli();
    if (x) *cr |= CAM_CR_INH;
	else *cr &= ~CAM_CR_INH;
    restore_flags(flags);
}
int cam_get_I(void)
{
    int x;
    
    x = *sr & CAM_SR_INH;
    return(x >> 24);
}
int cam_bus_lamenable(int lam, LAMHANDLER handler)
{
    unsigned mask;
    unsigned long flags;
    
    if ((lam < 0) || (lam > 23)) return -1;
    if (!handler) return -1;
    if (lam_owner[lam].used && lam_owner[lam].task) return -1;
    lam_owner[lam].used = 1;
    lam_owner[lam].lamhandler = handler;
    mask = 1 << lam;
    save_flags(flags);cli();
    *cr |= mask;
    restore_flags(flags);
    return 0;
}
int cam_bus_lamdisable(int lam)
{
    unsigned mask;
    unsigned long flags;

    if ((lam < 0) || (lam > 23)) return -1;
    if (lam_owner[lam].used && lam_owner[lam].task) return -1;
    mask = 1 << lam;
    save_flags(flags);cli();
    *cr &= ~mask;
    restore_flags(flags);
    lam_owner[lam].used = 0;
    lam_owner[lam].lamhandler = NULL;
    return 0;
}
int cam_bus_lampending(void)
{
    return(*sr & CAM_SR_LAM);
}

static unsigned int cam_poll(struct file *file, poll_table *wait)
{
  int  n = (int)(file->private_data) - 1;

    if (n < 0  ||  n > 23) return 0;
}

static int cam_ioctl (struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
    unsigned x,mask;
    unsigned long flags;

    switch(cmd){
        case CAM_IOCTL_LAMENABLE:
	    if (copy_from_user(&x, (unsigned *)arg, sizeof(unsigned)))
		return -EFAULT;
		
	    if (x > 23) return -1;
            if (lam_owner[x].used)
            {
		if ((lam_owner[x].task != current) || (lam_owner[x].lamhandler)){
		    DP(printk("LAMENABLE: not owner!\n"));
		    return -1;
                }
            }
            else
                init_waitqueue_head(lam_owner[x].queue);
	    lam_owner[x].task = current;
            lam_owner[x].used = 1;
            file->private_data = (void *)(x + 1);
	    mask = 1 << x;
	    save_flags(flags);cli();
	    *cr |= mask;
	    restore_flags(flags);
	    return 0;

	case CAM_IOCTL_LAMDISABLE:
	    if (copy_from_user(&x, (unsigned *)arg, sizeof(unsigned)))
		return -EFAULT;
	    if (x > 23) return -1;
	    if (lam_owner[x].used)
		if((lam_owner[x].task != current) || (lam_owner[x].lamhandler)){
		    DP(printk("LAMDISABLE: not owner!\n"));
		    return -1;
		}
	    mask = 1 << x;
	    save_flags(flags);cli();
	    *cr &= ~mask;
	    restore_flags(flags);
	    lam_owner[x].task = NULL;
	    lam_owner[x].used = 0;
	    return 0;

	case CAM_IOCTL_EXC:
	    cam_execute_C();
	    return 0;
	
	case CAM_IOCTL_EXZ:
	    cam_execute_Z();
	    return 0;
	    
	case CAM_IOCTL_SETI:
	    if (copy_from_user(&x, (unsigned *)arg, sizeof(unsigned)))
		return -EFAULT;
	    cam_set_I(x);
	    return 0;

	case CAM_IOCTL_GETI:
	    x = cam_get_I();
	    if (copy_to_user((unsigned *)arg, &x, sizeof(unsigned)))
		return -EFAULT;
	    return 0;
	
	case CAM_IOCTL_LAMPENDING:
	    x = cam_bus_lampending();
	    if (copy_to_user((unsigned *)arg, &x, sizeof(unsigned)))
		return -EFAULT;
	    return 0;
    }
    return -1;
}

static struct file_operations char_fops = {
	.read		= cam_read,
	.write		= cam_write,
        .ioctl		= cam_ioctl,
        .poll           = cam_poll,
	.open		= cam_open,
	.release	= cam_release,
};

static int init_cs(void)
{
    volatile immap_t *im = (immap_t *)IMAP_ADDR;

    im->im_memctl.memc_or4 = 0xfff00108; /* external TA, 1M space */
    im->im_memctl.memc_br4 = CAM_CHIP_ADDR | 0x4001;

    if (!request_mem_region(CAM_CHIP_ADDR, CAM_CHIP_SPACE, "CAMAC driver")) {
    	printk(KERN_ERR "Can't request memory region\n");
	return -EBUSY;
    }
    
    cam_chip = (unsigned *)ioremap_nocache(CAM_CHIP_ADDR, CAM_CHIP_SPACE);
    if (!cam_chip) {
       printk(KERN_ERR "unable to ioremap() memory \n");
       release_mem_region(CAM_CHIP_ADDR, CAM_CHIP_SPACE);
       return -EINVAL;
    }
    
    cam_bus = cam_chip + CAM_BUS_OFFSET / sizeof(unsigned);
    cr = cam_chip + CAM_CR_OFFSET / sizeof(unsigned);
    sr = cam_chip + CAM_SR_OFFSET / sizeof(unsigned);

    /* turn off led */
    im->im_cpm.cp_pbpar |= 0x10000;
    im->im_cpm.cp_pbdir |= 0x10000;


    return 0;
}
int init_module(void)
{
    init_cs();
    
    *cr = 0;

    if (register_chrdev (CAMAC_MAJOR, "Camac bus", &char_fops)) {
	printk ("CAMAC: Cannot get major %d.\n", CAMAC_MAJOR);
	return -1;
    }
    
    memset((char *)lam_owner, 0, sizeof(lam_owner));
    
    if (request_8xxirq(SIU_IRQ1, lam_interrupt, 0, "CAMAC", NULL) != 0) {
	unregister_chrdev (CAMAC_MAJOR, "Camac bus");
	iounmap((void *)cam_chip);
	release_mem_region(CAM_CHIP_ADDR, CAM_CHIP_SPACE);
	printk("Can't not allocate IRQ!");
	return -EINVAL;
    }

    printk("Generic CAMAC driver, V.R.Mamkin, 2001 (PowerPC, release 1.0) \n");
    return 0;
}
void cleanup_module (void)
{ 
    ((immap_t *)IMAP_ADDR)->im_siu_conf.sc_siel &=
	~(0x80000000 >> SIU_IRQ1);

    free_irq(SIU_IRQ1, 0);

    unregister_chrdev (CAMAC_MAJOR, "Camac bus");

    iounmap((void *)cam_chip);
    release_mem_region(CAM_CHIP_ADDR, CAM_CHIP_SPACE);

    printk("CAMAC driver unloaded\n");
}
MODULE_AUTHOR("Vitaly Mamkin");
MODULE_DESCRIPTION("PowerPC camac driver");
MODULE_LICENSE("GPL");
