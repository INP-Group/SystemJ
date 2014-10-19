#include "cxsd_includes.h"


enum {DEVSCB_MAGIC = 0x42437344};  // Little-endian 'DsCB'


static void CallDevsOnchgCallbacks(int devid, int newstate)
{
  devs_cbitem_t *cbp;
  devs_cbitem_t *next_cbp;

    for (cbp  = d_cblist[devid];
         cbp != NULL  &&  cbp->magicnumber == DEVSCB_MAGIC;
         cbp = next_cbp)
    {
        next_cbp = cbp->next;
        if (cbp->callback != NULL)
            cbp->callback(devid, cbp, newstate);
    }
}

int  InitDriver(CxsdDriverModRec *metric)
{
  int                    drv;
  int                    r;
  
  static CxsdDriverModRec *already_started[CX_MAX_DEVS];
  static int               already_code   [CX_MAX_DEVS];
  static int               already_count = 0;

    /* Find if this driver is already in the list */
    for (drv = 0;  drv < already_count;  drv++)
        if (already_started[drv] == metric)
            return already_code[drv];

    /* No, it isn't.  So: */

    /* 1. Init it */
    r = 0;
    if (!option_simulate)
    {
        if (metric->mr.init_mod != NULL)
            r = (metric->mr.init_mod)();
    }

    /* 2. Remember it */
    already_started[already_count] = metric;
    already_code   [already_count] = r;
    already_count++;

    return r;
}


void SetDevRflags(int devid, rflags_t rflags_to_set)
{
  int  x;

////fprintf(stderr, "%s(%d) %d; [%d,+%d)\n", __FUNCTION__, devid, rflags_to_set, d_ofs[devid], d_size[devid]);
    if (rflags_to_set != 0)
    {
        for (x = d_ofs[devid];  x < d_ofs[devid] + d_size[devid];  x++)
        {
            c_rflags [x] |= rflags_to_set;
            c_crflags[x] |= rflags_to_set;
        }
        /*!!! Do the same to big channels...*/
        for (x = d_big_first[devid];  x < d_big_first[devid] + d_big_count[devid];  x++)
        {
            b_data[x].cur_rflags |= rflags_to_set;
            b_data[x].crflags    |= rflags_to_set;
        }
    }
}

void RstDevRflags(int devid)
{
    bzero(&(c_rflags [d_ofs[devid]]), d_size[devid] * sizeof(c_rflags [0]));
    bzero(&(c_crflags[d_ofs[devid]]), d_size[devid] * sizeof(c_crflags[0]));
}

static void stop_dev(int devid, int call_term, rflags_t set_in_rflags);

void InitDev  (int devid)
{
  int  s_devid;
  int               state;
  CxsdDriverModRec *metric  = d_metric[devid];
  const char       *auxinfo = GET_DRVINFOBUF_PTR(d_auxinfo_o[devid], char);

    if (!option_simulate)
    {
        if (d_businfocount[devid] < metric->min_businfo_n  ||
            d_businfocount[devid] > metric->max_businfo_n)
        {
            DoDriverLog(devid, 0, "businfocount=%d, out_of[%d,%d]",
                        d_businfocount[devid],
                        metric->min_businfo_n, metric->max_businfo_n);
            stop_dev(devid, 0, CXRF_CFG_PROBL);
            return;
        }

        if (metric->privrecsize != 0)
        {
            d_devptr[devid] = malloc(metric->privrecsize);
            if (d_devptr[devid] == NULL);
            /*!!!Should check!  And return "device initialization failure"!*/
            bzero(d_devptr[devid], metric->privrecsize);

            if (metric->paramtable != NULL)
            {
                if (psp_parse(auxinfo, NULL,
                              d_devptr[devid],
                              '=', " \t", "",
                              metric->paramtable) != PSP_R_OK)
                {
                    DoDriverLog(devid, 0, "psp_parse(auxinfo)@server: %s", psp_error());
                    stop_dev(devid, 0, CXRF_CFG_PROBL);
                    return;
                }
            }
        }

        ENTER_DRIVER_S(devid, s_devid);

        d_doio  [devid] = metric->do_rw;
        d_dobig [devid] = metric->do_big;

        state = d_state[devid] = DEVSTATE_OPERATING;
        //d_state[devid] = DEVSTATE_NOTREADY;
        if (metric->init_dev != NULL)
            state = (metric->init_dev)(devid, d_devptr[devid],
                                       d_businfocount[devid], d_businfo[devid],
                                       auxinfo);

        d_state[devid] = DEVSTATE_OPERATING;
        gettimeofday(d_stattime + devid, NULL);
        d_statflags[devid] = 0;
        
        LEAVE_DRIVER_S(s_devid);

        /* Was initialization really successful? */
        if (state < 0)
        {
            logline(LOGF_SYSTEM, 0, "%s: %s[%d].init_dev()=%d -- refusal",
                    __FUNCTION__, metric->mr.name, devid, state);
            stop_dev(devid, 0,
                     state == -1? 0 : -state);
        }
        else if (state == DEVSTATE_NOTREADY)
            SetDevState(devid, DEVSTATE_NOTREADY,  0, "init_dev NOTREADY");
        else
            SetDevState(devid, DEVSTATE_OPERATING, 0, NULL);
    }
}

static void stop_dev(int devid, int call_term, rflags_t set_in_rflags)
{
  int  x;
  int  fd;
    
    /* I. Call its termination function */
    if (d_metric[devid]->term_dev != NULL  &&  call_term)
        d_metric[devid]->term_dev(devid, d_devptr[devid]);
    
    /* II. Free all of its resources which are still left */

    /* Private pointer */
    if (d_metric[devid]->privrecsize != 0  &&  d_devptr[devid] != NULL)
    {
        if (d_metric[devid]->paramtable != NULL)
            psp_free(d_devptr[devid], d_metric[devid]->paramtable);
        free(d_devptr[devid]);
    }
    d_devptr[devid] = NULL;

    /* Other resources */
    fdio_do_cleanup(devid);
    sl_do_cleanup  (devid);

    /* Devstat-, channel- and bigc-callbacks */
    for (x = 0;  x < countof(d_devstat_events[devid]);  x++)
        if (d_devstat_events[devid][x].is_used)
            inserver_del_devstat_evproc(devid, d_devstat_events[devid][x].ref,
                                        d_devstat_events[devid][x].proc, d_devstat_events[devid][x].privptr);
    for (x = 0;  x < countof(d_chanref_events[devid]);  x++)
        if (d_chanref_events[devid][x].is_used)
            inserver_del_chanref_evproc(devid, d_chanref_events[devid][x].ref,
                                        d_chanref_events[devid][x].proc, d_chanref_events[devid][x].privptr);
    for (x = 0;  x < countof(d_bigcref_events[devid]);  x++)
        if (d_bigcref_events[devid][x].is_used)
            inserver_del_bigcref_evproc(devid, d_bigcref_events[devid][x].ref,
                                        d_bigcref_events[devid][x].proc, d_bigcref_events[devid][x].privptr);
    for (x = 0;  x < countof(d_bigcrq_holders[devid]);  x++)
        if (d_bigcrq_holders[devid][x].is_used)
        {
            DelBigcRequest(&(d_bigcrq_holders[devid][x].bigclistitem));
            d_bigcrq_holders[devid][x].is_used = 0;
        }
    
    d_doio [devid] = NULL;
    d_dobig[devid] = NULL;

    d_state[devid] = DEVSTATE_OFFLINE;
    gettimeofday(d_stattime + devid, NULL);
    d_statflags[devid] = set_in_rflags;

    /* Modify rflags if requested */
    SetDevRflags(devid, set_in_rflags | CXRF_OFFLINE);

    /* Notify everybody about the funeral... */
    CallDevsOnchgCallbacks(devid, DEVSTATE_OFFLINE);

    /* ...and perform cleanup */
    for (x = 0;  x < d_size     [devid];  x++)
        ClnChanOnRetCbList (d_ofs      [devid] + x);
    for (x = 0;  x < d_big_count[devid];  x++)
        ClnBigcOnRetCbList (d_big_first[devid] + x);
    for (x = 0;  x < d_big_count[devid];  x++)
        ClnBigcRequestChain(d_big_first[devid] + x);

    /* Mark all its channels as "unrequested" */
    RemitChanRequestsOf(devid);
    RemitBigcRequestsOf(devid);
}

void TermDev  (int devid, rflags_t rflags_to_set)
{
  int  s_devid;

    logline(LOGF_SYSTEM, 0, "%s: %s[%d]",
            __FUNCTION__, d_metric[devid]->mr.name, devid);
    
    if (!option_simulate)
    {
        if (d_state[devid] == DEVSTATE_OFFLINE) return;
        
        ENTER_DRIVER_S(devid, s_devid);
        stop_dev(devid, 1, rflags_to_set);
        LEAVE_DRIVER_S(s_devid);
    }

    d_state[devid] = DEVSTATE_OFFLINE;
    gettimeofday(d_stattime + devid, NULL);
    d_statflags[devid] = rflags_to_set;
}

void FreezeDev(int devid, rflags_t rflags_to_set)
{
    d_state[devid] = DEVSTATE_NOTREADY;
    gettimeofday(d_stattime + devid, NULL);
    d_statflags[devid] = rflags_to_set;

    SetDevRflags(devid, rflags_to_set | CXRF_OFFLINE);

    CallDevsOnchgCallbacks(devid, DEVSTATE_NOTREADY);
}

void ReviveDev(int devid)
{
    d_state[devid] = DEVSTATE_OPERATING;
    gettimeofday(d_stattime + devid, NULL);
    d_statflags[devid] = 0;
    
    /*!!! Temporary measure -- should be somewhere else */
    if (d_size[devid] != 0)
    {
        bzero(c_time    + d_ofs[devid], d_size[devid] * sizeof(c_time   [0]));
        bzero(c_wr_time + d_ofs[devid], d_size[devid] * sizeof(c_wr_time[0]));
    }
    
    ReRequestDevChans   (devid);
    ReRequestDevBigcs   (devid);
    RequestReadOfWrChsOf(devid);

    CallDevsOnchgCallbacks(devid, DEVSTATE_OPERATING);
}

void ResetDev(int devid)  /*!!! Does it belong here?  Or to cxsd_driver.c? */
{
    /* Check if `devid' is sane */
    if (devid < 0  ||  devid >= numdevs)
    {
        logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d): devid out of [0..numdevs=%d)",
                __FUNCTION__, devid, active_dev, numdevs);
        return;
    }

    /* Stop the dev... */
    TermDev(devid, 0);

    /*!!! Temporary measure -- should be somewhere else */
    if (d_size[devid] != 0)
        bzero(c_time + d_ofs[devid], d_size[devid] * sizeof(c_time[0]));
    
    /* ...and start it again */
    InitDev(devid);

    RequestReadOfWrChsOf(devid);
    ReRequestDevBigcs   (devid);
}

//////////////////////////////////////////////////////////////////////

static int AddToOnchgList  (int devid,  devs_cbitem_t *item)
{
    item->prev = d_cblist_end[devid];
    item->next = NULL;

    if (d_cblist_end[devid] != NULL)
        d_cblist_end[devid]->next = item;
    else
        d_cblist[devid]           = item;
    d_cblist_end[devid] = item;

    return 0;
}

static int DelFromOnchgList(int devid,  devs_cbitem_t *item)
{
  devs_cbitem_t *p1, *p2;

    p1 = item->prev;
    p2 = item->next;

    if (p1 == NULL) d_cblist    [devid] = p2; else p1->next = p2;
    if (p2 == NULL) d_cblist_end[devid] = p1; else p2->prev = p1;
  
    item->prev = item->next = NULL;
    
    return 0;
}

int AddDevsOnchgCallback(int devid, devs_cbitem_t *cbrec)
{
    /* Check if this 'item' is already used */
    if (cbrec->magicnumber != 0)
    {
        logline(LOGF_SYSTEM, 0, "%s(devid=%d): cbrec->magicnumber != 0",
                __FUNCTION__, devid);
        return -1;
    }

    /* Fill in the fields */
    cbrec->magicnumber = DEVSCB_MAGIC;
    
    /* Physically add to the chain */
    AddToOnchgList(devid, cbrec);

    return 0;
}

int DelDevsOnchgCallback(int devid, devs_cbitem_t *cbrec)
{
    /* Check if it is in the list... */
    if (cbrec->magicnumber != DEVSCB_MAGIC) return 1;
    /* ...and if it is in OUR list */
    /*!!!*/

    /* Physicaly remove from the chain */
    DelFromOnchgList(devid, cbrec);
    
    /* Clean the item */
    bzero(cbrec, sizeof(*cbrec));
    
    return 0;
}

typedef struct
{
    const char *name;
    int         c;
} catdesc_t;

/* Note: this list should be kept in sync/order with
   DRIVERLOG_CN_ enums in cxsd_driver.h */
static catdesc_t  catlist[] =
{
    {"DEFAULT",           DRIVERLOG_C_DEFAULT},
    {"ENTRYPOINT",        DRIVERLOG_C_ENTRYPOINT},
    {"PKTDUMP",           DRIVERLOG_C_PKTDUMP},
    {"PKTINFO",           DRIVERLOG_C_PKTINFO},
    {"DATACONV",          DRIVERLOG_C_DATACONV},
    {"REMDRV_PKTDUMP",    DRIVERLOG_C_REMDRV_PKTDUMP},
    {"REMDRV_PKTINFO",    DRIVERLOG_C_REMDRV_PKTINFO},
    {NULL,         0}
};

char * ParseDrvlogCategories(const char *str, const char **endptr,
                             int *set_mask_p, int *clr_mask_p)
{
  int          set_mask = 0;
  int          clr_mask = 0;
  const char  *srcp;
  const char  *name_b;
  int          op_is_set;
  char         namebuf[100];
  size_t       namelen;
  int          cat_value;
  
  static char  errdescr[100];

  catdesc_t   *cat;
  
    srcp = str;
    errdescr[0] = '\0';

    if (srcp != NULL)
        while (1)
        {
            op_is_set = 1;
            
            /* A leading '+'/'-'? */
            if (*srcp == '+'  ||  *srcp == '-')
            {
                op_is_set = (*srcp == '+');
                srcp++;
            }

            /* Okay, let's extract the name... */
            name_b = srcp;
            while (isalnum(*srcp)  ||  *srcp == '_') srcp++;

            /* Did we hit the terminator? */
            if (srcp == name_b)
            {
                /*!!!Here we should check that there was no orphan '+'/'-' --
                 if (srcp!=str && (*(srcp-1) == '+' || *(srcp-1) == '-') {error} */
                goto END_PARSE;
            }

            /* Extract name... */
            namelen = srcp - name_b;
            if (namelen > sizeof(namebuf) - 1) namelen = sizeof(namebuf) - 1;
            memcpy(namebuf, name_b, namelen);
            namebuf[namelen] = '\0';
            
            /* Find category in the list... */
            cat_value = DRIVERLOG_m_CHECKMASK_ALL;
            for (cat = catlist;  cat->name != NULL;  cat++)
                if (strcasecmp(namebuf, cat->name) == 0)
                {
                    cat_value = DRIVERLOG_m_C_TO_CHECKMASK(cat->c);
                    break;
                }

            if (cat->name != NULL  ||  strcasecmp(namebuf, "all") == 0)
            {
                if (op_is_set) set_mask |= cat_value;
                else           clr_mask |= cat_value;
            }
            else
            {
                if (errdescr[0] == '\0')
                    snprintf(errdescr, sizeof(errdescr),
                             "Unrecognized drvlog category '%s'", namebuf);
            }
            
            /* Well, if there is a ',', there must be more to parse... */
            if (*srcp != ',') goto END_PARSE;
            srcp++;
        }
  
 END_PARSE:
    
    if (set_mask_p != NULL) *set_mask_p = set_mask;
    if (clr_mask_p != NULL) *clr_mask_p = clr_mask;
    if (endptr != NULL) *endptr = srcp;

    return errdescr[0] == '\0'? NULL : errdescr;
}

const char *GetDrvlogCatName(int category)
{
    category = (category & DRIVERLOG_C_mask) >> DRIVERLOG_C_shift;
    if (category > DRIVERLOG_CN_default  ||  category < countof(catlist) - 1)
        return catlist[category].name;
    else
        return "";
}
