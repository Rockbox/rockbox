/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _MPEG_H_
#define _MPEG_H_

#include <stdbool.h>

void mpeg_init(int volume, int bass, int treble);
void mpeg_play(char* trackname);
void mpeg_stop(void);
void mpeg_pause(void);
void mpeg_resume(void);
void mpeg_next(void);
void mpeg_prev(void);
bool mpeg_is_playing(void);
void mpeg_sound_set(int setting, int value);
int mpeg_sound_min(int setting);
int mpeg_sound_max(int setting);
int mpeg_sound_default(int setting);
int mpeg_val2phys(int setting, int value);
char *mpeg_sound_unit(int setting);
int mpeg_sound_numdecimals(int setting);
struct mp3entry* mpeg_current_track(void);

#define SOUND_VOLUME 0
#define SOUND_BASS 1
#define SOUND_TREBLE 2
#define SOUND_BALANCE 3

#ifdef ARCHOS_RECORDER
#define SOUND_LOUDNESS 4
#define SOUND_SUPERBASS 5
#define SOUND_NUMSETTINGS 6
#else
#define SOUND_DEEMPH 4
#define SOUND_NUMSETTINGS 5
#endif

#endif
