#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#include "misc_types.h"
#include "misc_macros.h"
#include "misclib.h"

#include "cankoz_numbers.h"
#include "cankoz_strdb.h"

#ifndef CANHAL_FILE_H
  #error The "CANHAL_FILE_H" macro is undefined
#else
  #include CANHAL_FILE_H
#endif


//////////////////////////////////////////////////////////////////////

static int  CanutilOpenDev  (const char *argv0,
                             const char *linespec, const char *baudspec)
{
  int   fd;
  int   minor;
  int   speed_n;
  char *err;
    
    if (linespec[0] == 'c'  &&  linespec[1] == 'a'  &&  linespec[2] == 'n')
        linespec += 3;
    minor = strtol(linespec, &err, 10);
    if (err == linespec  ||  *err != '\0')
    {
        fprintf(stderr, "%s: bad CAN_LINE spec \"%s\"\n", argv0, linespec);
        exit(1);
    }

    if (baudspec == NULL) baudspec = "125";
    if      (strcasecmp(baudspec, "125")  == 0) speed_n = 0;
    else if (strcasecmp(baudspec, "250")  == 0) speed_n = 1;
    else if (strcasecmp(baudspec, "500")  == 0) speed_n = 2;
    else if (strcasecmp(baudspec, "1000") == 0) speed_n = 3;
    else
    {
        fprintf(stderr, "%s: bad speed spec \"%s\" (allowed are 125, 250, 500, 1000)\n",
                argv0, baudspec);
        exit(1);
    }

    fd = canhal_open_and_setup_line(minor, speed_n, &err);
    if (fd < 0)
    {
        fprintf(stderr, "%s: unable to open line %d: %s: %s\n",
                argv0, minor, err, strerror(errno));
        exit(2);
    }
    
    return fd;
}

static void CanutilCloseDev (int fd)
{
    canhal_close_line(fd);
}

static void CanutilReadFrame(int fd, int *id_p, int *dlc_p, uint8 *data)
{
  int     r;
  fd_set  fds;
  char   *err;
    
    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        r = select(fd + 1, &fds, NULL, NULL, NULL);
        if (r == 1)
        {
            r = canhal_recv_frame(fd, id_p, dlc_p, data);
            if (r != CANHAL_OK)
            {
                if      (r == CANHAL_ZERO)   err = "ZERO";
                else if (r == CANHAL_BUSOFF) err = "BUSOFF";
                else                         err = strerror(errno);
                fprintf(stderr, "%s: recv_frame: %s\n",
                        "argv0", err);
                exit (2);
            }
            return;
        }
    }
}

static void CanutilSendFrame(int fd, int id,    int  dlc,   uint8 *data)
{
  int     r;
  char   *err;
    
    r = canhal_send_frame(fd, id, dlc, data);
    if (r != CANHAL_OK)
    {
        if      (r == CANHAL_ZERO)   err = "ZERO";
        else if (r == CANHAL_BUSOFF) err = "BUSOFF";
        else                         err = strerror(errno);
        fprintf(stderr, "%s: send_frame(id=%d=0x%x, dlc=%d): %s\n",
                "argv0", id, id, dlc, err);
        exit (2);
    }
}

//////////////////////////////////////////////////////////////////////
static inline int  encode_frameid(int kid, int prio)
{
    return (kid << 2) | (prio << 8);
}

static inline void decode_frameid(int frameid, int *kid_p, int *prio_p)
{
    *kid_p  = (frameid >> 2) & 63;
    *prio_p = (frameid >> 8) & 7;
}
//////////////////////////////////////////////////////////////////////

static int  SayAndHear(int can_fd, int kid, uint8 *data)
{
  int     rid;
  uint8   rcvd[8];
  int     rdlc;

    CanutilSendFrame(can_fd, encode_frameid(kid, CANKOZ_PRIO_UNICAST),
                     8, data);
//return 0;
    while (1)
    {
        CanutilReadFrame(can_fd, &rid, &rdlc, rcvd);
        if (rid != encode_frameid(kid, CANKOZ_PRIO_REPLY)) continue;

        if (rcvd[0] == 0xBB  ||
            rcvd[0] == 0x1E  ||
            rcvd[0] == 0xAE  ||
            rcvd[0] == 0xEE  ||
            rcvd[0] == 0xFE) continue;

        if (rcvd[0] != data[0])
        {
            fprintf(stderr, "reply %02x!=%02x\n", rcvd[0], data[0]);
            exit (3);
        }

        if (rcvd[0] == CANKOZ_DESC_GETDEVATTRS)
        {
            printf("GETDEVATTRS/0xFF: DevCode=%d HWver=%d SWver=%d reason=%d:%s\n",
                   rcvd[1],
                   rcvd[2],
                   rcvd[3],
                   rcvd[4],
                   rcvd[4] >= 0  &&  rcvd[4] < countof(cankoz_GETDEVATTRS_reasons)?
                       cankoz_GETDEVATTRS_reasons[rcvd[4]] : "???");
        }

        return 0;
    }
}

static void JustSay(int can_fd, int kid, uint8 *data)
{
    CanutilSendFrame(can_fd, encode_frameid(kid, CANKOZ_PRIO_UNICAST),
                     8, data);
    SleepBySelect(100000);
}

static void EraseInQueue(int can_fd)
{
  int     rid;
  uint8   rcvd[8];
  int     rdlc;

  int     r;
  fd_set  fds;
  struct timeval tv;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(can_fd, &fds);

        tv.tv_sec = tv.tv_usec = 0;
        r = select(can_fd + 1, &fds, NULL, NULL, &tv);
        if (r != 1) return;

        CanutilReadFrame(can_fd, &rid, &rdlc, rcvd);
    }
}

static void rtrim_line(char *line)
{
  char *p = line + strlen(line) - 1;

    while (p > line  &&
           (*p == '\n'  ||  isspace(*p)))
        *p-- = '\0';
}

static uint8 x2u8(const char *p)
{
  int  x1, x2;

    if (isdigit(p[0])) x1 =         p[0]  - '0';
    else               x1 = tolower(p[0]) - 'a' + 10;

    if (isdigit(p[1])) x2 =         p[1]  - '0';
    else               x2 = tolower(p[1]) - 'a' + 10;

//printf("%c%c->0x%02x\n", p[0], p[1], x1 * 16 + x2);
    return x1 * 16 + x2;
}

static void FlashUBS(const char *argv0, const char *address, 
                     FILE *fp, int can_fd)
{
  enum
  {
      DESC_FF         = 0xFF,
      DESC_ERASEFLASH = 0x01,
      DESC_WRITE_LP1  = 0x02,
      DESC_WRITE_LP2  = 0x03,
      DESC_WRITE_LP3  = 0x04,
      DESC_FL_REBOOT  = 0x05,
      DESC_SET_MODE   = 0x17,
      DESC_SETTARGET  = 0x37,
      DESC_REBOOT     = 0xF7,
  };

  int         kid;
  const char *errp;
  uint8       data[8];

  char        line[1000];
  int         lineno;

  int         pn;
  int         b;
  
    kid = strtol(address, &errp, 0);
    if (errp == address  ||  (*errp != '\0'  &&  !isspace(*errp)))
    {
        fprintf(stderr, "%s: invalid address spec \"%s\"\n",
                argv0, address);
        exit(2);
    }

    printf("About to flash kid=%d\n", kid);
    rewind(fp);

    data[0] = DESC_SET_MODE;
    data[1] = 1;
    JustSay(can_fd, kid, data);
    data[0] = DESC_SET_MODE;
    data[1] = 4;
    JustSay(can_fd, kid, data);
    data[0] = DESC_SETTARGET;
    data[1] = 0x07;
    JustSay(can_fd, kid, data);
    data[0] = DESC_REBOOT;
    JustSay(can_fd, kid, data);
    SleepBySelect(5000000);
    EraseInQueue(can_fd);
    data[0] = DESC_ERASEFLASH;
    SayAndHear(can_fd, kid, data);

    lineno = 0;
    while (fgets(line, sizeof(line) - 1, fp) != NULL)
    {
        lineno++;
        rtrim_line(line);

        if (line[0] == '\0') continue;
        if (line[0] != ':')
        {
            fprintf(stderr, "Non-':' line %d\n", lineno);
            exit(2);
        }

        if (strlen(line) < 43)
        {
            memset(line + strlen(line), '0', 43 - strlen(line));
            line[43] = '\0';
        }
        //fprintf(stderr, "line=%s\n", line);

        for (pn = 0;  pn < 3;  pn++)
        {
            data[0] = DESC_WRITE_LP1 + pn;
            for (b = 0;  b <= 6;  b++)
                data[b+1] = x2u8(line + (1 + pn * 14 + b * 2));
            SayAndHear(can_fd, kid, data);
        }
    }

    data[0] = DESC_FL_REBOOT;

    printf("Done\n");
}

int main(int argc, char *argv[])
{
  int   can_fd;
  FILE *fp;
  int   n;

    if (argc < 2                       ||
        strcmp(argv[1], "-h")     == 0 ||
        strcmp(argv[1], "--help") == 0)
    {
        printf("Usage: %s FILE UBS-ID {UBS-ID...}\n", argv[0]);
        exit(1);
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        fprintf(stderr, "%s: unable to open \"%s\" for reading: %s\n",
                argv[0], argv[1], strerror(errno));
        exit(1);
    }

    can_fd = CanutilOpenDev(argv[0], "can0", "125");

    for (n = 2;  n < argc;  n++)
        FlashUBS(argv[0], argv[n], fp, can_fd);

    CanutilCloseDev(can_fd);

    fclose(fp);

    printf("Finished\n");

    return 0;
}
