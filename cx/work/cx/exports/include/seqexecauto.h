#ifndef __SEQEXECAUTO_H
#define __SEQEXECAUTO_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <stdarg.h>
#include <sys/time.h>


typedef enum
{
    SEA_RSLT_NONE    =  0,
    SEA_RSLT_SUCCESS = +1,
    SEA_RSLT_FATAL   = -1,
    SEA_RSLT_USRERR  = -2,
    SEA_RSLT_GOAWAY  = -3
} sea_rslt_t;


struct _sea_context_t_struct;
struct _sea_step_t_struct;

typedef sea_rslt_t (*sea_action_t)(struct _sea_context_t_struct *ctx,
                                   struct _sea_step_t_struct    *step);
typedef sea_rslt_t (*sea_cndchk_t)(struct _sea_context_t_struct *ctx,
                                   struct _sea_step_t_struct    *step,
                                   int                           evtype);

typedef void (*sea_cleanup_t)(struct _sea_context_t_struct *ctx,
                              sea_rslt_t reason);
typedef void (*sea_message_t)(struct _sea_context_t_struct *ctx,
                              const char *format, va_list ap);

typedef void (*sea_on_term_t)(struct _sea_context_t_struct *ctx,
                              sea_rslt_t reason);

typedef struct _sea_context_t_struct
{
    /* Client's part ("public") */
    sea_cleanup_t              cleanup;
    sea_message_t              make_message;
    void                      *privptr;

    /* Library's part ("private") */
    struct _sea_step_t_struct *firstcmd;     /* Program beginning */
    struct _sea_step_t_struct *curcmd;       /* Program Counter */
    int                        must_wait;    /* Delay is active */
    struct timeval             wait_until;   /* Delay register */
    char                       progname[31]; /* Program name */
    sea_on_term_t              on_term;
} sea_context_t;

typedef struct _sea_step_t_struct
{
    const char   *label;
    sea_action_t  action;
    sea_cndchk_t  check;
    void         *actarg;
    int           chkevtype;
    int           chkdelay;
    void         *param2;
} sea_step_t;


void        sea_run  (sea_context_t *ctx, const char *progname, sea_step_t *step);
void        sea_run4 (sea_context_t *ctx, const char *progname, sea_step_t *step,
                      sea_on_term_t  on_term);
void        sea_check(sea_context_t *ctx, int evtype);
void        sea_stop (sea_context_t *ctx);
void        sea_next (sea_context_t *ctx);
sea_rslt_t  sea_goto (sea_context_t *ctx, const char *label);

int  sea_is_running(sea_context_t *ctx);


sea_rslt_t SEA_EMPTY_ACTION  (sea_context_t *ctx, sea_step_t *step);
sea_rslt_t SEA_SUCCESS_ACTION(sea_context_t *ctx, sea_step_t *step);
sea_rslt_t SEA_SUCCESS_CHECK (sea_context_t *ctx, sea_step_t *step, int evtype);

#define SEA_DELAY_STEP(usecs) \
    {"SEA_DELAY", SEA_EMPTY_ACTION, SEA_SUCCESS_CHECK, NULL, 0, usecs, NULL}

#define SEA_END() \
    {":end_of_sequence:", NULL, NULL, NULL, 0, 0, NULL}

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SEQEXECAUTO_H */
