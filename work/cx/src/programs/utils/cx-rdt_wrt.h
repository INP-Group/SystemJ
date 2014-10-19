/*
 *  rdt_wrt.h
 *      ReaD and WRite Test utilities body
 *
 *  Usage:
 *      {cx-rdt|cx-wrt|cx-sub} [-f first] [-c count] [-t times] [server]
 */


#if (defined(__CX_RDT_C) + defined(__CX_WRT_C) + defined(__CX_SUB_C)) != 1
  #error __FILE__: exactly one of "__CX_RDT_C/__CX_WRT_C/__CX_SUB_C" must be defined
#endif

#if   defined(__CX_WRT_C)
  #define PROGNAME "cx-wrt"
  #define ACTION   "writing"
#elif defined(__CX_RDT_C)
  #define PROGNAME "cx-rdt"
  #define ACTION   "reading"
#elif defined(__CX_SUB_C)
  #define PROGNAME "cx-sub"
  #define ACTION   "subscribing and waiting for"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "misclib.h"
#include "cxscheduler.h"

#include "cxlib.h"


int    conn;
int    r;

enum      {NUM = 1000};
int32     values[NUM];
tag_t     tags  [NUM];
rflags_t  rflags[NUM];

int    first  = 0;
int    count  = 10;
int    limit  = 1 << (sizeof(int)*8 - 2);
char  *server = NULL;
int    step   = 0;

static enum
{
    TIMES_OFF,
    TIMES_REL,
    TIMES_ISO
}                  option_times    = TIMES_OFF;


///////////////////////////////

#include <string.h>

typedef enum
{
    DT_TAG_COLON,
    DT_TAG_FLAG,
    DT_TAG_VALUE
} dt_tag;

typedef enum
{
    DT_RFLAGS_NONE,
    DT_RFLAGS_DEC,
    DT_RFLAGS_HEX,
    DT_RFLAGS_SHORT,
    DT_RFLAGS_LONG
} dt_rflags;

typedef enum
{
    DT_DATA_DEC,
    DT_DATA_OCT,
    DT_DATA_HEX
} dt_data;

static dt_tag    dpy_tag    = DT_TAG_FLAG;
static dt_rflags dpy_rflags = DT_RFLAGS_NONE;
static dt_data   dpy_data   = DT_DATA_DEC;
static int       dpy_data_w = 0;
static int       dpy_tabs   = 0;

static int ParseDisplayOption(const char *argv0)
{
  char  subsys;
  char  style;
  
    subsys = optarg[0];
    if (strchr("dtf", subsys) == NULL)
        goto BARK;

    style = optarg[1];
    
    switch (subsys)
    {
        case 'd':
            switch (style)
            {
                case 'h': dpy_tabs = 1;           break;
                case 'd': dpy_data = DT_DATA_DEC; break;
                case 'o': dpy_data = DT_DATA_OCT; break;
                case 'x': dpy_data = DT_DATA_HEX; break;
                default: goto BARK;
            }
            break;

        case 't':
            switch (style)
            {
                case '-': dpy_tag = DT_TAG_COLON;  break;
                case 'f': dpy_tag = DT_TAG_FLAG;   break;
                case 'v': dpy_tag = DT_TAG_VALUE;  break;
                default: goto BARK;
            }
            break;

        case 'f':
            switch (style)
            {
                case '-': dpy_rflags = DT_RFLAGS_NONE;   break;
                case 'd': dpy_rflags = DT_RFLAGS_DEC;    break;
                case 'x': dpy_rflags = DT_RFLAGS_HEX;    break;
                case 's': dpy_rflags = DT_RFLAGS_SHORT;  break;
                case 'l': dpy_rflags = DT_RFLAGS_LONG;   break;
                default: goto BARK;
            }
            break;
    }
    
    return 0;

 BARK:
    fprintf(stderr, "%s: invalid -d option \"%s\"\n", argv0, optarg);
    return 1;

}

static void PrintTag(tag_t tag)
{
    if      (dpy_tag == DT_TAG_COLON) printf(":");
    else if (dpy_tag == DT_TAG_FLAG)  printf("%c", tag != 0? '*' : ':');
    else                              printf("<%d>", (int)tag);
}

static void PrintRflags(rflags_t flags)
{
  int  shift;
  int  notfirst;
    
    switch (dpy_rflags)
    {
        case DT_RFLAGS_DEC:
        case DT_RFLAGS_HEX:
            printf(dpy_rflags == DT_RFLAGS_DEC? "{%d}" : "{%x}", flags);
            break;

        case DT_RFLAGS_SHORT:
        case DT_RFLAGS_LONG:
            printf("{");
            for (shift = 0, notfirst = 0;
                 ((1 << shift) & CXRF_SERVER_MASK) != 0;
                 shift++)
            {
                if (((1 << shift) & flags) != 0)
                {
                    if (notfirst) printf(",");
                    printf("%s", (dpy_rflags == DT_RFLAGS_SHORT)? cx_strrflag_short(shift) : cx_strrflag_long(shift));
                    notfirst = 1;
                }
            }
            printf("}");
            break;
            
        default:;
    }
}

static void PrintDatum(int d)
{
    if      (dpy_data == DT_DATA_HEX) printf("0x%-*x", dpy_data_w, d);
    else if (dpy_data == DT_DATA_OCT) printf("%s%-*o", d != 0? "0":"", dpy_data_w, d);
    else                              printf("%-*d",   dpy_data_w, d);
}

static void PrintSpace(void)
{
    printf(dpy_tabs? "\t" : " ");
}


///////////////////////////////


static void Check(const char *where)
{
    if (r >= 0) return;
    cx_perror2(where, PROGNAME);
    exit(1);
}

static void PreparePacket(void)
{
  int   x;
  
    /* Fill values (in fact, for cx-wrt only) */
    for (x = 0; x < count; x++)
        values[x] = +(step * 1000 + x);
    
    r = cx_begin(conn);
    Check("cx_begin()");
#ifdef __CX_WRT_C
    r = cx_setvgroup(conn, first, count, values, values, tags, rflags);
    Check("cx_setvgroup()");
#else
    r = cx_getvgroup(conn, first, count,         values, tags, rflags);
    Check("cx_getvgroup()");
#endif
}

static void PrintData(void)
{
  int   x;
  struct timeval  timenow;

    gettimeofday(&timenow, NULL);

    if (option_times == TIMES_ISO)
        printf("@%s", stroftime_msc(&timenow, "-"));
    else
        printf("%4d:", step);
    PrintSpace();
    
    for (x = 0; x < count; x++)
    {
        printf("%d", first + x);
        PrintTag   (tags[x]);
        PrintRflags(rflags[x]);
        PrintDatum (values[x]);
        if (x < count - 1)
            PrintSpace();
    }
    
    printf("\n");
}

#ifdef __CX_SUB_C
static void handler(int         cd      __attribute__((unused)),
                    int         reason,
                    void       *privptr __attribute__((unused)),
                    const void *info    __attribute__((unused)))
{
    if (reason != CAR_SUBSCRIPTION) return;
    PrintData();
}
#endif

int main(int argc, char *argv[])
{
  int   c;		/* Option character */
  int   err = 0;	/* ++'ed on errors */

    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    /* Parse command line */
    while ((c = getopt(argc, argv, "c:d:f:t:Th")) != EOF)
    {
        switch (c)
        {
            case 'c':
                count = atoi(optarg);
                break;
            case 'd':
                if (ParseDisplayOption(PROGNAME) != 0) err++;
                break;
            case 'f':
                first = atoi(optarg);
                break;
            case 't':
                limit = atoi(optarg);
                break;
            case 'T':
                option_times    = TIMES_ISO;
                break;
            case 'h':
            case ':':
            case '?':
                err++;
        }
    }

    if (err != 0)
    {
        fprintf(stderr, "Usage: " PROGNAME " [-f first] [-c count] [-t times] [-dDPYOPT] [server]\n");
        fprintf(stderr, "    dpyopts:  -ddd, -ddo, -ddx print data as Dec,Oct,heX\n");
        fprintf(stderr, "              -dt- omit tags;   -dtf *=old, :=fresh; -dtv number\n");
        fprintf(stderr, "              -df- omit rflags; -dfd,-dfx,-dfs,-dfl Dec,heX,Short,Long\n");
        fprintf(stderr, "    defaults: -f0 -c10 -t<infinity> -ddd -dtf -df- :0\n");
        exit(1);
    }

    if (count > NUM)
    {
        fprintf(stderr, PROGNAME ": count %d is too large, setting to %d\n",
                count, NUM);
        count = NUM;
    }
    
    if (optind < argc) server = argv[optind++];

    /* Really do connect */
    r = conn = cx_connect(server,  argv[0]);
    Check("cx_connect()");

    /* And run */
    printf(PROGNAME ": " ACTION " v[%d..%d] %d times\n",
           first, first + count - 1, limit);

#ifdef __CX_SUB_C
    /* Register notifier */
    r = cx_setnotifier(conn, handler, NULL);
    Check("cx_setnotifier()");

    /* Subscribe */
    PreparePacket();
    r = cx_subscribe(conn);
    Check("cx_subscribe()");

    sl_main_loop();

#else
    for (step = 0; step < limit; step++)
    {
        PreparePacket();
        r = cx_run(conn);
        Check("cx_run()");

        PrintData();
    }
#endif

    cx_close(conn);
    
    return 0;
}
