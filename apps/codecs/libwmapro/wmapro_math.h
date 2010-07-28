#ifndef _WMAPRO_MATH_H_
#define _WMAPRO_MATH_H_

#include <inttypes.h>

/* rockbox: not used
#define fixtof16(x)       (float)((float)(x) / (float)(1 << 16))
#define fixtof31(x)       (float)((float)(x) / (float)(1 << 31))
#define ftofix16(x)       ((int32_t)((x) * (float)(1 << 16) + ((x) < 0 ? -0.5:0.5)))
#define ftofix31(x)       ((int32_t)((x) * (float)(1 << 31) + ((x) < 0 ? -0.5:0.5)))
*/

#if defined(CPU_ARM)
    /* Calculates: result = (X*Y)>>Z */
    #define fixmulshift(X,Y,Z) \
    ({ \
        int32_t lo; \
        int32_t hi; \
        asm volatile ( \
            "smull %[lo], %[hi], %[x], %[y] \n\t"   /* multiply */ \
            "mov   %[lo], %[lo], lsr %[shr] \n\t"   /* lo >>= Z */ \
            "orr   %[lo], %[lo], %[hi], lsl %[shl]" /* lo |= (hi << (32-Z)) */ \
            : [lo]"=&r"(lo), [hi]"=&r"(hi) \
            : [x]"r"(X), [y]"r"(Y), [shr]"r"(Z), [shl]"r"(32-Z)); \
        lo; \
    })
     
    /* Calculates: result = (X*Y)>>16 */
    #define fixmul16(X,Y) \
     ({ \
        int32_t lo; \
        int32_t hi; \
        asm volatile ( \
           "smull %[lo], %[hi], %[x], %[y] \n\t" /* multiply */ \
           "mov   %[lo], %[lo], lsr #16    \n\t" /* lo >>= 16 */ \
           "orr   %[lo], %[lo], %[hi], lsl #16"  /* lo |= (hi << 16) */ \
           : [lo]"=&r"(lo), [hi]"=&r"(hi) \
           : [x]"r"(X), [y]"r"(Y)); \
        lo; \
     })
     
    /* Calculates: result = (X*Y)>>24 */
    #define fixmul24(X,Y) \
     ({ \
        int32_t lo; \
        int32_t hi; \
        asm volatile ( \
           "smull %[lo], %[hi], %[x], %[y] \n\t" /* multiply */ \
           "mov   %[lo], %[lo], lsr #24    \n\t" /* lo >>= 24 */ \
           "orr   %[lo], %[lo], %[hi], lsl #8"   /* lo |= (hi << 8) */ \
           : [lo]"=&r"(lo), [hi]"=&r"(hi) \
           : [x]"r"(X), [y]"r"(Y)); \
        lo; \
     })
     
    /* Calculates: result = (X*Y)>>31 */
    #define fixmul31(X,Y) \
     ({ \
        int32_t lo; \
        int32_t hi; \
        asm volatile ( \
           "smull %[lo], %[hi], %[x], %[y] \n\t" /* multiply */ \
           "mov   %[lo], %[lo], lsr #31    \n\t" /* lo >>= 31 */ \
           "orr   %[lo], %[lo], %[hi], lsl #1"   /* lo |= (hi << 1) */ \
           : [lo]"=&r"(lo), [hi]"=&r"(hi) \
           : [x]"r"(X), [y]"r"(Y)); \
        lo; \
     })
#elif defined(CPU_COLDFIRE)
    /* Calculates: result = (X*Y)>>Z */
    #define fixmulshift(X,Y,Z) \
    ({ \
        int32_t t1; \
        int32_t t2; \
        asm volatile ( \
            "mac.l   %[x],%[y],%%acc0\n\t" /* multiply */ \
            "mulu.l  %[y],%[x]       \n\t" /* get lower half, avoid emac stall */ \
            "movclr.l %%acc0,%[t1]   \n\t" /* get higher half */ \
            "moveq.l #31,%[t2]       \n\t" \
            "sub.l   %[sh],%[t2]     \n\t" /* t2 = 31 - shift */ \
            "ble.s   1f              \n\t" \
            "asl.l   %[t2],%[t1]     \n\t" /* hi <<= 31 - shift */ \
            "lsr.l   %[sh],%[x]      \n\t" /* (unsigned)lo >>= shift */ \
            "or.l    %[x],%[t1]      \n\t" /* combine result */ \
            "bra.s   2f              \n\t" \
         "1:                         \n\t" \
            "neg.l   %[t2]           \n\t" /* t2 = shift - 31 */ \
            "asr.l   %[t2],%[t1]     \n\t" /* hi >>= t2 */ \
         "2:                         \n" \
        : [t1]"=&d"(t1), [t2]"=&d"(t2) \
        : [x] "d"((X)), [y] "d"((Y)), [sh]"d"((Z))); \
        t1; \
    })

    /* Calculates: result = (X*Y)>>16 */
    #define fixmul16(X,Y) \
    ({ \
        int32_t t, x = (X); \
        asm volatile ( \
            "mac.l    %[x],%[y],%%acc0\n\t" /* multiply */ \
            "mulu.l   %[y],%[x]       \n\t" /* get lower half, avoid emac stall */ \
            "movclr.l %%acc0,%[t]     \n\t" /* get higher half */ \
            "lsr.l    #1,%[t]         \n\t" /* hi >>= 1 to compensate emac shift */ \
            "move.w   %[t],%[x]       \n\t" /* combine halfwords */\
            "swap     %[x]            \n\t" \
            : [t]"=&d"(t), [x] "+d" (x) \
            : [y] "d" ((Y))); \
        x; \
    })
    
    /* Calculates: result = (X*Y)>>24 */
    #define fixmul24(X,Y) \
    ({ \
        int32_t t1, t2; \
        asm volatile ( \
            "mac.l    %[x],%[y],%%acc0 \n\t" /* multiply */ \
            "move.l   %%accext01, %[t1]\n\t" /* get lower 8 bits */ \
            "movclr.l %%acc0,%[t2]     \n\t" /* get higher 24 bits */ \
            "asl.l    #7,%[t2]         \n\t" /* hi <<= 7, plus one free */ \
            "move.b   %[t1],%[t2]      \n\t" /* combine result */ \
            : [t1]"=&d"(t1), [t2]"=&d"(t2) \
            : [x] "d" ((X)), [y] "d" ((Y))); \
        t2; \
    })

    /* Calculates: result = (X*Y)>>32 */
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
    static inline int32_t fixmulshift(int32_t x, int32_t y, int shamt)
    {
        int64_t temp;
        temp = x;
        temp *= y;
    
        temp >>= shamt;
    
        return (int32_t)temp;
    }
    
    static inline int32_t fixmul31(int32_t x, int32_t y)
    {
        int64_t temp;
        temp = x;
        temp *= y;
    
        temp >>= 31;
    
        return (int32_t)temp;
    }
    
    static inline int32_t fixmul24(int32_t x, int32_t y)
    {
        int64_t temp;
        temp = x;
        temp *= y;
    
        temp >>= 24;
    
        return (int32_t)temp;
    }
    
    static inline int32_t fixmul16(int32_t x, int32_t y)
    {
        int64_t temp;
        temp = x;
        temp *= y;
    
        temp >>= 16;
    
        return (int32_t)temp;
    }
#endif /* CPU_COLDFIRE, CPU_ARM */

#ifdef CPU_COLDFIRE
static inline void vector_fixmul_window(int32_t *dst, const int32_t *src0, 
                                   const int32_t *src1, const int32_t *win, 
                                   int len)
{
    int i, j;
    dst += len;
    win += len;
    src0+= len;
        for(i=-len, j=len-1; i<0; i++, j--) {
        int32_t s0 = src0[i];
        int32_t s1 = src1[j];
        int32_t wi = -win[i];
        int32_t wj = -win[j];

        asm volatile ("mac.l    %[s0], %[wj], %%acc0\n\t"
                      "msac.l   %[s1], %[wi], %%acc0\n\t"
                      "mac.l    %[s0], %[wi], %%acc1\n\t"
                      "mac.l    %[s1], %[wj], %%acc1\n\t"
                      "movclr.l %%acc0, %[s0]\n\t"
                      "move.l   %[s0], (%[dst_i])\n\t"
                      "movclr.l %%acc1, %[s0]\n\t"
                      "move.l   %[s0], (%[dst_j])\n\t"
                      : [s0] "+r" (s0) /* this register is clobbered so specify it as an input */
                      : [dst_i] "a" (&dst[i]), [dst_j] "a" (&dst[j]),
                        [s1] "r" (s1), [wi] "r" (wi), [wj] "r" (wj)
                      : "cc", "memory");
    }
}
#else
static inline void vector_fixmul_window(int32_t *dst, const int32_t *src0, 
                                   const int32_t *src1, const int32_t *win, 
                                   int len)
{
    int i, j;
    dst += len;
    win += len;
    src0+= len;
    for(i=-len, j=len-1; i<0; i++, j--) {
        int32_t s0 = src0[i]; /* s0 = src0[      0 ... len-1] */
        int32_t s1 = src1[j]; /* s1 = src1[2*len-1 ... len]   */
        int32_t wi = -win[i]; /* wi = -win[      0 ... len-1] */
        int32_t wj = -win[j]; /* wj = -win[2*len-1 ... len]   */
        dst[i] = fixmul31(s0, wj) - fixmul31(s1, wi); /* dst[      0 ... len-1] */
        dst[j] = fixmul31(s0, wi) + fixmul31(s1, wj); /* dst[2*len-1 ... len]   */
    }
}
#endif

static inline void vector_fixmul_scalar(int32_t *dst, const int32_t *src, 
                                        int32_t mul, int len)
{
    /* len is _always_ a multiple of 4, because len is the difference of sfb's
     * which themselves are always a multiple of 4. */
    int i;
    for (i=0; i<len; i+=4) {
        dst[i  ] = fixmul24(src[i  ], mul);
        dst[i+1] = fixmul24(src[i+1], mul);
        dst[i+2] = fixmul24(src[i+2], mul);
        dst[i+3] = fixmul24(src[i+3], mul);
    }
}

static inline int av_clip(int a, int amin, int amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}
#endif /* _WMAPRO_MATH_H_ */
