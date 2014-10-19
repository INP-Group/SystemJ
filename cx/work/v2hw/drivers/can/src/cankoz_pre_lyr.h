#ifndef __CANKOZ_PRE_LYR_H
#define __CANKOZ_PRE_LYR_H


#include "cxsd_driver.h"
#include "cankoz_numbers.h"


/* General -- registration/deregistration, etc. */

typedef void (*DisconnectDeviceFromLayer)(int devid);

typedef void (*CanKozPktinProc)(int devid, void *devptr,
                                int desc, int datasize, uint8 *data);
typedef void (*CanKozOnIdProc) (int devid, void *devptr, int is_a_reset);

typedef int  (*CanKozAddDevice)(int devid, void *devptr,
                                int businfocount, int businfo[],
                                int devcode,
                                CanKozOnIdProc  rstproc,
                                CanKozPktinProc inproc,
                                int queue_size, int regs_base);

typedef int  (*CanKozGetDevVer)(int handle, int *hw_ver_p, int *sw_ver_p, int *hw_code_p);

typedef int  (*CanKozSendFrame)(int handle, int prio, int datasize, uint8 *data);

typedef void (*CanKozRegsRW)   (int handle, int action, int chan, int32 *wvp);


/* Queue management */

typedef struct
{
    int    prio;
    int    oneshot;
    int    datasize;
    uint8  data[8];
} canqelem_t;

typedef int (*queue_checker_t)(canqelem_t *elem, void *ptr);

#define QFE_ELEM_IS_EQUAL NULL

typedef enum
{
    QFE_FIND_FIRST,
    QFE_FIND_LAST,
    QFE_ERASE_FIRST,
    QFE_ERASE_ALL
} qfe_action_t;

typedef enum
{
    QFE_ERROR    = -1,
    QFE_NOTFOUND = 0,
    QFE_FOUND    = 1,
    QFE_ISFIRST  = 2,
} qfe_status_t;

typedef enum
{
    QFE_ALWAYS,
    QFE_IF_ABSENT,
    QFE_IF_NONEORFIRST
} qfe_enqcond_t;

typedef void         (*CanKozQClear)   (int handle);
typedef qfe_status_t (*CanKozQEnqueue) (int handle,
                                        canqelem_t *item, qfe_enqcond_t how);
typedef qfe_status_t (*CanKozQForeach) (int handle,
                                        queue_checker_t checker, void *ptr,
                                        qfe_action_t action);
typedef qfe_status_t (*CanKozQErAndSNx)(int handle,
                                        int datasize, ...);


/* CanKoz interface itself */

typedef struct
{
    DisconnectDeviceFromLayer  disconnect;
    CanKozAddDevice            add;
    CanKozGetDevVer            get_dev_ver;

    CanKozSendFrame            send_frame;
    
    CanKozRegsRW               regs_rw;

    CanKozQClear               q_clear;
    CanKozQEnqueue             q_enqueue;
    CanKozQEnqueue             q_enq_ons;
    CanKozQForeach             q_foreach;
    CanKozQErAndSNx            q_erase_and_send_next;
} cankoz_liface_t;

cankoz_liface_t *CanKozGetIface(void);

void             CanKozSetLogTo(int onoff);
void             CanKozSend0xFF(void);
int              CanKozGetDevInfo(int devid,
                                  int *line_p, int *kid_p, int *devcode_p);


/* Standard regs-block definition for nnn_main_info[] */

#define CANKOZ_REGS_MAIN_INFO {8, 1}, {8, 0}, {1, 1}, {1, 0}


#endif /* __CANKOZ_PRE_LYR_H */
