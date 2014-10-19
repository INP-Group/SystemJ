#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>

#include "uspci.h"


#define countof(array) ((signed int)(sizeof(array) / sizeof(array[0])))

//////////////////////////////////////////////////////////////////////
/*
 *  timeval_subtract
 *
   Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.

 *  Note:
 *      Copied from the '(libc.info.gz)High-Resolution Calendar'.
 *      A small change: copy *y to yc.
 */

int timeval_subtract  (struct timeval *result,
                       struct timeval *x, struct timeval *y)
{
  struct timeval yc = *y; // A copy of y -- to preserve y from garbling
    
    /* Perform the carry for the later subtraction by updating Y. */
    if (x->tv_usec < yc.tv_usec) {
      int nsec = (yc.tv_usec - x->tv_usec) / 1000000 + 1;
        yc.tv_usec -= 1000000 * nsec;
        yc.tv_sec += nsec;
    }
    if (x->tv_usec - yc.tv_usec > 1000000) {
      int nsec = (yc.tv_usec - x->tv_usec) / 1000000;
        yc.tv_usec += 1000000 * nsec;
        yc.tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
     `tv_usec' is certainly positive. */
    result->tv_sec = x->tv_sec - yc.tv_sec;
    result->tv_usec = x->tv_usec - yc.tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < yc.tv_sec;
}

/*
 *  timeval_add
 *      Sums the `struct timeval' values, result=x+y.
 *
 *  Note:
 *      It is safe for `result' and `x'-or-`y' to be the same var.
 */

void timeval_add      (struct timeval *result,
                       struct timeval *x, struct timeval *y)
{
    result->tv_sec  = x->tv_sec  + y->tv_sec;
    result->tv_usec = x->tv_usec + y->tv_usec;
    /* Perform carry in case of usec overflow; may be done unconditionally */
    if (result->tv_usec >= 1000000)
    {
        result->tv_sec  += result->tv_usec / 1000000;
        result->tv_usec %= 1000000;
    }
}

void timeval_add_usecs(struct timeval *result,
                       struct timeval *x, int usecs_to_add)
{
  struct timeval arg2;
  
    arg2.tv_sec  = 0;
    arg2.tv_usec = usecs_to_add;
    timeval_add(result, x, &arg2);
}
//////////////////////////////////////////////////////////////////////





static int         option_hex_data = 0;
static int         option_quiet    = 0;
static enum
{
    TIMES_OFF,
    TIMES_REL,
    TIMES_ISO
}                  option_times    = TIMES_OFF;

enum
{
    PAS_COUNT     = 1 << 0,
    PAS_MASK      = 1 << 1,
    PAS_VALUE     = 1 << 2,
    PAS_VALARR    = 1 << 3,
    PAS_VALUE_RQD = 1 << 4,
    PAS_COMMA_TRM = 1 << 5,
};

static void ParseAddrSpec(const char *argv0, const char *place,
                          const char *spec, const char **end_p,
                          uspci_rw_params_t *addr_p,
                          void *databuf, size_t databufsize,
                          int flags)
{
  const char   *p = spec;
  char         *errp;
  
  char          u_c;

  __u8         *p8;
  __u16        *p16;
  __u32        *p32;
    
    if (place == NULL) place = "";

    bzero(addr_p, sizeof(*addr_p));
    addr_p->addr = databuf;
  
    u_c = tolower(*p);
    if      (u_c == 'b') addr_p->units = 1;
    else if (u_c == 's') addr_p->units = 2;
    else if (u_c == 'i') addr_p->units = 4;
    /*else if (u_c == 'q') units = 8;*/
    else                         goto ERR;
    p++;
    if (*p == ':') p++;  // Optional separator, to separate words for shell

    addr_p->bar    = strtol(p, &errp, 10);
    if (errp == p  ||  *errp != ':') goto ERR;
    p = errp + 1;

    addr_p->offset = strtol(p, &errp, 0);
    if (errp == p) goto ERR;
    p = errp;

    if (*p == '/')
    {
        if ((flags & PAS_COUNT) == 0)
        {
            fprintf(stderr, "%s: no /COUNT is allowed in %s%saddr spec \"%s\"\n",
                    argv0, place, *place != '\0'? " " : "", spec);
            exit(1);
        }
        
        p++;
        addr_p->count = strtol(p, &errp, 0);
        if (errp == p) goto ERR;
        p = errp;
        if (addr_p->count <= 0  ||
            addr_p->count > databufsize / addr_p->units)
        {
            fprintf(stderr, "%s: COUNT=%d is out of range [1-%d] in %s%saddr spec \"%s\"\n",
                    argv0,
                    addr_p->count, databufsize / addr_p->units,
                    place, *place != '\0'? " " : "", spec);
            exit(1);
        }
    }
    else
        addr_p->count = 1;

    if (*p == '@')
    {
        if ((flags & PAS_MASK) == 0)
        {
            fprintf(stderr, "%s: no @MASK is allowed in %s%saddr spec \"%s\"\n",
                    argv0, place, *place != '\0'? " " : "", spec);
            exit(1);
        }
        
        p++;
        addr_p->mask = strtol(p, &errp, 0);
        if (errp == p) goto ERR;
        p = errp;
    }
    else
        addr_p->mask = -1; /*!!! Is there any other way to set bitsize-independent all-1s? */

    if (*p == '=')
    {
        if ((flags & PAS_VALUE) == 0)
        {
            fprintf(stderr, "%s: no =VALUE is allowed in %s%saddr spec \"%s\"\n",
                    argv0, place, *place != '\0'? " " : "", spec);
            exit(1);
        }
        
        addr_p->t_op = USPCI_OP_WRITE;
        addr_p->cond = USPCI_COND_EQUAL;

        addr_p->count = 0;
        p8 = p16 = p32 = databuf;

        while (1)
        {
            p++;
            addr_p->value = strtol(p, &errp, 0);
            if (errp == p) goto ERR;
            p = errp;

            if ((flags & PAS_VALARR) != 0)
            {
                if      (addr_p->units == 1) *p8++  = addr_p->value;
                else if (addr_p->units == 2) *p16++ = addr_p->value;
                else if (addr_p->units == 4) *p32++ = addr_p->value;
                addr_p->count++;
            }

            if ((flags & PAS_COMMA_TRM) != 0) break; /* That's specifically for IRQ_CHECK,IRQ_RESET */

            if (*p == '\0') break;
            if (*p != ',')  goto ERR;

            /* Okay, next value... */
            if ((flags & PAS_VALARR) == 0)
            {
                fprintf(stderr, "%s: multiple VALUEs in %s%saddr spec \"%s\"\n",
                        argv0, place, *place != '\0'? " " : "", spec);
                exit(1);
            }

            if (addr_p->count >= databufsize / addr_p->units)
            {
                fprintf(stderr, "%s: too many VALUEs in %s%saddr spec \"%s\"; max is %d\n",
                        argv0, place, *place != '\0'? " " : "", spec,
                        databufsize / addr_p->units);
                exit(1);
            }

            /* Everything is okay -- let's slip to next iteration */
        }
    }
    else if (*p == '+'  &&  ((flags & PAS_MASK) != 0))
    {
        addr_p->value = 0;
        addr_p->t_op  = USPCI_OP_NONE;
        addr_p->cond  = USPCI_COND_NONZERO;
        p++;
    }
    else
    {
        if ((flags & PAS_VALUE_RQD) != 0)
        {
            fprintf(stderr, "%s: =VALUE is required in %s%saddr spec \"%s\"\n",
                    argv0, place, *place != '\0'? " " : "", spec);
            exit(1);
        }
        
        addr_p->t_op = USPCI_OP_READ;
    }

    if (end_p != NULL) *end_p = p;

    return;
    
 ERR:
    fprintf(stderr, "%s: error in %s%saddr spec \"%s\"\n",
            argv0, place, *place != '\0'? " " : "", spec);
    exit(1);
}

static void ParseDevSpec (const char *argv0, const char *spec,
                          uspci_setpciid_params_t *dev_p)
{
  const char *p = spec;
  const char *errp;
    
  int         a1;
  int         a2;
  
    a1 = strtol(p, &errp, 16);
    if (errp == p  ||  *errp != ':') goto ERR;
    p = errp + 1;
    a2 = strtol(p, &errp, 16);
    if (errp == p)                   goto ERR;
    p = errp;
    
    if      (*p == '.')
    {
        p++;
        dev_p->t_addr   = USPCI_SETDEV_BY_BDF;
        dev_p->bus      = a1;
        dev_p->device   = a2;
        dev_p->function = strtol(p, &errp, 16);
        if (errp == p) goto ERR;
    }
    else if (*p == ':')
    {
        p++;
        dev_p->vendor_id = a1;
        dev_p->device_id = a2;
        
        if (*p == '+')
        {
            p++;
            dev_p->t_addr = USPCI_SETDEV_BY_SERIAL;
            ParseAddrSpec(argv0, "serial-number", p, NULL,
                          &(dev_p->serial), NULL, 0,
                          PAS_VALUE | PAS_VALUE_RQD);
        }
        else
        {
            dev_p->t_addr = USPCI_SETDEV_BY_NUM;
            dev_p->n      = strtol(p, &errp, 10);
            if (errp == p) goto ERR;
        }
    }
    else goto ERR;
    
    return;
    
 ERR:
    fprintf(stderr, "%s: error in device spec \"%s\"\n", argv0, spec);
    exit(1);
}

static void WaitForIRQ(const char *argv0, int ref, const char *spec)
{
  const char     *p = spec;
  const char     *errp;
  int             all;
  int             was_irq;
  int             expired;

  __u32           irq_acc_mask;

  int             r;
  int             usecs;

  struct timeval  deadline;
  struct timeval  now;
  struct timeval  timeout;
  
  fd_set          xfds;
  
    all = *p == '.';
    p++;
    if (*p == '\0')
        usecs = -1;
    else
    {
        usecs = strtol(p, &errp, 0);
        if (errp == p)
            goto ERR;
    }

    if (usecs >= 0)
    {
        gettimeofday(&deadline, NULL);
        timeval_add_usecs(&deadline, &deadline, usecs);
    }

    was_irq = 0;
    do
    {
        expired = 0;
        if (usecs > 0)
        {
            gettimeofday(&now, NULL);
            expired = (timeval_subtract(&timeout, &deadline, &now) != 0);
        }
        if (usecs <= 0  ||  expired)
            timeout.tv_sec = timeout.tv_usec = 0;

        FD_ZERO(&xfds);
        FD_SET(ref, &xfds);
        r = select(ref + 1, NULL, NULL, &xfds, usecs >= 0? &timeout : NULL);
        
        if      (r < 0)
        {
            fprintf(stderr, "%s: select(): %s\n", argv0, strerror(errno));
            exit(2);
        }
        else if (r > 0)
        {
            irq_acc_mask = 0;
            ioctl(ref, USPCI_FGT_IRQ, &irq_acc_mask);
            was_irq = 1;
            if (!option_quiet)
            {
                printf("# IRQ; mask=");
                printf(option_hex_data? "0x%x" : "%u", irq_acc_mask);
                printf("\n");
            }
        }
        else /* r == 0 */
            expired = 1;

        if (expired  ||  (was_irq  &&  !all)) break;
    } while (1);

    if (!was_irq  &&  !option_quiet)
        printf("# NO IRQ in %d microseconds\n", usecs);

    return;
    
 ERR:
    fprintf(stderr, "%s: error in wait-for-IRQ spec \"%s\"\n", argv0, spec);
    exit(1);
}

static void PrintAddr(const char *descr, uspci_rw_params_t *addr_p)
{
    printf("%sbar=%d ofs=%d u=%d count=%d addr=%p cond=%d mask=%u value=%u t_op=%d\n",
           descr,
           addr_p->bar, addr_p->offset, addr_p->units, addr_p->count,
           addr_p->addr, addr_p->cond, addr_p->mask, addr_p->value, addr_p->t_op);
}

static void PerformIO (const char *argv0, int ref, const char *spec)
{
  int                r;
  uspci_rw_params_t  addr;
  __u32              buf[1000];
  __u8              *p8;
  __u16             *p16;
  __u32             *p32;
  char               format[100];
  __u32              v;
  int                x;
  
    ParseAddrSpec(argv0, "", spec, NULL,
                  &addr, buf, sizeof(buf),
                  PAS_COUNT | PAS_VALUE | PAS_VALARR);
    //PrintAddr(": ", &addr);
    ////for (x=0;x<sizeof(buf)/sizeof(buf[0]);x++)buf[x]=0;
    r = ioctl(ref, USPCI_DO_IO, &addr);
    if (r < 0)
    {
        fprintf(stderr, "%s: IO(\"%s\"): %s\n", argv0, spec, strerror(errno));
        exit(2);
    }

    if (addr.t_op == USPCI_OP_WRITE)
    {
    }
    else
    {
        if (option_quiet) return;

        if (option_hex_data)
            sprintf(format, "%%s0x%%0%dX", addr.units * 2);
        else
            sprintf(format, "%%s%%d");

        printf("%s=", spec);
        for (x = 0, p8 = p16 = p32 = buf;
             x < addr.count;
             x++)
        {
            if      (addr.units == 1) v = *p8++;
            else if (addr.units == 2) v = *p16++;
            else if (addr.units == 4) v = *p32++;
            printf(format, x > 0? "," : "", v);
        }
        printf("\n");
    }
}


int main(int argc, char *argv[])
{
  int                      c;              /* Option character */
  int                      err       = 0;  /* ++'ed on errors */

  int                      r;
  int                      ref;

  const char              *p;
  uspci_setpciid_params_t  dev;
  uspci_irq_params_t       irq;
  int                      irq_specified = 0;
  uspci_rw_params_t        onclose_params[10];
  int                      onclose_count = 0;
  
    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);

    while ((c = getopt(argc, argv, "c:hi:qtTx")) > 0)
    {
        switch (c)
        {
            case 'c':
                if (onclose_count != 0)
                {
                    fprintf(stderr, "%s: duplicate on-close specification\n",
                            argv[0]);
                    exit(1);
                }
                p = optarg;
                while (1)
                {
                    ParseAddrSpec(argv[0], "on-close", p, &p,
                                  onclose_params + onclose_count, NULL, 0,
                                  PAS_VALUE);
                    onclose_count++;
                    if (*p == '\0') break;
                    if (*p != ',')
                    {
                        fprintf(stderr, "%s: error in on-close spec after [%d]\n",
                                argv[0], onclose_count-1);
                        exit(1);
                    }
                    if (onclose_count < countof(onclose_params))
                        p++; /* Skip ',' */
                    else
                    {
                        fprintf(stderr, "%s: too many on-close actions (limit is %d)\n",
                                argv[0], countof(onclose_params));
                        exit(1);
                    }
                }
                onclose_params[0].batch_count = onclose_count;
                break;
            
            case 'h': goto PRINT_HELP;

            case 'i':
                if (irq_specified)
                {
                    fprintf(stderr, "%s: duplicate IRQ-handling specification\n",
                            argv[0]);
                    exit(1);
                }
                bzero(&irq, sizeof(irq));
                ParseAddrSpec(argv[0], "IRQ-check", optarg, &p,
                              &irq.check, NULL, 0,
                              PAS_MASK | PAS_VALUE | PAS_VALUE_RQD | PAS_COMMA_TRM);
                if      (*p == ',')
                    ParseAddrSpec(argv[0], "IRQ-reset", p + 1, NULL,
                                  &irq.reset, NULL, 0,
                                  PAS_VALUE);
                else if (*p != '\0')
                {
                    fprintf(stderr, "%s: error in IRQ-handling spec \"%s\"\n",
                            argv[0], optarg);
                    exit(1);
                }
                irq_specified = 1;
                break;

            case 'q': option_quiet    = 1;            break;
            case 't': option_times    = TIMES_REL;    break;
            case 'T': option_times    = TIMES_ISO;    break;

            case 'x': option_hex_data = 1;            break;
            
            case '?':
            default:
                err++;
        }
    }

    if (err) goto ADVICE_HELP;

    if (optind >= argc)
    {
        fprintf(stderr, "%s: device reference is required\n", argv[0]);
        goto ADVICE_HELP;
    }

    ParseDevSpec(argv[0], argv[optind++], &dev);
    
    ref = open("/dev/uspci", O_RDONLY);
    if (ref < 0)
    {
        fprintf(stderr, "%s: open(\"/dev/uspci\"): %s\n", argv[0], strerror(errno));
        exit(2);
    }
    r = ioctl(ref, USPCI_SETPCIID, &dev);
    if (r < 0)
    {
        fprintf(stderr, "%s: USPCI_SETPCIID: %s\n", argv[0], strerror(errno));
        exit(2);
    }
    if (onclose_count  &&  ioctl(ref, USPCI_ON_CLOSE, onclose_params) < 0)
    {
        fprintf(stderr, "%s: USPCI_ON_CLOSE: %s\n", argv[0], strerror(errno));
        exit(2);
    }
    if (irq_specified  &&  ioctl(ref, USPCI_SET_IRQ, &irq) < 0)
    {
        fprintf(stderr, "%s: USPCI_SET_IRQ: %s\n", argv[0], strerror(errno));
        exit(2);
    }
    //PrintAddr("check", &irq.check);
    //PrintAddr("reset", &irq.reset);

    if (optind >= argc)
        WaitForIRQ(argv[0], ref, ".");
    for (;  optind < argc;  optind++)
    {
        if      (argv[optind][0] == '-')
        {
        }
        else if (argv[optind][0] == ':'  ||  argv[optind][0] == '.')
        {
            WaitForIRQ(argv[0], ref, argv[optind]);
        }
        else
            PerformIO (argv[0], ref, argv[optind]);
    }
    
    return 0;

 ADVICE_HELP:
    fprintf(stderr, "Try `%s -h' for more information.\n", argv[0]);
    exit(1);

 PRINT_HELP:   
    fprintf(stderr, "Usage: %s [OPTIONS] DEVSPEC [COMMAND...]\n", argv[0]);
    fprintf(stderr, "    OPTIONS:\n");
    fprintf(stderr, "        -c OP1[,OP2]      operations to perform upon close\n");
    fprintf(stderr, "        -h                display this help and exit\n");
    fprintf(stderr, "        -i CHECK[,RESET]  IRQ handling spec (see below)\n");
    fprintf(stderr, "        -q                quiet operation\n");
    fprintf(stderr, "        -t                print relative timestamps\n");
    fprintf(stderr, "        -T                print ISO8601 timestamps\n");
    fprintf(stderr, "        -x                print data in hex\n");
    fprintf(stderr, "    DEVSPEC is either VENDOR:DEVICE:INSTANCE\n");
    fprintf(stderr, "                   or VENDOR:DEVICE:+U:BAR:OFFSET=SERIAL\n");
    fprintf(stderr, "                   or BUS:SLOT.FUNCTION\n");
    fprintf(stderr, "       (VENDOR, DEVICE, BUS, SLOT must be in hex,\n");
    fprintf(stderr, "        INSTANCE and SERIAL are decimal)\n");
    fprintf(stderr, "    COMMAND format is either U:BAR:OFFSET                  # read 1 unit\n");
    fprintf(stderr, "                          or U:BAR:OFFSET/COUNT            # read COUNT units\n");
    fprintf(stderr, "                          or U:BAR:OFFSET=VALUE{,VALUE...} # write VALUE(s)\n");
    fprintf(stderr, "                          or :[microseconds-to-wait]\n");
    fprintf(stderr, "        - U is either b (bytes), s (shorts, 16bits) or i (ints, 32bits)\n");
    fprintf(stderr, "        - OFFSET is always in bytes; COUNT is in units\n");
    fprintf(stderr, "    IRQ handling: \"-i CHECK[,RESET]\":\n");
    fprintf(stderr, "        CHECK format is U:BAR:OFFSET[@MASK]=VALUE or U:BAR:OFFSET+ # !=0\n");
    fprintf(stderr, "        RESET format is U:BAR:OFFSET[=VALUE]\n");
    exit(0);
}
