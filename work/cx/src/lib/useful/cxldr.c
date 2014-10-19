#ifndef CPU_MCF5200
  #define  MAY_USE_DLOPEN 1
#else
  #define  MAY_USE_DLOPEN 0
#endif


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#if MAY_USE_DLOPEN
  #include <dlfcn.h>
#endif /* MAY_USE_DLOPEN */

#include "misc_macros.h"
#include "findfilein.h"

#include "cxldr.h"


enum { CURLIST_GROW_INCREMENT = 10 };

static void *dlopen_checker(const char *name,
                            const char *path,
                            void       *privptr)
{
  void            *handle;
  char             buf[PATH_MAX];
  char             plen = strlen(path);
  cxldr_context_t *ctx  = privptr;
  int              flags;

    check_snprintf(buf, sizeof(buf), "%s%s%s%s%s",
                   path,
                   plen == 0  ||  path[plen-1] == '/'? "" : "/",
                   ctx->fprefix, name, ctx->fsuffix);
    flags = RTLD_NOW | (ctx->flags & CXLDR_FLAG_GLOBAL? RTLD_GLOBAL : 0);
    handle = dlopen(buf, flags);
    ////fprintf(stderr, "dlopen(\"%s\"): %p; %s\n", buf, handle, dlerror());
    return handle;
}


int  cxldr_get_module(cxldr_context_t  *ctx,
                      const char       *name,
                      const char       *argv0,
                      void            **symptr_p,
                      void *privptr)
{
  int               n;
  cxldr_used_rec_t *mod_p;
  cxldr_bltn_rec_t *blt_p;
  void             *handle  = NULL;
  void             *symptr  = NULL;
  const char       *nameptr = NULL;
  int               is_bltn = 0;
  cxldr_used_rec_t *new_curlist;
  char              symbuf[200];
  int               saved_errno;

    if (name == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    /* 0. Look through already-used modules */
    for (n = 0, mod_p = ctx->curlist;
         n < ctx->curlistcount;
         n++,   mod_p++)
        if (mod_p->name != NULL  &&  strcmp(mod_p->name, name) == 0)
            goto MODULE_FOUND;

    /* 1. Try to load module */
#if MAY_USE_DLOPEN
    handle = findfilein(name,
                        argv0,
                        dlopen_checker, ctx,
                        ctx->path);
    if (handle != NULL)
    {
        check_snprintf(symbuf, sizeof(symbuf),
                       "%s%s%s", ctx->symprefix, name, ctx->symsuffix);
        symptr = dlsym(handle, symbuf);
        ////fprintf(stderr, "dlsym(%s::%s): %p\n", name, symbuf, symptr);
        if (symptr == NULL)
        {
            dlclose(handle);
            errno = 0; /*!!! No better code -- '0' means "no symbol found" */
            return -1;
        }

        nameptr = strdup(name);
        goto REGISTER_MODULE;
    }
#endif /* MAY_USE_DLOPEN */

    /* 2. Check builtin modules */
    for (blt_p = ctx->builtins;
         blt_p != NULL  &&  blt_p->name != NULL;
         blt_p++)
        if (strcmp(blt_p->name, name) == 0)
        {
            symptr  = blt_p->symptr;
            nameptr = blt_p->name;
            is_bltn = 1;
            goto REGISTER_MODULE;
        }

    /* Ooops... None found... */
    errno = ENOENT;
    goto CLEANUP_ON_ERROR;

REGISTER_MODULE:
    /* First check if module is acceptable */
    if (ctx->checker(name, symptr, privptr) < 0) goto CLEANUP_ON_ERROR;
    /* Are we constrained to program-supplied buffer? */
    if (ctx->curlistfixed)
    {
        for (n = 0,  mod_p = ctx->curlist;
             n < ctx->curlistfixed;
             n++,    mod_p++)
            if (mod_p->name == NULL) goto REG_CELL_FOUND;
        /* No free cell found... */
        errno = ENOMEM;
        goto CLEANUP_ON_ERROR;
    }
    /* No, we may use malloc()/realloc() */
    else
    {
        for (n = 0,  mod_p = ctx->curlist;
             n < ctx->curlistcount;
             n++,    mod_p++)
            if (mod_p->name == NULL) goto REG_CELL_FOUND;

        /* No free cell found?  Okay, let's allocate some more */
        new_curlist = safe_realloc(ctx->curlist,
                                   sizeof(*new_curlist) * (ctx->curlistcount + CURLIST_GROW_INCREMENT));
        if (new_curlist == NULL) goto CLEANUP_ON_ERROR;
        
        bzero(new_curlist + ctx->curlistcount, sizeof(*new_curlist) * CURLIST_GROW_INCREMENT);
        ctx->curlist       = new_curlist;
        ctx->curlistcount += CURLIST_GROW_INCREMENT;
        mod_p = ctx->curlist + ctx->curlistcount - CURLIST_GROW_INCREMENT;
    }

REG_CELL_FOUND:
    bzero(mod_p, sizeof(*mod_p));
    mod_p->name    = nameptr;
    mod_p->handle  = handle;
    mod_p->symptr  = symptr;
    mod_p->is_bltn = is_bltn;

MODULE_FOUND:
    mod_p->ref_count++;
    if (mod_p->ref_count == 1)
    {
        if (ctx->cm_init != NULL  &&
            ctx->cm_init(mod_p->name, mod_p->symptr) < 0)
        {
            /*!!! should release cell somehow...*/
            return -1;
        }
    }

    if (symptr_p != NULL) *symptr_p = mod_p->symptr;
    
    return 0;
    
CLEANUP_ON_ERROR:
    saved_errno = errno;
#if MAY_USE_DLOPEN
    if (handle != NULL) dlclose(handle);
#endif /* MAY_USE_DLOPEN */
    if (!is_bltn  &&  nameptr != NULL) free(nameptr);
    errno = saved_errno;
    return -1;
}

int  cxldr_fgt_module(cxldr_context_t  *ctx,
                      const char       *name)
{
  int               n;
  cxldr_used_rec_t *mod_p;
  
    if (name == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    
    for (n = 0, mod_p = ctx->curlist;
         n < ctx->curlistcount;
         n++,   mod_p++)
        if (mod_p->name != NULL  &&  strcmp(mod_p->name, name) == 0)
        {
            mod_p->ref_count--;
            if (mod_p->ref_count == 0)
            {
                if (ctx->cm_fini != NULL)
                    ctx->cm_fini(mod_p->name, mod_p->symptr);
                if (!mod_p->is_bltn) dlclose(mod_p->handle);
                if (!mod_p->is_bltn) free(mod_p->name);
                bzero(mod_p, sizeof(*mod_p));
            }
            
            return 0;
        }

    errno = ENOENT;
    return -1;
}

int  cxldr_set_path  (cxldr_context_t  *ctx, const char *path)
{
    ctx->path = path;

    return 0;
}

int  cxldr_set_bltns (cxldr_context_t  *ctx, cxldr_bltn_rec_t *list)
{
    ctx->builtins = list;

    return 0;
}
