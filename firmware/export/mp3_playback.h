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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _MP3_PLAYBACK_H_
#define _MP3_PLAYBACK_H_

#include <stdbool.h>

/* functions formerly in mpeg.c */
void mp3_init(int volume, int bass, int treble, int balance,
               int loudness, int bass_boost, int avc, int channel_config);
void mpeg_sound_set(int setting, int value);
int mpeg_sound_min(int setting);
int mpeg_sound_max(int setting);
int mpeg_sound_default(int setting);
void mpeg_sound_channel_config(int configuration);
int mpeg_val2phys(int setting, int value);
int mpeg_phys2val(int setting, int value);
char *mpeg_sound_unit(int setting);
int mpeg_sound_numdecimals(int setting);
#if defined(HAVE_MAS3587F) || defined(SIMULATOR)
void mpeg_set_pitch(int percent);
#endif


/* exported just for mpeg.c, to keep the recording there */
#ifdef HAVE_MAS3587F
void demand_irq_enable(bool on);
#endif

/* new functions, exported to plugin API */
void mp3_play_init(void);
void mp3_play_data(unsigned char* start, int size,
    void (*get_more)(unsigned char** start, int* size) /* callback fn */
);
void mp3_play_pause(bool play);
void mp3_play_stop(void);
long mp3_get_playtime(void);
void mp3_reset_playtime(void);
bool mp3_is_playing(void);


#define SOUND_VOLUME 0
#define SOUND_BASS 1
#define SOUND_TREBLE 2
#define SOUND_BALANCE 3
#define SOUND_LOUDNESS 4
#define SOUND_SUPERBASS 5
#define SOUND_AVC 6
#define SOUND_CHANNELS 7
#define SOUND_LEFT_GAIN 8
#define SOUND_RIGHT_GAIN 9
#define SOUND_MIC_GAIN 10
#define SOUND_NUMSETTINGS 11

#define MPEG_SOUND_STEREO 0
#define MPEG_SOUND_STEREO_NARROW 1
#define MPEG_SOUND_MONO 2
#define MPEG_SOUND_MONO_LEFT 3
#define MPEG_SOUND_MONO_RIGHT 4
#define MPEG_SOUND_KARAOKE 5
#define MPEG_SOUND_STEREO_WIDE 6

#endif /* #ifndef _MP3_PLAYBACK_H_ */
