/* Copyright (c) 2007-2008 CSIRO
   Copyright (c) 2007-2008 Xiph.Org Foundation
   Written by Jean-Marc Valin */
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

/* This is a simple MDCT implementation that uses a N/4 complex FFT
   to do most of the work. It should be relatively straightforward to
   plug in pretty much and FFT here.

   This replaces the Vorbis FFT (and uses the exact same API), which
   was a bit too messy and that was ending up duplicating code
   (might as well use the same FFT everywhere).

   The algorithm is similar to (and inspired from) Fabrice Bellard's
   MDCT implementation in FFMPEG, but has differences in signs, ordering
   and scaling in many places.
*/

#ifndef SKIP_CONFIG_H
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#endif

#include "mdct.h"
#include "kiss_fft.h"
#include "_kiss_fft_guts.h"
#include <math.h>
#include "os_support.h"
#include "mathops.h"
#ifdef OPUS_PFA
#include "pfa.h"
#include "pfa_tables.h"

/* The pre-rotation already scatters through a table; for the prime factor
   transform it reads this one instead of the bit-reversal, which is why the
   gather costs nothing. */
static const opus_int16 *pfa_gather(int n)
{
   switch (n)
   {
      case  60: return pfa_in_60;
      case 120: return pfa_in_120;
      case 240: return pfa_in_240;
      default:  return pfa_in_480;
   }
}

/* Where the post-rotation finds X[k], as a byte offset. */
static const opus_int16 *pfa_postmap(int n)
{
   switch (n)
   {
      case  60: return pfa_post_60;
      case 120: return pfa_post_120;
      case 240: return pfa_post_240;
      default:  return pfa_post_480;
   }
}

/* Walk p(k) = M*(k mod 15) + (k mod M) by one step.  Both residues are
   running counters, so the post-rotation needs no index table. */
#define PFA_ADV_UP(p, k1, k2, M) do {    (p) += (M)+1;    if (++(k1) == 15) { (k1) = 0;     (p) -= 15*(M); }    if (++(k2) == (M)) { (k2) = 0;    (p) -= (M); } } while (0)
#define PFA_ADV_DN(p, k1, k2, M) do {    (p) -= (M)+1;    if ((k1)-- == 0) { (k1) = 14;     (p) += 15*(M); }    if ((k2)-- == 0) { (k2) = (M)-1;  (p) += (M); } } while (0)
#endif
#if defined(OPUS_ARM_ASM)
#include "arm/mdct_armv4.h"
#include "arm/mdct_armv5e.h"
#endif
#include "stack_alloc.h"

#if defined(MIPSr1_ASM)
#include "mips/mdct_mipsr1.h"
#endif


#ifdef CUSTOM_MODES

int clt_mdct_init(mdct_lookup *l,int N, int maxshift, int arch)
{
   int i;
   kiss_twiddle_scalar *trig;
   int shift;
   int N2=N>>1;
   l->n = N;
   l->maxshift = maxshift;
   for (i=0;i<=maxshift;i++)
   {
      if (i==0)
         l->kfft[i] = opus_fft_alloc(N>>2>>i, 0, 0, arch);
      else
         l->kfft[i] = opus_fft_alloc_twiddles(N>>2>>i, 0, 0, l->kfft[0], arch);
#ifndef ENABLE_TI_DSPLIB55
      if (l->kfft[i]==NULL)
         return 0;
#endif
   }
   l->trig = trig = (kiss_twiddle_scalar*)opus_alloc((N-(N2>>maxshift))*sizeof(kiss_twiddle_scalar));
   if (l->trig==NULL)
     return 0;
   for (shift=0;shift<=maxshift;shift++)
   {
      /* We have enough points that sine isn't necessary */
#if defined(FIXED_POINT)
#if 1
      for (i=0;i<N2;i++)
         trig[i] = TRIG_UPSCALE*celt_cos_norm(DIV32(ADD32(SHL32(EXTEND32(i),17),N2+16384),N));
#else
      for (i=0;i<N2;i++)
         trig[i] = (kiss_twiddle_scalar)MAX32(-32767,MIN32(32767,floor(.5+32768*cos(2*M_PI*(i+.125)/N))));
#endif
#else
      for (i=0;i<N2;i++)
         trig[i] = (kiss_twiddle_scalar)cos(2*PI*(i+.125)/N);
#endif
      trig += N2;
      N2 >>= 1;
      N >>= 1;
   }
   return 1;
}

void clt_mdct_clear(mdct_lookup *l, int arch)
{
   int i;
   for (i=0;i<=l->maxshift;i++)
      opus_fft_free(l->kfft[i], arch);
   opus_free((kiss_twiddle_scalar*)l->trig);
}

#endif /* CUSTOM_MODES */

/* Forward MDCT trashes the input array */
#ifndef OVERRIDE_clt_mdct_forward
void clt_mdct_forward_c(const mdct_lookup *l, kiss_fft_scalar *in, kiss_fft_scalar * OPUS_RESTRICT out,
      const opus_val16 *window, int overlap, int shift, int stride, int arch)
{
   int i;
   int N, N2, N4;
   VARDECL(kiss_fft_scalar, f);
   VARDECL(kiss_fft_cpx, f2);
   const kiss_fft_state *st = l->kfft[shift];
   const kiss_twiddle_scalar *trig;
   opus_val16 scale;
#ifdef FIXED_POINT
   /* Allows us to scale with MULT16_32_Q16(), which is faster than
      MULT16_32_Q15() on ARM. */
   int scale_shift = st->scale_shift-1;
#endif
   SAVE_STACK;
   (void)arch;
   scale = st->scale;

   N = l->n;
   trig = l->trig;
   for (i=0;i<shift;i++)
   {
      N >>= 1;
      trig += N;
   }
   N2 = N>>1;
   N4 = N>>2;

   ALLOC(f, N2, kiss_fft_scalar);
   ALLOC(f2, N4, kiss_fft_cpx);

   /* Consider the input to be composed of four blocks: [a, b, c, d] */
   /* Window, shuffle, fold */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const kiss_fft_scalar * OPUS_RESTRICT xp1 = in+(overlap>>1);
      const kiss_fft_scalar * OPUS_RESTRICT xp2 = in+N2-1+(overlap>>1);
      kiss_fft_scalar * OPUS_RESTRICT yp = f;
      const opus_val16 * OPUS_RESTRICT wp1 = window+(overlap>>1);
      const opus_val16 * OPUS_RESTRICT wp2 = window+(overlap>>1)-1;
      for(i=0;i<((overlap+3)>>2);i++)
      {
         /* Real part arranged as -d-cR, Imag part arranged as -b+aR*/
         *yp++ = MULT16_32_Q15(*wp2, xp1[N2]) + MULT16_32_Q15(*wp1,*xp2);
         *yp++ = MULT16_32_Q15(*wp1, *xp1)    - MULT16_32_Q15(*wp2, xp2[-N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
      wp1 = window;
      wp2 = window+overlap-1;
      for(;i<N4-((overlap+3)>>2);i++)
      {
         /* Real part arranged as a-bR, Imag part arranged as -c-dR */
         *yp++ = *xp2;
         *yp++ = *xp1;
         xp1+=2;
         xp2-=2;
      }
      for(;i<N4;i++)
      {
         /* Real part arranged as a-bR, Imag part arranged as -c-dR */
         *yp++ =  -MULT16_32_Q15(*wp1, xp1[-N2]) + MULT16_32_Q15(*wp2, *xp2);
         *yp++ = MULT16_32_Q15(*wp2, *xp1)     + MULT16_32_Q15(*wp1, xp2[N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
   }
   /* Pre-rotation */
   {
      kiss_fft_scalar * OPUS_RESTRICT yp = f;
      const kiss_twiddle_scalar *t = &trig[0];
      for(i=0;i<N4;i++)
      {
         kiss_fft_cpx yc;
         kiss_twiddle_scalar t0, t1;
         kiss_fft_scalar re, im, yr, yi;
         t0 = t[i];
         t1 = t[N4+i];
         re = *yp++;
         im = *yp++;
         yr = S_MUL(re,t0)  -  S_MUL(im,t1);
         yi = S_MUL(im,t0)  +  S_MUL(re,t1);
         yc.r = yr;
         yc.i = yi;
         yc.r = PSHR32(MULT16_32_Q16(scale, yc.r), scale_shift);
         yc.i = PSHR32(MULT16_32_Q16(scale, yc.i), scale_shift);
         f2[st->bitrev[i]] = yc;
      }
   }

   /* N/4 complex FFT, does not downscale anymore */
   opus_fft_impl(st, f2);

   /* Post-rotate */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const kiss_fft_cpx * OPUS_RESTRICT fp = f2;
      kiss_fft_scalar * OPUS_RESTRICT yp1 = out;
      kiss_fft_scalar * OPUS_RESTRICT yp2 = out+stride*(N2-1);
      const kiss_twiddle_scalar *t = &trig[0];
      /* Temp pointers to make it really clear to the compiler what we're doing */
      for(i=0;i<N4;i++)
      {
         kiss_fft_scalar yr, yi;
         yr = S_MUL(fp->i,t[N4+i]) - S_MUL(fp->r,t[i]);
         yi = S_MUL(fp->r,t[N4+i]) + S_MUL(fp->i,t[i]);
         *yp1 = yr;
         *yp2 = yi;
         fp++;
         yp1 += 2*stride;
         yp2 -= 2*stride;
      }
   }
   RESTORE_STACK;
}
#endif /* OVERRIDE_clt_mdct_forward */

#ifndef OVERRIDE_clt_mdct_backward
void clt_mdct_backward_c(const mdct_lookup *l, kiss_fft_scalar *in, kiss_fft_scalar * OPUS_RESTRICT out,
      const opus_val16 * OPUS_RESTRICT window, int overlap, int shift, int stride, int arch)
{
   int i;
   int N, N2, N4;
   const kiss_twiddle_scalar *trig;
#ifdef OPUS_PFA
   int use_pfa;
   kiss_fft_cpx *pfa_out = NULL;
   VARDECL(kiss_fft_cpx, pfa_tmp);
#endif
   SAVE_STACK;
   (void) arch;

   N = l->n;
   trig = l->trig;
   for (i=0;i<shift;i++)
   {
      N >>= 1;
      trig += N;
   }
   N2 = N>>1;
   N4 = N>>2;

#ifdef OPUS_PFA
   /* Pass 1 of the prime factor transform reads fifteen contiguous points
      and writes them strided, so it cannot work in place.  The input buffer
      is exactly the right size and is documented as destroyed here, so for
      the usual stride==1 call it costs nothing; a strided call cannot reuse
      it, but those are always the short transform, N4 == 60. */
   use_pfa = OPUS_PFA_SIZE(N4) && (stride == 1 || N4 == 60);
   ALLOC(pfa_tmp, (use_pfa && stride != 1) ? N4 : ALLOC_NONE, kiss_fft_cpx);
   if (use_pfa)
      pfa_out = (stride == 1) ? (kiss_fft_cpx *)in : pfa_tmp;
#endif

   /* Pre-rotate */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const kiss_fft_scalar * OPUS_RESTRICT xp1 = in;
      const kiss_fft_scalar * OPUS_RESTRICT xp2 = in+stride*(N2-1);
      kiss_fft_scalar * OPUS_RESTRICT yp = out+(overlap>>1);
      const kiss_twiddle_scalar * OPUS_RESTRICT t = &trig[0];
      const opus_int16 * OPUS_RESTRICT bitrev = l->kfft[shift]->bitrev;
#ifdef OPUS_PFA
      if (use_pfa) bitrev = pfa_gather(N4);
#endif
#ifdef OVERRIDE_MDCT_PREROT
      mdct_prerot_opt(xp1, xp2, t, bitrev, yp, N4,
                      2*stride*(int)sizeof(kiss_fft_scalar));
#else
      for(i=0;i<N4;i++)
      {
         int rev;
         kiss_fft_scalar yr, yi;
         rev = *bitrev++;
         yr = ADD32_ovflw(S_MUL(*xp2, t[i]), S_MUL(*xp1, t[N4+i]));
         yi = SUB32_ovflw(S_MUL(*xp1, t[i]), S_MUL(*xp2, t[N4+i]));
         /* We swap real and imag because we use an FFT instead of an IFFT. */
         yp[2*rev+1] = yr;
         yp[2*rev] = yi;
         /* Storing the pre-rotation directly in the bitrev order. */
         xp1+=2*stride;
         xp2-=2*stride;
      }
#endif
   }

#ifdef OPUS_PFA
   if (use_pfa)
      opus_pfa_impl((const kiss_fft_cpx*)(out+(overlap>>1)), pfa_out, N4);
   else
#endif
   opus_fft_impl(l->kfft[shift], (kiss_fft_cpx*)(out+(overlap>>1)));

   /* Post-rotate and de-shuffle from both ends of the buffer at once to make
      it in-place. */
   {
      kiss_fft_scalar * yp0 = out+(overlap>>1);
      kiss_fft_scalar * yp1 = out+(overlap>>1)+N2-2;
      const kiss_twiddle_scalar *t = &trig[0];
      /* Loop to (N4+1)>>1 to handle odd N4. When N4 is odd, the
         middle pair will be computed twice. */
#ifdef OPUS_PFA
      if (use_pfa)
      {
         /* Same rotation, but the transform is in Good-Thomas order, so the
            two ends are gathered through p(k) rather than read in place.
            That is also why this writes a different buffer than it reads. */
         const kiss_fft_scalar *S = (const kiss_fft_scalar *)pfa_out;
         int M = N4/15;
         int pa = 0, ka = 0, qa = 0;
         int kd = (N4-1)%15, qd = (N4-1)&(M-1);
         int pd = M*kd + qd;
#ifdef OVERRIDE_MDCT_POSTROT_PFA
         mdct_postrot_pfa_opt(S, yp0, yp1, t, pfa_postmap(N4), N4);
         (void)pa; (void)ka; (void)qa; (void)pd; (void)kd; (void)qd; (void)M;
#else
         for(i=0;i<(N4+1)>>1;i++)
         {
            kiss_fft_scalar re, im, yr, yi;
            kiss_twiddle_scalar t0, t1;
            re = S[2*pa+1];
            im = S[2*pa];
            t0 = t[i];
            t1 = t[N4+i];
            yr = ADD32_ovflw(S_MUL(re,t0), S_MUL(im,t1));
            yi = SUB32_ovflw(S_MUL(re,t1), S_MUL(im,t0));
            re = S[2*pd+1];
            im = S[2*pd];
            yp0[0] = yr;
            yp1[1] = yi;

            t0 = t[(N4-i-1)];
            t1 = t[(N2-i-1)];
            yr = ADD32_ovflw(S_MUL(re,t0), S_MUL(im,t1));
            yi = SUB32_ovflw(S_MUL(re,t1), S_MUL(im,t0));
            yp1[0] = yr;
            yp0[1] = yi;
            yp0 += 2;
            yp1 -= 2;
            PFA_ADV_UP(pa, ka, qa, M);
            PFA_ADV_DN(pd, kd, qd, M);
         }
#endif
      }
      else
#endif
#ifdef OVERRIDE_MDCT_POSTROT
      mdct_postrot_opt(yp0, yp1, t, N4, (N4+1)>>1);
#else
      for(i=0;i<(N4+1)>>1;i++)
      {
         kiss_fft_scalar re, im, yr, yi;
         kiss_twiddle_scalar t0, t1;
         /* We swap real and imag because we're using an FFT instead of an IFFT. */
         re = yp0[1];
         im = yp0[0];
         t0 = t[i];
         t1 = t[N4+i];
         /* We'd scale up by 2 here, but instead it's done when mixing the windows */
         yr = ADD32_ovflw(S_MUL(re,t0), S_MUL(im,t1));
         yi = SUB32_ovflw(S_MUL(re,t1), S_MUL(im,t0));
         /* We swap real and imag because we're using an FFT instead of an IFFT. */
         re = yp1[1];
         im = yp1[0];
         yp0[0] = yr;
         yp1[1] = yi;

         t0 = t[(N4-i-1)];
         t1 = t[(N2-i-1)];
         /* We'd scale up by 2 here, but instead it's done when mixing the windows */
         yr = ADD32_ovflw(S_MUL(re,t0), S_MUL(im,t1));
         yi = SUB32_ovflw(S_MUL(re,t1), S_MUL(im,t0));
         yp1[0] = yr;
         yp0[1] = yi;
         yp0 += 2;
         yp1 -= 2;
      }
#endif
   }

   /* Mirror on both sides for TDAC */
   {
      kiss_fft_scalar * OPUS_RESTRICT xp1 = out+overlap-1;
      kiss_fft_scalar * OPUS_RESTRICT yp1 = out;
      const opus_val16 * OPUS_RESTRICT wp1 = window;
      const opus_val16 * OPUS_RESTRICT wp2 = window+overlap-1;

#ifdef OVERRIDE_MDCT_MIRROR
      mdct_mirror_opt(xp1, yp1, wp1, wp2, overlap/2);
#else
      for(i = 0; i < overlap/2; i++)
      {
         kiss_fft_scalar x1, x2;
         x1 = *xp1;
         x2 = *yp1;
         *yp1++ = SUB32_ovflw(MULT16_32_Q15(*wp2, x2), MULT16_32_Q15(*wp1, x1));
         *xp1-- = ADD32_ovflw(MULT16_32_Q15(*wp1, x2), MULT16_32_Q15(*wp2, x1));
         wp1++;
         wp2--;
      }
#endif
   }
   RESTORE_STACK;
}
#endif /* OVERRIDE_clt_mdct_backward */
