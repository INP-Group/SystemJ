#ifndef __CXSD_MODULE_H
#define __CXSD_MODULE_H


#include <sys/types.h>

#include "cx.h"


/*********************************************************************
*                                                                    *
*  Reg. channels interface.  Implementation lives in cxsd_channels.c *
*                                                                    *
*********************************************************************/

tag_t FreshFlag(int chan);

void MarkRangeForIO(chanaddr_t start, int count, int action, int32 *values);

void RequestReadOfWriteChannels(void);
void HandleSimulatedHardware   (void);
    
void ReRequestDevChans   (int devid);
void RemitChanRequestsOf (int devid);
void RequestReadOfWrChsOf(int devid);


/*********************************************************************
*                                                                    *
*  Big channels interface.  Implementation lives in cxsd_bigc.c      *
*                                                                    *
*********************************************************************/

struct _bigc_listitem_t;

typedef void (*bigc_callback_t)(int bigchan, void *closure);
typedef void (*bigc_on_stopd_t)(int bigchan, void *closure);

typedef struct _bigc_listitem_t {
    int32                    magicnumber;  // Magic number
    
    struct _bigc_listitem_t *next;         // Link to previous list item
    struct _bigc_listitem_t *prev;         // Link to next list item

    int                      bigchan;      // Bigc #
    
    bigc_callback_t          callback;     // Proc to call on data arrival
    void                    *user_data;    // Pointer to pass to that proc
    bigc_on_stopd_t          on_stop;
    
    int32                   *args;
    int                      nargs;
    void                    *data;
    size_t                   datasize;
    size_t                   dataunits;
    int                      cachectl;
} bigc_listitem_t;


int AddBigcRequest(int bigchan, bigc_listitem_t *item,
                   bigc_callback_t callback, void *user_data,
                   int32 *args, int    nargs,
                   void  *data, size_t datasize, size_t dataunits,
                   int    cachectl);
int DelBigcRequest(bigc_listitem_t *item);

void ReRequestDevBigcs   (int devid);
void RemitBigcRequestsOf (int devid);


/*********************************************************************
*                                                                    *
*  Devices/channels/bigcs callbacks interface.  Implementations live *
*  in cxsd_drvmgr.c, cxsd_channels.c, cxsd_bigc.c                    *
*                                                                    *
*********************************************************************/

struct _devs_cbitem_t;
struct _chan_cbitem_t;
struct _bigc_cbitem_t;

typedef void (*devs_onchg_callback_t)(int mgid, struct _devs_cbitem_t *cbrec,
                                      int newstate);
typedef void (*chan_onret_callback_t)(int chan, struct _chan_cbitem_t *cbrec);
typedef void (*bigc_onret_callback_t)(int bigc, struct _bigc_cbitem_t *cbrec);

typedef void (*chan_stopd_callback_t)(int chan, struct _chan_cbitem_t *cbrec);
typedef void (*bigc_stopd_callback_t)(int bigc, struct _bigc_cbitem_t *cbrec);

typedef struct _devs_cbitem_t
{
    int32                  magicnumber;

    struct _devs_cbitem_t *next;
    struct _devs_cbitem_t *prev;

    devs_onchg_callback_t  callback;
    void                  *privptr;
} devs_cbitem_t;

typedef struct _chan_cbitem_t
{
    int32                  magicnumber;
    
    struct _chan_cbitem_t *next;
    struct _chan_cbitem_t *prev;

    chan_onret_callback_t  callback;
    chan_stopd_callback_t  on_stop;
    void                  *privptr;
} chan_cbitem_t;

typedef struct _bigc_cbitem_t
{
    int32                  magicnumber;
    
    struct _bigc_cbitem_t *next;
    struct _bigc_cbitem_t *prev;

    bigc_onret_callback_t  callback;
    bigc_stopd_callback_t  on_stop;
    void                  *privptr;
} bigc_cbitem_t;

int AddDevsOnchgCallback(int devid, devs_cbitem_t *cbrec);
int DelDevsOnchgCallback(int devid, devs_cbitem_t *cbrec);

int AddChanOnretCallback(int chan, chan_cbitem_t *cbrec);
int DelChanOnretCallback(int chan, chan_cbitem_t *cbrec);
int ClnChanOnRetCbList  (int chan);

int AddBigcOnretCallback(int bigc, bigc_cbitem_t *cbrec);
int DelBigcOnretCallback(int bigc, bigc_cbitem_t *cbrec);
int ClnBigcOnRetCbList  (int bigc);
int ClnBigcRequestChain (int bigc);


#endif /* __CXSD_MODULE_H */
