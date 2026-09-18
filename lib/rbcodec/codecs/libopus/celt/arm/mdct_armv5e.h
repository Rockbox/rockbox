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

#ifndef MDCT_ARMv5E_H
#define MDCT_ARMv5E_H

/* Hand-written inner loops for the backward MDCT, the ARMv5E counterpart of
   arm/mdct_armv4.h.  The compiled loops on this architecture already use the
   32x16 multiply; what they miss is the packed halfword operand, the
   multiply-accumulate and the multi-register transfer.  See
   celt/arm/mdct_armv5e_asm.S.

   These are bit-exact with the C on this architecture, unlike the ARMv4
   kernels, which are more accurate than theirs.

   Building with OPUS_ARM_NO_MDCT_ASM selects the C loops instead, which is
   how the two are compared. */

#if defined(OPUS_ARM_INLINE_EDSP) && defined(FIXED_POINT) \
 && (ARM_ARCH >= 5) && !defined(OPUS_ARM_NO_MDCT_ASM)

#define OVERRIDE_MDCT_PREROT
#define OVERRIDE_MDCT_POSTROT
#define OVERRIDE_MDCT_MIRROR

#define mdct_prerot_opt  mdct_prerot_armv5e
#define mdct_postrot_opt mdct_postrot_armv5e

#ifdef OPUS_PFA
#define OVERRIDE_MDCT_POSTROT_PFA
#define mdct_postrot_pfa_opt mdct_postrot_pfa_armv5e
void mdct_postrot_pfa_armv5e(const kiss_fft_scalar *S, kiss_fft_scalar *yp0,
                             kiss_fft_scalar *yp1, const kiss_twiddle_scalar *t,
                             const opus_int16 *pmap, int N4);
#endif
#define mdct_mirror_opt  mdct_mirror_armv5e

/* step is a byte stride, so the caller scales by sizeof(kiss_fft_scalar). */
void mdct_prerot_armv5e(const kiss_fft_scalar *xp1,
                        const kiss_fft_scalar *xp2,
                        const kiss_twiddle_scalar *t,
                        const opus_int16 *bitrev,
                        kiss_fft_scalar *yp, int N4, int step);

void mdct_postrot_armv5e(kiss_fft_scalar *yp0, kiss_fft_scalar *yp1,
                         const kiss_twiddle_scalar *t, int N4, int count);

void mdct_mirror_armv5e(kiss_fft_scalar *xp1, kiss_fft_scalar *yp1,
                        const opus_val16 *wp1, const opus_val16 *wp2,
                        int count);

#endif

#endif /* MDCT_ARMv5E_H */
