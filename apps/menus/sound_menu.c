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
#include "sound.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "sound_menu.h"
#include "eq_menu.h"
#include "exported_menus.h"
#include "menu_common.h"
#include "splash.h"
#include "kernel.h"
#include "talk.h"
#include "option_select.h"
#include "misc.h"

static int32_t get_dec_talkid(int value, int unit)
{
    return TALK_ID_DECIMAL(value, 1, unit);
}

static int volume_limit_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;

    static struct int_setting volume_limit_int_setting;
    volume_limit_int_setting.option_callback = NULL;
    volume_limit_int_setting.unit = UNIT_DB;
    volume_limit_int_setting.min = sound_min(SOUND_VOLUME);
    volume_limit_int_setting.max = sound_max(SOUND_VOLUME);
    volume_limit_int_setting.step = sound_steps(SOUND_VOLUME);
    volume_limit_int_setting.formatter = NULL;
    volume_limit_int_setting.get_talk_id = get_dec_talkid;

    struct settings_list setting;
    setting.flags = F_BANFROMQS|F_INT_SETTING|F_T_INT|F_NO_WRAP;
    setting.lang_id = LANG_VOLUME_LIMIT;
    setting.default_val.int_ = sound_max(SOUND_VOLUME);
    setting.int_setting = &volume_limit_int_setting;

    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            setting.setting = &global_settings.volume_limit;
            option_screen(&setting, NULL, false, ID2P(LANG_VOLUME_LIMIT));
        case ACTION_EXIT_MENUITEM: /* on exit */
            setvol();
            break;
    }
    return action;
}

/***********************************/
/*    SOUND MENU                   */
MENUITEM_SETTING(volume, &global_settings.volume, NULL);
MENUITEM_SETTING(volume_limit, &global_settings.volume_limit, volume_limit_callback);
#ifdef AUDIOHW_HAVE_BASS
MENUITEM_SETTING(bass, &global_settings.bass,
#ifdef HAVE_SW_TONE_CONTROLS
    lowlatency_callback
#else
    NULL
#endif
);

#ifdef AUDIOHW_HAVE_BASS_CUTOFF
MENUITEM_SETTING(bass_cutoff, &global_settings.bass_cutoff, NULL);
#endif
#endif /* AUDIOHW_HAVE_BASS */


#ifdef AUDIOHW_HAVE_TREBLE
MENUITEM_SETTING(treble, &global_settings.treble,
#ifdef HAVE_SW_TONE_CONTROLS
    lowlatency_callback
#else
    NULL
#endif
);

#ifdef AUDIOHW_HAVE_TREBLE_CUTOFF
MENUITEM_SETTING(treble_cutoff, &global_settings.treble_cutoff, NULL);
#endif
#endif /* AUDIOHW_HAVE_TREBLE */


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

#ifdef AUDIOHW_HAVE_DEPTH_3D
MENUITEM_SETTING(depth_3d, &global_settings.depth_3d, NULL);
#endif

#ifdef AUDIOHW_HAVE_FILTER_ROLL_OFF
MENUITEM_SETTING(roll_off, &global_settings.roll_off, NULL);
#endif

#ifdef AUDIOHW_HAVE_FUNCTIONAL_MODE
MENUITEM_SETTING(func_mode, &global_settings.func_mode, NULL);
#endif

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

#ifdef HAVE_PITCHCONTROL
static int timestretch_callback(int action,const struct menu_item_ex *this_item)
{
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            if (global_settings.timestretch_enabled && !dsp_timestretch_available())
                splash(HZ*2, ID2P(LANG_PLEASE_REBOOT));
            break;
    }
    lowlatency_callback(action, this_item);
    return action;
}
    MENUITEM_SETTING(timestretch_enabled,
                     &global_settings.timestretch_enabled, timestretch_callback);
#endif

    MENUITEM_SETTING(dithering_enabled,
                     &global_settings.dithering_enabled, lowlatency_callback);
    MENUITEM_SETTING(afr_enabled,
                     &global_settings.afr_enabled, lowlatency_callback);
    MENUITEM_SETTING(pbe,
                     &global_settings.pbe, lowlatency_callback);
    MENUITEM_SETTING(pbe_precut,
                     &global_settings.pbe_precut, lowlatency_callback);
    MAKE_MENU(pbe_menu,ID2P(LANG_PBE), NULL, Icon_NOICON,
              &pbe,&pbe_precut);
    MENUITEM_SETTING(surround_enabled,
                     &global_settings.surround_enabled, lowlatency_callback);
    MENUITEM_SETTING(surround_balance,
                     &global_settings.surround_balance, lowlatency_callback);
    MENUITEM_SETTING(surround_fx1,
                     &global_settings.surround_fx1, lowlatency_callback);
    MENUITEM_SETTING(surround_fx2,
                     &global_settings.surround_fx2, lowlatency_callback);
    MENUITEM_SETTING(surround_method2,
                     &global_settings.surround_method2, lowlatency_callback);
    MENUITEM_SETTING(surround_mix,
                     &global_settings.surround_mix, lowlatency_callback);
    MAKE_MENU(surround_menu,ID2P(LANG_SURROUND), NULL, Icon_NOICON,
              &surround_enabled,&surround_balance,&surround_fx1,&surround_fx2,&surround_method2,&surround_mix);

    /* compressor submenu */
    MENUITEM_SETTING(compressor_threshold,
                     &global_settings.compressor_settings.threshold,
                     lowlatency_callback);
    MENUITEM_SETTING(compressor_gain,
                     &global_settings.compressor_settings.makeup_gain,
                     lowlatency_callback);
    MENUITEM_SETTING(compressor_ratio,
                     &global_settings.compressor_settings.ratio,
                     lowlatency_callback);
    MENUITEM_SETTING(compressor_knee,
                     &global_settings.compressor_settings.knee,
                     lowlatency_callback);
    MENUITEM_SETTING(compressor_attack,
                     &global_settings.compressor_settings.attack_time,
                     lowlatency_callback);
    MENUITEM_SETTING(compressor_release,
                     &global_settings.compressor_settings.release_time,
                     lowlatency_callback);
    MAKE_MENU(compressor_menu,ID2P(LANG_COMPRESSOR), NULL, Icon_NOICON,
              &compressor_threshold, &compressor_gain, &compressor_ratio,
              &compressor_knee, &compressor_attack, &compressor_release);
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

#ifdef HAVE_SPEAKER
    MENUITEM_SETTING(speaker_mode, &global_settings.speaker_mode, NULL);
#endif

#ifdef AUDIOHW_HAVE_EQ
#endif /* AUDIOHW_HAVE_EQ */

MAKE_MENU(sound_settings, ID2P(LANG_SOUND_SETTINGS), NULL, Icon_Audio,
          &volume
          ,&volume_limit
#ifdef AUDIOHW_HAVE_BASS
          ,&bass
#endif
#ifdef AUDIOHW_HAVE_BASS_CUTOFF
          ,&bass_cutoff
#endif
#ifdef AUDIOHW_HAVE_TREBLE
          ,&treble
#endif
#ifdef AUDIOHW_HAVE_TREBLE_CUTOFF
          ,&treble_cutoff
#endif
#ifdef AUDIOHW_HAVE_EQ
          ,&audiohw_eq_tone_controls
#endif
          ,&balance,&channel_config,&stereo_width
#ifdef AUDIOHW_HAVE_DEPTH_3D
          ,&depth_3d
#endif
#ifdef AUDIOHW_HAVE_FILTER_ROLL_OFF
          ,&roll_off
#endif
#ifdef AUDIOHW_HAVE_FUNCTIONAL_MODE
          ,&func_mode
#endif
#if CONFIG_CODEC == SWCODEC
          ,&crossfeed_menu, &equalizer_menu, &dithering_enabled
          ,&surround_menu, &pbe_menu, &afr_enabled
#ifdef HAVE_PITCHCONTROL
          ,&timestretch_enabled
#endif
          ,&compressor_menu
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
         ,&loudness,&avc,&superbass,&mdb_enable,&mdb_strength
         ,&mdb_harmonics,&mdb_center,&mdb_shape
#endif
#ifdef HAVE_SPEAKER
         ,&speaker_mode
#endif
         );

