/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Michael Sevakis (jhMikeS)
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
    "ldrh    %[t0], [%[samp]]             \n" /* t0=s0      */
    "ldrh    %[t2], [%[fwd]]              \n" /* t2=f0      */
    "ldrh    %[t1], [%[samp], #2]         \n" /* t1=s1      */
    "ldrh    %[t3], [%[fwd], #2]          \n" /* r3=f1      */
    "smulbb  %[out], %[t0], %[t2]         \n" /* out=s0*f0  */
    "ldrh    %[t2], [%[rev], #2]          \n" /* r2=r1      */
    "ldrh    %[t0], [%[samp], #4]         \n" /* t0=s2      */            
    "smlabb  %[out], %[t1], %[t3], %[out] \n" /* out+=s1*f1 */
    "ldrh    %[t3], [%[rev]]              \n" /* t3=r0      */
    "ldrh    %[t1], [%[samp], #6]         \n" /* t1=s3      */
    "smlabb  %[out], %[t0], %[t2], %[out] \n" /* out+=s2*r1 */
    "smlabb  %[out], %[t1], %[t3], %[out] \n" /* out+=s3*r0 */
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
    asm volatile (
    "smulwb  %[out], %[out], %[envx] \n"
    : [out]"+r"(output)
    : [envx]"r"(voice->envx));
    asm volatile (
    "mov     %[out], %[out], asl #6  \n"
    "smulwb  %[a0], %[out], %[v0]    \n"
    "smulwb  %[a1], %[out], %[v1]    \n"
    "mov     %[out], %[out], asr #5  \n"
    : [out]"+r"(output),
      [a0]"=&r"(*amp_0), [a1]"=r"(*amp_1)
    : [v0]"r"(voice->volume [0]),
      [v1]"r"(voice->volume [1]));

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
    "ldrsh   %[t0], [%[samp]]               \n" /* t0=s0                */
    "ldrh    %[t2], [%[fwd]]                \n" /* t2=f0                */
    "ldrsh   %[t1], [%[samp], #2]           \n" /* t1=s1                */
    "ldrh    %[t3], [%[fwd], #2]            \n" /* t3=f1                */
    "smulwb  %[out], %[t0], %[t2]           \n" /* out=s0*f0>>16        */
    "ldrsh   %[t0], [%[samp], #4]           \n" /* t0=s2                */
    "ldrh    %[t2], [%[rev], #2]            \n" /* t2=r1                */
    "smlawb  %[out], %[t1], %[t3], %[out]   \n" /* out+=s1*f1>>16       */
    "ldrsh   %[t1], [%[samp], #6]           \n" /* t1=s3                */
    "ldrh    %[t3], [%[rev]]                \n" /* t3=r0                */
    "smlawb  %[out], %[t0], %[t2], %[out]   \n" /* out+=s2*r1>>16       */
    "smulwb  %[t0], %[t1], %[t3]            \n" /* t0=s3*r0>>16         */
    "mov     %[out], %[out], asl #17        \n" /* out=(int16_t)(out*2) */
    "mov     %[t0], %[t0], asl #1           \n" /* out+=t0*2            */
    "add     %[out], %[t0], %[out], asr #16 \n"
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
    asm volatile (
    "smulbb  %[out], %[out], %[envx]"
    : [out]"+r"(output)
    : [envx]"r"(voice->envx));
    asm volatile (
    "mov     %[out], %[out], asr #11 \n"
    "bic     %[out], %[out], #0x1    \n"
    "smulbb  %[a0], %[out], %[v0]    \n"
    "smulbb  %[a1], %[out], %[v1]    \n"
    : [out]"+r"(output),
      [a0]"=&r"(*amp_0), [a1]"=r"(*amp_1)
    : [v0]"r"(voice->volume [0]), [v1]"r"(voice->volume [1]));

    return output;
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
    /* Keep last 8 samples */
    int32_t* fir_ptr;
    int t0;
    asm volatile (
    "ldr     %[t0], [%[ep]]           \n"
    "add     %[p], %[t_p], #4         \n"
    "bic     %[t_p], %[p], %[mask]    \n"
    "str     %[t0], [%[p], #-4]       \n"
    /* duplicate at +8 eliminates wrap checking below */
    "str     %[t0], [%[p], #28]       \n"
    : [p]"=&r"(fir_ptr), [t_p]"+r"(this->fir.ptr),
      [t0]"=&r"(t0)
    : [ep]"r"(echo_ptr), [mask]"i"(~FIR_BUF_MASK));

    int32_t* fir_coeff = (int32_t *)this->fir.coeff;

    asm volatile (
    "ldmia   %[c]!, { r0-r1 }         \n" /* C0C1-C2C3 = r0-r1 */
    "ldmia   %[p]!, { r2-r5 }         \n" /* L1R1-L4R4 = r2-r5 */
    "smulbb  %[acc0], %[t0], r0       \n" /* acc0  = L0*C0     */
    "smultb  %[acc1], %[t0], r0       \n" /* acc1  = R0*C0     */
    "smlabt  %[acc0], r2, r0, %[acc0] \n" /* acc0 += L1*C1     */
    "smlatt  %[acc1], r2, r0, %[acc1] \n" /* acc1 += R1*C1     */
    "smlabb  %[acc0], r3, r1, %[acc0] \n" /* acc0 += L2*C2     */
    "smlatb  %[acc1], r3, r1, %[acc1] \n" /* acc1 += R2*C2     */
    "smlabt  %[acc0], r4, r1, %[acc0] \n" /* acc0 += L3*C3     */
    "smlatt  %[acc1], r4, r1, %[acc1] \n" /* acc1 += R3*C3     */
    "ldmia   %[c], { r0-r1 }          \n" /* C4C5-C6C7 = r0-r1 */
    "ldmia   %[p], { r2-r4 }          \n" /* L5R5-L7R7 = r2-r5 */
    "smlabb  %[acc0], r5, r0, %[acc0] \n" /* acc0 += L4*C4     */
    "smlatb  %[acc1], r5, r0, %[acc1] \n" /* acc1 += R4*C4     */
    "smlabt  %[acc0], r2, r0, %[acc0] \n" /* acc0 += L5*C5     */
    "smlatt  %[acc1], r2, r0, %[acc1] \n" /* acc1 += R5*C5     */
    "smlabb  %[acc0], r3, r1, %[acc0] \n" /* acc0 += L6*C6     */
    "smlatb  %[acc1], r3, r1, %[acc1] \n" /* acc1 += R6*C6     */
    "smlabt  %[acc0], r4, r1, %[acc0] \n" /* acc0 += L7*C7     */
    "smlatt  %[acc1], r4, r1, %[acc1] \n" /* acc1 += R7*C7     */
    : [t0]"+r"(t0), [acc0]"=&r"(*out_0), [acc1]"=&r"(*out_1),
      [p]"+r"(fir_ptr), [c]"+r"(fir_coeff)
    :
    : "r0", "r1", "r2", "r3", "r4", "r5");
}

#define SPC_DSP_ECHO_FEEDBACK
static inline void echo_feedback(struct Spc_Dsp* this, uint8_t* echo_ptr,
                                 int echo_0, int echo_1, int fb_0, int fb_1)
{
    int e0, e1;
    asm volatile (
    "mov      %[e0], %[ei0], asl #7        \n"
    "mov      %[e1], %[ei1], asl #7        \n"
    "mla      %[e0], %[fb0], %[ef], %[e0]  \n"
    "mla      %[e1], %[fb1], %[ef], %[e1]  \n"
    : [e0]"=&r"(e0), [e1]"=&r"(e1)
    : [ei0]"r"(echo_0), [ei1]"r"(echo_1),
      [fb0]"r"(fb_0), [fb1]"r"(fb_1),
      [ef]"r"((int)this->r.g.echo_feedback));

    e0 = CLAMP16( e0 >> 14 );
    SET_LE16A( echo_ptr    , e0 );
    e1 = CLAMP16( e1 >> 14 );
    SET_LE16A( echo_ptr + 2, e1 );
}

#define SPC_DSP_GENERATE_OUTPUT
static inline void echo_output( struct Spc_Dsp* this, int global_muting,
    int global_vol_0, int global_vol_1, int chans_0, int chans_1, 
    int fb_0, int fb_1, int* out_0, int* out_1 )
{
    int t0, t1;

    asm volatile (
    "mul      %[t0], %[gv0], %[ch0]       \n"
    "mul      %[t1], %[gv1], %[ch1]       \n"
    : [t0]"=&r"(t0), [t1]"=r"(t1)
    : [gv0]"r"(global_vol_0), [gv1]"r"(global_vol_1),
      [ch0]"r"(chans_0), [ch1]"r"(chans_1));
    asm volatile (
    "mla      %[t0], %[i0], %[ev0], %[t0] \n"
    "mla      %[t1], %[i1], %[ev1], %[t1] \n"
    : [t0]"+r"(t0), [t1]"+r"(t1)
    : [i0]"r"(fb_0), [i1]"r"(fb_1),
      [ev0]"r"((int)this->r.g.echo_volume_0),
      [ev1]"r"((int)this->r.g.echo_volume_1));
    asm volatile (
    "mov      %[o0], %[t0], asr %[gm]     \n"
    "mov      %[o1], %[t1], asr %[gm]     \n"
    : [o0]"=&r"(*out_0), [o1]"=r"(*out_1)
    : [t0]"r"(t0), [t1]"r"(t1), 
      [gm]"r"(global_muting));
}

#endif /* SPC_NOECHO */
