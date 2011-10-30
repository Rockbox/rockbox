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
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "platform.h"
#include "strlcpy.h"
#include "strcasecmp.h"
#include "metadata.h"
#include "debug.h"
#include "replaygain.h"
#include "fixedpoint.h"

#define FP_BITS         (12)
#define FP_ONE          (1 << FP_BITS)
#define FP_MIN          (-48 * FP_ONE)
#define FP_MAX          ( 17 * FP_ONE)

void replaygain_itoa(char* buffer, int length, long int_gain)
{
    /* int_gain uses Q19.12 format. */
    int one  = abs(int_gain) >> FP_BITS;
    int cent = ((abs(int_gain) & 0x0fff) * 100 + (FP_ONE/2)) >> FP_BITS;
    snprintf(buffer, length, "%s%d.%02d dB", (int_gain<0) ? "-":"", one, cent);
}

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
     * Our math code can't handle it properly anyway. :) */
    gain = MAX(gain, FP_MIN);
    gain = MIN(gain, FP_MAX);

    return fp_factor(gain, FP_BITS) << (24 - FP_BITS);
}

/* Get the sample scale factor in Q19.12 format from a gain value. Returns 0
 * for no gain.
 *
 * str  Gain in dB as a string. E.g., "-3.45 dB"; the "dB" part is ignored.
 */
static long get_replaygain(const char* str)
{
    return fp_atof(str, FP_BITS);
}

/* Get the peak volume in Q7.24 format.
 *
 * str  Peak volume. Full scale is specified as "1.0". Returns 0 for no peak.
 */
static long get_replaypeak(const char* str)
{
    return fp_atof(str, 24);
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
 * are not overwritten.
 *
 * key     Name of the tag.
 * value   Value of the tag.
 * entry   mp3entry struct to update.
 */
void parse_replaygain(const char* key, const char* value, 
                      struct mp3entry* entry)
{
    if (((strcasecmp(key, "replaygain_track_gain") == 0) || 
         (strcasecmp(key, "rg_radio") == 0)) && 
        !entry->track_gain)
    {
        entry->track_level = get_replaygain(value);
        entry->track_gain  = convert_gain(entry->track_level);
    }
    else if (((strcasecmp(key, "replaygain_album_gain") == 0) || 
              (strcasecmp(key, "rg_audiophile") == 0)) && 
             !entry->album_gain)
    {
        entry->album_level = get_replaygain(value);
        entry->album_gain  = convert_gain(entry->album_level);
    }
    else if (((strcasecmp(key, "replaygain_track_peak") == 0) || 
              (strcasecmp(key, "rg_peak") == 0)) && 
             !entry->track_peak)
    {
        entry->track_peak = get_replaypeak(value);
    }
    else if ((strcasecmp(key, "replaygain_album_peak") == 0) && 
             !entry->album_peak)
    {
        entry->album_peak = get_replaypeak(value);
    }
}

/* Set ReplayGain values from integers. Existing values are not overwritten. 
 *
 * album   If true, set album values, otherwise set track values.
 * gain    Gain value in dB, multiplied by 512. 0 for no gain.
 * peak    Peak volume in Q7.24 format, where 1.0 is full scale. 0 for no 
 *         peak volume.
 * entry   mp3entry struct to update.
 */
void parse_replaygain_int(bool album, long gain, long peak,
                          struct mp3entry* entry)
{
    gain = gain * FP_ONE / 512;

    if (album)
    {
        entry->album_level = gain;
        entry->album_gain  = convert_gain(gain);
        entry->album_peak  = peak;
    }
    else
    {
        entry->track_level = gain;
        entry->track_gain  = convert_gain(gain);
        entry->track_peak  = peak;
    }
}
