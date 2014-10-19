#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "misc_macros.h"

#include "remdrv_proto.h"
#include "remdrvlet.h"


static void newdev_p(int fd,
                     struct  sockaddr  *addr __attribute__((unused)),
                     const char *name)
{
  const char *prefix = remdrvlet_srvmain_param(0);
  const char *suffix = remdrvlet_srvmain_param(1);
  char        path[1024];
  int         r;
  int         saved_errno;
  char       *args[2];

    /* Prepare name of file to execute */
    if (prefix == NULL) prefix = "";
    if (suffix == NULL) suffix = "";
    check_snprintf(path, sizeof(path), "%s%s%s", prefix, name, suffix);

    /* Fork into separate process */
    r = fork();
    switch (r)
    {
        case  0:  /* Child */
            break;
        case -1:  /* Failed to fork */
            saved_errno = errno;
            remdrvlet_debug ("fork=%d: %s", r, strerror(saved_errno));
            remdrvlet_report(fd, REMDRV_C_RRUNDP, "%s::%s(): fork=%d: %s",
                             __FILE__, __FUNCTION__, r, strerror(saved_errno));
            /*FALLTHROUGH*/
        default:  /* Parent */
            close(fd);
            return;
    }
    
    /* Clone connection fd into stdin/stdout. !!!Don't touch stderr!!! */
    dup2(fd, 0);  fcntl(0, F_SETFD, 0);
    dup2(fd, 1);  fcntl(1, F_SETFD, 0);
    close(fd);
    
    /* Exec the required program */
    args[0] = path;
    args[1] = NULL;
    ////fprintf(stderr, "About to exec \n\"%s\"\n", path);
    r = execv(path, args);
    saved_errno = errno;
    remdrvlet_debug ("can't execv(\"%s\"): %s", path, strerror(saved_errno));
    remdrvlet_report(1, REMDRV_C_RRUNDP, "%s::%s(): can't execv(\"%s\"): %s",
                     __FILE__, __FUNCTION__, path, strerror(saved_errno));
    _exit(0);
}

int main(int argc, char *argv[])
{
    return remdrvlet_srvmain(argc, argv,
                             newdev_p,
                             NULL);
}
