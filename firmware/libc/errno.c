#include "thread.h"
int * __errno(void)
{
    return &thread_self_entry()->__errno;
}
