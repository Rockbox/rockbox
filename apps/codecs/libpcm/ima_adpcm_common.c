/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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
#include "codeclib.h"
#include "pcm_common.h"
#include "ima_adpcm_common.h"

/*
 * Functions for IMA ADPCM and IMA ADPCM series format
 *
 * References
 * [1] The IMA Digital Audio Focus and Technical Working Groups,
 *     Recommended Practices for Enhancing Digital Audio Compatibility
 *     in Multimedia Systems Revision 3.00, 1992
 * [2] Microsoft Corporation, New Multimedia Data Types and Data Techniques,
 *     Revision:3.0, 1994
 * [3] ffmpeg source code, libavcodec/adpcm.c
 */

/* step table */
static const uint16_t step_table[89] ICONST_ATTR = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

/* step index tables */
static const int index_tables[4][16] ICONST_ATTR = {
    /* adpcm data size is 2 */
    { -1, 2 },
    /* adpcm data size is 3 */
    { -1, -1, 1, 2 },
    /* adpcm data size is 4 */
    { -1, -1, -1, -1, 2, 4, 6, 8 },
    /* adpcm data size is 5 */
    { -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16 },
};

static int32_t pcmdata[2];
static int8_t index[2];

static int adpcm_data_size;
static uint8_t step_mask;
static uint8_t step_sign_mask;
static int8_t step_shift;
static const int *use_index_table;

/*
 * Before first decoding, this function must be executed.
 *
 * params
 *     bit: adpcm data size (2 <= bit <= 5).
 *     index_table: step index table
 *                  if index_table is null, then step index table
 *                  is used index_tables[bit-2].
 */
void init_ima_adpcm_decoder(int bit, const int *index_table)
{
    adpcm_data_size = bit;
    step_sign_mask = 1 << (adpcm_data_size - 1);
    step_mask = step_sign_mask - 1;
    step_shift = adpcm_data_size - 2;
    if (index_table)
        use_index_table = index_table;
    else
        use_index_table = index_tables[adpcm_data_size - 2];
}

/*
 * When starting decoding for each block, this function must be executed.
 *
 * params
 *     channels: channel count
 *     init_pcmdata: array of init pcmdata
 *     init_index:   array of init step indexes
 */
void set_decode_parameters(int channels, int32_t *init_pcmdata, int8_t *init_index)
{
    int ch;

    for (ch = 0; ch < channels; ch++)
    {
        pcmdata[ch] = init_pcmdata[ch];
        index[ch] = init_index[ch];
    }
}

/*
 * convert ADPCM to PCM for any adpcm data size.
 *
 * If adpcm_data_size is 4, then you use create_pcmdata_size4()
 * in place of this functon.
 */
int16_t create_pcmdata(int ch, uint8_t nibble)
{
    int check_bit = 1 << step_shift;
    int32_t delta = 0;
    int16_t step = step_table[index[ch]];

    do {
        if (nibble & check_bit)
            delta += step;
        step >>= 1;
        check_bit >>= 1;
    } while (check_bit);
    delta += step;

    if (nibble & step_sign_mask)
        pcmdata[ch] -= delta;
    else
        pcmdata[ch] += delta;

    index[ch] += use_index_table[nibble & step_mask];
    CLIP(index[ch], 0, 88);

    CLIP(pcmdata[ch], -32768, 32767);

    return (int16_t)pcmdata[ch];
}

/*
 * convert ADPCM to PCM when adpcm data size is 4.
 */
int16_t create_pcmdata_size4(int ch, uint8_t nibble)
{
    int32_t delta;
    int16_t step = step_table[index[ch]];

    delta = (step >> 3);
    if (nibble & 4) delta += step;
    if (nibble & 2) delta += (step >> 1);
    if (nibble & 1) delta += (step >> 2);

    if (nibble & 0x08)
        pcmdata[ch] -= delta;
    else
        pcmdata[ch] += delta;

    index[ch] += use_index_table[nibble & 0x07];
    CLIP(index[ch], 0, 88);

    CLIP(pcmdata[ch], -32768, 32767);

    return (int16_t)pcmdata[ch];
}
