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

#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "mas.h"
#include "settings.h"
#include "button.h"
#include "status.h"
#include "thread.h"
#include "audio.h"
#include "mp3_playback.h"
#include "ctype.h"
#include "file.h"
#include "general.h"
#include "errno.h"
#include "string-extra.h"
#include "system.h"
#include "radio.h"
#include "menu.h"
#include "misc.h"
#include "keyboard.h"
#include "screens.h"
#include "peakmeter.h"
#include "lang.h"
#include "font.h"
#include "sound_menu.h"
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
#include "iap.h"
#endif
#include "appevents.h"
#include "talk.h"
#include "tuner.h"
#include "power.h"
#include "sound.h"
#include "screen_access.h"
#include "splash.h"
#include "yesno.h"
#include "buttonbar.h"
#include "tree.h"
#include "dir.h"
#include "action.h"
#include "list.h"
#include "menus/exported_menus.h"
#include "root_menu.h"
#include "viewport.h"
#include "skin_engine/skin_engine.h"
#include "statusbar-skinned.h"
#include "buffering.h"
#if CONFIG_CODEC == SWCODEC
#include "playback.h"
#endif

#if CONFIG_TUNER

#if CONFIG_KEYPAD == RECORDER_PAD
#define FM_RECORD
#define FM_PRESET_ADD
#define FM_PRESET_ACTION
#define FM_PRESET
#define FM_MODE

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define FM_PRESET
#define FM_MODE
#define FM_NEXT_PRESET
#define FM_PREV_PRESET

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define FM_PRESET
#define FM_MODE

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define FM_PRESET
#define FM_MODE
/* This should be removeable if the whole tuning thing is sorted out since
   proper tuning quiets the screen almost entirely in that extreme measures
   have to be taken to hear any interference. */
#define HAVE_NOISY_IDLE_MODE

#elif CONFIG_KEYPAD == ONDIO_PAD
#define FM_RECORD_DBLPRE
#define FM_RECORD

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || (CONFIG_KEYPAD == SANSA_C200_PAD) ||\
      (CONFIG_KEYPAD == SANSA_FUZE_PAD) || (CONFIG_KEYPAD == SANSA_CLIP_PAD)
#define FM_MENU
#define FM_PRESET
#define FM_STOP
#define FM_MODE
#define FM_EXIT
#define FM_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define FM_PRESET
#define FM_MODE

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
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

#endif

/* presets.c needs these so keep unstatic or redo the whole thing! */
int curr_freq; /* current frequency in Hz */
/* these are all in presets.c... someone PLEASE rework this ! */
int handle_radio_presets(void);
static bool radio_menu(void);
int radio_add_preset(void);
int save_preset_list(void);
int load_preset_list(void);
int clear_preset_list(void);
void next_preset(int direction);
void set_current_preset(int preset);
int scan_presets(void *viewports);
int find_preset(int freq);
void radio_save_presets(void);
bool has_presets_changed(void);
void talk_preset(int preset, bool fallback, bool enqueue);
void presets_save(void);



int radio_mode = RADIO_SCAN_MODE;
static int search_dir = 0;

static int radio_status = FMRADIO_OFF;
static bool in_screen = false;


static void radio_off(void);

bool radio_scan_mode(void)
{
    return radio_mode == RADIO_SCAN_MODE;
}

bool radio_is_stereo(void)
{
    return tuner_get(RADIO_STEREO) && !global_settings.fm_force_mono;
}
int radio_current_frequency(void)
{
    return curr_freq;
}

/* Function to manipulate all yesno dialogues.
   This function needs the output text as an argument. */
bool yesno_pop(const char* text)
{
    int i;
    const char *lines[]={text};
    const struct text_message message={lines, 1};
    bool ret = (gui_syncyesno_run(&message,NULL,NULL)== YESNO_YES);
    FOR_NB_SCREENS(i)
        screens[i].clear_viewport();
    return ret;
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

    if(radio_status == FMRADIO_OFF)
        tuner_power(true);

    curr_freq = global_status.last_frequency * fmr->freq_step + fmr->freq_min;

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
    /* For si4700: 2==this is really 'pause'. other tuners treat it
     * like 'bool'. */
    tuner_set(RADIO_SLEEP, 2);

    radio_status = FMRADIO_PAUSED;
} /* radio_pause */

static void radio_off(void)
{
    tuner_set(RADIO_MUTE, 1);
    tuner_set(RADIO_SLEEP, 1); /* low power mode, if available */
    radio_status = FMRADIO_OFF;
    tuner_power(false); /* status update, power off if avail. */
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

void remember_frequency(void)
{
    const struct fm_region_data * const fmr =
        &fm_region_data[global_settings.fm_region];
    global_status.last_frequency = (curr_freq - fmr->freq_min) 
                                   / fmr->freq_step;
    status_save();
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
        next_preset(direction);
        return;
    }

    curr_freq = step_freq(curr_freq, direction);

    if (radio_status == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 1);

    tuner_set(RADIO_FREQUENCY, curr_freq);

    if (radio_status == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 0);

    set_current_preset(find_preset(curr_freq));
    remember_frequency();
}

/* Ends an in-progress search */
static void end_search(void)
{
    if (search_dir != 0 && radio_status == FMRADIO_PLAYING)
        tuner_set(RADIO_MUTE, 0);
    search_dir = 0;
}

/* Speak a frequency. */
void talk_freq(int freq, bool enqueue)
{
    freq /= 10000;
    talk_number(freq / 100, enqueue);
    talk_id(LANG_POINT, true);
    talk_number(freq % 100 / 10, true);
    if (freq % 10)
        talk_number(freq % 10, true);
}


int radio_screen(void)
{
    bool done = false;
    int ret_val = GO_TO_ROOT;
    int button;
    int i;
    bool stereo = false, last_stereo = false;
    bool update_screen = true, restore = true;
    bool screen_freeze = false;
    bool keep_playing = false;
    bool talk = false;
#ifdef FM_RECORD_DBLPRE
    int lastbutton = BUTTON_NONE;
    unsigned long rec_lastclick = 0;
#endif
#if CONFIG_CODEC != SWCODEC
    bool have_recorded = false;
    int timeout = current_tick + HZ/10;
    unsigned int last_seconds = 0;
#if !defined(SIMULATOR)
    unsigned int seconds = 0;
    struct audio_recording_options rec_options;
#endif /* SIMULATOR */
#endif /* CONFIG_CODEC != SWCODEC */
#ifndef HAVE_NOISY_IDLE_MODE
    int button_timeout = current_tick + (2*HZ);
#endif

    /* change status to "in screen" */
    in_screen = true;

    if(radio_preset_count() <= 0)
    {
        radio_load_presets(global_settings.fmr_file);
    }
#ifdef HAVE_ALBUMART
    radioart_init(true);
#endif    

    if(radio_status == FMRADIO_OFF)
        audio_stop();
    
#ifndef SIMULATOR

#if CONFIG_CODEC != SWCODEC
    if(rec_create_directory() > 0)
        have_recorded = true;

    audio_init_recording(talk_get_bufsize());

    sound_settings_apply();
    /* Yes, we use the D/A for monitoring */
    peak_meter_playback(true);

    peak_meter_enable(true);

    rec_init_recording_options(&rec_options);
    rec_options.rec_source = AUDIO_SRC_LINEIN;
    rec_set_recording_options(&rec_options);

    audio_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
            sound_default(SOUND_RIGHT_GAIN), AUDIO_GAIN_LINEIN);

#endif /* CONFIG_CODEC != SWCODEC */
#endif /* ndef SIMULATOR */

    /* turn on radio */
#if CONFIG_CODEC == SWCODEC
    /* This should be done before touching audio settings */
    while (!audio_is_thread_ready())
       sleep(0);

    audio_set_input_source(AUDIO_SRC_FMRADIO,
                           (radio_status == FMRADIO_PAUSED) ?
                               SRCF_FMRADIO_PAUSED : SRCF_FMRADIO_PLAYING);
#else
    if (radio_status == FMRADIO_OFF)
        radio_start();
#endif

   if(radio_preset_count() < 1 && yesno_pop(ID2P(LANG_FM_FIRST_AUTOSCAN)))
        scan_presets(NULL);

    set_current_preset(find_preset(curr_freq));
    if(radio_current_preset() != -1)
         radio_mode = RADIO_PRESET_MODE;

#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(true);
#endif

    while(!done)
    {
        if(search_dir != 0)
        {
            curr_freq = step_freq(curr_freq, search_dir);
            update_screen = true;

            if(tuner_set(RADIO_SCAN_FREQUENCY, curr_freq))
            {
                set_current_preset(find_preset(curr_freq));
                remember_frequency();
                end_search();
                talk = true;
            }
            trigger_cpu_boost();
        }

        if (!update_screen)
        {
            cancel_cpu_boost();
        }

        button = fms_do_button_loop(update_screen);

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
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
                if(audio_status() == AUDIO_STATUS_RECORD)
                {
                    audio_stop();
                }
                else
#endif
                {
                    done = true;
                    if(has_presets_changed())
                    {
                        if(yesno_pop(ID2P(LANG_FM_SAVE_CHANGES)))
                        {
                            presets_save();
                        }
                    }
                }
                update_screen = true;
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
                    update_screen = true;
                }
                else
                {
                    have_recorded = true;
                    rec_command(RECORDING_CMD_START);
                    update_screen = true;
                }
#endif /* SIMULATOR */
                last_seconds = 0;
                break;
#endif /* #ifdef FM_RECORD */

            case ACTION_FM_EXIT:
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
                if(audio_status() == AUDIO_STATUS_RECORD)
                    audio_stop();
#endif
                keep_playing = true;
                done = true;
                ret_val = GO_TO_ROOT;
                if(has_presets_changed())
                {
                    if(yesno_pop(ID2P(LANG_FM_SAVE_CHANGES)))
                    {
                        presets_save();
                    }
                }

                break;

            case ACTION_STD_PREV:
            case ACTION_STD_NEXT:
                next_station(button == ACTION_STD_PREV ? -1 : 1);
                end_search();
                update_screen = true;
                talk = true;
                break;

            case ACTION_STD_PREVREPEAT:
            case ACTION_STD_NEXTREPEAT:
            {
                int dir = search_dir;
                search_dir = button == ACTION_STD_PREVREPEAT ? -1 : 1;
                if (radio_mode != RADIO_SCAN_MODE)
                {
                    next_preset(search_dir);
                    end_search();
                    update_screen = true;
                    talk = true;
                }
                else if (dir == 0)
                {
                    /* Starting auto scan */
                    tuner_set(RADIO_MUTE, 1);
                    update_screen = true;
                }
                break;
                }

            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_INCREPEAT:
                global_settings.volume++;
                setvol();
                update_screen = true;
                break;

            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                global_settings.volume--;
                setvol();
                update_screen = true;
                break;

            case ACTION_FM_PLAY:
                if (radio_status == FMRADIO_PLAYING)
                    radio_pause();
                else
                    radio_start();

                update_screen = true;
                talk = false;
                talk_shutup();
                break;

            case ACTION_FM_MENU:
                fms_fix_displays(FMS_EXIT);
                radio_menu();
                set_current_preset(find_preset(curr_freq));
                update_screen = true;
                restore = true;
                break;

#ifdef FM_PRESET
            case ACTION_FM_PRESET:
                if(radio_preset_count() < 1)
                {
                    splash(HZ, ID2P(LANG_FM_NO_PRESETS));
                    update_screen = true;
                    break;
                }
                fms_fix_displays(FMS_EXIT);
                handle_radio_presets();
                update_screen = true;
                restore = true;
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
                    update_screen = true;
                    screen_freeze = false;
                }
                break;
#endif /* FM_FREEZE */

            case SYS_USB_CONNECTED:
#if CONFIG_CODEC != SWCODEC
                /* Only accept USB connection when not recording */
                if(audio_status() != AUDIO_STATUS_RECORD)
#endif
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
                update_screen = true;
                cond_talk_ids_fq(radio_mode ?
                                 LANG_PRESET : LANG_RADIO_SCAN_MODE);
                talk = true;
                break;
#endif /* FM_MODE */

#ifdef FM_NEXT_PRESET
            case ACTION_FM_NEXT_PRESET:
                next_preset(1);
                end_search();
                update_screen = true;
                talk = true;
                break;
#endif

#ifdef FM_PREV_PRESET
            case ACTION_FM_PREV_PRESET:
                next_preset(-1);
                end_search();
                update_screen = true;
                talk = true;
                break;
#endif

            default:
                default_event_handler(button);
#ifdef HAVE_RDS_CAP
                if (tuner_get(RADIO_EVENT))
                    update_screen = true;
#endif
                if (!tuner_get(RADIO_PRESENT))
                {
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
                    if(audio_status() == AUDIO_STATUS_RECORD)
                        audio_stop();
#endif
                    keep_playing = false;
                    done = true;
                    ret_val = GO_TO_ROOT;
                    if(has_presets_changed())
                    {
                        if(yesno_pop(ID2P(LANG_FM_SAVE_CHANGES)))
                        {
                            radio_save_presets();
                        }
                    }

                    /* Clear the preset list on exit. */
                    clear_preset_list();
                }
                break;
        } /*switch(button)*/

#ifdef FM_RECORD_DBLPRE
        if (button != ACTION_NONE)
            lastbutton = button;
#endif

#if CONFIG_CODEC != SWCODEC
        peak_meter_peek();
#endif

        if(!screen_freeze)
        {
            /* Only display the peak meter when not recording */
#if CONFIG_CODEC != SWCODEC
            if(TIME_AFTER(current_tick, timeout))
            {
                timeout = current_tick + HZ;
#else  /* SWCODEC */
            {
#endif /* CONFIG_CODEC == SWCODEC */

                /* keep "mono" from always being displayed when paused */
                if (radio_status != FMRADIO_PAUSED)
                {
                    stereo = tuner_get(RADIO_STEREO) &&
                        !global_settings.fm_force_mono;

                    if(stereo != last_stereo)
                    {
                        update_screen = true;
                        last_stereo = stereo;
                    }
                }
            }

#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
            seconds = audio_recorded_time() / HZ;
            if (update_screen || seconds > last_seconds || restore)
            {
                last_seconds = seconds;
#else
            if (update_screen || restore)
            {
#endif
                if (restore)
                    fms_fix_displays(FMS_ENTER);
                FOR_NB_SCREENS(i)
                    skin_update(fms_get(i), WPS_REFRESH_ALL);
                restore = false; 
            }
        }
        update_screen = false;

        if (global_settings.talk_file && talk
            && radio_status == FMRADIO_PAUSED)
        {
            talk = false;
            bool enqueue = false;
            if (radio_mode == RADIO_SCAN_MODE)
            {
                talk_freq(curr_freq, enqueue);
                enqueue = true;
            }
            if (radio_current_preset() >= 0)
                talk_preset(radio_current_preset(), radio_mode == RADIO_PRESET_MODE,
                            enqueue);
        }

#if CONFIG_CODEC != SWCODEC
        if(audio_status() & AUDIO_STATUS_ERROR)
        {
            done = true;
        }
#endif

#ifndef HAVE_NOISY_IDLE_MODE
        if (TIME_AFTER(current_tick, button_timeout))
        {
            cpu_idle_mode(true);
        }
#endif
    } /*while(!done)*/

#ifndef SIMULATOR
#if CONFIG_CODEC != SWCODEC
    if(audio_status() & AUDIO_STATUS_ERROR)
    {
        splash(0, str(LANG_DISK_FULL));
        audio_error_clear();

        while(1)
        {
            button = get_action(CONTEXT_FM, TIMEOUT_BLOCK);
            if(button == ACTION_FM_STOP)
                break;
        }
    }

    audio_init_playback();
#endif /* CONFIG_CODEC != SWCODEC */

    sound_settings_apply();
#endif /* SIMULATOR */

    if(keep_playing)
    {
/* Catch FMRADIO_PLAYING status for the sim. */
#ifndef SIMULATOR
#if CONFIG_CODEC != SWCODEC
        /* Enable the Left and right A/D Converter */
        audio_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
                                 sound_default(SOUND_RIGHT_GAIN),
                                 AUDIO_GAIN_LINEIN);
        mas_codec_writereg(6, 0x4000);
#endif
        end_search();
#endif /* SIMULATOR */
    }
    else
    {
#if CONFIG_CODEC == SWCODEC
        audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
#else
        radio_stop();
#endif
    }

#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(false);
#endif
    fms_fix_displays(FMS_EXIT);
    in_screen = false;
#if CONFIG_CODEC != SWCODEC
    return have_recorded;
#else
    return false;
#endif
} /* radio_screen */

void toggle_mono_mode(bool mono)
{
    tuner_set(RADIO_FORCE_MONO, mono);
}

void set_radio_region(int region)
{
#ifdef HAVE_RADIO_REGION
    tuner_set(RADIO_REGION, region);
#endif
    next_station(0);
    remember_frequency();
    (void)region;
}

MENUITEM_SETTING(set_region, &global_settings.fm_region, NULL);
MENUITEM_SETTING(force_mono, &global_settings.fm_force_mono, NULL);

#ifndef FM_MODE
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



#ifdef HAVE_RECORDING

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

#endif /* defined(HAVE_FMRADIO_REC) || CONFIG_CODEC != SWCODEC */
#endif /* HAVE_RECORDING */

#ifdef FM_RECORDING_SCREEN
MENUITEM_FUNCTION(recscreen_item, 0, ID2P(LANG_RECORDING),
                    fm_recording_screen, NULL, NULL, Icon_Recording);
#endif
#ifdef FM_RECORDING_SETTINGS
MENUITEM_FUNCTION(recsettings_item, 0, ID2P(LANG_RECORDING_SETTINGS),
                    fm_recording_settings, NULL, NULL, Icon_Recording);
#endif
#ifndef FM_PRESET
int handle_radio_presets_menu(void)
{
    return handle_radio_presets();
}
MENUITEM_FUNCTION(radio_presets_item, 0, ID2P(LANG_PRESET),
                    handle_radio_presets_menu, NULL, NULL, Icon_NOICON);
#endif
#ifndef FM_PRESET_ADD
int handle_radio_addpreset_menu(void)
{
    return radio_add_preset();
}
MENUITEM_FUNCTION(radio_addpreset_item, 0, ID2P(LANG_FM_ADD_PRESET),
                    radio_add_preset, NULL, NULL, Icon_NOICON);
#endif


MENUITEM_FUNCTION(presetload_item, 0, ID2P(LANG_FM_PRESET_LOAD),
                    load_preset_list, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(presetsave_item, 0, ID2P(LANG_FM_PRESET_SAVE),
                    save_preset_list, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(presetclear_item, 0, ID2P(LANG_FM_PRESET_CLEAR),
                    clear_preset_list, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(scan_presets_item, MENU_FUNC_USEPARAM,
                    ID2P(LANG_FM_SCAN_PRESETS),
                    scan_presets, NULL, NULL, Icon_NOICON);

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
/* main menu of the radio screen */
static bool radio_menu(void)
{
    return do_menu(&radio_settings_menu, NULL, NULL, false) ==
                                                            MENU_ATTACHED_USB;
}

#endif
