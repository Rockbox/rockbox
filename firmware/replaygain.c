/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Magnus Holmgren
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

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system.h>
#include "id3.h"
#include "debug.h"
#include "replaygain.h"

/* The fixed point math routines (with the exception of fp_atof) are based
 * on oMathFP by Dan Carter (http://orbisstudios.com).
 */

/* 12 bits of precision gives fairly accurate result, but still allows a
 * compact implementation. The math code supports up to 13...
 */

#define FP_BITS         (12)
#define FP_MASK         ((1 << FP_BITS) - 1)
#define FP_ONE          (1 << FP_BITS)
#define FP_TWO          (2 << FP_BITS)
#define FP_HALF         (1 << (FP_BITS - 1))
#define FP_LN2          ( 45426 >> (16 - FP_BITS))
#define FP_LN2_INV      ( 94548 >> (16 - FP_BITS))
#define FP_EXP_ZERO     ( 10922 >> (16 - FP_BITS))
#define FP_EXP_ONE      (  -182 >> (16 - FP_BITS))
#define FP_EXP_TWO      (     4 >> (16 - FP_BITS))
#define FP_INF          (0x7fffffff)
#define FP_LN10         (150902 >> (16 - FP_BITS))

#define FP_MAX_DIGITS       (4)
#define FP_MAX_DIGITS_INT   (10000)

#define FP_FAST_MUL_DIV

#ifdef FP_FAST_MUL_DIV

/* These macros can easily overflow, but they are good enough for our uses,
 * and saves some code.
 */
#define fp_mul(x, y) (((x) * (y)) >> FP_BITS)
#define fp_div(x, y) (((x) << FP_BITS) / (y))

#else

static long fp_mul(long x, long y)
{
    long x_neg = 0;
    long y_neg = 0;
    long rc;

    if ((x == 0) || (y == 0))
    {
        return 0;
    }

    if (x < 0)
    {
        x_neg = 1;
        x = -x;
    }

    if (y < 0)
    {
        y_neg = 1;
        y = -y;
    }

    rc = (((x >> FP_BITS) * (y >> FP_BITS)) << FP_BITS)
        + (((x & FP_MASK) * (y & FP_MASK)) >> FP_BITS)
        + ((x & FP_MASK) * (y >> FP_BITS))
        + ((x >> FP_BITS) * (y & FP_MASK));

    if ((x_neg ^ y_neg) == 1)
    {
        rc = -rc;
    }

    return rc;
}

static long fp_div(long x, long y)
{
    long x_neg = 0;
    long y_neg = 0;
    long shifty;
    long rc;
    int msb = 0;
    int lsb = 0;

    if (x == 0)
    {
        return 0;
    }

    if (y == 0)
    {
        return (x < 0) ? -FP_INF : FP_INF;
    }

    if (x < 0)
    {
        x_neg = 1;
        x = -x;
    }

    if (y < 0)
    {
        y_neg = 1;
        y = -y;
    }

    while ((x & (1 << (30 - msb))) == 0)
    {
        msb++;
    }

    while ((y & (1 << lsb)) == 0)
    {
        lsb++;
    }

    shifty = FP_BITS - (msb + lsb);
    rc = ((x << msb) / (y >> lsb));

    if (shifty > 0)
    {
        rc <<= shifty;
    }
    else
    {
        rc >>= -shifty;
    }

    if ((x_neg ^ y_neg) == 1)
    {
        rc = -rc;
    }

    return rc;
}

#endif /* FP_FAST_MUL_DIV */

static long fp_exp(long x)
{
    long k;
    long z;
    long R;
    long xp;

    if (x == 0)
    {
        return FP_ONE;
    }

    k = (fp_mul(abs(x), FP_LN2_INV) + FP_HALF) & ~FP_MASK;

    if (x < 0)
    {
        k = -k;
    }

    x -= fp_mul(k, FP_LN2);
    z = fp_mul(x, x);
    R = FP_TWO + fp_mul(z, FP_EXP_ZERO + fp_mul(z, FP_EXP_ONE
        + fp_mul(z, FP_EXP_TWO)));
    xp = FP_ONE + fp_div(fp_mul(FP_TWO, x), R - x);

    if (k < 0)
    {
        k = FP_ONE >> (-k >> FP_BITS);
    }
    else
    {
        k = FP_ONE << (k >> FP_BITS);
    }

    return fp_mul(k, xp);
}

static long fp_exp10(long x)
{
    if (x == 0)
    {
        return FP_ONE;
    }

    return fp_exp(fp_mul(FP_LN10, x));
}

static long fp_atof(const char* s, int precision)
{
    long int_part = 0;
    long int_one = 1 << precision;
    long frac_part = 0;
    long frac_count = 0;
    long frac_max = ((precision * 4) + 12) / 13;
    long frac_max_int = 1;
    long sign = 1;
    bool point = false;

    while ((*s != '\0') && isspace(*s))
    {
        s++;
    }

    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }

    while (*s != '\0')
    {
        if (*s == '.')
        {
            if (point)
            {
                break;
            }

            point = true;
        }
        else if (isdigit(*s))
        {
            if (point)
            {
                if (frac_count < frac_max)
                {
                    frac_part = frac_part * 10 + (*s - '0');
                    frac_count++;
                    frac_max_int *= 10;
                }
            }
            else
            {
                int_part = int_part * 10 + (*s - '0');
            }
        }
        else
        {
            break;
        }

        s++;
    }

    while (frac_count < frac_max)
    {
      frac_part *= 10;
      frac_count++;
      frac_max_int *= 10;
    }

    return sign * ((int_part * int_one)
        + (((int64_t) frac_part * int_one) / frac_max_int));
}

static long convert_gain(long gain)
{
    /* Don't allow unreasonably low or high gain changes.
     * Our math code can't handle it properly anyway. :)
     */
    if (gain < (-48 * FP_ONE))
    {
        gain = -48 * FP_ONE;
    }

    if (gain > (17 * FP_ONE))
    {
        gain = 17 * FP_ONE;
    }

    gain = fp_exp10(gain / 20) << (24 - FP_BITS);

    return gain;
}

/* Get the sample scale factor in Q7.24 format from a gain value. Returns 0
 * for no gain.
 *
 * str  Gain in dB as a string. E.g., "-3.45 dB"; the "dB" part is ignored.
 */
static long get_replaygain(const char* str)
{
    long gain = 0;

    if (str)
    {
        gain = fp_atof(str, FP_BITS);
        gain = convert_gain(gain);
    }

    return gain;
}

/* Get the peak volume in Q7.24 format.
 *
 * str  Peak volume. Full scale is specified as "1.0". Returns 0 for no peak.
 */
static long get_replaypeak(const char* str)
{
    long peak = 0;

    if (str)
    {
        peak = fp_atof(str, 24);
    }

    return peak;
}

/* Get a sample scale factor in Q7.24 format from a gain value.
 *
 * int_gain  Gain in dB, multiplied by 100.
 */
long get_replaygain_int(long int_gain)
{
    return convert_gain(int_gain * FP_ONE / 100);
}

/* Parse a ReplayGain tag conforming to the "VorbisGain standard". If a
 * valid tag is found, update mp3entry struct accordingly. Existing values 
 * are not overwritten. Returns number of bytes written to buffer.
 *
 * key     Name of the tag.
 * value   Value of the tag.
 * entry   mp3entry struct to update.
 * buffer  Where to store the text for gain values (for later display).
 * length  Bytes left in buffer.
 */
long parse_replaygain(const char* key, const char* value,
    struct mp3entry* entry, char* buffer, int length)
{
    char **p = NULL;

    if (((strcasecmp(key, "replaygain_track_gain") == 0)
        || (strcasecmp(key, "rg_radio") == 0)) && !entry->track_gain)
    {
        entry->track_gain = get_replaygain(value);
        p = &(entry->track_gain_string);
    }
    else if (((strcasecmp(key, "replaygain_album_gain") == 0)
        || (strcasecmp(key, "rg_audiophile") == 0)) && !entry->album_gain)
    {
        entry->album_gain = get_replaygain(value);
        p = &(entry->album_gain_string);
    }
    else if (((strcasecmp(key, "replaygain_track_peak") == 0)
        || (strcasecmp(key, "rg_peak") == 0)) && !entry->track_peak)
    {
        entry->track_peak = get_replaypeak(value);
    }
    else if ((strcasecmp(key, "replaygain_album_peak") == 0)
        && !entry->album_peak)
    {
        entry->album_peak = get_replaypeak(value);
    }

    if (p)
    {
        int len = strlen(value);

        len = MIN(len, length - 1);

        /* A few characters just isn't interesting... */
        if (len > 1)
        {
            strncpy(buffer, value, len);
            buffer[len] = 0;
            *p = buffer;
            return len + 1;
        }
    }

    return 0;
}

/* Set ReplayGain values from integers. Existing values are not overwritten. 
 * Returns number of bytes written to buffer.
 *
 * album   If true, set album values, otherwise set track values.
 * gain    Gain value in dB, multiplied by 512. 0 for no gain.
 * peak    Peak volume in Q7.24 format, where 1.0 is full scale. 0 for no 
 *         peak volume.
 * buffer  Where to store the text for gain values (for later display).
 * length  Bytes left in buffer.
 */
long parse_replaygain_int(bool album, long gain, long peak,
    struct mp3entry* entry, char* buffer, int length)
{
    long len = 0;

    if (buffer != NULL)
    {
        len = snprintf(buffer, length, "%d.%02d dB", gain / 512,
            ((abs(gain) & 0x01ff) * 100 + 256) / 512);
        len++;
    }

    if (gain != 0)
    {
        gain = convert_gain(gain * FP_ONE / 512);
    }

    if (album)
    {
        entry->album_gain = gain;
        entry->album_gain_string = buffer;

        if (peak)
        {
            entry->album_peak = peak;
        }
    }
    else
    {
        entry->track_gain = gain;
        entry->track_gain_string = buffer;

        if (peak)
        {
            entry->track_peak = peak;
        }
    }
    
    return len;
}
