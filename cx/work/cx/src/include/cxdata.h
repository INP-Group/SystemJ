#ifndef __CXDATA_H
#define __CXDATA_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "cx_version.h"
#include "misc_macros.h"


enum
{
    OP_code = 0x7F, OP_imm = 0x80,

                       OP_COMPILE_I = 'c'        | OP_imm,
    
    OP_ADD     = '+',  OP_ADD_I     = OP_ADD     | OP_imm,
    OP_SUB     = '-',  OP_SUB_I     = OP_SUB     | OP_imm,
    OP_MUL     = '*',  OP_MUL_I     = OP_MUL     | OP_imm,
    OP_DIV     = '/',  OP_DIV_I     = OP_DIV     | OP_imm,
    OP_MOD     = '%',  OP_MOD_I     = OP_MOD     | OP_imm,
    OP_NEG     = '=',

    OP_ABS     = '|',  OP_ABS_I     = OP_ABS     | OP_imm,
    OP_ROUND   = '1',  OP_ROUND_I   = OP_ROUND   | OP_imm,
    OP_TRUNC   = '0',  OP_TRUNC_I   = OP_TRUNC   | OP_imm,

    OP_CASE    = '?',  OP_CASE_I    = OP_CASE    | OP_imm,
    OP_TEST    = 'i',  OP_TEST_I    = OP_TEST    | OP_imm,
                       OP_CMP_I     = 'I'        | OP_imm,
    OP_ALLOW_W = 'w',
                       OP_GOTO_I    = 'g'        | OP_imm,
                       OP_LABEL_I   = 'l'        | OP_imm,
    OP_GETOTHEROP = 'o',

    OP_BOOLEANIZE = 'b', OP_BOOLEANIZE_I = OP_BOOLEANIZE | OP_imm,
    OP_BOOL_OR    = 'O',  OP_BOOL_OR_I   = OP_BOOL_OR    | OP_imm,
    OP_BOOL_AND   = '&',  OP_BOOL_AND_I  = OP_BOOL_AND   | OP_imm,

    OP_SQRT    = 'r',  OP_SQRT_I    = OP_SQRT    | OP_imm,
    OP_PW2     = '2',  OP_PW2_I     = OP_PW2     | OP_imm,
    OP_PWR     = '^',  OP_PWR_I     = OP_PWR     | OP_imm,
    OP_EXP     = 'e',  OP_EXP_I     = OP_EXP     | OP_imm,
    OP_LOG     = 'E',  OP_LOG_I     = OP_LOG     | OP_imm,

    OP_POLY    = 'n',  OP_POLY_I    = OP_POLY    | OP_imm,
    
                       OP_LAPPROX_I  = 'a'       | OP_imm,
                       OP_LAPPROX_BYFUNC_I = 'A' | OP_imm,

    OP_NOOP    = '(',  OP_PUSH_I    = OP_NOOP    | OP_imm,
    OP_POP     = ')',
    OP_DUP     = 'd',  OP_DUP_I     = OP_DUP     | OP_imm,

                       OP_GETP_I    = 'p'        | OP_imm,
                       OP_SETP_I    = 's'        | OP_imm,
                       OP_GETP_BYNAME_I = 'P'    | OP_imm,
                       OP_SETP_BYNAME_I = 'S'    | OP_imm,
                       OP_ADDSERVER_I   = 'R'    | OP_imm,
    OP_PRGLY   = 'y',

                       
    OP_QRYVAL  = '!',

    OP_GETTIME = 't',
    
    OP_SETREG  = '>',  OP_SETREG_I  = OP_SETREG  | OP_imm,
    OP_GETREG  = '<',  OP_GETREG_I  = OP_GETREG  | OP_imm,

                       OP_SAVEVAL_I = '{'        | OP_imm,
                       OP_LOADVAL_I = '}'        | OP_imm,

    OP_REFRESH = '#',  OP_REFRESH_I = OP_REFRESH | OP_imm,

    OP_CALCERR = '\a', OP_CALCERR_I = OP_CALCERR | OP_imm,
    OP_WEIRD   = '\\', OP_WEIRD_I   = OP_WEIRD   | OP_imm,
    OP_SVFLAGS = '['-64,
    OP_LDFLAGS = ']'-64,
    
                       OP_DEBUGP_I  = 'D'        | OP_imm,
                       
                       OP_DEBUGPV_I = 'V'        | OP_imm,

                       OP_LOGSTR_I  = 'v'        | OP_imm,

    OP_RET     = '.',  OP_RET_I     = OP_RET     | OP_imm,
    OP_RETSEP  = ',',  OP_RETSEP_I  = OP_RETSEP  | OP_imm,
    OP_BREAK   = ';',  OP_BREAK_I   = OP_BREAK   | OP_imm,

    OP_ARG_REG_NUM_MASK  = 0x0000FFFF,  /* Reg # */
    OP_ARG_REG_TYPE_MASK = 0x00FF0000,  /* Type: temp/local */
    OP_ARG_REG_TYPE_TMP  = 0 << 16,     /*  Temporary (scope is formula) */
    OP_ARG_REG_TYPE_LCL  = 1 << 16,     /*  Local (scope is app-defined) */
    OP_ARG_REG_COND_MASK = 0xFF000000,  /* Assignment condition */
    OP_ARG_REG_COND_ALWS = 0 << 24,     /*  Always */
    OP_ARG_REG_COND_INIT = 1 << 24,     /*  Initialize -- if isnan() */

    OP_ARG_CMP_COND_MASK = 0x0000FFFF,
    OP_ARG_CMP_FLGS_MASK = 0xFFFF0000,
    OP_ARG_CMP_LT        = 0100,    // <     Less-Than
    OP_ARG_CMP_LE        = 0110,    // <=    Less-of-Equal
    OP_ARG_CMP_EQ        = 0010,    // ==    EQual
    OP_ARG_CMP_GE        = 0011,    // >=    Greater-or-Equal
    OP_ARG_CMP_GT        = 0001,    // >     Greater-Than
    OP_ARG_CMP_NE        = 0101,    // !=,<> Not Equal
    OP_ARG_CMP_TR        = 0111,    //       TRue  } Just for
    OP_ARG_CMP_FL        = 0000,    //       FaLse } completeness
    OP_ARG_CMP_TEST      = 1 << 16, // Immediately-do-TEST flag
};

#define __CMD_ARG_MISSING__ {.number=0}

#define CMD_COMPILE_I(v) {OP_COMPILE_I, {.str=v}}

#define CMD_ADD          {OP_ADD,      __CMD_ARG_MISSING__}
#define CMD_ADD_I(v)     {OP_ADD_I,    {.number=v}}
#define CMD_SUB          {OP_SUB,      __CMD_ARG_MISSING__}
#define CMD_SUB_I(v)     {OP_SUB_I,    {.number=v}}
#define CMD_MUL          {OP_MUL,      __CMD_ARG_MISSING__}
#define CMD_MUL_I(v)     {OP_MUL_I,    {.number=v}}
#define CMD_DIV          {OP_DIV,      __CMD_ARG_MISSING__}
#define CMD_DIV_I(v)     {OP_DIV_I,    {.number=v}}
#define CMD_MOD          {OP_MOD,      __CMD_ARG_MISSING__}
#define CMD_MOD_I(v)     {OP_MOD_I,    {.number=v}}
#define CMD_NEG          {OP_NEG,      __CMD_ARG_MISSING__}

#define CMD_ABS          {OP_ABS,      __CMD_ARG_MISSING__}
#define CMD_ABS_I(v)     {OP_ABS,      {.number=v}}
#define CMD_ROUND        {OP_ROUND,    __CMD_ARG_MISSING__}
#define CMD_ROUND_I      {OP_ROUND,    {.number=v}}
#define CMD_TRUNC        {OP_TRUNC,    __CMD_ARG_MISSING__}
#define CMD_TRUNC_I      {OP_TRUNC,    {.number=v}}

#define CMD_CASE         {OP_CASE,     __CMD_ARG_MISSING__}
#define CMD_CASE_I(v)    {OP_CASE_I,   {.number=v}}
#define CMD_TEST         {OP_TEST,     __CMD_ARG_MISSING__}
#define CMD_TEST_I(v)    {OP_TEST_I,   {.number=v}}
#define CMD_IF_LT_TEST       {OP_CMP_I, {.number=OP_ARG_CMP_LT|OP_ARG_CMP_TEST}}
#define CMD_IF_LE_TEST       {OP_CMP_I, {.number=OP_ARG_CMP_LE|OP_ARG_CMP_TEST}}
#define CMD_IF_EQ_TEST       {OP_CMP_I, {.number=OP_ARG_CMP_EQ|OP_ARG_CMP_TEST}}
#define CMD_IF_GE_TEST       {OP_CMP_I, {.number=OP_ARG_CMP_GE|OP_ARG_CMP_TEST}}
#define CMD_IF_GT_TEST       {OP_CMP_I, {.number=OP_ARG_CMP_GT|OP_ARG_CMP_TEST}}
#define CMD_IF_NE_TEST       {OP_CMP_I, {.number=OP_ARG_CMP_NE|OP_ARG_CMP_TEST}}
#define CMD_ALLOW_W      {OP_ALLOW_W,  __CMD_ARG_MISSING__}
#define CMD_GOTO_I(v)    {OP_GOTO_I,   {.label=v}}
#define CMD_LABEL_I(v)   {OP_LABEL_I,  {.label=v}}
#define CMD_GETOTHEROP   {OP_GETOTHEROP, __CMD_ARG_MISSING__}

#define CMD_BOOLEANIZE      {OP_BOOLEANIZE,   __CMD_ARG_MISSING__}
#define CMD_BOOLEANIZE_I(v) {OP_BOOLEANIZE_I, {.number=v}}
#define CMD_BOOL_OR         {OP_BOOL_OR,      __CMD_ARG_MISSING__}
#define CMD_BOOL_OR_I(v)    {OP_BOOL_OR_I,    {.number=v}}
#define CMD_BOOL_AND        {OP_BOOL_AND,     __CMD_ARG_MISSING__}
#define CMD_BOOL_AND_I(v)   {OP_BOOL_AND_I    {.number=v}}

#define CMD_SQRT         {OP_SQRT,     __CMD_ARG_MISSING__}
#define CMD_SQRT_I(v)    {OP_SQRT_I,   {.number=v}}
#define CMD_PW2          {OP_PW2,      __CMD_ARG_MISSING__}
#define CMD_PW2_I(v)     {OP_PW2_I,    {.number=v}}
#define CMD_PWR          {OP_PWR,      __CMD_ARG_MISSING__}
#define CMD_PWR_I(v)     {OP_PWR_I,    {.number=v}}
#define CMD_EXP          {OP_EXP,      __CMD_ARG_MISSING__}
#define CMD_EXP_I(v)     {OP_EXP_I,    {.number=v}}
#define CMD_LOG          {OP_LOG,      __CMD_ARG_MISSING__}
#define CMD_LOG_I(v)     {OP_LOG_I,    {.number=v}}

#define CMD_POLY         {OP_POLY,     __CMD_ARG_MISSING__}
#define CMD_POLY_I(v)    {OP_POLY_I,   {.number=v}}

#define CMD_LAPPROX_I(v) {OP_LAPPROX_I, {.lapprox_rp=v}}
#define CMD_LAPPROX_BYFUNC_I(v, p, i) {OP_LAPPROX_BYFUNC_I, {.lapprox_srcinfo=&(excmd_lapproxnet_rec){v, &(excmd_lapprox_info_rec){p, i}}}}
#define CMD_LAPPROX_FROM_I(p,i) CMD_LAPPROX_BYFUNC_I(NULL, p, i)

#define CMD_NOOP         {OP_NOOP,     __CMD_ARG_MISSING__}
#define CMD_PUSH_I(v)    {OP_PUSH_I,   {.number=v}}
#define CMD_POP          {OP_POP,      __CMD_ARG_MISSING__}
#define CMD_DUP          {OP_DUP,      __CMD_ARG_MISSING__}
#define CMD_DUP_I(v)     {OP_DUP_I,    {.number=v}}

#define CMD_GETP_I(v)    {OP_GETP_I,   {.chan_id=v}}
#define CMD_SETP_I(v)    {OP_SETP_I,   {.chan_id=v}}
#define CMD_GETP_BYNAME_I(v) {OP_GETP_BYNAME_I, {.chanref=v}}
#define CMD_SETP_BYNAME_I(v) {OP_SETP_BYNAME_I, {.chanref=v}}
#define CMD_ADDSERVER_I(  v) {OP_ADDSERVER_I,   {.chanref=v}}
#define CMD_PRGLY        {OP_PRGLY,    __CMD_ARG_MISSING__}

#define CMD_QRYVAL       {OP_QRYVAL,   __CMD_ARG_MISSING__}

#define CMD_GETTIME      {OP_GETTIME,  __CMD_ARG_MISSING__}

#define CMD_SETREG         {OP_SETREG,   __CMD_ARG_MISSING__}
#define CMD_SETREG_I(v)    {OP_SETREG_I, {.number=v}}
#define CMD_SETLCLREG_I(v) {OP_SETREG_I, {.number=(v) | OP_ARG_REG_TYPE_LCL}}
#define CMD_SETLCLREGDEFVAL_I(v) {OP_SETREG_I, {.number=(v) | OP_ARG_REG_TYPE_LCL | OP_ARG_REG_COND_INIT}}
#define CMD_GETREG         {OP_GETREG,   __CMD_ARG_MISSING__}
#define CMD_GETREG_I(v)    {OP_GETREG_I, {.number=v}}
#define CMD_GETLCLREG_I(v) {OP_GETREG_I, {.number=(v) | OP_ARG_REG_TYPE_LCL}}

#define CMD_SAVEVAL_I(v)   {OP_SAVEVAL_I, {.str=v}}
#define CMD_LOADVAL_I(v)   {OP_LOADVAL_I, {.str=v}}

#define CMD_REFRESH      {OP_REFRESH,  __CMD_ARG_MISSING__}
#define CMD_REFRESH_I(v) {OP_REFRESH_I,  {.number=v}}

#define CMD_CALCERR      {OP_CALCERR,  __CMD_ARG_MISSING__}
#define CMD_CALCERR_I(v) {OP_CALCERR_I,  {.number=v}}

#define CMD_WEIRD        {OP_WEIRD,    __CMD_ARG_MISSING__}
#define CMD_WEIRD_I(v)   {OP_WEIRD_I,    {.number=v}}
#define CMD_SVFLAGS      {OP_SVFLAGS,  __CMD_ARG_MISSING__},
#define CMD_LDFLAGS      {OP_LDFLAGS,  __CMD_ARG_MISSING__},

#define CMD_DEBUGP_I(v)  {OP_DEBUGP_I,  {.str=v}}
#define CMD_DEBUGPV_I(v) {OP_DEBUGPV_I, {.str=v}}
#define CMD_LOGSTR_I(v)  {OP_LOGSTR_I,  {.str=v}}

#define CMD_RET          {OP_RET,      __CMD_ARG_MISSING__}
#define CMD_RET_I(v)     {OP_RET_I,      {.number=v}}
#define CMD_RETSEP       {OP_RETSEP,   __CMD_ARG_MISSING__}
#define CMD_RETSEP_I(v)  {OP_RETSEP_I,   {.number=v}}
#define CMD_BREAK        {OP_BREAK,    __CMD_ARG_MISSING__}
#define CMD_BREAK_I(v)   {OP_BREAK_I,    {.number=v}}

#define CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK            \
    CMD_PUSH_I(0.0), CMD_PUSH_I(0.0), CMD_PUSH_I(1.0), \
    CMD_QRYVAL, CMD_SUB_I(CX_VALUE_COMMAND), CMD_ABS,  \
    CMD_SUB_I(CX_VALUE_COMMAND/10.0),                  \
    CMD_CASE,                                          \
    CMD_TEST, CMD_BREAK_I(0.0)


#define PHYS_REC 1

#if PHYS_REC
#define PHYS_ID(  id)   {PHYS_KIND_BY_ID,   {.phys_id=id}}
#define PHYS_NAME(name) {PHYS_KIND_BY_NAME, {.phys_name=name}}
#else
#define PHYS_ID(  id)   (id)
#endif


enum {
    ELEM_SINGLECOL = 1,
    ELEM_MULTICOL  = 2,

    ELEM_CANVAS    = 10,
    ELEM_TABBER    = 11,
    ELEM_SUBWIN    = 12,
    ELEM_INVISIBLE = 13,
    ELEM_SUBMENU   = 14,
    ELEM_SUBWINV4  = 15,
    ELEM_ROWCOL    = 16,
    ELEM_OUTLINE   = 17,
};

enum {
    LOGT_READ    = 1000,
    LOGT_DEVN    = 1001,
    LOGT_MINMAX  = 1003,
    
    LOGT_WRITE1  = 2000,

    LOGT_SUBELEM = 3000,
};

enum {
    LOGD_TEXT     = 2000,
    LOGD_TEXTND   = 2001,
    LOGD_ALARM    = 2002,
    LOGD_ONOFF    = 2003,
    LOGD_GREENLED = 2004,
    LOGD_AMBERLED = 2005,
    LOGD_REDLED   = 2006,
    LOGD_BLUELED  = 2007,
    LOGD_DIAL     = 2008,
    LOGD_BUTTON   = 2009,
    LOGD_SELECTOR = 2010,
    LOGD_LIGHT    = 2011,
    LOGD_HSLIDER  = 2012,
    LOGD_VSLIDER  = 2013,
    LOGD_ARROW_LT = 2014,
    LOGD_ARROW_RT = 2015,
    LOGD_ARROW_UP = 2016,
    LOGD_ARROW_DN = 2017,
    LOGD_KNOB     = 2018,
    LOGD_GAUGE    = 2019,
    LOGD_HSEP     = 2020,
    LOGD_VSEP     = 2021,
    LOGD_HLABEL   = 2022,
    LOGD_VLABEL   = 2023,
    LOGD_RADIOBTN = 2024,
    LOGD_CHOICEBS = 2025,
    LOGD_USERTEXT = 2026,

    LOGD_RECTANGLE  = 3000,
    LOGD_FRECTANGLE = 3001,
    LOGD_ELLIPSE    = 3002,
    LOGD_FELLIPSE   = 3003,
    LOGD_POLYGON    = 3004,
    LOGD_FPOLYGON   = 3006,
    LOGD_POLYLINE   = 3007,
    LOGD_PIPE       = 3008,
    LOGD_STRING     = 3009,

    LOGD_SRV_LEDS   = 4000,

    LOGD_MODE_BTN   = 5000,
    LOGD_SCENARIO   = 5001,
    LOGD_EXEC_BTN   = 5002,

    LOGD_HISTPLOT   = 6000,
};

enum {
    LOGK_DIRECT = 1,
    LOGK_CALCED = 2,
    LOGK_NOP    = 3,
    LOGK_USRTXT = 4,
};

enum {
    LOGC_NORMAL    = 1,
    LOGC_HILITED   = 2,
    LOGC_IMPORTANT = 3,
    LOGC_VIC       = 4,
    LOGC_DIM       = 5,
    LOGC_HEADING   = 6,
};

enum
{
    GU_FLAG_FROMNEWLINE = 1 << 0,
    GU_FLAG_FILLHORZ    = 1 << 2,
    GU_FLAG_FILLVERT    = 1 << 3,
};

enum
{
    PHYS_KIND_BY_ID   = 1,
    PHYS_KIND_BY_NAME = 2,
};

typedef struct
{
    int kind;
    union {
        int         phys_id;
        const char *chan_name;
    } d;
} phys_rec;

typedef struct
{
    int     numpts;
    double *pts;
    double  data[0];
} excmd_lapprox_rec;

typedef excmd_lapprox_rec * (*excmd_lapproxld_func)(const void *ptr, int param);

typedef struct
{
    const void *ptrparam;
    int         intparam;
} excmd_lapprox_info_rec;

typedef struct
{
    excmd_lapproxld_func    ldr;
    excmd_lapprox_info_rec *info;
} excmd_lapproxnet_rec;

typedef union
{
    double                number;
    int                   chan_id;
    int                   handle;
    int                   displacement;
    char                 *label;
    char                 *chanref;
    char                 *str;
    excmd_lapprox_rec    *lapprox_rp;
    excmd_lapproxnet_rec *lapprox_srcinfo;
    void                 *biginfo;  /*!!!<- what for?!*/
} excmd_content_t;

typedef struct
{
    unsigned char    cmd;
    excmd_content_t  arg;
} excmd_t;

typedef struct {
    char *ident;
    char *label;
    char *units;
    char *dpyfmt;
    char *widdepinfo;
    char *comment;

    /* Classification */
    int         type;     /* LOGT_XXX -- read/.../write */
    int         look;     /* LOGD_XXX -- which widget to display */
    int         kind;     /* LOGK_XXX -- direct/calculated */
    int         color;    /* LOGC_XXX -- colorization */

    /* Reference */
#if PHYS_REC
    phys_rec    pr;
#else
    int         phys;     /* Physical channel number... */
#endif
    excmd_t    *formula;  /* Formula for calculated channels */
    excmd_t    *revformula; /* In fact, "excmd_t *" -- reverse calculation formula */

    /* Information properties */
    double      minnorm;  /* Min possible value */
    double      maxnorm;  /* Max possible value */
    double      defnorm;  /* Default value */
    double      minyelw;  /* Min "poorly acceptable" */
    double      maxyelw;  /* Max "poorly acceptable" */

    double      incdec_step; /* Step for up/down arrows */
    
    /* Display properties */
    double      mindisp;  /* lower... */
    double      maxdisp;  /* ...and upper bounds for graph displaying */

    /**/
    char *placement;
#if PHYS_REC
    char         *style;
#endif
} logchannet_t;

typedef struct
{
    int         n;
    double      r;
    double      d;
    int         q;
} physprops_t;

typedef struct
{
    const char  *srv;
    physprops_t *info;
    int          count;
} physinfodb_rec_t;


typedef struct {
    char         *ident;
    char         *label;
    char         *colnames;
    char         *rownames;
    int           type;
    int           count;
    int           ncols;
    logchannet_t *channels;
    char         *options;
#if PHYS_REC
    char         *style;
    char         *defserver;
#endif
} elemnet_t;

typedef struct {
    elemnet_t *e;
    int        fromnewline;
} groupunit_t;


//////////////////////////////////////////////////////////////////////

#define SUBSYSDESCR_SUFFIX     _subsysdescr
#define SUBSYSDESCR_SUFFIX_STR __CX_STRINGIZE(SUBSYSDESCR_SUFFIX)

enum {CXSS_SUBSYS_MAGIC   = 0x53627553}; /* Little-endian 'SubS' */
enum
{
#if PHYS_REC
    CXSS_SUBSYS_VERSION_MAJOR = 5,
#else
    CXSS_SUBSYS_VERSION_MAJOR = 4,
#endif
    CXSS_SUBSYS_VERSION_MINOR = 0,
    CXSS_SUBSYS_VERSION = CX_ENCODE_VERSION(CXSS_SUBSYS_VERSION_MAJOR,
                                            CXSS_SUBSYS_VERSION_MINOR)
};

typedef struct
{
    int           magicnumber;
    int           version;
    const char   *sysname;
    int           kind;
    const char   *defserver;
    const char   *app_name;
    const char   *app_class;
    const char   *win_title;
    const char  **icon;
    physprops_t  *phys_info;
    int           phys_info_count;
    groupunit_t  *grouping;
    const char   *clientprog;
    const char   *options;
} subsysdescr_t;

#define DEFINE_SUBSYS_DESCR(name, defserver,                           \
                            app_name, app_class,                       \
                            win_title, icon,                           \
                            phys_info, phys_info_count,                \
                            grouping,                                  \
                            clientprog, options)                       \
    subsysdescr_t __CX_CONCATENATE(name, SUBSYSDESCR_SUFFIX) =         \
    {                                                                  \
        CXSS_SUBSYS_MAGIC, CXSS_SUBSYS_VERSION,                        \
        __CX_STRINGIZE(name),                                          \
        0,                                                             \
        defserver, app_name, app_class,                                \
        win_title, icon,                                               \
        phys_info, phys_info_count,                                    \
        grouping,                                                      \
        clientprog, options                                            \
    }


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CXDATA_H */
