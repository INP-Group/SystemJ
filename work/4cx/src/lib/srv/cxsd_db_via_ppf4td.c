#include <ctype.h>

#include "cxsd_db_via_ppf4td.h"


static void CxsdDbList(FILE *fp, CxsdDb db)
{
  int  i,j;
    
    fprintf(stderr, "devlist[%d]={\n", db->numdevs);
    for (i = 0; i < db->numdevs;  i++)
    {
        fprintf(stderr, "\tdev %s", db->devlist[i].instname);
        if (db->devlist[i].options != NULL) fprintf(stderr, ":%s", db->devlist[i].options);
        fprintf(stderr, " %s", db->devlist[i].typename);
        if (db->devlist[i].drvname[0] != '\0') fprintf(stderr, "/%s", db->devlist[i].drvname);
        if (db->devlist[i].lyrname[0] != '\0') fprintf(stderr, "@%s", db->devlist[i].lyrname);
        fprintf(stderr, " ");
        if (db->devlist[i].changrpcount == 0)
            fprintf(stderr, "-");
        else
            for (j = 0;  j < db->devlist[i].changrpcount; j++)
                fprintf(stderr, "%s%c%d%c%d",
                        j == 0? "" : ",",
                        db->devlist[i].changroups[j].rw?'w':'r',
                        db->devlist[i].changroups[j].count,
                        '.', db->devlist[i].changroups[j].nelems
                        );
        fprintf(stderr, " ");
        if (db->devlist[i].businfocount == 0)
            fprintf(stderr, "-");
        else
            for (j = 0;  j < db->devlist[i].businfocount; j++)
                fprintf(stderr, "%s%d",
                        j == 0? "" : ",",
                        db->devlist[i].businfo[j]
                        );
        if (db->devlist[i].auxinfo != NULL) fprintf(stderr, " %s", db->devlist[i].auxinfo);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "}\n");

    fprintf(stderr, "layerinfos[%d]={\n", db->numlios);
    for (i = 0; i < db->numlios;  i++)
        fprintf(stderr, "\tlayerinfo %s:%d <%s>\n",
                db->liolist[i].lyrname, db->liolist[i].bus_n, db->liolist[i].info);
    fprintf(stderr, "}\n");
}

//////////////////////////////////////////////////////////////////////

static int  IsAtEOL(ppf4td_ctx_t *ctx)
{
  int  ch;

    return
      ppf4td_is_at_eol(ctx)  ||
      (ppf4td_peekc(ctx, &ch) > 0  &&  ch == '#');
}

static void SkipToNextLine(ppf4td_ctx_t *ctx)
{
  int  ch;

    /* 1. Skip non-CR/LF */
    while (ppf4td_peekc(ctx, &ch) > 0  &&
           (ch != '\r'  &&  ch != '\n')) ppf4td_nextc(ctx, &ch);
    /* 2. Skip all CR/LF (this skips multiple empty lines too) */
    while (ppf4td_peekc(ctx, &ch) > 0  &&
           (ch == '\r'  ||  ch == '\n')) ppf4td_nextc(ctx, &ch);
}

static int ParseXName(const char *argv0, ppf4td_ctx_t *ctx,
                      int flags,
                      const char *name, char *buf, size_t bufsize)
{
  int  r;

    r = ppf4td_get_ident(ctx, flags, buf, bufsize);
    if (r < 0)
    {
        fprintf(stderr, "%s %s: %s:%d: %s expected, %s\n",
                strcurtime(), argv0,
                ppf4td_cur_ref(ctx), ppf4td_cur_line(ctx), name, ppf4td_strerror(errno));
        return -1;
    }

    return 0;
}

static int ParseAName(const char *argv0, ppf4td_ctx_t *ctx,
                      const char *name, char *buf, size_t bufsize)
{
    return ParseXName(argv0, ctx, PPF4TD_FLAG_NONE, name, buf, bufsize);
}

static int ParseDName(const char *argv0, ppf4td_ctx_t *ctx,
                      const char *name, char *buf, size_t bufsize)
{
    return ParseXName(argv0, ctx, PPF4TD_FLAG_DASH, name, buf, bufsize);
}

//////////////////////////////////////////////////////////////////////

typedef struct
{
    const char  *name;
    int        (*parser)(const char *argv0, ppf4td_ctx_t *ctx, CxsdDb db);
} keyworddef_t;

#define BARK(format, params...)         \
    (fprintf(stderr, "%s %s: %s:%d: ",  \
             strcurtime(), argv0, ppf4td_cur_ref(ctx), ppf4td_cur_line(ctx)), \
     fprintf(stderr, format, ##params), \
     fprintf(stderr, "\n"),             \
     -1)

static int GetToken(const char *argv0, ppf4td_ctx_t *ctx, char *buf, size_t bufsize, const char *location)
{
  int  r;

    r = ppf4td_get_string(ctx,
                          PPF4TD_FLAG_NOQUOTES |
                          PPF4TD_FLAG_SPCTERM | PPF4TD_FLAG_HSHTERM | PPF4TD_FLAG_EOLTERM |
                          PPF4TD_FLAG_COMTERM | PPF4TD_FLAG_SEMTERM,
                          buf, bufsize);
    if (r < 0)
        return BARK("%s; %s", location, ppf4td_strerror(errno));
    if (buf[0] == '0')
        return BARK("%s", location);

    return 0;
}

static int ParseChangroup(const char *argv0, ppf4td_ctx_t *ctx,
                          const char *buf,
                          CxsdChanInfoRec *rec, int *n_p)
{
  const char *p = buf;
  char       *endptr;
  char        rw_char;
  char        ut_char;

    rec += *n_p;

    /* 1. R/W */
    rw_char = tolower(*p++);
    if      (rw_char == 'r') rec->rw = 0;
    else if (rw_char == 'w') rec->rw = 1;
    else
        return BARK("chan-group-%d should start with either 'r' or 'w'", *n_p);

    /* 2. # of channels in this segment */
    rec->count = strtol(p, &endptr, 10);
    if (endptr == p)
        return BARK("invalid channel-count spec in chan-group-%d", *n_p);
    if (rec->count < 0)
        return BARK("negative channel-count spec %d in chan-group-%d",
                    rec->count, *n_p);
    p = endptr;

    /* 3. Data type */
    ut_char = tolower(*p++);
    if      (ut_char == 'b') rec->dtype = CXDTYPE_INT8;
    else if (ut_char == 'h') rec->dtype = CXDTYPE_INT16;
    else if (ut_char == 'i') rec->dtype = CXDTYPE_INT32;
    else if (ut_char == 'q') rec->dtype = CXDTYPE_INT64;
    else if (ut_char == 's') rec->dtype = CXDTYPE_SINGLE;
    else if (ut_char == 'd') rec->dtype = CXDTYPE_DOUBLE;
    else if (ut_char == 't') rec->dtype = CXDTYPE_TEXT;
    else if (ut_char == '\0')
        return BARK("channel-data-type expected after \"%d\" in chan-group-%d",
                    rec->count, *n_p);
    else
        return BARK("invalid channel-data-type in chan-group-%d", *n_p);

    /* 4. Optional "Max # of units" */
    if (*p == '\0')
    {
        rec->nelems = 1;
        return 0;
    }
    rec->nelems = strtol(p, &endptr, 10);
    if (endptr == p)
        return BARK("invalid max-units-count spec in chan-group-%d", *n_p);
    if (rec->nelems <= 0)
        return BARK("invalid max-units-count %d in chan-group-%d",
                    rec->nelems, *n_p);
    p = endptr;

    /* Finally, that must be end... */
    if (*p != '\0')
        return BARK("junk after max-units-count %d in chan-group-%d",
                    rec->nelems, *n_p);

    return +1;
}

static int dev_parser(const char *argv0, ppf4td_ctx_t *ctx, CxsdDb db)
{
  CxsdDbDevLine_t    dline;
  int                r;
  int                ch;
  char               optbuf[1000];
  char               auxbuf[1000];
  char               tokbuf[30];
  char              *endptr;
  const char        *loc;
    
    bzero(&dline, sizeof(dline));
    optbuf[0] = auxbuf[0] = '\0';

    /* INSTANCE_NAME */
    if (ParseAName(argv0, ctx, "device-instance-name",
                   dline.instname, sizeof(dline.instname)) < 0) return -1;

    /* [:OPTIONS] */
    r = ppf4td_peekc(ctx, &ch);
    if (r < 0) return -1;
    if (r > 0  &&  ch == ':')
    {
        ppf4td_nextc(ctx, &ch);
        r = ppf4td_get_string(ctx,
                              PPF4TD_FLAG_SPCTERM | PPF4TD_FLAG_HSHTERM | PPF4TD_FLAG_EOLTERM,
                              optbuf, sizeof(optbuf));
        if (r < 0)
            return BARK("device-options expected after ':'; %s", ppf4td_strerror(errno));
    }

    /* TYPE */
    ppf4td_skip_white(ctx);
    if (ParseAName(argv0, ctx, "device-type",
                   dline.typename, sizeof(dline.typename)) < 0) return -1;

    /* [/DRIVER] */
    r = ppf4td_peekc(ctx, &ch);
    if (r < 0) return -1;
    if (r > 0  &&  ch == '/')
    {
        ppf4td_nextc(ctx, &ch);
        if (ParseAName(argv0, ctx, "driver-name (after '/')",
                       dline.drvname, sizeof(dline.drvname)) < 0) return -1;
    }

    /* [@LAYER] */
    r = ppf4td_peekc(ctx, &ch);
    if (r < 0) return -1;
    if (r > 0  &&  ch == '@')
    {
        ppf4td_nextc(ctx, &ch);
        if (ParseAName(argv0, ctx, "layer-name (after '@')",
                       dline.lyrname, sizeof(dline.lyrname)) < 0) return -1;
    }

    /* CHANINFO */
    ppf4td_skip_white(ctx);
    r = ppf4td_peekc(ctx, &ch);
    if (r < 0) return -1;
    if (r > 0  &&  ch == '~')
    {
        ppf4td_nextc(ctx, &ch);

    }
    else
    {
        for (dline.changrpcount = 0, loc = "chan-group";
             ;
                                     loc = "chan-group after ','")
        {
            if (dline.changrpcount >= countof(dline.changroups))
                return BARK("too many chan-group-components (>%d)",
                            countof(dline.businfo));

            if (GetToken(argv0, ctx, tokbuf, sizeof(tokbuf), loc) < 0)
                return -1;

            /* "-" means "no channels" */
            if (dline.changrpcount == 0  &&  strcmp(tokbuf, "-") == 0) break;

            r = ParseChangroup(argv0, ctx, tokbuf,
                               dline.changroups, &(dline.changrpcount));
            if (r < 0) return -1;
            // if (r  == 0) break;

            dline.changrpcount++;

            r = ppf4td_peekc(ctx, &ch);
            if (r < 0) return -1;
            if (r > 0  &&  ch == ',')
            {
                ppf4td_nextc(ctx, &ch);
            }
            else
                break;
        }
    }

    /* BUSINFO */
    ppf4td_skip_white(ctx);
    for (dline.businfocount = 0, loc = "bus-id";
         ;
                                 loc = "bus-id after ','")
    {
        if (dline.businfocount >= countof(dline.businfo))
            return BARK("too many bus-info-components (>%d)",
                        countof(dline.businfo));

        if (GetToken(argv0, ctx, tokbuf, sizeof(tokbuf), loc) < 0)
            return -1;
        
        /* "-" means "no channels" */
        if (dline.businfocount == 0  &&  strcmp(tokbuf, "-") == 0) break;
        
        dline.businfo[dline.businfocount] = strtol(tokbuf, &endptr, 10);
        if (endptr == tokbuf  ||  *endptr != '\0')
            return BARK("invalid bus-id-%d specification \"%s\"",
                        dline.businfocount, tokbuf);

        dline.businfocount++;

        r = ppf4td_peekc(ctx, &ch);
        if (r < 0) return -1;
        if (r > 0  &&  ch == ',')
        {
            ppf4td_nextc(ctx, &ch);
        }
        else
            break;
    }

    /* AUXINFO */
    ppf4td_skip_white(ctx);
    r = ppf4td_get_string(ctx, PPF4TD_FLAG_HSHTERM | PPF4TD_FLAG_EOLTERM,
                          auxbuf, sizeof(auxbuf));
    if (r < 0)
        return BARK("auxinfo-string expected; %s", ppf4td_strerror(errno));

    if (CxsdDbAddDev(db, &dline, optbuf, auxbuf))
        return BARK("unable to CxsdDbAddDev(%s): %s",
                    dline.instname, strerror(errno));

    return 0;
}

static int devtype_parser(const char *argv0, ppf4td_ctx_t *ctx, CxsdDb db)
{

    return 0;
}

static int layerinfo_parser(const char *argv0, ppf4td_ctx_t *ctx, CxsdDb db)
{
  CxsdDbLayerinfo_t  lyrinfo;
  int                r;
  int                ch;
  char               numbuf[10];
  char              *endptr;
  char               infobuf[1000];

    bzero(&lyrinfo, sizeof(lyrinfo));

    if (ParseAName(argv0, ctx, "layer-name",
                   lyrinfo.lyrname, sizeof(lyrinfo.lyrname)) < 0) return -1;
    r = ppf4td_peekc(ctx, &ch);
    if (r < 0) return -1;
    if (r > 0  &&  ch == ':')
    {
        ppf4td_nextc(ctx, &ch);
        if (ParseAName(argv0, ctx, "bus-number",
                       numbuf, sizeof(numbuf)) < 0) return -1;
        lyrinfo.bus_n = strtol(numbuf, &endptr, 10);
        if (endptr == numbuf  ||  *endptr != '\0')
            return BARK("invalid bus-number specification \"%s\"", numbuf);
    }
    else
        lyrinfo.bus_n = -1;

    if (!IsAtEOL(ctx)  &&  ppf4td_peekc(ctx, &ch) > 0  &&  !isspace(ch))
        return BARK("junk after layer-name[:bus-number] spec");
    ppf4td_skip_white(ctx);

    r = ppf4td_get_string(ctx, PPF4TD_FLAG_HSHTERM | PPF4TD_FLAG_EOLTERM,
                          infobuf, sizeof(infobuf));
    if (r < 0)
        return BARK("layer-info-string expected; %s", ppf4td_strerror(errno));

    if (CxsdDbAddLyrI(db, &lyrinfo, infobuf))
        return BARK("unable to CxsdDbAddLyrI(%s:%d): %s",
                    lyrinfo.lyrname, lyrinfo.bus_n, strerror(errno));

    return 0;
}

static keyworddef_t keyword_list[] =
{
    {"dev",       dev_parser},
    {"channels",  NULL},
    {"devtype",   devtype_parser},
    {"cpoint",    NULL},
    {"layerinfo", layerinfo_parser},
    {NULL, NULL}
};

CxsdDb CxsdDbLoadDbViaPPF4TD(const char *argv0, ppf4td_ctx_t *ctx)
{
  CxsdDb        db  = NULL;

  int           r;
  int           ch;

  char          keyword[50];
  keyworddef_t *kdp;

    db = CxsdDbCreate();
    if (db == NULL)
    {
        fprintf(stderr, "%s %s: unable to CxsdDbCreate()\n",
                strcurtime(), argv0);
        goto FATAL;
    }

    while (1)
    {
        ppf4td_skip_white(ctx);
        if (IsAtEOL(ctx)) goto SKIP_TO_NEXT_LINE;

        if (ParseAName(argv0, ctx, "keyword", keyword, sizeof(keyword)) < 0) goto FATAL;

        for (kdp = keyword_list;  kdp->name != NULL;  kdp++)
            if (strcasecmp(keyword, kdp->name) == 0) break;
        if (kdp->name == NULL)
        {
            fprintf(stderr, "%s %s: %s:%d: unknown keyword \"%s\"\n",
                    strcurtime(), argv0, ppf4td_cur_ref(ctx), ppf4td_cur_line(ctx),
                    keyword);
            goto FATAL;
        }

        fprintf(stderr, "->%s\n", kdp->name);
        ppf4td_skip_white(ctx);
        r = kdp->parser(argv0, ctx, db);
        if (r != 0) goto FATAL;

 SKIP_TO_NEXT_LINE:
        SkipToNextLine(ctx);
        /* Check for EOF */
        if (ppf4td_peekc(ctx, &ch) <= 0) goto END_PARSE_FILE;
    }
 END_PARSE_FILE:

    ppf4td_close(ctx);

    if (1) CxsdDbList(stderr, db);

    return db;

 FATAL:
    ppf4td_close(ctx);
    CxsdDbDestroy(db);
    return NULL;
}
