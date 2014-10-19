#include <stdarg.h>

#include "cxsd_driver.h"

#include "remdrv_proto.h"
#include "remdrvlet.h"


void        SetDevState      (int devid, int state,
                              rflags_t rflags_to_set, const char *description)
{
}

void        ReRequestDevData (int devid)
{
}


void        DoDriverLog      (int devid, int level,
                              const char *format, ...)
{
  va_list        ap;
  
    va_start(ap, format);
    vDoDriverLog(devid, level, format, ap);
    va_end(ap);
}

void        vDoDriverLog     (int devid, int level,
                              const char *format, va_list ap)
{
}


int         RegisterDevPtr   (int devid, void *devptr)
{
}


//////////////////////////////////////////////////////////////////////

static void newdev_p(int fd,
                     struct  sockaddr  *addr,
                     const char *name)
{
}

static void newcon_p(int fd,
                     struct  sockaddr  *addr)
{
}

int main(int argc, char *argv[])
{
    return remdrvlet_srvmain(argc, argv,
                             newdev_p,
                             newcon_p);
}
