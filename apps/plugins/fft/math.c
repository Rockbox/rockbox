#include "math.h"

int64_t fsqrt64(int64_t a, unsigned int fracbits)
{
    int64_t b = a/2 + (1 << fracbits); /* initial approximation */
    unsigned int n;
    const unsigned int iterations = 3; /* very rough approximation */

    for (n = 0; n < iterations; ++n)
        b = (b + (((int64_t)(a) << fracbits)/b))/2;

    return b;
}
