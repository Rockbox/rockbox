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

#endif
