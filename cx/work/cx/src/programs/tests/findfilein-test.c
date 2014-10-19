#include <stdio.h>
#include <sys/param.h> // For PATH_MAX, at least on ARM@MOXA

#include "misc_macros.h"
#include "findfilein.h"


static void *test_fopen_checker(const char *name,
                                const char *path,
                                void       *privptr)
{
  char  buf[PATH_MAX];
  char  plen = strlen(path);

    printf("...\"%s\" in \"%s\"\n", name, path);
    check_snprintf(buf, sizeof(buf), "%s%s%s%s",
                   path,
                   plen == 0  ||  path[plen-1] == '/'? "" : "/",
                   name,
                   privptr == NULL? "" : (char *)privptr);
    return fopen(buf, "r");
}

int main(int argc, char *argv[])
{
  int   n;
  FILE *fp;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s SEARCH-PATH filename...\n", argv[0]);
        exit (1);
    }

    for (n = 2;  n < argc;  n++)
    {
        printf("Searching for \"%s\"\n", argv[n]);
        fp = findfilein(argv[n], argv[0],
                        test_fopen_checker, NULL, argv[1]);
        printf("\t%s\n", fp != NULL? "found" : "not-found");
        if (fp != NULL) fclose(fp);
    }

    return 0;
}
