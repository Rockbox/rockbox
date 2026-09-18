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

#ifndef KISS_FFT_ARMv5E_H
#define KISS_FFT_ARMv5E_H

#if !defined(KISS_FFT_GUTS_H)
#error "This file should only be included from _kiss_fft_guts.h"
#endif

#ifdef FIXED_POINT

#if defined(__thumb__)||defined(__thumb2__)
#define LDRD_CONS "Q"
#else
#define LDRD_CONS "Uq"
#endif

#undef C_MUL
#define C_MUL(m,a,b) \
    do{ \
        int mr1__; \
        int mr2__; \
        int mi__; \
        long long aval__; \
        int bval__; \
        __asm__( \
            "#C_MUL\n\t" \
            "ldrd %[aval], %H[aval], %[ap]\n\t" \
            "ldr %[bval], %[bp]\n\t" \
            "smulwb %[mi], %H[aval], %[bval]\n\t" \
            "smulwb %[mr1], %[aval], %[bval]\n\t" \
            "smulwt %[mr2], %H[aval], %[bval]\n\t" \
            "smlawt %[mi], %[aval], %[bval], %[mi]\n\t" \
            : [mr1]"=r"(mr1__), [mr2]"=r"(mr2__), [mi]"=r"(mi__), \
              [aval]"=&r"(aval__), [bval]"=r"(bval__) \
            : [ap]LDRD_CONS(a), [bp]"m"(b) \
        ); \
        (m).r = SHL32(SUB32(mr1__, mr2__), 1); \
        (m).i = SHL32(mi__, 1); \
    } \
    while(0)

#undef C_MUL4
#define C_MUL4(m,a,b) \
    do{ \
        int mr1__; \
        int mr2__; \
        int mi__; \
        long long aval__; \
        int bval__; \
        __asm__( \
            "#C_MUL4\n\t" \
            "ldrd %[aval], %H[aval], %[ap]\n\t" \
            "ldr %[bval], %[bp]\n\t" \
            "smulwb %[mi], %H[aval], %[bval]\n\t" \
            "smulwb %[mr1], %[aval], %[bval]\n\t" \
            "smulwt %[mr2], %H[aval], %[bval]\n\t" \
            "smlawt %[mi], %[aval], %[bval], %[mi]\n\t" \
            : [mr1]"=r"(mr1__), [mr2]"=r"(mr2__), [mi]"=r"(mi__), \
              [aval]"=&r"(aval__), [bval]"=r"(bval__) \
            : [ap]LDRD_CONS(a), [bp]"m"(b) \
        ); \
        (m).r = SHR32(SUB32(mr1__, mr2__), 1); \
        (m).i = SHR32(mi__, 1); \
    } \
    while(0)

#undef C_MULC
#define C_MULC(m,a,b) \
    do{ \
        int mr__; \
        int mi1__; \
        int mi2__; \
        long long aval__; \
        int bval__; \
        __asm__( \
            "#C_MULC\n\t" \
            "ldrd %[aval], %H[aval], %[ap]\n\t" \
            "ldr %[bval], %[bp]\n\t" \
            "smulwb %[mr], %[aval], %[bval]\n\t" \
            "smulwb %[mi1], %H[aval], %[bval]\n\t" \
            "smulwt %[mi2], %[aval], %[bval]\n\t" \
            "smlawt %[mr], %H[aval], %[bval], %[mr]\n\t" \
            : [mr]"=r"(mr__), [mi1]"=r"(mi1__), [mi2]"=r"(mi2__), \
              [aval]"=&r"(aval__), [bval]"=r"(bval__) \
            : [ap]LDRD_CONS(a), [bp]"m"(b) \
        ); \
        (m).r = SHL32(mr__, 1); \
        (m).i = SHL32(SUB32(mi1__, mi2__), 1); \
    } \
    while(0)

/* Assembly radix-3, radix-4 and radix-5 passes, in kiss_fft_armv5e_asm.S.
   The C_MUL above is already as tight as the core allows; what the assembly
   recovers is the bookkeeping gcc cannot keep resident.  Measured over eight
   packets of stereo decode, 16.0% of the instructions executed in the
   compiled radix-4 pass are stack traffic, 11.4% of radix-3 and 23.0% of
   radix-5, almost all of it output pointers spilled and reloaded around the
   complex multiplies.  Radix-2 is left in C: it needs no twiddle pointers
   and spills only 4.7%, so there is nothing to win.  The kernels take
   st->twiddles rather than st, so the assembly needs no knowledge of the
   layout of kiss_fft_state. */
#ifndef OPUS_ARM_NO_FFT_ASM
#define OVERRIDE_kf_bfly3
#define OVERRIDE_kf_bfly4
#define OVERRIDE_kf_bfly5

void kf_bfly3_armv5e(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
                     int fstride, int m, int N, int mm);
void kf_bfly4_armv5e(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
                     int fstride, int m, int N, int mm);
void kf_bfly5_armv5e(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
                     int fstride, int m, int N, int mm);

#define kf_bfly3(Fout, fstride, st, m, N, mm)    kf_bfly3_armv5e((Fout), (st)->twiddles, (int)(fstride), (m), (N), (mm))
#define kf_bfly4(Fout, fstride, st, m, N, mm)    kf_bfly4_armv5e((Fout), (st)->twiddles, (int)(fstride), (m), (N), (mm))
#define kf_bfly5(Fout, fstride, st, m, N, mm)    kf_bfly5_armv5e((Fout), (st)->twiddles, (int)(fstride), (m), (N), (mm))
#endif /* OPUS_ARM_NO_FFT_ASM */

#endif /* FIXED_POINT */

#endif /* KISS_FFT_GUTS_H */
