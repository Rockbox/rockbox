/* For archs that lack assembly optimized versions of those */

#include "string.h"

_PTR _EXFUN(memset,(_PTR, int, size_t));

_PTR memset(_PTR data, int val, size_t count)
{
    for (int i=0; i < count; i++)
        ((char*)data)[i] = val;
    return data;
}

_PTR memcpy(_PTR dst, const _PTR src, size_t count)
{
    for (int i=0; i < count; i++)
        ((char*)dst)[i] = ((char*)src)[i];
    return dst;
}
