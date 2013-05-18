/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2010 Michael Sevakis (jhMikeS)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#if !SPC_NOINTERP

#define SPC_GAUSSIAN_FAST_INTERP
static inline int gaussian_fast_interp( int16_t const* samples,
                                        int32_t position,
                                        int16_t const* fwd,
                                        int16_t const* rev )
{
    int output;
    int t0, t1, t2, t3;

    asm volatile (
    "ldrsh   %[t0], [%[samp]]             \n"
    "ldrsh   %[t2], [%[fwd]]              \n"
    "ldrsh   %[t1], [%[samp], #2]         \n"
    "ldrsh   %[t3], [%[fwd], #2]          \n"
    "mul     %[out], %[t0], %[t2]         \n" /* out= fwd[0]*samp[0] */
    "ldrsh   %[t0], [%[samp], #4]         \n"
    "ldrsh   %[t2], [%[rev], #2]          \n"
    "mla     %[out], %[t1], %[t3], %[out] \n" /* out+=fwd[1]*samp[1] */
    "ldrsh   %[t1], [%[samp], #6]         \n"
    "ldrsh   %[t3], [%[rev]]              \n"
    "mla     %[out], %[t0], %[t2], %[out] \n" /* out+=rev[1]*samp[2] */
    "mla     %[out], %[t1], %[t3], %[out] \n" /* out+=rev[0]*samp[3] */
    : [out]"=&r"(output),
      [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3)
    : [fwd]"r"(fwd), [rev]"r"(rev),
      [samp]"r"(samples + (position >> 12)));

    return output;
}

#define SPC_GAUSSIAN_FAST_AMP
static inline int gaussian_fast_amp( struct voice_t* voice, int output,
                                     int* amp_0, int* amp_1 )
{
    int t0;

    asm volatile (
    "mov    %[t0], %[out], asr #11  \n"
    "mul    %[out], %[t0], %[envx]  \n"
    : [out]"+r"(output), [t0]"=&r"(t0)
    : [envx]"r"((int) voice->envx));

    asm volatile (
    "mov    %[out], %[out], asr #11 \n"
    "mul    %[a0], %[out], %[v0]    \n"
    "mul    %[a1], %[out], %[v1]    \n"
    : [out]"+r"(output),
      [a0]"=&r"(*amp_0), [a1]"=r"(*amp_1)
    : [v0]"r"((int) voice->volume [0]),
      [v1]"r"((int) voice->volume [1]));

    return output;
}

#define SPC_GAUSSIAN_SLOW_INTERP
static inline int gaussian_slow_interp( int16_t const* samples,
                                        int32_t position,
                                        int16_t const* fwd,
                                        int16_t const* rev )
{
    int output;
    int t0, t1, t2, t3;

    asm volatile (
    "ldrsh   %[t0], [%[samp]]              \n"
    "ldrsh   %[t2], [%[fwd]]               \n"
    "ldrsh   %[t1], [%[samp], #2]          \n"
    "ldrsh   %[t3], [%[fwd], #2]           \n"
    "mul     %[out], %[t2], %[t0]          \n" /* fwd[0]*samp[0] */
    "ldrsh   %[t2], [%[rev], #2]           \n"
    "mul     %[t0], %[t3], %[t1]           \n" /* fwd[1]*samp[1] */
    "ldrsh   %[t1], [%[samp], #4]          \n"
    "mov     %[out], %[out], asr #12       \n"
    "ldrsh   %[t3], [%[rev]]               \n"
    "mul     %[t2], %[t1], %[t2]           \n" /* rev[1]*samp[2] */
    "ldrsh   %[t1], [%[samp], #6]          \n"
    "add     %[t0], %[out], %[t0], asr #12 \n"
    "mul     %[t3], %[t1], %[t3]           \n" /* rev[0]*samp[3] */
    "add     %[t2], %[t0], %[t2], asr #12  \n"
    "mov     %[t2], %[t2], lsl #17         \n"
    "mov     %[t3], %[t3], asr #12         \n"
    "mov     %[t3], %[t3], asl #1          \n"
    "add     %[out], %[t3], %[t2], asr #16 \n"
    : [out]"=&r"(output),
      [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3)
    : [fwd]"r"(fwd), [rev]"r"(rev),
      [samp]"r"(samples + (position >> 12)));

    return CLAMP16( output );
}

#define SPC_GAUSSIAN_SLOW_AMP
static inline int gaussian_slow_amp( struct voice_t* voice, int output,
                                     int* amp_0, int* amp_1 )
{
    int t0;

    asm volatile (
    "mul     %[t0], %[out], %[envx]"
    : [t0]"=r"(t0)
    : [out]"r"(output), [envx]"r"((int) voice->envx));
    asm volatile (
    "mov     %[t0], %[t0], asr #11 \n"
    "bic     %[t0], %[t0], #0x1    \n"
    "mul     %[a0], %[t0], %[v0]   \n"
    "mul     %[a1], %[t0], %[v1]   \n"
    : [t0]"+r"(t0),
      [a0]"=&r"(*amp_0), [a1]"=r"(*amp_1)
    : [v0]"r"((int) voice->volume [0]),
      [v1]"r"((int) voice->volume [1]));

    return t0;
}

#else /* SPC_NOINTERP */

#define SPC_LINEAR_INTERP
static inline int linear_interp( int16_t const* samples, int32_t position )
{
    int output = (int) samples;
    int y1;

    asm volatile(
    "mov    %[y1], %[f], lsr #12        \n"
    "eor    %[f], %[f], %[y1], lsl #12  \n" 
    "add    %[y1], %[y0], %[y1], lsl #1 \n"
    "ldrsh  %[y0], [%[y1], #2]          \n"
    "ldrsh  %[y1], [%[y1], #4]          \n"
    "sub    %[y1], %[y1], %[y0]         \n"
    "mul    %[f], %[y1], %[f]           \n"
    "add    %[y0], %[y0], %[f], asr #12 \n"
    : [f]"+r"(position), [y0]"+r"(output), [y1]"=&r"(y1));

    return output;
}

#define SPC_LINEAR_AMP
static inline int linear_amp( struct voice_t* voice, int output,
                              int* amp_0, int* amp_1 )
{
    int t0;

    asm volatile(
    "mul    %[t0], %[out], %[envx]"
    : [t0]"=&r"(t0)
    : [out]"r"(output), [envx]"r"(voice->envx));
    asm volatile(
    "mov    %[t0], %[t0], asr #11 \n"
    "mul    %[a1], %[t0], %[v1]   \n"
    "mul    %[a0], %[t0], %[v0]   \n"
    : [t0]"+r"(t0),
      [a0]"=&r"(*amp_0), [a1]"=&r"(*amp_1)
    : [v0]"r"((int) voice->volume [0]),
      [v1]"r"((int) voice->volume [1]));

    return t0;
}

#endif /* !SPC_NOINTERP */


#if !SPC_NOECHO

#define SPC_DSP_ECHO_APPLY

/* Echo filter history */
static int32_t fir_buf[FIR_BUF_CNT] IBSS_ATTR_SPC
    __attribute__(( aligned(FIR_BUF_ALIGN*1) ));

static inline void echo_init( struct Spc_Dsp* this )
{
    this->fir.ptr = fir_buf;
    ci->memset( fir_buf, 0, sizeof fir_buf );
}

static inline void echo_apply( struct Spc_Dsp* this, uint8_t *echo_ptr,
                               int* out_0, int* out_1 )
{
    int t0 = GET_LE16SA( echo_ptr     );
    int t1 = GET_LE16SA( echo_ptr + 2 );

    /* Keep last 8 samples */
    int32_t *fir_ptr;
    asm volatile (
    "add    %[p], %[t_p], #8         \n"
    "bic    %[t_p], %[p], %[mask]    \n"
    "str    %[t0], [%[p], #-8]       \n"
    "str    %[t1], [%[p], #-4]       \n"
    /* duplicate at +8 eliminates wrap checking below */
    "str    %[t0], [%[p], #56]       \n"
    "str    %[t1], [%[p], #60]       \n"
    : [p]"=&r"(fir_ptr), [t_p]"+r"(this->fir.ptr)
    : [t0]"r"(t0), [t1]"r"(t1), [mask]"i"(~FIR_BUF_MASK));

    int32_t *fir_coeff = this->fir.coeff;

    asm volatile (
    "ldmia  %[c]!, { r0-r1 }         \n"
    "ldmia  %[p]!, { r4-r5 }         \n"
    "mul    %[acc0],     r0, %[acc0] \n"
    "mul    %[acc1],     r0, %[acc1] \n"
    "mla    %[acc0], r4, r1, %[acc0] \n"
    "mla    %[acc1], r5, r1, %[acc1] \n"
    "ldmia  %[c]!, { r0-r1 }         \n"
    "ldmia  %[p]!, { r2-r5 }         \n"
    "mla    %[acc0], r2, r0, %[acc0] \n"
    "mla    %[acc1], r3, r0, %[acc1] \n"
    "mla    %[acc0], r4, r1, %[acc0] \n"
    "mla    %[acc1], r5, r1, %[acc1] \n"
    "ldmia  %[c]!, { r0-r1 }         \n"
    "ldmia  %[p]!, { r2-r5 }         \n"
    "mla    %[acc0], r2, r0, %[acc0] \n"
    "mla    %[acc1], r3, r0, %[acc1] \n"
    "mla    %[acc0], r4, r1, %[acc0] \n"
    "mla    %[acc1], r5, r1, %[acc1] \n"
    "ldmia  %[c]!, { r0-r1 }         \n"
    "ldmia  %[p]!, { r2-r5 }         \n"
    "mla    %[acc0], r2, r0, %[acc0] \n"
    "mla    %[acc1], r3, r0, %[acc1] \n"
    "mla    %[acc0], r4, r1, %[acc0] \n"
    "mla    %[acc1], r5, r1, %[acc1] \n"
    : [acc0]"+r"(t0), [acc1]"+r"(t1),
      [p]"+r"(fir_ptr), [c]"+r"(fir_coeff)
    :
    : "r0", "r1", "r2", "r3", "r4", "r5");

    *out_0 = t0;
    *out_1 = t1;
}

#endif /* SPC_NOECHO */
