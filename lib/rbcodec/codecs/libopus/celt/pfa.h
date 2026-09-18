/* Good-Thomas (prime factor) FFT for the CELT backward MDCT.
   See pfa.c for what it is and why.

   Copyright (c) 2026 Michael Giacomelli

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the conditions stated in
   celt/kiss_fft.c are met.
*/
#ifndef PFA_H
#define PFA_H

#include "kiss_fft.h"

#ifdef OPUS_PFA

void pfa_fft15_c(const kiss_fft_cpx *in, kiss_fft_cpx *out, int ostride);

#if defined(OPUS_ARM_ASM) && !defined(OPUS_ARM_NO_PFA_ASM)
# include "arm/pfa_arm.h"
#endif

#ifndef OVERRIDE_PFA_FFT15
# define PFA_FFT15(in, out, ostride) pfa_fft15_c(in, out, ostride)
#endif

/* Pass 1 and pass 2.  fin holds the gathered spectrum and fout receives the
   transform; they must be different buffers, because pass 1 reads fifteen
   contiguous points and writes them strided.  nfft is 60, 120, 240 or 480.

   The result is left in Good-Thomas order: X[k] sits at
   fout[(nfft/15)*(k % 15) + (k % (nfft/15))].  The post-rotation reads it
   through that index, which costs two running counters and no table. */
void opus_pfa_impl(const kiss_fft_cpx *fin, kiss_fft_cpx *fout, int nfft);

/* True for the lengths opus_pfa_impl handles.  Every 48 kHz CELT transform
   qualifies; the test exists so a custom mode falls back to kiss_fft. */
#define OPUS_PFA_SIZE(n) \
   ((n) == 60 || (n) == 120 || (n) == 240 || (n) == 480)

#endif /* OPUS_PFA */

#endif /* PFA_H */
