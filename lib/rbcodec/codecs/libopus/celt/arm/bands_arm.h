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

#ifndef BANDS_ARM_H
#define BANDS_ARM_H

/* Hand-written inner loops for celt/bands.c on both ARM generations.  See
   celt/arm/denorm_armv4_asm.S and denorm_armv5e_asm.S.  Both are bit-exact
   with the C they replace.

   Build with OPUS_ARM_NO_BANDS_ASM to select the C loops instead, which is
   how the two are compared. */

#if defined(FIXED_POINT) && !defined(OPUS_ARM_NO_BANDS_ASM)

# if defined(OPUS_ARM_INLINE_ASM) && (ARM_ARCH == 4)

#  define OVERRIDE_DENORM_BAND
void denorm_band_armv4(celt_sig *f, const celt_norm *x, int n, int g,
                       int shift);
#  define DENORM_BAND denorm_band_armv4

# elif defined(OPUS_ARM_INLINE_EDSP) && (ARM_ARCH == 5)

#  define OVERRIDE_DENORM_BAND
void denorm_band_armv5e(celt_sig *f, const celt_norm *x, int n, int g,
                        int shift);
#  define DENORM_BAND denorm_band_armv5e

/* ARMv5E only: on ARMv4 haar1 is multiply-bound and gains only the hoisted
   rounding add.  bands.h has already declared haar1 by this point, so the
   macro renames the calls and the guarded C definition drops out. */
#  define OVERRIDE_haar1
void haar1_armv5e(celt_norm *X, int N0, int stride);
#  define haar1(X, N0, stride) haar1_armv5e((X), (N0), (stride))

# endif

#endif

#endif /* BANDS_ARM_H */
