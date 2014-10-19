#ifndef __USPCI_H
#define __USPCI_H


#include <linux/types.h> /* needed for u32 */
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */


#define USPCI_IOC_MAGIC  'u'
#define USPCI_IOCRESET    _IO  (USPCI_IOC_MAGIC,  0)
#define USPCI_SETPCIID    _IOW (USPCI_IOC_MAGIC,  1, int)
#define USPCI_DO_IO       _IOWR(USPCI_IOC_MAGIC,  2, int)
#define USPCI_SET_IRQ     _IOW (USPCI_IOC_MAGIC,  3, int)
#define USPCI_FGT_IRQ     _IOWR(USPCI_IOC_MAGIC,  4, int)
#define USPCI_ON_CLOSE    _IOW (USPCI_IOC_MAGIC,  5, int)
#define USPCI_IOC_MAXNR                           5

enum
{
    USPCI_SETDEV_BY_NUM    = 1,
    USPCI_SETDEV_BY_SERIAL = 2,
    USPCI_SETDEV_BY_BDF    = 3,

    USPCI_OP_NONE          = 0,
    USPCI_OP_READ          = 'r',
    USPCI_OP_WRITE         = 'w',

    USPCI_COND_NONZERO     = '!',  // (data & mask) != 0
    USPCI_COND_EQUAL       = '=',  // (data & mask  == value
};

typedef struct uspci_rw_params
{
    int     batch_count;
    int     bar;
    int     offset;
    int     units;   // 1, 2, 4
    int     count;

    void   *addr;
    int     cond;
    __u32   mask;    // mask
    __u32   value;   // value
    int     t_op;    // operation type
} uspci_rw_params_t;

typedef struct uspci_setpciid_params
{
    unsigned int       t_addr;

    unsigned int       vendor_id;
    unsigned int       device_id;
    unsigned int       n;
    uspci_rw_params_t  serial;

    unsigned int       bus;
    unsigned int       device;
    unsigned int       function;

} uspci_setpciid_params_t;

typedef struct uspci_irq_params
{
    uspci_rw_params_t check;       // Our IRQ if
                                   // check.cond(ccheck.mask,check.value)
    uspci_rw_params_t reset;       // Command to reset IRQ
} uspci_irq_params_t;

#endif /* __USPCI_H */
