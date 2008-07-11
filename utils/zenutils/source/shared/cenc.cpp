/* zenutils - Utilities for working with creative firmwares.
 * Copyright 2007 (c) Rasmus Ry <rasmus.ry{at}gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "cenc.h"
#include <firmware.h>
#include <stdexcept>


namespace {
const byte CODE_MASK      = 0xC0;
const byte ARGS_MASK      = 0x3F;

const byte REPEAT_CODE    = 0x00;
const byte BLOCK_CODE     = 0x40;
const byte LONG_RUN_CODE  = 0x80;
const byte SHORT_RUN_CODE = 0xC0;

const byte BLOCK_ARGS     = 0x1F;
const byte BLOCK_MODE     = 0x20;


void decode_run(byte* dst, word len, byte val,
                int& dstidx)
{
    memset(dst + dstidx, val, len);
    dstidx += len;
}

void decode_pattern(byte* src, byte* dst,
                    word len, int& srcidx, int& dstidx,
                    bool bdecode, int npasses)
{
    for (int i = 0; i < npasses; i++)
    {
        if (bdecode)
        {
            for (int j = 0; j < len; j++)
            {
                word c, d;
                c = src[srcidx + j];
                d = (c >> 5) & 7;
                c = (c << 3) & 0xF8;
                src[srcidx + j] = static_cast<byte>(c | d);
            }
            bdecode = false;
        }
        memcpy(dst + dstidx, src + srcidx, len);
        dstidx += len;
    }
    srcidx += len;
}
}; //namespace

int zen::cenc_decode(byte* src, int srclen, byte* dst, int dstlen)
{
    if (!src || !srclen || !dst || !dstlen)
    {
        throw std::invalid_argument("Invalid argument(s).");
    }

    int i = 0, j = 0;
    do
    {
        word c, d, e;
        c = src[i++];
        switch (c & CODE_MASK)
        {
        case REPEAT_CODE: // 2 bytes
            d = src[i++];
            d = d + 2;

            e = (c & ARGS_MASK) + 2;

            decode_pattern(src, dst, e, i, j, false, d);
            break;

        case BLOCK_CODE: // 1/2/3 bytes
            d = c & BLOCK_ARGS;
            if (!(c & BLOCK_MODE))
            {
                e = src[i++];
                e = (d << 8) + (e + 0x21);

                d = static_cast<word>(i ^ j);
            }
            else
            {
                e = d + 1;

                d = static_cast<word>(i ^ j);
            }
            if (d & 1)
            {
                i++;
            }

            decode_pattern(src, dst, e, i, j, true, 1);
            break;

        case LONG_RUN_CODE: // 3 bytes
            d = src[i++];
            e = ((c & ARGS_MASK) << 8) + (d + 0x42);

            d = src[i++];
            d = ((d & 7) << 5) | ((d >> 3) & 0x1F);

            decode_run(dst, e, static_cast<byte>(d), j);
            break;

        case SHORT_RUN_CODE: // 2 bytes
            d = src[i++];
            d = ((d & 3) << 6) | ((d >> 2) & 0x3F);

            e = (c & ARGS_MASK) + 2;

            decode_run(dst, e, static_cast<byte>(d), j);
            break;
        };
    } while (i < srclen && j < dstlen);

    return j;
}

namespace {
int encode_run(byte* dst, int& dstidx, byte val, int len, int dstlen)
{
    if (len < 2)
        throw std::invalid_argument("Length is too small.");

    int ret = 0;
    if (len <= 0x41)
    {
        if ((dstidx + 2) > dstlen)
            throw std::runtime_error("Not enough space to store run.");

        dst[dstidx++] = SHORT_RUN_CODE | (((len - 2) & ARGS_MASK));
        dst[dstidx++] = ((val >> 6) & 3) | ((val & 0x3F) << 2);

        ret = 2;
    }
    else if (len <= 0x4041)
    {
        if ((dstidx + 3) > dstlen)
            throw std::runtime_error("Not enough space to store run.");

        byte b1 = (len - 0x42) >> 8;
        byte b2 = (len - 0x42) & 0xFF;

        dst[dstidx++] = LONG_RUN_CODE | ((b1 & ARGS_MASK));
        dst[dstidx++] = b2;
        dst[dstidx++] = ((val >> 5) & 7) | ((val & 0x1F) << 3);

        ret = 3;
    }
    else
    {
        int long_count  = len / 0x4041;
        int short_len   = len % 0x4041;
        bool toosmall   = short_len == 1;

        int run_len = 0x4041;
        for (int i = 0; i < long_count; i++)
        {
            if (toosmall && (i == (long_count-1)))
            {
                run_len--;
                toosmall = false;
            }
            int tmp = encode_run(dst, dstidx, val, run_len, dstlen);
            if (!tmp) return 0;
            ret += tmp;
            len -= run_len;
        }

        if (len)
        {
            int short_count = len / 0x41;
            int short_rest  = short_count ? (len % 0x41) : 0;
            toosmall = short_rest == 1;

            run_len = 0x41;
            for (int i = 0; i < short_count; i++)
            {
                if (toosmall && (i == (short_count-1)))
                {
                    run_len--;
                    toosmall = false;
                }
                int tmp = encode_run(dst, dstidx, val, run_len, dstlen);
                if (!tmp) return 0;
                ret += tmp;
                len -= run_len;
            }
            int tmp = encode_run(dst, dstidx, val, len, dstlen);
            if (!tmp) return 0;
            ret += tmp;
            len -= len;
        }
    }

    return ret;
}

int encode_block(byte* dst, int& dstidx, byte* src, int& srcidx, int len,
                 int dstlen)
{
    if (len < 1)
        throw std::invalid_argument("Length is too small.");

    int startidx = dstidx;
    if (len < 0x21)
    {
        if ((dstidx + 2 + len) > dstlen)
            throw std::runtime_error("Not enough space to store block.");

        dst[dstidx++] = BLOCK_CODE  | BLOCK_MODE | ((len - 1) & BLOCK_ARGS);
        if ((dstidx ^ srcidx) & 1)
            dst[dstidx++] = 0;

        for (int i = 0; i < len; i++)
        {
            byte c = src[srcidx++];
            byte d = (c & 7) << 5;
            c = (c & 0xF8) >> 3;
            dst[dstidx++] = c | d;
        }
    }
    else if (len < 0x2021)
    {
        if ((dstidx + 3 + len) > dstlen)
            throw std::runtime_error("Not enough space to store block.");

        dst[dstidx++] = BLOCK_CODE | (((len - 0x21) >> 8) & BLOCK_ARGS);
        dst[dstidx++] = (len - 0x21) & 0xFF;
        if ((dstidx ^ srcidx) & 1)
            dst[dstidx++] = 0;

        for (int i = 0; i < len; i++)
        {
            byte c = src[srcidx++];
            byte d = (c & 7) << 5;
            c = (c & 0xF8) >> 3;
            dst[dstidx++] = c | d;
        }
    }
    else
    {
        int longblocks = len / 0x2020;
        int rest = len % 0x2020;
        for (int i = 0; i < longblocks; i++)
        {
            int tmp = encode_block(dst, dstidx, src, srcidx, 0x2020, dstlen);
            if (!tmp) return 0;
        }
        if (rest)
        {
            int shortblocks = rest / 0x20;
            for (int i = 0; i < shortblocks; i++)
            {
                int tmp = encode_block(dst, dstidx, src, srcidx, 0x20, dstlen);
                if (!tmp) return 0;
            }
            rest = rest % 0x20;
            int tmp = encode_block(dst, dstidx, src, srcidx, rest, dstlen);
            if (!tmp) return 0;
        }
    }

    return (dstidx - startidx);
}
}; //namespace

int zen::cenc_encode(byte* src, int srclen, byte* dst, int dstlen)
{
    if (!src || !srclen || !dst || !dstlen)
    {
        throw std::invalid_argument("Invalid argument(s).");
    }

    int i = 0, j = 0, k = 0;
    word c, d, e;
    int runlen = 0;
    while (i < srclen && j < dstlen)
    {
        k = i;
        c = src[i++];
        runlen = 1;
        while (i < srclen && src[i] == c)
        {
            runlen++;
            i++;
        }
        if (runlen >= 2)
        {
            if (!encode_run(dst, j, c, runlen, dstlen))
                return 0;
        }
        else
        {
            runlen = 0;
            i = k;
            while (i < (srclen - 1) && (src[i] != src[i + 1]))
            {
                runlen++;
                i++;
            }
            if (i == (srclen - 1))
            {
                runlen++;
                i++;
            }
            if (!encode_block(dst, j, src, k, runlen, dstlen))
                return 0;
        }
    }

    return j;
}
