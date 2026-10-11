/*
 * Direct Stream Transfer decoder adapted from FFmpeg's dstdec.c.
 * Copyright (c) 2014 Peter Ross <pross@xvid.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#ifndef DST_DECODER_H
#define DST_DECODER_H

#include "lib/ffmpeg_get_bits.h"
#include "libffmpegFLAC/golomb.h"

#define DST_CHANNELS 2
#define DST_ELEMENTS (DST_CHANNELS * 2)
#define DST_FRAME_BYTES 4704

struct dst_arith
{
    unsigned int a;
    unsigned int c;
};

struct dst_table
{
    unsigned int elements;
    unsigned int length[DST_ELEMENTS];
    int coeff[DST_ELEMENTS][128];
};

struct dst_decoder
{
    GetBitContext gb;
    struct dst_arith ac;
    struct dst_table fsets;
    struct dst_table probs;
    uint32_t status[DST_CHANNELS][4];
    int16_t filter[DST_ELEMENTS][16][256];
};

static const int8_t dst_fset_pred[3][3] = {
    { -8 }, { -16, 8 }, { -9, -5, 6 }
};

static const int8_t dst_prob_pred[3][3] = {
    { -8 }, { -16, 8 }, { -24, 24, -8 }
};

static int dst_read_map(GetBitContext *gb, struct dst_table *table,
                        unsigned int map[DST_CHANNELS])
{
    table->elements = 1;
    map[0] = 0;
    if (!get_bits1(gb))
    {
        for (int ch = 1; ch < DST_CHANNELS; ++ch)
        {
            int bits = av_log2(table->elements) + 1;
            map[ch] = get_bits(gb, bits);
            if (map[ch] == table->elements)
            {
                if (++table->elements >= DST_ELEMENTS)
                    return -1;
            }
            else if (map[ch] > table->elements)
                return -1;
        }
    }
    else
        ci->memset(map, 0, sizeof(unsigned int) * DST_CHANNELS);
    return 0;
}

static int dst_signed_rice(GetBitContext *gb, unsigned int k)
{
    int value = get_ur_golomb_jpegls(gb, k, get_bits_left(gb), 0);
    if (value < 0)
        return INT_MIN;
    if (value && get_bits1(gb))
        value = -value;
    return value;
}

static int dst_read_plain(GetBitContext *gb, int *dst, unsigned int count,
                          int bits, bool is_signed, int offset)
{
    if (get_bits_left(gb) < (int)(count * bits))
        return -1;
    for (unsigned int i = 0; i < count; ++i)
        dst[i] = (is_signed ? get_sbits(gb, bits) : (int)get_bits(gb, bits)) + offset;
    return 0;
}

static int dst_read_table(GetBitContext *gb, struct dst_table *table,
                          const int8_t predictor[3][3], int length_bits,
                          int coeff_bits, bool is_signed, int offset)
{
    for (unsigned int i = 0; i < table->elements; ++i)
    {
        if (get_bits_left(gb) < length_bits + 1)
            return -1;
        table->length[i] = get_bits(gb, length_bits) + 1;
        if (table->length[i] > 128)
            return -1;
        if (!get_bits1(gb))
        {
            if (dst_read_plain(gb, table->coeff[i], table->length[i],
                               coeff_bits, is_signed, offset) < 0)
                return -1;
            continue;
        }

        int method = get_bits(gb, 2);
        if (method == 3 ||
            dst_read_plain(gb, table->coeff[i], method + 1,
                           coeff_bits, is_signed, offset) < 0)
            return -1;
        int lsb_size = get_bits(gb, 3);
        for (unsigned int j = method + 1; j < table->length[i]; ++j)
        {
            int predicted = 0;
            for (int k = 0; k < method + 1; ++k)
                predicted += predictor[method][k] *
                             (unsigned int)table->coeff[i][j - k - 1];
            int value = dst_signed_rice(gb, lsb_size);
            if (value == INT_MIN)
                return -1;
            if (predicted >= 0)
                value -= (predicted + 4) / 8;
            else
                value += (-predicted + 3) / 8;
            if (!is_signed &&
                (value < offset || value >= offset + (1 << coeff_bits)))
                return -1;
            table->coeff[i][j] = value;
        }
    }
    return 0;
}

static void dst_ac_init(struct dst_arith *ac, GetBitContext *gb)
{
    ac->a = 4095;
    ac->c = get_bits(gb, 12);
}

static inline int dst_ac_get(struct dst_arith *ac, GetBitContext *gb,
                             int probability)
{
    unsigned int k = (ac->a >> 8) | ((ac->a >> 7) & 1);
    unsigned int q = k * probability;
    unsigned int remaining = ac->a - q;
    int value = ac->c < remaining;
    if (value)
        ac->a = remaining;
    else
    {
        ac->a = q;
        ac->c -= remaining;
    }
    if (ac->a < 2048)
    {
        int bits = 11 - av_log2(ac->a);
        ac->a <<= bits;
        ac->c = (ac->c << bits) | get_bits(gb, bits);
    }
    return value;
}

static unsigned char dst_probability(int value)
{
    unsigned int bits = value & 127;
    bits = ((bits & 0x55) << 1) | ((bits >> 1) & 0x55);
    bits = ((bits & 0x33) << 2) | ((bits >> 2) & 0x33);
    bits = ((bits & 0x0f) << 4) | ((bits >> 4) & 0x0f);
    return (bits >> 1) + 1;
}

static int dst_build_filter(struct dst_decoder *decoder)
{
    for (unsigned int i = 0; i < decoder->fsets.elements; ++i)
    {
        int length = decoder->fsets.length[i];
        for (int j = 0; j < 16; ++j)
        {
            int total = MIN(MAX(length - j * 8, 0), 8);
            for (int value = 0; value < 256; ++value)
            {
                int64_t sum = 0;
                for (int bit = 0; bit < total; ++bit)
                    sum += (((value >> bit) & 1) * 2 - 1) *
                           decoder->fsets.coeff[i][j * 8 + bit];
                if ((int16_t)sum != sum)
                    return -1;
                decoder->filter[i][j][value] = sum;
            }
        }
    }
    return 0;
}

static inline void dst_push_status(uint32_t status[4], int value)
{
    status[3] = (status[3] << 1) | (status[2] >> 31);
    status[2] = (status[2] << 1) | (status[1] >> 31);
    status[1] = (status[1] << 1) | (status[0] >> 31);
    status[0] = (status[0] << 1) | value;
}

static inline int dst_predict(int16_t filter[16][256],
                              const uint32_t words[4])
{
    const unsigned char *status = (const unsigned char *)words;
    return filter[0][status[0]] + filter[1][status[1]] +
           filter[2][status[2]] + filter[3][status[3]] +
           filter[4][status[4]] + filter[5][status[5]] +
           filter[6][status[6]] + filter[7][status[7]] +
           filter[8][status[8]] + filter[9][status[9]] +
           filter[10][status[10]] + filter[11][status[11]] +
           filter[12][status[12]] + filter[13][status[13]] +
           filter[14][status[14]] + filter[15][status[15]];
}

static inline __attribute__((always_inline))
int dst_decode_channel(struct dst_decoder *decoder,
                       int16_t filter[16][256],
                       const int *probabilities, unsigned int probability_count,
                       uint32_t status[4])
{
    int predict = dst_predict(filter, status);
    unsigned int index = abs(predict) >> 3;
    index = MIN(index, probability_count - 1);
    int residual = dst_ac_get(&decoder->ac, &decoder->gb,
                              probabilities[index]);
    int value = ((predict >> 15) ^ residual) & 1;
    dst_push_status(status, value);
    return value;
}

static int __attribute__((optimize("O3")))
dst_decode(struct dst_decoder *decoder, const unsigned char *input,
           size_t input_size, unsigned char *output)
{
    unsigned int fset_map[DST_CHANNELS];
    unsigned int prob_map[DST_CHANNELS];
    unsigned int half_prob[DST_CHANNELS];
    const unsigned int sample_count = DST_FRAME_BYTES * 8;

    if (input_size <= 1 || input_size > 65536)
        return -1;
    init_get_bits(&decoder->gb, input, input_size * 8);
    if (!get_bits1(&decoder->gb))
    {
        skip_bits1(&decoder->gb);
        if (get_bits(&decoder->gb, 6) || input_size - 1 < DST_FRAME_BYTES * 2)
            return -1;
        ci->memcpy(output, input + 1, DST_FRAME_BYTES * 2);
        return DST_FRAME_BYTES * 2;
    }

    if (!get_bits1(&decoder->gb) || !get_bits1(&decoder->gb) ||
        !get_bits1(&decoder->gb))
        return -1;
    int same_map = get_bits1(&decoder->gb);
    if (dst_read_map(&decoder->gb, &decoder->fsets, fset_map) < 0)
        return -1;
    if (same_map)
    {
        decoder->probs.elements = decoder->fsets.elements;
        ci->memcpy(prob_map, fset_map, sizeof(prob_map));
    }
    else if (dst_read_map(&decoder->gb, &decoder->probs, prob_map) < 0)
        return -1;

    for (int ch = 0; ch < DST_CHANNELS; ++ch)
        half_prob[ch] = get_bits1(&decoder->gb);
    if (dst_read_table(&decoder->gb, &decoder->fsets, dst_fset_pred,
                       7, 9, true, 0) < 0 ||
        dst_read_table(&decoder->gb, &decoder->probs, dst_prob_pred,
                       6, 7, false, 1) < 0 ||
        get_bits1(&decoder->gb) || get_bits_left(&decoder->gb) < 12)
        return -1;

    dst_ac_init(&decoder->ac, &decoder->gb);
    if (dst_build_filter(decoder) < 0)
        return -1;
    ci->memset(decoder->status, 0xaa, sizeof(decoder->status));
    ci->memset(output, 0, DST_FRAME_BYTES * DST_CHANNELS);
    dst_ac_get(&decoder->ac, &decoder->gb,
               dst_probability(decoder->fsets.coeff[0][0]));

    unsigned int output_pos = 0;
    unsigned int packed[DST_CHANNELS] = { 0, 0 };
    unsigned int fast_start = MAX(decoder->fsets.length[fset_map[0]],
                                  decoder->fsets.length[fset_map[1]]);
    for (unsigned int sample = 0; sample < fast_start; ++sample)
    {
        for (int ch = 0; ch < DST_CHANNELS; ++ch)
        {
            unsigned int felem = fset_map[ch];
            uint32_t *status = decoder->status[ch];
            int predict = dst_predict(decoder->filter[felem], status);
            int probability = 128;
            if (!half_prob[ch] || sample >= decoder->fsets.length[felem])
            {
                unsigned int pelem = prob_map[ch];
                unsigned int index = abs(predict) >> 3;
                index = MIN(index, decoder->probs.length[pelem] - 1);
                probability = decoder->probs.coeff[pelem][index];
            }
            int residual = dst_ac_get(&decoder->ac, &decoder->gb, probability);
            int value = ((predict >> 15) ^ residual) & 1;
            packed[ch] = (packed[ch] << 1) | value;
            dst_push_status(status, value);
        }
        if ((sample & 7) == 7)
        {
            output[output_pos++] = packed[0];
            output[output_pos++] = packed[1];
            packed[0] = packed[1] = 0;
        }
    }

    int16_t (*filter0)[256] = decoder->filter[fset_map[0]];
    int16_t (*filter1)[256] = decoder->filter[fset_map[1]];
    const int *prob0 = decoder->probs.coeff[prob_map[0]];
    const int *prob1 = decoder->probs.coeff[prob_map[1]];
    unsigned int prob0_count = decoder->probs.length[prob_map[0]];
    unsigned int prob1_count = decoder->probs.length[prob_map[1]];
    uint32_t *status0 = decoder->status[0];
    uint32_t *status1 = decoder->status[1];

    for (unsigned int sample = fast_start; sample < sample_count; ++sample)
    {
        packed[0] = (packed[0] << 1) |
                    dst_decode_channel(decoder, filter0, prob0,
                                       prob0_count, status0);
        packed[1] = (packed[1] << 1) |
                    dst_decode_channel(decoder, filter1, prob1,
                                       prob1_count, status1);
        if ((sample & 7) == 7)
        {
            output[output_pos++] = packed[0];
            output[output_pos++] = packed[1];
            packed[0] = packed[1] = 0;
        }
    }
    return DST_FRAME_BYTES * DST_CHANNELS;
}

#endif
