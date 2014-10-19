#include "cxsd_includes.h"

#include "paramstr_parser.h"


typedef char string[256];

string       dberrmsg;
static FILE *fp;
static char  line[256];
static int   lineno;
static int   linelen;
static int   linepos;
static int   prevpos;


#define SavePos() (prevpos = linepos)
#define Back()    (linepos = prevpos)


/*==== ctype extensions ============================================*/

static inline int isletter(int c)
{
    return isalpha(c)  ||  c == '_'  ||  c == '-';
}

static inline int isletnum(int c)
{
    return isalnum(c)  ||  c == '_'  ||  c == '-';
}


/*==== Error reporting =============================================*/

static void ErrLine(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));
static void ErrLine(const char *format, ...)
{
  va_list ap;
  
    sprintf(dberrmsg, "file.l%d: ", lineno);
    va_start(ap, format);
    vsprintf(dberrmsg+strlen(dberrmsg), format, ap);
    va_end(ap);
}

static void ErrLineCol(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));
static void ErrLineCol(const char *format, ...)
{
  va_list ap;
  
    sprintf(dberrmsg, "file.l%d.c%d: ", lineno, linepos);
    va_start(ap, format);
    vsprintf(dberrmsg+strlen(dberrmsg), format, ap);
    va_end(ap);
}


/*==== Parsing primitives ==========================================*/

static int SkipWhite(void)
{
    while (linepos < linelen  &&  isspace(line[linepos]))  linepos++;

    return linepos < linelen;
}

static int AtEOL(void)
{
    return linepos >= linelen;
}

static int CheckAtEOL(void)
{
    if (AtEOL())
    {
        ErrLine("unexpected EOL");
        return 1;
    }

    return 0;
}

static int GetIdentifier(string buf)
{
  int   c;
  char *p = buf;

    SavePos();

    if (CheckAtEOL()) return 1;
    if (!isletter(line[linepos]))
    {
        ErrLineCol("identifier expected");
        return 1;
    }

    while(1) {
      c = tolower(line[linepos]);
      if (!isletnum(c)) break;
      linepos++;
      *(p++) = c;
    }

    *p = '\0';

    return 0;
}

static int GetInt(int *value)
{
#if 1
  char *p = line + linepos;
  char *endptr;

    SkipWhite();
    SavePos();

    if (CheckAtEOL()) return 1;
    *value = strtol(p, &endptr, 0);
    if (endptr == p)
    {
        ErrLineCol("integer expected");
        return 1;
    }

    linepos = endptr - line;

    return 0;
#else
  int  v = 0;
  int  c;
  int  was_minus = 0;

    SkipWhite();
    SavePos();

    if (CheckAtEOL()) return 1;
    if (!isdigit(line[linepos]) && line[linepos] != '-' && line[linepos] != '+')
    {
        ErrLineCol("integer expected");
        return 1;
    }

    while(1) {
        c = line[linepos];
        if (linepos == prevpos  &&  (c == '-'  ||  c == '+'))
        {
            was_minus = (c == '-');
        }
        else if (isdigit(c))
        {
            v = (v * 10) + (c - '0');
        }
        else break;
        linepos++;
    }

    if (was_minus) v = -v;
    *value = v;

    return 0;
#endif
}


/*==== Readline itself =============================================*/

static int ReadLine(void)
{
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        ++lineno;
        linelen = strlen(line);
        if (linelen > 0  &&  line[linelen - 1] == '\n')  line[--linelen] = '\0';
        linepos = 0;
        SkipWhite();

        if (AtEOL()  ||  line[linepos] == '#')  continue;

        /*logline(LOGF_SYSTEM, 0, "line#%d=%s", lineno, line);*/
    
        return 1;
    }

    return 0;
}


static int CheckDriverMetric(const char *fname, void *symptr, void *privinfo __attribute__((unused)))
{
  CxsdDriverModRec *metric = (CxsdDriverModRec *)symptr;

    if (metric->mr.magicnumber != CXSRV_DRIVERREC_MAGIC)
    {
        logline(LOGF_SYSTEM, 0, "%s: (%s).magicnumber mismatch",
                __FUNCTION__, fname);
        return -1;
    }
    if (!CX_VERSION_IS_COMPATIBLE(metric->mr.version, CXSRV_DRIVERREC_VERSION))
    {
        logline(LOGF_SYSTEM, 0, "%s: (%s).version=%d, incompatible with %d",
                __FUNCTION__, fname, metric->mr.version, CXSRV_DRIVERREC_VERSION);
        return -1;
    }
    
    return 0;
}
static int LoadDriver(const char *mod_name, const char *dev_name,
                      CxsdDriverModRec **metric_p)
{
    return LoadModule(mod_name, config_drivers_dir, "_drv.so",
                      dev_name, DRIVERREC_SUFFIX_STR,
                      0,
                      CheckDriverMetric, NULL,
                      (void**)metric_p);
}

static int CheckDevInfo(CxsdDriverModRec *metric, const char *mod_name, const char *dev_name,
                        int main_nsegs, CxsdMainInfoRec *main_info,
                        int bigc_nsegs, CxsdBigcInfoRec *bigc_info)
{
    if (metric->main_nsegs >= 0)
    {
        if (metric->main_nsegs != main_nsegs)
        {
            logline(LOGF_SYSTEM, 0, "%s: (%s:%s).main_nsegs=%d, != db.main_nsegs=%d",
                    __FUNCTION__, mod_name, dev_name, metric->main_nsegs, main_nsegs);
            return -1;
        }

        if (main_nsegs != 0  &&
            memcmp(metric->main_info, main_info, main_nsegs * sizeof(CxsdMainInfoRec)) != 0)
        {
            logline(LOGF_SYSTEM, 0, "%s: (%s:%s).main_info != db.main_info",
                    __FUNCTION__, mod_name, dev_name);
            return -1;
        }
    }
    
    if (metric->bigc_nsegs >= 0)
    {
        if (metric->bigc_nsegs != bigc_nsegs)
        {
            logline(LOGF_SYSTEM, 0, "%s: (%s:%s).bigc_nsegs=%d, != db.bigc_nsegs=%d",
                    __FUNCTION__, mod_name, dev_name, metric->bigc_nsegs, bigc_nsegs);
            return -1;
        }

        if (bigc_nsegs != 0  &&
            memcmp(metric->bigc_info, bigc_info, bigc_nsegs * sizeof(CxsdMainInfoRec)) != 0)
        {
            logline(LOGF_SYSTEM, 0, "%s: (%s:%s).bigc_info != db.bigc_info",
                    __FUNCTION__, mod_name, dev_name);
            return -1;
        }
    }

    return 0;
}


static void FillChanProps(int devid, int type, int start, int count)
{
  int  x;

    logline(LOGF_SYSTEM, 0, "\t@%-4d %c%d", start, type?'w':'r', count);
  
    for (x = start;  x < start + count;  x++)
    {
        c_devid [x] = devid;
        c_type  [x] = type;

        /*!!! Shouldn' we move everything below to InitDev()?*/
        c_time  [x] = 0;
        c_rd_req[x] = c_wr_req [x] = 0;
        c_rflags[x] = c_crflags[x] = 0;
        c_wr_time      [x] = 0;
        c_next_wr_val_p[x] = 0;
    }
}

static int  FillBigcProps(int devid, int start, CxsdBigcInfoRec *info)
{
  int           r;
  int           x;
  size_t        argssize = info->nargs * sizeof(int32);
  size_t        infosize = info->ninfo * sizeof(int32);
  size_t        retsize4 = (info->retdatasize + 3) &~ 3U;
  size_t        per_chan = argssize + infosize + retsize4 + argssize;
  size_t        per_dev  = per_chan * info->count;
  size_t        offset;

  bigchaninfo_t *bp;

  static char   sizes[5]  = "zbsti";
  
    logline(LOGF_SYSTEM, 0, "\t@%-3d %d*%d+%zu%c/%d+%zu%c, %zu bytes",
            start,
            info->count,
            info->nargs, info->datasize    / info->dataunits,    sizes[info->dataunits],
            info->ninfo, info->retdatasize / info->retdataunits, sizes[info->retdataunits],
            per_dev);
  
    if (per_dev != 0)
    {
        offset = bigbufused;
        r = GrowBuf(&bigbuf, &bigbufsize, bigbufused + per_dev);
        if (r < 0)
        {
            /*!!!???*/
        }
        bigbufused += per_dev;
    }

    for (x = start,               bp = b_data + start;
         x < start + info->count;
         x++,                     bp++)
    {
        bzero(bp, sizeof(*bp));
        
        bp->devid        = devid;

        bp->nargs        = info->nargs;
        bp->datasize     = info->datasize;
        bp->dataunits    = info->dataunits;
        bp->ninfo        = info->ninfo;
        bp->retdatasize  = info->retdatasize;
        bp->retdataunits = info->retdataunits;

        CONSUME_BIGBUF(&(bp->cur_args_o),    argssize, &offset);
        CONSUME_BIGBUF(&(bp->cur_info_o),    infosize, &offset);
        CONSUME_BIGBUF(&(bp->cur_retdata_o), retsize4, &offset);
        CONSUME_BIGBUF(&(bp->sent_args_o),   argssize, &offset);
    }
         
    return 0;
}

static int AddDev(int *cdevid, int *cur_nchans, int *cur_nbigcs,
                  int main_nsegs, CxsdMainInfoRec *main_info,
                  int bigc_nsegs, CxsdBigcInfoRec *bigc_info,
                  int businfocount, int32 *businfo,
                  const char *inst_name,
                  const char *mod_name, const char *dev_name,
                  const char *lyr_name,
                  const char *auxinfo,
                  int logmask)
{
  int               r;
  CxsdDriverModRec *metric;
  
  int            thisdevid = *cdevid;

  int            drv_ld_problem;
  
  int            x;
  int            cur_m;
  int            cur_b;
  int            total_main_count;
  int            total_bigc_count;

  size_t         drvinfobytes;
  
  char           bus_id_str[countof(d_businfo[0]) * (12+2)];
  char          *bis_p;
  
  int            log_auxinfo;

  static DEFINE_DRIVER(stub, "Stub for devid=0",
                       NULL, NULL,
                       0, NULL,
                       0, CXSD_DB_MAX_BUSINFOCOUNT,
                       NULL, 0,
                       -1, NULL,
                       -1, NULL,
                       NULL, NULL,
                       NULL, NULL);
#define STUB_NAME __CX_CONCATENATE(stub,DRIVERREC_SUFFIX)

    if (businfocount > CXSD_DB_MAX_BUSINFOCOUNT) businfocount = CXSD_DB_MAX_BUSINFOCOUNT;

    if (businfocount == 0)
        sprintf(bus_id_str, "-");
    else
        for (bis_p = bus_id_str, x = 0;  x < businfocount;  x++)
            bis_p += sprintf(bis_p, "%s%d", x > 0? "," : "", businfo[x]);

    log_auxinfo = auxinfo != NULL  &&  auxinfo[0] != '\0';

    if (inst_name == NULL) inst_name = "";
    
    logline(LOGF_SYSTEM, 0, "%s[%d](%s: %s:%s@\"%s\", businfo=%s%s%s%s)",
            __FUNCTION__, thisdevid, inst_name, mod_name, dev_name, lyr_name, bus_id_str,
            log_auxinfo? ", auxinfo=\"" : "",
            log_auxinfo? auxinfo        : "",
            log_auxinfo? "\""           : "");
  
    /* Count totals */
    for (x = 0, total_main_count = 0;  x < main_nsegs;  x++)
        total_main_count += main_info[x].count;
    for (x = 0, total_bigc_count = 0;  x < bigc_nsegs;  x++)
        total_bigc_count += bigc_info[x].count;

    /* Is it a simulation (for devid=0)? */
    if (mod_name == NULL  ||  dev_name == NULL)
    {
        drv_ld_problem = 0;
        metric = &STUB_NAME;
    }
    else
    {
        drv_ld_problem =
            LoadDriver(mod_name, dev_name, &metric)   != 0  ||
            InitDriver   (metric)                     != 0  ||
            CheckDevInfo (metric, mod_name, dev_name,
                          main_nsegs, main_info,
                          bigc_nsegs, bigc_info)      != 0;

        if (drv_ld_problem)
        {
            metric = &STUB_NAME;
            //!!! return -1; -- Should remember for future
        }
    }
 SKIP_DRIVER_LOAD:

    /* Store dev info: */
    /* General */
    d_loadok   [thisdevid] = (metric == &STUB_NAME? -1 : 0);
    d_metric   [thisdevid] = metric;
    d_businfocount[thisdevid] = businfocount;
    for (x = 0;  x < businfocount;  x++) d_businfo[thisdevid][x] = businfo[x];
    d_ofs      [thisdevid] = *cur_nchans;
    d_size     [thisdevid] = total_main_count;
    d_big_first[thisdevid] = *cur_nbigcs;
    d_big_count[thisdevid] = total_bigc_count;
    d_devptr   [thisdevid] = NULL; /* The same -- whoever needs a ptr, should do a RegisterDevPtr() */
    d_logmask  [thisdevid] = logmask;
    d_doio     [thisdevid] = NULL;
    d_dobig    [thisdevid] = NULL;

    /* 32-bit channels */
    for (cur_m = *cur_nchans,         x = 0;
         x < main_nsegs;
         cur_m += main_info[x].count, x++)
    {
        FillChanProps(thisdevid, main_info[x].type, cur_m, main_info[x].count);
    }

    /* Big channels */
    for (cur_b = *cur_nbigcs,         x = 0;
                                      x < bigc_nsegs;
         cur_b += bigc_info[x].count, x++)
    {
        FillBigcProps(thisdevid, cur_b, &(bigc_info[x]));
    }

    /* Instance name */
    strzcpy(d_instname[thisdevid], inst_name, sizeof(d_instname[thisdevid]));
    
    /* Driver name */
    if (dev_name == NULL) dev_name = "-";
    drvinfobytes = strlen(dev_name) + 1;
    r = GrowBuf(&drvinfobuf, &drvinfobufsize, drvinfobufused + drvinfobytes);
    if (r < 0)
    {
        /*!!!???*/
    }
    CONSUME_DRVINFOBUF(&(d_drvname_o[thisdevid]), drvinfobytes, &drvinfobufused);
    memcpy(GET_DRVINFOBUF_PTR(d_drvname_o[thisdevid], char), dev_name, drvinfobytes);
    
    /* Auxinfo */
    drvinfobytes = 0;
    if (auxinfo != NULL  &&  auxinfo[0] != '\0')
    {
        drvinfobytes = strlen(auxinfo) + 1;
        r = GrowBuf(&drvinfobuf, &drvinfobufsize, drvinfobufused + drvinfobytes);
        if (r < 0)
        {
            /*!!!???*/
        }
    }
    CONSUME_DRVINFOBUF(&(d_auxinfo_o[thisdevid]), drvinfobytes, &drvinfobufused);
    if (drvinfobytes != 0)
        memcpy(GET_DRVINFOBUF_PTR(d_auxinfo_o[thisdevid], char), auxinfo, drvinfobytes);
    
    /* Initialize dev */
    (*cdevid)++;
    InitDev(thisdevid);

    if (drv_ld_problem  &&  !option_simulate)
        SetDevRflags(thisdevid, CXRF_NO_DRV);
    
    /* Advance counters */
    *cur_nchans += total_main_count;
    *cur_nbigcs += total_bigc_count;
    
    return 0;
}


/*==== blklist.txt reading code ====================================*/

static int GetArgsSizeUnits(CxsdBigcInfoRec *binfo, int which)
{
  int     nargs;
  int     size;  /*!!! size_t desired, but impossible because of GetInt() */
  size_t  units;
  char    c;

    if (GetInt(&nargs))         return 1;
    if (line[linepos++] != '+') return 1;
    if (GetInt(&size))          return 1;

    c = toupper(line[linepos++]);
    switch (c)
    {
        case 'B': units = 1; break;
        case 'S': units = 2; break;
        case 'I': units = 4; break;
        default:
            return 1;
    }

    size *= units;
    
    if (which == 0)
    /* Input */
    {
        binfo->nargs        = nargs;
        binfo->datasize     = size;
        binfo->dataunits    = units;
    }
    else
    /* Output */
    {
        binfo->ninfo        = nargs;
        binfo->retdatasize  = size;
        binfo->retdataunits = units;
    }

    return 0;
}

static int LogspecPluginParser(const char *str, const char **endptr,
                               void *rec, size_t recsize __attribute__((unused)),
                               const char *separators __attribute__((unused)), const char *terminators __attribute__((unused)),
                               void *privptr __attribute__((unused)), char **errstr)
{
  int         logmask;
  int        *lmp = (int *)rec;
  
  int         log_set_mask;
  int         log_clr_mask;
  char       *log_parse_r;
  
    logmask = option_defdrvlog_mask;
  
    if (str != NULL)
    {
        log_parse_r = ParseDrvlogCategories(str, endptr,
                                            &log_set_mask, &log_clr_mask);
        if (log_parse_r != NULL)
        {
            if (errstr != NULL) *errstr = log_parse_r;
            return PSP_R_USRERR;
        }
        
        logmask = (logmask &~ log_clr_mask) | log_set_mask;
    }

    *lmp = logmask;

    return PSP_R_OK;
}

typedef struct
{
    int  logmask;
} drvopts_t;

static psp_paramdescr_t text2drvopts[] =
{
    PSP_P_PLUGIN ("log", drvopts_t, logmask, LogspecPluginParser, NULL),
    PSP_P_END()
};

void SimulateDatabase(const char *argv0)
{
  #define     cur_devid    numdevs
  #define     cur_numchans numchans
  #define     cur_numbigcs numbigcs
    
  const char *filename;
  string      inst_name;
  string      module_name;
  string      layer_name;
  drvopts_t   drvopts;
  string      driver_name;
  int         businfocount;
  int32       businfo[CXSD_DB_MAX_BUSINFOCOUNT];
  int         bid_val;

  enum {MAX_MAIN_SEGS = 20, MAX_BIGC_SEGS = 20};
  int              main_nsegs;
  CxsdMainInfoRec  main_info[MAX_MAIN_SEGS];
  int              bigc_nsegs;
  CxsdBigcInfoRec  bigc_info[MAX_BIGC_SEGS];
  char             c;

  int         prev_id;
  
  const char *log_endptr;
  
    filename = option_dbref;

    lineno = 0;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr,
                "%s: %s(): fopen(\"%s\"): %s\n",
                argv0, __FUNCTION__, filename, strerror(errno));
        normal_exit = 1;
        exit(1);
    }

    numdevs      = 0;
    numchans     = 0;
    numbigcs     = 0;
    
    /* Consume devid=0 */
    AddDev(&numdevs, &numchans, &numbigcs,
           0, NULL,
           0, NULL,
           0, NULL,
           NULL,
           NULL, NULL,
           NULL,
           NULL,
           0);

    bzero(&drvopts, sizeof(drvopts));
    
    while (ReadLine())
    {
        /* Is there an optional ":devname:"? */
        inst_name[0] = '\0';
        if (linepos < linelen  &&  line[linepos] == ':')
        {
            linepos++;
            if (GetIdentifier(inst_name)  ||  line[linepos] != ':')
            {
                logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: 'devicename:' expected after ':'",
                        __FUNCTION__, filename, lineno, linepos);
                goto SKIP_BOGUS_LINE;
            }
            linepos++;

            /* Check for duplicate names */
            for (prev_id = 1;  prev_id < numdevs;  prev_id++)
                if (d_instname[prev_id][0] != '\0'  &&
                    strncasecmp(inst_name, d_instname[prev_id], sizeof(d_instname[prev_id]) - 1) == 0)
                    logline(LOGF_SYSTEM, 0, "%s: WARNING: %s.line#%d: devicename[%d] \"%s\" duplicates [%d]",
                            __FUNCTION__, filename, lineno, numdevs, d_instname[prev_id], prev_id);
        }
        
        /* Read the module/driver name (and optional "@layer") */
        if (!SkipWhite()  ||  GetIdentifier(module_name))
        {
            logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: module name expected",
                    __FUNCTION__, filename, lineno, linepos);
            goto SKIP_BOGUS_LINE;
        }
        layer_name[0] = '\0';
        if (line[linepos] == '@')
        {
            linepos++;
            if (GetIdentifier(layer_name))
            {
                logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: layer name expected",
                        __FUNCTION__, filename, lineno, linepos);
                goto SKIP_BOGUS_LINE;
            }
        }
        /* ...and optional ";log=..." */
        if (line[linepos] == ';')
        {
            linepos++;
            if (psp_parse(line + linepos, &log_endptr,
                          &drvopts,
                          '=', "", " \t",
                          text2drvopts) != PSP_R_OK)
            {
                logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: %s",
                        __FUNCTION__, filename, lineno, linepos, psp_error());
                goto SKIP_BOGUS_LINE;
            }

            linepos = log_endptr - line;
        }
        else
            /* Just initialize drvopts */
            psp_parse(NULL, NULL,
                      &drvopts,
                      '=', "", "",
                      text2drvopts);

        /* The almost-obsolete "mod_name" */
        if (!SkipWhite()  ||  GetIdentifier(driver_name))
        {
            logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: driver name expected",
                    __FUNCTION__, filename, lineno, linepos);
            goto SKIP_BOGUS_LINE;
        }

        /* Bus id */
        SkipWhite();
        if (line[linepos] == '-'  &&  isspace(line[linepos+1]))
        {
            businfocount = 0;
            linepos++;
        }
        else
        {
            for (businfocount = 0; ;)
            {
                if (businfocount >= countof(businfo))
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: too many businfo components (limit is %d)",
                            __FUNCTION__, filename, lineno, linepos, countof(businfo));
                    goto SKIP_BOGUS_LINE;
                }
                if (GetInt(&(bid_val)))
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: bus_id #%d (integer) expected",
                            __FUNCTION__, filename, lineno, linepos, businfocount);
                    goto SKIP_BOGUS_LINE;
                }
                businfo[businfocount++] = bid_val;

                if (line[linepos] != ',') break;
                linepos++;
            }
        }
        
        SkipWhite();
        
        /* Obtain main_info */
        main_nsegs = 0;
        if (line[linepos] == '-'  ||  line[linepos] == '0')
        {
            while (line[linepos] != '\0'  &&  !isspace(line[linepos]))
                linepos++;
        }
        else
        {
            while (1)
            {
                if (main_nsegs >= MAX_MAIN_SEGS)
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d: too many 'main channels' segments",
                            __FUNCTION__, filename, lineno);
                    goto SKIP_BOGUS_LINE;
                }

                c = toupper(line[linepos]);
                if      (c == 'R') main_info[main_nsegs].type = 0;
                else if (c == 'W') main_info[main_nsegs].type = 1;
                else
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d: invalid channel type char '%c'",
                            __FUNCTION__, filename, lineno, line[linepos]);
                    goto SKIP_BOGUS_LINE;
                }
                linepos++;

                if (GetInt(&(main_info[main_nsegs].count)))
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d@%d: syntax error",
                            __FUNCTION__, filename, lineno, linepos);
                    goto SKIP_BOGUS_LINE;
                }

                /* Proceed... */
                main_nsegs++;
                
                if (line[linepos] != ',') break;
                linepos++;
            }
        }

        SkipWhite();
        
        /* Obtain bigc_info */
        bigc_nsegs = 0;
        if (line[linepos] == '-'  ||  line[linepos] == '0')
        {
            while (line[linepos] != '\0'  &&  !isspace(line[linepos]))
                linepos++;
        }
        else
        {
            while (1)
            {
                if (bigc_nsegs >= MAX_BIGC_SEGS)
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d: too many 'big channels' segments",
                            __FUNCTION__, filename, lineno);
                    goto SKIP_BOGUS_LINE;
                }
                
                /*
                 numchans*nargs+datasizeU/ninfo+retdataU
                 U: b=1, s=2, i=4
                 !!!Note: we should check at least for NEGATIVE values.
                          And, probably, against CX_ABSOLUTE_MAX_DATASIZE too.
                 */
                if (GetInt(&(bigc_info[bigc_nsegs].count))         ||
                    line[linepos++] != '*'                        ||
                    GetArgsSizeUnits(&(bigc_info[bigc_nsegs]), 0)  ||
                    line[linepos++] != '/'                        ||
                    GetArgsSizeUnits(&(bigc_info[bigc_nsegs]), 1))
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d: syntax error in bigc_info#%d",
                            __FUNCTION__, filename, lineno, bigc_nsegs);
                    goto SKIP_BOGUS_LINE;
                }
                if (bigc_info[bigc_nsegs].nargs < 0  ||
                    bigc_info[bigc_nsegs].nargs > CX_MAX_BIGC_PARAMS)
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d: bigc_info#%d: nargs=%d, out_of[0...%d]",
                            __FUNCTION__, filename, lineno, bigc_nsegs,
                            bigc_info[bigc_nsegs].nargs, CX_MAX_BIGC_PARAMS);
                }
                if (bigc_info[bigc_nsegs].ninfo < 0  ||
                    bigc_info[bigc_nsegs].ninfo > CX_MAX_BIGC_PARAMS)
                {
                    logline(LOGF_SYSTEM, 0, "%s: %s.line#%d: bigc_info#%d: ninfo=%d, out_of[0...%d]",
                            __FUNCTION__, filename, lineno, bigc_nsegs,
                            bigc_info[bigc_nsegs].ninfo, CX_MAX_BIGC_PARAMS);
                }
                
                /* Proceed... */
                bigc_nsegs++;
                
                if (line[linepos] != ',') break;
                linepos++;
            }
        }

        SkipWhite();

        AddDev(&numdevs, &numchans, &numbigcs,
               main_nsegs, main_info,
               bigc_nsegs, bigc_info,
               businfocount, businfo,
               inst_name,
               module_name, driver_name,
               layer_name,
               line + linepos,
               drvopts.logmask);
        
 SKIP_BOGUS_LINE:;
    }

    fclose(fp);

    logline(LOGF_SYSTEM, 0, "%s: statistics: %d devs, %d channels, %d bigcs %zu bytes bigbuf",
            __FUNCTION__, numdevs, numchans, numbigcs, bigbufused);
///    usleep(3000000);/*XXX!!!*/
}
