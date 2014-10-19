#include "cxsd_includes.h"


enum {BIGC_MAGIC = 0x43676942};  // Little-endian 'BigC'

enum {BIGCCB_MAGIC = 0x42436342};  // Little-endian 'BcCB'


static int AddToReqList (int bigchan,  bigc_listitem_t *item)
{
  bigchaninfo_t   *bp = b_data + bigchan;
  
    item->prev = bp->listlast;
    item->next = NULL;

    if (bp->listlast != NULL)
        bp->listlast->next = item;
    else
        bp->list           = item;
    bp->listlast = item;

    return 0;
}

static int DelFromReqList(int bigchan,  bigc_listitem_t *item)
{
  bigchaninfo_t   *bp = b_data + bigchan;
  
  bigc_listitem_t *p1, *p2;

    p1 = item->prev;
    p2 = item->next;

    if (p1 == NULL) bp->list     = p2; else p1->next = p2;
    if (p2 == NULL) bp->listlast = p1; else p2->prev = p1;
  
    item->prev = item->next = NULL;
    
    return 0;
}


static void SendBigcForExecution(bigc_listitem_t *item)
{
  int              globalchan = item->bigchan;
  bigchaninfo_t   *bp         = b_data + globalchan;
  int              devid      = bp->devid;
  int              s_devid;

    /* Record that this request was sent */
    memcpy(GET_BIGBUF_PTR(bp->sent_args_o, int32), item->args, item->nargs * sizeof(*(item->args)));
                          bp->sent_nargs         = item->nargs;
                          bp->sent_datasize      = item->datasize;
                          bp->sent_dataunits     = item->dataunits;
           
    bp->req_sent = 1;
    bp->curitem  = item;
           
    /* And physically send it to driver */
    ENTER_DRIVER_S(devid, s_devid);
    if (option_simulate)
    {
        /*!!! Do something!!! */
        ReturnBigc(devid,
                   globalchan - d_big_first[devid],
                   item->args, item->nargs,
                   item->data, item->datasize, item->dataunits, 0);
    }
    else
    {
        if (d_dobig[devid] != NULL  &&  d_state[devid] == DEVSTATE_OPERATING)
            d_dobig[devid](devid, d_devptr[devid],
                           globalchan - d_big_first[devid],
                           item->args, item->nargs,
                           item->data, item->datasize, item->dataunits);
    }
    LEAVE_DRIVER_S(s_devid);

}

    
int AddBigcRequest(int bigchan, bigc_listitem_t *item,
                   bigc_callback_t callback, void *user_data,
                   int32 *args, int    nargs,
                   void  *data, size_t datasize, size_t dataunits,
                   int    cachectl)
{
  ////logline(LOGF_DBG1, 0, __FUNCTION__);
  
    /* Check if this 'item' is already used */
    if (item->magicnumber != 0)
    {
        logline(LOGF_SYSTEM, 0, "%s(bigchan=%d): item->magicnumber != 0",
                __FUNCTION__, bigchan);
        return -1;
    }

    /* "Count==0 implies ptr:=NULL" */
    if (nargs    == 0) args = NULL;
    if (datasize == 0) data = NULL;

    /* Fill in the 'item' fields with passed parameters */
    item->magicnumber = BIGC_MAGIC;
    
    item->bigchan   = bigchan;
    item->callback  = callback;
    item->user_data = user_data;
    item->args      = args;
    item->nargs     = nargs;
    item->data      = data;
    item->datasize  = datasize;
    item->dataunits = dataunits;
    item->cachectl  = cachectl;
    
    /* Physically add to the chain */
    AddToReqList(bigchan, item);

    /* May we request execution right now? */
    if (!b_data[bigchan].req_sent  &&  
        (cachectl != CX_CACHECTL_SNIFF  &&  cachectl != CX_CACHECTL_FROMCACHE))
    {
        SendBigcForExecution(item);
    }
    
    return 0;
}


int DelBigcRequest(bigc_listitem_t *item)
{
  int  bigchan;
    
    /* Check if it is in the list */
    if (item->magicnumber != BIGC_MAGIC) return 1;

    bigchan = item->bigchan;

    /* Physicaly remove from the chain */
    DelFromReqList(bigchan, item);

    /* Was it the "current request owner"? */
    if (item == b_data[bigchan].curitem) b_data[bigchan].curitem = NULL;
    
    /* Clean the item */
    bzero(item, sizeof(*item));
    
    return 0;
}

#define CHECK_LOG_CORRECT(condition, correction, format, params...)   \
    if (condition)                                                    \
    {                                                                 \
        logline(LOGF_SYSTEM, 0, "%s" format, __FUNCTION__, ##params); \
        correction;                                                   \
    }

static inline int may_return_data(bigchaninfo_t *bp, bigc_listitem_t *item)
{
    return
        item == bp->curitem                      ||
        item->cachectl == CX_CACHECTL_SNIFF      ||
        item->cachectl == CX_CACHECTL_FROMCACHE  ||
        ((item->cachectl == CX_CACHECTL_SHARABLE)  &&
         bp->sent_datasize == 0  &&
         (item->nargs <= bp->sent_nargs  &&  (item->nargs == 0  ||  memcmp(item->args, GET_BIGBUF_PTR(bp->sent_args_o, int32), item->nargs * sizeof(*(item->args))) == 0))
        );
}

void ReturnBigc     (int devid, int  bigchan, int32 *info, int ninfo, void *retdata, size_t retdatasize, size_t retdataunits, rflags_t rflags)
{
  int              globalchan;
  bigchaninfo_t   *bp;

  bigc_cbitem_t   *cbp, *next_cbp;

  bigc_listitem_t *item, *nextitem;

  bigc_callback_t  callback;
  void            *user_data;

    ////logline(LOGF_DBG2, 0, "%s", __FUNCTION__);
  
    /* Check if `devid' is sane */
    CHECK_LOG_CORRECT(devid < 0  ||  devid >= numdevs,
                      return,
                      "(devid=%d/active=%d): devid out of [0..numdevs=%d)",
                      devid, active_dev, numdevs);
    CHECK_LOG_CORRECT(d_state[devid] == DEVSTATE_OFFLINE,
                      return,
                      "(devid=%d/active=%d): state=OFFLINE",
                      devid, active_dev);

    /* Check `bigchan' */
    CHECK_LOG_CORRECT(bigchan < 0  ||  bigchan > d_big_count[devid],
                      return,
                      "(devid=%d/active=%d): bigchan=%d, out_of[0..%d)",
                      devid, active_dev, bigchan, d_big_count[devid]);
    
    /* Calculate global bigchan # */
    globalchan = bigchan + d_big_first[devid];
    bp = b_data + globalchan;

    /* Check if parameters are sane */
    CHECK_LOG_CORRECT(ninfo > bp->ninfo,
                      ninfo = bp->ninfo,
                      "(devid=%d/active=%d, bigchan=%d): ninfo=%d, >%d",
                      devid, active_dev, bigchan, ninfo, bp->ninfo);
    CHECK_LOG_CORRECT(ninfo < 0,
                      ninfo = 0,
                      "(devid=%d/active=%d, bigchan=%d): ninfo=%d, <0",
                      devid, active_dev, bigchan, ninfo);
    CHECK_LOG_CORRECT(retdatasize > bp->retdatasize,
                      retdatasize = bp->retdatasize,
                      "(devid=%d/active=%d, bigchan=%d): retdatasize=%zu, >%zu",
                      devid, active_dev, bigchan, retdatasize, bp->retdatasize);
    /* retdataunits? */
    if (retdatasize == 0) retdataunits = 1;
    CHECK_LOG_CORRECT((retdataunits != 1  &&  retdataunits != 2  &&  retdataunits != 4)  ||
                      retdatasize % retdataunits != 0,
                      return,
                      "(devid=%d/active=%d, bigchan=%d): retdataunits=%zu, retdatasize=%zu, bp->rdu=%zu",
                      devid, active_dev, bigchan, retdataunits, retdatasize, bp->retdataunits);

    /* "Flip buffers" -- copy shadow buffer to the primary one... */
    if (bp->sent_nargs != 0)
        fast_memcpy(GET_BIGBUF_PTR(bp->cur_args_o, int32), GET_BIGBUF_PTR(bp->sent_args_o, int32), bp->sent_nargs * sizeof(int));
                                   bp->cur_nargs                        = bp->sent_nargs;
                                   bp->cur_datasize                     = bp->sent_datasize;
                                   bp->cur_dataunits                    = bp->sent_dataunits;
    
    /* Store data in the cache */
    if (ninfo != 0)
        fast_memcpy(GET_BIGBUF_PTR(bp->cur_info_o, void),    info,               ninfo * sizeof(*info));
                                   bp->cur_ninfo           = ninfo;
    if (retdatasize != 0)
        fast_memcpy(GET_BIGBUF_PTR(bp->cur_retdata_o, void), retdata,            retdatasize);
                                   bp->cur_retdatasize     = retdatasize;
                                   bp->cur_retdataunits    = retdataunits;
    
                                   bp->cur_rflags          = rflags;
                                   bp->crflags            |= rflags;
    
                                   bp->cur_time            = current_cycle;

    /* Use callbacks, if any are present... */
    for (cbp  = bp->cblist;
         cbp != NULL  &&  cbp->magicnumber == BIGCCB_MAGIC;
         cbp  = next_cbp)
    {
        next_cbp = cbp->next;
        if (cbp->callback != NULL)
            cbp->callback(globalchan, cbp);
    }
                               
    /* Okay, now we can serve these data to requester(s) */
    /*
       A BIG FAT NOTE: the following code supposes that the chain isn't
       changed at the same time by anybody else, so that it is "finite".
       But future frontend-plugins (such as CORBA) may use different
       approaches when interacting with clients, and may have their own
       buffers, so that immediately upon receiving data they'll call
       AddBigcRequest() with the same `item'.

       To solve this potential problem, a "double-buffering" approach
       can be used:

       - before the loop, the chain is "moved" into another
         "object" -- in fact, list and listlast pointers are copied,
         and NULLified afterwards;
       - if any other AddBigcRequest()s are done during this loop, they
         are added to a "new generation" of the chain, while the loop
         operates with "previous generation";
       - after all "before-loop-existing" requests are checked, everything
         remaining is added back to the list/listlast chain, at the
         beginning.

       Problems: this loop must do "DelBigcRequest" itself (or
       DelBigcRequest must call another function, which accepts explicit
       head/tail pointers as parameters).
     */
    for (item = bp->list;  item != NULL;  item = nextitem)
    {
        nextitem = item->next;

        if (may_return_data(bp, item))
        {
            /* Cache data from item */
            callback  = item->callback;
            user_data = item->user_data;

            /* Delete item from chain */
            DelBigcRequest(item);

            /* Call user */
            callback(globalchan, user_data);
        }
    }

    /* Drop the "request is being executed" flag */
    /*
       We do it here instead of at before the loop (which could allow
       to send a next request for driver to execute it in parallel
       with current result being dispatched) because if the driver manages
       to return data directly from do_big, that new data would overwrite
       current contents of bp->cur_*.
     */
    bp->req_sent = 0;

    /* Is there anything sendable in the chain? */
    for (item = bp->list;  item != NULL;  item = item->next)
        if (item->cachectl != CX_CACHECTL_SNIFF  &&
            item->cachectl != CX_CACHECTL_FROMCACHE)
            break;
    if (item != NULL)
        SendBigcForExecution(bp->list);
}


void ReRequestDevBigcs   (int devid)
{
  int              x;
  bigchaninfo_t   *bp;
    
    /* Internal call, so no checks for `devid' sanity */

    for (x = 0;  x < d_big_count[devid];  x++)
    {
        bp = b_data + d_big_first[devid] + x;
        if (bp->req_sent  &&  bp->curitem != NULL)
            SendBigcForExecution(bp->curitem);
    }
}

void RemitBigcRequestsOf (int devid)
{
}


//////////////////////////////////////////////////////////////////////

static int AddToOnretList  (int bigc,  bigc_cbitem_t *item)
{
  bigchaninfo_t   *bp = b_data + bigc;

    item->prev = bp->cblist_end;
    item->next = NULL;

    if (bp->cblist_end != NULL)
        bp->cblist_end->next = item;
    else
        bp->cblist           = item;
    bp->cblist_end = item;

    return 0;
}

static int DelFromOnretList(int bigc,  bigc_cbitem_t *item)
{
  bigchaninfo_t   *bp = b_data + bigc;
  bigc_cbitem_t   *p1, *p2;

    p1 = item->prev;
    p2 = item->next;

    if (p1 == NULL) bp->cblist     = p2; else p1->next = p2;
    if (p2 == NULL) bp->cblist_end = p1; else p2->prev = p1;
  
    item->prev = item->next = NULL;
    
    return 0;
}

int AddBigcOnretCallback(int bigc, bigc_cbitem_t *cbrec)
{
    /* Check if this 'item' is already used */
    if (cbrec->magicnumber != 0)
    {
        logline(LOGF_SYSTEM, 0, "%s(bigc=%d): cbrec->magicnumber != 0",
                __FUNCTION__, bigc);
        return -1;
    }

    /* Fill in the fields */
    cbrec->magicnumber = BIGCCB_MAGIC;
    
    /* Physically add to the chain */
    AddToOnretList(bigc, cbrec);

    return 0;
}

int DelBigcOnretCallback(int bigc, bigc_cbitem_t *cbrec)
{
    /* Check if it is in the list... */
    if (cbrec->magicnumber != BIGCCB_MAGIC) return 1;
    /* ...and if it is in OUR list */
    /*!!!*/

    /* Physicaly remove from the chain */
    DelFromOnretList(bigc, cbrec);
    
    /* Clean the item */
    bzero(cbrec, sizeof(*cbrec));
    
    return 0;
}

int ClnBigcOnRetCbList  (int bigc)
{
  bigc_cbitem_t *cbp;
  bigc_cbitem_t *next_cbp;

    for (cbp  = b_data[bigc].cblist;
         cbp != NULL  &&  cbp->magicnumber == BIGCCB_MAGIC;
         cbp  = next_cbp)
    {
        next_cbp = cbp->next;
        if (cbp->on_stop != NULL)
            cbp->on_stop(bigc, cbp);
        if (cbp->magicnumber == BIGCCB_MAGIC)
            DelBigcOnretCallback(bigc, cbp);
    }

    return 0;
}

int ClnBigcRequestChain (int bigc)
{
  bigc_listitem_t *item, *next_item;

    for (item  = b_data[bigc].list;
         item != NULL  &&  item->magicnumber == BIGC_MAGIC;
         item  = next_item)
    {
        if (item->on_stop != NULL)
            item->on_stop(bigc, item->user_data);
        next_item = item->next;
    }

    return 0;
}
