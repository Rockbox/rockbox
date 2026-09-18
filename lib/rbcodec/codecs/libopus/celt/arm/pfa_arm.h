/* ARM hooks for the Good-Thomas 15-point kernel.

   Copyright (c) 2026 Michael Giacomelli

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the conditions stated in
   celt/kiss_fft.c are met.
*/
#ifndef PFA_ARM_H
#define PFA_ARM_H

#if defined(FIXED_POINT) && defined(OPUS_PFA)

#if defined(OPUS_ARM_INLINE_EDSP)

#define OVERRIDE_PFA_FFT15
void pfa_fft15_armv5e(const kiss_fft_cpx *in, kiss_fft_cpx *out, int ostride);
#define PFA_FFT15(in, out, ostride) pfa_fft15_armv5e(in, out, ostride)

#elif defined(OPUS_ARM_INLINE_ASM)

#define OVERRIDE_PFA_FFT15
void pfa_fft15_armv4(const kiss_fft_cpx *in, kiss_fft_cpx *out, int ostride);
#define PFA_FFT15(in, out, ostride) pfa_fft15_armv4(in, out, ostride)

#endif

#endif /* FIXED_POINT && OPUS_PFA */

#endif /* PFA_ARM_H */
