/*
 * WMA compatible codec
 * Copyright (c) 2002-2007 The FFmpeg Project
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

#include "wma.h"
#include "codeclib.h" /* needed for av_log2() */

/**
 *@brief Get the samples per frame for this stream.
 *@param sample_rate output sample_rate
 *@param version wma version
 *@param decode_flags codec compression features
 *@return log2 of the number of output samples per frame
 */
int ff_wma_get_frame_len_bits(int sample_rate, int version,
                                      unsigned int decode_flags)
{

    int frame_len_bits;

    if (sample_rate <= 16000) {
        frame_len_bits = 9;
    } else if (sample_rate <= 22050 ||
             (sample_rate <= 32000 && version == 1)) {
        frame_len_bits = 10;
    } else if (sample_rate <= 48000) {
        frame_len_bits = 11;
    } else if (sample_rate <= 96000) {
        frame_len_bits = 12;
    } else {
        frame_len_bits = 13;
    }

    if (version == 3) {
        int tmp = decode_flags & 0x6;
        if (tmp == 0x2) {
            ++frame_len_bits;
        } else if (tmp == 0x4) {
            --frame_len_bits;
        } else if (tmp == 0x6) {
            frame_len_bits -= 2;
        }
    }

    return frame_len_bits;
}

/**
 * Decode an uncompressed coefficient.
 * @param s codec context
 * @return the decoded coefficient
 */
unsigned int ff_wma_get_large_val(GetBitContext* gb)
{
    /** consumes up to 34 bits */
    int n_bits = 8;
    /** decode length */
    if (get_bits1(gb)) {
        n_bits += 8;
        if (get_bits1(gb)) {
            n_bits += 8;
            if (get_bits1(gb)) {
                n_bits += 7;
            }
        }
    }
    return get_bits_long(gb, n_bits);
}

/**
 * Decode run level compressed coefficients.
 * @param avctx codec context
 * @param gb bitstream reader context
 * @param vlc vlc table for get_vlc2
 * @param level_table level codes
 * @param run_table run codes
 * @param version 0 for wma1,2 1 for wmapro
 * @param ptr output buffer
 * @param offset offset in the output buffer
 * @param num_coefs number of input coefficents
 * @param block_len input buffer length (2^n)
 * @param frame_len_bits number of bits for escaped run codes
 * @param coef_nb_bits number of bits for escaped level codes
 * @return 0 on success, -1 otherwise
 */
#define av_log(...)
int ff_wma_run_level_decode(GetBitContext* gb,
                            VLC *vlc,
                            const int32_t *level_table, const uint16_t *run_table,
                            int version, int32_t *ptr, int offset,
                            int num_coefs, int block_len, int frame_len_bits,
                            int coef_nb_bits)
{
    int32_t code, level, sign;
    const unsigned int coef_mask = block_len - 1;
    /* Rockbox: To be able to use rockbox' optimized mdct we need to pre-shift
     * the values by >>(nbits-3). */
    const int nbits = av_log2(block_len)+1;
    const int shift = WMAPRO_FRACT-(nbits-3);
    for (; offset < num_coefs; offset++) {
        code = get_vlc2(gb, vlc->table, VLCBITS, VLCMAX);
        if (code > 1) {
            /** normal code */
            offset += run_table[code];
            sign = !get_bits1(gb);
            /* Rockbox: To be able to use rockbox' optimized mdct we need
             * invert the sign. */
            ptr[offset & coef_mask] = sign ? level_table[code] : -level_table[code];
            ptr[offset & coef_mask] <<= shift;
        } else if (code == 1) {
            /** EOB */
            break;
        } else {
            /** escape */
            if (!version) {
                level = get_bits(gb, coef_nb_bits);
                /** NOTE: this is rather suboptimal. reading
                    block_len_bits would be better */
                offset += get_bits(gb, frame_len_bits);
            } else {
                level = ff_wma_get_large_val(gb);
                /** escape decode */
                if (get_bits1(gb)) {
                    if (get_bits1(gb)) {
                        if (get_bits1(gb)) {
                            av_log(avctx,AV_LOG_ERROR,
                                "broken escape sequence\n");
                            return -1;
                        } else
                            offset += get_bits(gb, frame_len_bits) + 4;
                    } else
                        offset += get_bits(gb, 2) + 1;
                }
            }
            sign = !get_bits1(gb);
            ptr[offset & coef_mask] = sign ? level : -level;
            ptr[offset & coef_mask] <<= shift;
        }
    }
    /** NOTE: EOB can be omitted */
    if (offset > num_coefs) {
        av_log(avctx, AV_LOG_ERROR, "overflow in spectral RLE, ignoring\n");
        return -1;
    }

    return 0;
}
