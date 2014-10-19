#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

#include "cxlib.h"

#include "misclib.h"
#include "timeval_utils.h"


#define PROGNAME "cx-bigc"


static int    conn;
static int    r;

enum  {MAXARGS = CX_MAX_BIGC_PARAMS, MAXDATA = 10000, MAXINFO=100, MAXRESULTS=10000*1+1000000*1};
static int32  args  [MAXARGS];
static int8   data8 [MAXDATA];
static int16  data16[MAXDATA];
static int32  data32[MAXDATA];
static int32  info  [MAXINFO];
static int32  retbuf[MAXRESULTS];
static int    nargs     = 0;
static int    datacount = 0;
static int    dataunits = 1;

static int       rninfo;
static size_t    retbufused;
static size_t    retbufunits;
static tag_t     r_tag;
static int32     r_tag32;
static rflags_t  rflags;

static int    chan   = 0;
static int    limit  = 1 << (sizeof(int)*8 - 2);
static char  *server = NULL;
static int    step   = 0;

static char  *float_format    = NULL;

static int    flag_binary     = 0;
static int    flag_gnuplot    = 0;
static int    flag_colheight  = 0;
static int    flag_linewidth  = 0;
static int    flag_quiet      = 0;
static int    flag_quiet_data = 0;
static int    flag_verbose    = 0;
static int    flag_rtime      = 0;
static int    flag_cachectl   = CX_CACHECTL_SHARABLE;
static int    flag_immediate  = CX_BIGC_IMMED_NO;
static int    flag_unsigned   = 0;

static enum
{
    TIMES_OFF,
    TIMES_REL,
    TIMES_ISO
}             option_times    = TIMES_OFF;

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

static dt_tag    dpy_tag    = DT_TAG_VALUE;
static dt_rflags dpy_rflags = DT_RFLAGS_DEC;
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


static int32 GetDatum(int idx)
{
  int8     *p8  = (void *)retbuf;
  int16    *p16 = (void *)retbuf;
  int32    *p32 = (void *)retbuf;
  
    if (retbufunits == 4) return                                     p32[idx];
    if (retbufunits == 2) return flag_unsigned? (uint16)(p16[idx]) : p16[idx];
    if (retbufunits == 1) return flag_unsigned? (uint8) (p8 [idx]) : p8 [idx];
    return 0xFFFFFFFF;
}

static void DoPrintDatum(const char *space_pfx, const char *dformat, int idx)
{
  union {int32 i32; float32 f32;} v;

    v.i32 =  GetDatum(idx);

    printf("%s", space_pfx);
    if (retbufunits == sizeof(float32)  &&  float_format != NULL)
        printf(float_format, v.f32);
    else
        printf(dformat, v.i32);

}

static void Check(const char *where)
{
    if (r >= 0) return;
    cx_perror2(where, PROGNAME);
    exit(1);
}


int main(int argc, char *argv[])
{
  int       c;		/* Option character */
  int       err  = 0;	/* ++'ed on errors */
  int       help = 0;
  char     *tp;         /* TextPointer -- for string parsing */
  char     *ep;         /* endptr for strtol()s */
  void     *data;

  char     *dformat;
  int       x;
  char     *u_s;
  char     *retunits_s;

  int       arg_num;
  int32     arg_val;

  int       ndata;
  int       nrows;
  int       ncols;
  int       y;
  int       idx;

  struct timeval  tv_req;
  struct timeval  tv_rsp;
  struct timeval  tv_now;

    /* Parse command line */
    while ((c = getopt(argc, argv, "a:A:bc:d:D:f:F:gH:it:TqQRuUvhW:")) != EOF)
    {
        switch (c)
        {
            case 'a':
                tp = optarg;
                arg_num = strtol(tp, &ep, 0);
                if (ep == tp  ||  *ep != '=')
                {
                    fprintf(stderr, "%s: -%c %s: error parsing argN\n",
                            argv[0], c, optarg);
                    err++;
                    break;
                }
                tp = ep + 1;
                arg_val = strtol(tp, &ep, 0);
                if (ep == tp  ||  *ep != '\0')
                {
                    fprintf(stderr, "%s: -%c %s: error parsing VALUE\n",
                            argv[0], c, optarg);
                    err++;
                    break;
                }
                if (arg_num < 0  ||  arg_num >= MAXARGS)
                {
                    fprintf(stderr, "%s: -%c %s: argN=%d out of range [0..%d]\n",
                            argv[0], c, optarg, arg_num, MAXARGS-1);
                    err++;
                    break;
                }
                args[arg_num] = arg_val;
                if (arg_num >= nargs) nargs = arg_num + 1;
                break;
                
            case 'A':
                tp = optarg;

                while (1)
                {
                    while (*tp != '\0'  &&  isspace(*tp)) tp++;
                    if (*tp == '\0') goto BREAK_A_PARSE;

                    if (nargs >= MAXARGS)
                    {
                        fprintf(stderr, "%s: too many arguments in -%c spec, max is %d\n",
                                argv[0], c, MAXARGS);
                        err++;
                        goto BREAK_A_PARSE;
                    }

                    arg_val = strtol(tp, &ep, 0);
                    if (ep == tp)
                    {
                        fprintf(stderr, "%s: error in -%c spec, near \"%s\"\n",
                                argv[0], c, tp);
                        err++;
                        goto BREAK_A_PARSE;
                    }
                    args[nargs++] = arg_val;
                    tp = ep;

                    if (*tp == ',') tp++;
                }
 BREAK_A_PARSE:;
                break;
                
            case 'b':
                flag_binary = 1;
                break;

            case 'c':
                if (optarg[0] >= '0'  &&  optarg[0] <= '3')
                    flag_cachectl = optarg[0] - '0';
                else
                {
                    fprintf(stderr, "%s: -%c expects a digit 0-3\n",
                            argv[0], c);
                    err++;
                }
                break;

            case 'd':
                if (ParseDisplayOption(PROGNAME) != 0) err++;
                break;
                
            case 'D':
                tp = optarg;

                switch (toupper(*tp))
                {
                    case 'B': dataunits = 1; break;
                    case 'S': dataunits = 2; break;
                    case 'I': dataunits = 4; break;
                    default:
                        fprintf(stderr, "%s: -%c expects a data-units prefix (b, s or i)\n",
                               argv[0], c);
                        err++;
                        goto BREAK_D_PARSE;
                }
                tp++;

                while (1)
                {
                    while (*tp != '\0'  &&  isspace(*tp)) tp++;
                    if (*tp == '\0') goto BREAK_D_PARSE;

                    if (datacount >= MAXDATA)
                    {
                        fprintf(stderr, "%s: too many data in -%c spec, max is %d\n",
                                argv[0], c, MAXDATA);
                        err++;
                        goto BREAK_D_PARSE;
                    }

                    arg_val = strtol(tp, &ep, 0);
                    if (ep == tp)
                    {
                        fprintf(stderr, "%s: error in -%c spec, near \"%.20s\"\n",
                                argv[0], c, tp);
                        err++;
                        goto BREAK_D_PARSE;
                    }
                    switch (dataunits)
                    {
                        case 1: data8 [datacount++] = arg_val; break;
                        case 2: data16[datacount++] = arg_val; break;
                        case 4: data32[datacount++] = arg_val; break;
                    }
                    tp = ep;

                    if (*tp == ',') tp++;
                }
                
 BREAK_D_PARSE:;
                break;
                
            case 'f':
                chan = atoi(optarg);
                break;
            case 'F':
                float_format = optarg;
                break;
            case 'g':
                flag_gnuplot   = 1;
                break;
            case 'H':
                flag_colheight = atoi(optarg);
                break;
            case 'i':
                flag_immediate = CX_BIGC_IMMED_YES;
                break;
            case 't':
                limit = atoi(optarg);
                break;
            case 'q':
                flag_quiet    = 1;
                flag_verbose  = 0;
                break;
            case 'Q':
                flag_quiet_data = 1;
                flag_verbose    = 0;
                break;
            case 'R':
                flag_rtime    = 1;
                break;
            case 'T':
                option_times  = TIMES_ISO;
                break;
            case 'u':
                flag_cachectl = CX_CACHECTL_FORCE;
                break;
            case 'U':
                flag_unsigned = 1;
                break;
            case 'v':
                flag_verbose  = 1;
                flag_quiet    = 0;
                break;
            case 'W':
                flag_linewidth = atoi(optarg);
                break;
            case 'h':
                help = 1;
                break;
            case ':':
            case '?':
                err++;
        }
    }
    dpy_tag = DT_TAG_VALUE; /*!!! As for now -- we force value */

    if (help != 0)
    {
        fprintf(stderr,
                "Usage: %s [options] [server]\n"
                "    -a argN=VALUE set argument N to VALUE\n"
                "    -A ARGLIST    comma-separated list of aruments\n"
                "    -b            binary output (implies -q)\n"
                "    -c N          N=[0-3] - cachectl value\n"
                "    -d DPYFMT     -dfd,-dfx,-dfs,-dfl: Dec,heX,Short,Long\n"
                "    -D uDATALIST  units, followed by comma-separated list of data\n"
                "                  units are: b - bytes, s - shorts, i - ints\n"
                "    -f BIGCHAN#   channel\n"
                "    -g            gnuplot-compatible output\n"
                "    -H colheight  column height for -g; >0 - height, <0 - param #\n"
                "    -i            turn on \"immediate\" flag\n"
                "    -R            print round-trip time\n"
                "    -T            print ISO8601 timestamps\n"
                "    -t TIMES      \n"
                "    -q            quiet operation (turns off -v)\n"
                "    -u            turn on \"uncacheable\" flag\n"
                "    -U            treat data as unsigned\n"
                "    -v            verbose operation (turns off -q)\n"
                "    -W linewidth  line width for -g (mutually exclusive with -H)\n"
                "    -h            show this help\n"
                "  Defaults: \n"
                "    ARGLIST=empty, DATALIST=empty, BIGCHAN#=0, TIMES=infinity, server=:0\n",
                argv[0]);
        
        exit(1);
    }

    if (err != 0)
    {
        fprintf(stderr, "Try `%s -h' for more information.\n", argv[0]);
        exit(1);
    }

    if (optind < argc) server = argv[optind++];

    /* Really do connect */
    r = conn = cx_openbigc(server,  argv[0]);
    Check("cx_openbigc()");

    for (step = 0; step < limit; step++)
    {
        data = data32;
        if (dataunits == 1) data = data8;
        if (dataunits == 2) data = data16;

        gettimeofday(&tv_req, NULL);
        r = cx_bigcmsg(conn, chan,
                       args, nargs,
                       data, datacount * dataunits, dataunits,
                       info, MAXINFO, &rninfo,
                       retbuf, sizeof(retbuf), &retbufused, &retbufunits,
                       &r_tag, &rflags,
                       flag_cachectl, flag_immediate);
        Check("cx_bigcmsg()");
        gettimeofday(&tv_now, NULL);
        tv_rsp = tv_now;
        timeval_subtract(&tv_rsp, &tv_rsp, &tv_req);
        r_tag32 = r_tag;

        if      (flag_quiet)
        {
            /* Print-nothing */
        }
        else if (flag_binary)
        {
            write(1, &rflags,      sizeof(rflags));
            write(1, &r_tag32,     sizeof(r_tag32));
            write(1, &rninfo,      sizeof(rninfo));
            write(1, &retbufused,  sizeof(retbufused));
            write(1, &retbufunits, sizeof(retbufunits));
            write(1, info,         sizeof(info[0]) * rninfo);
            write(1, retbuf,       retbufused);
        }
        else if (flag_gnuplot)
        {
            printf("# Data\n");
            if (option_times == TIMES_ISO)
                printf("# timestamp %s:%.06ld\n", stroftime(tv_now.tv_sec, "-"), (long)tv_rsp.tv_usec);
            printf("# age_tag "); PrintTag   (r_tag);  printf("\n");
            printf("# rflags  "); PrintRflags(rflags); printf("\n");
            printf("# ninfo %d\n",    rninfo);
            for (x = 0;  x < rninfo;  x++)
                printf("#     info %2d:%d\n", x, info[x]);
            printf("\n");
            
            dformat = "%d";
            if (flag_unsigned)
            {
                if (retbufunits == 1) dformat = "%3u";
                if (retbufunits == 2) dformat = "%5u";
                if (retbufunits == 4) dformat = "%10u";
            }
            else
            {
                if (retbufunits == 1) dformat = "%4d";
                if (retbufunits == 2) dformat = "%6d";
                if (retbufunits == 4) dformat = "%11d";
            }
            
            ndata = retbufused / retbufunits;

            ncols = 0;
            if (flag_linewidth > 0) ncols = flag_linewidth;
            if (flag_linewidth < 0  &&  rninfo > - flag_linewidth) ncols =  info[-flag_linewidth];
            
            if (ncols > 0)
            {
                nrows = (ndata + ncols - 1) / ncols;
                for (y = 0;  y < nrows;  y++)
                {
                    printf("%6d", y);
                    for (x = 0;  x < ncols  &&  (idx=y*ncols+x) < ndata; x++)
                    {
                        DoPrintDatum(" ", dformat, idx);
                    }
                    printf("\n");
                }
            }
            else
            {
                nrows = 0;
                if (flag_colheight > 0) nrows = flag_colheight;
                if (flag_colheight < 0  &&  rninfo > -flag_colheight) nrows = info[-flag_colheight];
                if (nrows <= 0) nrows = ndata;
    
                ncols = (ndata + nrows - 1) / nrows;
    
                for (y = 0;  y < nrows;  y++)
                {
                    printf("%6d", y);
                    for (x = 0;  x < ncols  &&  (idx=x*nrows+y) < ndata; x++)
                    {
                        DoPrintDatum(" ", dformat, idx);
                    }
                    printf("\n");
                }
            }
        }
        else /* Default output style */
        {
            if (option_times == TIMES_ISO)
                printf("@%s ", stroftime_msc(&tv_now, "-"));
            printf("age_tag="); PrintTag(r_tag); printf(",rflags="); PrintRflags(rflags); printf("\n");
            printf("info[%d]={", rninfo);
            for (x = 0;  x < rninfo;  x++)
            {
                printf("%s%d", x == 0? "" : ",", info[x]);
            }
            printf("}\n");

            u_s = flag_unsigned? "u-" : "";
            retunits_s = "ints";
            if (retbufunits == 2) retunits_s = "shorts";
            if (retbufunits == 1) retunits_s = "bytes";
            printf("data[%zu%s%s]={", retbufused / retbufunits, u_s, retunits_s);
            dformat = flag_unsigned? "%u" : "%d";
            if (flag_quiet_data)
                printf("...");
            else
                for (x = 0;  x < retbufused / retbufunits;  x++)
                {
                    if (x != 0) printf(",");
                    DoPrintDatum("", dformat, x);
                }
            printf("}\n");

            if (flag_rtime) printf("time=%d.%06ds\n",
                                   (int)tv_rsp.tv_sec, (int)tv_rsp.tv_usec);

            fflush(stdout);
        }
    }

    cx_close(conn);
    
    return 0;
}
