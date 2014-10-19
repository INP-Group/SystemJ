#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "misclib.h"
#include "memcasecmp.h"

#include "lightscript_engine.h"


static inline int isletnum(int c)
{
    return isalnum(c)  ||  c == '_'  ||  c == '-';
}


static int script_sleep_cmd(lse_context_t *ctx,
                            const char    *p, const char **after_p,
                            int            options)
{
  double  seconds;
  char   *err;

    seconds = strtod(p, &err);
    if (err == p)
    {
        return LSE_R_ERR;
    }
    if (seconds < 0) seconds = 0;

    *after_p = err;
    return (options & LSE_PARSE_ONLY) != 0?  LSE_R_OK : (int)(seconds * 1000000);
}

static int script_echo_cmd(lse_context_t *ctx,
                           const char    *p, const char **after_p,
                           int            options)
{
    if ((options & LSE_PARSE_ONLY) == 0) fprintf(stderr, "%s SCRIPT-\"%s\" ECHO: ", strcurtime(), ctx->scriptname);
    while (*p != '\0'  &&  *p != '\n')
    {
        if ((options & LSE_PARSE_ONLY) == 0) fprintf(stderr, "%c", *p);
        p++;
    }
    if ((options & LSE_PARSE_ONLY) == 0) fprintf (stderr, "\n");

    if (after_p != NULL) *after_p = p;

    return LSE_R_OK;
}

static lse_cmd_descr_t builtin_cmdset[] =
{
    {"sleep", script_sleep_cmd},
    {"echo",  script_echo_cmd},
    {NULL, NULL}
};

static lse_cmd_proc_t FindProcesserIn(const char  *name, size_t namelen,
                                      lse_cmd_descr_t *cmdset)
{
  lse_cmd_proc_t   proc;
  lse_cmd_descr_t *cmdd;

    for (proc =  NULL,     cmdd = cmdset;
         proc == NULL  &&  cmdd->name != NULL;
                           cmdd++)
        if (cx_strmemcasecmp(cmdd->name, name, namelen) == 0)
            proc = cmdd->proc;
    return proc;
}


static int do_exec_script(lse_context_t *ctx, int  options)
{
  const char     *begp;
  lse_cmd_proc_t  proc;
  int             r;
  const char     *endptr;

    if (ctx->cur_ptr == NULL) return -1;

    while (1)
    {
        /* Skip whitespace */
        while (*(ctx->cur_ptr) != '\0'  &&  isspace(*(ctx->cur_ptr)))
            ctx->cur_ptr++;

        /* End? */
        if (*(ctx->cur_ptr) == '\0')
        {
            lse_stop(ctx);
            return 0;
        }

        /* Okay, let's extract command name... */
        begp = ctx->cur_ptr;
        while (isletnum(*(ctx->cur_ptr))) ctx->cur_ptr++;
        if (ctx->cur_ptr == begp)
        {
            /*!!! Bark... */
            return -1;
        }
        
        /* ...and find the processer */
        proc = FindProcesserIn(begp, ctx->cur_ptr - begp, ctx->cmdset);
        if (proc == NULL)
            proc = FindProcesserIn(begp, ctx->cur_ptr - begp, builtin_cmdset);
        if (proc == NULL)
        {
            /*!!! Bark... */
            return -1;
        }

        /* Skip separating whitespace */
        while (*(ctx->cur_ptr) != '\0'  &&  isspace(*(ctx->cur_ptr)))
            ctx->cur_ptr++;

        /* Finally, call the processer */
        r = proc(ctx, ctx->cur_ptr, &(ctx->cur_ptr), options);
        if      (r < 0)
        {
            /*!!! Bark... */
            return r;
        }
        else if (r > 0)
        {
            return r;
        }
        /* else proceed to next command */
    }
}


/*** Public API *****************************************************/

int lse_init(lse_context_t *ctx,
             const char *scriptname, const char *script,
             lse_cmd_descr_t *cmdset,  void  *privptr)
{
  int  r;
    
    errno = 0;
    
    bzero(ctx, sizeof(*ctx));

    if (scriptname == NULL) scriptname = "UNNAMED";
    strzcpy(ctx->scriptname, scriptname, sizeof(ctx->scriptname));
    ctx->script  = script;

    ctx->cmdset  = cmdset;
    ctx->privptr = privptr;

    r = lse_run(ctx, LSE_PARSE_ONLY);
    lse_stop(ctx);

    return r;
}

int lse_run (lse_context_t *ctx, int options)
{
    errno = 0;

    if (ctx->cur_ptr != NULL)
    {
        errno = EBUSY;
        return -1;
    }

    ctx->cur_ptr = ctx->script;
    return do_exec_script(ctx, options);
}

int lse_cont(lse_context_t *ctx)
{
    errno = 0;

    return do_exec_script(ctx, 0);
}

int lse_stop(lse_context_t *ctx)
{
    ctx->cur_ptr = NULL;
    
    return 0;
}

int lse_is_running(lse_context_t *ctx)
{
    return ctx->cur_ptr != NULL;
}
