#ifndef __VDEV_H
#define __VDEV_H


/*
    Note:
        "trgc" is a channel# in target device
        "sodc" is a channel# in ARRAY of target devices (i.e. sodc=trgc+o_p->offset)
        In case of a SINGLE target device sodc==trgc
 */

//// Part 1: observer ////////////////////////////////////////////////

struct _vdev_observer_t_struct;

typedef void (*vdev_stat_cb_t)(int devid, void *devptr,
                               struct _vdev_observer_t_struct *o_p,
                               void *privptr);

typedef void (*vdev_trgc_cb_t)(int devid, void *devptr,
                               struct _vdev_observer_t_struct *o_p,
                               int trgc,
                               void *privptr);

enum
{
    VDEV_IMPR = 1 << 0,  // Internally-IMPoRtant channel
    VDEV_PRIV = 1 << 1,  // Privately-used channel
    VDEV_TUBE = 1 << 2,  // Tube -- map our channel to target's one
    VDEV_RERQ = 1 << 3,  // Re-request channel after each receive (DANGEROUS!!! Might cause infinite call chain for target's driver-internal channels, which are returned immediately from its rw_p())
};

typedef struct
{
    int  mode;
    int  ourc;
} vdev_trgc_dsc_t;

typedef struct
{
    inserver_chanref_t  ref;
    int                 isrw;
    int32               val;
    rflags_t            flgs;
    int                 rcvd;  // Was the value received?
    struct _vdev_observer_t_struct *o_p;
} vdev_trgc_cur_t;

typedef struct _vdev_observer_t_struct
{
    int                m_count;

    char               targ_name[32];
    int                targ_numchans;
    vdev_trgc_dsc_t   *map;
    vdev_trgc_cur_t   *cur;
    int                targ_offset;   // Sum of previous observers' .targ_numchans

    vdev_stat_cb_t     stat_cb;
    vdev_trgc_cb_t     trgc_cb;
    void              *privptr;

    //
    inserver_devref_t  targ_ref;
    int                targ_state;
    //
    int                devid;
    void              *devptr;
    int                bind_hbt_period;
    sl_tid_t           bind_hbt_tid;
    int                is_suffering;
} vdev_observer_t;


int  vdev_observer_init(vdev_observer_t *obs,
                        int devid, void *devptr,
                        int bind_hbt_period);
int  vdev_observer_fini(vdev_observer_t *obs);

void vdev_observer_req_chan(vdev_observer_t *o_p, int trgc);
void vdev_observer_snd_chan(vdev_observer_t *o_p, int trgc, int32 val);

void vdev_observer_forget_all(vdev_observer_t *o_p);


//// Part 2: state machine & Co. /////////////////////////////////////

typedef int  (*vdev_sw_alwd_t)(void *devptr, int prev_state);
typedef void (*vdev_swch_to_t)(void *devptr, int prev_state);

// Note: there's no "ctx" parameter for sodc_cb, since each device should use a SINGLE vdev, so devptr provides enough info
typedef void (*vdev_sodc_cb_t)(int devid, void *devptr,
                               int sodc, int32 val);

typedef struct
{
    int  ourc;
    int  state;
    int  enabled_val;
    int  disabled_val;
} vdev_sr_chan_dsc_t;

typedef struct
{
    int             delay_us;   // to wait after switch (in us; 0=>no-delay-required)
    int             next_state; // state-# to automatically switch in work_hbt() after specified delay or when sw_alwd; -1=>this-state-is-stable
    vdev_sw_alwd_t  sw_alwd;
    vdev_swch_to_t  swch_to;
    int            *impr_chans; // -1-terminated list of channels, which may affect readiness to switch to this state
} vdev_state_dsc_t;

typedef struct
{
    int                 chan_state_n;
    vdev_observer_t    *obs;

    int                 state_unknown_val;
    int                 state_determine_val;
    vdev_state_dsc_t   *state_descr;
    int                 state_count;

    vdev_sr_chan_dsc_t *state_related_channels;
    int                *state_important_channels; /* if !=NULL, this table must be [state_count*sum(obs[].targ_numchans)] */
    CxsdDevRWProc       do_rw;
    vdev_sodc_cb_t      sodc_cb;

    //
    int                 devid;
    void               *devptr;
    int                 work_hbt_period;
    sl_tid_t            work_hbt_tid;

    int                 total_numchans;

    int                 cur_state;
    struct timeval      next_state_time;
} vdev_context_t;


int  vdev_init(vdev_context_t *ctx, vdev_observer_t *obs,
               int devid, void *devptr,
               int bind_hbt_period, int work_hbt_period);
int  vdev_fini(vdev_context_t *ctx);

void vdev_set_state(vdev_context_t *ctx, int nxt_state);


#endif /* __VDEV_H */
