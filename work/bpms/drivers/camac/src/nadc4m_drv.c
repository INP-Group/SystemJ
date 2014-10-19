#include "cxsd_driver.h"

#include "drv_i/nadc4m_drv_i.h"
#include "drv_i/delayunit_drv_i.h"
#include "drv_i/psctrlunit_drv_i.h"

#include "delayunit_io.h"
#include "psctrlunit_io.h"


enum
{
    FAC_TYPE_UNKNOWN = 0,
    FAC_TYPE_V2K     = 1,
    FAC_TYPE_VEPP5   = 2,
};

#define NADC4_EXT_PRIVREC          \
    int                N_DEL;      \
    int                C_DEL;      \
    int                N_ATN;      \
    int                C_ATN;      \
                                   \
    rflags_t           del_rflags; \
    rflags_t           atn_rflags; \
    int                ftype;      \
    delayunit_dscr_t   dd;         \
    psctrlunit_dscr_t  pd;


#define NADC4_EXT_PARAMS                                            \
    PSP_P_INT ("n_del", nadc4_privrec_t, N_DEL, -1, 1, 23),         \
    PSP_P_INT ("c_del", nadc4_privrec_t, C_DEL, -1, 0, DELAYUNIT_NUMCHANS-1), \
    PSP_P_INT ("n_atn", nadc4_privrec_t, N_ATN, -1, 1, 23),         \
    PSP_P_INT ("c_atn", nadc4_privrec_t, C_ATN, -1, 0, PSCTRLUNIT_NUM_GAIN_CHANS-1), \
    PSP_P_FLAG("v2k",   nadc4_privrec_t, ftype, FAC_TYPE_V2K,   0), \
    PSP_P_FLAG("vepp5", nadc4_privrec_t, ftype, FAC_TYPE_VEPP5, 0),


#define NADC4_EXT_VALIDATE_PARAM                    \
    else if (n == NADC4_PARAM_DELAY)  { return v; } \
    else if (n == NADC4_PARAM_ATTEN)  { return v; } \
    else if (n == NADC4_PARAM_DECMTN) { return 1; }


#define NADC4_EXT_INIT_PARAMS                                       \
    pdr->nxt_args[NADC4_PARAM_DELAY]  = 0;                          \
    pdr->nxt_args[NADC4_PARAM_ATTEN]  = 7;                          \
    pdr->nxt_args[NADC4_PARAM_DECMTN] = 1;                          \
    if      (PDR2ME(pdr)->ftype == FAC_TYPE_V2K)                    \
    {                                                               \
        PDR2ME(pdr)->dd = v2k_dd;                                   \
        PDR2ME(pdr)->pd = v2k_pd;                                   \
    }                                                               \
    else if (PDR2ME(pdr)->ftype == FAC_TYPE_VEPP5)                  \
    {                                                               \
        PDR2ME(pdr)->dd = vepp5_dd;                                 \
        PDR2ME(pdr)->pd = vepp5_pd;                                 \
    }                                                               \
    else                                                            \
    {                                                               \
        DoDriverLog(PDR2ME(pdr)->devid, DRIVERLOG_CRIT,             \
                    "type specivication (v2k/vepp5) is mandatory"); \
        return -CXRF_CFG_PROBL;                                     \
    }


static int WriteCurAuxilaryParameters(void *ptr);

#define NADC4_EXT_START \
    WriteCurAuxilaryParameters(me);


#include "nadc4_drv.c"


static int WriteCurAuxilaryParameters(void *ptr)
{
  nadc4_privrec_t *me = ptr;
  int32    v      = 0;
  int32    code   = 0;
  rflags_t rflags = 0;
  int      action = 0 * DRVA_READ + 1 * DRVA_WRITE;

    if (me->N_DEL > 0  &&  me->C_DEL >= 0)
    {
        v = me->pz.cur_args[NADC4_PARAM_DELAY];
        delayunit_do_io(&(me->dd),
                        CAMAC_REF,
                        me->N_DEL, me->C_DEL, action,
                        &v, &rflags,
                        &code);
        // DoDriverLog(devid, 0, "%s: (%s) %s: channel #%d, delay = %d, code = 0x%04x, rflags = 0x%04x",
        //             __FUNCTION__, DELAY_ATTEN_TYPE_ACTIVATED,
        //             action == DRVA_READ ? " READ" : "WRITE", me->C_DEL, v, code, rflags);
        me->pz.cur_args[NADC4_PARAM_DELAY] = v;
        me->del_rflags = rflags;
    }
    else
    {
        DoDriverLog(me->devid, 0, "%s: WARNING: Wrong values of n_del (= %d) or c_del (= %d)",
                    __FUNCTION__, me->N_DEL, me->C_DEL);
    }

    if (me->N_ATN > 0  &&  me->C_ATN >= 0)
    {
        v = me->pz.cur_args[NADC4_PARAM_ATTEN];
        psctrlunit_do_io(&(me->pd),
                         CAMAC_REF,
                         me->N_ATN, me->C_ATN, action,
                         &v, &rflags,
                         &code);
        // DoDriverLog(devid, 0, "%s: (%s) %s: channel #%d, atten = %d, code = 0x%04x, rflags = 0x%04x",
        //             __FUNCTION__, DELAY_ATTEN_TYPE_ACTIVATED,
        //             action == DRVA_READ ? " READ" : "WRITE", me->C_ATN, v, v, rflags);
        me->pz.cur_args[NADC4_PARAM_ATTEN] = v;
	me->atn_rflags = rflags;
    }
    else
    {
        DoDriverLog(me->devid, 0, "%s: WARNING: Wrong values of n_atn (= %d) or c_atn (= %d)",
                    __FUNCTION__, me->N_ATN, me->C_ATN);
    }

    return 1;
}
