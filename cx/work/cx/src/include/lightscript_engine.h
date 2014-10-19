#ifndef __LIGHTSCRIPT_ENGINE_H
#define __LIGHTSCRIPT_ENGINE_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


enum
{
    LSE_R_OK  = 0,
    LSE_R_ERR = -1,
};

enum
{
    LSE_PARSE_ONLY = 1 << 0,
};


struct _lse_context_t_struct;

typedef int (*lse_cmd_proc_t)(struct _lse_context_t_struct *ctx,
                              const char *p, const char **after_p,
                              int options);

typedef struct
{
    const char     *name;
    lse_cmd_proc_t  proc;
} lse_cmd_descr_t;

typedef struct _lse_context_t_struct
{
    char             scriptname[31];
    const char      *script;
    const char      *cur_ptr;

    lse_cmd_descr_t *cmdset;
    void            *privptr;
} lse_context_t;


int lse_init(lse_context_t *ctx,
             const char *scriptname, const char *script,
             lse_cmd_descr_t *cmdset,  void  *privptr);
int lse_run (lse_context_t *ctx, int options);
int lse_cont(lse_context_t *ctx);
int lse_stop(lse_context_t *ctx);

int lse_is_running(lse_context_t *ctx);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __LIGHTSCRIPT_ENGINE_H */
