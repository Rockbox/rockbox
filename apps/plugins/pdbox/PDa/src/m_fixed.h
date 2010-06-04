#ifndef __M_FIXED_H__
#define __M_FIXED_H__

#ifdef ROCKBOX
#include "plugin.h"
#endif

typedef int t_sample;

#define t_fixed int
#define fix1 18  /* (18) number of bits after comma */


#define fixfac ((float)(1<<fix1))  /* float factor (for scaling ftofix ..) */


/* fixed point multiplication and division */

#ifdef ROCKBOX
#if defined(CPU_ARM)
#define mult(A,B) \
     ({ \
        t_fixed lo; \
        t_fixed hi; \
        asm volatile ( \
           "smull %[lo], %[hi], %[x], %[y] \n\t"   /* multiply */ \
           "mov   %[lo], %[lo], lsr %[shr] \n\t"   /* lo >>= fix1 */ \
           "orr   %[lo], %[lo], %[hi], lsl %[shl]" /* lo |= (hi << (32-fix1)) */ \
           : [lo]"=&r"(lo), [hi]"=&r"(hi) \
           : [x]"r"(A), [y]"r"(B), [shr]"r"(fix1), [shl]"r"(32-fix1)); \
        lo; \
     })
#define idiv(a,b) ((((long long) (a) )<<fix1)/(long long) (b) )
#elif defined(CPU_COLDFIRE)
#define mult(a,b) mult_cf((a),(b))
static inline t_fixed mult_cf(t_fixed x, t_fixed y)
{
    t_fixed t1, t2;
    asm volatile (
       "mac.l   %[x],%[y],%%acc0 \n" /* multiply */
       "mulu.l  %[y],%[x]        \n" /* get low half, avoid emac stall */
       "movclr.l %%acc0,%[t1]    \n" /* get higher half */
       "asl.l   %[shl],%[t1]     \n" /* hi <<= 13, plus one free */
       "lsr.l   %[shr],%[x]      \n" /* (unsigned)lo >>= 18 */
       "or.l    %[x],%[t1]       \n" /* combine result */
       : [t1]"=&d"(t1), [t2]"=&d"(t2), [x]"+d"(x)
       : [y]"d"(y), [shl]"d"(31-fix1), [shr]"d"(fix1));
    return t1;
}
#define idiv(a,b) ((((long long) (a) )<<fix1)/(long long) (b) )
#endif /* CPU_... */
#else /* ROCKBOX */
#define mult(a,b) (long long)(((long long) (a) * (long long) (b))>>fix1)
#define idiv(a,b) ((((long long) (a) )<<fix1)/(long long) (b) )
#endif /* ROCKBOX */

/* conversion macros */

#define itofix(a) ((a) << fix1)
#define ftofix(a) ((t_fixed)( (a) *(double)fixfac + 0.5))

#define fixtof(a) ((double) (a) * 1./(fixfac-0.5))
#define fixtoi(a) ((a) >>fix1)


/* Not working !! */

#define fnum(a) ( (a) >>(fix1-16))
#define ffrac(a) (0)


/* mapping of fft functions */

#ifdef FIXEDPOINT
#define mayer_realifft imayer_realifft
#define mayer_realfft imayer_realfft
#define mayer_fft imayer_fft
#define mayer_ifft imayer_ifft
#endif

#ifdef FIXEDPOINT
#define SCALE16(x) (x>>(fix1-15))
#define SCALE32(x) (x<<(32-fix1))
#define INVSCALE16(x) (x<<8)
#else
#define SCALE16(x) (32767.*x)
#define SCALE32(x) (2147483648.*x)
#define INVSCALE16(x) ((float)3.051850e-05*x)
#endif


#endif

