#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "fqkn.h"


static inline int isletnumdot(int c)
{
    return isalnum(c)  ||  c == '_'  ||  c == '-'  ||  c == '.';
}


static const char *FindCloseBr(const char *p)
{
    while (1)
    {
        if      (isletnumdot(*p)  ||  *p == ',') p++;
        else if (*p == '}')       return p;
        else                      {fprintf(stderr, "Find}: p=<%s>\n", p);return NULL;}
    }
}

static int do_one_stage(char *path_buf, size_t path_buf_size, size_t path_len,
                        const char *seg, size_t seg_len,
                        const char *curp,
                        fqkn_expand_cb_t cb, void *privptr)
{
  int         ret  = 0;
  const char *srcp = curp;

  const char *clsp;
  const char *begp;

    /* Append next segment */
    if (seg_len > path_buf_size - 1 - path_len)
        seg_len = path_buf_size - 1 - path_len;
    if (seg_len > 0)
    {
        memcpy(path_buf + path_len, seg, seg_len);
        path_len += seg_len;
    }
  
    while (1)
    {
        if      (*srcp == '{')
        {
            /*!!! Note: now it is a simple, non-nestable, comma-list-only  */
            srcp++;

            /* Find a closing '}' first */
            clsp = FindCloseBr(srcp);
            if (clsp == NULL)
            {
                ret = -1;
                /*!!!Some error-signaling? */
                goto END_PARSE;
            }

            /* Go through list */
            for (begp = srcp; ;)
            {
                if (*srcp == ','  ||  *srcp == '}')
                {
                    ret = do_one_stage(path_buf, path_buf_size, path_len,
                                       begp, srcp - begp,
                                       clsp + 1,
                                       cb, privptr);
                    if (ret != 0) goto END_PARSE;

                    if (*srcp == '}') goto END_PARSE;

                    srcp++; // Skip ','
                    begp = srcp;
                }
                else
                    srcp++;
            }
        }
        else if (*srcp == '\0'  ||  !isletnumdot(*srcp))
        {
            path_buf[path_len] = '\0';
            if (cb != NULL)
                ret = cb(path_buf, srcp, privptr);
            goto END_PARSE;
        }
        else
        {
            if (path_len < path_buf_size - 1)
                path_buf[path_len++] = *srcp;
            srcp++;
        }
    }
 END_PARSE:

    return ret;
}

int fqkn_expand(const char *p, fqkn_expand_cb_t cb, void *privptr)
{
  char    path_buf[1000];
    
    if (p == NULL) return -1;

    return do_one_stage(path_buf, sizeof(path_buf), 0,
                        "", 0,
                        p,
                        cb, privptr);
}
