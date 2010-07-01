/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Namco custom waveform sound generator 3 (Pacman hardware)
 *
 * Copyright (c) 2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
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
#include "plugin.h"
#include "wsg3.h"

struct wsg3 wsg3;

static bool wsg3_get_voice(struct wsg3_voice *voice, int index)
{
    int base = 5*index;
    const unsigned char *regs = wsg3.sound_regs;
    unsigned x;
    
    x = regs[0x15 + base] & 0x0F;

    if (x == 0)
        return false;

    voice->volume = x;

    x =            (regs[0x14 + base] & 0x0F);
    x = (x << 4) | (regs[0x13 + base] & 0x0F);
    x = (x << 4) | (regs[0x12 + base] & 0x0F);
    x = (x << 4) | (regs[0x11 + base] & 0x0F);
    x = (x << 4);

    if (index == 0)
    {
        /* The first voice has an extra 4-bit of data */
        x |= regs[0x10 + base] & 0x0F; 
    }

    if (x == 0)
        return false;

    voice->frequency = x;

    voice->waveform = regs[0x05 + base] & 0x07;

    return true;
}

void wsg3_play_sound(int16_t * buf, int len)
{
    struct wsg3_voice voice;

    if (wsg3_get_voice(&voice, 0))
    {
        unsigned offset = wsg3.wave_offset[0];
        unsigned step = voice.frequency * wsg3.resample_step;
        int16_t * wave_data = wsg3.sound_wave_data + 32 * voice.waveform;
        int volume = voice.volume;
        int i;

        for (i = 0; i < len; i++)
        {
            /* Should be shifted right by 15, but we must also get rid
             * of the 10 bits used for decimals */
            buf[i] = (int)wave_data[(offset >> 25) & 0x1F] * volume;
            offset += step;
        }

        wsg3.wave_offset[0] = offset;
    }

    if (wsg3_get_voice(&voice, 1))
    {
        unsigned offset = wsg3.wave_offset[1];
        unsigned step = voice.frequency * wsg3.resample_step;
        int16_t * wave_data = wsg3.sound_wave_data + 32 * voice.waveform;
        int volume = voice.volume;
        int i;

        for (i = 0; i < len; i++)
        {
            /* Should be shifted right by 15, but we must also get rid
             * of the 10 bits used for decimals */
            buf[i] += (int)wave_data[(offset >> 25) & 0x1F] * volume;
            offset += step;
        }

        wsg3.wave_offset[1] = offset;
    }

    if (wsg3_get_voice(&voice, 2))
    {
        unsigned offset = wsg3.wave_offset[2];
        unsigned step = voice.frequency * wsg3.resample_step;
        int16_t * wave_data = wsg3.sound_wave_data + 32 * voice.waveform;
        int volume = voice.volume;
        int i;

        for (i = 0; i < len; i++)
        {
            /* Should be shifted right by 15, but we must also get rid
             * of the 10 bits used for decimals */
            buf[i] += (int)wave_data[(offset >> 25) & 0x1F] * volume;
            offset += step;
        }

        wsg3.wave_offset[2] = offset;
    }
}

void wsg3_set_sampling_rate(unsigned sampling_rate)
{
    wsg3.sampling_rate = sampling_rate;
    wsg3.resample_step = (wsg3.master_clock << 10) / sampling_rate;
}

void wsg3_set_sound_prom( const unsigned char * prom )
{
    int i;

    memcpy(wsg3.sound_prom, prom, 32*8);

    /* Copy wave data and convert 4-bit unsigned -> 16-bit signed,
     * prenormalized */
    for (i = 0; i < 32*8; i++)
        wsg3.sound_wave_data[i] = ((int16_t)wsg3.sound_prom[i] - 8) * 85;
}

void wsg3_init(unsigned master_clock)
{
    memset(&wsg3, 0, sizeof (struct wsg3));
    wsg3.master_clock = master_clock;
}
