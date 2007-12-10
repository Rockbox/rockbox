/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: wm8975.c 13453 2007-05-20 23:10:15Z christian $
 *
 * Driver for MAS35xx audio codec
 *
 *
 * Copyright (c) 2007 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "audiohw.h"

const struct sound_settings_info audiohw_settings[] = {
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_VOLUME]        = {"dB", 0,  1,-100,  12, -25},
    [SOUND_BASS]          = {"dB", 0,  1, -12,  12,   6},
    [SOUND_TREBLE]        = {"dB", 0,  1, -12,  12,   6},
    [SOUND_BALANCE]       = {"dB", 0,  1,-128, 127,   0},
#else /* MAS3507D */
    [SOUND_VOLUME]        = {"dB", 0,  1, -78,  18, -18},
    [SOUND_BASS]          = {"dB", 0,  1, -15,  15,   7},
    [SOUND_TREBLE]        = {"dB", 0,  1, -15,  15,   7},
    [SOUND_BALANCE]       = {"dB", 0,  1, -96,  96,   0},
#endif
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_LOUDNESS]      = {"dB", 0,  1,   0,  17,   0},
    [SOUND_AVC]           = {"",   0,  1,  -1,   4,   0},
    [SOUND_MDB_STRENGTH]  = {"dB", 0,  1,   0, 127,  48},
    [SOUND_MDB_HARMONICS] = {"%",  0,  1,   0, 100,  50},
    [SOUND_MDB_CENTER]    = {"Hz", 0, 10,  20, 300,  60},
    [SOUND_MDB_SHAPE]     = {"Hz", 0, 10,  50, 300,  90},
    [SOUND_MDB_ENABLE]    = {"",   0,  1,   0,   1,   0},
    [SOUND_SUPERBASS]     = {"",   0,  1,   0,   1,   0},
#endif
#if CONFIG_CODEC == MAS3587F
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  15,   8},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  15,   8},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,  15,   2},
#endif
};
