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
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "settings.h"
#include "status.h"
#include "audio.h"
#include "general.h"
#include "radio.h"
#include "menu.h"
#include "menus/exported_menus.h"
#include "misc.h"
#include "screens.h"
#include "peakmeter.h"
#include "lang.h"
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
#include "iap.h"
#endif
#include "talk.h"
#include "tuner.h"
#include "power.h"
#include "sound.h"
#include "screen_access.h"
#include "splash.h"
#include "yesno.h"
#include "tree.h"
#include "dir.h"
#include "action.h"
#include "viewport.h"
#include "skin_engine/skin_engine.h"
#include "statusbar-skinned.h"
#include "playback.h"
#include "presets.h"
#include "wps.h" /* for wps_state... */

#if CONFIG_TUNER

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define FM_PRESET
#define FM_MODE
#define FM_NEXT_PRESET
#define FM_PREV_PRESET

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD) || (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define FM_PRESET
#define FM_MODE

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define FM_PRESET
#define FM_MODE
/* This should be removeable if the whole tuning thing is sorted out since
   proper tuning quiets the screen almost entirely in that extreme measures
   have to be taken to hear any interference. */
#define HAVE_NOISY_IDLE_MODE

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || (CONFIG_KEYPAD == SANSA_C200_PAD) ||\
      (CONFIG_KEYPAD == SANSA_FUZE_PAD) || (CONFIG_KEYPAD == SANSA_CLIP_PAD) ||\
      (CONFIG_KEYPAD == PHILIPS_HDD1630_PAD)||(CONFIG_KEYPAD == COWON_D2_PAD)||\
      (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)||(CONFIG_KEYPAD == MPIO_HD200_PAD)||\
      (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD) || (CONFIG_KEYPAD == SONY_NWZ_PAD)
#define FM_MENU
#define FM_PRESET
#define FM_STOP
#define FM_MODE
#define FM_EXIT
#define FM_PLAY

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define FM_MENU
#define FM_STOP
#define FM_EXIT
#define FM_PLAY
#define FM_MODE

#elif (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)
#define FM_MENU
#define FM_PRESET
#define FM_STOP
#define FM_MODE
#define FM_EXIT
#define FM_PLAY
#define FM_PREV_PRESET
#define FM_NEXT_PRESET

#elif (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
#define FM_PRESET_ADD
#define FM_PRESET_ACTION
#define FM_MENU
#define FM_PRESET
#define FM_STOP
#define FM_MODE
#define FM_EXIT
#define FM_PLAY
#define FM_RECORD
#define FM_PREV_PRESET
#define FM_NEXT_PRESET

#endif

static int curr_freq; /* current frequency in Hz */
static int radio_mode = RADIO_SCAN_MODE;

static int search_dir = 0;
static int radio_status = FMRADIO_OFF;
static bool in_screen = false;


static void radio_off(void);

enum radio_scan_mode radio_get_mode(void)
{
    return radio_mode;
}

void radio_set_mode(enum radio_scan_mode mode)
{
    radio_mode = mode;
}

bool radio_is_stereo(void)
{
    return tuner_get(RADIO_STEREO) && !global_settings.fm_force_mono;
}

int radio_get_current_frequency(void)
{
    return curr_freq;
}

void radio_set_current_frequency(int freq)
{
    curr_freq = freq;
}

void radio_init(void)
{
    tuner_init();
    radio_off();
#ifdef HAVE_ALBUMART
    radioart_init(false);
#endif
}

int get_radio_status(void)
{
    return radio_status;
}

bool in_radio_screen(void)
{
    return in_screen;
}

/* Keep freq on the grid for the current region */
int snap_freq_to_grid(int freq)
{
    const struct fm_region_data * const fmr =
        &fm_region_data[global_settings.fm_region];

    /* Range clamp if out of range or just round to nearest */
    if (freq < fmr->freq_min)
        freq = fmr->freq_min;
    else if (freq > fmr->freq_max)
        freq = fmr->freq_max;
    else
        freq = (freq - fmr->freq_min + fmr->freq_step/2) /
                fmr->freq_step * fmr->freq_step + fmr->freq_min;

    return freq;
}

/* TODO: Move some more of the control functionality to firmware
         and clean up the mess */

/* secret flag for starting paused - prevents unmute */
#define FMRADIO_START_PAUSED 0x8000
void radio_start(void)
{
    const struct fm_region_data *fmr;
    bool start_paused;

    if(radio_status == FMRADIO_PLAYING)
        return;

    fmr = &fm_region_data[global_settings.fm_region];

    start_paused = radio_status & FMRADIO_START_PAUSED;
    /* clear flag before any yielding */
    radio_status &= ~FMRADIO_START_PAUSED;

    /* ensure the frequency is in range otherwise bad things happen --Bilgus */
    curr_freq = snap_freq_to_grid(global_status.last_frequency * fmr->freq_step + fmr->freq_min);

    tuner_set(RADIO_SLEEP, 0); /* wake up the tuner */

    if(radio_status == FMRADIO_OFF)
    {
#ifdef HAVE_RADIO_REGION
        tuner_set(RADIO_REGION, global_settings.fm_region);
#endif
        tuner_set(RADIO_FORCE_MONO, global_settings.fm_force_mono);
    }

    tuner_set(RADIO_FREQUENCY, curr_freq);

#ifdef HAVE_RADIO_MUTE_TIMEOUT
    {
        unsigned long mute_timeout = current_tick + HZ;
        if (radio_status != FMRADIO_OFF)
        {
            /* paused */
            mute_timeout += HZ;
        }

        while(!tuner_get(RADIO_STEREO) && !tuner_get(RADIO_TUNED))
        {
            if(TIME_AFTER(current_tick, mute_timeout))
                 break;
            yield();
        }
    }
#endif

    /* keep radio from sounding initially */
    if(!start_paused)
        tuner_set(RADIO_MUTE, 0);

    radio_status = FMRADIO_PLAYING;
} /* radio_start */

void radio_pause(void)
{
    if(radio_status == FMRADIO_PAUSED)
        return;

    if(radio_status == FMRADIO_OFF)
    {
        radio_status |= FMRADIO_START_PAUSED;
        radio_start();
    }

    tuner_set(RADIO_MUTE, 1);

    radio_status = FMRADIO_PAUSED;
} /* radio_pause */

static void radio_off(void)
{
    tuner_set(RADIO_MUTE, 1);
    tuner_set(RADIO_SLEEP, 1); /* low power mode, if available */
    radio_status = FMRADIO_OFF;
}

void radio_stop(void)
{
    if(radio_status == FMRADIO_OFF)
        return;

    radio_off();
} /* radio_stop */

bool radio_hardware_present(void)
{
    return tuner_get(RADIO_PRESENT);
}

void remember_frequency(void)
{
    const struct fm_region_data * const fmr =
        &fm_region_data[global_settings.fm_region];
    global_status.last_frequency = (curr_freq - fmr->freq_min) 
                                   / fmr->freq_step;
    status_save(true);
}

/* Step to the next or previous frequency */
static int step_freq(int freq, int direction)
{
    const struct fm_region_data * const fmr =
        &fm_region_data[global_settings.fm_region];

    freq += direction*fmr->freq_step;

    /* Wrap first or snapping to grid will not let us on the band extremes */
    if (freq > fmr->freq_max)
        freq = direction > 0 ? fmr->freq_min : fmr->freq_max;
    else if (freq < fmr->freq_min)
        freq = direction < 0 ? fmr->freq_max : fmr->freq_min;
    else
        freq = snap_freq_to_grid(freq);

    return freq;
}

/* Step to the next or previous station */
void next_station(int direction)
{
    if (direction != 0 && radio_mode != RADIO_SCAN_MODE)
    {
        preset_next(direction);
        return;
    }

    curr_freq = step_freq(curr_freq, direction);

    if (radio_status == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 1);

    if(radio_status != FMRADIO_OFF)
        tuner_set(RADIO_FREQUENCY, curr_freq);

    if (radio_status == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 0);

    preset_set_current(preset_find(curr_freq));
    remember_frequency();
}

/* Ends an in-progress search */
static void end_search(void)
{
    if (search_dir != 0 && radio_status == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 0);
    search_dir = 0;
}

void radio_screen(void)
{
    bool done = false;
    int button;
    bool stereo = false, last_stereo = false;
    int update_type = 0;
    bool screen_freeze = false;
    bool keep_playing = false;
    bool talk = false;
#ifdef FM_RECORD_DBLPRE
    int lastbutton = BUTTON_NONE;
    unsigned long rec_lastclick = 0;
#endif
#ifndef HAVE_NOISY_IDLE_MODE
    int button_timeout = current_tick + (2*HZ);
#endif

    /* change status to "in screen" */
    push_current_activity(ACTIVITY_FM);
    in_screen = true;

    if(radio_preset_count() <= 0)
    {
        radio_load_presets(global_settings.fmr_file);
    }
    /* TODO: Can this be moved somewhere else? */
    get_wps_state()->id3 = NULL;
    get_wps_state()->nid3 = NULL;
#ifdef HAVE_ALBUMART
    radioart_init(true);
#endif    

    if(radio_status == FMRADIO_OFF)
        audio_stop();

    fms_fix_displays(FMS_ENTER);

    /* turn on radio */
    /* This should be done before touching audio settings */
    while (!pcm_is_initialized())
       sleep(0);

    audio_set_input_source(AUDIO_SRC_FMRADIO,
                           (radio_status == FMRADIO_PAUSED) ?
                               SRCF_FMRADIO_PAUSED : SRCF_FMRADIO_PLAYING);

    if(radio_preset_count() < 1 && yesno_pop(ID2P(LANG_FM_FIRST_AUTOSCAN)))
        presets_scan(NULL);

    preset_set_current(preset_find(curr_freq));
    if(radio_current_preset() != -1)
         radio_mode = RADIO_PRESET_MODE;

    /* Load/update the skin at last, when fully initialzed, so that it can
     * display the right content from the beginning */
    FOR_NB_SCREENS(i)
        skin_update(FM_SCREEN, i, SKIN_REFRESH_ALL);

#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(true);
#endif

    while(!done)
    {
        if(search_dir != 0)
        {
            curr_freq = step_freq(curr_freq, search_dir);
            update_type = SKIN_REFRESH_ALL;

            if(tuner_set(RADIO_SCAN_FREQUENCY, curr_freq))
            {
                preset_set_current(preset_find(curr_freq));
                remember_frequency();
                end_search();
                talk = true;
            }
            trigger_cpu_boost();
        }

        if (!update_type)
        {
            cancel_cpu_boost();
        }

        button = fms_do_button_loop(update_type>0);

#ifndef HAVE_NOISY_IDLE_MODE
        if (button != ACTION_NONE)
        {
            cpu_idle_mode(false);
            button_timeout = current_tick + (2*HZ);
        }
#endif
        switch(button)
        {
             case ACTION_FM_STOP:
                {
                    done = true;
                    if(presets_have_changed())
                    {
                        if(yesno_pop(ID2P(LANG_SAVE_CHANGES)))
                        {
                            presets_save();
                        }
                    }
                }
                update_type = SKIN_REFRESH_NON_STATIC;
                break;

#ifdef FM_RECORD
            case ACTION_FM_RECORD:
#ifdef FM_RECORD_DBLPRE
                if (lastbutton != ACTION_FM_RECORD_DBLPRE)
                {
                    rec_lastclick = 0;
                    break;
                }
                if (current_tick - rec_lastclick > HZ/2)
                {
                    rec_lastclick = current_tick;
                    break;
                }
#endif /* FM_RECORD_DBLPRE */
#ifndef SIMULATOR
                if(audio_status() == AUDIO_STATUS_RECORD)
                {
                    rec_command(RECORDING_CMD_START_NEWFILE);
                    update_type = SKIN_REFRESH_ALL;
                }
                else
                {
                    rec_command(RECORDING_CMD_START);
                    update_type = SKIN_REFRESH_ALL;
                }
#endif /* SIMULATOR */
                break;
#endif /* #ifdef FM_RECORD */

            case ACTION_FM_EXIT:
                keep_playing = true;
                done = true;
                if(presets_have_changed())
                {
                    if(yesno_pop(ID2P(LANG_SAVE_CHANGES)))
                    {
                        presets_save();
                    }
                }

                break;

            case ACTION_STD_PREV:
            case ACTION_STD_NEXT:
                next_station(button == ACTION_STD_PREV ? -1 : 1);
                end_search();
                update_type = SKIN_REFRESH_ALL;
                talk = true;
                break;

            case ACTION_STD_PREVREPEAT:
            case ACTION_STD_NEXTREPEAT:
            {
                int dir = search_dir;
                search_dir = button == ACTION_STD_PREVREPEAT ? -1 : 1;
                if (radio_mode != RADIO_SCAN_MODE)
                {
                    preset_next(search_dir);
                    end_search();
                    talk = true;
                }
                else if (dir == 0)
                {
                    /* Starting auto scan */
                    tuner_set(RADIO_MUTE, 1);
                }
                update_type = SKIN_REFRESH_ALL;
                break;
            }

            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_INCREPEAT:
                global_status.volume += sound_steps(SOUND_VOLUME);
                setvol();
                update_type = SKIN_REFRESH_NON_STATIC;
                break;

            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                global_status.volume -= sound_steps(SOUND_VOLUME);
                setvol();
                update_type = SKIN_REFRESH_NON_STATIC;
                break;

            case ACTION_FM_PLAY:
                if (radio_status == FMRADIO_PLAYING)
                    radio_pause();
                else
                    radio_start();

                update_type = SKIN_REFRESH_NON_STATIC;
                talk = true;
                talk_shutup();
                break;

            case ACTION_FM_MENU:
                fms_fix_displays(FMS_EXIT);
                do_menu(&radio_settings_menu, NULL, NULL, false);
                preset_set_current(preset_find(curr_freq));
                fms_fix_displays(FMS_ENTER);
                update_type = SKIN_REFRESH_ALL;
                break;

#ifdef FM_PRESET
            case ACTION_FM_PRESET:
                if(radio_preset_count() < 1)
                {
                    splash(HZ, ID2P(LANG_FM_NO_PRESETS));
                    update_type = SKIN_REFRESH_ALL;
                    break;
                }
                fms_fix_displays(FMS_EXIT);
                handle_radio_presets();
                fms_fix_displays(FMS_ENTER);
                update_type = SKIN_REFRESH_ALL;
                break;
#endif /* FM_PRESET */

#ifdef FM_FREEZE
            case ACTION_FM_FREEZE:
                if(!screen_freeze)
                {
                    splash(HZ, str(LANG_FM_FREEZE));
                    screen_freeze = true;
                }
                else
                {
                    update_type = SKIN_REFRESH_ALL;
                    screen_freeze = false;
                }
                break;
#endif /* FM_FREEZE */

            case SYS_USB_CONNECTED:
                {
                    default_event_handler(SYS_USB_CONNECTED);
                    screen_freeze = true; /* Cosmetic: makes sure the
                                             radio screen doesn't redraw */
                    done = true;
                }
                break;

#ifdef FM_MODE
           case ACTION_FM_MODE:
                if(radio_mode == RADIO_SCAN_MODE)
                {
                    /* Force scan mode if there are no presets. */
                    if(radio_preset_count() > 0)
                        radio_mode = RADIO_PRESET_MODE;
                }
                else
                    radio_mode = RADIO_SCAN_MODE;
                update_type = SKIN_REFRESH_ALL;
                cond_talk_ids_fq(radio_mode ?
                                 LANG_PRESET : LANG_RADIO_SCAN_MODE);
                talk = true;
                break;
#endif /* FM_MODE */

#ifdef FM_NEXT_PRESET
            case ACTION_FM_NEXT_PRESET:
                preset_next(1);
                end_search();
                update_type = SKIN_REFRESH_ALL;
                talk = true;
                break;
#endif

#ifdef FM_PREV_PRESET
            case ACTION_FM_PREV_PRESET:
                preset_next(-1);
                end_search();
                update_type = SKIN_REFRESH_ALL;
                talk = true;
                break;
#endif
            case ACTION_NONE:
                update_type = SKIN_REFRESH_NON_STATIC;
                break;
             /* this case is used by the softlock feature
              * it requests a full update here */
            case ACTION_REDRAW:
                skin_request_full_update(FM_SCREEN);
                skin_request_full_update(CUSTOM_STATUSBAR); /* if SBS is used */
                break;

            default:
                default_event_handler(button);
                if (!tuner_get(RADIO_PRESENT))
                {
                    keep_playing = false;
                    done = true;
                    if(presets_have_changed())
                    {
                        if(yesno_pop(ID2P(LANG_SAVE_CHANGES)))
                        {
                            radio_save_presets();
                        }
                    }

                    /* Clear the preset list on exit. */
                    preset_list_clear();
                }
                break;
        } /*switch(button)*/

#ifdef FM_RECORD_DBLPRE
        if (button != ACTION_NONE)
            lastbutton = button;
#endif

        if(!screen_freeze)
        {
            /* Only display the peak meter when not recording */
            {
                /* keep "mono" from always being displayed when paused */
                if (radio_status != FMRADIO_PAUSED)
                {
                    stereo = tuner_get(RADIO_STEREO) &&
                        !global_settings.fm_force_mono;

                    if(stereo != last_stereo)
                    {
                        update_type = SKIN_REFRESH_ALL;
                        last_stereo = stereo;
                    }
                }
            }

            if (update_type)
            {
                FOR_NB_SCREENS(i)
                    skin_update(FM_SCREEN, i, update_type);
                if (update_type == (int)SKIN_REFRESH_ALL)
                    skin_request_full_update(CUSTOM_STATUSBAR);
            }
        }
        update_type = 0;

        if (global_settings.talk_file && talk
            && radio_status == FMRADIO_PAUSED)
        {
            talk = false;
            bool enqueue = false;
            if (radio_mode == RADIO_SCAN_MODE)
            {
                talk_value_decimal(curr_freq, UNIT_INT, 6, enqueue);
                enqueue = true;
            }
            if (radio_current_preset() >= 0)
                preset_talk(radio_current_preset(), radio_mode == RADIO_PRESET_MODE,
                            enqueue);
        }

#ifndef HAVE_NOISY_IDLE_MODE
        if (TIME_AFTER(current_tick, button_timeout))
        {
            cpu_idle_mode(true);
        }
#endif
    } /*while(!done)*/

#ifndef SIMULATOR
    sound_settings_apply();
#endif /* SIMULATOR */

    if(keep_playing)
    {
/* Catch FMRADIO_PLAYING status for the sim. */
#ifndef SIMULATOR
        end_search();
#endif /* SIMULATOR */
    }
    else
    {
        audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    }

#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(false);
#endif
    fms_fix_displays(FMS_EXIT);
    pop_current_activity();
    in_screen = false;
} /* radio_screen */

void toggle_mono_mode(bool mono)
{
    if(radio_status != FMRADIO_OFF)
        tuner_set(RADIO_FORCE_MONO, mono);
}

void set_radio_region(int region)
{
#ifdef HAVE_RADIO_REGION
    if(radio_status != FMRADIO_OFF)
        tuner_set(RADIO_REGION, region);
#else
    (void)region;
#endif
    next_station(0);
}

#endif
