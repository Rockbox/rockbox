/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2002 Dave Chapman
 *
 * sound.h - common sound driver file.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _SOUND_H
#define _SOUND_H

#ifdef HAVE_OSS

/* The "sound device type" */

typedef struct {
  int fd;
  int freq;
  int channels;
} sound_t;

#else
 #ifdef WIN32
   #warning "No sound yet in win32"
 #else
   #warning "No sound in this environment"
 #endif
#endif

int init_sound(sound_t* sound);
int config_sound(sound_t* sound, int sound_freq, int channels);
void close_sound(sound_t* sound);
int output_sound(sound_t* sound,const void* buf, int count);

#endif
