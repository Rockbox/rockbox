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
static inline void render_line_unrolled(int x, int y, int x1,
                                        int sy, int ady, int adx,
                                        ogg_int32_t *buf)
{
    int err = -adx;
    x -= x1 - 1;
    buf += x1 - 1;
    while (++x < 0) {
        err += ady;
        if (err >= 0) {
            err += ady - adx;
            y   += sy;
            buf[x] = MULT31_SHIFT15(buf[x],FLOOR_fromdB_LOOKUP[y]);
            x++;
        }
        buf[x] = MULT31_SHIFT15(buf[x],FLOOR_fromdB_LOOKUP[y]);
    }
    if (x <= 0) {
        if (err + ady >= 0)
            y += sy;
        buf[x] = MULT31_SHIFT15(buf[x],FLOOR_fromdB_LOOKUP[y]);
    }
}

static void render_line(int x0, int y0, int x1, int y1, ogg_int32_t *buf)
{
    int dy  = y1 - y0;
    int adx = x1 - x0;
    int ady = abs(dy);
    int sy  = dy < 0 ? -1 : 1;
    buf[x0] = MULT31_SHIFT15(buf[x0],FLOOR_fromdB_LOOKUP[y0]);
    if (ady*2 <= adx) { // optimized common case
        render_line_unrolled(x0, y0, x1, sy, ady, adx, buf);
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
            buf[x] = MULT31_SHIFT15(buf[x],FLOOR_fromdB_LOOKUP[y]);
        }
    }
}
