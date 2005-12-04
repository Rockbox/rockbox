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

enum {
    SOUND_VOLUME = 0,
    SOUND_BASS,
    SOUND_TREBLE,
    SOUND_BALANCE,
    SOUND_CHANNELS,
    SOUND_STEREO_WIDTH,
#ifdef HAVE_UDA1380
    SOUND_SCALING,
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    SOUND_LOUDNESS,
    SOUND_AVC,
    SOUND_MDB_STRENGTH,
    SOUND_MDB_HARMONICS,
    SOUND_MDB_CENTER,
    SOUND_MDB_SHAPE,
    SOUND_MDB_ENABLE,
    SOUND_SUPERBASS,
#endif
#if CONFIG_CODEC == MAS3587F || defined(HAVE_UDA1380)
    SOUND_LEFT_GAIN,
    SOUND_RIGHT_GAIN,
    SOUND_MIC_GAIN,
#endif
#if defined(HAVE_UDA1380)
    SOUND_ADC_LEFT_GAIN,
    SOUND_ADC_RIGHT_GAIN,
#endif
};

enum {
    SOUND_CHAN_STEREO = 0,
    SOUND_CHAN_MONO,
    SOUND_CHAN_CUSTOM,
    SOUND_CHAN_MONO_LEFT,
    SOUND_CHAN_MONO_RIGHT,
    SOUND_CHAN_KARAOKE,
};

#ifdef HAVE_UDA1380
enum {
    SOUND_SCALE_VOLUME = 0,
    SOUND_SCALE_BASS,
    SOUND_SCALE_CURRENT,
    SOUND_SCALE_OFF
};
#endif

typedef void sound_set_type(int value);

const char *sound_unit(int setting);
int sound_numdecimals(int setting);
int sound_steps(int setting);
int sound_min(int setting);
int sound_max(int setting);
int sound_default(int setting);
sound_set_type* sound_get_fn(int setting);

void sound_set_volume(int value);
void sound_set_balance(int value);
void sound_set_bass(int value);
void sound_set_treble(int value);
void sound_set_channels(int value);
void sound_set_stereo_width(int value);
#ifdef HAVE_UDA1380
void sound_set_scaling(int value);
#endif
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

void sound_set(int setting, int value);
int sound_val2phys(int setting, int value);

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void sound_set_pitch(int permille);
int sound_get_pitch(void);
#endif

#endif
