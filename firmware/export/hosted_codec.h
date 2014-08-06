/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#ifndef HOSTED_CODEC_H
#define HOSTED_CODEC_H

#if defined(HAVE_SDL_AUDIO) \
    && !(CONFIG_PLATFORM & PLATFORM_MAEMO5)
AUDIOHW_SETTING(VOLUME,      "dB",   0,  1, -80,   0,   0)
#else
#define AUDIOHW_CAPS    (MONO_VOL_CAP)
AUDIOHW_SETTING(VOLUME,      "dB",   0,  1, -99,   0,   0)
#endif /* CONFIG_PLATFORM & PLATFORM_SDL */

#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
/* Bass and treble tone controls */
#ifdef AUDIOHW_HAVE_BASS
AUDIOHW_SETTING(BASS,        "dB",   0,  1, -24,  24,   0)
#endif
#ifdef AUDIOHW_HAVE_TREBLE
AUDIOHW_SETTING(TREBLE,      "dB",   0,  1, -24,  24,   0)
#endif
#if defined(HAVE_RECORDING)
AUDIOHW_SETTING(LEFT_GAIN,   "dB",   1,  1,-128,  96,   0)
AUDIOHW_SETTING(RIGHT_GAIN,  "dB",   1,  1,-128,  96,   0)
AUDIOHW_SETTING(MIC_GAIN,    "dB",   1,  1,-128, 108,  16)
#endif
#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
AUDIOHW_SETTING(BASS_CUTOFF,   "",   0,  1,   1,   4,   1)
#endif
#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
AUDIOHW_SETTING(TREBLE_CUTOFF, "",   0,  1,   1,   4,   1)
#endif
#endif /* CONFIG_PLATFORM & PLATFORM_ANDROID */

#endif /* HOSTED_CODEC_H */
