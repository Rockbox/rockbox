/* Copyright (c) 2025 Xiph.Org Foundation and contributors
   Copyright (c) 2026 Michael Giacomelli

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
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef COMB_FILTER_ARM_H
#define COMB_FILTER_ARM_H

/* Hand-written comb_filter_const for both ARM generations, reached through
   the OVERRIDE_COMB_FILTER_CONST hook celt.h already provides.  Both are
   bit-exact with the C they replace; see the assembly for why the reordered
   accumulation is exact rather than merely close.

   Neither implements the CUSTOM_MODES scalar tail, so the override stands
   aside when that is defined.  Build with OPUS_ARM_NO_COMB_ASM to select the
   C version instead, which is how the two are compared. */

#if defined(FIXED_POINT) && !defined(CUSTOM_MODES) \
 && !defined(OPUS_ARM_NO_COMB_ASM)

# if defined(OPUS_ARM_INLINE_ASM) && (ARM_ARCH == 4)

#  define OVERRIDE_COMB_FILTER_CONST
void comb_filter_const_armv4(opus_val32 *y, opus_val32 *x, int T, int N,
                             opus_val16 g10, opus_val16 g11, opus_val16 g12);
#  define comb_filter_const(y, x, T, N, g10, g11, g12, arch) \
     ((void)(arch), comb_filter_const_armv4((y), (x), (T), (N), \
                                            (g10), (g11), (g12)))

/* The SIG_SAT guard celt_synthesis applies to the IMDCT output before the
   postfilter reads it; lives with the comb filter it protects.  Replaces the
   static inline celt_sat in celt_decoder.c. */
#  define OVERRIDE_CELT_SAT
void celt_sat_armv4(celt_sig *x, int n);
#  define celt_sat(x, n) celt_sat_armv4((x), (n))

# elif defined(OPUS_ARM_INLINE_EDSP) && (ARM_ARCH == 5)

#  define OVERRIDE_COMB_FILTER_CONST
void comb_filter_const_armv5e(opus_val32 *y, opus_val32 *x, int T, int N,
                              opus_val16 g10, opus_val16 g11, opus_val16 g12);
#  define comb_filter_const(y, x, T, N, g10, g11, g12, arch) \
     ((void)(arch), comb_filter_const_armv5e((y), (x), (T), (N), \
                                             (g10), (g11), (g12)))

#  define OVERRIDE_CELT_SAT
void celt_sat_armv5e(celt_sig *x, int n);
#  define celt_sat(x, n) celt_sat_armv5e((x), (n))

# endif

#endif

#endif /* COMB_FILTER_ARM_H */
