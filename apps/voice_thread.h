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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
