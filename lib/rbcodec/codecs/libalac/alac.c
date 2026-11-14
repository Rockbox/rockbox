/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * This is the actual decoder.
 *
 * http://crazney.net/programs/itunes/alac.html
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "codeclib.h"
#include "decomp.h"

#define SIGNEXTEND24(val) (((signed)val<<8)>>8)

static int16_t predictor_coef_table[32] IBSS_ATTR;
static int16_t predictor_coef_table_a[32] IBSS_ATTR;
static int16_t predictor_coef_table_b[32] IBSS_ATTR;

/* the uncompressed low bytes of a frame's samples, kept apart from the
 * output buffers which the entropy decoder writes */
static uint16_t shift_buffer[ALAC_MAX_CHANNELS][ALAC_BLOCKSIZE];


/* Endian/aligment safe functions - only used in alac_set_info() */
static uint32_t get_uint32be(unsigned char* p)
{
  return((p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3]);
}

static uint16_t get_uint16be(unsigned char* p)
{
  return((p[0]<<8) | p[1]);
}

void alac_set_info(alac_file *alac, char *inputbuffer)
{
  unsigned char* ptr = (unsigned char*)inputbuffer;
  ptr += 4; /* size */
  ptr += 4; /* frma */
  ptr += 4; /* alac */
  ptr += 4; /* size */
  ptr += 4; /* alac */

  ptr += 4; /* 0 ? */

  alac->setinfo_max_samples_per_frame = get_uint32be(ptr); /* buffer size / 2 ? */
  ptr += 4;
  alac->setinfo_7a = *ptr++;
  alac->setinfo_sample_size = *ptr++;
  alac->setinfo_rice_historymult = *ptr++;
  alac->setinfo_rice_initialhistory = *ptr++;
  alac->setinfo_rice_kmodifier = *ptr++;
  alac->setinfo_7f = *ptr++;

  alac->setinfo_80 = get_uint16be(ptr);
  ptr += 2;
  alac->setinfo_82 = get_uint32be(ptr);
  ptr += 4;
  alac->setinfo_86 = get_uint32be(ptr);
  ptr += 4;
  alac->setinfo_8a_rate = get_uint32be(ptr);
  ptr += 4;
}

/* stream reading */

/* supports reading 1 to 16 bits, in big endian format */
static inline uint32_t readbits_16(alac_file *alac, int bits)
{
    uint32_t result;
    int new_accumulator;

    result = (alac->input_buffer[0] << 16) |
             (alac->input_buffer[1] << 8) |
             (alac->input_buffer[2]);

    /* shift left by the number of bits we've already read,
     * so that the top 'n' bits of the 24 bits we read will
     * be the return bits */
    result = result << alac->input_buffer_bitaccumulator;

    result = result & 0x00ffffff;

    /* and then only want the top 'n' bits from that, where
     * n is 'bits' */
    result = result >> (24 - bits);

    new_accumulator = (alac->input_buffer_bitaccumulator + bits);

    /* increase the buffer pointer if we've read over n bytes. */
    alac->input_buffer += (new_accumulator >> 3);

    /* and the remainder goes back into the bit accumulator */
    alac->input_buffer_bitaccumulator = (new_accumulator & 7);

    return result;
}

/* supports reading 1 to 32 bits, in big endian format */
static inline uint32_t readbits(alac_file *alac, int bits)
{
    int32_t result = 0;

    if (bits > 16)
    {
        bits -= 16;
        result = readbits_16(alac, 16) << bits;
    }

    result |= readbits_16(alac, bits);

    return result;
}

/* reads a single bit */
static inline int readbit(alac_file *alac)
{
    int result;
    int new_accumulator;

    result = alac->input_buffer[0];

    result = result << alac->input_buffer_bitaccumulator;

    result = result >> 7 & 1;

    new_accumulator = (alac->input_buffer_bitaccumulator + 1);

    alac->input_buffer += (new_accumulator / 8);

    alac->input_buffer_bitaccumulator = (new_accumulator % 8);

    return result;
}

static inline void unreadbits(alac_file *alac, int bits)
{
    int new_accumulator = (alac->input_buffer_bitaccumulator - bits);

    alac->input_buffer += (new_accumulator >> 3);

    alac->input_buffer_bitaccumulator = (new_accumulator & 7);
    if (alac->input_buffer_bitaccumulator < 0)
        alac->input_buffer_bitaccumulator *= -1;
}

#define count_leading_zeros(x) bs_generic(x, BS_CLZ)

/* Without a CLZ instruction, bs_generic() is a table lookup and its
   16-bit form (BS_SHORT) is cheaper; use it when the argument fits. */
#if !defined(CPU_ARM) || ARM_ARCH < 5
#define CLZ_BY_TABLE
#endif

#define RICE_THRESHOLD 8 // maximum number of bits for a rice prefix.

static inline int32_t entropy_decode_value(alac_file* alac,
                             int readsamplesize,
                             int k)
{
    int32_t x = 0; // decoded value
    
    // read x, number of 1s before 0 represent the rice value.
    while (x <= RICE_THRESHOLD && readbit(alac))
    {
        x++;
    }
    
    if (x > RICE_THRESHOLD)
    {
        // read the number from the bit stream (raw value)
        int32_t value;
        
        value = readbits(alac, readsamplesize);
        
        /* mask value to readsamplesize size */
        if (readsamplesize != 32)
            value &= (((uint32_t)0xffffffff) >> (32 - readsamplesize));
        
        x = value;
    }
    else
    {
        if (k != 1)
        {
            int extrabits = readbits(alac, k);
            
            // x = x * (2^k - 1)
            x = (x << k) - x;
            
            if (extrabits > 1)
                x += extrabits - 1;
            else
                unreadbits(alac, 1);
        }
    }
    
    return x;
}

/* The original decoder, kept for streams outside what the fast one
 * handles (see entropy_rice_decode). */
static void entropy_rice_decode_generic(alac_file* alac,
                         int32_t* output_buffer,
                         int output_size,
                         int readsamplesize,
                         int rice_initialhistory,
                         int rice_kmodifier,
                         int rice_historymult,
                         int rice_kmodifier_mask)
{
    int                output_count;
    int                history = rice_initialhistory;
    int                sign_modifier = 0;
    
    for (output_count = 0; output_count < output_size; output_count++)
    {
        int32_t        decoded_value;
        int32_t        final_value;
        int32_t        k;
        uint32_t       x = (history >> 9) + 3;
        
#ifdef CLZ_BY_TABLE
        if (x < 0x10000)
        {
            /* the same k as below, for 16-bit x */
            k = 31 - bs_generic(x, BS_CLZ|BS_SHORT);
            if (k > rice_kmodifier)
                k = rice_kmodifier;
        }
        else
#endif
        {
            k = 31 - rice_kmodifier - count_leading_zeros(x);
            if (k < 0) k += rice_kmodifier;
            else k = rice_kmodifier;
        }
        
        decoded_value = entropy_decode_value(alac, readsamplesize, k);
        
        decoded_value += sign_modifier;
        final_value = (decoded_value + 1) / 2; // inc by 1 and shift out sign bit
        if (decoded_value & 1) // the sign is stored in the low bit
            final_value *= -1;
        
        output_buffer[output_count] = final_value;
        
        sign_modifier = 0;
        
        // update history
        history += (decoded_value * rice_historymult)
                - ((history * rice_historymult) >> 9);
        
        if (decoded_value > 0xFFFF)
            history = 0xFFFF;
        
        // special case, for compressed blocks of 0
        if ((history < 128) && (output_count + 1 < output_size))
        {
            int32_t        block_size;
            
            sign_modifier = 1;
            
            k = count_leading_zeros(history) + ((history + 16) / 64) - 24;
            
            // note: block_size is always 16bit
            block_size = entropy_decode_value(alac, 16, k) & rice_kmodifier_mask;
            
            if (block_size > 0xFFFF)
                sign_modifier = 0;

            // got block_size 0s, no more than the frame has left
            if (block_size > output_size - output_count - 1)
                block_size = output_size - output_count - 1;
            if (block_size > 0)
            {
                memset(&output_buffer[output_count + 1], 0, 
                       block_size * sizeof(*output_buffer));
                output_count += block_size;
            }
            
            history = 0;
        }
    }
}

/* Fast entropy decoder.  The next bits of the stream are kept in a word,
 * left-aligned, with zeros below them, so that a whole Rice code is read
 * with a count of leading ones and one shift instead of a bit at a time.
 * It holds at most 32 bits, so it reads at most four bytes past the
 * current position, against three for readbits(). */

#define CACHE_REFILL()                                      \
    while (bits <= 24) {                                    \
        cache |= (uint32_t)*p++ << (24 - bits);             \
        bits += 8;                                          \
    }

/* Two bytes when 16 bits or fewer are left, which leaves at least 17.
 * The empty asm stops gcc turning the two byte loads into a halfword
 * load and a byte swap, which costs three more instructions on ARM. */
#define CACHE_REFILL16()                                    \
    if (bits <= 16) {                                       \
        uint32_t b0_ = p[0];                                \
        asm ("" : "+r" (b0_));                              \
        cache |= ((b0_ << 8) | p[1]) << (16 - bits);        \
        p += 2;                                             \
        bits += 16;                                         \
    }

/* Number of leading ones in the cache, at most RICE_THRESHOLD + 1. */
static inline unsigned int cache_leading_ones(uint32_t cache)
{
#if defined(CPU_ARM) && ARM_ARCH >= 5
    return __builtin_clz(~cache | (1u << (31 - (RICE_THRESHOLD + 1))));
#else
    /* bs_log2_tab, as rice_k() uses, so the loop needs one table address */
    uint32_t top = (~cache) >> 24;
    if (LIKELY(top))
        return 7 - bs_log2_tab[top];
    return 8 + ((cache >> 23) & 1);
#endif
}

/* k for the next value: floor(log2((history >> 9) + 3)), at most kmodifier.
 * lim is 1 << kmodifier. */
static inline int rice_k(int history, int kmodifier, uint32_t lim)
{
    uint32_t v = (history >> 9) + 3;
#if defined(CPU_ARM) && ARM_ARCH >= 5
    int k = 31 - __builtin_clz(v);
    (void)lim;
    return k < kmodifier ? k : kmodifier;
#else
    (void)kmodifier;
    if (v > lim)
        v = lim;
    return bs_generic(v, BS_LOG2 | BS_SHORT);
#endif
}

/* One value, as entropy_decode_value().  Needs 1 <= k <= 16 and
 * 1 <= readsamplesize <= 32, and enough bits: ones + 1 + k for the code,
 * which 25 always is; the escape refills for itself after its prefix. */
#define CACHE_DECODE_VALUE(x, k, readsamplesize)                        \
do {                                                                    \
    unsigned int ones_ = cache_leading_ones(cache);                     \
    if (LIKELY(ones_ <= RICE_THRESHOLD)) {                              \
        uint32_t v_;                                                    \
        int used_ = (k);                                                \
        cache <<= ones_ + 1;                                            \
        v_ = cache >> (32 - (k));                                       \
        (x) = (ones_ << (k)) - ones_;                                   \
        if (v_ > 1)                                                     \
            (x) += v_ - 1;                                              \
        else                                                            \
            used_ -= 1;                                                 \
        cache <<= used_;                                                \
        bits -= ones_ + 1 + used_;                                      \
    } else {                                                            \
        int n_ = (readsamplesize);                                      \
        cache <<= RICE_THRESHOLD + 1;                                   \
        bits -= RICE_THRESHOLD + 1;                                     \
        CACHE_REFILL();                                                 \
        (x) = 0;                                                        \
        if (n_ > 16) {                                                  \
            n_ -= 16;                                                   \
            (x) = (cache >> (32 - n_)) << 16;                           \
            cache <<= n_;                                               \
            bits -= n_;                                                 \
            CACHE_REFILL();                                             \
            n_ = 16;                                                    \
        }                                                               \
        (x) |= cache >> (32 - n_);                                      \
        cache <<= n_;                                                   \
        bits -= n_;                                                     \
    }                                                                   \
} while (0)

static void entropy_rice_decode(alac_file* alac,
                         int32_t* output_buffer,
                         int output_size,
                         int readsamplesize,
                         int rice_initialhistory,
                         int rice_kmodifier,
                         int rice_historymult,
                         int rice_kmodifier_mask) ICODE_ATTR_ALAC;
static void entropy_rice_decode(alac_file* alac,
                         int32_t* output_buffer,
                         int output_size,
                         int readsamplesize,
                         int rice_initialhistory,
                         int rice_kmodifier,
                         int rice_historymult,
                         int rice_kmodifier_mask)
{
    const unsigned char *p;
    uint32_t cache;
    int bits;
    int history = rice_initialhistory;
    int sign_modifier = 0;
    uint32_t lim = (uint32_t)rice_kmodifier_mask + 1;
    int32_t *out = output_buffer;
    int32_t *end = output_buffer + output_size;

    /* The fast path needs k <= 15 (for BS_SHORT on ARMv4 and the 16-bit
     * peek) and values below 2^31 (for the sign arithmetic). */
    if (UNLIKELY(rice_kmodifier < 1 || rice_kmodifier > 15 ||
                 readsamplesize < 1 || readsamplesize > 31))
    {
        entropy_rice_decode_generic(alac, output_buffer, output_size,
                                    readsamplesize, rice_initialhistory,
                                    rice_kmodifier, rice_historymult,
                                    rice_kmodifier_mask);
        return;
    }

    p = alac->input_buffer;
    cache = 0;
    bits = 0;
    CACHE_REFILL();
    cache <<= alac->input_buffer_bitaccumulator;
    bits -= alac->input_buffer_bitaccumulator;

    while (out < end)
    {
        uint32_t x;
        int32_t decoded_value;
        int k;

        CACHE_REFILL16();
        k = rice_k(history, rice_kmodifier, lim);
        /* 17 bits cover the code unless the prefix or k is long */
        if (UNLIKELY(cache_leading_ones(cache) + 1 + k > (unsigned)bits))
            CACHE_REFILL();
        CACHE_DECODE_VALUE(x, k, readsamplesize);

        decoded_value = x + sign_modifier;
        /* (decoded_value + 1) / 2, negated if decoded_value is odd */
        *out++ = (decoded_value >> 1) ^ -(decoded_value & 1);

        sign_modifier = 0;

        history += (decoded_value * rice_historymult)
                - ((history * rice_historymult) >> 9);

        if (decoded_value > 0xFFFF)
            history = 0xFFFF;

        /* special case, for compressed blocks of 0 */
        if (UNLIKELY(history < 128))
        {
            if (out >= end)
                break;

            int32_t block_size;

            sign_modifier = 1;

            k = count_leading_zeros(history) + ((history + 16) / 64) - 24;

            CACHE_REFILL();
            CACHE_DECODE_VALUE(x, k, 16);
            block_size = x & rice_kmodifier_mask;

            if (block_size > 0xFFFF)
                sign_modifier = 0;

            /* no more than the frame has left */
            if (block_size > end - out)
                block_size = end - out;
            if (block_size > 0)
            {
                memset(out, 0, block_size * sizeof(*out));
                out += block_size;
            }

            history = 0;
        }
    }

    /* hand the position back, rounded to the bit */
    alac->input_buffer = (unsigned char *)p - ((bits + 7) >> 3);
    alac->input_buffer_bitaccumulator = (8 - (bits & 7)) & 7;
}


#define SIGN_EXTENDED32(val, bits) ((val << (32 - bits)) >> (32 - bits))

#define SIGN_ONLY(v) \
                     ((v < 0) ? (-1) : \
                                ((v > 0) ? (1) : \
                                           (0)))

/* The unrolled predictors below hold the coefficients as 32-bit values
 * for a whole frame, where the table and the generic loop keep them as
 * int16_t and wrap.  Each sample moves a coefficient by at most one, so the
 * two agree unless one could pass the int16 range within the frame. */
static inline int coefs_fit(const int16_t *coefs, int n, int output_size)
{
    int j;

    for (j = 0; j < n; j++)
    {
        int c = coefs[j];
        if (c < 0)
            c = -c;
        if (c > 32767 - output_size)
            return 0;
    }
    return 1;
}

#ifdef CPU_ARM_CLASSIC
/* arm.S */
void alac_fir_adapt_4_arm(int32_t *x, int32_t *end, int16_t *coefs,
                          uint32_t packed);
void alac_fir_adapt_5_arm(int32_t *x, int32_t *end, int16_t *coefs,
                          uint32_t packed);
void alac_fir_adapt_6_arm(int32_t *x, int32_t *end, int16_t *coefs,
                          uint32_t packed);
void alac_fir_adapt_7_arm(int32_t *x, int32_t *end, int16_t *coefs,
                          uint32_t packed);
void alac_fir_adapt_8_arm(int32_t *x, int32_t *end, int16_t *coefs,
                          uint32_t packed);
#endif

#ifndef CPU_ARM_CLASSIC
/* The adaptive FIR for a fixed order N, which the compiler unrolls.  The
 * coefficients are held in locals for the whole call, and the differences
 * from the oldest sample, x[N - j] - x[0], are computed once per sample and
 * used both for the prediction and for adapting the coefficients.
 *
 * The adaptation is sign-sign: coefficient j moves by the sign of its
 * difference, against the sign of the error, starting from the newest
 * sample, until the error is used up.  Each step uses up
 * (|d| >> quant) * (N - j) of it; for negative errors the shift is of
 * -|d|, which rounds the other way. */
static inline __attribute__((always_inline))
void fir_adapt_order(const int32_t *error_buffer, int32_t *x, int output_size,
                     int readsamplesize, int16_t *coef_table, const int N,
                     int quant)
{
    int32_t c[8];
    const int round = 1 << (quant - 1);
    const int sh = 32 - readsamplesize;
    int i, j;

    for (j = 0; j < N; j++)
        c[j] = coef_table[j];

    for (i = N + 1; i < output_size; i++)
    {
        int32_t d[8];
        int32_t top = x[0];
        int32_t sum = 0;
        int32_t err = error_buffer[i];
        int32_t out;

        for (j = 0; j < N; j++)
        {
            d[j] = x[N - j] - top;
            sum += d[j] * c[j];
        }

        out = ((sum + round) >> quant) + top + err;
        x[N + 1] = (out << sh) >> sh;

        if (err > 0)
        {
            for (j = N - 1; j >= 0; j--)
            {
                int32_t dj = d[j];
                if (dj > 0)
                {
                    c[j]++;
                    err -= (dj >> quant) * (N - j);
                }
                else if (dj < 0)
                {
                    c[j]--;
                    err -= ((-dj) >> quant) * (N - j);
                }
                if (err <= 0)
                    break;
            }
        }
        else if (err < 0)
        {
            for (j = N - 1; j >= 0; j--)
            {
                int32_t dj = d[j];
                if (dj > 0)
                {
                    c[j]--;
                    err -= ((-dj) >> quant) * (N - j);
                }
                else if (dj < 0)
                {
                    c[j]++;
                    err -= (dj >> quant) * (N - j);
                }
                if (err >= 0)
                    break;
            }
        }

        x++;
    }

    for (j = 0; j < N; j++)
        coef_table[j] = c[j];
}

#define FIR_ADAPT(n)                                                        \
static void fir_adapt_##n(const int32_t *error_buffer, int32_t *x,         \
                          int output_size, int readsamplesize,             \
                          int16_t *coef_table, int quant)                  \
                          __attribute__((noinline)) ICODE_ATTR_ALAC;        \
static void fir_adapt_##n(const int32_t *error_buffer, int32_t *x,         \
                          int output_size, int readsamplesize,             \
                          int16_t *coef_table, int quant)                  \
{                                                                           \
    fir_adapt_order(error_buffer, x, output_size, readsamplesize,          \
                    coef_table, n, quant);                                  \
}
FIR_ADAPT(4)
FIR_ADAPT(5)
FIR_ADAPT(6)
FIR_ADAPT(7)
FIR_ADAPT(8)
#endif /* !CPU_ARM_CLASSIC */

static void predictor_decompress_fir_adapt(int32_t *error_buffer,
                                           int32_t *buffer_out,
                                           int output_size,
                                           int readsamplesize,
                                           int16_t *predictor_coef_table,
                                           int predictor_coef_num,
                                           int predictor_quantitization) ICODE_ATTR_ALAC;
static void predictor_decompress_fir_adapt(int32_t *error_buffer,
                                           int32_t *buffer_out,
                                           int output_size,
                                           int readsamplesize,
                                           int16_t *predictor_coef_table,
                                           int predictor_coef_num,
                                           int predictor_quantitization)
{
    int i;

    /* first sample always copies */
    *buffer_out = *error_buffer;

    if (!predictor_coef_num)
    {
        if (output_size <= 1) return;
        memcpy(buffer_out+1, error_buffer+1, (output_size-1) * 4);
        return;
    }

    if (predictor_coef_num == 0x1f) /* 11111 - max value of predictor_coef_num */
    { /* second-best case scenario for fir decompression,
       * error describes a small difference from the previous sample only
       */
        if (output_size <= 1) return;
        for (i = 0; i < output_size - 1; i++)
        {
            int32_t prev_value;
            int32_t error_value;

            prev_value = buffer_out[i];
            error_value = error_buffer[i+1];
            buffer_out[i+1] = SIGN_EXTENDED32((prev_value + error_value), readsamplesize);
        }
        return;
    }

    /* read warm-up samples */
    if (predictor_coef_num > 0)
    {
        int i;
        for (i = 0; i < predictor_coef_num; i++)
        {
            int32_t val;

            val = buffer_out[i] + error_buffer[i+1];

            val = SIGN_EXTENDED32(val, readsamplesize);

            buffer_out[i+1] = val;
        }
    }

    /* Apple's encoder uses orders 4 and 8, FFmpeg's 4 to 6 by default */
    if (predictor_coef_num >= 4 && predictor_coef_num <= 8 &&
        output_size > predictor_coef_num + 1 &&
        predictor_quantitization >= 1 && predictor_quantitization <= 15 &&
        coefs_fit(predictor_coef_table, predictor_coef_num, output_size))
    {
#ifdef CPU_ARM_CLASSIC
        if (error_buffer == buffer_out &&
            readsamplesize >= 1 && readsamplesize <= 32)
        {
            int q = predictor_quantitization;
            uint32_t packed = q | ((32 - readsamplesize) << 8) |
                              ((1u << (q - 1)) << 16);
            int32_t *end = buffer_out + output_size - predictor_coef_num - 1;

            switch (predictor_coef_num)
            {
            case 4:
                alac_fir_adapt_4_arm(buffer_out, end, predictor_coef_table,
                                     packed);
                return;
            case 5:
                alac_fir_adapt_5_arm(buffer_out, end, predictor_coef_table,
                                     packed);
                return;
            case 6:
                alac_fir_adapt_6_arm(buffer_out, end, predictor_coef_table,
                                     packed);
                return;
            case 7:
                alac_fir_adapt_7_arm(buffer_out, end, predictor_coef_table,
                                     packed);
                return;
            case 8:
                alac_fir_adapt_8_arm(buffer_out, end, predictor_coef_table,
                                     packed);
                return;
            }
        }
#else
        switch (predictor_coef_num)
        {
        case 4:
            fir_adapt_4(error_buffer, buffer_out, output_size,
                        readsamplesize, predictor_coef_table,
                        predictor_quantitization);
            return;
        case 5:
            fir_adapt_5(error_buffer, buffer_out, output_size,
                        readsamplesize, predictor_coef_table,
                        predictor_quantitization);
            return;
        case 6:
            fir_adapt_6(error_buffer, buffer_out, output_size,
                        readsamplesize, predictor_coef_table,
                        predictor_quantitization);
            return;
        case 7:
            fir_adapt_7(error_buffer, buffer_out, output_size,
                        readsamplesize, predictor_coef_table,
                        predictor_quantitization);
            return;
        case 8:
            fir_adapt_8(error_buffer, buffer_out, output_size,
                        readsamplesize, predictor_coef_table,
                        predictor_quantitization);
            return;
        }
#endif
    }

    /* general case */
    if (predictor_coef_num > 0)
    {
        for (i = predictor_coef_num + 1;
             i < output_size;
             i++)
        {
            int j;
            int sum = 0;
            int outval;
            int error_val = error_buffer[i];

            for (j = 0; j < predictor_coef_num; j++)
            {
                sum += (buffer_out[predictor_coef_num-j] - buffer_out[0]) *
                       predictor_coef_table[j];
            }

            outval = (1 << (predictor_quantitization-1)) + sum;
            outval = outval >> predictor_quantitization;
            outval = outval + buffer_out[0] + error_val;
            outval = SIGN_EXTENDED32(outval, readsamplesize);

            buffer_out[predictor_coef_num+1] = outval;

            if (error_val > 0)
            {
                int predictor_num = predictor_coef_num - 1;

                while (predictor_num >= 0 && error_val > 0)
                {
                    int val = buffer_out[0] - buffer_out[predictor_coef_num - predictor_num];
                    int sign = SIGN_ONLY(val);

                    predictor_coef_table[predictor_num] -= sign;

                    val *= sign; /* absolute value */

                    error_val -= ((val >> predictor_quantitization) *
                                  (predictor_coef_num - predictor_num));

                    predictor_num--;
                }
            }
            else if (error_val < 0)
            {
                int predictor_num = predictor_coef_num - 1;

                while (predictor_num >= 0 && error_val < 0)
                {
                    int val = buffer_out[0] - buffer_out[predictor_coef_num - predictor_num];
                    int sign = - SIGN_ONLY(val);

                    predictor_coef_table[predictor_num] -= sign;

                    val *= sign; /* neg value */

                    error_val -= ((val >> predictor_quantitization) *
                                  (predictor_coef_num - predictor_num));

                    predictor_num--;
                }
            }

            buffer_out++;
        }
    }
}

static void deinterlace_16(int32_t* buffer0,
                           int32_t* buffer1,
                           int numsamples,
                           uint8_t interlacing_shift,
                           uint8_t interlacing_leftweight) ICODE_ATTR_ALAC;
static void deinterlace_16(int32_t* buffer0,
                           int32_t* buffer1,
                           int numsamples,
                           uint8_t interlacing_shift,
                           uint8_t interlacing_leftweight)
{
    int i;
    if (numsamples <= 0) return;

    /* weighted interlacing */
    if (interlacing_leftweight)
    {
        for (i = 0; i < numsamples; i++)
        {
            int32_t difference, midright;

            midright = buffer0[i];
            difference = buffer1[i];

            buffer0[i] = ((midright - ((difference * interlacing_leftweight)
                            >> interlacing_shift)) + difference) << SCALE16;
            buffer1[i] = (midright - ((difference * interlacing_leftweight) 
                            >> interlacing_shift)) << SCALE16;
        }

        return;
    }

    /* otherwise basic interlacing took place */
    for (i = 0; i < numsamples; i++)
    {
        buffer0[i] = buffer0[i] << SCALE16;
        buffer1[i] = buffer1[i] << SCALE16;
    }
}

static void deinterlace_24(int32_t *buffer0, int32_t *buffer1,
                           int uncompressed_bytes,
                           uint16_t *uncompressed_bytes_buffer0,
                           uint16_t *uncompressed_bytes_buffer1,
                           int numsamples,
                           uint8_t interlacing_shift,
                           uint8_t interlacing_leftweight) ICODE_ATTR_ALAC;
static void deinterlace_24(int32_t *buffer0, int32_t *buffer1,
                           int uncompressed_bytes,
                           uint16_t *uncompressed_bytes_buffer0,
                           uint16_t *uncompressed_bytes_buffer1,
                           int numsamples,
                           uint8_t interlacing_shift,
                           uint8_t interlacing_leftweight)
{
    int i;
    if (numsamples <= 0) return;
    
    /* weighted interlacing */
    if (interlacing_leftweight)
    {
        for (i = 0; i < numsamples; i++)
        {
            int32_t difference, midright;
            
            midright = buffer0[i];
            difference = buffer1[i];
            
            buffer0[i] = (midright - ((difference * interlacing_leftweight)
                            >> interlacing_shift)) + difference;
            buffer1[i] = midright - ((difference * interlacing_leftweight)
                            >> interlacing_shift);
            
            if (uncompressed_bytes)
            {
                uint32_t mask = ~(0xFFFFFFFF << (uncompressed_bytes * 8));
                buffer0[i] <<= (uncompressed_bytes * 8);
                buffer1[i] <<= (uncompressed_bytes * 8);
                
                buffer0[i] |= uncompressed_bytes_buffer0[i] & mask;
                buffer1[i] |= uncompressed_bytes_buffer1[i] & mask;
            }

            buffer0[i] = buffer0[i] << SCALE24;
            buffer1[i] = buffer1[i] << SCALE24;
        }
        
        return;
    }
    
    /* otherwise basic interlacing took place */
    for (i = 0; i < numsamples; i++)
    {
        if (uncompressed_bytes)
        {
            uint32_t mask = ~(0xFFFFFFFF << (uncompressed_bytes * 8));
            buffer0[i] <<= (uncompressed_bytes * 8);
            buffer1[i] <<= (uncompressed_bytes * 8);
            
            buffer0[i] |= uncompressed_bytes_buffer0[i] & mask;
            buffer1[i] |= uncompressed_bytes_buffer1[i] & mask;
        }
        
        buffer0[i] = buffer0[i] << SCALE24;
        buffer1[i] = buffer1[i] << SCALE24;
    }
    
}

static inline int decode_frame_mono(
                    alac_file *alac,
                    int32_t outputbuffer[ALAC_MAX_CHANNELS][ALAC_BLOCKSIZE],
                    void (*yield)(void))
{
    int hassize;
    int isnotcompressed;
    int readsamplesize;
    int infosamplesize = alac->setinfo_sample_size;
    int outputsamples = alac->setinfo_max_samples_per_frame;

    int uncompressed_bytes;
    int ricemodifier;


    /* 2^result = something to do with output waiting.
     * perhaps matters if we read > 1 frame in a pass?
     */
    readbits(alac, 4);

    readbits(alac, 12); /* unknown, skip 12 bits */

    hassize = readbits(alac, 1); /* the output sample size is stored soon */

    /* number of bytes in the (compressed) stream that are not compressed */
    uncompressed_bytes = readbits(alac, 2);

    isnotcompressed = readbits(alac, 1); /* whether the frame is compressed */

    if (hassize)
    {
        /* now read the number of samples,
         * as a 32bit integer */
        outputsamples = readbits(alac, 32);
    }
    /* the output buffers hold ALAC_BLOCKSIZE */
    if (outputsamples < 0 || outputsamples > ALAC_BLOCKSIZE)
        return -1;

    readsamplesize = infosamplesize - (uncompressed_bytes * 8);

    if (!isnotcompressed)
    { /* so it is compressed */
        int predictor_coef_num;
        int prediction_type;
        int prediction_quantitization;
        int i;

        /* skip 16 bits, not sure what they are. seem to be used in
         * two channel case */
        readbits(alac, 8);
        readbits(alac, 8);

        prediction_type = readbits(alac, 4);
        prediction_quantitization = readbits(alac, 4);

        ricemodifier = readbits(alac, 3);
        predictor_coef_num = readbits(alac, 5);

        /* read the predictor table */
        for (i = 0; i < predictor_coef_num; i++)
        {
            predictor_coef_table[i] = (int16_t)readbits(alac, 16);
        }

        if (uncompressed_bytes)
        {
            int i;
            for (i = 0; i < outputsamples; i++)
            {
                shift_buffer[0][i] = readbits(alac, uncompressed_bytes * 8);
            }
        }

        yield();

        entropy_rice_decode(alac,
                            outputbuffer[0],
                            outputsamples,
                            readsamplesize,
                            alac->setinfo_rice_initialhistory,
                            alac->setinfo_rice_kmodifier,
                            ricemodifier * alac->setinfo_rice_historymult / 4,
                            (1 << alac->setinfo_rice_kmodifier) - 1);

        yield();

        if (prediction_type == 0)
        { /* adaptive fir */
            predictor_decompress_fir_adapt(outputbuffer[0],
                                           outputbuffer[0],
                                           outputsamples,
                                           readsamplesize,
                                           predictor_coef_table,
                                           predictor_coef_num,
                                           prediction_quantitization);
        }
        else
        {
            //fprintf(stderr, "FIXME: unhandled predicition type: %i\n", prediction_type);
            /* i think the only other prediction type (or perhaps this is just a
             * boolean?) runs adaptive fir twice.. like:
             * predictor_decompress_fir_adapt(predictor_error, tempout, ...)
             * predictor_decompress_fir_adapt(predictor_error, outputsamples ...)
             * little strange..
             */
        }

    }
    else
    { /* not compressed, easy case */
        if (infosamplesize <= 16)
        {
            int i;
            for (i = 0; i < outputsamples; i++)
            {
                int32_t audiobits = readbits(alac, infosamplesize);

                audiobits = SIGN_EXTENDED32(audiobits, infosamplesize);

                outputbuffer[0][i] = audiobits;
            }
        }
        else
        {
            int i;
            for (i = 0; i < outputsamples; i++)
            {
                int32_t audiobits;

                audiobits = readbits(alac, 16);
                /* special case of sign extension..
                 * as we'll be ORing the low 16bits into this */
                audiobits = audiobits << (infosamplesize - 16);
                audiobits |= readbits(alac, infosamplesize - 16);
                audiobits = SIGNEXTEND24(audiobits);

                outputbuffer[0][i] = audiobits;
            }
        }
        uncompressed_bytes = 0; // always 0 for uncompressed
    }

    yield();

    switch(infosamplesize)
    {
    case 16:
    {
        int i;
        for (i = 0; i < outputsamples; i++)
        {
            /* Output mono data as stereo */
            outputbuffer[0][i] = outputbuffer[0][i] << SCALE16;
            outputbuffer[1][i] = outputbuffer[0][i];
        }
        break;
    }
    case 24:
    {
        int i;
        for (i = 0; i < outputsamples; i++)
        {
            int32_t sample = outputbuffer[0][i];
            
            if (uncompressed_bytes)
            {
                uint32_t mask;
                sample = sample << (uncompressed_bytes * 8);
                mask = ~(0xFFFFFFFF << (uncompressed_bytes * 8));
                sample |= shift_buffer[0][i] & mask;
            }
            
            outputbuffer[0][i] = sample << SCALE24;
            outputbuffer[1][i] = outputbuffer[0][i];
        }
        break;
    }
    case 20:
    case 32:
        //fprintf(stderr, "FIXME: unimplemented sample size %i\n", infosamplesize);
        break;
    default:
        break;
    }

    return outputsamples;
}

static inline int decode_frame_stereo(
                    alac_file *alac,
                    int32_t outputbuffer[ALAC_MAX_CHANNELS][ALAC_BLOCKSIZE],
                    void (*yield)(void))
{
    int hassize;
    int isnotcompressed;
    int readsamplesize;
    int infosamplesize = alac->setinfo_sample_size;
    int outputsamples = alac->setinfo_max_samples_per_frame;
    int uncompressed_bytes;

    uint8_t interlacing_shift;
    uint8_t interlacing_leftweight;

    /* 2^result = something to do with output waiting.
     * perhaps matters if we read > 1 frame in a pass?
     */
    readbits(alac, 4);

    readbits(alac, 12); /* unknown, skip 12 bits */

    hassize = readbits(alac, 1); /* the output sample size is stored soon */

    /* the number of bytes in the (compressed) stream that are not compressed */
    uncompressed_bytes = readbits(alac, 2);

    isnotcompressed = readbits(alac, 1); /* whether the frame is compressed */

    if (hassize)
    {
        /* now read the number of samples,
         * as a 32bit integer */
        outputsamples = readbits(alac, 32);
    }
    /* the output buffers hold ALAC_BLOCKSIZE */
    if (outputsamples < 0 || outputsamples > ALAC_BLOCKSIZE)
        return -1;

    readsamplesize = infosamplesize - (uncompressed_bytes * 8) + 1;

    yield();
    if (!isnotcompressed)
    { /* compressed */
        int predictor_coef_num_a;
        int prediction_type_a;
        int prediction_quantitization_a;
        int ricemodifier_a;

        int predictor_coef_num_b;
        int prediction_type_b;
        int prediction_quantitization_b;
        int ricemodifier_b;

        int i;

        interlacing_shift = readbits(alac, 8);
        interlacing_leftweight = readbits(alac, 8);

        /******** channel 1 ***********/
        prediction_type_a = readbits(alac, 4);
        prediction_quantitization_a = readbits(alac, 4);

        ricemodifier_a = readbits(alac, 3);
        predictor_coef_num_a = readbits(alac, 5);

        /* read the predictor table */
        for (i = 0; i < predictor_coef_num_a; i++)
        {
            predictor_coef_table_a[i] = (int16_t)readbits(alac, 16);
        }

        /******** channel 2 *********/
        prediction_type_b = readbits(alac, 4);
        prediction_quantitization_b = readbits(alac, 4);

        ricemodifier_b = readbits(alac, 3);
        predictor_coef_num_b = readbits(alac, 5);

        /* read the predictor table */
        for (i = 0; i < predictor_coef_num_b; i++)
        {
            predictor_coef_table_b[i] = (int16_t)readbits(alac, 16);
        }

        /*********************/
        if (uncompressed_bytes)
        { /* see mono case */
            int i;
            for (i = 0; i < outputsamples; i++)
            {
                shift_buffer[0][i] = readbits(alac, uncompressed_bytes * 8);
                shift_buffer[1][i] = readbits(alac, uncompressed_bytes * 8);
            }
        }

        yield();
        /* channel 1 */
        entropy_rice_decode(alac,
                            outputbuffer[0],
                            outputsamples,
                            readsamplesize,
                            alac->setinfo_rice_initialhistory,
                            alac->setinfo_rice_kmodifier,
                            ricemodifier_a * alac->setinfo_rice_historymult / 4,
                            (1 << alac->setinfo_rice_kmodifier) - 1);

        yield();
        if (prediction_type_a == 0)
        { /* adaptive fir */
            predictor_decompress_fir_adapt(outputbuffer[0],
                                           outputbuffer[0],
                                           outputsamples,
                                           readsamplesize,
                                           predictor_coef_table_a,
                                           predictor_coef_num_a,
                                           prediction_quantitization_a);
        }
        else
        { /* see mono case */
            //fprintf(stderr, "FIXME: unhandled predicition type: %i\n", prediction_type_a);
        }

        yield();

        /* channel 2 */
        entropy_rice_decode(alac,
                            outputbuffer[1],
                            outputsamples,
                            readsamplesize,
                            alac->setinfo_rice_initialhistory,
                            alac->setinfo_rice_kmodifier,
                            ricemodifier_b * alac->setinfo_rice_historymult / 4,
                            (1 << alac->setinfo_rice_kmodifier) - 1);

        yield();
        if (prediction_type_b == 0)
        { /* adaptive fir */
            predictor_decompress_fir_adapt(outputbuffer[1],
                                           outputbuffer[1],
                                           outputsamples,
                                           readsamplesize,
                                           predictor_coef_table_b,
                                           predictor_coef_num_b,
                                           prediction_quantitization_b);
        }
        else
        {
            //fprintf(stderr, "FIXME: unhandled predicition type: %i\n", prediction_type_b);
        }
    }
    else
    { /* not compressed, easy case */
        if (infosamplesize <= 16)
        {
            int i;
            for (i = 0; i < outputsamples; i++)
            {
                int32_t audiobits_a, audiobits_b;

                audiobits_a = readbits(alac, infosamplesize);
                audiobits_b = readbits(alac, infosamplesize);

                audiobits_a = SIGN_EXTENDED32(audiobits_a, infosamplesize);
                audiobits_b = SIGN_EXTENDED32(audiobits_b, infosamplesize);

                outputbuffer[0][i] = audiobits_a;
                outputbuffer[1][i] = audiobits_b;
            }
        }
        else
        {
            int i;
            for (i = 0; i < outputsamples; i++)
            {
                int32_t audiobits_a, audiobits_b;

                audiobits_a = readbits(alac, 16);
                audiobits_a = audiobits_a << (infosamplesize - 16);
                audiobits_a |= readbits(alac, infosamplesize - 16);
                audiobits_a = SIGNEXTEND24(audiobits_a);

                audiobits_b = readbits(alac, 16);
                audiobits_b = audiobits_b << (infosamplesize - 16);
                audiobits_b |= readbits(alac, infosamplesize - 16);
                audiobits_b = SIGNEXTEND24(audiobits_b);

                outputbuffer[0][i] = audiobits_a;
                outputbuffer[1][i] = audiobits_b;
            }
        }
        uncompressed_bytes = 0; // always 0 for uncompressed
        interlacing_shift = 0;
        interlacing_leftweight = 0;
    }

    yield();

    switch(infosamplesize)
    {
    case 16:
    {
        deinterlace_16(outputbuffer[0],
                       outputbuffer[1],
                       outputsamples,
                       interlacing_shift,
                       interlacing_leftweight);
        break;
    }
    case 24:
    {
        deinterlace_24(outputbuffer[0],
                       outputbuffer[1],
                       uncompressed_bytes,
                       shift_buffer[0],
                       shift_buffer[1],
                       outputsamples,
                       interlacing_shift,
                       interlacing_leftweight);            
        break;
    }
    case 20:
    case 32:
        //fprintf(stderr, "FIXME: unimplemented sample size %i\n", infosamplesize);
        break;
    default:
        break;
    }
    return outputsamples;
}

int alac_decode_frame(alac_file *alac,
                      unsigned char *inbuffer,
                      int32_t outputbuffer[ALAC_MAX_CHANNELS][ALAC_BLOCKSIZE],
                      void (*yield)(void))
{
    int channels;
    int outputsamples;
    unsigned char *input_buffer_start;

    /* the rice limit sizes reads and shifts in the entropy decoder;
     * encoders write 14 */
    if (alac->setinfo_rice_kmodifier < 1 || alac->setinfo_rice_kmodifier > 25)
        return -1;

    /* setup the stream */
    alac->input_buffer = inbuffer;
    alac->input_buffer_bitaccumulator = 0;
    
    /* save to gather byte consumption */
    input_buffer_start = alac->input_buffer;

    channels = readbits(alac, 3);

    /* TODO: The mono and stereo functions should be combined. */
    switch(channels)
    {
        case 0: /* 1 channel */
            outputsamples=decode_frame_mono(alac,outputbuffer,yield);
            break;
        case 1: /* 2 channels */
            outputsamples=decode_frame_stereo(alac,outputbuffer,yield);
            break;
        default: /* Unsupported */
            return -1;
    }
    
    /* calculate consumed bytes */
    alac->bytes_consumed = (int)(alac->input_buffer - input_buffer_start);
    alac->bytes_consumed += (alac->input_buffer_bitaccumulator>5) ? 2 : 1;

    return outputsamples;
}

/* rockbox: not used
void create_alac(int samplesize, int numchannels, alac_file* alac)
{
    alac->samplesize = samplesize;
    alac->numchannels = numchannels;
    alac->bytespersample = (samplesize / 8) * numchannels;
} */
