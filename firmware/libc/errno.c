#include "../thread-internal.h"
int * __errno(void)
{
    return &__running_self_entry()->__errno;
}
