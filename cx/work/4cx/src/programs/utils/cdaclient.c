#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include "misc_macros.h"
#include "misclib.h"
#include "cxscheduler.h"

#include "cx.h"
#include "cda.h"


//////////////////////////////////////////////////////////////////////

typedef struct
{
    int            in_use;
    cda_dataref_t  ref;
    cxdtype_t      dtype;
    int            n_items;
    char           dpyfmt[20];
} refrec_t;

static refrec_t *refrecs_list        = NULL;
static int       refrecs_list_allocd = 0;

// GetRefRecSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, RefRec, refrec_t,
                                 refrecs, in_use, 0, 1,
                                 0, 10, 1000,
                                 , , void)

static void RlsRefRecSlot(int rn)
{
  refrec_t *rp = AccessRefRecSlot(rn);

    rp->in_use = 0;
}

//////////////////////////////////////////////////////////////////////

enum
{
    EC_OK      = 0,
    EC_HELP    = 1,
    EC_USR_ERR = 2,
    EC_ERR     = 3,
};

static int   option_help = 0;

static char          *defpfx  = NULL;
static char          *baseref = NULL;
static cda_context_t  the_cid;


static void ProcessContextEvent(int            uniq,
                                void          *privptr1,
                                cda_context_t  cid,
                                int            reason,
                                int            info_int,
                                void          *privptr2)
{
fprintf(stderr, "%s(cid=%d, reason=%d, info_int=%d)\n", __FUNCTION__, cid, reason, info_int);
    switch (reason)
    {
    }
}

static void ProcessDatarefEvent(int            uniq,
                                void          *privptr1,
                                cda_dataref_t  ref,
                                int            reason,
                                void          *info_ptr,
                                void          *privptr2)
{
  int            rn = ptr2lint(privptr2);
  refrec_t      *rp = AccessRefRecSlot(rn);
  double         v;
  const char    *src_p;

    if (cda_get_dcval(ref, &v) >= 0)
    {
        if (cda_src_of_ref(ref, &src_p) < 0) src_p = "UNKNOWN";
        printf("%s  %d:%s: ", strcurtime(), ref, src_p);
        printf(rp->dpyfmt, v);
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
  int            c;
  int            err = 0;

  char           chanref[CDA_PATH_MAX];
  const char    *an_p;  // ArgvN
  const char    *bs_p;  // BegofSpec
  const char    *cl_p;  // ColoN ptr
  size_t         slen;
  char           ut_char;
  char          *endptr;

  cda_dataref_t  ref;
  cxdtype_t      dtype;
  int            n_items;
  char           dpyfmt[20];

  char    buf[100];
  int     r;

  int     flags;
  int     width;
  int     precision;
  char    conv_c;

  int            nrefs;

  int            rn;
  refrec_t      *rp;

  int            nsids;
  int            nth;

    /* Make stdout ALWAYS line-buffered */
    setvbuf(stdout, NULL, _IOLBF, 0);

    set_signal(SIGPIPE, SIG_IGN);

    while ((c = getopt(argc, argv, "b:d:h")) != EOF)
        switch (c)
        {
            case 'b':
                baseref = optarg;
                break;

            case 'd':
                defpfx  = optarg;
                break;

            case 'h':
                option_help = 1;
                break;

            case ':':
            case '?':
                err++;
        }

    if (option_help)
    {
        printf("Usage: %s [OPTIONS] {COMMANDS}\n"
               "    -b BASEREF\n"
               "    -h  show this help\n",
               argv[0]);
        exit(EC_HELP);
    }

    if (err)
    {
        fprintf(stderr, "Try '%s -h' for help.\n", argv[0]);
        exit(EC_USR_ERR);
    }

    //////////////////////////////////////////////////////////////////

    the_cid = cda_new_context(0, NULL, defpfx, 0, argv[0],
                              CDA_CTX_EVMASK_CYCLE, ProcessContextEvent, NULL);
    if (the_cid == CDA_CONTEXT_ERROR)
    {
        fprintf(stderr, "%s %s: cda_new_context(\"%s\"): %s\n",
                strcurtime(), argv[0],
                defpfx != NULL? defpfx : "",
                cda_last_err());
        exit(EC_ERR);
    }
    
    for (nrefs = 0;  optind < argc;  optind++)
    {
        dtype   = CXDTYPE_DOUBLE;
        n_items = 1;
        strcpy(dpyfmt, "%8.3f");

        an_p = argv[optind];
        bs_p = an_p;

        while (1)
        {
            if (*bs_p == '@')
            {
                bs_p++;
                ut_char = tolower(*bs_p);
                if      (ut_char == 'b') dtype = CXDTYPE_INT8;
                else if (ut_char == 'h') dtype = CXDTYPE_INT16;
                else if (ut_char == 'i') dtype = CXDTYPE_INT32;
                else if (ut_char == 'q') dtype = CXDTYPE_INT64;
                else if (ut_char == 's') dtype = CXDTYPE_SINGLE;
                else if (ut_char == 'd') dtype = CXDTYPE_DOUBLE;
                else if (ut_char == 't') dtype = CXDTYPE_TEXT;
                else if (ut_char == '\0')
                {
                    fprintf(stderr, "%s %s: data-type expected after '@' in \"%s\"\n",
                            strcurtime(), argv[0], an_p);
                    exit(EC_USR_ERR);
                }
                else
                {
                    fprintf(stderr, "%s %s: invalid channel-data-type after '@' in \"%s\"\n",
                            strcurtime(), argv[0], an_p);
                    exit(EC_USR_ERR);
                }
                // Optional COUNT
                bs_p++;
                if (isdigit(*bs_p))
                {
                    n_items = strtol(bs_p, &endptr, 10);
                    if (endptr == bs_p)
                    {
                        fprintf(stderr, "%s %s: invalid channel-n_items after '/%c' in \"%s\"\n",
                                strcurtime(), argv[0], ut_char, an_p);
                        exit(EC_USR_ERR);
                    }
                    bs_p = endptr;
                }
                // Mandatory ':' separator
                if (*bs_p != ':')
                {
                    fprintf(stderr, "%s %s: ':' expected after '@%c[N]' in \"%s\"\n",
                            strcurtime(), argv[0], ut_char, an_p);
                    exit(EC_USR_ERR);
                }
                else
                    bs_p++;
            }
            else if (*bs_p == '%')
            {
                cl_p = strchr(bs_p, ':');
                if (cl_p == NULL)
                {
                    fprintf(stderr, "%s %s: ':' expected after %%DPYFMT spec in \"%s\"\n",
                            strcurtime(), argv[0], an_p);
                    exit(EC_USR_ERR);
                }
                slen = cl_p - bs_p;
                if (slen > sizeof(buf) - 1)
                {
                    fprintf(stderr, "%s %s: %%DPYFMT spec too long in \"%s\"\n",
                            strcurtime(), argv[0], an_p);
                    exit(EC_USR_ERR);
                }
                memcpy(buf, bs_p, slen);
                buf[slen] = '\0';

                if (ParseDoubleFormat(GetTextFormat(buf),
                                      &flags, &width, &precision, &conv_c) < 0)
                if (slen > sizeof(buf) - 1)
                {
                    fprintf(stderr, "%s %s: invalid %%DPYFMT spec in \"%s\": %s\n",
                            strcurtime(), argv[0], an_p, printffmt_lasterr());
                    exit(EC_USR_ERR);
                }
                CreateDoubleFormat(dpyfmt, sizeof(dpyfmt), 20, 10,
                                   flags, width, precision, conv_c);

                bs_p = cl_p + 1;
            }
            else break;
        }

        slen = strlen(bs_p);
        if (slen > sizeof(chanref) - 1)
            slen = sizeof(chanref) - 1;
        memcpy(chanref, bs_p, slen); chanref[slen] = '\0';

        ref = cda_add_chan(the_cid,
                           baseref,
                           chanref, CDA_OPT_NONE/*!!!CDA_OPT_IS_W*/, dtype, n_items,
                           0, NULL, NULL);
        if (ref == CDA_DATAREF_ERROR)
        {
            fprintf(stderr, "%s %s: cda_add_dchan(\"%s\"): %s\n",
                    strcurtime(), argv[0], argv[optind], cda_last_err());
            /* Note: we do NOT exit(EC_ERR) and allow other references to proceed */
        }
        else
        {
            rn = GetRefRecSlot();
            if (rn >= 0)
            {
                rp = AccessRefRecSlot(rn);

                rp->ref     = ref;
                rp->dtype   = dtype;
                rp->n_items = n_items;
                strzcpy(rp->dpyfmt,  dpyfmt,  sizeof(rp->dpyfmt));

                cda_add_dataref_evproc(ref, CDA_REF_EVMASK_UPDATE, ProcessDatarefEvent, lint2ptr(rn));

                nrefs++;
            }
        }
    }

    if (nrefs == 0)
    {
        fprintf(stderr, "%s %s: no channels to work with\n",
                strcurtime(), argv[0]);
        exit(EC_USR_ERR);
    }

    nsids = cda_status_srvs_count(the_cid);
    printf("servers[%d]={", nsids);
    for (nth = 0;  nth < nsids;  nth++)
        printf("%s[%d] \"%s\"",
               nth == 0? "" : ", ",
               nth, cda_status_srv_name(the_cid, nth));
    printf("}\n");

    sl_main_loop();
    
    return EC_OK;
}
