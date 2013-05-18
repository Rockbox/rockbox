/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Michael Sevakis (jhMikeS)
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
#if SPC_NOINTERP

#define SPC_LINEAR_INTERP
static inline int linear_interp( int16_t const* samples, int32_t position )
{
    uint32_t f = position;
    int32_t y0, y1;

    /**
     * output = y0 + f*y1 - f*y0
     */
    asm volatile (
    "move.l   %[f], %[y1]               \n" /* separate frac and whole */
    "and.l    #0xfff, %[f]              \n"
    "asr.l    %[sh], %[y1]              \n"
    "move.l   2(%[s], %[y1].l*2), %[y1] \n" /* y0=upper, y1=lower  */
    "mac.w    %[f]l, %[y1]l, %%acc0     \n" /* %acc0 = f*y1 */
    "msac.w   %[f]l, %[y1]u, %%acc0     \n" /* %acc0 -= f*y0 */
    "swap     %[y1]                     \n" /* separate out y0 and sign extend */
    "movea.w  %[y1], %[y0]              \n"
    "movclr.l %%acc0, %[y1]             \n" /* fetch, scale down, add y0 */
    "asr.l    %[sh], %[y1]              \n" /* output = y0 + (result >> 12) */
    "add.l    %[y0], %[y1]              \n"
    : [f]"+d"(f), [y0]"=&a"(y0), [y1]"=&d"(y1)
    : [s]"a"(samples), [sh]"d"(12));

    return y1;
}

#define SPC_LINEAR_AMP
static inline int linear_amp( struct voice_t* voice, int output,
                              int* amp_0, int* amp_1 )
{
    asm volatile (
    "mac.w %[out]l, %[envx]l, %%acc0"
    :
    : [out]"r"(output), [envx]"r"(voice->envx));
    asm volatile (
    "movclr.l %%acc0, %[out]        \n"
    "asr.l    #8, %[out]            \n"
    "mac.l    %[v0], %[out], %%acc0 \n"
    "mac.l    %[v1], %[out], %%acc1 \n"
    "asr.l    #3, %[out]            \n"
    : [out]"+r"(output)
    : [v0]"r"((int) voice->volume [0]),
      [v1]"r"((int) voice->volume [1]));
    asm volatile (
    "movclr.l %%acc0, %[a0]         \n"
    "asr.l    #3, %[a0]             \n"
    "movclr.l %%acc1, %[a1]         \n"
    "asr.l    #3, %[a1]             \n"
    : [a0]"=d"(*amp_0), [a1]"=d"(*amp_1));

    return output;
}

#endif /* SPC_NOINTERP */


#if !SPC_NOECHO

#define SPC_DSP_ECHO_APPLY

/* Echo filter history */
static int32_t fir_buf[FIR_BUF_CNT] IBSS_ATTR_SPC
    __attribute__(( aligned(FIR_BUF_ALIGN*1) ));

static inline void echo_init( struct Spc_Dsp* this )
{
    /* Initialize mask register with the buffer address mask */
    asm volatile ("move.l %0, %%mask" : : "i"(FIR_BUF_MASK));
    this->fir.ptr = fir_buf;
    this->fir.hist_ptr = &fir_buf [1];
    ci->memset( fir_buf, 0, sizeof fir_buf );
}

static inline void echo_apply( struct Spc_Dsp* this, uint8_t* echo_ptr,
                               int* out_0, int* out_1 )
{
    int t0, t1, t2;

    t1 = swap_odd_even32( *(int32_t *)echo_ptr );

    /* Keep last 8 samples */
    *this->fir.ptr = t1;
    this->fir.ptr = this->fir.hist_ptr;

    asm volatile (
    "move.l                         (%[c])  , %[t2]         \n"
    "mac.w    %[t1]u, %[t2]u, <<,   (%[p])+&, %[t0], %%acc0 \n"
    "mac.w    %[t1]l, %[t2]u, <<,   (%[p])& , %[t1], %%acc1 \n"
    "mac.w    %[t0]u, %[t2]l, <<                   , %%acc0 \n"
    "mac.w    %[t0]l, %[t2]l, <<,  4(%[c])  , %[t2], %%acc1 \n"
    "mac.w    %[t1]u, %[t2]u, <<,  4(%[p])& , %[t0], %%acc0 \n"
    "mac.w    %[t1]l, %[t2]u, <<,  8(%[p])& , %[t1], %%acc1 \n"
    "mac.w    %[t0]u, %[t2]l, <<                   , %%acc0 \n"
    "mac.w    %[t0]l, %[t2]l, <<,  8(%[c])  , %[t2], %%acc1 \n"
    "mac.w    %[t1]u, %[t2]u, <<, 12(%[p])& , %[t0], %%acc0 \n"
    "mac.w    %[t1]l, %[t2]u, <<, 16(%[p])& , %[t1], %%acc1 \n"
    "mac.w    %[t0]u, %[t2]l, <<                   , %%acc0 \n"
    "mac.w    %[t0]l, %[t2]l, <<, 12(%[c])  , %[t2], %%acc1 \n"
    "mac.w    %[t1]u, %[t2]u, <<, 20(%[p])& , %[t0], %%acc0 \n"
    "mac.w    %[t1]l, %[t2]u, <<                   , %%acc1 \n"
    "mac.w    %[t0]u, %[t2]l, <<                   , %%acc0 \n"
    "mac.w    %[t0]l, %[t2]l, <<                   , %%acc1 \n"
    : [t0]"=&r"(t0), [t1]"+r"(t1), [t2]"=&r"(t2),
      [p]"+a"(this->fir.hist_ptr)
    : [c]"a"(this->fir.coeff));
    asm volatile (
    "movclr.l %%acc0, %[o0]             \n"
    "movclr.l %%acc1, %[o1]             \n"
    "mac.l    %[ev0], %[o0], >>, %%acc2 \n" /* echo volume */
    "mac.l    %[ev1], %[o1], >>, %%acc3 \n"
    : [o0]"=&r"(*out_0), [o1]"=&r"(*out_1)
    : [ev0]"r"((int) this->r.g.echo_volume_0),
      [ev1]"r"((int) this->r.g.echo_volume_1));
}

#define SPC_DSP_ECHO_FEEDBACK
static inline void echo_feedback( struct Spc_Dsp* this, uint8_t* echo_ptr,
                                  int echo_0, int echo_1, int fb_0, int fb_1 )
{
    asm volatile (
    /* scale echo voices; saturate if overflow */
    "mac.l    %[sh], %[e1]     , %%acc1 \n"
    "mac.l    %[sh], %[e0]     , %%acc0 \n"
    /* add scaled output from FIR filter */
    "mac.l    %[fb1], %[ef], <<, %%acc1 \n"
    "mac.l    %[fb0], %[ef], <<, %%acc0 \n"
    :
    : [e0]"d"(echo_0), [e1]"d"(echo_1),
      [fb0]"r"(fb_0), [fb1]"r"(fb_1),
      [ef]"r"((int)this->r.g.echo_feedback),
      [sh]"r"(1 << 9));
    /* swap and fetch feedback results */
    int t0;
    asm volatile(
    "move.l   #0x00ff00ff, %[t0]        \n"
    "movclr.l %%acc1, %[e1]             \n"
    "swap.w   %[e1]                     \n"
    "movclr.l %%acc0, %[e0]             \n"
    "move.w   %[e1], %[e0]              \n"
    "and.l    %[e0], %[t0]              \n"
    "eor.l    %[t0], %[e0]              \n"
    "lsl.l    #8, %[t0]                 \n"
    "lsr.l    #8, %[e0]                 \n"
    "or.l     %[e0], %[t0]              \n"
    : [e0]"=&d"(echo_0), [e1]"=&d"(echo_1),
      [t0]"=&d"(t0));

    /* save final feedback into echo buffer */
    *(int32_t *)echo_ptr = t0;
}

#define SPC_DSP_GENERATE_OUTPUT
static inline void echo_output( struct Spc_Dsp* this, int global_muting,
    int global_vol_0, int global_vol_1, int chans_0, int chans_1, 
    int fb_0, int fb_1, int* out_0, int* out_1 )
{
    asm volatile (
    "mac.l    %[ch0], %[gv0], %%acc2 \n" /* global volume */
    "mac.l    %[ch1], %[gv1], %%acc3 \n"
    :
    : [ch0]"r"(chans_0), [gv0]"r"(global_vol_0),
      [ch1]"r"(chans_1), [gv1]"r"(global_vol_1));
    asm volatile (
    "movclr.l %%acc2, %[a0]          \n" /* fetch mixed output */
    "movclr.l %%acc3, %[a1]          \n"
    "asr.l    %[gm],  %[a0]          \n" /* scale by global_muting shift */
    "asr.l    %[gm],  %[a1]          \n"
    : [a0]"=&d"(*out_0), [a1]"=&d"(*out_1)
    : [gm]"d"(global_muting));

    /* scaled echo is stored in %acc2 and %acc3 */
    (void)this; (void)fb_0; (void)fb_1;
}

#endif /* !SPC_NOECHO */
