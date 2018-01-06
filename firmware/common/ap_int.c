/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2018 by Michael A. Sevakis
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
#include "ap_int.h"
#include "fixedpoint.h"

/* Miscellaneous large-sized integer functions */

/* round string, base 10 */
bool round_number_string10(char *p_rdig, long len)
{
    /*
     * * p should point to the digit that determines if rounding should occur
     * * buffer is updated in reverse
     * * an additional '1' may be added to the beginning: eg. 9.9 => 10.0
     */
#if 1 /* nearest */
    bool round = p_rdig[0] >= '5';
#else /* even */
    bool round = p_rdig[0] >= '5' && (p_rdig[-1] & 1);
#endif

    while (round && len-- > 0) {
        int d = *--p_rdig;
        round = ++d > '9';
        *p_rdig = round ? '0' : d;
    }

    if (round) {
        /* carry to the next place */
        *--p_rdig = '1';
    }

    return round;
}

/* format arbitrary-precision base 10 integer */
char * format_ap_int10(struct ap_int *a,
                       char *p_end)
{
    /*
     * * chunks are in least-to-most-significant order
     * * chunk array is used for intermediate division results
     * * digit string buffer is written high-to-low address order
     */
    long numchunks = a->numchunks;
    char *p = p_end;

    if (numchunks == 0) {
        /* fast formatting */
        uint64_t val = a->val;

        do {
            *--p = val % 10 + '0';
            val /= 10;
        } while (val);

        a->len = p_end - p;
        return p;
    }

    uint32_t *chunks = a->chunks;
    long topchunk = numchunks - 1;

    /* if top chunk(s) are zero, ignore */
    while (topchunk >= 0 && chunks[topchunk] == 0) {
        topchunk--;
    }

    /* optimized to divide number by the biggest 10^x a uint32_t can hold
       so that r_part holds the remainder (x % 1000000000) at the end of
       the division */
    do {
        uint64_t r_part = 0;

        for (long i = topchunk; i >= 0; i--) {
            /*
             * Testing showed 29 bits as a sweet spot:
             * * Is a 32-bit constant (good for 32-bit hardware)
             * * No more normalization is required than with 30 and 31
             *   (32 bits requires the least but also a large constant)
             * * Doesn't need to be reduced before hand by subtracting the
             *   divisor in order to keep it 32-bits which obviates the need
             *   to correct with another term of the remainder after
             *   multiplying
             *
             * 2305843009 = floor(ldexp(1, 29) / 1000000000.0 * ldexp(1, 32))
             */
            static const unsigned long c = 2305843009; /* .213693952 */
            uint64_t q_part = r_part*c >> 29;
            r_part = (r_part << 32) | chunks[i];
            r_part -= q_part*1000000000;

            /* if remainder is still out of modular range, normalize it
               and carry over into quotient */
            while (r_part >= 1000000000) {
                r_part -= 1000000000;
                q_part++;
            }

            chunks[i] = q_part;
        }

        /* if top chunk(s) became zero, ignore from now on */
        while (topchunk >= 0 && chunks[topchunk] == 0) {
            topchunk--;
        }

        /* format each digit chunk, padded to width 9 if not the leading one */
        uint32_t val = r_part;
        int len = 8*(topchunk >= 0);

        while (len-- >= 0 || val) {
            *--p = (val % 10) + '0';
            val /= 10;
        }
    } while (topchunk >= 0);

    a->len = p_end - p;
    return p;
}

/* format arbitrary-precision base 10 fraction */
char * format_ap_frac10(struct ap_int *a,
                        char *p_start,
                        long precision)
{
    /*
     * * chunks are in least-to-most-significant order
     * * chunk array is used for intermediate multiplication results
     * * digit string buffer is written low-to-high address order
     * * high bit of fraction must be left-justified to a chunk
     *   boundary
     */
    long numchunks = a->numchunks;
    bool trimlz = precision < 0;
    char *p = p_start;

    if (trimlz) {
        /* trim leading zeros and provide <precision> digits; a->len
           will end up greater than the specified precision unless the
           value is zero */
        precision = -precision;
    }

    a->len = precision;

    if (numchunks == 0) {
        /* fast formatting; shift must be <= 60 as top four bits are used
           for digit carryout */
        if (trimlz && !a->val) {
            /* value is zero */
            trimlz = false;
        }

        uint64_t val = a->val << (60 - a->shift);

        while (precision > 0) {
            val *= 10;
            uint32_t c_part = val >> 60;

            if (trimlz) {
                if (!c_part) {
                    a->len++;
                    continue;
                }

                trimlz = false;
            }

            *p++ = c_part + '0';
            val ^= (uint64_t)c_part << 60;
            precision--;
        }

        return p;
    }

    uint32_t *chunks = a->chunks;
    long bottomchunk = 0, topchunk = numchunks;

    while (topchunk > 0 && chunks[topchunk - 1] == 0) {
        topchunk--;
    }

    /* optimized to multiply number by the biggest 10^x a uint32_t can hold
       so that c_part holds the carryover into the integer part at the end
       of the multiplication */
    while (precision > 0) {
        /* if bottom chunk(s) are or became zero, skip them */
        while (bottomchunk < numchunks && chunks[bottomchunk] == 0) {
            bottomchunk++;
        }

        uint32_t c_part = 0;

        for (long i = bottomchunk; i < topchunk; i++) {
            uint64_t p_part = chunks[i];

            p_part = p_part * 1000000000 + c_part;
            c_part = p_part >> 32;

            chunks[i] = p_part;
        }

        if (topchunk < numchunks && c_part) {
            chunks[topchunk++] = c_part;
            c_part = 0;
        }

        int len = 9;

        if (trimlz && bottomchunk < numchunks) {
            if (!c_part) {
                a->len += 9;
                continue;
            }

            /* first non-zero chunk has leading zeros? */
            for (uint32_t val = c_part; val < 100000000; val *= 10) {
                len--;
            }

            a->len += 9 - len;
            trimlz = false;
        }

        /* format each digit chunk, padded to width 9 if not exceeding
           precision */
        precision -= len;

        if (precision < 0) {
            /* remove extra digits */
            c_part /= ipow(10, -precision);
            len += precision;
        }

        p += len;

        char *p2 = p;

        while (len-- > 0) {
            *--p2 = (c_part % 10) + '0';
            c_part /= 10;
        }
    }

    return p;
}
