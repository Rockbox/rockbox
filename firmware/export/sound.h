/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SOUND_H
#define SOUND_H

#define SOUND_VOLUME 0
#define SOUND_BASS 1
#define SOUND_TREBLE 2
#define SOUND_BALANCE 3
#define SOUND_LOUDNESS 4
#define SOUND_AVC 5
#define SOUND_CHANNELS 6
#define SOUND_STEREO_WIDTH 7
#define SOUND_LEFT_GAIN 8
#define SOUND_RIGHT_GAIN 9
#define SOUND_MIC_GAIN 10
#define SOUND_MDB_STRENGTH 11
#define SOUND_MDB_HARMONICS 12
#define SOUND_MDB_CENTER 13
#define SOUND_MDB_SHAPE 14
#define SOUND_MDB_ENABLE 15
#define SOUND_SUPERBASS 16
#define SOUND_NUMSETTINGS 17

#define SOUND_CHAN_STEREO 0
#define SOUND_CHAN_MONO 1
#define SOUND_CHAN_CUSTOM 2
#define SOUND_CHAN_MONO_LEFT 3
#define SOUND_CHAN_MONO_RIGHT 4
#define SOUND_CHAN_KARAOKE 5

#ifndef SIMULATOR
void sound_set_volume(int value);
void sound_set_balance(int value);
void sound_set_bass(int value);
void sound_set_treble(int value);
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void sound_set_loudness(int value);
void sound_set_avc(int value);
void sound_set_mdb_strength(int value);
void sound_set_mdb_harmonics(int value);
void sound_set_mdb_center(int value);
void sound_set_mdb_shape(int value);
void sound_set_mdb_enable(int value);
void sound_set_superbass(int value);
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
void sound_set_channels(int value);
void sound_set_stereo_width(int value);
#endif

void (*sound_get_fn(int setting))(int value);
void sound_set(int setting, int value);
int sound_min(int setting);
int sound_max(int setting);
int sound_default(int setting);
void sound_channel_config(int configuration);
int sound_val2phys(int setting, int value);
const char *sound_unit(int setting);
int sound_numdecimals(int setting);
int sound_steps(int setting);
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) || defined(SIMULATOR)
void sound_set_pitch(int permille);
int sound_get_pitch(void);
#endif

#endif
