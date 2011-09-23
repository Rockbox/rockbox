/**
 * @file
 * Common code for Vorbis I encoder and decoder
 * @author Denes Balatoni  ( dbalatoni programozo hu )
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* render_line and friend taken from ffmpeg (libavcodec/vorbis.c) */

#include "misc.h"

static inline void render_line_unrolled(int x, int y, int x1,
                                        int sy, int ady, int adx,
                                        const ogg_int32_t *lookup, ogg_int32_t *buf)
{
    int err = -adx;
    x -= x1 - 1;
    buf += x1 - 1;
    while (++x < 0) {
        err += ady;
        if (err >= 0) {
            err += ady - adx;
            y   += sy;
            buf[x] = MULT31_SHIFT15(buf[x],lookup[y]);
            x++;
        }
        buf[x] = MULT31_SHIFT15(buf[x],lookup[y]);
    }
    if (x <= 0) {
        if (err + ady >= 0)
            y += sy;
        buf[x] = MULT31_SHIFT15(buf[x],lookup[y]);
    }
}

static inline void render_line(int x0, int y0, int x1, int y1,
                               const ogg_int32_t *lookup, ogg_int32_t *buf)
{
    int dy  = y1 - y0;
    int adx = x1 - x0;
    int ady = abs(dy);
    int sy  = dy < 0 ? -1 : 1;
    buf[x0] = MULT31_SHIFT15(buf[x0],lookup[y0]);
    if (ady*2 <= adx) { // optimized common case
        render_line_unrolled(x0, y0, x1, sy, ady, adx, lookup, buf);
    } else {
        int base = dy / adx;
        int x    = x0;
        int y    = y0;
        int err  = -adx;
        ady -= abs(base) * adx;
        while (++x < x1) {
            y += base;
            err += ady;
            if (err >= 0) {
                err -= adx;
                y   += sy;
            }
            buf[x] = MULT31_SHIFT15(buf[x],lookup[y]);
        }
    }
}

#ifndef INCL_OPTIMIZED_VECTOR_FMUL_WINDOW
#define INCL_OPTIMIZED_VECTOR_FMUL_WINDOW
static inline void ff_vector_fmul_window_c(ogg_int32_t *dst, const ogg_int32_t *src0,
                                           const ogg_int32_t *src1, const ogg_int32_t *win, int len){
    int i,j;
    dst += len;
    win += len;
    src0+= len;
    for(i=-len, j=len-1; i<0; i++, j--) {
        ogg_int32_t s0 = src0[i];
        ogg_int32_t s1 = src1[j];
        ogg_int32_t wi = win[i];
        ogg_int32_t wj = win[j];
        XNPROD31(s0, s1, wj, wi, &dst[i], &dst[j]);
        /*
        dst[i] = MULT31(s0,wj) - MULT31(s1,wi);
        dst[j] = MULT31(s0,wi) + MULT31(s1,wj);
        */
    }
}
#endif

static inline void copy_normalize(ogg_int32_t *dst, ogg_int32_t *src, int len)
{
    memcpy(dst, src, len * sizeof(ogg_int32_t));
}

static inline void window_overlap_add(unsigned int blocksize, unsigned int lastblock,
                                      unsigned int bs0, unsigned int bs1, int ch,
                                      const ogg_int32_t *win, vorbis_dsp_state *v)
{
    unsigned retlen = (blocksize + lastblock) / 4;
    int j;
    for (j = 0; j < ch; j++) {
        ogg_int32_t *residue = v->residues[v->ri] + j * blocksize / 2;
        ogg_int32_t   *saved = v->saved_ptr[j];
        ogg_int32_t     *ret = v->floors + j * retlen;
        ogg_int32_t     *buf = residue;

        if (v->W == v->lW) {
            ff_vector_fmul_window_c(ret, saved, buf, win, blocksize / 4);
        } else if (v->W > v->lW) {
            ff_vector_fmul_window_c(ret, saved, buf, win, bs0 / 4);
            copy_normalize(ret+bs0/2, buf+bs0/4, (bs1-bs0)/4);
        } else {
            copy_normalize(ret, saved, (bs1 - bs0) / 4);
            ff_vector_fmul_window_c(ret + (bs1 - bs0) / 4, saved + (bs1 - bs0) / 4, buf, win, bs0 / 4);
        }
        if (v->residues[1] == NULL) {
            memcpy(saved, buf + blocksize / 4, blocksize / 4 * sizeof(ogg_int32_t));
            v->saved_ptr[j] = v->saved + j * bs1 / 4;
        } else {
            v->saved_ptr[j] = buf + blocksize / 4;
        }

        v->pcmb[j] = ret;
    }

    if (v->residues[1] != NULL) {
      v->ri ^= 1;
    }
}
