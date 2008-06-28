/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Code that has been in mpeg.c/h before, now creating an encapsulated play
 * data module, to be used by other sources than file playback as well.
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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
#ifndef _MP3_PLAYBACK_H_
#define _MP3_PLAYBACK_H_

#include <stdbool.h>

/* functions formerly in mpeg.c */
void mp3_init(int volume, int bass, int treble, int balance, int loudness,
              int avc, int channel_config, int stereo_width,
              int mdb_strength, int mdb_harmonics,
              int mdb_center, int mdb_shape, bool mdb_enable,
              bool superbass);

/* exported just for mpeg.c, to keep the recording there */
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void demand_irq_enable(bool on);
#endif

/* new functions, exported to plugin API */
void mp3_play_init(void);
void mp3_play_data(const unsigned char* start, int size,
    void (*get_more)(unsigned char** start, size_t* size) /* callback fn */
);
void mp3_play_pause(bool play);
bool mp3_pause_done(void);
void mp3_play_stop(void);
long mp3_get_playtime(void);
void mp3_reset_playtime(void);
bool mp3_is_playing(void);
unsigned char* mp3_get_pos(void);
void mp3_shutdown(void);

#endif /* #ifndef _MP3_PLAYBACK_H_ */
