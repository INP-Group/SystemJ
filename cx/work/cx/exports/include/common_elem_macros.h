#ifndef __COMMON_ELEM_MACROS_H
#define __COMMON_ELEM_MACROS_H


#define SUBELEM_START(id, label, count, ncols)                     \
    {id, label, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, label,                                             \
            NULL, NULL,                                            \
            ELEM_MULTICOL, count, ncols,                           \
            (logchannet_t []){

#define SUBELEM_START_CRN(id, label, colns, rowns, count, ncols)   \
    {id, label, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, label,                                             \
            colns, rowns,                                          \
            ELEM_MULTICOL, count, ncols,                           \
            (logchannet_t []){

#define CANVAS_START(id, label, count)                             \
    {id, label, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, label,                                             \
            NULL, NULL,                                            \
            ELEM_CANVAS, count, 0,                                 \
            (logchannet_t []){

#define ROWCOL_START(id, label, count, ncols)                      \
    {id, label, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, label,                                             \
            NULL, NULL,                                            \
            ELEM_ROWCOL, count, ncols,                             \
            (logchannet_t []){

#define TABBER_START(id, label, count)                             \
    {id, label, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, label,                                             \
            NULL, NULL,                                            \
            ELEM_TABBER, count, 1,                                 \
            (logchannet_t []){

#define TABBER_START_RN(id, label, rowns, count)                   \
    {id, label, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, label,                                             \
            colns, NULL,                                           \
            ELEM_TABBER, count, 1,                                 \
            (logchannet_t []){

#define SUBELEM_END(opts, plcmnt)                                  \
            },                                                     \
            opts                                                   \
        },                                                         \
    .placement=plcmnt                                              \
    }

#define SUBWIN_START(id, title, count, ncols)                      \
    {id, title, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, title,                                             \
            NULL, NULL,                                            \
            ELEM_SUBWIN, count, ncols,                             \
            (logchannet_t []){

#define SUBWINV4_START(id, title, count)                           \
    {id, title, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, title,                                             \
            NULL, NULL,                                            \
            ELEM_SUBWINV4, count, 1,                               \
            (logchannet_t []){

#define SUBWIN_END(label, opts, plcmnt)                            \
            },                                                     \
            "nofold,label='"label"',"opts                          \
        },                                                         \
    .placement=plcmnt                                              \
    }

#define SUBMENU_START(id, label, count, ncols)                     \
    {id, label, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
        (void *)&(elemnet_t){                                      \
            id, label,                                             \
            NULL, NULL,                                            \
            ELEM_SUBMENU, count, ncols,                            \
            (logchannet_t []){

#define SUBMENU_END(label, opts, plcmnt)                           \
            },                                                     \
            "label="label","opts                                   \
        },                                                         \
    .placement=plcmnt                                              \
    }

#define GLOBELEM_START(id, label, count, ncols)                    \
    {                                                              \
        &(elemnet_t){                                              \
            id, label,                                             \
            NULL, NULL,                                            \
            ELEM_MULTICOL, count, ncols,                           \
            (logchannet_t []){

#define GLOBELEM_START_CRN(id, label, colns, rowns, count, ncols)  \
    {                                                              \
        &(elemnet_t){                                              \
            id, label,                                             \
            colns, rowns,                                          \
            ELEM_MULTICOL, count, ncols,                           \
            (logchannet_t []){

#define GLOBTABBER_START(id, count)                                \
    {                                                              \
        &(elemnet_t){                                              \
            id, NULL,                                              \
            NULL, NULL,                                            \
            ELEM_TABBER, count, 1,                                 \
            (logchannet_t []){

#define GLOBTABBER_START_RN(id, rowns, count)                      \
    {                                                              \
        &(elemnet_t){                                              \
            id, NULL,                                              \
            NULL, rowns,                                           \
            ELEM_TABBER, count, 1,                                 \
            (logchannet_t []){

#define GLOBELEM_END(opts, flags)                                  \
            },                                                     \
            opts                                                   \
        }, flags                                                   \
    }


#define EMPTY_CELL() {NULL, " ",  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP}

#define HLABEL_CELL(l)  {NULL, l,  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP}
#define RHLABEL_CELL(l) {NULL, l,  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP, .placement="horz=right"}
#define CHLABEL_CELL(l) {NULL, l,  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP, .placement="horz=center"}
#define VLABEL_CELL(l)  {NULL, l,   NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VLABEL, LOGK_NOP}

#define USRTXT_CELL(id, l) {id, l,  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_USERTEXT, LOGK_USRTXT}

#define SEP_L()      {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HSEP,   LOGK_NOP, .placement="horz=fill"}
#define VSEP_L()     {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP,   LOGK_NOP, .placement="vert=fill"}

#define BUTT_CELL(id, l, cn) {id, l, NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define SCENARIO_CELL(label, lookopts, scenario) \
    {NULL, label, NULL, NULL, lookopts",scenario="scenario, NULL, LOGT_READ, LOGD_SCENARIO, LOGK_NOP, LOGC_NORMAL}
    
#define RGSWCH_LINE(id, label, off_l, on_l, c_s, c_off, c_on) \
    {id,  label, NULL, NULL, "#!#L\v#T"off_l"\f\flit=red\v"on_l"\f\flit=green", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){CMD_GETP_I(c_s), CMD_RET},                               \
        (excmd_t[]){CMD_QRYVAL,                           CMD_SETP_I(c_on),  \
                    CMD_PUSH_I(1.0), CMD_QRYVAL, CMD_SUB, CMD_SETP_I(c_off), \
                    CMD_RET}                                                 \
    }
#define RGSWCH_LINE_RED(id, label, off_l, on_l, c_s, c_off, c_on) \
    {id,  label, NULL, NULL, "#!#L\v#T"off_l"\f\flit=red\v"on_l"\f\flit=green", NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){CMD_GETP_I(c_s), CMD_RETSEP, CMD_GETP_I(c_s), CMD_RET},  \
        (excmd_t[]){CMD_QRYVAL,                           CMD_SETP_I(c_on),  \
                    CMD_PUSH_I(1.0), CMD_QRYVAL, CMD_SUB, CMD_SETP_I(c_off), \
                    CMD_RET},                                                \
        .minyelw=0.5, .maxyelw=1.5                                           \
    }

#define LED_LINE(id, label, cn) \
    {id,  label, NULL, NULL, "shape=circle",            NULL, LOGT_READ, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define BLUELED_LINE(id, label, cn) \
    {id,  label, NULL, NULL, "shape=circle",            NULL, LOGT_READ, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define REDLED_LINE(id, label, cn) \
    {id,  label, NULL, NULL, "shape=circle",            NULL, LOGT_READ, LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define AMBERLED_LINE(id, label, cn) \
    {id,  label, NULL, NULL, "shape=circle",            NULL, LOGT_READ, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define RGLED_LINE(id, label, cn) \
    {id,  label, NULL, NULL, "offcol=red,shape=circle", NULL, LOGT_READ, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define RGSQR_LINE(id, label, cn) \
    {id,  label, NULL, NULL, "offcol=red,shape=default", NULL, LOGT_READ, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}


#endif /* __COMMON_ELEM_MACROS_H */
