#include "cxsd_includes.h"


#define CHECK_SANITY_OF_DEVID_WO_STATE(errret)             \
    if (devid < 0  ||  devid >= numdevs)                   \
    {                                                      \
        logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d): devid out of [0..numdevs=%d)", \
                __FUNCTION__, devid, active_dev, numdevs); \
        return errret;                                     \
    }
#define CHECK_SANITY_OF_DEVID(errret)                      \
    CHECK_SANITY_OF_DEVID_WO_STATE(errret)                 \
    if (d_state[devid] == DEVSTATE_OFFLINE)                \
    {                                                      \
        logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d): state=OFFLINE", \
                __FUNCTION__, devid, active_dev);          \
        return errret;                                     \
    }


enum {DRVA_IGNORE = -999};

enum {CHANCB_MAGIC = 0x42436e43};  // Little-endian 'CnCB'


static int IsReqRofWrChsIng = 0; // Is-(request-read-of-write-channels)'ing


tag_t FreshFlag(int chan)
{
    if (c_type[chan] == 0  ||  !option_cacherfw)
        return age_of(c_time[chan]);
    else
    {
        if (c_wr_req[chan] | c_rd_req[chan]) return age_of(c_wr_time[chan] - 1);
        else                                 return 0;
    }
}

static void SendChanRequest(int devid, int g_start, int count, int32 *values, int action)
{
  int    s_devid;

    ENTER_DRIVER_S(devid, s_devid);
    if (option_simulate)
    {
        if (action == DRVA_WRITE)
            memcpy(c_simnew + g_start, values, count * sizeof(c_simnew[0]));
    }
    else
    {
        if (d_doio[devid] != NULL  &&  d_state[devid] == DEVSTATE_OPERATING)
            d_doio[devid](devid, d_devptr[devid],
                          g_start - d_ofs[devid], count,
                          values, action);
    }
    LEAVE_DRIVER_S(s_devid);
}

static int ConsiderRequest(int chan, int32 *v_p, int action)
{
    /* Is it a readonly channel? */
    if (c_type[chan] == 0)
    {
        return (c_rd_req[chan]  ||  c_time[chan] == current_cycle)? DRVA_IGNORE
                                                                  : DRVA_READ;
    }

    /* Okay, it is a write channel...  What do we need? */
    if (action == DRVA_WRITE)
    {
        c_next_wr_val[chan] = *v_p;
        if (c_wr_req[chan])
        {
            c_next_wr_val_p    [chan] = 1;
            c_next_wr_val_time [chan] = current_cycle;
            return DRVA_IGNORE;
        }
        else
        {
            return DRVA_WRITE;
        }
    }
    else
    {
        if ((option_cacherfw  && !IsReqRofWrChsIng)  ||
            c_rd_req[chan] || c_wr_req[chan]                     ||
            c_time[chan] == current_cycle)
            return DRVA_IGNORE;
        else
            return DRVA_READ;
    }
}

void MarkRangeForIO(chanaddr_t start, int count, int action, int32 *values)
{
  int    barrier = start + count;

  int    g;
  int    first;
  int    length;
  int    devid;
  int    f_act;

    for (g = start;  g < barrier;  /*NO-OP*/)
    {
        /* Get "model" parameters */
        first = g;
        devid = c_devid[first];
        f_act = ConsiderRequest(first, values + (first - start), action);
        g++;

        /* Find out how many channels can be packed */
        while (g < barrier          &&
               c_devid[g] == devid  &&
               ConsiderRequest(g, values + (g - start), action) == f_act)
            g++;

        length = g - first;

        switch (f_act)
        {
            case DRVA_IGNORE: break;
            case DRVA_READ:
                memset(c_rd_req + first, 1, sizeof(c_rd_req[0])*length);
                SendChanRequest(devid, first, length, NULL,                     DRVA_READ);
                break;
            case DRVA_WRITE:
                memset(c_wr_req + first, 1, sizeof(c_wr_req[0])*length);
                memset32(c_wr_time + first, current_cycle, length);
                SendChanRequest(devid, first, length, values + (first - start), DRVA_WRITE);
                break;
        }
    }
}


static void ReqRofWrChs(int from_c, int until_c)
{
  int    x;
  int    first;
  int    rrowc;

    if (option_otokarevs) return;
    
    /* Inform ConsiderRequest() that we are REALLY reading write-channels */
    rrowc = IsReqRofWrChsIng;
    IsReqRofWrChsIng = 1;

    for (x = from_c;  x < until_c;  /* NO-OP */)
    {
        if (c_type[x] != 0)
        {
            first = x;

            while (x < until_c  &&  c_type[x] != 0)
                x++;

            MarkRangeForIO(first, x - first, DRVA_READ, NULL);
        }
        else x++;
    }

    IsReqRofWrChsIng = rrowc;
}

void RequestReadOfWriteChannels(void)
{
    ReqRofWrChs(0, numchans);
}


void HandleSimulatedHardware(void)
{
  int  x;
    
    if (!option_simulate) return;

    for (x = 0;  x < numchans;  x++)
    {
        if (c_rd_req[x] | c_wr_req[x])
        {
            if (c_type[x])
                c_value[x] = c_simnew[x];
            else
                c_value[x] = x * 1000 + current_cycle;

            c_time[x] = current_cycle;
            c_rd_req[x] = c_wr_req[x] = 0;
        }
    }
}

static void TryWrNext(chanaddr_t start, int count)
{
  int  g     = start;
  int  first;
  int  length;
    
    while (1)
    {
        /* Skip non-next_p channels */
        while (1)
        {
            if (count <= 0) return;
            if (c_next_wr_val_p[g]) break;
            g++;
            count--;
        }

        /* Find how many channels can be packed */
        first = g;
        while (count > 0  &&  c_next_wr_val_p[g])
        {
            g++;
            count--;
        }
        length = g - first;

        memset(c_wr_req + first, 1, sizeof(c_wr_req[0])*length);
        memcpy(c_wr_time + first, c_next_wr_val_time + first, sizeof(c_wr_time[0])*length);
        bzero (c_next_wr_val_p + first, sizeof(c_next_wr_val_p[0])*length);
        SendChanRequest(c_devid[first],
                        first, g - first, c_next_wr_val + first, DRVA_WRITE);
    }

}

void ReturnChanGroup(int devid, int  start, int count, int32 *values, rflags_t *rflags)
{
  int  blk_size;
  int  globalchan;

  //logline(LOGF_SYSTEM, 0, "%s(devid=%d/active=%d, start=%d, count=%d)", __FUNCTION__, devid, active_dev, start, count);
    CHECK_SANITY_OF_DEVID();

    blk_size = d_size[devid];

    if (count == RETURNCHAN_COUNT_PONG) return; // In server that's just a NOP

    /* Check the `start' */
    if (start < 0  ||  start >= blk_size)
    {
        logline(LOGF_SYSTEM, 0, "%s:(devid=%d/active=%d) start=%d, out_of[0..%d)",
                __FUNCTION__, devid, active_dev, start, blk_size);
///        raise(11); /*XXX!!!*/
        return;
    }

    /* Now check the `count' */
    if (count < 1  ||  count > blk_size - start)
    {
        logline(LOGF_SYSTEM, 0, "%s:(devid=%d/active=%d) count=%d, out_of[1..%d] (start=%d, blk_size=%d)",
                __FUNCTION__, devid, active_dev, count, blk_size - start, start, blk_size);
        return;
    }

    /* Calculate global chan # */
    globalchan = start + d_ofs[devid];

    /* Store the data */
    fast_memcpy(c_value + globalchan, values, sizeof(c_value[0]) * count);
    memset32(c_time + globalchan, current_cycle, count);

    /* And flags... */
    if (rflags != NULL)
    {
        fast_memcpy(c_rflags + globalchan, rflags, sizeof(c_rflags[0]) * count);
        {
          int    x;
          int32 *fp;
          int32 *cfp;
          
            for (x = 0, fp = c_rflags + globalchan, cfp = c_crflags + globalchan;
                 x < count;
                 x++,   fp++,                      cfp++)
                *cfp |= *fp;
        }
    }

    /* Use callbacks, if any are present... */
    {
      int            x;
      int            gcn;
      chan_cbitem_t *cbp;
      chan_cbitem_t *next_cbp;
        
        for (x = 0, gcn = globalchan + x;
             x < count;
             x++,   gcn++)
        {
            for (cbp  = c_cblist[gcn];
                 cbp != NULL  &&  cbp->magicnumber == CHANCB_MAGIC;
                 cbp  = next_cbp)
            {
                next_cbp = cbp->next;
                if (cbp->callback != NULL)
                    cbp->callback(gcn, cbp);
            }
        }
    }

    /* And clear "request sent" flags */
    bzero(c_rd_req + globalchan, sizeof(c_rd_req[0]) * count);
    bzero(c_wr_req + globalchan, sizeof(c_wr_req[0]) * count);

    TryWrNext(globalchan, count);
}

void ReturnChanSet  (int devid, int *addrs, int count, int32 *values, rflags_t *rflags)
{
  int  blk_size;
  int  blk_ofs;
  int  x;
  int  chan;
  int  globalchan;

    CHECK_SANITY_OF_DEVID();

    /* Cache dev parameters */
    blk_size = d_size[devid];
    blk_ofs  = d_ofs [devid];
    
    /* And check the `count' */
    if (count < 1  ||  count > blk_size*2)
    {
        logline(LOGF_SYSTEM, 0, "%s:(devid=%d/active=%d) count=%d, out_of[1..blk_size*2=%d] range",
                __FUNCTION__, devid, active_dev, count, blk_size*2);
        return;
    }

    /* Walk through the list */
    for (x = 0;  x < count;  x++)
    {
        /* Get the chan # */
        chan = addrs[x];

        /* Check it */
        if (chan < 0  ||  chan >= blk_size)
        {
            logline(LOGF_SYSTEM, 0, "%s:(devid=%d/active=%d) addrs[%d]=%d, out_of[0..%d)",
                    __FUNCTION__, devid, active_dev, x, chan, blk_size);
            return;
        }

        /* Calculate global chan # */
        globalchan = chan + blk_ofs;

        /* Store the data */
        c_value[globalchan] = values[x];
        c_time [globalchan] = current_cycle;

        /* And flags... */
        if (rflags != NULL)
        {
            c_rflags [globalchan]  = rflags[x];
            c_crflags[globalchan] |= rflags[x];
        }
        
        /* Use callbacks, if any are present... */
        {
          chan_cbitem_t *cbp;
          chan_cbitem_t *next_cbp;

            for (cbp  = c_cblist[globalchan];
                 cbp != NULL  &&  cbp->magicnumber == CHANCB_MAGIC;
                 cbp  = next_cbp)
            {
                next_cbp = cbp->next;
                if (cbp->callback != NULL)
                    cbp->callback(globalchan, cbp);
            }
        }

        /* And clear "request sent" flags */
        c_rd_req[globalchan] = c_wr_req[globalchan] = 0;

        if (c_next_wr_val_p[globalchan]) TryWrNext(globalchan, 1);
    }
}


static int ShouldReRequestChan(int chan)
{
    if      (c_wr_req[chan]) return DRVA_WRITE;
    else if (c_rd_req[chan]) return DRVA_READ;
    else                     return DRVA_IGNORE;
}

void ReRequestDevChans   (int devid)
{
  int  barrier = d_ofs[devid] + d_size[devid];
  int    g;
  int    first;
  int    length;
  int    f_act;
  
    for (g = d_ofs[devid];  g < barrier;  /*NO-OP*/)
    {
        /* Get "model" parameters */
        first = g;
        f_act = ShouldReRequestChan(g);
        g++;

        /* Find out how many channels can be packed */
        while (g < barrier  &&  ShouldReRequestChan(g) == f_act)
            g++;

        length = g - first;

        switch (f_act)
        {
            case DRVA_IGNORE: break;
            case DRVA_READ:
                SendChanRequest(devid, first, length, NULL,                  DRVA_READ);
                break;
            case DRVA_WRITE:
                SendChanRequest(devid, first, length, c_next_wr_val + first, DRVA_WRITE);
                break;
        }
    }
}

void RemitChanRequestsOf (int devid)
{
    bzero(&(c_rd_req[d_ofs[devid]]), d_size[devid] * sizeof(c_rd_req[0]));
    bzero(&(c_wr_req[d_ofs[devid]]), d_size[devid] * sizeof(c_wr_req[0]));
    bzero(&(c_next_wr_val_p[d_ofs[devid]]), d_size[devid] * sizeof(c_next_wr_val_p[0]));

    // "RstDevRflags(devid)"
    //bzero(&(c_rflags [d_ofs[devid]]), d_size[devid] * sizeof(c_rflags [0]));
    //bzero(&(c_crflags[d_ofs[devid]]), d_size[devid] * sizeof(c_crflags[0]));
}

void RequestReadOfWrChsOf(int devid)
{
    ReqRofWrChs(d_ofs[devid], d_ofs[devid] + d_size[devid]);
}


//////////////////////////////////////////////////////////////////////

static int AddToOnretList  (int chan,  chan_cbitem_t *item)
{
    item->prev = c_cblist_end[chan];
    item->next = NULL;

    if (c_cblist_end[chan] != NULL)
        c_cblist_end[chan]->next = item;
    else
        c_cblist[chan]           = item;
    c_cblist_end[chan] = item;

    return 0;
}

static int DelFromOnretList(int chan,  chan_cbitem_t *item)
{
  chan_cbitem_t *p1, *p2;

    p1 = item->prev;
    p2 = item->next;

    if (p1 == NULL) c_cblist[chan]     = p2; else p1->next = p2;
    if (p2 == NULL) c_cblist_end[chan] = p1; else p2->prev = p1;
  
    item->prev = item->next = NULL;
    
    return 0;
}

int AddChanOnretCallback(int chan, chan_cbitem_t *cbrec)
{
    /* Check if this 'item' is already used */
    if (cbrec->magicnumber != 0)
    {
        logline(LOGF_SYSTEM, 0, "%s(chan=%d): cbrec->magicnumber != 0",
                __FUNCTION__, chan);
        return -1;
    }

    /* Fill in the fields */
    cbrec->magicnumber = CHANCB_MAGIC;
    
    /* Physically add to the chain */
    AddToOnretList(chan, cbrec);

    return 0;
}

int DelChanOnretCallback(int chan, chan_cbitem_t *cbrec)
{
    /* Check if it is in the list... */
    if (cbrec->magicnumber != CHANCB_MAGIC) return 1;
    /* ...and if it is in OUR list */
    /*!!!*/

    /* Physicaly remove from the chain */
    DelFromOnretList(chan, cbrec);
    
    /* Clean the item */
    bzero(cbrec, sizeof(*cbrec));
    
    return 0;
}

int ClnChanOnRetCbList  (int chan)
{
  chan_cbitem_t *cbp;
  chan_cbitem_t *next_cbp;

    for (cbp  = c_cblist[chan];
         cbp != NULL  &&  cbp->magicnumber == CHANCB_MAGIC;
         cbp  = next_cbp)
    {
        next_cbp = cbp->next;
        if (cbp->on_stop != NULL)
            cbp->on_stop(chan, cbp);
        if (cbp->magicnumber == CHANCB_MAGIC)
            DelChanOnretCallback(chan, cbp);
    }

    return 0;
}
