#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>

#include "misclib.h"

#include "cda.h"
#include "Xh.h"
#include "Xh_plot.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"
#include "ChlI.h"
#include "fqkn.h"

#include "fastadc_data.h" 
#include "fastadc_gui.h" 
#include "fastadc_cpanel_decor.h"
#include "fastadc_knobplugin.h"

#include "drv_i/adc200me_drv_i.h"
#include "adc200me_data.h"
#include "manyadcs_knobplugin.h"
#include "liuclient.h"

#include "pixmaps/btn_mini_save.xpm"


// _drv_i.h
typedef double MANYADCS_DATUM_T;
enum
{
    MANYADCS_NUM_LINES = 8,
    MANYADCS_DATAUNITS = sizeof(MANYADCS_DATUM_T),
};

//--------------------------------------------------------------------
// _data.c
enum
{
    MANYADCS_DTYPE = ENCODE_DTYPE(MANYADCS_DATAUNITS,
                                  CXDTYPE_REPR_FLOAT, 0)
};

static const char *manyadcs_line_name_list[] =
                              {"1", "2", "3", "4", "5", "6", "7", "8"};

pzframe_type_dscr_t *manyadcs_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;
  
  fastadc_type_dscr_t        *adc200me_atd;

    if (!dscr_inited)
    {
        adc200me_atd = pzframe2fastadc_type_dscr(adc200me_get_type_dscr());
        FastadcDataFillStdDscr(&dscr, "manyadcs",
                               PZFRAME_B_ARTIFICIAL | PZFRAME_B_NO_SVD | PZFRAME_B_NO_ALLOC,
                               /* Bigc-related ADC characteristics */
                               MANYADCS_DTYPE,
                               0,
                               adc200me_atd->max_numpts, MANYADCS_NUM_LINES,
                               adc200me_atd->param_numpts,
                               NULL/*_info2mes*/,
                               /* Description of bigc-parameters */
                               NULL/*text2_params*/,
                               NULL/*_param_dscrs*/,
                               "", manyadcs_line_name_list, NULL,
                               (plotdata_range_t){.dbl_r={-100.0, +100.0}},
                               "% 8.3f", adc200me_atd->raw2pvl,
                               adc200me_atd->x2xs, adc200me_atd->xs
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}

//////////////////////////////////////////////////////////////////////
enum
{
    MANYADCS_NUM_RCKS    = 8,
    MANYADCS_MODS_IN_RCK = 6,
    MANYADCS_NUM_MODS    = MANYADCS_NUM_RCKS * MANYADCS_MODS_IN_RCK,
    MANYADCS_RCKS_IN_GRP = 4,
    MANYADCS_NUM_GRPS    = MANYADCS_NUM_RCKS / MANYADCS_RCKS_IN_GRP,

    MANYADCS_NUM_SVBS    = 100,
};

enum {UPDATE_PERIOD_MS = 200};

//////////////////////////////////////////////////////////////////////

enum
{
    LINEKIND_END = 0,
    LINEKIND_NOP = -1,

    LINEKIND_IU_MASK = 1 << 30,
    LINEKIND_I       = 0 << 30,
    LINEKIND_U       = 1 << 30,

    LINEKIND_KIND_MASK     = 255 << 8,
    LINEKIND_SUM_ALL_RACKS = 1 << 8,
    LINEKIND_SUM_ONE_MEV   = 2 << 8,
    LINEKIND_SUM_ONE_RACK  = 3 << 8,
    LINEKIND_ALL_RACKS     = 4 << 8,
    LINEKIND_ONE_MEV       = 5 << 8,
    LINEKIND_ONE_RACK      = 6 << 8,
    LINEKIND_ONE_MOD       = 7 << 8,

    LINEKIND_PARAM_MASK    = 255,
};

typedef struct
{
    const char *name;
    int         kind;
}linekind_descr_t;

static linekind_descr_t linekind_list[] =
{
    {"I sum ВСЕГО",     LINEKIND_I + LINEKIND_SUM_ALL_RACKS},

    {"I sum 1-го MeV",  LINEKIND_I + LINEKIND_SUM_ONE_MEV  + 0},
    {"I sum 2-го MeV",  LINEKIND_I + LINEKIND_SUM_ONE_MEV  + 1},

    {"I sum стойки 1",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 0},
    {"I sum стойки 2",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 1},
    {"I sum стойки 3",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 2},
    {"I sum стойки 4",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 3},
    {"I sum стойки 5",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 4},
    {"I sum стойки 6",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 5},
    {"I sum стойки 7",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 6},
    {"I sum стойки 8",  LINEKIND_I + LINEKIND_SUM_ONE_RACK + 7},

    {"I ВСЕ",           LINEKIND_I + LINEKIND_ALL_RACKS},

    {"I все 1-го MeV",  LINEKIND_I + LINEKIND_ONE_MEV      + 0},
    {"I все 2-го MeV",  LINEKIND_I + LINEKIND_ONE_MEV      + 1},

    {"I все стойки 1",  LINEKIND_I + LINEKIND_ONE_RACK     + 0},
    {"I все стойки 2",  LINEKIND_I + LINEKIND_ONE_RACK     + 1},
    {"I все стойки 3",  LINEKIND_I + LINEKIND_ONE_RACK     + 2},
    {"I все стойки 4",  LINEKIND_I + LINEKIND_ONE_RACK     + 3},
    {"I все стойки 5",  LINEKIND_I + LINEKIND_ONE_RACK     + 4},
    {"I все стойки 6",  LINEKIND_I + LINEKIND_ONE_RACK     + 5},
    {"I все стойки 7",  LINEKIND_I + LINEKIND_ONE_RACK     + 6},
    {"I все стойки 8",  LINEKIND_I + LINEKIND_ONE_RACK     + 7},

    {"I модулятора...", LINEKIND_I + LINEKIND_ONE_MOD},

    {"\n ",      LINEKIND_NOP},

    {"U sum ВСЕГО",     LINEKIND_U + LINEKIND_SUM_ALL_RACKS},

    {"U sum 1-го MeV",  LINEKIND_U + LINEKIND_SUM_ONE_MEV  + 0},
    {"U sum 2-го MeV",  LINEKIND_U + LINEKIND_SUM_ONE_MEV  + 1},

    {"U sum стойки 1",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 0},
    {"U sum стойки 2",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 1},
    {"U sum стойки 3",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 2},
    {"U sum стойки 4",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 3},
    {"U sum стойки 5",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 4},
    {"U sum стойки 6",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 5},
    {"U sum стойки 7",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 6},
    {"U sum стойки 8",  LINEKIND_U + LINEKIND_SUM_ONE_RACK + 7},

    {"U ВСЕ",           LINEKIND_U + LINEKIND_ALL_RACKS},

    {"U все 1-го MeV",  LINEKIND_U + LINEKIND_ONE_MEV      + 0},
    {"U все 2-го MeV",  LINEKIND_U + LINEKIND_ONE_MEV      + 1},

    {"U все стойки 1",  LINEKIND_U + LINEKIND_ONE_RACK     + 0},
    {"U все стойки 2",  LINEKIND_U + LINEKIND_ONE_RACK     + 1},
    {"U все стойки 3",  LINEKIND_U + LINEKIND_ONE_RACK     + 2},
    {"U все стойки 4",  LINEKIND_U + LINEKIND_ONE_RACK     + 3},
    {"U все стойки 5",  LINEKIND_U + LINEKIND_ONE_RACK     + 4},
    {"U все стойки 6",  LINEKIND_U + LINEKIND_ONE_RACK     + 5},
    {"U все стойки 7",  LINEKIND_U + LINEKIND_ONE_RACK     + 6},
    {"U все стойки 8",  LINEKIND_U + LINEKIND_ONE_RACK     + 7},

    {"U модулятора...", LINEKIND_U + LINEKIND_ONE_MOD},

    {NULL,              LINEKIND_END},
};

static void linekind2fc(int linekind, int *f_p, int *c_p)
{
  int  kind = (linekind & LINEKIND_KIND_MASK);
  int  prm  = (linekind & LINEKIND_PARAM_MASK);

    switch (kind)
    {
        case LINEKIND_ALL_RACKS:
        case LINEKIND_SUM_ALL_RACKS:
            *c_p = MANYADCS_NUM_MODS;
            break;

        case LINEKIND_ONE_MEV:
        case LINEKIND_SUM_ONE_MEV:
            *c_p = MANYADCS_MODS_IN_RCK * MANYADCS_RCKS_IN_GRP;
            break;

        case LINEKIND_ONE_RACK:
        case LINEKIND_SUM_ONE_RACK:
            *c_p = MANYADCS_MODS_IN_RCK;
            break;

        default:
            *c_p = 1;
    }

    *f_p = prm * *c_p;
}
//////////////////////////////////////////////////////////////////////


typedef struct
{
    struct _MANYADCS_privrec_t_struct *me;
    const char                        *name;
    fastadc_data_t                    *adc_p;
    int                                n;
} adc200me_ref_t;

typedef struct
{
    struct _MANYADCS_privrec_t_struct *me;
    int                                idx;
} adc200me_idx_t;

typedef struct
{
    void *p;
    int   nl;
} cbc_dpl_t;

typedef struct
{
    MANYADCS_DATUM_T *rck[MANYADCS_NUM_RCKS];
    MANYADCS_DATUM_T *grp[MANYADCS_NUM_GRPS];
    MANYADCS_DATUM_T *all;
} iu_bufs;

typedef struct
{
    const char                        *name;
    fastadc_data_t                    *adc_p;
    char                               type[20];
} savable_ref_t;

typedef struct _MANYADCS_privrec_t_struct
{
    fastadc_knobplugin_t  kpn;

    char             *sources;
    adc200me_ref_t    subord_refs     [MANYADCS_NUM_MODS];  // References, UNSORTED (sorted in the order of reference in .sources)
    int               subord_adcs_used;
    adc200me_idx_t    subord_adcs_idxs[MANYADCS_NUM_MODS];  // Indexes to references -- sorted by subord_refs[n].name
    adc200me_ref_t   *subord_ptrs     [MANYADCS_NUM_MODS];  // Pointers to references, sorted in parallel with subord_adcs_idxs[]

    char             *savables;
    savable_ref_t     savadc_refs     [MANYADCS_NUM_SVBS];
    int               savadc_refs_used;

    char             *ia_src;
    fastadc_gui_t    *ia_gui;
    fastadc_data_t   *ia_data;
    Knob              k_iavg;
    Knob              k_uavg;
    Knob              k_uprv;

    knobinfo_t       *ki;
    XhWindow          win;

    int               autosave;

    int               linekind[MANYADCS_NUM_LINES];
    Knob              k_kind  [MANYADCS_NUM_LINES];
    Knob              k_modn  [MANYADCS_NUM_LINES];
    cbc_dpl_t         line_dpl[MANYADCS_NUM_LINES];

    int               max_numpts;

    int               line_numpts[MANYADCS_NUM_LINES];

    int               mod_numpts[MANYADCS_NUM_MODS];
    int               rck_numpts[MANYADCS_NUM_RCKS];
    int               grp_numpts[MANYADCS_NUM_GRPS];
    int               all_numpts;

    iu_bufs           sum[2];
    
    struct
    {
        int           mod[MANYADCS_NUM_MODS];
        int           rck[MANYADCS_NUM_RCKS];
        int           grp[MANYADCS_NUM_GRPS];
        int           any;
        int           info;
    } chg;

    MANYADCS_DATUM_T  common_buf[0]; // Here starts a common buf, allocated by fastadc_knobplugin as part of privrec
} MANYADCS_privrec_t;

static inline MANYADCS_privrec_t *pzframe_gui2MANYADCS_privrec(pzframe_gui_t *gui)
{
    return
        (MANYADCS_privrec_t *)
        (
         (char*)(fastadc_gui2knobplugin(pzframe2fastadc_gui(gui))) -
         offsetof(MANYADCS_privrec_t, kpn)
        );
}

static psp_paramdescr_t text2MANYADCS_opts[] =
{
    PSP_P_MSTRING("sources",  MANYADCS_privrec_t, sources,  NULL, 1000),
    PSP_P_MSTRING("savables", MANYADCS_privrec_t, savables, NULL, 1000),
    PSP_P_MSTRING("ia_src",   MANYADCS_privrec_t, ia_src,   NULL, 1000),
    PSP_P_END()
};


static void UpdateUprv(MANYADCS_privrec_t *me);


static void manyadcs_subord_newdata_cb(pzframe_data_t *pfr,
                                       int   reason,
                                       int   info_changed,
                                       void *privptr)
{
  adc200me_ref_t     *rp = privptr;
  MANYADCS_privrec_t *me = rp->me;

    if (reason != PZFRAME_REASON_BIGC_DATA) return;

    me->chg.any   = 1;
    me->chg.info |= info_changed;

    me->chg.mod[rp->n]                        = 1;
    me->chg.rck[rp->n / MANYADCS_MODS_IN_RCK] = 1;
}

static void add_adc200me_vect(MANYADCS_DATUM_T *dstp,
                              ADC200ME_DATUM_T *srcp,
                              int               count,
                              double            coeff,
                              double            zero)
{
    for (;  count > 0;  --count) *dstp++ += (*srcp++) * coeff / 1000000. +
                                            zero;
}

static void add_manyadcs_vect(MANYADCS_DATUM_T *dstp,
                              MANYADCS_DATUM_T *srcp,
                              int               count)
{
    for (;  count > 0;  --count) *dstp++ += *srcp++;
}


static void RecalcSums(MANYADCS_privrec_t *me)
{
  int               iu;     // I=>0, U=>1
  int               gn;     // Group #
  int               rn;     // Rack # in group
  int               mn;     // Mod # in rack
  int               gg;     // Global group # (in fact, ===gn)
  int               rg;     // Global rack #
  int               mg;     // Global mod #

    /* 1st pass -- obtain statistics */
    for (gn = 0,  gg = 0,  me->all_numpts = 0;
         gn < MANYADCS_NUM_GRPS;
         gn++,    gg++)
    {
        for (rn = 0,  rg = gg * MANYADCS_RCKS_IN_GRP,  me->grp_numpts[gg] = 0;
             rn < MANYADCS_RCKS_IN_GRP;
             rn++,    rg++)
        {
            for (mn = 0,  mg = rg * MANYADCS_MODS_IN_RCK,  me->rck_numpts[rg] = 0;
                 mn < MANYADCS_MODS_IN_RCK;
                 mn++,    mg++)
            {
                if (me->chg.mod[mg])
                {
                    me->mod_numpts[mg] = me->subord_ptrs[mg]->adc_p->mes.inhrt->info[ADC200ME_PARAM_NUMPTS];
                    if (me->mod_numpts[mg] > me->max_numpts)
                        me->mod_numpts[mg] = me->max_numpts;
////printf("mod_numpts[%d]=%d\n", mg, me->mod_numpts[mg]);
                }

                me->chg.rck[rg] |= me->chg.mod[mg];
                if (me->rck_numpts[rg] < me->mod_numpts[mg])
                    me->rck_numpts[rg] = me->mod_numpts[mg];
            }

            me->chg.grp[gg] |= me->chg.rck[rg];
            if (me->grp_numpts[gg] < me->rck_numpts[rg])
                me->grp_numpts[gg] = me->rck_numpts[rg];
        }

        if (me->all_numpts < me->grp_numpts[gg])
            me->all_numpts = me->grp_numpts[gg];
    }

    /* 2nd pass -- re-sum those which components had changed */
    for (iu = 0;  iu < 2;  iu++)
    {
        bzero(me->sum[iu].all, sizeof(MANYADCS_DATUM_T) * me->all_numpts);
        for (gn = 0, gg = 0;
             gn < MANYADCS_NUM_GRPS;
             gn++,    gg++)
        {
            if (me->chg.grp[gg])
            {
                bzero(me->sum[iu].grp[gg], sizeof(MANYADCS_DATUM_T) * me->grp_numpts[gg]);
                for (rn = 0, rg = gg * MANYADCS_RCKS_IN_GRP;
                     rn < MANYADCS_RCKS_IN_GRP;
                     rn++,    rg++)
                {
                    if (me->chg.rck[rg])
                    {
                        bzero(me->sum[iu].rck[rg], sizeof(MANYADCS_DATUM_T) * me->rck_numpts[rg]);
                        for(mn = 0,  mg = rg * MANYADCS_MODS_IN_RCK;
                            mn < MANYADCS_MODS_IN_RCK;
                            mn++,    mg++)
                        {
                            add_adc200me_vect(me->sum[iu].rck[rg],
                                              me->subord_ptrs[mg]->adc_p->mes.plots[iu].x_buf,
                                              me->mod_numpts[mg],
                                              me->subord_ptrs[mg]->adc_p->dcnv.lines[iu].coeff,
                                              me->subord_ptrs[mg]->adc_p->dcnv.lines[iu].zero);
                        }
                    }
                    add_manyadcs_vect(me->sum[iu].grp[gg], me->sum[iu].rck[rg], me->rck_numpts[rg]);
                }
            }
            add_manyadcs_vect(me->sum[iu].all, me->sum[iu].grp[gg], me->grp_numpts[gg]);
        }
    }
}

             
static void FillLineParams(MANYADCS_privrec_t *me, int nl)
{
  fastadc_gui_t      *gui = &(me->kpn.g);
  fastadc_data_t     *adc = &(gui->a);
  plotdata_t         *dp  = gui->a.mes.plots + nl;

  int                 iu   = ((me->linekind[nl] & LINEKIND_IU_MASK) == LINEKIND_I? 0 : 1);
  int                 kind = (me->linekind[nl] & LINEKIND_KIND_MASK);
  int                 prm  = (me->linekind[nl] & LINEKIND_PARAM_MASK);
  int                 m_f;  // First
  int                 m_c;  // Count

  int                 minmax;
  int                 mg;
  fastadc_data_t     *subord_adc;

  char                d[2];

    linekind2fc(me->linekind[nl], &m_f, &m_c);
    //fprintf(stderr, "nl=%d f=%d c=%d\n", nl, m_f, m_c);

    /* Fill the properties first (what ProcessAdcInfo() does) */
    /*
       Note: we initialize with adc200me's params (except 'on'),
       switching to float only in sums
     */
    dp->on          = 1;
    dp->all_range   = dp->cur_range = adc->atd->range;
    dp->x_buf_dtype = adc->atd->ftd.dtype;

    memcpy(adc->dcnv.lines[nl].units,
           me->subord_ptrs[0]->adc_p->dcnv.lines[iu].units,
           sizeof(adc->dcnv.lines[nl].units));
//fprintf(stderr, "\t[%d] iu=%d units=<%s>\n",
//        nl, iu, adc->cfg.dcnv.units[nl]);

    switch (kind)
    {
        case LINEKIND_SUM_ALL_RACKS:
            dp->numpts      = me->all_numpts;
            dp->x_buf       = me->sum[iu].all;
            dp->x_buf_dtype = MANYADCS_DTYPE;
            break;

        case LINEKIND_SUM_ONE_MEV:
            dp->numpts      = me->grp_numpts [prm];
            dp->x_buf       = me->sum[iu].grp[prm];
            dp->x_buf_dtype = MANYADCS_DTYPE;
            break;

        case LINEKIND_SUM_ONE_RACK:
            dp->numpts      = me->rck_numpts [prm];
            dp->x_buf       = me->sum[iu].rck[prm];
            dp->x_buf_dtype = MANYADCS_DTYPE;
            break;

        default:
            dp->numpts      = me->mod_numpts [m_f];
            dp->on          = me->subord_ptrs[m_f]->adc_p->mes.plots[iu].on;
            dp->x_buf       = me->subord_ptrs[m_f]->adc_p->mes.plots[iu].x_buf;
            //if (kind == LINEKIND_ONE_MOD  &&  0)
            dp->x_buf_dtype = me->subord_ptrs[m_f]->adc_p->mes.plots[iu].x_buf_dtype;
            if (0  &&
                kind == LINEKIND_I + LINEKIND_ONE_MOD  &&
                iu   == 0  &&  m_f == 4)
                fprintf(stderr, "\t[%d] x_buf=%p dtype=%d,%d\n", nl, dp->x_buf, reprof_cxdtype(dp->x_buf_dtype), sizeof_cxdtype(dp->x_buf_dtype));
    }

    me->line_numpts[nl] = dp->numpts;

    if (reprof_cxdtype(dp->x_buf_dtype) == CXDTYPE_REPR_FLOAT)
    {
        for (minmax = 0;  minmax <= 1;  minmax++)
        {
            dp->cur_range.dbl_r[minmax] = 0;
            for (mg = m_f;  mg < m_f + m_c;  mg++)
            {
                subord_adc = me->subord_ptrs[mg]->adc_p;
                dp->cur_range.dbl_r[minmax] +=
                    FastadcDataPvl2Dsp(subord_adc, &(subord_adc->mes), iu,
                                       subord_adc->mes.plots[iu].cur_range.int_r[minmax] / 1000000.);
            }
            dp->all_range.dbl_r[minmax] = dp->cur_range.dbl_r[minmax];
        }
        adc->dcnv.lines[nl].coeff = 1;
        adc->dcnv.lines[nl].zero  = 0;
        //if (nl==0) fprintf(stderr, "\t[%.3f,%.3f]\n", dp->cur_range.dbl_r[0], dp->cur_range.dbl_r[1]);
    }
    else
    {
        dp->cur_range = me->subord_ptrs[m_f]->adc_p->mes.plots[iu].cur_range;
        adc->dcnv.lines[nl].coeff = me->subord_ptrs[m_f]->adc_p->dcnv.lines[iu].coeff;
        adc->dcnv.lines[nl].zero  = me->subord_ptrs[m_f]->adc_p->dcnv.lines[iu].zero;
    }

    d[0] = '1' + nl;  d[1] = '\0';
    XhPlotOneSetStrs(gui->plot, nl,
                     d,
                     adc->atd->dpyfmt,
                     adc->dcnv.lines[nl].units);
}

static void FindLargestNumpts(MANYADCS_privrec_t *me)
{
  fastadc_data_t *adc = &(me->kpn.g.a);

  int        nl;
  int        largest;

    for (nl = 0,  largest = 0;  nl < MANYADCS_NUM_LINES;  nl++)
        if (largest < me->line_numpts[nl])
            largest = me->line_numpts[nl];

    adc->mes.inhrt->info[adc->atd->param_numpts] = largest;
}

static void UpdateProc(XtPointer     closure,
                       XtIntervalId *id      __attribute__((unused)))
{
  MANYADCS_privrec_t *me  = closure;
  fastadc_gui_t      *gui = &(me->kpn.g);
  int                 nl;

    if (me->chg.any)
    {
        RecalcSums(me);
        if (me->chg.info)
        {
            for (nl = 0;  nl < MANYADCS_NUM_LINES;  nl++) FillLineParams(me, nl);
            FindLargestNumpts(me);
        }
        bzero(&(me->chg), sizeof(me->chg));

        FastadcGuiRenewPlot(gui,
                            gui->a.mes.inhrt->info[gui->a.atd->param_numpts],
                            me->chg.info);

        UpdateUprv(me);
    }

    XtAppAddTimeOut(xh_context, UPDATE_PERIOD_MS, UpdateProc, me);
}

static void SetLineKind(MANYADCS_privrec_t *me, int nl,
                        int linekind, int do_update)
{
  fastadc_gui_t *gui  = &(me->kpn.g);
  int            i;
  int            kind = linekind & LINEKIND_KIND_MASK;

    if (nl >= MANYADCS_NUM_LINES) return;

    /* Set 1st selector */
    for (i = 0;  linekind_list[i].name != NULL;  i++)
        if (linekind_list[i].kind ==
            (linekind &~ ((kind == LINEKIND_ONE_MOD)? LINEKIND_PARAM_MASK : 0)))
        {
            SetKnobValue(me->k_kind[nl], i);

            break;
        }

    /* Set 2nd selector */
    if (kind == LINEKIND_ONE_MOD)
    {
        SetKnobValue(me->k_modn[nl], linekind & LINEKIND_PARAM_MASK);
        XtSetSensitive(GetKnobWidget(me->k_modn[nl]), 1);
    }
    else
        XtSetSensitive(GetKnobWidget(me->k_modn[nl]), 0);

    me->linekind[nl] = linekind;
    FillLineParams   (me, nl);
    FindLargestNumpts(me);

    XhPlotCalcMargins(gui->plot);
    //FastadcGuiCalcMargins    (gui);
    //FastadcGuiSetMargins     (gui);
    if (do_update)
        FastadcGuiRenewPlot(gui,
                            gui->a.mes.inhrt->info[gui->a.atd->param_numpts],
                            1);
}

static void   manyadcs_draw  (void *privptr, int nl, int age,
                              int magn, GC gc)
{
  pzframe_gui_t      *g   = privptr;
  MANYADCS_privrec_t *me  = pzframe_gui2MANYADCS_privrec(g);
  fastadc_gui_t      *gui = pzframe2fastadc_gui(g);

  int                 iu   = ((me->linekind[nl] & LINEKIND_IU_MASK) == LINEKIND_I? 0 : 1);
  int                 kind = (me->linekind[nl] & LINEKIND_KIND_MASK);
  int                 prm  = (me->linekind[nl] & LINEKIND_PARAM_MASK);

  int                 m_f;  // First
  int                 m_c;  // Count
  int                 mg;

    if (me->chg.any) return; /* Don't try to draw if smth. had changed but recalc wasn't performed afterwards yet */

    if (kind == LINEKIND_SUM_ALL_RACKS  ||
        kind == LINEKIND_SUM_ONE_MEV    ||
        kind == LINEKIND_SUM_ONE_RACK)
    {
        XhPlotOneDraw(gui->plot,
                      gui->a.mes.plots + nl, magn, gc);
    }
    else
    {
        linekind2fc(me->linekind[nl], &m_f, &m_c);
        for (mg = m_f;  mg < m_f + m_c;  mg++)
        {
        //raise(11);
            XhPlotOneDraw(gui->plot, 
                          me->subord_ptrs[mg]->adc_p->mes.plots + iu,
                          magn, gc);
        }
    }
}

static void KindKCB(Knob k, double v, void *privptr)
{
  cbc_dpl_t          *pp       = privptr;
  MANYADCS_privrec_t *me       = pp->p;
  int                 nl       = pp->nl;
  int                 kv       = round(v);
  int                 nv       = round(me->k_modn[nl]->curv); /*!!! GetKnobValue() */
  int                 linekind = linekind_list[kv].kind;

    if ((linekind & LINEKIND_KIND_MASK) != LINEKIND_ONE_MOD) nv = 0;
    SetLineKind(me, nl, linekind + nv, 1);
}

static void ModnKCB(Knob k, double v, void *privptr)
{
  cbc_dpl_t          *pp       = privptr;
  MANYADCS_privrec_t *me       = pp->p;
  int                 nl       = pp->nl;
  int                 nv       = round(v);
  int                 kv       = round(me->k_kind[nl]->curv); /*!!! GetKnobValue() */
  int                 linekind = linekind_list[kv].kind;

    SetLineKind(me, nl, linekind + nv, 1);
}

static void SaveAllData(MANYADCS_privrec_t *me)
{
  time_t              timenow = time(NULL);
  struct tm          *tp;
  char                subdir  [PATH_MAX];
  char                filename[PATH_MAX];
  char                command [PATH_MAX];
  int                 r;
  int                 i;

  plotdata_t          plots[6];
  static const char  *names[6] = {"IsumA", "UsumA", "Isum1", "Usum1", "Isum2", "Usum2"};
  plotdata_t         *dp;
  int                 kind;
  int                 iu;
  int                 nl;

  FILE               *fp;
  int                 t;

    tp = localtime(&timenow);
    sprintf(subdir, "savedata_%04d%02d%02d_%02d-%02d-%02d",
            tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday,
            tp->tm_hour, tp->tm_min, tp->tm_sec);

    r = mkdir(subdir, 0777);
    if (r != 0)
    {
        XhMakeMessage(me->win, "Failed to mkdir(\"%s\")", subdir);
        return;
    }

    sprintf(filename,
            "%s/mode_%s_%04d%02d%02d.dat",
            subdir, ChlGetSysname(me->win),
            tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday);
    ChlSaveWindowMode(me->win, filename, "savedata");

    for (i = 0;  i < me->subord_adcs_used;  i++)
    {
        sprintf(filename,
                "%s/data_adc200me-%s.dat",
                subdir, me->subord_ptrs[i]->name);
        FastadcDataSave(me->subord_ptrs[i]->adc_p, filename,
                        me->subord_ptrs[i]->name, NULL);
    }

    for (i = 0;  i < me->savadc_refs_used;  i++)
    {
        sprintf(filename,
                "%s/data_%s-%s.dat",
                subdir, me->savadc_refs[i].type, me->savadc_refs[i].name);
        FastadcDataSave(me->savadc_refs[i].adc_p, filename,
                        me->savadc_refs[i].name, NULL);
    }

#if 0 /* Never-never-never -- that's just for tests of FastadcDataSave() (e.g., with different mes.plots[NL].numpts) */
    sprintf(filename, "%s/manyadcs.dat", subdir);
    FastadcDataSave(&(me->kpn.g.a), filename, "manyadcs", NULL);
#endif
#if 1
    for (kind = 0;  kind <= 2;  kind++)
        for (iu = 0;  iu <= 1;  iu++)
        {
            nl = kind * 2 + iu;
            dp = plots + nl;

            if (kind == 0)
            {
                dp->numpts  = me->all_numpts;
                dp->x_buf   = me->sum[iu].all;
            }
            else
            {
                dp->numpts  = me->grp_numpts [kind-1];
                dp->x_buf   = me->sum[iu].grp[kind-1];
            }
            dp->x_buf_dtype = MANYADCS_DTYPE;
        }

    sprintf(filename, "%s/data_sums.dat", subdir);
    fp = fopen(filename, "w");
    for (t = -1;  t < me->all_numpts;  t++)
    {
        if (t < 0) fprintf(fp, "%-6s %6s", "#N", "ns");
        else       fprintf(fp, "%6d %6d", t, t*5);

        for (nl = 0;  nl < countof(plots);  nl++)
            if      (t < 0)
                fprintf(fp, " %7s", names[nl]);
            else if (t < plots[nl].numpts)
                fprintf(fp, " % 7.3f", plotdata_get_dbl(plots + nl, t));
            else
                fprintf(fp, " % 7.0f", 0.0);

        fprintf(fp, "\n");
    }
    fclose(fp);
#endif

    sprintf(command, "tar cjf %s.tar.bz2 --remove-files %s; rmdir %s",
            subdir, subdir, subdir);
    system(command);

    XhMakeMessage(me->win, "All data saved to %s", subdir);
}

static void SaveBtnCB(Widget     w         __attribute__((unused)),
                      XtPointer  closure,
                      XtPointer  call_data __attribute__((unused)))
{
  MANYADCS_privrec_t *me = (MANYADCS_privrec_t *)closure;

    SaveAllData(me);
}

static void AutoSaveKCB(Knob k, double v, void *privptr)
{
  MANYADCS_privrec_t *me = (MANYADCS_privrec_t *)privptr;

    me->autosave = (int)v;
}


static void UpdateUprv(MANYADCS_privrec_t *me)
{
  fastadc_gui_t *my_gui = &(me->kpn.g);

  int            i_x1, i_x2;
  int            u_x1, u_x2;
  int            tmpx;
  double        *buf;
  double         sum;
  int            x;

  double  iavg = NAN;
  double  uavg = NAN;
  double  uprv = NAN;

  int            ia_iu = 0/*!!! 0 is Тр1I1, 1 is Шарик */;
  int            us_iu = 1/*!!! 1 is U */;
  int            us_gn = 0/*!!! 0 is 1st MeV */;

    if (me->ia_gui != NULL  &&  me->ia_data != NULL  &&
        me->ia_gui->rpr_on[0]  &&  me->ia_gui->rpr_on[1])
    {
        i_x1 = me->ia_gui->rpr_at[0];
        i_x2 = me->ia_gui->rpr_at[1];
        if (i_x1 > i_x2) {tmpx = i_x2; i_x2 = i_x1; i_x1 = tmpx;}

        if (i_x1 < me->ia_data->mes.plots[ia_iu].numpts  &&
            i_x2 < me->ia_data->mes.plots[ia_iu].numpts)
        {
            for (x  = i_x1, sum = 0;
                 x <= i_x2;
                 x++)
                 sum += FastadcDataGetDsp(me->ia_data,
                                          &(me->ia_data->mes),
                                          ia_iu, x);
            iavg = sum / (i_x2 - i_x1 + 1);
        }
    }

////fprintf(stderr, "grp_numpts[%d]=%d\n", us_gn, me->grp_numpts[us_gn]);
    if (my_gui->rpr_on[0]  &&  my_gui->rpr_on[1])
    {
        u_x1 = my_gui->rpr_at[0];
        u_x2 = my_gui->rpr_at[1];
        if (u_x1 > u_x2) {tmpx = u_x2; u_x2 = u_x1; u_x1 = tmpx;}

        if (u_x1 < me->grp_numpts[us_gn]  &&  u_x2 < me->grp_numpts[us_gn])
        {
            buf = me->sum[us_iu].grp[us_gn];
            for (x  = u_x1, sum = 0;
                 x <= u_x2;
                 x++)
                 sum += buf[x];
            uavg = sum / (u_x2 - u_x1 + 1);
        }
    }

    /* Calculate */
    uprv = fabs(iavg * 1000 /*kA->A*/)
           / 
           sqrt(pow(fabs(uavg * 1000 /*kV->V*/), 3))
           * 1e6;

    /* Display */
    SetKnobValue(me->k_iavg, iavg);
    SetKnobValue(me->k_uavg, uavg);
    SetKnobValue(me->k_uprv, uprv);
}

static void manyadcs_repers_cb(pzframe_data_t *pfr,
                               int   reason,
                               int   info_changed,
                               void *privptr)
{
  MANYADCS_privrec_t *me = privptr;

    if (reason != FASTADC_REASON_REPER_CHANGE) return;

    manyadcs_x1 = me->ia_gui->rpr_at[0];
    manyadcs_x2 = me->ia_gui->rpr_at[1];

    if (me->ia_gui->rpr_on[0] == 0  ||  me->ia_gui->rpr_on[1] == 0)
        manyadcs_x1 = manyadcs_x2 = -1;

    UpdateUprv(me);
}

static void manyadcs_iavg_newdata_cb(pzframe_data_t *pfr,
                                     int   reason,
                                     int   info_changed,
                                     void *privptr)
{
  MANYADCS_privrec_t *me = privptr;

    if (reason != PZFRAME_REASON_BIGC_DATA  &&
        reason != FASTADC_REASON_REPER_CHANGE) return;

    UpdateUprv(me);
}

static Widget manyadcs_mkctls(pzframe_gui_t           *gui,
                              fastadc_type_dscr_t     *atd,
                              Widget                   parent,
                              pzframe_gui_mkstdctl_t   mkstdctl,
                              pzframe_gui_mkparknob_t  mkparknob)
{
  MANYADCS_privrec_t *me = pzframe_gui2MANYADCS_privrec(gui);

  Widget  cform;    // Controls form
  Widget  pcgrid;   // Per-Channel-parameters grid
  Widget  icform;   // Intra-cell form

  Widget  w1;
  Widget  w2;

  XmString  s;

  int     i;
  char    kind_buf[1000];
  char    modn_buf[1000];
  char    spec[2000];
  char   *p;

  int     nl;
  int     nr;

  Widget  svb;
  Knob    asv;

    kind_buf[0] = '\0';
    for (i = 0;  linekind_list[i].name != NULL;  i++)
        sprintf(kind_buf + strlen(kind_buf), 
                "%s%s",
                i == 0? "" : "\v", linekind_list[i].name);
    for (p = kind_buf;  *p != '\0';  p++) if (*p == '\'') *p = '?';

    modn_buf[0] = '\0';
    for (i = 0;  i < me->subord_adcs_used;  i++)
        sprintf(modn_buf + strlen(modn_buf),
                "%s%s",
                i == 0? "" : "\v", me->subord_ptrs[i]->name);
    for (p = modn_buf;  *p != '\0';  p++) if (*p == '\'') *p = '?';

    /* 1. General layout */
    /* A container form */
    cform = XtVaCreateManagedWidget("form", xmFormWidgetClass, parent,
                                    XmNshadowThickness, 0,
                                    NULL);

    /* A grid for per-channel parameters */
    pcgrid = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (pcgrid, 0, 0);
    XhGridSetSpacing(pcgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(pcgrid, 0, 0);

    /* Per-channel parameters and repers */
    for (nl = 0;  nl < MANYADCS_NUM_LINES;  nl++)
    {
        MakeStdCtl(pcgrid, 0, nl + 1, gui, mkstdctl, FASTADC_GUI_CTL_LINE_LABEL, nl, 1);
        MakeStdCtl(pcgrid, 1, nl + 1, gui, mkstdctl, FASTADC_GUI_CTL_LINE_MAGN,  nl, 1);

        me->line_dpl[nl].p  = me;
        me->line_dpl[nl].nl = nl;

        snprintf(spec, sizeof(spec),
                 " look=selector label=''"
                 " widdepinfo='#T%s'",
                 kind_buf);
        me->k_kind[nl] = MakeAKnob(pcgrid, 2, nl + 1, spec, KindKCB, me->line_dpl + nl);

        snprintf(spec, sizeof(spec),
                 " look=selector label=''"
                 " widdepinfo='#-#T%s'",
                 modn_buf);
        me->k_modn[nl] = MakeAKnob(pcgrid, 3, nl + 1, spec, ModnKCB, me->line_dpl + nl);

        for (nr = 0;  nr < XH_PLOT_NUM_REPERS;  nr++)
        {
            if (nl == 0)
            {
                icform = MakeAForm(pcgrid, 4 + nr, 0);
                w1 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_ONOFF, 0, nr);
                w2 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_TIME,  0, nr);
                attachleft(w2, w1, NULL);
            }
            MakeStdCtl(pcgrid, 4 + nr, nl + 1, gui, mkstdctl, FASTADC_GUI_CTL_REPER_VALUE, nl, nr);
        }
    }

    /* Cmpr */
    MakeStdCtl(pcgrid, 3, 0, gui, mkstdctl, FASTADC_GUI_CTL_X_CMPR, 0, 0);

    SetLineKind(me, 0, LINEKIND_U + LINEKIND_ONE_MEV + 0,     0);
    SetLineKind(me, 1, LINEKIND_U + LINEKIND_ONE_MEV + 1,     0);
    SetLineKind(me, 2, LINEKIND_U + LINEKIND_SUM_ONE_MEV + 0, 0);
    SetLineKind(me, 3, LINEKIND_U + LINEKIND_SUM_ONE_MEV + 1, 0);
    SetLineKind(me, 4, LINEKIND_I + LINEKIND_ONE_MOD + 0,     0);
    SetLineKind(me, 5, LINEKIND_I + LINEKIND_ONE_MOD + 1,     0);
    SetLineKind(me, 6, LINEKIND_I + LINEKIND_ONE_MOD + 2,     0);
    SetLineKind(me, 7, LINEKIND_I + LINEKIND_ONE_MOD + 3,     0);

    svb =
        XtVaCreateManagedWidget("push_i", 
                                xmPushButtonWidgetClass,
                                cform,
                                XmNlabelString, s = XmStringCreateLtoR(">F", xh_charset),
                                XmNtraversalOn, False,
                                NULL);
    XmStringFree(s);
    XhAssignPixmap(svb, btn_mini_save_xpm);
    XhSetBalloon  (svb, "Сохранить все данные на диск");
    XtAddCallback (svb, XmNactivateCallback, SaveBtnCB, me);
    attachleft    (svb, pcgrid, CHL_INTERELEM_H_SPACING);

    asv = CreateSimpleKnob(" rw look=greenled label=Auto"
                           " comment='Сохранять автоматически после каждого выстрела'",
                           0, cform, AutoSaveKCB, me);
    SetKnobValue(asv, 0);
    attachleft(GetKnobWidget(asv), svb, CHL_INTERKNOB_H_SPACING);

    /* uPerveance */
    pcgrid = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (pcgrid, 0, 0);
    XhGridSetSpacing(pcgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(pcgrid, 0, 0);
    attachleft(pcgrid, GetKnobWidget(asv), CHL_INTERKNOB_H_SPACING + 30);

    MakeALabel(pcgrid, 0, 0, "Iavg@ТрI1");
    MakeALabel(pcgrid, 0, 1, "Uavg@1stMeV");
    MakeALabel(pcgrid, 0, 2, "МикроПервеанс");
    me->k_iavg = MakeAKnob(pcgrid, 1, 0,
                           "ro look=text dpyfmt=%7.2f units=kA",
                           NULL, NULL);
    me->k_uavg = MakeAKnob(pcgrid, 1, 1,
                           "ro look=text dpyfmt=%7.2f units=kV",
                           NULL, NULL);
    me->k_uprv = MakeAKnob(pcgrid, 1, 2,
                           "ro look=text dpyfmt=%7.2f units=''",
                           NULL, NULL);

    return cform;
}

pzframe_gui_dscr_t *manyadcs_get_gui_dscr(void)
{
  static fastadc_gui_dscr_t  dscr;
  static int                 dscr_inited;

    if (!dscr_inited)
    {
        FastadcGuiFillStdDscr(&dscr,
                              pzframe2fastadc_type_dscr(manyadcs_get_type_dscr()));

        dscr.plotdraw   = manyadcs_draw;
        dscr.def_plot_w = 700;
        dscr.min_plot_w = 100;
        dscr.max_plot_w = 3000;
        dscr.def_plot_h = 256;
        dscr.min_plot_h = 100;
        dscr.max_plot_h = 3000;

        dscr.cpanel_loc = XH_PLOT_CPANEL_AT_BOTTOM;
        dscr.mkctls     = manyadcs_mkctls;

        dscr_inited = 1;
    }
    return &(dscr.gkd);
}
//--------------------------------------------------------------------
typedef struct
{
    MANYADCS_privrec_t *me;
    char               *endptr;
} add_adc_privrec_t;

static int add_adc(const char *name, const char *p, void *privptr)
{
  add_adc_privrec_t  *rp = privptr;
  MANYADCS_privrec_t *me = rp->me;
  knobinfo_t         *sk = ChlFindKnob(me->win, name);
  adc200me_ref_t     *refp;

//fprintf(stderr, "!'%s'->%p\n", name, sk);

    if (sk == NULL)
    {
        fprintf(stderr, "%s(): adc200me \"%s\" not found\n",
                __FUNCTION__, name);
        return -1;
    }
    refp = me->subord_refs + me->subord_adcs_used;
    refp->adc_p = pzframe2fastadc_data
        (PzframeKnobpluginRegisterCB(sk, "adc200me",
                                     manyadcs_subord_newdata_cb,
                                     refp,
                                     &(refp->name)));
    me->subord_adcs_used++;

    rp->endptr = p;

    return 0;
}

typedef struct
{
    MANYADCS_privrec_t *me;
    char               *endptr;
    char                type[20];
} add_svb_privrec_t;

static int add_svb(const char *name, const char *p, void *privptr)
{
  add_svb_privrec_t  *rp = privptr;
  MANYADCS_privrec_t *me = rp->me;
  knobinfo_t         *sk = ChlFindKnob(me->win, name);
  savable_ref_t      *refp;

    if (sk == NULL)
    {
        fprintf(stderr, "%s(): %s ADC-knob \"%s\" not found\n",
                __FUNCTION__, rp->type, name);
        return -1;
    }

    refp = me->savadc_refs + me->savadc_refs_used;
    refp->adc_p = pzframe2fastadc_data
        (PzframeKnobpluginRegisterCB(sk, rp->type,
                                     NULL,
                                     NULL,
                                     NULL));
    if (refp->adc_p == NULL)
    {
        fprintf(stderr, "%s(): ADC-knob \"%s\" isn't of type \"%s\"\n",
                __FUNCTION__, name, rp->type);
        return -1;
    }
    refp->name = sk->label;
    strzcpy(refp->type, rp->type, sizeof(refp->type));
    fprintf(stderr, "%s(): +%s:\"%s\"\n", __FUNCTION__, rp->type, refp->name);
    me->savadc_refs_used++;

    rp->endptr = p;

    return 0;
}

static int adc200_idx_compare(const void *a, const void *b)
{
  adc200me_idx_t     *pa = a;
  adc200me_idx_t     *pb = b;
  MANYADCS_privrec_t *me = pa->me;

    return strcasecmp(me->subord_refs[pa->idx].name, me->subord_refs[pb->idx].name);
}
static int  manyadcs_kp_realize(pzframe_knobplugin_t *kpn,
                                Knob                  k)
{
  MANYADCS_privrec_t *me = (MANYADCS_privrec_t *)(k->widget_private);

  const char         *p;
  int                 r;
  add_adc_privrec_t   adr;

  add_svb_privrec_t   svr;
  const char         *col_p;
  int                 len;

  int                 n;

  int                 iu;
  int                 x;
  MANYADCS_DATUM_T   *bufptr;

  Knob                ia_k;

    //fprintf(stderr, "sources=<%s>\n", me->sources);

    /* I. Connect to all sources */
    p = me->sources;
    if (p == NULL)
    {
        return -1;
    }

    bzero(&adr, sizeof(adr));
    adr.me      = me;

    while (1)
    {
        while (*p != '\0'  &&  isspace(*p)) p++;
        if (*p == '\0') break;

        r = fqkn_expand(p, add_adc, &adr);
        if (r < 0) return -1;

        p = adr.endptr;
    }

    for (n = 0;  n < me->subord_adcs_used;  n++)
    {
        me->subord_adcs_idxs[n].me  = me->subord_refs[n].me = me;
        me->subord_adcs_idxs[n].idx = n;
    }

    qsort(me->subord_adcs_idxs, me->subord_adcs_used,
          sizeof(me->subord_adcs_idxs[0]),
          adc200_idx_compare);
    for (n = 0;  n < me->subord_adcs_used;  n++)
    {
        me->subord_refs[me->subord_adcs_idxs[n].idx].n = n;
        //fprintf(stderr, ">%s\n", me->subord_refs[me->subord_adcs_idxs[n].idx].name);
        me->subord_ptrs[n] = me->subord_refs + me->subord_adcs_idxs[n].idx;
    }
    //for (n = 0;  n < me->subord_adcs_used;  n++) fprintf(stderr, ".%s\n", me->subord_refs[n].name);

    /* Prepare buffer-pointers */
    for (iu = 0, bufptr = me->common_buf;  iu < 2;  iu++)
    {
        for (x = 0;  x < countof(me->sum[iu].rck);  x++)
        {
            me->sum[iu].rck[x] = bufptr;  bufptr += me->max_numpts;
        }
        for (x = 0;  x < countof(me->sum[iu].grp);  x++)
        {
            me->sum[iu].grp[x] = bufptr;  bufptr += me->max_numpts;
        }
        me->sum[iu].all = bufptr;  bufptr += me->max_numpts;
    }

    /* Connect to [optional] aux-savables */
    p = me->savables;

    bzero(&svr, sizeof(svr));
    svr.me      = me;

    while (p != NULL)
    {
        while (*p != '\0'  &&  isspace(*p)) p++;
        if (*p == '\0') break;

        col_p = strchr(p, ':');
        if (col_p == NULL)
        {
            fprintf(stderr, "%s(): \"TYPE:\" expected in savables list at \"%.40s\"\n",
                    __FUNCTION__, p);
            return -1;
        }
        len = col_p - p;
        if (len > sizeof(svr.type) - 1)
        {
            fprintf(stderr, "%s(): \"TYPE:\" too long in savables list at \"%.40s\"\n",
                    __FUNCTION__, p);
            return -1;
        }
        memcpy(svr.type, p, len); svr.type[len] = '\0';
        p = col_p + 1;

        r = fqkn_expand(p, add_svb, &svr);
        if (r < 0) return -1;

        p = svr.endptr;
    }

    /* Connect to microperveanse-source */
    ia_k = ChlFindKnob(me->win, me->ia_src);
    me->ia_gui  = pzframe2fastadc_gui
        (PzframeKnobpluginGetGui(ia_k, "adc200me"));
    me->ia_data = pzframe2fastadc_data
        (PzframeKnobpluginRegisterCB(ia_k, "adc200me",
                                     manyadcs_iavg_newdata_cb,
                                     me,
                                     NULL));
    fprintf(stderr, "\"%s\"->%p; gui=%p, data=%p\n",
            me->ia_src, ia_k, me->ia_gui, me->ia_data);

    return 0;
}

static MANYADCS_privrec_t *zzz_hack_me = NULL;
static XhWidget MANYADCS_Create_m(knobinfo_t *ki, XhWidget parent)
{
  MANYADCS_privrec_t         *me;
  XhWidget                    ret;
  pzframe_gui_dscr_t         *gkd    = manyadcs_get_gui_dscr();
  fastadc_gui_dscr_t         *my_gkd = pzframe2fastadc_gui_dscr(gkd);
  size_t                      bufsize;

    bufsize = my_gkd->atd->max_numpts * sizeof(MANYADCS_DATUM_T) *
              (
               1                 +
               MANYADCS_NUM_GRPS +
               MANYADCS_NUM_RCKS
              ) * 2 /* I,U */;

    me = ki->widget_private = XtMalloc(sizeof(*me) + bufsize);
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));

    me->ki         = ki;
    me->win        = XhWindowOf(parent);
    me->max_numpts = my_gkd->atd->max_numpts;

    ret = FastadcKnobpluginDoCreate(ki, parent,
                                    &(me->kpn),
                                    manyadcs_kp_realize,
                                    gkd,
                                    text2MANYADCS_opts, me);
    if (ret == NULL) return PzframeKnobpluginRedRectWidget(parent); /*!!! Never can happen */

    PzframeKnobpluginRegisterCB(ki, gkd->ftd->type_name,
                                manyadcs_repers_cb,
                                me,
                                NULL);

    XtAppAddTimeOut(xh_context, UPDATE_PERIOD_MS, UpdateProc, me);

    zzz_hack_me = me;
    
    return ret;
}


static knobs_vmt_t KNOB_MANYADCS_VMT  = {MANYADCS_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t manyadcs_widgetinfo[] =
{
    {0, "manyadcs", &KNOB_MANYADCS_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  manyadcs_widgetset = {manyadcs_widgetinfo, NULL};


void  RegisterMANYADCS_widgetset(int look_id)
{
    manyadcs_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&manyadcs_widgetset);
}

void  MANYADCS_SaveAll(void)
{
    if (zzz_hack_me != NULL  &&  zzz_hack_me->autosave)
        SaveAllData(zzz_hack_me);
}
