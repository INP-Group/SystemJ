#ifndef __FQKN_H
#define __FQKN_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


typedef int (*fqkn_expand_cb_t)(const char *name, const char *p, void *privptr);


int fqkn_expand(const char *p, fqkn_expand_cb_t cb, void *privptr);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __FQKN_H */
