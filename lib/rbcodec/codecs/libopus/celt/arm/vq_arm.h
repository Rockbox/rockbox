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

#ifndef VQ_ARM_H
#define VQ_ARM_H

/* Hand-written loops for celt/vq.c, through the OVERRIDE_vq_exp_rotation1
   hook vq.c already provides.  Both architectures have one: the rotation
   chain writes X[i+stride] and reads it straight back as the next x1, so at
   stride 1 one of the two loads is redundant and one of the two stores is
   dead, and both kernels carry that value in a register instead.  Bit-exact;
   see celt/arm/exp_rotation1_armv5e_asm.S.

   Shares OPUS_ARM_NO_BANDS_ASM with arm/bands_arm.h as the escape hatch. */

#if defined(FIXED_POINT) && !defined(OPUS_ARM_NO_BANDS_ASM) \
 && defined(OPUS_ARM_INLINE_EDSP) && (ARM_ARCH == 5)

#define OVERRIDE_vq_exp_rotation1
void exp_rotation1_armv5e(celt_norm *X, int len, int stride, opus_val16 c,
                          opus_val16 s);
#define exp_rotation1(X, len, stride, c, s) \
   exp_rotation1_armv5e((X), (len), (stride), (c), (s))

/* The scaling loop at the end of normalise_residual.  ARMv5E only: it wants
   smulwb.  See celt/arm/normres_armv5e_asm.S. */
#define OVERRIDE_NORMRES_SCALE
void normres_scale_armv5e(celt_norm *X, const int *iy, int N, int g,
                          int shift);
#define NORMRES_SCALE normres_scale_armv5e

#endif

#endif /* VQ_ARM_H */
