/*
 * exp golomb vlc stuff
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2004 Alex Beregszaszi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <limits.h>
#include "codeclib.h"

/**
 * @file golomb.h
 * @brief 
 *     exp golomb vlc stuff
 * @author Michael Niedermayer <michaelni@gmx.at> and Alex Beregszaszi
 */

 
/**
 * read unsigned golomb rice code (jpegls).
 */
static inline int get_ur_golomb_jpegls(GetBitContext *gb, int k, int limit,
                                       int esc_len)
{
    unsigned int buf;
    int log;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    log = av_log2(buf);

    if (log - k >= 32 - MIN_CACHE_BITS + (MIN_CACHE_BITS == 32) &&
        32 - log < limit) {
        buf >>= log - k;
        buf  += (30U - log) << k;
        LAST_SKIP_BITS(re, gb, 32 + k - log);
        CLOSE_READER(re, gb);

        return buf;
    } else {
        int i;
        for (i = 0; i + MIN_CACHE_BITS <= limit && SHOW_UBITS(re, gb, MIN_CACHE_BITS) == 0; i += MIN_CACHE_BITS) {
            if (gb->size_in_bits <= (signed) re_index) {
                CLOSE_READER(re, gb);
                return -1;
            }
            LAST_SKIP_BITS(re, gb, MIN_CACHE_BITS);
            UPDATE_CACHE(re, gb);
        }
        for (; i < limit && SHOW_UBITS(re, gb, 1) == 0; i++) {
            SKIP_BITS(re, gb, 1);
        }
        LAST_SKIP_BITS(re, gb, 1);
        UPDATE_CACHE(re, gb);

        if (i < limit - 1) {
            if (k) {
                if (k > MIN_CACHE_BITS - 1) {
                    buf = SHOW_UBITS(re, gb, 16) << (k-16);
                    LAST_SKIP_BITS(re, gb, 16);
                    UPDATE_CACHE(re, gb);
                    buf |= SHOW_UBITS(re, gb, k-16);
                    LAST_SKIP_BITS(re, gb, k-16);
                } else {
                    buf = SHOW_UBITS(re, gb, k);
                    LAST_SKIP_BITS(re, gb, k);
                }
            } else {
                buf = 0;
            }

            buf += ((int32_t)i << k);
        } else if (i == limit - 1) {
            buf = SHOW_UBITS(re, gb, esc_len);
            LAST_SKIP_BITS(re, gb, esc_len);

            buf ++;
        } else {
            buf = -1;
        }
        CLOSE_READER(re, gb);
        return buf;
    }
}

/**
 * read signed golomb rice code (flac).
 */
static inline int get_sr_golomb_flac(GetBitContext *gb, int k, int limit, int esc_len){
    int v= get_ur_golomb_jpegls(gb, k, limit, esc_len);
    return (v>>1) ^ -(v&1);
}

/**
 * read unsigned golomb rice code (shorten).
 */
#define get_ur_golomb_shorten(gb, k) get_ur_golomb_jpegls(gb, k, INT_MAX, 0)
/*
static inline unsigned int get_ur_golomb_shorten(GetBitContext *gb, int k){
    return get_ur_golomb_jpegls(gb, k, INT_MAX, 0);
}
*/

/**
 * read signed golomb rice code (shorten).
 */
static inline int get_sr_golomb_shorten(GetBitContext* gb, int k)
{
    int uvar = get_ur_golomb_jpegls(gb, k + 1, INT_MAX, 0);
    return (uvar >> 1) ^ -(uvar & 1);
}
