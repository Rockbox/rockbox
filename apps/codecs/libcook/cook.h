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

#include <stdint.h>
#include "libavutil/lfg.h"
#include "bitstream.h"
#include "dsputil.h"
#include "bytestream.h"
#include "rm2wav.h"
#include "cookdata_fixpoint.h"

typedef struct {
    int *now;
    int *previous;
} cook_gains;

typedef struct cook {
    /*
     * The following 5 functions provide the lowlevel arithmetic on
     * the internal audio buffers.
     */
    void (* scalar_dequant)(struct cook *q, int index, int quant_index,
                            int* subband_coef_index, int* subband_coef_sign,
                            REAL_T* mlt_p);

    void (* decouple) (struct cook *q,
                       int subband,
                       REAL_T f1, REAL_T f2,
                       REAL_T *decode_buffer,
                       REAL_T *mlt_buffer1, REAL_T *mlt_buffer2);

    void (* imlt_window) (struct cook *q, float *buffer1,
                          cook_gains *gains_ptr, float *previous_buffer);

    void (* interpolate) (struct cook *q, REAL_T* buffer,
                          int gain_index, int gain_index_next);

    void (* saturate_output) (struct cook *q, int chan, int16_t *out);

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
    /* states */
    AVLFG               random_state;
    /* transform data */
    MDCTContext         mdct_ctx;
    float*              mlt_window;

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
    float               gain_table[23];

    /* data buffers */

    uint8_t*            decoded_bytes_buffer;
    REAL_T mono_mdct_output[2048] __attribute__ ((aligned(16))); //DECLARE_ALIGNED_16(float,mono_mdct_output[2048]);
    REAL_T              mono_previous_buffer1[1024];
    REAL_T              mono_previous_buffer2[1024];
    REAL_T              decode_buffer_1[1024];
    REAL_T              decode_buffer_2[1024];
    REAL_T              decode_buffer_0[1060]; /* static allocation for joint decode */

    const float         *cplscales[5];
} COOKContext;

av_cold int cook_decode_init(RMContext *rmctx, COOKContext *q);
int cook_decode_frame(RMContext *rmctx,COOKContext *q,
                      int16_t *outbuffer, int *data_size,
                      const uint8_t *inbuffer, int buf_size);
av_cold int cook_decode_close(COOKContext *q);
#endif
