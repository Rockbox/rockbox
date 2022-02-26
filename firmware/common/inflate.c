/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by James Buren (libflate adaptations for RockBox)
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

/* Copyright 2021 Plan 9 Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "inflate.h"
#include <stdbool.h>
#include <string.h>
#include "adler32.h"
#include "crc32.h"
#include "system.h"

enum {
    INFLATE_BUFFER_SIZE = 32768,
    INFLATE_SYMBOL_MAX = 288,
    INFLATE_OFFSET_MAX = 32,
    INFLATE_CODELEN_MAX = 19,
    INFLATE_HUFF_BITS = 17,
    INFLATE_FLAT_BITS = 7,
    INFLATE_SYMBOL_BITS = 7,
    INFLATE_OFFSET_BITS = 6,
    INFLATE_CODELEN_BITS = 6,
};

struct inflate_huff {
    uint32_t max_bits;
    uint32_t min_bits;
    uint32_t flat_mask;
    uint32_t flat[1 << INFLATE_FLAT_BITS];
    uint32_t max_code[INFLATE_HUFF_BITS];
    uint32_t last[INFLATE_HUFF_BITS];
    uint32_t decode[INFLATE_SYMBOL_MAX];
};

struct inflate {
    uint8_t in[INFLATE_BUFFER_SIZE];
    uint8_t out[INFLATE_BUFFER_SIZE];
    union {
        struct {
            uint8_t len[INFLATE_SYMBOL_MAX];
            uint8_t off[INFLATE_OFFSET_MAX];
        };
        uint8_t combo[INFLATE_SYMBOL_MAX + INFLATE_OFFSET_MAX];
    };
    uint8_t clen[INFLATE_CODELEN_MAX];
    struct inflate_huff lentab;
    struct inflate_huff offtab;
    struct inflate_huff clentab;
    uint32_t bits[INFLATE_HUFF_BITS];
    uint32_t codes[INFLATE_HUFF_BITS];
};

#define INFLATE_FILL(E) do {                               \
    const uint32_t _size = read(is, sizeof(it->in), rctx); \
    if (_size == 0) {                                      \
        rv = (E);                                          \
        goto bail;                                         \
    }                                                      \
    ip = is;                                               \
    ie = is + _size;                                       \
} while (0)

#define INFLATE_FLUSH(E) do {                 \
    const uint32_t _size = (op - os);         \
    if (write(os, _size, wctx) != _size) {    \
        rv = (E);                             \
        goto bail;                            \
    }                                         \
    flushed = true;                           \
    if (st == INFLATE_ZLIB)                   \
        chksum = adler_32(os, _size, chksum); \
    else if (st == INFLATE_GZIP)              \
        chksum = crc_32r(os, _size, chksum);  \
    op = os;                                  \
} while (0)

#define INFLATE_GET_BYTE(E,C) do { \
    if (ip == ie)                  \
        INFLATE_FILL(E);           \
    (C) = ip[0];                   \
    ++ip;                          \
} while (0)

#define INFLATE_PUT_BYTE(E,B) do { \
    if (op == oe)                  \
        INFLATE_FLUSH(E);          \
    op[0] = (B);                   \
    ++op;                          \
} while (0)

#define INFLATE_FILL_BITS(E,N) do { \
    while ((N) > nbits) {           \
        uint8_t _byte;              \
        INFLATE_GET_BYTE(E, _byte); \
        sreg |= (_byte << nbits);   \
        nbits += 8;                 \
    }                               \
} while (0)

#define INFLATE_CONSUME_BITS(N) do { \
    sreg >>= (N);                    \
    nbits -= (N);                    \
} while (0)

#define INFLATE_EXTRACT_BITS(N,B) do { \
    (B) = (sreg & ((1 << (N)) - 1));   \
    INFLATE_CONSUME_BITS(N);           \
} while (0)

#define INFLATE_CHECK_LENGTH(E,N) do { \
    if ((N) > (ie - ip)) {             \
        rv = (E);                      \
        goto bail;                     \
    }                                  \
} while (0)

#define INFLATE_REVERSE(C,B) ({                     \
    uint32_t _c = (C);                              \
    _c <<= (16 - (B));                              \
    ((revtab[_c >> 8]) | (revtab[_c & 0xff] << 8)); \
})

#define INFLATE_DECODE(E,H) ({                            \
    __label__ _found;                                     \
    uint32_t _c = (H)->flat[sreg & (H)->flat_mask];       \
    uint32_t _b = (_c & 0xff);                            \
    uint32_t _code;                                       \
    if (_b == 0xff) {                                     \
        for (_b = (_c >> 8); _b <= (H)->max_bits; _b++) { \
            _c = (revtab[sreg & 0xff] << 8);              \
            _c |= (revtab[(sreg >> 8) & 0xff]);           \
            _c >>= (16 - _b);                             \
            if (_c <= (H)->max_code[_b]) {                \
                _code = (H)->decode[(H)->last[_b] - _c];  \
                goto _found;                              \
            }                                             \
        }                                                 \
        rv = (E);                                         \
        goto bail;                                        \
    }                                                     \
    _code = (_c >> 8);                                    \
_found:                                                   \
    INFLATE_CONSUME_BITS(_b);                             \
    _code;                                                \
})

static int inflate_blocks(struct inflate* it, int st, inflate_reader read, void* rctx, inflate_writer write, void* wctx) {
    int rv = 0;
    uint8_t* is = it->in;
    uint8_t* ip;
    uint8_t* ie;
    uint8_t* os = it->out;
    uint8_t* op = os;
    uint8_t* oe = os + sizeof(it->out);
    bool flushed = false;
    uint32_t chksum;
    uint32_t nbits = 0;
    uint32_t sreg = 0;
    uint32_t i;
    uint32_t j;
    bool final;
    uint8_t type;

    INFLATE_FILL(-1);

    if (st == INFLATE_ZLIB) {
        uint16_t header;

        INFLATE_CHECK_LENGTH(-2, 2);

        header = (ip[0] << 8 | ip[1]);
        ip += 2;

        if ((header % 31) != 0) {
            rv = -3;
            goto bail;
        }

        if (((header & 0xf000) >> 12) > 7) {
            rv = -4;
            goto bail;
        }

        if (((header & 0x0f00) >> 8) != 8) {
            rv = -5;
            goto bail;
        }

        if (((header & 0x0020) >> 5) != 0) {
            rv = -6;
            goto bail;
        }

        chksum = 1;
    } else if (st == INFLATE_GZIP) {
        uint8_t flg;

        INFLATE_CHECK_LENGTH(-7, 10);

        if (ip[0] != 0x1f || ip[1] != 0x8b) {
            rv = -8;
            goto bail;
        }

        if (ip[2] != 8) {
            rv = -9;
            goto bail;
        }

        flg = ip[3];

        if ((flg & 0xe0) != 0) {
            rv = -10;
            goto bail;
        }

        ip += 10;
        chksum = 0xffffffff;

        if ((flg & 0x04) != 0) {
            uint16_t xlen;

            INFLATE_CHECK_LENGTH(-11, 2);

            xlen = (ip[0] | ip[1] << 8);
            ip += 2;

            while (xlen >= (ie - ip)) {
                xlen -= (ie - ip);

                if ((flg & 0x02) != 0)
                    chksum = crc_32r(is, ie - is, chksum);

                INFLATE_FILL(-12);
            }

            ip += xlen;
        }

        if ((flg & 0x08) != 0) {
            while (ip++[0] != '\0') {
                if (ip == ie) {
                    if ((flg & 0x02) != 0)
                        chksum = crc_32r(is, ie - is, chksum);

                    INFLATE_FILL(-13);
                }
            }
        }

        if ((flg & 0x10) != 0) {
            while (ip++[0] != '\0') {
                if (ip == ie) {
                    if ((flg & 0x02) != 0)
                        chksum = crc_32r(is, ie - is, chksum);

                    INFLATE_FILL(-14);
                }
            }
        }

        if ((flg & 0x02) != 0) {
            uint16_t crc16;

            INFLATE_CHECK_LENGTH(-15, 2);

            crc16 = (ip[0] | ip[1] << 8);
            chksum = crc_32r(is, ip - is, chksum);
            chksum &= 0xffff;
            chksum ^= 0xffff;

            if (crc16 != chksum) {
                rv = -16;
                goto bail;
            }

            ip += 2;
        }

        chksum = 0xffffffff;
    } else {
        chksum = 0;
    }

    do {
        INFLATE_FILL_BITS(-17, 3);
        final = (sreg & 0x01);
        type = ((sreg & 0x06) >> 1);
        INFLATE_CONSUME_BITS(3);

        if (type == 0) {
            uint8_t header[4];
            uint32_t len;
            uint32_t clen;

            INFLATE_CONSUME_BITS(nbits & 0x07);

            for (i = 0; i < 4; i++) {
                if (nbits != 0)
                    INFLATE_EXTRACT_BITS(8, header[i]);
                else
                    INFLATE_GET_BYTE(-18, header[i]);
            }

            len = (header[0] | (header[1] << 8));
            clen = (header[2] | (header[3] << 8)) ^ 0xffff;

            if (len != clen) {
                rv = -19;
                goto bail;
            }

            while (len != 0) {
                if (ip == ie)
                    INFLATE_FILL(-20);

                if (op == oe)
                    INFLATE_FLUSH(-21);

                j = MIN(len, MIN((uint32_t) (ie - ip), (uint32_t) (oe - op)));
                for (i = 0; i < j; i++)
                    op[i] = ip[i];

                len -= j;
                ip += j;
                op += j;
            }
        } else if (type == 3) {
            rv = -22;
            goto bail;
        } else {
            static const uint8_t revtab[256] =
            {
                0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
                0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
                0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
                0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
                0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
                0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
                0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
                0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
                0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
                0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
                0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
                0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
                0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
                0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
                0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
                0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
                0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
                0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
                0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
                0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
                0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
                0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
                0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
                0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
                0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
                0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
                0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
                0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
                0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
                0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
                0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
                0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
            };
            uint8_t* tab[3];
            uint32_t tab_lens[3];
            uint32_t tab_bits[3];
            struct inflate_huff* tab_huffs[3] = { &it->lentab, &it->offtab, &it->clentab, };
            uint32_t c;
            uint32_t k;

            if (type == 2) {
                static const uint32_t min_tab_sizes[3] = { 257, 1, 4, };
                static const uint32_t min_tab_bits[3] = { 5, 5, 4, };
                static const uint32_t clen_order[INFLATE_CODELEN_MAX] = {
                    16, 17, 18,  0,  8,
                     7,  9,  6, 10,  5,
                    11,  4, 12,  3, 13,
                     2, 14,  1, 15,
                };

                INFLATE_FILL_BITS(-23, 14);
                for (i = 0; i < 3; i++) {
                    INFLATE_EXTRACT_BITS(min_tab_bits[i], tab_lens[i]);
                    tab_lens[i] += min_tab_sizes[i];
                }

                tab[2] = it->clen;
                for (i = 0; i < INFLATE_CODELEN_MAX; i++)
                    tab[2][i] = 0;
                for (i = 0; i < tab_lens[2]; i++) {
                    INFLATE_FILL_BITS(-24, 3);
                    INFLATE_EXTRACT_BITS(3, tab[2][clen_order[i]]);
                }

                tab[0] = it->combo;
                tab_bits[0] = INFLATE_SYMBOL_BITS;

                tab[1] = tab[0] + tab_lens[0];
                tab_bits[1] = INFLATE_OFFSET_BITS;

                tab_lens[2] = INFLATE_CODELEN_MAX;
                tab_bits[2] = INFLATE_CODELEN_BITS;
            } else {
                tab[0] = it->len;
                for (i = 0; i < 144; i++)
                    tab[0][i] = 8;
                for (; i < 256; i++)
                    tab[0][i] = 9;
                for (; i < 280; i++)
                    tab[0][i] = 7;
                for (; i < INFLATE_SYMBOL_MAX; i++)
                    tab[0][i] = 8;
                tab_lens[0] = INFLATE_SYMBOL_MAX;
                tab_bits[0] = INFLATE_FLAT_BITS;

                tab[1] = it->off;
                for (i = 0; i < INFLATE_OFFSET_MAX; i++)
                    tab[1][i] = 5;
                tab_lens[1] = INFLATE_OFFSET_MAX;
                tab_bits[1] = INFLATE_FLAT_BITS;
            }

            for(; type != 0xff; --type) {
                const uint8_t* hb = tab[type];
                uint32_t hb_len = tab_lens[type];
                uint32_t flat_bits = tab_bits[type];
                struct inflate_huff* h = tab_huffs[type];
                uint32_t* bits = it->bits;
                uint32_t* codes = it->codes;
                uint32_t max_bits = 0;
                uint32_t min_bits = INFLATE_HUFF_BITS + 1;
                uint32_t min_code;
                uint32_t b;
                uint32_t code;
                uint32_t fc;
                uint32_t ec;

                for (i = 0; i < INFLATE_HUFF_BITS; i++)
                    bits[i] = 0;

                for (i = 0; i < hb_len; i++) {
                    b = hb[i];

                    if (b != 0) {
                        bits[b]++;

                        if (b < min_bits)
                            min_bits = b;

                        if (b > max_bits)
                            max_bits = b;
                    }
                }

                if (max_bits == 0) {
                    h->flat_mask = h->min_bits = h->max_bits = 0;
                    goto table_done;
                }

                h->max_bits = max_bits;

                for (b = c = code = 0; b <= max_bits; b++) {
                    h->last[b] = c;
                    c += bits[b];

                    min_code = (code << 1);
                    codes[b] = min_code;
                    code = (min_code + bits[b]);

                    if (code > (1U << b)) {
                        rv = -25;
                        goto bail;
                    }

                    h->max_code[b] = (code - 1);
                    h->last[b] += (code - 1);
                }

                if (flat_bits > max_bits)
                    flat_bits = max_bits;

                h->flat_mask = ((1 << flat_bits) - 1);

                if (min_bits > flat_bits)
                    min_bits = flat_bits;

                h->min_bits = min_bits;

                for (i = 0, b = (1 << flat_bits); i < b; i++)
                    h->flat[i] = 0xffffffff;

                for (b = max_bits; b > flat_bits; b--) {
                    code = h->max_code[b];

                    if (code == 0xffffffff)
                        break;

                    min_code = ((code + 1) - bits[b]);
                    min_code >>= (b - flat_bits);
                    code >>= (b - flat_bits);

                    for (; min_code <= code; min_code++)
                        h->flat[INFLATE_REVERSE(min_code, flat_bits)] = ((b << 8) | 0xff);
                }

                for (i = 0; i < hb_len; i++) {
                    b = hb[i];

                    if (b == 0)
                        continue;

                    c = codes[b]++;

                    if (b <= flat_bits) {
                        code = ((i << 8) | b);
                        ec = ((c + 1) << (flat_bits - b));

                        if (ec > (1U << flat_bits)) {
                            rv = -26;
                            goto bail;
                        }

                        for (fc = (c << (flat_bits - b)); fc < ec; fc++)
                            h->flat[INFLATE_REVERSE(fc, flat_bits)] = code;
                    }

                    if (b > min_bits) {
                        c = h->last[b] - c;

                        if (c >= hb_len) {
                            rv = -27;
                            goto bail;
                        }

                        h->decode[c] = i;
                    }
                }

                table_done:

                if (type == 2) {
                    static const uint32_t bits[3] = { 2, 3, 7, };
                    static const uint32_t bases[3] = { 3, 3, 11, };

                    for (i = 0, j = tab_lens[0] + tab_lens[1]; i < j; ) {
                        uint32_t len;
                        uint8_t byte;

                        INFLATE_FILL_BITS(-28, h->max_bits);

                        c = INFLATE_DECODE(-29, h);

                        if (c < 16) {
                            tab[0][i++] = c;
                            continue;
                        }

                        c -= 16;

                        if (c == 0 && i == 0) {
                            rv = -30;
                            goto bail;
                        }

                        INFLATE_FILL_BITS(-31, bits[c]);
                        INFLATE_EXTRACT_BITS(bits[c], len);
                        len += bases[c];

                        if ((i + len) > j) {
                            rv = -32;
                            goto bail;
                        }

                        byte = ((c == 0) ? tab[0][i - 1] : 0);
                        for (k = 0; k < len; k++)
                            tab[0][i + k] = byte;

                        i += len;
                    }
                }
            }

            while (1) {
                static const uint32_t lenbase[INFLATE_SYMBOL_MAX - 257] = {
                      3,   4,   5,   6,   7,  8,  9,  10,
                     11,  13,  15,  17,  19, 23, 27,  31,
                     35,  43,  51,  59,  67, 83, 99, 115,
                    131, 163, 195, 227, 258,  0,  0,
                };
                static const uint32_t lenextra[INFLATE_SYMBOL_MAX - 257] = {
                    0, 0, 0, 0, 0, 0, 0, 0,
                    1, 1, 1, 1, 2, 2, 2, 2,
                    3, 3, 3, 3, 4, 4, 4, 4,
                    5, 5, 5, 5, 0, 0, 0,
                };
                static const uint32_t offbase[INFLATE_OFFSET_MAX] = {
                       1,    2,    3,     4,     5,     7,    9,   13,
                      17,   25,   33,    49,    65,    97,  129,  193,
                     257,  385,  513,   769,  1025,  1537, 2049, 3073,
                    4097, 6145, 8193, 12289, 16385, 24577,    0,    0,
                };
                static const uint32_t offextra[INFLATE_OFFSET_MAX] = {
                     0,  0,  0,  0,  1,  1,  2,  2,
                     3,  3,  4,  4,  5,  5,  6,  6,
                     7,  7,  8,  8,  9,  9, 10, 10,
                    11, 11, 12, 12, 13, 13,  0,  0,
                };
                uint32_t len;
                uint32_t off;
                uint8_t* cp;

                INFLATE_FILL_BITS(-33, tab_huffs[0]->max_bits);
                c = INFLATE_DECODE(-34, tab_huffs[0]);

                if (c < 256) {
                    INFLATE_PUT_BYTE(-35, c);
                    continue;
                }

                if (c == 256)
                    break;

                if (c > 285) {
                    rv = -36;
                    goto bail;
                }

                c -= 257;
                INFLATE_FILL_BITS(-37, lenextra[c]);
                INFLATE_EXTRACT_BITS(lenextra[c], len);
                len += lenbase[c];

                INFLATE_FILL_BITS(-38, tab_huffs[1]->max_bits);
                c = INFLATE_DECODE(-39, tab_huffs[1]);

                if (c > 29) {
                    rv = -40;
                    goto bail;
                }

                INFLATE_FILL_BITS(-41, offextra[c]);
                INFLATE_EXTRACT_BITS(offextra[c], off);
                off += offbase[c];

                cp = op - off;
                if (cp < os) {
                    if (!flushed) {
                        rv = -42;
                        goto bail;
                    }

                    cp += sizeof(it->out);
                }

                while (len != 0) {
                    if (op == oe)
                        INFLATE_FLUSH(-43);

                    if (cp == oe)
                        cp = os;

                    j = MIN(len, MIN((uint32_t) (oe - op), (uint32_t) (oe - cp)));

                    for (i = 0; i < j; i++)
                        op[i] = cp[i];

                    op += j;
                    cp += j;
                    len -= j;
                }
            }
        }
    } while (!final);

    INFLATE_FLUSH(-44);

    if (st != INFLATE_RAW) {
        uint8_t header[4];
        uint32_t chksum2;

        INFLATE_CONSUME_BITS(nbits & 0x07);

        for (i = 0; i < 4; i++) {
            if (nbits != 0)
                INFLATE_EXTRACT_BITS(8, header[i]);
            else
                INFLATE_GET_BYTE(-45, header[i]);
        }

        if (st == INFLATE_ZLIB) {
            chksum2 = ((header[0] << 24) | (header[1] << 16) | (header[2] << 8) | (header[3] << 0));

            if (chksum != chksum2) {
                rv = -46;
                goto bail;
            }
        } else if (st == INFLATE_GZIP) {
            chksum2 = ((header[3] << 24) | (header[2] << 16) | (header[1] << 8) | (header[0] << 0));

            if (~chksum != chksum2) {
                rv = -47;
                goto bail;
            }
        }
    }

bail:
    return rv;
}

const uint32_t inflate_size = sizeof(struct inflate);
const uint32_t inflate_align = _Alignof(struct inflate);

int inflate(struct inflate* it, int st, inflate_reader read, void* rctx, inflate_writer write, void* wctx) {
    if (it == NULL || read == NULL || write == NULL || st < 0 || st > 2 || (((uintptr_t) it) & (_Alignof(struct inflate) - 1)) != 0)
        return -48;

    return inflate_blocks(it, st, read, rctx, write, wctx);
}

static uint32_t inflate_buffer_rw(struct inflate_bufferctx* c,
                                  void* dst, const void* src, uint32_t block_size)
{
    size_t size_left = c->end - c->buf;
    size_t copy_size = MIN((size_t)block_size, size_left);

    memcpy(dst, src, copy_size);
    c->buf += copy_size;

    return copy_size;
}

uint32_t inflate_buffer_reader(void* block, uint32_t block_size, void* ctx)
{
    struct inflate_bufferctx* c = ctx;
    return inflate_buffer_rw(c, block, c->buf, block_size);
}

uint32_t inflate_buffer_writer(const void* block, uint32_t block_size, void* ctx)
{
    struct inflate_bufferctx* c = ctx;
    return inflate_buffer_rw(c, c->buf, block, block_size);
}

uint32_t inflate_getsize_writer(const void* block, uint32_t block_size, void* ctx)
{
    (void)block;

    size_t* size = ctx;
    *size += block_size;
    return block_size;
}
