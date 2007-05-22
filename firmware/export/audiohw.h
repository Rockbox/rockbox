/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _AUDIOHW_H_
#define _AUDIOHW_H_

#include "config.h"

enum {
    SOUND_VOLUME = 0,
    SOUND_BASS,
    SOUND_TREBLE,
    SOUND_BALANCE,
    SOUND_CHANNELS,
    SOUND_STEREO_WIDTH,
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
#if CONFIG_CODEC == MAS3587F || defined(HAVE_UDA1380) || defined(HAVE_TLV320)\
    || defined(HAVE_WM8975) || defined(HAVE_WM8758) || defined(HAVE_WM8731)
    SOUND_LEFT_GAIN,
    SOUND_RIGHT_GAIN,
    SOUND_MIC_GAIN,
#endif
};

enum Channel {
    SOUND_CHAN_STEREO,
    SOUND_CHAN_MONO,
    SOUND_CHAN_CUSTOM,
    SOUND_CHAN_MONO_LEFT,
    SOUND_CHAN_MONO_RIGHT,
    SOUND_CHAN_KARAOKE,
    SOUND_CHAN_NUM_MODES,
};

struct sound_settings_info {
    const char *unit;
    int numdecimals;
    int steps;
    int minval;
    int maxval;
    int defaultval;
};

/* This struct is used by every driver to export its min/max/default values for
 * its audio settings. Keep in mind that the order must be correct! */
extern const struct sound_settings_info audiohw_settings[];

#endif /* _AUDIOHW_H_ */
