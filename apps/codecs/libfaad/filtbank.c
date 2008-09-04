/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id$
**/

#include "common.h"
#include "structs.h"

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32_WCE
#define assert(x)
#else
#include <assert.h>
#endif

#include "filtbank.h"
#include "decoder.h"
#include "syntax.h"
#include "kbd_win.h"
#include "sine_win.h"


/*Windowing functions borrowed from libwmai*/

#ifdef CPU_ARM
static inline 
void vector_fmul_add_add(real_t *dst, const real_t *src0, const real_t *src1, const real_t *src2, int len)
{
    /* Block sizes are always power of two */
    asm volatile (
        "0:"
        "ldmia %[d]!, {r0, r1};"
        "ldmia %[w]!, {r4, r5};"
        /* consume the first data and window value so we can use those
         * registers again */
        "smull r8, r9, r0, r4;"
        "ldmia %[src2]!, {r0, r4};"
        "add   r0, r0, r9, lsl #1;"  /* *dst=*dst+(r9<<1)*/
        "smull r8, r9, r1, r5;"
        "add   r1, r4, r9, lsl #1;"
        "stmia %[dst]!, {r0, r1};"
        "subs  %[n], %[n], #2;"
        "bne   0b;"
        : [d] "+r" (src0), [w] "+r" (src1), [src2] "+r" (src2), [dst] "+r" (dst), [n] "+r" (len)
        : 
        : "r0", "r1", "r4", "r5", "r8", "r9", "memory", "cc");
}
static inline
void vector_fmul_reverse(real_t *dst, const real_t *src0, const real_t *src1,
                         int len)
{
    /* Block sizes are always power of two */
    asm volatile (
        "add   %[s1], %[s1], %[n], lsl #2;"
        "0:"
        "ldmia %[s0]!, {r0, r1};"
        "ldmdb %[s1]!, {r4, r5};"
        "smull r8, r9, r0, r5;"
        "mov   r0, r9, lsl #1;"
        "smull r8, r9, r1, r4;"
        "mov   r1, r9, lsl #1;"
        "stmia %[dst]!, {r0, r1};"
        "subs  %[n], %[n], #2;"
        "bne   0b;"
        : [s0] "+r" (src0), [s1] "+r" (src1), [dst] "+r" (dst), [n] "+r" (len)
        : 
        : "r0", "r1", "r4", "r5", "r8", "r9", "memory", "cc");
}

#elif defined(CPU_COLDFIRE)
static inline
void vector_fmul_add_add(real_t *dst, const real_t *src0, const real_t *src1, const real_t *src2, int len)
{
    /* Block sizes are always power of two. Smallest block is always way bigger
     * than four too.*/
    asm volatile (
        "0:"
        "movem.l (%[src0]), %%d0-%%d3;"
        "movem.l (%[src1]), %%d4-%%d5/%%a0-%%a1;"
        "mac.l %%d0, %%d4, %%acc0;"
        "mac.l %%d1, %%d5, %%acc1;"
        "mac.l %%d2, %%a0, %%acc2;"
        "mac.l %%d3, %%a1, %%acc3;"
        "lea.l (16, %[src0]), %[src0];"
        "lea.l (16, %[src1]), %[src1];"
        "movclr.l %%acc0, %%d0;"
        "movclr.l %%acc1, %%d1;"
        "movclr.l %%acc2, %%d2;"
        "movclr.l %%acc3, %%d3;"
        "movem.l (%[src2]), %%d4-%%d5/%%a0-%%a1;"
        "lea.l (16, %[src2]), %[src2];"
        "add.l %%d4, %%d0;"
        "add.l %%d5, %%d1;"
        "add.l %%a0, %%d2;"
        "add.l %%a1, %%d3;"
        "movem.l %%d0-%%d3, (%[dst]);"
        "lea.l (16, %[dst]), %[dst];"
        "subq.l #4, %[n];"
        "jne 0b;"
        : [src0] "+a" (src0), [src1] "+a" (src1), [src2] "+a" (src2), [dst] "+a" (dst), [n] "+d" (len)
        : 
        : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "memory", "cc");
}

static inline
void vector_fmul_reverse(real_t *dst, const real_t *src0, const real_t *src1,
                         int len)
{
    /* Block sizes are always power of two. Smallest block is always way bigger
     * than four too.*/
    asm volatile (
        "lea.l (-16, %[s1], %[n]*4), %[s1];"
        "0:"
        "movem.l (%[s0]), %%d0-%%d3;"
        "movem.l (%[s1]), %%d4-%%d5/%%a0-%%a1;"
        "mac.l %%d0, %%a1, %%acc0;"
        "mac.l %%d1, %%a0, %%acc1;"
        "mac.l %%d2, %%d5, %%acc2;"
        "mac.l %%d3, %%d4, %%acc3;"
        "lea.l (16, %[s0]), %[s0];"
        "lea.l (-16, %[s1]), %[s1];"
        "movclr.l %%acc0, %%d0;"
        "movclr.l %%acc1, %%d1;"
        "movclr.l %%acc2, %%d2;"
        "movclr.l %%acc3, %%d3;"
        "movem.l %%d0-%%d3, (%[dst]);"
        "lea.l (16, %[dst]), %[dst];"
        "subq.l #4, %[n];"
        "jne 0b;"
        : [s0] "+a" (src0), [s1] "+a" (src1), [dst] "+a" (dst), [n] "+d" (len)
        : : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "memory", "cc");
}

#else
static inline void vector_fmul_add_add(real_t *dst, const real_t *src0, const real_t *src1, const real_t *src2, int len){
    int i;
    for(i=0; i<len; i++)
        dst[i] = MUL_F(src0[i], src1[i]) + src2[i];
}

static inline void vector_fmul_reverse(real_t *dst, const real_t *src0, const real_t *src1, int len){
    int i;
    src1 += len-1;
    for(i=0; i<len; i++)
        dst[i] = MUL_F(src0[i], src1[-i]);
}
#endif

#ifdef LTP_DEC
static INLINE void mdct(fb_info *fb, real_t *in_data, real_t *out_data, uint16_t len)
{
    mdct_info *mdct = NULL;

    switch (len)
    {
    case 2048:
    case 1920:
        mdct = fb->mdct2048;
        break;
    case 256:
    case 240:
        mdct = fb->mdct256;
        break;
#ifdef LD_DEC
    case 1024:
    case 960:
        mdct = fb->mdct1024;
        break;
#endif
    }

    faad_mdct(mdct, in_data, out_data);
}
#endif

ALIGN real_t transf_buf[2*1024] IBSS_ATTR;

void ifilter_bank(uint8_t window_sequence,
                  real_t *freq_in,
                  real_t *time_out, real_t *overlap,
                  uint8_t object_type, uint16_t frame_len)
{
    int16_t i;

    const real_t *window_long = NULL;
    const real_t *window_long_prev = NULL;
    const real_t *window_short = NULL;
    const real_t *window_short_prev = NULL;

    uint16_t nlong = frame_len;
    uint16_t nshort = frame_len/8;
    uint16_t trans = nshort/2;

    uint16_t nflat_ls = (nlong-nshort)/2;

#ifdef PROFILE
    int64_t count = faad_get_ts();
#endif

    memset(transf_buf,0,sizeof(transf_buf));
    /* select windows of current frame and previous frame (Sine or KBD) */
#ifdef LD_DEC
    if (object_type == LD)
    {
        window_long       = fb->ld_window[window_shape];
        window_long_prev  = fb->ld_window[window_shape_prev];
    } else {
#else
        (void) object_type;
#endif

        window_long       = sine_long_1024;
        window_long_prev  = kbd_long_1024;
        window_short      = sine_short_128;
        window_short_prev = kbd_short_128;

#ifdef LD_DEC
    }
#endif

#if 0
    for (i = 0; i < 1024; i++)
    {
        printf("%d\n", freq_in[i]);
    }
#endif

#if 0
    printf("%d %d\n", window_sequence, window_shape);
#endif
    switch (window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        /* perform iMDCT */
        mdct_backward(2048, freq_in, transf_buf);

        /* add second half output of previous frame to windowed output of current frame */
        vector_fmul_add_add(time_out, transf_buf, window_long_prev, overlap,  nlong);

        /* window the second half and save as overlap for next frame */
        vector_fmul_reverse(overlap, transf_buf+nlong, window_long, nlong);

        break;

    case LONG_START_SEQUENCE:
        /* perform iMDCT */
        mdct_backward(2048, freq_in, transf_buf);

        /* add second half output of previous frame to windowed output of current frame */
        vector_fmul_add_add(time_out, transf_buf, window_long_prev, overlap,  nlong);

        /* window the second half and save as overlap for next frame */
        /* construct second half window using padding with 1's and 0's */
        
        memcpy(overlap, transf_buf+nlong, nflat_ls*sizeof(real_t));

        vector_fmul_reverse(overlap+nflat_ls, transf_buf+nlong+nflat_ls, window_short, nshort);

        memset(overlap+nflat_ls+nshort, 0, nflat_ls*sizeof(real_t));
        break;

    case EIGHT_SHORT_SEQUENCE:
         /*this could be assemblerized too, but this case is extremely uncommon*/   
         
        /* perform iMDCT for each short block */
        mdct_backward(256, freq_in+0*nshort,  transf_buf+2*nshort*0);
        mdct_backward(256, freq_in+1*nshort, transf_buf+2*nshort*1);
        mdct_backward(256, freq_in+2*nshort, transf_buf+2*nshort*2);
        mdct_backward(256, freq_in+3*nshort, transf_buf+2*nshort*3);
        mdct_backward(256, freq_in+4*nshort, transf_buf+2*nshort*4);
        mdct_backward(256, freq_in+5*nshort, transf_buf+2*nshort*5);
        mdct_backward(256, freq_in+6*nshort, transf_buf+2*nshort*6);
        mdct_backward(256, freq_in+7*nshort, transf_buf+2*nshort*7);

        /* add second half output of previous frame to windowed output of current frame */
        for (i = 0; i < nflat_ls; i++)
            time_out[i] = overlap[i]; 
        for(i = 0; i < nshort; i++)
        {
            time_out[nflat_ls+         i] = overlap[nflat_ls+         i] + MUL_F(transf_buf[nshort*0+i],window_short_prev[i]);
            time_out[nflat_ls+1*nshort+i] = overlap[nflat_ls+nshort*1+i] + MUL_F(transf_buf[nshort*1+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*2+i],window_short[i]);
            time_out[nflat_ls+2*nshort+i] = overlap[nflat_ls+nshort*2+i] + MUL_F(transf_buf[nshort*3+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*4+i],window_short[i]);
            time_out[nflat_ls+3*nshort+i] = overlap[nflat_ls+nshort*3+i] + MUL_F(transf_buf[nshort*5+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*6+i],window_short[i]);
            if (i < trans)
                time_out[nflat_ls+4*nshort+i] = overlap[nflat_ls+nshort*4+i] + MUL_F(transf_buf[nshort*7+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*8+i],window_short[i]);
        }

        /* window the second half and save as overlap for next frame */
        for(i = 0; i < nshort; i++)
        {
            if (i >= trans)
                overlap[nflat_ls+4*nshort+i-nlong] = MUL_F(transf_buf[nshort*7+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*8+i],window_short[i]);
            overlap[nflat_ls+5*nshort+i-nlong] = MUL_F(transf_buf[nshort*9+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*10+i],window_short[i]);
            overlap[nflat_ls+6*nshort+i-nlong] = MUL_F(transf_buf[nshort*11+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*12+i],window_short[i]);
            overlap[nflat_ls+7*nshort+i-nlong] = MUL_F(transf_buf[nshort*13+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*14+i],window_short[i]);
            overlap[nflat_ls+8*nshort+i-nlong] = MUL_F(transf_buf[nshort*15+i],window_short[nshort-1-i]);
        }
        memset(overlap+nflat_ls+nshort, 0, nflat_ls*sizeof(real_t));

        break;

    case LONG_STOP_SEQUENCE:
        /* perform iMDCT */
        mdct_backward(2048, freq_in, transf_buf);

        /* add second half output of previous frame to windowed output of current frame */
        /* construct first half window using padding with 1's and 0's */
        memcpy(time_out, overlap, nflat_ls*sizeof(real_t));

        vector_fmul_add_add(time_out+nflat_ls, transf_buf+nflat_ls, window_short_prev, overlap+nflat_ls,  nshort);

        for (i = 0; i < nflat_ls; i++)
            time_out[nflat_ls+nshort+i] = overlap[nflat_ls+nshort+i] + transf_buf[nflat_ls+nshort+i];

        /* window the second half and save as overlap for next frame */
        vector_fmul_reverse(overlap, transf_buf+nlong, window_long, nlong);
        break;
    }

#if 0
    for (i = 0; i < 1024; i++)
    {
        printf("%d\n", time_out[i]);
        //printf("0x%.8X\n", time_out[i]);
    }
#endif


#ifdef PROFILE
    count = faad_get_ts() - count;
    fb->cycles += count;
#endif
}


#ifdef LTP_DEC
ALIGN real_t windowed_buf[2*1024] = {0};
/* only works for LTP -> no overlapping, no short blocks */
void filter_bank_ltp(fb_info *fb, uint8_t window_sequence, uint8_t window_shape,
                     uint8_t window_shape_prev, real_t *in_data, real_t *out_mdct,
                     uint8_t object_type, uint16_t frame_len)
{
    int16_t i;

    const real_t *window_long = NULL;
    const real_t *window_long_prev = NULL;
    const real_t *window_short = NULL;
    const real_t *window_short_prev = NULL;

    uint16_t nlong = frame_len;
    uint16_t nshort = frame_len/8;
    uint16_t nflat_ls = (nlong-nshort)/2;

    //assert(window_sequence != EIGHT_SHORT_SEQUENCE);

    memset(windowed_buf,0,sizeof(windowed_buf));
#ifdef LD_DEC
    if (object_type == LD)
    {
        window_long       = fb->ld_window[window_shape];
        window_long_prev  = fb->ld_window[window_shape_prev];
    } else {
#else
        (void) object_type;
#endif
        window_long       = fb->long_window[window_shape];
        window_long_prev  = fb->long_window[window_shape_prev];
        window_short      = fb->short_window[window_shape];
        window_short_prev = fb->short_window[window_shape_prev];
#ifdef LD_DEC
    }
#endif

    switch(window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        for (i = nlong-1; i >= 0; i--)
        {
            windowed_buf[i] = MUL_F(in_data[i], window_long_prev[i]);
            windowed_buf[i+nlong] = MUL_F(in_data[i+nlong], window_long[nlong-1-i]);
        }
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case LONG_START_SEQUENCE:
        for (i = 0; i < nlong; i++)
            windowed_buf[i] = MUL_F(in_data[i], window_long_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nlong] = in_data[i+nlong];
        for (i = 0; i < nshort; i++)
            windowed_buf[i+nlong+nflat_ls] = MUL_F(in_data[i+nlong+nflat_ls], window_short[nshort-1-i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nlong+nflat_ls+nshort] = 0;
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case LONG_STOP_SEQUENCE:
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i] = 0;
        for (i = 0; i < nshort; i++)
            windowed_buf[i+nflat_ls] = MUL_F(in_data[i+nflat_ls], window_short_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nflat_ls+nshort] = in_data[i+nflat_ls+nshort];
        for (i = 0; i < nlong; i++)
            windowed_buf[i+nlong] = MUL_F(in_data[i+nlong], window_long[nlong-1-i]);
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;
    }
}
#endif
