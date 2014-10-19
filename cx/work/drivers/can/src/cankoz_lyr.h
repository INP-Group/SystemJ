#ifndef __CANKOZ_LYR_H
#define __CANKOZ_LYR_H


#include "cx_version.h"
#include "sendqlib.h"


#define CANKOZ_LYR_NAME "cankoz"
enum
{
    CANKOZ_LYR_VERSION_MAJOR = 2,
    CANKOZ_LYR_VERSION_MINOR = 0,
    CANKOZ_LYR_VERSION = CX_ENCODE_VERSION(CANKOZ_LYR_VERSION_MAJOR,
                                           CANKOZ_LYR_VERSION_MINOR)
};


enum
{
    CANKOZ_IOREGS_OFS_OUTRB0           =  0,
    CANKOZ_IOREGS_OFS_OUTRB1           =  1,
    CANKOZ_IOREGS_OFS_OUTRB2           =  2,
    CANKOZ_IOREGS_OFS_OUTRB3           =  3,
    CANKOZ_IOREGS_OFS_OUTRB4           =  4,
    CANKOZ_IOREGS_OFS_OUTRB5           =  5,
    CANKOZ_IOREGS_OFS_OUTRB6           =  6,
    CANKOZ_IOREGS_OFS_OUTRB7           =  7,
    CANKOZ_IOREGS_OFS_INPRB0           =  8,
    CANKOZ_IOREGS_OFS_INPRB1           =  9,
    CANKOZ_IOREGS_OFS_INPRB2           = 10,
    CANKOZ_IOREGS_OFS_INPRB3           = 11,
    CANKOZ_IOREGS_OFS_INPRB4           = 12,
    CANKOZ_IOREGS_OFS_INPRB5           = 13,
    CANKOZ_IOREGS_OFS_INPRB6           = 14,
    CANKOZ_IOREGS_OFS_INPRB7           = 15,
    CANKOZ_IOREGS_OFS_IR_IEB0          = 16,
    CANKOZ_IOREGS_OFS_IR_IEB1          = 17,
    CANKOZ_IOREGS_OFS_IR_IEB2          = 18,
    CANKOZ_IOREGS_OFS_IR_IEB3          = 19,
    CANKOZ_IOREGS_OFS_IR_IEB4          = 20,
    CANKOZ_IOREGS_OFS_IR_IEB5          = 21,
    CANKOZ_IOREGS_OFS_IR_IEB6          = 22,
    CANKOZ_IOREGS_OFS_IR_IEB7          = 23,
    CANKOZ_IOREGS_OFS_OUTR8B           = 24,
    CANKOZ_IOREGS_OFS_INPR8B           = 25,
    CANKOZ_IOREGS_OFS_IR_IE8B          = 26,
    CANKOZ_IOREGS_OFS_RESERVED_REGS_27 = 27,
    CANKOZ_IOREGS_OFS_RESERVED_REGS_28 = 28,
    CANKOZ_IOREGS_OFS_RESERVED_REGS_29 = 29,

    CANKOZ_IOREGS_CHANCOUNT            = 30
};


/* Driver-provided callbacks */

typedef void (*CanKozOnIdProc)  (int devid, void *devptr, int is_a_reset);
typedef void (*CanKozPktinProc) (int devid, void *devptr,
                                 int desc,  size_t dlc, uint8 *data);
typedef void (*CanKozOnSendProc)(int devid, void *devptr,
                                 sq_try_n_t try_n, void *privptr);


/* Layer API for drivers */

typedef int  (*CanKozAddDevice)(int devid, void *devptr,
                                int businfocount, int businfo[],
                                int devcode,
                                CanKozOnIdProc  ffproc,
                                CanKozPktinProc inproc,
                                int queue_size);
typedef int  (*CanKozGetDevVer)(int handle, int *hw_ver_p, int *sw_ver_p);

typedef void        (*CanKozQClear)     (int handle);

typedef sq_status_t (*CanKozQEnqTotal)  (int handle, sq_enqcond_t how,
                                         int tries, int timeout_us,
                                         CanKozOnSendProc on_send, void *privptr,
                                         int model_dlc, int dlc, uint8 *data);
typedef sq_status_t (*CanKozQEnqTotal_v)(int handle, sq_enqcond_t how,
                                         int tries, int timeout_us,
                                         CanKozOnSendProc on_send, void *privptr,
                                         int model_dlc, int dlc, ...);
typedef sq_status_t (*CanKozQEnq)       (int handle, sq_enqcond_t how,
                                         int dlc, uint8 *data);
typedef sq_status_t (*CanKozQEnq_v)     (int handle, sq_enqcond_t how,
                                         int dlc, ...);

typedef sq_status_t (*CanKozQErAndSNx)  (int handle,
                                         int dlc, uint8 *data);
typedef sq_status_t (*CanKozQErAndSNx_v)(int handle,
                                         int dlc, ...);


typedef int  (*CanKozHasRegs)  (int handle, int chan_base);
typedef void (*CanKozRegsRW)   (int handle, int action, int chan, int32 *wvp);


typedef struct
{
    /* General device management */
    CanKozAddDevice   add;
    CanKozGetDevVer   get_dev_ver;

    /* Queue management */
    CanKozQClear      q_clear;
    
    CanKozQEnqTotal   q_enqueue;
    CanKozQEnqTotal_v q_enqueue_v;
    CanKozQEnq        q_enq;
    CanKozQEnq_v      q_enq_v;
    CanKozQEnq        q_enq_ons;
    CanKozQEnq_v      q_enq_ons_v;

    CanKozQErAndSNx   q_erase_and_send_next;
    CanKozQErAndSNx_v q_erase_and_send_next_v;
    
    /* Services */
    CanKozHasRegs     has_regs;
    CanKozRegsRW      regs_rw;
} cankoz_vmt_t;


#endif /* __CANKOZ_LYR_H */
