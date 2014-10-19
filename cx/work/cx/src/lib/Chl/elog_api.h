#ifndef __ELOG_API_H
#define __ELOG_API_H


#define ELOG_HOST_ENV "ELOG_HOST"


int DoELog(const char *host,
           const char *sysname,
           const char *sender,
           const char *title,
           const char *message,
           const char *msgcharset);

int IsELogPossible(const char *host);


#endif /* __ELOG_API_H */
