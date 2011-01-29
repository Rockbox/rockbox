/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Linus Nielsen Feltzing
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

#include <stdio.h>
#include "config.h"
#include "menu.h"
#include "icon.h"
#include "radio.h"
#include "lang.h"
#include "settings.h"
#include "presets.h"
#include "exported_menus.h"
#include "sound_menu.h" /* recording_menu()   */

#ifdef HAVE_RECORDING
#include "recording.h"  /* recording_screen() */

#if defined(HAVE_FMRADIO_REC) && CONFIG_CODEC == SWCODEC
#define FM_RECORDING_SCREEN
static int fm_recording_screen(void)
{
    bool ret;

    /* switch recording source to FMRADIO for the duration */
    int rec_source = global_settings.rec_source;
    global_settings.rec_source = AUDIO_SRC_FMRADIO;
    ret = recording_screen(true);

    /* safe to reset as changing sources is prohibited here */
    global_settings.rec_source = rec_source;

    return ret;
}

MENUITEM_FUNCTION(recscreen_item, 0, ID2P(LANG_RECORDING),
                    fm_recording_screen, NULL, NULL, Icon_Recording);
#endif /* defined(HAVE_FMRADIO_REC) && CONFIG_CODEC == SWCODEC */

#if defined(HAVE_FMRADIO_REC) || CONFIG_CODEC != SWCODEC
#define FM_RECORDING_SETTINGS
static int fm_recording_settings(void)
{
    bool ret = recording_menu(true);

#if CONFIG_CODEC != SWCODEC
    if (!ret)
    {
        struct audio_recording_options rec_options;
        rec_init_recording_options(&rec_options);
        rec_options.rec_source = AUDIO_SRC_LINEIN;
        rec_set_recording_options(&rec_options);
    }
#endif

    return ret;
}

MENUITEM_FUNCTION(recsettings_item, 0, ID2P(LANG_RECORDING_SETTINGS),
                    fm_recording_settings, NULL, NULL, Icon_Recording);
#endif /* defined(HAVE_FMRADIO_REC) || CONFIG_CODEC != SWCODEC */
#endif /* HAVE_RECORDING */

#ifndef FM_PRESET
MENUITEM_FUNCTION(radio_presets_item, 0, ID2P(LANG_PRESET),
                    handle_radio_presets, NULL, NULL, Icon_NOICON);
#endif
#ifndef FM_PRESET_ADD
MENUITEM_FUNCTION(radio_addpreset_item, 0, ID2P(LANG_FM_ADD_PRESET),
                    handle_radio_add_preset, NULL, NULL, Icon_NOICON);
#endif

MENUITEM_FUNCTION(presetload_item, 0, ID2P(LANG_FM_PRESET_LOAD),
                    preset_list_load, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(presetsave_item, 0, ID2P(LANG_FM_PRESET_SAVE),
                    preset_list_save, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(presetclear_item, 0, ID2P(LANG_FM_PRESET_CLEAR),
                    preset_list_clear, NULL, NULL, Icon_NOICON);

MENUITEM_SETTING(set_region, &global_settings.fm_region, NULL);
MENUITEM_SETTING(force_mono, &global_settings.fm_force_mono, NULL);

#ifndef FM_MODE
extern int radio_mode;
static char* get_mode_text(int selected_item, void * data, char *buffer)
{
    (void)selected_item;
    (void)data;
    snprintf(buffer, MAX_PATH, "%s %s", str(LANG_MODE),
             radio_mode ? str(LANG_PRESET) :
                          str(LANG_RADIO_SCAN_MODE));
    return buffer;
}
static int toggle_radio_mode(void)
{
    radio_mode = (radio_mode == RADIO_SCAN_MODE) ?
                 RADIO_PRESET_MODE : RADIO_SCAN_MODE;
    return 0;
}
MENUITEM_FUNCTION_DYNTEXT(radio_mode_item, 0,
                                 toggle_radio_mode, NULL, 
                                 get_mode_text, NULL, NULL, NULL, Icon_NOICON);
#endif

MENUITEM_FUNCTION(scan_presets_item, MENU_FUNC_USEPARAM,
                    ID2P(LANG_FM_SCAN_PRESETS),
                    presets_scan, NULL, NULL, Icon_NOICON);

MAKE_MENU(radio_settings_menu, ID2P(LANG_FM_MENU), NULL,
            Icon_Radio_screen,
#ifndef FM_PRESET
            &radio_presets_item,
#endif
#ifndef FM_PRESET_ADD
            &radio_addpreset_item,
#endif
            &presetload_item, &presetsave_item, &presetclear_item,
            &force_mono,
#ifndef FM_MODE
            &radio_mode_item,
#endif
            &set_region, &sound_settings,
#ifdef FM_RECORDING_SCREEN
            &recscreen_item,
#endif
#ifdef FM_RECORDING_SETTINGS
            &recsettings_item,
#endif
            &scan_presets_item);

