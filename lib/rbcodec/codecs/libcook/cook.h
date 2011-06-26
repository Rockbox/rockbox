/*
 * COOK compatible decoder
 * Copyright (c) 2003 Sascha Sommer
 * Copyright (c) 2005 Benjamin Larsson
 *
 * This file is taken from FFmpeg.
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
#ifndef _COOK_H
#define _COOK_H

#include <inttypes.h>
#include "ffmpeg_get_bits.h"
#include "../librm/rm.h"
#include "cookdata_fixpoint.h"

#include "codeclib.h"

#if (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024) || (CONFIG_CPU == MCF5250)
/* PP5022/24, MCF5250 have large IRAM */
#define IBSS_ATTR_COOK_LARGE_IRAM    IBSS_ATTR
#define ICODE_ATTR_COOK_LARGE_IRAM   ICODE_ATTR
#define ICONST_ATTR_COOK_LARGE_IRAM  ICONST_ATTR
#define IBSS_ATTR_COOK_VLCBUF
#define ICODE_ATTR_COOK_DECODE

#elif defined(CPU_S5L870X)
/* S5L870X have even larger IRAM and it is faster to use ICODE_ATTR. */
#define IBSS_ATTR_COOK_LARGE_IRAM    IBSS_ATTR
#define ICODE_ATTR_COOK_LARGE_IRAM   ICODE_ATTR
#define ICONST_ATTR_COOK_LARGE_IRAM  ICONST_ATTR
#define IBSS_ATTR_COOK_VLCBUF        IBSS_ATTR
#define ICODE_ATTR_COOK_DECODE       ICODE_ATTR

#else
/* other CPUs IRAM is not large enough */
#define IBSS_ATTR_COOK_LARGE_IRAM
#define ICODE_ATTR_COOK_LARGE_IRAM
#define ICONST_ATTR_COOK_LARGE_IRAM
#define IBSS_ATTR_COOK_VLCBUF
#define ICODE_ATTR_COOK_DECODE

#endif

typedef struct {
    int *now;
    int *previous;
} cook_gains;

typedef struct cook {
    /*
     * The following 2 functions provide the lowlevel arithmetic on
     * the internal audio buffers.
     */
    void (* scalar_dequant)(struct cook *q, int index, int quant_index,
                            int* subband_coef_index, int* subband_coef_sign,
                            REAL_T* mlt_p);

    void (* interpolate) (struct cook *q, REAL_T* buffer,
                          int gain_index, int gain_index_next);

    GetBitContext       gb;
    int                 frame_number;
    int                 block_align;
    int                 extradata_size;
    /* stream data */
    int                 nb_channels;
    int                 joint_stereo;
    int                 bit_rate;
    int                 sample_rate;
    int                 samples_per_channel;
    int                 samples_per_frame;
    int                 subbands;
    int                 log2_numvector_size;
    int                 numvector_size;                //1 << log2_numvector_size;
    int                 js_subband_start;
    int                 total_subbands;
    int                 num_vectors;
    int                 bits_per_subpacket;
    int                 cookversion;
    int                 mdct_nbits; /* is this the same as one of above? */
    /* states */
    int                 random_state;

    /* gain buffers */
    cook_gains          gains1;
    cook_gains          gains2;
    int                 gain_1[9];
    int                 gain_2[9];
    int                 gain_3[9];
    int                 gain_4[9];

    /* VLC data */
    int                 js_vlc_bits;
    VLC                 envelope_quant_index[13];
    VLC                 sqvh[7];          //scalar quantization
    VLC                 ccpl;             //channel coupling

    /* generatable tables and related variables */
    int                 gain_size_factor;

    /* data buffers */

    uint8_t             decoded_bytes_buffer[1024]  MEM_ALIGN_ATTR;
    REAL_T              mono_mdct_output[2048]      MEM_ALIGN_ATTR;
    REAL_T              mono_previous_buffer1[1024] MEM_ALIGN_ATTR;
    REAL_T              mono_previous_buffer2[1024] MEM_ALIGN_ATTR;
    REAL_T              decode_buffer_1[1024]       MEM_ALIGN_ATTR;
    REAL_T              decode_buffer_2[1024]       MEM_ALIGN_ATTR;
    /* static allocation for joint decode */
    REAL_T              decode_buffer_0[1060]       MEM_ALIGN_ATTR;
} COOKContext;

int cook_decode_init(RMContext *rmctx, COOKContext *q);
int cook_decode_frame(RMContext *rmctx,COOKContext *q,
                      int32_t *outbuffer, int *data_size,
                      const uint8_t *inbuffer, int buf_size);
#endif /*_COOK_H */
