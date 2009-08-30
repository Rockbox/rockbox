#include <stdlib.h>
#include <inttypes.h>

/* Macros for converting between various fixed-point representations and floating point. */
#define ONE_16 (1L << 16)
#define fixtof64(x)       (float)((float)(x) / (float)(1 << 16))        //does not work on int64_t!
#define ftofix32(x)       ((int32_t)((x) * (float)(1 << 16) + ((x) < 0 ? -0.5 : 0.5)))
#define ftofix31(x)       ((int32_t)((x) * (float)(1 << 31) + ((x) < 0 ? -0.5 : 0.5)))
#define fix31tof64(x)     (float)((float)(x) / (float)(1 << 31))

/* Fixed point math routines for use in atrac3.c */

#if defined(CPU_ARM)
    #define fixmul16(X,Y) \
     ({ \
        int32_t low; \
        int32_t high; \
        asm volatile (                   /* calculates: result = (X*Y)>>16 */ \
           "smull  %0,%1,%2,%3 \n\t"     /* 64 = 32x32 multiply */ \
           "mov %0, %0, lsr #16 \n\t"    /* %0 = %0 >> 16 */ \
           "orr %0, %0, %1, lsl #16 \n\t"/* result = %0 OR (%1 << 16) */ \
           : "=&r"(low), "=&r" (high) \
           : "r"(X),"r"(Y)); \
        low; \
     })
     
    #define fixmul31(X,Y) \
     ({ \
        int32_t low; \
        int32_t high; \
        asm volatile (                   /* calculates: result = (X*Y)>>31 */ \
           "smull  %0,%1,%2,%3 \n\t"     /* 64 = 32x32 multiply */ \
           "mov %0, %0, lsr #31 \n\t"    /* %0 = %0 >> 31 */ \
           "orr %0, %0, %1, lsl #1 \n\t" /* result = %0 OR (%1 << 1) */ \
           : "=&r"(low), "=&r" (high) \
           : "r"(X),"r"(Y)); \
        low; \
     })
#elif defined(CPU_COLDFIRE)
    #define fixmul16(X,Y) \
    ({ \
        int32_t t1, t2; \
        asm volatile ( \
            "mac.l   %[x],%[y],%%acc0\n\t" /* multiply */ \
            "mulu.l  %[y],%[x]   \n\t"     /* get lower half, avoid emac stall */ \
            "movclr.l %%acc0,%[t1]   \n\t" /* get higher half */ \
            "moveq.l #15,%[t2]   \n\t" \
            "asl.l   %[t2],%[t1] \n\t"     /* hi <<= 15, plus one free */ \
            "moveq.l #16,%[t2]   \n\t" \
            "lsr.l   %[t2],%[x]  \n\t"     /* (unsigned)lo >>= 16 */ \
            "or.l    %[x],%[t1]  \n\t"     /* combine result */ \
            : /* outputs */ \
            [t1]"=&d"(t1), \
            [t2]"=&d"(t2) \
            : /* inputs */ \
            [x] "d" ((X)), \
            [y] "d" ((Y))); \
        t1; \
    })

    #define fixmul31(X,Y) \
    ({ \
       int32_t t; \
       asm volatile ( \
          "mac.l %[x], %[y], %%acc0\n\t"   /* multiply */ \
          "movclr.l %%acc0, %[t]\n\t"      /* get higher half as result */ \
          : [t] "=d" (t) \
          : [x] "r" ((X)), [y] "r" ((Y))); \
       t; \
    })
#else
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
#endif

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
    * whether (op - res - one) underflowed.
    */
    int32_t op, res, one;
    op = n;
    res = 0;
    /* "one" starts at the highest power of four <= than the argument. */
    one = 1 << 30; /* second-to-top bit set */
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
