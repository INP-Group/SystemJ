#include <stdio.h>

#include "cxscheduler.h"

#include "Cdr.h"


static void DataCB(int handle, double val, void *privptr)
{
    printf("#%d: %8.3f  // %s\n", handle, val, (char *)privptr);
}

static void BigcCB(int handle,             void *privptr)
{
    printf("#%d: // %s\n",        handle,      (char *)privptr);
}

int main(int argc, char *argv[])
{
  int  x;
  int  cid;
  int  bid;

    for (x = 1;  x < argc;  x++)
        if (argv[x][0] != '.')
        {
            cid = CdrRegisterSimpleChan(argv[x], argv[0], DataCB, argv[x]);
            fprintf(stderr, "chan\"%s\"->%d\n", argv[x], cid);
        }
        else
        {
            bid = CdrRegisterSimpleBigc(argv[x] + 1, argv[0],
                                        1,
                                        BigcCB, argv[x]);
            fprintf(stderr, "bigc\"%s\"->%d\n", argv[x], bid);
        }

    sl_main_loop();

    return 0;
}
