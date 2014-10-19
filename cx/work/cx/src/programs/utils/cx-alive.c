#include "cxlib.h"

int main(int argc __attribute__((unused)), char *argv[])
{
  int r;
    
    r = cx_connect(argv[1], argv[0]);
    if (r >= 0) cx_close(r);
  
    return r < 0;
}
