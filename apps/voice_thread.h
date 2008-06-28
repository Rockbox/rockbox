/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Michael Sevakis
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
#ifndef VOICE_THREAD_H
#define VOICE_THREAD_H

void mp3_play_data(const unsigned char* start, int size,
                   void (*get_more)(unsigned char** start, size_t* size));
void mp3_play_stop(void);
void mp3_play_pause(bool play);
bool mp3_is_playing(void);

void voice_stop(void);
void voice_thread_init(void);
void voice_thread_resume(void);
void voice_thread_set_priority(int priority);

#endif /* VOICE_THREAD_H */
