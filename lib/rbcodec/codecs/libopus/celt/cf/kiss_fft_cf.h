/*Copyright (c) 2013, Xiph.Org Foundation and contributors.

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

#ifndef KISS_FFT_CF_H
#define KISS_FFT_CF_H

#if !defined(KISS_FFT_GUTS_H)
#error "This file should only be included from _kiss_fft_guts.h"
#endif

#ifdef FIXED_POINT

#undef C_MUL
#define C_MUL(m,a,b) \
    { \
      asm volatile("move.l (%[bp]), %%d2;" \
                   "clr.l %%d3;" \
                   "move.w %%d2, %%d3;" \
                   "swap %%d3;" \
                   "clr.w %%d2;" \
                   "movem.l (%[ap]), %%d0-%%d1;" \
                   "mac.l %%d0, %%d2, %%acc0;" \
                   "msac.l %%d1, %%d3, %%acc0;" \
                   "mac.l %%d1, %%d2, %%acc1;" \
                   "mac.l %%d0, %%d3, %%acc1;" \
                   "movclr.l %%acc0, %[mr];" \
                   "movclr.l %%acc1, %[mi];" \
                   : [mr] "=r" ((m).r), [mi] "=r" ((m).i) \
                   : [ap] "a" (&(a)), [bp] "a" (&(b)) \
                   : "d0", "d1", "d2", "d3", "cc"); \
    }


#undef C_MULC
#define C_MULC(m,a,b) \
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

#endif /* FIXED_POINT */

#endif /* KISS_FFT_CF_H */
