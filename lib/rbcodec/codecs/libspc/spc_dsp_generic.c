/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
 * Copyright (C) 2004-2007 Shay Green (blargg)
 * Copyright (C) 2002 Brad Martin
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
static inline int apply_gen_envx( struct voice_t* voice, int output )
{
    return (output * voice->envx) >> 11;
}

static inline int apply_gen_volume( struct voice_t* voice, int output,
                                    int* amp_0, int* amp_1 )
{
    *amp_0 = voice->volume [0] * output;
    *amp_1 = voice->volume [1] * output;
    return output;
}

static inline int apply_gen_amp( struct voice_t* voice, int output,
                                 int* amp_0, int* amp_1)
{
    output = apply_gen_envx( voice, output );
    output = apply_gen_volume( voice, output, amp_0, amp_1 );
    return output;
}

#if !SPC_NOINTERP

#ifndef SPC_GAUSSIAN_FAST_INTERP
static inline int gaussian_fast_interp( int16_t const* samples,
                                        int32_t position,
                                        int16_t const* fwd,
                                        int16_t const* rev )
{
    samples += position >> 12;
    return (fwd [0] * samples [0] +
            fwd [1] * samples [1] +
            rev [1] * samples [2] +
            rev [0] * samples [3]) >> 11;
}
#endif /* SPC_GAUSSIAN_FAST_INTERP */

#ifndef SPC_GAUSSIAN_FAST_AMP
#define gaussian_fast_amp   apply_amp
#endif /* SPC_GAUSSIAN_FAST_AMP */

#ifndef SPC_GAUSSIAN_SLOW_INTERP
static inline int gaussian_slow_interp( int16_t const* samples,
                                        int32_t position,
                                        int16_t const* fwd,
                                        int16_t const* rev )
{
    int output;
    samples += position >> 12;
    output = (fwd [0] * samples [0]) & ~0xFFF;
    output = (output + fwd [1] * samples [1]) & ~0xFFF;
    output = (output + rev [1] * samples [2]) >> 12;
    output = (int16_t) (output * 2);
    output += ((rev [0] * samples [3]) >> 12) * 2;
    return CLAMP16( output );
}
#endif /* SPC_GAUSSIAN_SLOW_INTERP */

#ifndef SPC_GAUSSIAN_SLOW_AMP
static inline int gaussian_slow_amp( struct voice_t* voice, int output,
                                     int *amp_0, int *amp_1 )
{
    output = apply_gen_envx( voice, output ) & ~1;
    output = apply_gen_volume( voice, output, amp_0, amp_1 );
    return output;
}
#endif /* SPC_GAUSSIAN_SLOW_AMP */

#define interp      gaussian_slow_interp
#define apply_amp   gaussian_slow_amp

#else /* SPC_NOINTERP */

#ifndef SPC_LINEAR_INTERP
static inline int linear_interp( int16_t const* samples, int32_t position )
{
    int32_t fraction = position & 0xfff;
    int16_t const* pos = (samples + (position >> 12)) + 1;
    return pos[0] + ((fraction * (pos[1] - pos[0])) >> 12);
}
#endif /* SPC_LINEAR_INTERP */

#define interp( samp, pos, fwd, rev ) \
    linear_interp( (samp), (pos) )

#ifndef SPC_LINEAR_AMP
#define linear_amp   apply_gen_amp
#endif /* SPC_LINEAR_AMP */

#define apply_amp    linear_amp
#endif /* SPC_NOINTERP */


#if !SPC_NOECHO

#ifndef SPC_DSP_ECHO_APPLY
/* Init FIR filter */
static inline void echo_init( struct Spc_Dsp* this )
{
    this->fir.pos = 0;
    ci->memset( this->fir.buf, 0, sizeof this->fir.buf );
}

/* Apply FIR filter */
static inline void echo_apply(struct Spc_Dsp* this,
        uint8_t* const echo_ptr, int* out_0, int* out_1)
{
    int fb_0 = GET_LE16SA( echo_ptr     );
    int fb_1 = GET_LE16SA( echo_ptr + 2 );
        
    /* Keep last 8 samples */
    int (* const fir_ptr) [2] = this->fir.buf + this->fir.pos;
    this->fir.pos = (this->fir.pos + 1) & (FIR_BUF_HALF - 1);

    fir_ptr [           0] [0] = fb_0;
    fir_ptr [           0] [1] = fb_1;
    /* duplicate at +8 eliminates wrap checking below */
    fir_ptr [FIR_BUF_HALF] [0] = fb_0;
    fir_ptr [FIR_BUF_HALF] [1] = fb_1;

    fb_0 *= this->fir.coeff [0];
    fb_1 *= this->fir.coeff [0];

    #define DO_PT( i ) \
        fb_0 += fir_ptr [i] [0] * this->fir.coeff [i]; \
        fb_1 += fir_ptr [i] [1] * this->fir.coeff [i];

    DO_PT( 1 )
    DO_PT( 2 )
    DO_PT( 3 )
    DO_PT( 4 )
    DO_PT( 5 )
    DO_PT( 6 )
    DO_PT( 7 )

    #undef DO_PT

    *out_0 = fb_0;
    *out_1 = fb_1;
}
#endif /* SPC_DSP_ECHO_APPLY */

#ifndef SPC_DSP_ECHO_FEEDBACK
/* Feedback into echo buffer */
static inline void echo_feedback( struct Spc_Dsp* this, uint8_t *echo_ptr,
                                  int echo_0, int echo_1, int fb_0, int fb_1 )
{
    int e0 = (echo_0 >> 7) + ((fb_0 * this->r.g.echo_feedback) >> 14);
    int e1 = (echo_1 >> 7) + ((fb_1 * this->r.g.echo_feedback) >> 14);
    e0 = CLAMP16( e0 );
    SET_LE16A( echo_ptr    , e0 );
    e1 = CLAMP16( e1 );
    SET_LE16A( echo_ptr + 2, e1 );
}
#endif /* SPC_DSP_ECHO_FEEDBACK */

#ifndef SPC_DSP_GENERATE_OUTPUT
/* Generate final output */
static inline void echo_output( struct Spc_Dsp* this, int global_muting,
    int global_vol_0, int global_vol_1, int chans_0, int chans_1, 
    int fb_0, int fb_1, int* out_0, int* out_1 )
{
    *out_0 = (chans_0 * global_vol_0 + fb_0 * this->r.g.echo_volume_0)
                >> global_muting;
    *out_1 = (chans_1 * global_vol_1 + fb_1 * this->r.g.echo_volume_1)
                >> global_muting;
}
#endif /* SPC_DSP_GENERATE_OUTPUT */

#define mix_output echo_output

#else /* SPC_NOECHO */

#ifndef SPC_DSP_GENERATE_OUTPUT
/* Generate final output */
static inline void noecho_output( struct Spc_Dsp* this, int global_muting,
    int global_vol_0, int global_vol_1, int chans_0, int chans_1, 
    int* out_0, int* out_1 )
{
    *out_0 = (chans_0 * global_vol_0) >> global_muting;
    *out_1 = (chans_1 * global_vol_1) >> global_muting;
    (void)this;
}
#endif /* SPC_DSP_GENERATE_OUTPUT */

#define mix_output(this, gm, gv0, gv1, ch0, ch1, fb_0, fb_1, o0, o1) \
    noecho_output( (this), (gm), (gv0), (gv1), (ch0), (ch1), (o0), (o1) )

#endif /* !SPC_NOECHO */
