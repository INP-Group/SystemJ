#include "cxsd_driver.h"

#include "drv_i/nfrolov_d16_drv_i.h"


enum
{
    A_A  = 0,
    A_B  = 4,
    
    A_RT = 0,
    A_RF = 1,
    A_RM = 2,
    A_RS = 3
};


typedef struct
{
    int  devid;
    int  N;
    int  F_IN_NS;
} nfrolov_d16_privrec_t;


static void nfrolov_d16_rw_p(int devid, void *devptr,
                             int first, int count, int32 *values, int action);

static void ReturnOne(nfrolov_d16_privrec_t *me, int nc)
{
    nfrolov_d16_rw_p(me->devid, me, nc, 1, NULL, DRVA_READ);
}

static void ReturnVs(nfrolov_d16_privrec_t *me)
{
    nfrolov_d16_rw_p(me->devid, me,
                     NFROLOV_D16_CHAN_V_BASE, NFROLOV_D16_NUMOUTPUTS,
                     NULL, DRVA_READ);
}


static int nfrolov_d16_init_d(int devid, void *devptr,
                              int businfocount, int *businfo,
                              const char *auxinfo)
{
  nfrolov_d16_privrec_t *me = (nfrolov_d16_privrec_t*)devptr;
  
    me->devid   = devid;
    me->N       = businfo[0];
    me->F_IN_NS = 0;

    return DEVSTATE_OPERATING;
}

static void nfrolov_d16_rw_p(int devid, void *devptr,
                             int first, int count, int32 *values, int action)
{
  nfrolov_d16_privrec_t *me = (nfrolov_d16_privrec_t*)devptr;

  int                    n;  // channel N in values[] (loop ctl var)
  int                    x;  // channel indeX
  int                    l;  // Line #
  
  int                    c;
  int32                  value;
  rflags_t               rflags;

  int                    c_rf;
  int                    c_rs;
  int                    v_kclk;
  int                    v_fclk;
  int                    quant;
  int                    c_Ai;
  int                    c_Bi;
  int                    v_R;
  
    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) value = values[n];
        switch (x)
        {
            case NFROLOV_D16_CHAN_A_BASE ... NFROLOV_D16_CHAN_A_BASE   + NFROLOV_D16_NUMOUTPUTS - 1:
                l = x - NFROLOV_D16_CHAN_A_BASE;
                if (action == DRVA_WRITE)
                {
                    if (value < 0)      value = 0;
                    if (value > 0xFFFF) value = 0xFFFF;
                    c = value;
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_A+l, 16, &c));
                    ReturnOne(me, NFROLOV_D16_CHAN_V_BASE + l);
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_A+l, 0, &c));
                    value = c;
                }
                break;

            case NFROLOV_D16_CHAN_B_BASE ... NFROLOV_D16_CHAN_B_BASE   + NFROLOV_D16_NUMOUTPUTS - 1:
                l = x - NFROLOV_D16_CHAN_B_BASE;
                if (action == DRVA_WRITE)
                {
                    if (value < 0)    value = 0;
                    if (value > 0xFF) value = 0xFF;
                    c = value;
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_B+l, 16, &c));
                    ReturnOne(me, NFROLOV_D16_CHAN_V_BASE + l);
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_B+l, 0, &c));
                    value = c;
                }
                break;

            case NFROLOV_D16_CHAN_V_BASE ... NFROLOV_D16_CHAN_V_BASE   + NFROLOV_D16_NUMOUTPUTS - 1:
                l = x - NFROLOV_D16_CHAN_V_BASE;

                DO_NAF(CAMAC_REF, me->N, A_RF, 1, &c_rf);
                DO_NAF(CAMAC_REF, me->N, A_RS, 1, &c_rs);
                v_kclk = 1 << (c_rf & 3); // {0,1,2,3}=>{1,2,4,8}
                v_fclk = ((c_rs & 3) == 0)? me->F_IN_NS : 40; // 0(Fin)=>???, else=>40ns
                quant = v_fclk * v_kclk;

                rflags = 0;
                if (action == DRVA_WRITE  &&  quant != 0)
                {
                    if (value < 0) value = 0;
                    
                    c_Ai = value / quant;        if (c_Ai > 65535) c_Ai = 65535;
                    v_R  = value % quant;
                    c_Bi = v_R * 4; /* =*0.25 */ if (c_Bi > 255)   c_Bi = 255;

                    rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_A+l, 16, &c_Ai));
                    rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_B+l, 16, &c_Bi));
                    
                    ReturnOne(me, NFROLOV_D16_CHAN_A_BASE + l);
                    ReturnOne(me, NFROLOV_D16_CHAN_B_BASE + l);
                }
                // Perform "read" anyway
                rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_A+l, 0, &c_Ai));
                rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_B+l, 0, &c_Bi));
                if (quant != 0)
                {
                    value = c_Ai * quant + c_Bi / 4;
                }
                else
                {
                    rflags |= CXRF_OVERLOAD;
                    value   = -999999;
                }
                break;

            case NFROLOV_D16_CHAN_OFF_BASE ... NFROLOV_D16_CHAN_OFF_BASE + NFROLOV_D16_NUMOUTPUTS - 1:
            case NFROLOV_D16_CHAN_ALLOFF: // "ALL" immediately follows individual ones in both channels and bits
                l = x - NFROLOV_D16_CHAN_OFF_BASE;
                if (action == DRVA_WRITE)
                {
                    value = (value != 0);
                    rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RM, 1,  &c));
                    c = (c &~ (1 << l)) | (value << l);
                    rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_RM, 17, &c));
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RM, 1, &c));
                    value  = ((c & (1 << l)) != 0);
                }
                break;

            case NFROLOV_D16_CHAN_DO_SHOT:
                if (action == DRVA_WRITE  &&  value != 0)
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, 0, 10, &c));
                else
                    rflags = 0;
                value = 0;
                break;

            case NFROLOV_D16_CHAN_KSTART:
                if (action == DRVA_WRITE)
                {
                    if (value < 0)   value = 0;
                    if (value > 255) value = 255;
                    c = value;
                    rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RT, 17, &c));
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RT, 1, &c));
                    value  = c;
                }
                break;

            case NFROLOV_D16_CHAN_KCLK_N:
                if (action == DRVA_WRITE)
                {
                    if (value < 0) value = 0;
                    if (value > 3) value = 3;
                    c = value;
                    rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RF, 17, &c));
                    ReturnVs(me);
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RF, 1, &c));
                    value = c;
                }
                break;

            case NFROLOV_D16_CHAN_FCLK_SEL:
                if (action == DRVA_WRITE)
                {
                    if (value < 0) value = 0;
                    if (value > 2) value = 2;
                    rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 1,  &c));
                    c = (c &~ (3 << 0)) | (value << 0);
                    rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 17, &c));
                    ReturnVs(me);
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 1, &c));
                    c = (c >> 0) & 3;
                    if (c == 3) c = 2; // Since '11' means the same as '10'
                    value = c;
                }
                break;

            case NFROLOV_D16_CHAN_START_SEL:
                if (action == DRVA_WRITE)
                {
                    if (value < 0) value = 0;
                    if (value > 2) value = 2;
                    rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 1,  &c));
                    c = (c &~ (3 << 2)) | (value << 2);
                    rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 17, &c));
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 1, &c));
                    c = (c >> 2) & 3;
                    if (c == 3) c = 2; // Since '11' means the same as '10'
                    value = c;
                }
                break;

            case NFROLOV_D16_CHAN_MODE:
                if (action == DRVA_WRITE)
                {
                    value = (value != 0);
                    rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 1,  &c));
                    c = (c &~ (1 << 4)) | (value << 4);
                    rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 17, &c));
                }
                else
                {
                    rflags = x2rflags(DO_NAF(CAMAC_REF, me->N, A_RS, 1, &c));
                    value  = ((c & (1 << 4)) != 0);
                }
                break;

            case NFROLOV_D16_CHAN_F_IN_NS:
                if (action == DRVA_WRITE)
                {
                    if (value < 0)          value = 0;
                    if (value > 2000000000) value = 2000000000; // 2e9ns=2s
                    me->F_IN_NS = value;
                    ReturnVs(me);
                }
                value  = me->F_IN_NS;
                rflags = 0;
                break;
                
            case NFROLOV_D16_CHAN_WAITING:
                value  = 0;
                rflags = CXRF_UNSUPPORTED;
                break;
                
            default:
                value  = 0;
                rflags = CXRF_UNSUPPORTED;
        }
        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}


DEFINE_DRIVER(nfrolov_d16, "n-Frolov D16",
              NULL, NULL,
              sizeof(nfrolov_d16_privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              nfrolov_d16_init_d, NULL, nfrolov_d16_rw_p, NULL);
