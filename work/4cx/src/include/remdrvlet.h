#ifndef __REMDRVLET_H
#define __REMDRVLET_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <sys/types.h>
#include <sys/socket.h>

#include "cxsd_driver.h"


typedef void (*remdrvlet_newdev_t)(int fd,
                                   struct  sockaddr  *addr,
                                   const char *name);
typedef void (*remdrvlet_newcon_t)(int fd,
                                   struct  sockaddr  *addr);


int  remdrvlet_listener(const char *argv0,
                        int port, int dontdaemonize,
                        remdrvlet_newdev_t newdev_p,
                        remdrvlet_newcon_t newcon_p);

int  remdrvlet_srvmain(int argc, char *argv[],
                       remdrvlet_newdev_t newdev_p,
                       remdrvlet_newcon_t newcon_t);
const char *remdrvlet_srvmain_param(int n);


/*** Utilities ******************************************************/

void remdrvlet_report(int fd, int code, const char *format, ...)
                     __attribute__ ((format (printf, 3, 4)));
void remdrvlet_debug (const char *format, ...)
                     __attribute__ ((format (printf, 1, 2)));


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __REMDRVLET_H */
