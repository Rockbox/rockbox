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
#include "metadata.h"
#include "debug.h"
#include "replaygain.h"
#include "fixedpoint.h"

#define FP_BITS         (12)
#define FP_ONE          (1 << FP_BITS)


static long fp_atof(const char* s, int precision)
{
    long int_part = 0;
    long int_one = BIT_N(precision);
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

    gain = fp_factor(gain, FP_BITS) << (24 - FP_BITS);

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
            strlcpy(buffer, value, len + 1);
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
