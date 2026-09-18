/* Good-Thomas (prime factor) FFT for the CELT backward MDCT.
 *
 * Copyright (c) 2007-2008 CSIRO
 * Copyright (c) 2007-2008 Xiph.Org Foundation
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/kiss_fft.c are met.
 *
 * Why this exists
 * ---------------
 * Every FFT length a 48 kHz Opus stream uses is 15 times a power of two:
 * 60, 120, 240, 480.  15 and 2^k are co-prime, so the prime factor algorithm
 * applies, and its whole point is that co-prime factors need no twiddle
 * factors between them -- only an index permutation.
 *
 * That matters because in the mixed-radix decomposition kiss_fft picks
 * (5, 3, 4, 2, 4 for N=480) the inter-stage twiddles are 4,020 of the 5,540
 * real multiplies, 73% of the total.  Splitting 480 as 15 x 32 deletes that
 * class of work rather than making it cheaper: 2,780 multiplies, half as
 * many.  Multiplies are 28.4% of ARMv4 decode, so this is the largest single
 * item the profile still offers on that CPU.
 *
 * Structure, for N = 15*M:
 *
 *   gather     x[n] -> u[15*n2 + 3*m2 + m1], a table the pre-rotation reads
 *              in place of the bit-reversal it read before, so it is free
 *   pass 1     M independent 15-point DFTs, contiguous in, strided M out
 *   pass 2     15 independent M-point DFTs, contiguous, which is exactly
 *              what the existing kf_bfly4/kf_bfly2 kernels already do
 *   scatter    X[k] = B[k mod 15][k mod M], absorbed into the post-rotation
 *              as two running counters, no table
 *
 * Buffers.  Pass 1 cannot be in place (it reads 15 contiguous and writes 15
 * strided), so it needs somewhere to land.  clt_mdct_backward already
 * destroys its input -- celt_synthesis copies the spectrum out first for
 * exactly that reason -- and that buffer is N2 scalars, which is N4 complex,
 * exactly the size wanted.  So the common stride==1 case costs no memory at
 * all.  Short blocks arrive with stride>1 and a strided input that cannot be
 * reused, but there N4 is only 60, so a 480-byte local covers it.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "kiss_fft.h"
#include "_kiss_fft_guts.h"
#include "mathops.h"
#include "stack_alloc.h"
#include "pfa.h"

#ifdef OPUS_PFA

/* The same constants the mixed-radix butterflies use, so the arithmetic
   below is the arithmetic that was already shipping, only reassociated.
   epi3 is -sin(2pi/3); ya/yb are the radix-5 sines; yc is sqrt(5)/4, which
   collapses the four radix-5 cosine products into one multiply because
   cos(2pi/5)+cos(4pi/5) is exactly -1/2 and the Q15 constants honour that
   identity exactly (10126 - 26510 == -16384). */
#define PFA_EPI3   (-28378)
#define PFA_YAI    (-31164)
#define PFA_YBI    (-19261)
#define PFA_YC     ( 18318)

/* PFA_KEEP_C keeps the reference compiled alongside an override so a
   harness can score one against the other on target. */
#if !defined(OVERRIDE_PFA_FFT15) || defined(PFA_KEEP_C)
/* One 15-point DFT, itself Good-Thomas over 3 and 5 so it too carries no
   twiddles: five 3-point DFTs, then three 5-point DFTs.  40 real multiplies.
   Input is five contiguous triples, laid out by the gather table.  Output is
   scattered by ostride complex elements; within one 5-point DFT the
   destinations run at a stride of 3, which is what lets the kernel walk them
   with a single post-indexed store. */
void pfa_fft15_c(const kiss_fft_cpx *in, kiss_fft_cpx *out, int ostride)
{
   kiss_fft_cpx v[15];
   int g, k1;

   /* Five 3-point DFTs.  Results land strided by 5 so each 5-point DFT
      below reads a contiguous run. */
   for (g=0;g<5;g++)
   {
      kiss_fft_cpx x0, s3, s0, xm;
      x0 = in[3*g];
      s3.r = ADD32_ovflw(in[3*g+1].r, in[3*g+2].r);
      s3.i = ADD32_ovflw(in[3*g+1].i, in[3*g+2].i);
      s0.r = SUB32_ovflw(in[3*g+1].r, in[3*g+2].r);
      s0.i = SUB32_ovflw(in[3*g+1].i, in[3*g+2].i);
      xm.r = SUB32_ovflw(x0.r, HALF_OF(s3.r));
      xm.i = SUB32_ovflw(x0.i, HALF_OF(s3.i));
      s0.r = S_MUL(s0.r, PFA_EPI3);
      s0.i = S_MUL(s0.i, PFA_EPI3);
      v[g].r      = ADD32_ovflw(x0.r, s3.r);
      v[g].i      = ADD32_ovflw(x0.i, s3.i);
      v[5+g].r    = SUB32_ovflw(xm.r, s0.i);
      v[5+g].i    = ADD32_ovflw(xm.i, s0.r);
      v[10+g].r   = ADD32_ovflw(xm.r, s0.i);
      v[10+g].i   = SUB32_ovflw(xm.i, s0.r);
   }

   /* Three 5-point DFTs.  Output k2 of DFT k1 belongs at 15-point index
      j = (10*k1 + 6*k2) mod 15; walking j from k1 in steps of 3 visits them
      in the order k2 = (k1 + 3*t) mod 5, which is the permutation applied
      when storing. */
   for (k1=0;k1<3;k1++)
   {
      const kiss_fft_cpx *x = v + 5*k1;
      kiss_fft_cpx X[5], s7, s8, s9, s10, s3, s4, s5, s11, s6, s12;
      int t;

      s7.r  = ADD32_ovflw(x[1].r, x[4].r);  s7.i  = ADD32_ovflw(x[1].i, x[4].i);
      s10.r = SUB32_ovflw(x[1].r, x[4].r);  s10.i = SUB32_ovflw(x[1].i, x[4].i);
      s8.r  = ADD32_ovflw(x[2].r, x[3].r);  s8.i  = ADD32_ovflw(x[2].i, x[3].i);
      s9.r  = SUB32_ovflw(x[2].r, x[3].r);  s9.i  = SUB32_ovflw(x[2].i, x[3].i);

      s3.r = ADD32_ovflw(s7.r, s8.r);  s3.i = ADD32_ovflw(s7.i, s8.i);
      s4.r = SUB32_ovflw(s7.r, s8.r);  s4.i = SUB32_ovflw(s7.i, s8.i);

      X[0].r = ADD32_ovflw(x[0].r, s3.r);
      X[0].i = ADD32_ovflw(x[0].i, s3.i);

      s3.r = SUB32_ovflw(x[0].r, QUARTER_OF(s3.r));
      s3.i = SUB32_ovflw(x[0].i, QUARTER_OF(s3.i));
      s4.r = S_MUL(s4.r, PFA_YC);
      s4.i = S_MUL(s4.i, PFA_YC);

      s5.r  = ADD32_ovflw(s3.r, s4.r);  s5.i  = ADD32_ovflw(s3.i, s4.i);
      s11.r = SUB32_ovflw(s3.r, s4.r);  s11.i = SUB32_ovflw(s3.i, s4.i);

      s6.r = ADD32_ovflw(S_MUL(s10.i, PFA_YAI), S_MUL(s9.i, PFA_YBI));
      s6.i = NEG32_ovflw(ADD32_ovflw(S_MUL(s10.r, PFA_YAI), S_MUL(s9.r, PFA_YBI)));
      X[1].r = SUB32_ovflw(s5.r, s6.r);  X[1].i = SUB32_ovflw(s5.i, s6.i);
      X[4].r = ADD32_ovflw(s5.r, s6.r);  X[4].i = ADD32_ovflw(s5.i, s6.i);

      s12.r = SUB32_ovflw(S_MUL(s9.i, PFA_YAI), S_MUL(s10.i, PFA_YBI));
      s12.i = SUB32_ovflw(S_MUL(s10.r, PFA_YBI), S_MUL(s9.r, PFA_YAI));
      X[2].r = ADD32_ovflw(s11.r, s12.r);  X[2].i = ADD32_ovflw(s11.i, s12.i);
      X[3].r = SUB32_ovflw(s11.r, s12.r);  X[3].i = SUB32_ovflw(s11.i, s12.i);

      for (t=0;t<5;t++)
         out[(k1 + 3*t)*ostride] = X[(k1 + 3*t) % 5];
   }
}
#endif /* !OVERRIDE_PFA_FFT15 || PFA_KEEP_C */

#endif /* OPUS_PFA */
