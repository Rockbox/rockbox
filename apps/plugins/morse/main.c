/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"

#include "morse.h"

static void rb_beep(int freq_hz, int len_ms)
{
    rb->beep_play(freq_hz, len_ms, 1500);
}

static void rb_silence(int len_ms)
{
    rb->sleep(len_ms / (1000 / HZ));
}

const struct morse_output_api rbaudio_api = {
    rb_beep,
    rb_silence
};

const char *koch_method = "KMRSUAPTLOWINJEF0YVG5Q9ZH38B427C1D6X";

const char *char_format(char *buf, size_t sz, int n, const char *unit)
{
    (void) unit;
    if(n == 2)
        return "K and M";
    else
    {
        rb->snprintf(buf, sz, "All the above and %c", koch_method[n - 1]);
        return buf;
    }
}

static void learn_koch(void)
{
    int wpm = 15;

    rb->set_int("Words per Minute", "", UNIT_INT, &wpm, NULL, 1, 10, 25, NULL);

    /* length of a dot in milliseconds */
    int dot = 1200 / wpm;

    rb->srand(*rb->current_tick);

    int range = 2;
    rb->set_int("Characters", "", UNIT_INT, &range, NULL, 1, 2, 36, char_format);

    int n = 0;
    for(;;)
    {
        char str[2];
        str[0] = koch_method[rb->rand() % range];
        str[1] = '\0';
        morse_play(str, dot, &rbaudio_api);

        if(!(n = (n+1)%5))
            rb->sleep(dot * 7 / (1000 / HZ));
    }
}

static void play_file(const char *fname)
{
    /* TODO */
}

enum plugin_status plugin_start(const void *param)
{
    if(!param)
        learn_koch();
    else
        play_file(param);
    return PLUGIN_OK;
}
