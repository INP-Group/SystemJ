#ifndef __REMCXSD_H
#define __REMCXSD_H


#include <netinet/in.h>

#include "cxsd_driver.h"
#include "cxscheduler.h"
#include "fdiolib.h"


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __REMCXSD_DRIVER_V2_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __REMCXSD_DRIVER_V2_C */


/*##################################################################*/

typedef struct
{
    int                 in_use;       // Is this cell currently used
    int                 being_processed;
    int                 being_destroyed;

    /* I/O stuff */
    int                 s;            // Host communication socket
    sl_fdh_t            s_fdh;        // ...its cxscheduler descriptor is used upon handshake only
    fdio_handle_t       fhandle;      // The fdiolib handle for .s, used after handshake

    time_t              when;
    struct sockaddr_in  cln_addr;
    char                drvname[32];  // Name of this driver
    size_t              drvnamelen;   // ...and its length (used only  when readin
    sl_tid_t            tid;          // Cleanup tid for semi-connected drivers (is active during handshake)

    /* Device's vital activity */
    int                 inited;       // Was the driver inited?
    CxsdDriverModRec   *metric;       // Pointer to the driver's methods-table
    int                 businfocount;
    int                 businfo[20];  // Bus ID, as supplied by host /*!!!==CXSD_DB_MAX_BUSINFOCOUNT*/
    void               *devptr;       // Device's private pointer

    int                 loglevel;
    int                 logmask;
} remcxsd_dev_t;


// These two should be provided by "user"
extern remcxsd_dev_t *remcxsd_devices;
extern int            remcxsd_maxdevs;

/*##################################################################*/

/* Note:
       report_TO_FD  is used on unfinished connections w/o working devid, and
                     such connections are always closed immediately afterwards.
                     So, there's no need to check for sending error.
       report_TO_DEV is mainly used prior to closing the connection,
                     but it also implements DoDriverLog(), so that there IS
                     need to check for sending error(s).
                     (However (as of 18.07.2013), that is unused.)
*/

void remcxsd_report_to_fd (int fd,    int code,            const char *format, ...)
                           __attribute__ ((format (printf, 3, 4)));
int  remcxsd_report_to_dev(int devid, int code, int level, const char *format, ...)
                           __attribute__ ((format (printf, 4, 5)));

int  AllocateDevID(void);
void FreeDevID    (int devid);

void ProcessPacket(int uniq, void *unsdptr,
                   fdio_handle_t handle, int reason,
                   void *inpkt,  size_t inpktsize,
                   void *privptr);

/*==== A little bit of macros for interaction with drivers =========*/

D int active_devid V(DEVID_NOT_IN_DRIVER);

#if 1
#define ENTER_DRIVER_S(devid, s) do {                              \
    s=active_devid; active_devid=devid;                            \
    if (devid >= 0               &&                                \
        devid < remcxsd_maxdevs  &&                                \
        remcxsd_devices[devid].in_use)                             \
        remcxsd_devices[devid].being_processed++;                  \
} while (0)
#define LEAVE_DRIVER_S(s)        do {                              \
    if (active_devid >= 0               &&                         \
        active_devid < remcxsd_maxdevs  &&                         \
        remcxsd_devices[active_devid].in_use)                      \
    {                                                              \
        remcxsd_devices[active_devid].being_processed--;           \
        if (remcxsd_devices[active_devid].being_processed == 0  && \
            remcxsd_devices[active_devid].being_destroyed)         \
            FreeDevID(active_devid);                               \
    }                                                              \
    active_devid=s;                                                \
} while (0)
#else
#define ENTER_DRIVER_S(devid, s) do {s=active_devid; active_devid=devid;} while (0)
#define LEAVE_DRIVER_S(s)        do {active_devid=s;                    } while (0)
#endif

/*** Utilities ******************************************************/

void InitRemDevRec(remcxsd_dev_t *dp);

void remcxsd_debug (const char *format, ...)
                   __attribute__ ((format (printf, 1, 2)));


#undef D
#undef V


#endif /* __REMCXSD_H */
