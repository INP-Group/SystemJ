#include <stdio.h>
#include <string.h>

#include "seqexecauto.h"
#include "timeval_utils.h"
#include "misc_macros.h"


static inline int IS_RUNNING(sea_context_t *ctx) {return ctx->curcmd != NULL;}

static void MakeMessage(sea_context_t *ctx, const char *format, ...)
                       __attribute__((format (printf, 2, 3)));
static void MakeMessage(sea_context_t *ctx, const char *format, ...)
{
  va_list  ap;
  
    va_start(ap, format);
    if (ctx->make_message != NULL) ctx->make_message(ctx, format, ap);
    va_end(ap);
}

typedef enum
{
    STOP_R_FINISH,
    STOP_R_ABORT,
    STOP_R_FATAL
} stopreason_t;

static void StopBecause(sea_context_t *ctx, stopreason_t reason)
{
  sea_cleanup_t  cleanup;
  sea_on_term_t  on_term;

    if (!IS_RUNNING(ctx)) return;

    switch (reason)
    {
        case SEA_RSLT_SUCCESS:
            MakeMessage(ctx, "%s: finish", ctx->progname);
            break;

        case SEA_RSLT_NONE:
            MakeMessage(ctx, "%s: abort at step \"%s\"", ctx->progname, ctx->curcmd->label);
            break;

        default: /* SEA_RSLT_FATAL */
            MakeMessage(ctx, "%s: fatal error at step \"%s\"", ctx->progname, ctx->curcmd->label);
            break;
    }

    ctx->firstcmd = ctx->curcmd = NULL;

    cleanup = ctx->cleanup;
    on_term = ctx->on_term;

    if (cleanup != NULL) cleanup(ctx, reason);
    if (on_term != NULL) on_term(ctx, reason);
}


static void ExecuteFrom(sea_context_t *ctx, sea_step_t *step)
{
  sea_rslt_t  r;
  char        dmbuf[100];
    
    for (ctx->curcmd = step;  ctx->curcmd->action != NULL;  ctx->curcmd++)
    {
        if (ctx->make_message != NULL)
        {
            if (ctx->curcmd->chkdelay != 0)
                sprintf(dmbuf, " (and %dus delay)", ctx->curcmd->chkdelay);
            else
                dmbuf[0] = '\0';
            MakeMessage(ctx, "%s: About to exec \"%s\" action%s",
                        ctx->progname, ctx->curcmd->label, dmbuf);
        }
        
        ctx->must_wait = 0;
        r = ctx->curcmd->action(ctx, ctx->curcmd);

        if (r == SEA_RSLT_FATAL)
        {
            StopBecause(ctx, SEA_RSLT_FATAL);
            return;
        }
        if (r == SEA_RSLT_SUCCESS)
            goto NEXT_STEP;
        if (r == SEA_RSLT_GOAWAY)
            return;
        /* SEA_RSLT_NONE */
        if (ctx->curcmd->chkdelay != 0)
        {
            gettimeofday(&(ctx->wait_until), NULL);
            timeval_add_usecs(&(ctx->wait_until),
                              &(ctx->wait_until),
                              ctx->curcmd->chkdelay);
            ctx->must_wait = 1;
        }

        return;

 NEXT_STEP:;
    }

    StopBecause(ctx, SEA_RSLT_SUCCESS);
}

void        sea_run4 (sea_context_t *ctx, const char *progname, sea_step_t *step,
                      sea_on_term_t  on_term)
{
    if (progname != NULL)
        strzcpy(ctx->progname, progname,    sizeof(ctx->progname));
    else
        strzcpy(ctx->progname, "<UNNAMED>", sizeof(ctx->progname));

    ctx->firstcmd = step;
    ctx->on_term  = on_term;
    
    ExecuteFrom(ctx, step);
}

void        sea_run  (sea_context_t *ctx, const char *progname, sea_step_t *step)
{
    sea_run4(ctx, progname, step, NULL);
}

void        sea_check(sea_context_t *ctx, int evtype)
{
  sea_rslt_t  r;
  struct timeval  now;
    
    if (!IS_RUNNING(ctx)) return;
    
    if (ctx->curcmd->check == NULL) return;
    if (ctx->curcmd->chkevtype != 0  &&  ctx->curcmd->chkevtype != evtype) return;

    if (ctx->must_wait)
    {
        gettimeofday(&now, NULL);
        if (!TV_IS_AFTER(now, ctx->wait_until)) return;

        ctx->must_wait = 0;
    }

    r = ctx->curcmd->check(ctx, ctx->curcmd, evtype);
    
    if (r == SEA_RSLT_FATAL)
    {
        StopBecause(ctx, SEA_RSLT_FATAL);
        return;
    }
    if (r == SEA_RSLT_SUCCESS)
    {
        ctx->curcmd++;
        ExecuteFrom(ctx, ctx->curcmd);
    }
    /*
     else -- SEA_RSLT_NONE, do nothing
     */
}

void        sea_stop (sea_context_t *ctx)
{
    StopBecause(ctx, SEA_RSLT_NONE);
}

void        sea_next (sea_context_t *ctx)
{
    if (!IS_RUNNING(ctx)) return;

    ctx->curcmd++;
    ExecuteFrom(ctx, ctx->curcmd);
}

sea_rslt_t  sea_goto (sea_context_t *ctx, const char *label)
{
  sea_step_t *step;
    
    if (!IS_RUNNING(ctx)) return SEA_RSLT_USRERR;

    for (step = ctx->firstcmd; step->action != NULL;  step++)
        if (strcasecmp(label, step->label) == 0)
        {
            ExecuteFrom(ctx, step);
            return SEA_RSLT_NONE;
        }

    return SEA_RSLT_FATAL;
}

int  sea_is_running(sea_context_t *ctx)
{
    return IS_RUNNING(ctx);
}


sea_rslt_t SEA_EMPTY_ACTION(sea_context_t *ctx __attribute__((unused)), sea_step_t *step __attribute__((unused)))
{
    return SEA_RSLT_NONE;
}

sea_rslt_t SEA_SUCCESS_ACTION(sea_context_t *ctx __attribute__((unused)), sea_step_t *step __attribute__((unused)))
{
    return SEA_RSLT_SUCCESS;
}

sea_rslt_t SEA_SUCCESS_CHECK(sea_context_t *ctx __attribute__((unused)), sea_step_t *step __attribute__((unused)), int evtype __attribute__((unused)))
{
    return SEA_RSLT_SUCCESS;
}

