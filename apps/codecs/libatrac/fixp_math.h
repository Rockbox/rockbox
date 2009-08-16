#include <stdlib.h>
#include <inttypes.h>

/* Macros for converting between various fixed-point representations and floating point. */
#define ONE_16 (1L << 16)
#define fixtof64(x)       (float)((float)(x) / (float)(1 << 16))        //does not work on int64_t!
#define ftofix32(x)       ((int32_t)((x) * (float)(1 << 16) + ((x) < 0 ? -0.5 : 0.5)))
#define ftofix31(x)       ((int32_t)((x) * (float)(1 << 31) + ((x) < 0 ? -0.5 : 0.5)))
#define fix31tof64(x)     (float)((float)(x) / (float)(1 << 31))

/* Fixed point math routines for use in atrac3.c */

static inline int32_t fixmul16(int32_t x, int32_t y)
{
    int64_t temp;
    temp = x;
    temp *= y;

    temp >>= 16;

    return (int32_t)temp;
}

static inline int32_t fixmul31(int32_t x, int32_t y)
{
    int64_t temp;
    temp = x;
    temp *= y;

    temp >>= 31;        //16+31-16 = 31 bits

    return (int32_t)temp;
}

static inline int32_t fixdiv16(int32_t x, int32_t y)
{
    int64_t temp;
    temp = x << 16;
    temp /= y;

    return (int32_t)temp;
}

/*
 * Fast integer square root adapted from algorithm, 
 * Martin Guy @ UKC, June 1985.
 * Originally from a book on programming abaci by Mr C. Woo.
 * This is taken from :
 * http://wiki.forum.nokia.com/index.php/How_to_use_fixed_point_maths#How_to_get_square_root_for_integers
 * with a added shift up of the result by 8 bits to return result in 16.16 fixed-point representation.
 */
static inline int32_t fastSqrt(int32_t n)
{
   /*
    * Logically, these are unsigned. 
    * We need the sign bit to test
    *	whether (op - res - one) underflowed.
    */
    int32_t op, res, one;
    op = n;
    res = 0;
    /* "one" starts at the highest power of four <= than the argument. */
    one = 1 << 30;	/* second-to-top bit set */
    while (one > op) one >>= 2;
    while (one != 0) 
    {
        if (op >= res + one) 
        {
            op = op - (res + one);
            res = res +  (one<<1);
        }
        res >>= 1;
        one >>= 2;
    }
    return(res << 8);
}
