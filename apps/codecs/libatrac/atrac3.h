/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Mohamed Tarek
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

#include "ffmpeg_bitstream.h"
#include "../librm/rm.h"
#ifdef ROCKBOX
#include "codeclib.h"
#endif

#if (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024) || (CONFIG_CPU == MCF5250)
/* PP5022/24 and MCF5250 have larger IRAM */
#define IBSS_ATTR_LARGE_IRAM  IBSS_ATTR
#define ICODE_ATTR_LARGE_IRAM ICODE_ATTR
#else
/* other CPUs IRAM is not large enough */
#define IBSS_ATTR_LARGE_IRAM
#define ICODE_ATTR_LARGE_IRAM
#endif

/* These structures are needed to store the parsed gain control data. */
typedef struct {
    int   num_gain_data;
    int   levcode[8];
    int   loccode[8];
} gain_info;

typedef struct {
    gain_info   gBlock[4];
} gain_block;

typedef struct {
    int     pos;
    int     numCoefs;
    int32_t coef[8];
} tonal_component;

typedef struct {
    int               bandsCoded;
    int               numComponents;
    tonal_component   components[64];
    int32_t           *prevFrame;
    int               gcBlkSwitch;
    gain_block        gainBlock[2];

    int32_t           *spectrum;
    int32_t           *IMDCT_buf;

    int32_t           delayBuf1[46]; ///<qmf delay buffers
    int32_t           delayBuf2[46];
    int32_t           delayBuf3[46];
} channel_unit;

typedef struct {
    GetBitContext       gb;
    //@{
    /** stream data */
    int                 channels;
    int                 codingMode;
    int                 bit_rate;
    int                 sample_rate;
    int                 samples_per_channel;
    int                 samples_per_frame;

    int                 bits_per_frame;
    int                 bytes_per_frame;
    int                 pBs;
    channel_unit*       pUnits;
    //@}
    //@{
    /** joint-stereo related variables */
    int                 matrix_coeff_index_prev[4];
    int                 matrix_coeff_index_now[4];
    int                 matrix_coeff_index_next[4];
    int                 weighting_delay[6];
    //@}
    //@{
    /** data buffers */
    int32_t             outSamples[2048];
    uint8_t             decoded_bytes_buffer[1024];
    int32_t             tempBuf[1070];
    //@}
    //@{
    /** extradata */
    int                 atrac3version;
    int                 delay;
    int                 scrambled_stream;
    int                 frame_factor;
    //@}
} ATRAC3Context;

int atrac3_decode_init(ATRAC3Context *q, struct mp3entry *id3);

int atrac3_decode_frame(unsigned long block_align, ATRAC3Context *q,
                        int *data_size, const uint8_t *buf, int buf_size);

