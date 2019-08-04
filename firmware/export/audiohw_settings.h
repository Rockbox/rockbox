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
 * Copyright (C) 2007 by Christian Gmeiner
 * Copyright (C) 2013 by Michael Sevakis
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
#if defined(AUDIOHW_SOUND_SETTINGS_ENTRIES)
#undef AUDIOHW_SOUND_SETTINGS_ENTRIES
/* Define sound_setting_entries table */

#define AUDIOHW_SETTINGS(...) \
    static const struct sound_setting_entry            \
    {                                                  \
        const struct sound_settings_info *info;        \
        sound_set_type *function;                      \
    } sound_setting_entries[] =                        \
    {                                                  \
        [0 ... SOUND_LAST_SETTING-1] = { NULL, NULL }, \
        __VA_ARGS__                                    \
    };

#define AUDIOHW_SETTING_ENT(name, fn) \
    [SOUND_##name] = { .info = &_audiohw_setting_##name, .function = fn },

#elif defined(AUDIOHW_SOUND_SETTINGS_VAL2PHYS)
#undef AUDIOHW_SOUND_SETTINGS_VAL2PHYS

/* Implements sound_val2phys */
#define AUDIOHW_SETTINGS(...) \
    int sound_val2phys(int setting, int value)         \
    {                                                  \
        switch (setting)                               \
        {                                              \
        __VA_ARGS__                                    \
        default:                                       \
            return value;                              \
        }                                              \
    }

#define AUDIOHW_SETTING_ENT(name, fn) \
    case SOUND_##name: return _sound_val2phys_##name(value);

#else

/* Generate enumeration of SOUND_xxx constants */
#define AUDIOHW_SETTINGS(...) \
    enum                                               \
    {                                                  \
        __VA_ARGS__                                    \
        SOUND_LAST_SETTING,                            \
    };

#define AUDIOHW_SETTING_ENT(name, fn) \
    SOUND_##name,

#endif /* setting table type selection */

AUDIOHW_SETTINGS(
    /* TODO: Volume shouldn't be needed if device doesn't have digital
       control */
    AUDIOHW_SETTING_ENT(VOLUME,             sound_set_volume)
#if defined(AUDIOHW_HAVE_BASS)
    AUDIOHW_SETTING_ENT(BASS,               sound_set_bass)
#endif
#if defined(AUDIOHW_HAVE_TREBLE)
    AUDIOHW_SETTING_ENT(TREBLE,             sound_set_treble)
#endif
    AUDIOHW_SETTING_ENT(BALANCE,            sound_set_balance)
    AUDIOHW_SETTING_ENT(CHANNELS,           sound_set_channels)
    AUDIOHW_SETTING_ENT(STEREO_WIDTH,       sound_set_stereo_width)
    AUDIOHW_SETTING_ENT(SWAP_CHANNELS,      sound_set_swap_channels)
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    AUDIOHW_SETTING_ENT(LOUDNESS,           sound_set_loudness)
    AUDIOHW_SETTING_ENT(AVC,                sound_set_avc)
    AUDIOHW_SETTING_ENT(MDB_STRENGTH,       sound_set_mdb_strength)
    AUDIOHW_SETTING_ENT(MDB_HARMONICS,      sound_set_mdb_harmonics)
    AUDIOHW_SETTING_ENT(MDB_CENTER,         sound_set_mdb_center)
    AUDIOHW_SETTING_ENT(MDB_SHAPE,          sound_set_mdb_shape)
    AUDIOHW_SETTING_ENT(MDB_ENABLE,         sound_set_mdb_enable)
    AUDIOHW_SETTING_ENT(SUPERBASS,          sound_set_superbass)
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
#if defined(AUDIOHW_HAVE_LIN_GAIN)
    AUDIOHW_SETTING_ENT(LEFT_GAIN,          NULL)
    AUDIOHW_SETTING_ENT(RIGHT_GAIN,         NULL)
#endif
#if defined(AUDIOHW_HAVE_MIC_GAIN)
    AUDIOHW_SETTING_ENT(MIC_GAIN,           NULL)
#endif
#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
    AUDIOHW_SETTING_ENT(BASS_CUTOFF,        sound_set_bass_cutoff)
#endif
#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
    AUDIOHW_SETTING_ENT(TREBLE_CUTOFF,      sound_set_treble_cutoff)
#endif
#if defined(AUDIOHW_HAVE_DEPTH_3D)
    AUDIOHW_SETTING_ENT(DEPTH_3D,           sound_set_depth_3d)
#endif
#if defined(AUDIOHW_HAVE_FILTER_ROLL_OFF)
    AUDIOHW_SETTING_ENT(FILTER_ROLL_OFF,    sound_set_filter_roll_off)
#endif
/* Hardware EQ tone controls */
#if defined(AUDIOHW_HAVE_EQ)
    AUDIOHW_SETTING_ENT(EQ_BAND1_GAIN,      sound_set_hw_eq_band1_gain)
#if defined(AUDIOHW_HAVE_EQ_BAND1_FREQUENCY)
    AUDIOHW_SETTING_ENT(EQ_BAND1_FREQUENCY, sound_set_hw_eq_band1_frequency)
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2)
    AUDIOHW_SETTING_ENT(EQ_BAND2_GAIN,      sound_set_hw_eq_band2_gain)
#if defined(AUDIOHW_HAVE_EQ_BAND2_FREQUENCY)
    AUDIOHW_SETTING_ENT(EQ_BAND2_FREQUENCY, sound_set_hw_eq_band2_frequency)
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2_WIDTH)
    AUDIOHW_SETTING_ENT(EQ_BAND2_WIDTH,     sound_set_hw_eq_band2_width)
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND2 */
#if defined(AUDIOHW_HAVE_EQ_BAND3)
    AUDIOHW_SETTING_ENT(EQ_BAND3_GAIN,      sound_set_hw_eq_band3_gain)
#if defined(AUDIOHW_HAVE_EQ_BAND3_FREQUENCY)
    AUDIOHW_SETTING_ENT(EQ_BAND3_FREQUENCY, sound_set_hw_eq_band3_frequency)
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3_WIDTH)
    AUDIOHW_SETTING_ENT(EQ_BAND3_WIDTH,     sound_set_hw_eq_band3_width)
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND3 */
#if defined(AUDIOHW_HAVE_EQ_BAND4)
    AUDIOHW_SETTING_ENT(EQ_BAND4_GAIN,      sound_set_hw_eq_band4_gain)
#if defined(AUDIOHW_HAVE_EQ_BAND4_FREQUENCY)
    AUDIOHW_SETTING_ENT(EQ_BAND4_FREQUENCY, sound_set_hw_eq_band4_frequency)
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND4_WIDTH)
    AUDIOHW_SETTING_ENT(EQ_BAND4_WIDTH,     sound_set_hw_eq_band4_width)
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND4 */
#if defined(AUDIOHW_HAVE_EQ_BAND5)
    AUDIOHW_SETTING_ENT(EQ_BAND5_GAIN,      sound_set_hw_eq_band5_gain)
#if defined(AUDIOHW_HAVE_EQ_BAND5_FREQUENCY)
    AUDIOHW_SETTING_ENT(EQ_BAND5_FREQUENCY, sound_set_hw_eq_band5_frequency)
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND5 */
#endif /* AUDIOHW_HAVE_EQ */
); /* AUDIOHW_SETTINGS */

#undef AUDIOHW_SETTINGS
#undef AUDIOHW_SETTING_ENT
