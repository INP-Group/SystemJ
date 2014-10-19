#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cx_sysdeps.h"
#include "misclib.h"

#include "remdrv_proto.h"
#include "remdrvlet.h"


void remdrvlet_report(int fd, int code, const char *format, ...)
{
  struct {remdrv_pkt_header_t hdr;  int32 data[1000];} pkt;
  int      len;
  va_list  ap;

    va_start(ap, format);
    len = vsprintf((char *)(pkt.data), format, ap);
    va_end(ap);

    bzero(&pkt.hdr, sizeof(pkt.hdr));
    pkt.hdr.pktsize = sizeof(pkt.hdr) + len + 1;
    pkt.hdr.command = code;

    write(fd, &pkt, pkt.hdr.pktsize);
}

void remdrvlet_debug (const char *format, ...)
{
  va_list  ap;

    fprintf(stderr, "%s ", strcurtime());
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
    fprintf(stderr, "%s: ", program_invocation_short_name);
#endif
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}
