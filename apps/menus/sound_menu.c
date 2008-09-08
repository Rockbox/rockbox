
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "sound_menu.h"
#include "eq_menu.h"
#include "exported_menus.h"
#include "menu_common.h"

/***********************************/
/*    SOUND MENU                   */
MENUITEM_SETTING(volume, &global_settings.volume, NULL);
MENUITEM_SETTING(bass, &global_settings.bass,
#ifdef HAVE_SW_TONE_CONTROLS
    lowlatency_callback
#else
    NULL
#endif
);
#ifdef HAVE_WM8758
MENUITEM_SETTING(bass_cutoff, &global_settings.bass_cutoff, NULL);
#endif
MENUITEM_SETTING(treble, &global_settings.treble,
#ifdef HAVE_SW_TONE_CONTROLS
    lowlatency_callback
#else
    NULL
#endif
);
#ifdef HAVE_WM8758
MENUITEM_SETTING(treble_cutoff, &global_settings.treble_cutoff, NULL);
#endif
MENUITEM_SETTING(balance, &global_settings.balance, NULL);
MENUITEM_SETTING(channel_config, &global_settings.channel_config, 
#if CONFIG_CODEC == SWCODEC
    lowlatency_callback
#else
    NULL
#endif
);
MENUITEM_SETTING(stereo_width, &global_settings.stereo_width, 
#if CONFIG_CODEC == SWCODEC
    lowlatency_callback
#else
    NULL
#endif
);

#if CONFIG_CODEC == SWCODEC
    /* Crossfeed Submenu */
    MENUITEM_SETTING(crossfeed, &global_settings.crossfeed, lowlatency_callback);
    MENUITEM_SETTING(crossfeed_direct_gain,
                     &global_settings.crossfeed_direct_gain, lowlatency_callback);
    MENUITEM_SETTING(crossfeed_cross_gain,
                     &global_settings.crossfeed_cross_gain, lowlatency_callback);
    MENUITEM_SETTING(crossfeed_hf_attenuation,
                     &global_settings.crossfeed_hf_attenuation, lowlatency_callback);
    MENUITEM_SETTING(crossfeed_hf_cutoff,
                     &global_settings.crossfeed_hf_cutoff, lowlatency_callback);
    MAKE_MENU(crossfeed_menu,ID2P(LANG_CROSSFEED), NULL, Icon_NOICON,
              &crossfeed, &crossfeed_direct_gain, &crossfeed_cross_gain,
              &crossfeed_hf_attenuation, &crossfeed_hf_cutoff);
              
    MENUITEM_SETTING(dithering_enabled,
                     &global_settings.dithering_enabled, lowlatency_callback);
    MENUITEM_SETTING(sound_speed, &global_settings.sound_speed,
                     lowlatency_callback);
#endif

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    MENUITEM_SETTING(loudness, &global_settings.loudness, NULL);
    MENUITEM_SETTING(avc, &global_settings.avc, NULL);
    MENUITEM_SETTING(superbass, &global_settings.superbass, NULL);
    MENUITEM_SETTING(mdb_enable, &global_settings.mdb_enable, NULL);
    MENUITEM_SETTING(mdb_strength, &global_settings.mdb_strength, NULL);
    MENUITEM_SETTING(mdb_harmonics, &global_settings.mdb_harmonics, NULL);
    MENUITEM_SETTING(mdb_center, &global_settings.mdb_center, NULL);
    MENUITEM_SETTING(mdb_shape, &global_settings.mdb_shape, NULL);
#endif



MAKE_MENU(sound_settings, ID2P(LANG_SOUND_SETTINGS), NULL, Icon_Audio,
          &volume,
          &bass,
#ifdef HAVE_WM8758
          &bass_cutoff,
#endif
          &treble,
#ifdef HAVE_WM8758
          &treble_cutoff,
#endif
          &balance,&channel_config,&stereo_width
#if CONFIG_CODEC == SWCODEC
          ,&crossfeed_menu, &equalizer_menu, &dithering_enabled, &sound_speed
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
         ,&loudness,&avc,&superbass,&mdb_enable,&mdb_strength
         ,&mdb_harmonics,&mdb_center,&mdb_shape
#endif
         );

