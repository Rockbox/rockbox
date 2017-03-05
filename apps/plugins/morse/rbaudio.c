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

#define AMPLITUDE INT16_MAX

static void rb_dit(void *userdata, int wpm)
{
    int dot = 1200 / wpm;
    rb->beep_play(1000, dot, AMPLITUDE);
    while(rb->mixer_channel_status(PCM_MIXER_CHAN_BEEP) == CHANNEL_PLAYING);
}

static void rb_dah(void *userdata, int wpm)
{
    int dot = 1200 / wpm;
    rb->beep_play(1000, dot * 3, AMPLITUDE);
    while(rb->mixer_channel_status(PCM_MIXER_CHAN_BEEP) == CHANNEL_PLAYING);
}

static void rb_interword_silence(void *userdata, int wpm)
{
    int dot = 1200 / wpm;
    rb->sleep((7 * dot) / (1000 / HZ));
}

static void rb_intraword_silence(void *userdata, int wpm)
{
    int dot = 1200 / wpm;
    rb->sleep((3 * dot) / (1000 / HZ));
}

static void rb_intrachar_silence(void *userdata, int wpm)
{
    int dot = 1200 / wpm;
    rb->sleep((dot * 1) / (1000 / HZ));
}

const struct morse_output_api rbaudio_api = {
    NULL,
    rb_dit,
    rb_dah,
    rb_interword_silence,
    rb_intraword_silence,
    rb_intrachar_silence,
    NULL,
};
