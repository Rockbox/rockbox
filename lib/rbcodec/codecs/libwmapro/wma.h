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

#ifndef AVCODEC_WMA_H
#define AVCODEC_WMA_H

#include "ffmpeg_get_bits.h"
#include "ffmpeg_put_bits.h"

#define WMAPRO_FRACT (17)
#define WMAPRO_DSP_SAMPLE_DEPTH (WMAPRO_FRACT + 8)

/* size of blocks */
#define BLOCK_MIN_BITS 7
#define BLOCK_MAX_BITS 11
#define BLOCK_MAX_SIZE (1 << BLOCK_MAX_BITS)

#define BLOCK_NB_SIZES (BLOCK_MAX_BITS - BLOCK_MIN_BITS + 1)

/* XXX: find exact max size */
#define HIGH_BAND_MAX_SIZE 16

#define NB_LSP_COEFS 10

/* XXX: is it a suitable value ? */
#define MAX_CODED_SUPERFRAME_SIZE 16384

#define MAX_CHANNELS 2

#define NOISE_TAB_SIZE 8192

#define LSP_POW_BITS 7

//FIXME should be in wmadec
#define VLCBITS 9
#define VLCMAX ((22+VLCBITS-1)/VLCBITS)

typedef struct CoefVLCTable {
    int n;                      ///< total number of codes
    int max_level;
    const uint32_t *huffcodes;  ///< VLC bit values
    const uint8_t *huffbits;    ///< VLC bit size
    const uint16_t *levels;     ///< table to build run/level tables
} CoefVLCTable;


int ff_wma_get_frame_len_bits(int sample_rate, int version,
                                      unsigned int decode_flags);
unsigned int ff_wma_get_large_val(GetBitContext* gb);
int ff_wma_run_level_decode(GetBitContext* gb,
                            VLC *vlc,
                            const int32_t *level_table, const uint16_t *run_table,
                            int version, int32_t *ptr, int offset,
                            int num_coefs, int block_len, int frame_len_bits,
                            int coef_nb_bits);

#endif /* AVCODEC_WMA_H */
