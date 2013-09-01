/* Copyright (C) 2013 Nils Wallménius */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FIXED_CF_H
#define FIXED_CF_H

#undef MULT16_32_Q15
static inline int32_t MULT16_32_Q15_cf(int32_t a, int32_t b)
{
  int32_t r;
  asm volatile ("mac.l %[a], %[b], %%acc0;"
                "movclr.l %%acc0, %[r];"
                : [r] "=r" (r)
                : [a] "r" (a<<16), [b] "r" (b)
                : "cc");
  return r;
}
#define MULT16_32_Q15(a, b) (MULT16_32_Q15_cf(a, b))

#undef MULT32_32_Q31
static inline int32_t MULT32_32_Q31_cf(int32_t a, int32_t b)
{
  int32_t r;
  asm volatile ("mac.l %[a], %[b], %%acc0;"
                "movclr.l %%acc0, %[r];"
                : [r] "=r" (r)
                : [a] "r" (a), [b] "r" (b)
                : "cc");
  return r;
}
#define MULT32_32_Q31(a, b) (MULT32_32_Q31_cf(a, b))

#define OVERRIDE_COMB_FILTER_CONST
static inline void comb_filter_const(opus_val32 *y, opus_val32 *x, int T, int N,
    opus_val16 g10, opus_val16 g11, opus_val16 g12)
{
  opus_val32 x0, x1, x2, x3, x4;
  int i;
  x4 = x[-T-2];
  x3 = x[-T-1];
  x2 = x[-T];
  x1 = x[-T+1];
  for (i=0;i<N;i++)
  {
    x0=x[i-T+2];
    asm volatile("mac.l %[g10], %[x2], %%acc0;"
                 /* just doing straight MACs here is faster than pre-adding */
                 "mac.l %[g11], %[x1], %%acc0;"
                 "mac.l %[g11], %[x3], %%acc0;"
                 "mac.l %[g12], %[x0], %%acc0;"
                 "mac.l %[g12], %[x4], %%acc0;"
                 "move.l %[x3], %[x4];"
                 "move.l %[x2], %[x3];"
                 "move.l %[x1], %[x2];"
                 "move.l %[x0], %[x1];"
                 "movclr.l %%acc0, %[x0];"
                 : [x0] "+r" (x0), [x1] "+r" (x1), [x2] "+r" (x2),
                   [x3] "+r" (x3), [x4] "+r" (x4)
                 : [g10] "r" (g10 << 16), [g11] "r" (g11 << 16),
                   [g12] "r" (g12 << 16)
                 : "cc");
    y[i] = x[i] + x0;
  }
}

#endif
