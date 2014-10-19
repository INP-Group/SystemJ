/*
 *  uspci.c -- User-Space PCI access driver
 *
 *  Copyright (C) 2009 Dmitry Yu. Bolkhovityanov and Pavel B. Cheblakov
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include "uspci.h"

//////////////////////////////////////////////////////////////////////
// Excerpt from CX's misc_macros.h
static inline void *lint2ptr(long  v){return (void*)v;}
static inline long  ptr2lint(void *p){return (long )p;}

#define LINT2PTR(v) ((void*)v)
#define PTR2LINT(p) ((long )p)
//////////////////////////////////////////////////////////////////////

#ifndef DEF_USPCI_MAJOR
#define DEF_USPCI_MAJOR 0 // Dynamic major by default
#endif

#ifndef USPCI_DEBUG
#define USPCI_DEBUG
#endif

int uspci_major = DEF_USPCI_MAJOR;
int uspci_minor = 0;

module_param(uspci_major, int, S_IRUGO);
module_param(uspci_minor, int, S_IRUGO);

#define DRV_NAME "uspci"

MODULE_AUTHOR ("Pavel B.Cheblakov, Dmitry Yu. Bolkhovityanov");
MODULE_DESCRIPTION(DRV_NAME " (UserSpace PCI access) driver "
                   __DATE__ " " __TIME__);
MODULE_VERSION("1.2.0");
MODULE_LICENSE("GPL");

#ifdef USPCI_DEBUG
static int debug;
module_param(debug, int, S_IRUGO);
#define DBG(args...)    (debug & 1 ? \
                        (printk(KERN_DEBUG "%s %s:%d ", DRV_NAME, __func__, \
                                __LINE__), printk(args)) : 0)
#define DBG_RW_PARAMS(label,rw_params) DBG("%s: bar=%d, offset=%x, units=%d, "\
                                          "count=%d, condition='%c', mask=%x, "\
                                          "value=%x, t_op='%c'\n",\
                                          label, \
                                          rw_params.bar, \
                                          rw_params.offset, \
                                          rw_params.units, \
                                          rw_params.count, \
                                          rw_params.cond, \
                                          rw_params.mask, \
                                          rw_params.value, \
                                          rw_params.t_op)
#else
#define DBG(args...)
#define DBG_RW_PARAMS(label,rw_params)
#endif


enum
{
    PCI_BAR_COUNT               = 6,
    NUM_BOARDS                  = 100,
    MAX_ON_CLOSE_BATCH_COUNT    = 10
};

typedef struct
{
    int                      in_use;
    int                      irq_set;
    int                      irq_counter;
    u32                      irq_acc_mask;

    struct pci_dev          *pdev;
    struct uspci_irq_params  irq_params;
    struct uspci_rw_params   on_close[MAX_ON_CLOSE_BATCH_COUNT];

    wait_queue_head_t        intr_queue;  /* wait queue for interrupt */
    spinlock_t               irq_counter_spinlock;

    void __iomem            *addr[PCI_BAR_COUNT];
    int                      size[PCI_BAR_COUNT];
    unsigned long            flags[PCI_BAR_COUNT];
} uspci_board_t;

static uspci_board_t  boards[NUM_BOARDS];

static struct cdev uspci_cdev;
dev_t              uspci_devid;

int do_pci_io_kernel(int bn, struct uspci_rw_params *pp);

//////////////////////////////////////////////////////////////////////

static void my_bzero(void *s, size_t size)
{
    char *p = s;

    while (size > 0)
    {
        *p++ = 0;
        size--;
    }
}


int do_setpciid(int bn, struct uspci_setpciid_params *pp)
{
    uspci_board_t  *b = boards + bn;
    struct pci_dev *pdev = NULL;
    int             n = 0;
    u32             tmp32 = 0;
    int             ret;

    if (b->pdev != NULL) return -EBUSY;

    if (pp->t_addr == USPCI_SETDEV_BY_NUM)
    {
        printk("%s: do_setpciid %04x:%04x:%d\n",
               DRV_NAME, pp->vendor_id, pp->device_id, pp->n);

        /* Find n-th (vendor,dev) device */
        do
        {
            pdev = pci_get_device(pp->vendor_id, pp->device_id, pdev);
            n++;
        }
        while (pdev != NULL  &&  n <= pp->n);
    }
    else if (pp->t_addr == USPCI_SETDEV_BY_BDF)
    {
        pdev = pci_get_bus_and_slot(pp->bus,
                                    PCI_DEVFN(pp->device, pp->function));
    }
    else if (pp->t_addr == USPCI_SETDEV_BY_SERIAL)
    {
        do
        {
            printk("%s: check device for serial\n", DRV_NAME);
            pdev = pci_get_device(pp->vendor_id, pp->device_id, pdev);
            if (pdev)
            {
                b->pdev = pdev; // Temporary set for do_pci_io_kernel
                pp->serial.addr = &tmp32;
                pp->serial.t_op = USPCI_OP_READ; // Force setting t_op
                pp->serial.count = 1;            // Force setting count
                ret = do_pci_io_kernel(bn, &(pp->serial));
                if (ret)
                {
                    DBG("serial: error during IO (%d)\n", ret);
                    b->pdev = NULL; // unset
                    pp->serial.addr = NULL;
                    return ret;
                }

                DBG_RW_PARAMS("serial", pp->serial);
                DBG("serial: %x\n", tmp32);

                if (pp->serial.cond == USPCI_COND_EQUAL &&
                    (tmp32 & pp->serial.mask) == pp->serial.value)
                    // Yeah, we've found our device
                    break;

                b->pdev = NULL; // unset
                pp->serial.addr = NULL;
            }
        }
        while (pdev != NULL);
    }

    if (pdev == NULL) return -ENXIO;

    /* Okay, the device is found -- let's remember it */
    b->pdev = pdev;

    return 0;
}

/*
 * Two following function is a same, except the context of their usage.
 * The first function is for interaction with user, and the second is
 * for calling from kernel context.
 */

int do_pci_io(int bn, struct uspci_rw_params *pp)
{
    uspci_board_t  *b    = boards + bn;
    struct pci_dev *pdev = b->pdev;

    void __iomem   *addr;

    int             i;
    int             rest;
    int             numread;
    int             offset;

    u8              tmp8;
    u16             tmp16;
    u32             tmp32;
    u8              buf[1024]; // 1024 is dividable by 1,2,4,8

    DBG("do_pci_io\n");

    if (pdev == NULL) return -ENODEV;

    if (pp->t_op == USPCI_OP_NONE) return 0; // There's nothing to do

    if (pp->bar < 0 || pp->bar > PCI_BAR_COUNT-1) return -EINVAL;

    if (pp->count < 1) return -EINVAL;

    if (pp->units != 1 &&
        pp->units != 2 &&
        pp->units != 4 &&
        pp->units != 8) /*TODO: units = 8 not supported yet*/
        return -EINVAL;

    addr = b->addr[pp->bar];
    if (addr == NULL) {
        addr = b->addr[pp->bar] = pci_iomap(pdev, pp->bar, 0);
        b->flags[pp->bar] = pci_resource_flags(pdev, pp->bar);
        b->size[pp->bar] = pci_resource_len(pdev, pp->bar);
    }

    if (pp->offset < 0 || pp->offset + pp->count * pp->units > b->size[pp->bar])
        return -EINVAL;

    if (b->flags[pp->bar] & IORESOURCE_MEM)
    {
        for (rest =  pp->count, offset =  0;
             rest >  0;
             rest -= numread,   offset += numread * pp->units)
        {
            numread = rest;
            if (numread > sizeof(buf) / pp->units)
                numread = sizeof(buf) / pp->units;

            if (pp->t_op == USPCI_OP_READ)
            {
                memcpy_fromio(buf,
                              addr + pp->offset + offset,
                              numread * pp->units);
                if (copy_to_user(pp->addr + offset,
                                 buf,
                                 numread * pp->units))
                    return -EFAULT;
            }
            else if (pp->t_op == USPCI_OP_WRITE)
            {
                if (copy_from_user(buf,
                                   pp->addr + offset,
                                   numread * pp->units))
                    return -EFAULT;
                memcpy_toio(addr + pp->offset + offset,
                            buf,
                            numread * pp->units);
            }
        }
    }

    if (b->flags[pp->bar] & IORESOURCE_IO)
    {
        DBG("do_pci_io: IORESOURCE_IO\n");
        switch (pp->units)
        {
            case 1:
                if (pp->t_op == USPCI_OP_READ)
                    for (i = 0; i < pp->count; i++)
                    {
                        tmp8  = ioread8 (addr + pp->offset + i*1);
                        if (copy_to_user(pp->addr + i*1, &tmp8, 1))
                            return -EINVAL;
                    }
                else if (pp->t_op == USPCI_OP_WRITE)
                    for (i = 0; i < pp->count; i++)
                    {
                        if (copy_from_user(&tmp8, pp->addr + i*1, 1))
                            return -EINVAL;
                        iowrite8 (tmp8, addr + pp->offset + i*1);
                    }
                break;

            case 2:
                if (pp->t_op == USPCI_OP_READ)
                    for (i = 0; i < pp->count; i++)
                    {
                        tmp16 = ioread16(addr + pp->offset + i*2);
                        if (copy_to_user(pp->addr + i*2, &tmp16, 2))
                            return -EINVAL;
                    }
                else if (pp->t_op == USPCI_OP_WRITE)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        if (copy_from_user(&tmp16, pp->addr + i*2, 2))
                            return -EINVAL;
                        iowrite16(tmp16, addr + pp->offset + i*2);
                    }
                break;

            case 4:
                if (pp->t_op == USPCI_OP_READ)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        tmp32 = ioread32(addr + pp->offset + i*4);
                        if (copy_to_user(pp->addr + i*4, &tmp32, 4))
                            return -EINVAL;
                    }
                else if (pp->t_op == USPCI_OP_WRITE)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        if (copy_from_user(&tmp32, pp->addr + i*4, 4))
                            return -EINVAL;
                        iowrite32(tmp32, addr + pp->offset + i*4);
                    }
                break;

            case 8:
                return -EINVAL; // 8-byte I/O-ports don't support
        }
    }

    return 0;
}


int do_pci_io_kernel(int bn, struct uspci_rw_params *pp)
{
    uspci_board_t  *b    = boards + bn;
    struct pci_dev *pdev = b->pdev;

    void __iomem   *addr;

    int             i;
    int             rest;
    int             numread;
    int             offset;

    u8              tmp8;
    u16             tmp16;
    u32             tmp32;
    u8              buf[1024]; // 1024 is dividable by 1,2,4,8

    if (pdev == NULL) return -ENODEV;

    if (pp->t_op == USPCI_OP_NONE) return 0; // There's nothing to do

    if (pp->bar < 0 || pp->bar > PCI_BAR_COUNT-1) return -EINVAL;

    if (pp->count < 1) return -EINVAL;

    if (pp->units != 1  &&
        pp->units != 2  &&
        pp->units != 4  &&
        pp->units != 8) /*!!!*/
        return -EINVAL;

    addr = b->addr[pp->bar];
    if (addr == NULL) {
        addr = b->addr[pp->bar] = pci_iomap(pdev, pp->bar, 0);
        b->flags[pp->bar] = pci_resource_flags(pdev, pp->bar);
        b->size[pp->bar] = pci_resource_len(pdev, pp->bar);
    }

    if (pp->offset < 0 || pp->offset + pp->count * pp->units > b->size[pp->bar])
        return -EINVAL;

    if (b->flags[pp->bar] & IORESOURCE_MEM)
    {
        for (rest =  pp->count, offset =  0;
             rest >  0;
             rest -= numread,   offset += numread * pp->units)
        {
            numread = rest;
            if (numread > sizeof(buf) / pp->units)
                numread = sizeof(buf) / pp->units;

            if (pp->t_op == USPCI_OP_READ)
            {
                memcpy_fromio(buf,
                              addr + pp->offset + offset,
                              numread * pp->units);
                memcpy(pp->addr + offset,
                       buf,
                       numread * pp->units);
            }
            else if (pp->t_op == USPCI_OP_WRITE)
            {
                memcpy(buf,
                       pp->addr + offset,
                       numread * pp->units);
                memcpy_toio(addr + pp->offset + offset,
                            buf,
                            numread * pp->units);
            }
        }
    }

    if (b->flags[pp->bar] & IORESOURCE_IO)
    {
        switch (pp->units)
        {
            case 1:
                if (pp->t_op == USPCI_OP_READ)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        tmp8  = ioread8 (addr + pp->offset + i*1);
                        memcpy(pp->addr + i*1, &tmp8, 1);
                    }
                else if (pp->t_op == USPCI_OP_WRITE)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        memcpy(&tmp8,  pp->addr + i*1, 1);
                        iowrite8 (tmp8,  addr + pp->offset + i*1);
                    }
                break;

            case 2:
                if (pp->t_op == USPCI_OP_READ)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        tmp16 = ioread16(addr + pp->offset + i*2);
                        memcpy(pp->addr + i*2, &tmp16, 2);
                    }
                else if (pp->t_op == USPCI_OP_WRITE)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        memcpy(&tmp16, pp->addr + i*2, 2);
                        iowrite16(tmp16, addr + pp->offset + i*2);
                    }
                break;

            case 4:
                if (pp->t_op == USPCI_OP_READ)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        tmp32 = ioread32(addr + pp->offset + i*4);
                        memcpy(pp->addr + i*4, &tmp32, 4);
                    }
                else if (pp->t_op == USPCI_OP_WRITE)
                    for (i = 0;  i < pp->count;  i++)
                    {
                        memcpy(&tmp32, pp->addr + i*4, 4);
                        iowrite32(tmp32, addr + pp->offset + i*4);
                    }
                break;

            case 8:
                return -EINVAL; // 8-byte I/O-ports don't support
        }
    }

    return 0;
}


irqreturn_t uspci_interrupt(int irq, void *dev_id)
{
    int bn               = ptr2lint(dev_id);
    uspci_board_t  *b    = boards + bn;

    u32 tmp32 = 0; // This variable is used for 8, 16 and 32
    unsigned long flags;

    DBG("interrupt!\n");

    b->irq_params.check.addr = &tmp32;
    do_pci_io_kernel(bn, &(b->irq_params.check));

    DBG("interrupt test data 0x%x = %.8x\n", b->irq_params.check.offset, tmp32);

    if ((b->irq_params.check.cond == USPCI_COND_NONZERO &&
         (tmp32 & b->irq_params.check.mask) != 0)
        ||
        (b->irq_params.check.cond == USPCI_COND_EQUAL &&
         (tmp32 & b->irq_params.check.mask) == b->irq_params.check.value))
    {
        if (do_pci_io_kernel(bn, &(b->irq_params.reset)))
            goto NONE;

        DBG("IRQ_HANDLED\n");

        spin_lock_irqsave(&b->irq_counter_spinlock, flags);
        if (b->irq_counter < 1 << 30) b->irq_counter++;
        b->irq_acc_mask |= tmp32;
        spin_unlock_irqrestore(&b->irq_counter_spinlock, flags);

        wake_up(&(b->intr_queue));

        return IRQ_HANDLED;
    }
    else
    {

NONE:
        DBG("IRQ_NONE\n");
        return IRQ_NONE;
    }
}


static int do_set_irq(int bn, struct uspci_irq_params *pp)
{
    uspci_board_t  *b    = boards + bn;
    struct pci_dev *pdev = b->pdev;
    int             result;

    b->irq_params = *pp;

    if (pdev == NULL) return -EINVAL;

    b->irq_params.check.t_op = USPCI_OP_READ;
    b->irq_params.check.count = 1;

    b->irq_params.reset.addr = &(b->irq_params.reset.value);
    b->irq_params.reset.count = 1;

    // Enable IRQ on this device
    if (pci_enable_device(pdev)) {
        printk(KERN_ERR "%s: enable PCI device failed\n", DRV_NAME);
        return -EIO;
    }

    // Register interrupt handler
    result = request_irq(pdev->irq,
                         &uspci_interrupt,
                         IRQF_SHARED,
                         DRV_NAME, // this name we can see in /proc/interrupts
                         lint2ptr(bn));
    if (result) return -EAGAIN;

    b->irq_set = 1;

    DBG("assigning IRQ #: %d\n", pdev->irq);
    DBG_RW_PARAMS("check", b->irq_params.check);
    DBG_RW_PARAMS("reset", b->irq_params.reset);

    return 0;
}


static int do_fgt_irq(int bn, void *acc_mask_p)
{
    uspci_board_t  *b    = boards + bn;
    unsigned long   flags;
    int             ret;

    spin_lock_irqsave(&b->irq_counter_spinlock, flags);

    ret = b->irq_counter;
    b->irq_counter = 0;

    if (acc_mask_p != NULL)
    {
        if (copy_to_user(acc_mask_p, &(b->irq_acc_mask), sizeof(b->irq_acc_mask)))
            ret = -EINVAL;
    }
    b->irq_acc_mask = 0;

    spin_unlock_irqrestore(&b->irq_counter_spinlock, flags);

    return ret;
}


static unsigned int uspci_poll(struct file *filp, poll_table *wait)
{
    int bn            = ptr2lint(filp->private_data);
    uspci_board_t *b  = boards + bn;
    unsigned int mask = 0;
    unsigned long flags;

    DBG("uspci_poll()\n");

    poll_wait(filp, &(boards[bn].intr_queue),  wait);

    spin_lock_irqsave(&b->irq_counter_spinlock, flags);
    if (b->irq_counter)
    {
        mask |= POLLPRI;       /* exception */
    }
    spin_unlock_irqrestore(&b->irq_counter_spinlock, flags);

    return mask;
}


int uspci_ioctl(struct inode *inode, struct file *filp,
                unsigned int cmd, unsigned long arg)
{
    int  ret = 0;
    int  err = 0;
    int  bn  = ptr2lint(filp->private_data);
    int batch_count = 0;
    int batch_cur = 0;

    struct uspci_setpciid_params  pciid_params;
    struct uspci_rw_params        rw_params;
    struct uspci_irq_params       irq_params;

    /* Zeroth, check the board-number */
    if (bn < 1  ||  bn >= NUM_BOARDS)
    {
        printk(KERN_WARNING "%s: ioctl() with bn=%d!\n", DRV_NAME, bn);
        return -EINVAL;
    }
    if (boards[bn].in_use == 0)
    {
        printk(KERN_WARNING "%s: ioctl() with unused bn=%d!\n", DRV_NAME, bn);
        return -EINVAL;
    }

    /* First, check the code */
    if (_IOC_TYPE(cmd) != USPCI_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd)   >  USPCI_IOC_MAXNR) return -ENOTTY;

    /* Next, check accessibility of memory */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ,  (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;

    switch (cmd)
    {
        case USPCI_IOCRESET:
            break;

        case USPCI_SETPCIID:
            ret = copy_from_user(&pciid_params,
                                 (void *)arg,
                                 sizeof(pciid_params));
            if (ret == 0)
                ret = do_setpciid(bn, &pciid_params);
            break;

        case USPCI_DO_IO:
            do {
                ret = copy_from_user(&rw_params,
                                     (void *)(arg + batch_cur * sizeof(rw_params)),
                                     sizeof(rw_params));

                if (ret == 0)
                {
                    if (!batch_count)
                        batch_count = rw_params.batch_count ?: 1;

                    ret = do_pci_io(bn, &rw_params);
                } else
                    break;

                batch_cur++;
            } while (!ret && batch_cur < batch_count);
            break;

        case USPCI_SET_IRQ:
            ret = copy_from_user(&irq_params,
                                 (void *)arg,
                                 sizeof(irq_params));
            if (ret == 0)
                ret = do_set_irq(bn, &irq_params);
            break;

        case USPCI_FGT_IRQ:
            ret = do_fgt_irq(bn, (void *)arg);
            break;

        case USPCI_ON_CLOSE:
            do {
                ret = copy_from_user(&(boards[bn].on_close[batch_cur]),
                                     (void *)(arg + batch_cur * sizeof(rw_params)),
                                     sizeof(rw_params));
                boards[bn].on_close[batch_cur].addr  = &(boards[bn].on_close[batch_cur].value);
                boards[bn].on_close[batch_cur].count = 1;
                if (boards[bn].on_close[0].batch_count > MAX_ON_CLOSE_BATCH_COUNT)
                {
                    boards[bn].on_close[0].batch_count = 0;
                    return -EINVAL;
                }
                batch_cur++;
            } while (!ret && batch_cur < boards[bn].on_close[0].batch_count);
            break;

        default:
            return -ENOTTY;
    }

    return ret;
}


int uspci_open(struct inode *inode, struct file *filp)
{
    int  bn;

    /* Find an unused slot */
    for (bn = 1;  bn < NUM_BOARDS;  bn++)
        if (boards[bn].in_use == 0)
        {
            my_bzero(&(boards[bn]), sizeof(boards[bn]));
            boards[bn].in_use      = 1;
            init_waitqueue_head(&(boards[bn].intr_queue));
            spin_lock_init     (&(boards[bn].irq_counter_spinlock));
            filp->private_data = lint2ptr(bn);
            return 0;
        }

    printk(KERN_WARNING "%s: there are no free slots\n", DRV_NAME);

    return -ENOMEM;
}


int uspci_release(struct inode *inode, struct file *filp)
{
    int  bn = ptr2lint(filp->private_data);
    uspci_board_t *b  = boards + bn;
    int batch_count;
    int ret = 0;
    int i;

    if (bn < 1  ||  bn >= NUM_BOARDS)
    {
        printk(KERN_WARNING "%s: release() with invalid bn=%d!\n", DRV_NAME, bn);
        return 0;
    }

    batch_count = b->on_close[0].batch_count;
    for (i = batch_count; i > 0 && !ret; i--)
    {
        ret = do_pci_io_kernel(bn, &(b->on_close[batch_count - i]));
    }

    /* Free resources... */
    if (boards[bn].irq_set)
        free_irq(b->pdev->irq, lint2ptr(bn));

    filp->private_data = 0;
    boards[bn].in_use      = 0;

    return 0;
}


struct file_operations uspci_fops =
{
    .owner   = THIS_MODULE,
    .ioctl   = uspci_ioctl,
    .poll    = uspci_poll,
    .open    = uspci_open,
    .release = uspci_release,
};

//////////////////////////////////////////////////////////////////////

void uspci_cleanup_module(void)
{
    cdev_del(&uspci_cdev);
    unregister_chrdev_region(uspci_devid, 1);
}


int uspci_init_module(void)
{
    int    ret;
    int    bn;

    printk("%s (UserSpace PCI access) driver " __DATE__ " " __TIME__ "\n",
           DRV_NAME);

    /* Allocate/register a major */
    if (uspci_major)
    {
        uspci_devid = MKDEV(uspci_major, uspci_minor);
        ret = register_chrdev_region(uspci_devid, 1, DRV_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&uspci_devid, uspci_minor, 1, DRV_NAME);
        uspci_major = MAJOR(uspci_devid);
    }
    if (ret < 0)
    {
        printk(KERN_WARNING "%s: can't get major %d\n", DRV_NAME, uspci_major);
        return ret;
    }

    /* Initialize array of private-structures */
    for (bn = 0;  bn < NUM_BOARDS;  bn++)
    {
        boards[bn].in_use      = 0;
        boards[bn].irq_set     = 0;
    }

    /* Initialize cdev */
    cdev_init(&uspci_cdev, &uspci_fops);
    uspci_cdev.owner = THIS_MODULE;
    uspci_cdev.ops   = &uspci_fops;
    ret = cdev_add(&uspci_cdev, uspci_devid, 1);
    if (ret < 0)
    {
        printk(KERN_NOTICE "%s: can't cdev_add()\n", DRV_NAME);
        return ret;
    }

    return 0;
}


module_init(uspci_init_module);
module_exit(uspci_cleanup_module);
