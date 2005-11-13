/*
 * Shorten decoder
 * Copyright (c) 2005 Jeff Muizelaar
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
 */

/**
 * @file shorten.c
 * Shorten decoder
 * @author Jeff Muizelaar
 *
 */
 
#include "bitstream.h"
#include "golomb.h"
#include "shndec.h"

/* These seem reasonable from my test files.
   Does MAX_HEADER_SIZE really need to be 16384? */
#define MAX_PRED_ORDER 16
#define MAX_HEADER_SIZE DEFAULT_BLOCK_SIZE*4
//#define MAX_HEADER_SIZE 16384

#define ULONGSIZE 2

#define WAVE_FORMAT_PCM 0x0001

#define TYPESIZE     4
#define CHANSIZE     0
#define LPCQSIZE     2
#define ENERGYSIZE   3
#define BITSHIFTSIZE 2

#define TYPE_S16HL 3  /* signed 16 bit shorts: high-low */
#define TYPE_S16LH 5  /* signed 16 bit shorts: low-high */

#define NWRAP 3
#define NSKIPSIZE 1

#define LPCQUANT 5
#define V2LPCQOFFSET (1 << LPCQUANT)

#define FNSIZE       2
#define FN_DIFF0     0
#define FN_DIFF1     1
#define FN_DIFF2     2
#define FN_DIFF3     3
#define FN_QUIT      4
#define FN_BLOCKSIZE 5
#define FN_BITSHIFT  6
#define FN_QLPC      7
#define FN_ZERO      8
#define FN_VERBATIM  9

#define VERBATIM_CKSIZE_SIZE  5
#define VERBATIM_BYTE_SIZE    8
#define CANONICAL_HEADER_SIZE 44

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))

#define get_le16(gb) bswap_16(get_bits_long(gb, 16))
#define get_le32(gb) bswap_32(get_bits_long(gb, 32))

static inline uint32_t bswap_32(uint32_t x){
    x= ((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF);
    return (x>>16) | (x<<16);
}

static inline uint16_t bswap_16(uint16_t x){
    return (x>>8) | (x<<8);
}

/* converts fourcc string to int */
static inline int ff_get_fourcc(const char *s){
    //assert( strlen(s)==4 );
    return (s[0]) + (s[1]<<8) + (s[2]<<16) + (s[3]<<24);
}

static unsigned int get_uint(ShortenContext *s, int k) ICODE_ATTR;
static unsigned int get_uint(ShortenContext *s, int k)
{
    if (s->version != 0)
        k = get_ur_golomb_shorten(&s->gb, ULONGSIZE);
    return get_ur_golomb_shorten(&s->gb, k);
}

static void decode_subframe_lpc(ShortenContext *s, int32_t *decoded,
                                int residual_size, int pred_order) ICODE_ATTR;
static void decode_subframe_lpc(ShortenContext *s, int32_t *decoded,
                                int residual_size, int pred_order)
{
    int sum, i, j;
    int coeffs[MAX_PRED_ORDER];

    for (i=0; i<pred_order; i++) {
        coeffs[i] = get_sr_golomb_shorten(&s->gb, LPCQUANT);
    }

    for (i=0; i < s->blocksize; i++) {
        sum = s->lpcqoffset;
        for (j=0; j<pred_order; j++)
            sum += coeffs[j] * decoded[i-j-1];

        decoded[i] =
            get_sr_golomb_shorten(&s->gb, residual_size) + (sum >> LPCQUANT);
    }
}

int shorten_decode_frame(ShortenContext *s,
                         int32_t *decoded,
                         int32_t *offset,
                         uint8_t *buf,
                         int buf_size)
{
    int i;
    int32_t sum;

    init_get_bits(&s->gb, buf, buf_size*8);
    get_bits(&s->gb, s->bitindex);

    int cmd = get_ur_golomb_shorten(&s->gb, FNSIZE);
    switch (cmd) {
        case FN_ZERO:
        case FN_DIFF0:
        case FN_DIFF1:
        case FN_DIFF2:
        case FN_DIFF3:
        case FN_QLPC:
        {
            int residual_size = 0;
            int32_t coffset;
            if (cmd != FN_ZERO) {
                residual_size = get_ur_golomb_shorten(&s->gb, ENERGYSIZE);
                /* this is a hack as version 0 differed in defintion of
                   get_sr_golomb_shorten */
                if (s->version == 0)
                    residual_size--;
              }

            if (s->nmean == 0) {
                coffset = offset[0];
            } else {
                sum = (s->version < 2) ? 0 : s->nmean / 2;
                for (i=0; i<s->nmean; i++)
                    sum += offset[i];

                coffset = sum / s->nmean;
                if (s->version >= 2)
                    coffset >>= FFMIN(1, s->bitshift);
            }

            switch (cmd) {
                case FN_ZERO:
                    for (i=0; i<s->blocksize; i++)
                        decoded[i] = 0;
                    break;

                case FN_DIFF0:
                    for (i=0; i<s->blocksize; i++)
                        decoded[i] =
                            get_sr_golomb_shorten(&s->gb, residual_size) +
                            coffset;
                    break;

                case FN_DIFF1:
                    for (i=0; i<s->blocksize; i++)
                        decoded[i] =
                            get_sr_golomb_shorten(&s->gb, residual_size) +
                            decoded[i - 1];
                    break;

                case FN_DIFF2:
                    for (i=0; i<s->blocksize; i++)
                        decoded[i] =
                            get_sr_golomb_shorten(&s->gb, residual_size) +
                            2*decoded[i-1] - decoded[i-2];
                    break;

                case FN_DIFF3:
                    for (i=0; i<s->blocksize; i++)
                        decoded[i] =
                            get_sr_golomb_shorten(&s->gb, residual_size) +
                            3*decoded[i-1] - 3*decoded[i-2] + decoded[i-3];
                    break;

                case FN_QLPC:
                {
                    int pred_order = get_ur_golomb_shorten(&s->gb, LPCQSIZE);
                    if (pred_order > MAX_PRED_ORDER) {
                        return -2;
                    }

                    for (i=0; i<pred_order; i++)
                        decoded[i - pred_order] -= coffset;
                    decode_subframe_lpc(s, decoded, residual_size, pred_order);
                    if (coffset != 0) {
                        for (i=0; i < s->blocksize; i++)
                            decoded[i] += coffset;
                    }
                }
            }

            if (s->nmean > 0) {
                sum = (s->version < 2) ? 0 : s->blocksize / 2;
                for (i=0; i<s->blocksize; i++)
                    sum += decoded[i];

                for (i=1; i<s->nmean; i++)
                    offset[i-1] = offset[i];

                if (s->version < 2) {
                    offset[s->nmean - 1] = sum / s->blocksize;
                } else {
                    offset[s->nmean - 1] =
                        (sum / s->blocksize) << s->bitshift;
                }
            }

            for (i=-s->nwrap; i<0; i++)
                decoded[i] = decoded[i + s->blocksize];

            int scale = s->bitshift + SHN_OUTPUT_DEPTH - s->bits_per_sample;
            for (i = 0; i < s->blocksize; i++)
                decoded[i] <<= scale;
            break;
        }

        case FN_VERBATIM:
            i = get_ur_golomb_shorten(&s->gb, VERBATIM_CKSIZE_SIZE);
            while (i--)
                get_ur_golomb_shorten(&s->gb, VERBATIM_BYTE_SIZE);
            return 4;
            break;

        case FN_BITSHIFT:
            s->bitshift = get_ur_golomb_shorten(&s->gb, BITSHIFTSIZE);
            return 3;
            break;

        case FN_BLOCKSIZE:
            s->blocksize = get_uint(s, av_log2(s->blocksize));
            return 2;
            break;

        case FN_QUIT:
            return 1;
            break;

        default:
            return -1;
            break;
    }

    return 0;
}

static int decode_wave_header(ShortenContext *s,
                              uint8_t *header,
                              int header_size)
{
    GetBitContext hb;
    int len;

    init_get_bits(&hb, header, header_size*8);
    if (get_le32(&hb) != MKTAG('R','I','F','F')) {
        return -8;
    }

    int chunk_size = get_le32(&hb);

    if (get_le32(&hb) != MKTAG('W','A','V','E')) {
        return -9;
    }

    while (get_le32(&hb) != MKTAG('f','m','t',' ')) {
        len = get_le32(&hb);
        skip_bits(&hb, 8*len);
    }

    len = get_le32(&hb);
    if (len < 16) {
        return -10;
    }

    if (get_le16(&hb) != WAVE_FORMAT_PCM ) {
        return -11;
    }

    s->channels = get_le16(&hb);
    if (s->channels > MAX_CHANNELS) {
        return -3;
    }

    s->sample_rate = get_le32(&hb);

    skip_bits(&hb, 32);
    //s->bit_rate = 8*get_le32(&hb);

    int block_align = get_le16(&hb);
    s->totalsamples = (chunk_size - 36) / block_align;

    s->bits_per_sample = get_le16(&hb);
    if (s->bits_per_sample != 16) {
        return -12;
    }

    len -= 16;
    if (len > 0) {
        return len;
    }

    return 0;
}

int shorten_init(ShortenContext* s, uint8_t *buf, int buf_size)
{
    int i;

    s->blocksize = DEFAULT_BLOCK_SIZE;
    s->channels = 1;
    s->nmean = -1;

    init_get_bits(&s->gb, buf, buf_size*8);
    get_bits(&s->gb, s->bitindex);

    /* shorten signature */
    if (get_bits_long(&s->gb, 32) != bswap_32(ff_get_fourcc("ajkg"))) {
        return -1;
    }

    s->version = get_bits(&s->gb, 8);

    int internal_ftype = get_uint(s, TYPESIZE);
    if ((internal_ftype != TYPE_S16HL) && (internal_ftype != TYPE_S16LH)) {
        return -2;
    }

    s->channels = get_uint(s, CHANSIZE);
    if (s->channels > MAX_CHANNELS) {
        return -3;
    }

    /* get blocksize if version > 0 */
    int maxnlpc = 0;
    if (s->version > 0) {
        s->blocksize = get_uint(s, av_log2(DEFAULT_BLOCK_SIZE));
        maxnlpc = get_uint(s, LPCQSIZE);
        s->nmean = get_uint(s, 0);

        int skip_bytes = get_uint(s, NSKIPSIZE);
        for (i=0; i<skip_bytes; i++) {
            skip_bits(&s->gb, 8);
        }
    }

    if (s->nmean > MAX_NMEAN) {
        return -4;
    }

    s->nwrap = FFMAX(NWRAP, maxnlpc);
    if (s->nwrap > MAX_NWRAP) {
        return -5;
    }

    if (s->version > 1)
        s->lpcqoffset = V2LPCQOFFSET;

    if (get_ur_golomb_shorten(&s->gb, FNSIZE) != FN_VERBATIM) {
        return -6;
    }

    uint8_t header[MAX_HEADER_SIZE];
    int header_size = get_ur_golomb_shorten(&s->gb, VERBATIM_CKSIZE_SIZE);
    if (header_size >= MAX_HEADER_SIZE || header_size < CANONICAL_HEADER_SIZE) {
        return -7;
    }

    for (i=0; i<header_size; i++)
        header[i] = (char)get_ur_golomb_shorten(&s->gb, VERBATIM_BYTE_SIZE);

    s->header_bits = s->gb.index;

    return decode_wave_header(s, header, header_size);
}
