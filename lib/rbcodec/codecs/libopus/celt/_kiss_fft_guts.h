/*Copyright (c) 2003-2004, Mark Borgerding

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.*/

#ifndef KISS_FFT_GUTS_H
#define KISS_FFT_GUTS_H

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

/* kiss_fft.h
   defines kiss_fft_scalar as either short or a float type
   and defines
   typedef struct { kiss_fft_scalar r; kiss_fft_scalar i; }kiss_fft_cpx; */
#include "kiss_fft.h"

/*
  Explanation of macros dealing with complex math:

   C_MUL(m,a,b)         : m = a*b
   C_FIXDIV( c , div )  : if a fixed point impl., c /= div. noop otherwise
   C_SUB( res, a,b)     : res = a - b
   C_SUBFROM( res , a)  : res -= a
   C_ADDTO( res , a)    : res += a
 * */
#ifdef FIXED_POINT
#include "arch.h"


#define SAMP_MAX 2147483647
#define TWID_MAX 32767
#define TRIG_UPSCALE 1

#define SAMP_MIN -SAMP_MAX


#   define S_MUL(a,b) MULT16_32_Q15(b, a)

#   define C_MUL(m,a,b) \
      do{ (m).r = SUB32(S_MUL((a).r,(b).r) , S_MUL((a).i,(b).i)); \
          (m).i = ADD32(S_MUL((a).r,(b).i) , S_MUL((a).i,(b).r)); }while(0)

#if defined (CPU_COLDFIRE)
#   define C_MULC(m,a,b) \
    { \
      asm volatile("move.l (%[bp]), %%d2;" \
                   "clr.l %%d3;" \
                   "move.w %%d2, %%d3;" \
                   "swap %%d3;" \
                   "clr.w %%d2;" \
                   "movem.l (%[ap]), %%d0-%%d1;" \
                   "mac.l %%d0, %%d2, %%acc0;" \
                   "mac.l %%d1, %%d3, %%acc0;" \
                   "mac.l %%d1, %%d2, %%acc1;" \
                   "msac.l %%d0, %%d3, %%acc1;" \
                   "movclr.l %%acc0, %[mr];" \
                   "movclr.l %%acc1, %[mi];" \
                   : [mr] "=r" ((m).r), [mi] "=r" ((m).i) \
                   : [ap] "a" (&(a)), [bp] "a" (&(b)) \
                   : "d0", "d1", "d2", "d3", "cc"); \
    }
#elif defined(CPU_ARM)
#if (ARM_ARCH < 5)


#   define C_MULC(m,a,b) \
    { \
      asm volatile( \
                   "ldm %[ap], {r0,r1}                  \n\t" \
                   "ldrsh r2, [%[bp], #0]                 \n\t" \
                   "ldrsh r3, [%[bp], #2]                 \n\t" \
                   \
                   "smull r4, %[mr], r0, r2               \n\t" \
                   "smlal r4, %[mr], r1, r3               \n\t" \
                   "mov   r4, r4, lsr #15                 \n\t" \
                   "orr   %[mr], r4, %[mr], lsl #17       \n\t" \
                   \
                   "smull r4, %[mi], r1, r2               \n\t" \
                   "rsb   r3, r3, #0                      \n\t" \
                   "smlal r4, %[mi], r0, r3               \n\t" \
                   "mov   r4, r4, lsr #15                 \n\t" \
                   "orr   %[mi], r4, %[mi], lsl #17       \n\t" \
                   : [mr] "=r" ((m).r), [mi] "=r" ((m).i) \
                   : [ap] "r" (&(a)), [bp] "r" (&(b)) \
                   : "r0", "r1", "r2", "r3", "r4"); \
}
#else
/*same as above but using armv5 packed multiplies*/
#   define C_MULC(m,a,b) \
    { \
      asm volatile( \
                   "ldm %[ap], {r0,r1}            \n\t" \
                   "ldr r2, [%[bp], #0] 	        \n\t" \
      				\
                   "smulwb r4, r0, r2               \n\t"  /*r4=a.r*b.r*/    \
                   "smlawt %[mr], r1, r2, r4        \n\t"  /*m.r=r4+a.i*b.i*/\
                   "mov   %[mr], %[mr], lsl #1      \n\t"  /*Q15 not Q16*/   \
                    \
                   "smulwb r1, r1, r2               \n\t"  /*r1=a.i*b.r*/    \
                   "smulwt r4, r0, r2               \n\t"  /*r4=a.r*b.i*/    \
                   "sub %[mi], r1, r4               \n\t" \
                   "mov   %[mi], %[mi], lsl #1      \n\t" \
                   : [mr] "=r" ((m).r), [mi] "=r" ((m).i) \
                   : [ap] "r" (&(a)), [bp] "r" (&(b)) \
                   : "r0", "r1", "r2", "r4"); \
}
#endif /*ARMv5 code*/
#else
#   define C_MULC(m,a,b) \
      do{ (m).r = ADD32(S_MUL((a).r,(b).r) , S_MUL((a).i,(b).i)); \
          (m).i = SUB32(S_MUL((a).i,(b).r) , S_MUL((a).r,(b).i)); }while(0)
#endif

#   define C_MUL4(m,a,b) \
      do{ (m).r = SHR32(SUB32(S_MUL((a).r,(b).r) , S_MUL((a).i,(b).i)),2); \
          (m).i = SHR32(ADD32(S_MUL((a).r,(b).i) , S_MUL((a).i,(b).r)),2); }while(0)

#   define C_MULBYSCALAR( c, s ) \
      do{ (c).r =  S_MUL( (c).r , s ) ;\
          (c).i =  S_MUL( (c).i , s ) ; }while(0)

#   define DIVSCALAR(x,k) \
        (x) = S_MUL(  x, (TWID_MAX-((k)>>1))/(k)+1 )

#   define C_FIXDIV(c,div) \
        do {    DIVSCALAR( (c).r , div);  \
                DIVSCALAR( (c).i  , div); }while (0)

#define  C_ADD( res, a,b)\
    do {(res).r=ADD32((a).r,(b).r);  (res).i=ADD32((a).i,(b).i); \
    }while(0)
#define  C_SUB( res, a,b)\
    do {(res).r=SUB32((a).r,(b).r);  (res).i=SUB32((a).i,(b).i); \
    }while(0)
#define C_ADDTO( res , a)\
    do {(res).r = ADD32((res).r, (a).r);  (res).i = ADD32((res).i,(a).i);\
    }while(0)

#define C_SUBFROM( res , a)\
    do {(res).r = ADD32((res).r,(a).r);  (res).i = SUB32((res).i,(a).i); \
    }while(0)

#else  /* not FIXED_POINT*/

#   define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) \
    do{ (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#define C_MULC(m,a,b) \
    do{ (m).r = (a).r*(b).r + (a).i*(b).i;\
        (m).i = (a).i*(b).r - (a).r*(b).i; }while(0)

#define C_MUL4(m,a,b) C_MUL(m,a,b)

#   define C_FIXDIV(c,div) /* NOOP */
#   define C_MULBYSCALAR( c, s ) \
    do{ (c).r *= (s);\
        (c).i *= (s); }while(0)
#endif

#ifndef CHECK_OVERFLOW_OP
#  define CHECK_OVERFLOW_OP(a,op,b) /* noop */
#endif

#ifndef C_ADD
#define  C_ADD( res, a,b)\
    do { \
            CHECK_OVERFLOW_OP((a).r,+,(b).r)\
            CHECK_OVERFLOW_OP((a).i,+,(b).i)\
            (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }while(0)
#define  C_SUB( res, a,b)\
    do { \
            CHECK_OVERFLOW_OP((a).r,-,(b).r)\
            CHECK_OVERFLOW_OP((a).i,-,(b).i)\
            (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }while(0)
#define C_ADDTO( res , a)\
    do { \
            CHECK_OVERFLOW_OP((res).r,+,(a).r)\
            CHECK_OVERFLOW_OP((res).i,+,(a).i)\
            (res).r += (a).r;  (res).i += (a).i;\
    }while(0)

#define C_SUBFROM( res , a)\
    do {\
            CHECK_OVERFLOW_OP((res).r,-,(a).r)\
            CHECK_OVERFLOW_OP((res).i,-,(a).i)\
            (res).r -= (a).r;  (res).i -= (a).i; \
    }while(0)
#endif /* C_ADD defined */

#ifdef FIXED_POINT
/*#  define KISS_FFT_COS(phase)  TRIG_UPSCALE*floor(MIN(32767,MAX(-32767,.5+32768 * cos (phase))))
#  define KISS_FFT_SIN(phase)  TRIG_UPSCALE*floor(MIN(32767,MAX(-32767,.5+32768 * sin (phase))))*/
#  define KISS_FFT_COS(phase)  floor(.5+TWID_MAX*cos (phase))
#  define KISS_FFT_SIN(phase)  floor(.5+TWID_MAX*sin (phase))
#  define HALF_OF(x) ((x)>>1)
#elif defined(USE_SIMD)
#  define KISS_FFT_COS(phase) _mm_set1_ps( cos(phase) )
#  define KISS_FFT_SIN(phase) _mm_set1_ps( sin(phase) )
#  define HALF_OF(x) ((x)*_mm_set1_ps(.5f))
#else
#  define KISS_FFT_COS(phase) (kiss_fft_scalar) cos(phase)
#  define KISS_FFT_SIN(phase) (kiss_fft_scalar) sin(phase)
#  define HALF_OF(x) ((x)*.5f)
#endif

#define  kf_cexp(x,phase) \
        do{ \
                (x)->r = KISS_FFT_COS(phase);\
                (x)->i = KISS_FFT_SIN(phase);\
        }while(0)

#define  kf_cexp2(x,phase) \
   do{ \
      (x)->r = TRIG_UPSCALE*celt_cos_norm((phase));\
      (x)->i = TRIG_UPSCALE*celt_cos_norm((phase)-32768);\
}while(0)

#endif /* KISS_FFT_GUTS_H */
