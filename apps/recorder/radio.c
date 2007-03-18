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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "sprintf.h"
#include "mas.h"
#include "settings.h"
#include "button.h"
#include "status.h"
#include "thread.h"
#include "mpeg.h"
#include "audio.h"
#include "mp3_playback.h"
#include "ctype.h"
#include "file.h"
#include "errno.h"
#include "atoi.h"
#include "string.h"
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
#include "recording.h"
#include "talk.h"
#include "tuner.h"
#include "power.h"
#include "sound.h"
#include "screen_access.h"
#include "statusbar.h"
#include "textarea.h"
#include "splash.h"
#include "yesno.h"
#include "buttonbar.h"
#include "power.h"
#include "tree.h"
#include "dir.h"
#include "action.h"
#include "list.h"
#include "menus/exported_menus.h"
#include "root_menu.h"

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
#endif

#define RADIO_SCAN_MODE 0
#define RADIO_PRESET_MODE 1

static const struct fm_region_setting fm_region[] = {
    /* Note: Desriptive strings are just for display atm and are not compiled. */
    FM_REGION_ENTRY("Europe",    87500000, 108000000,  50000, 0, 0),
    FM_REGION_ENTRY("US/Canada", 87900000, 107900000, 200000, 1, 0),
    FM_REGION_ENTRY("Japan",     76000000,  90000000, 100000, 0, 1),
    FM_REGION_ENTRY("Korea",     87500000, 108000000, 100000, 0, 0),
    };

static int curr_preset = -1;
static int curr_freq;
static int radio_mode = RADIO_SCAN_MODE;

static int radio_status = FMRADIO_OFF;
static bool in_screen = false; 

#define MAX_PRESETS 64
static bool presets_loaded = false, presets_changed = false;
static struct fmstation presets[MAX_PRESETS];

static char filepreset[MAX_PATH]; /* preset filename variable */

static int num_presets = 0; /* The number of presets in the preset list */

static void radio_save_presets(void);
static int handle_radio_presets(void);
static bool radio_menu(void);
static int radio_add_preset(void);
static int save_preset_list(void);
static int load_preset_list(void);
static int clear_preset_list(void);

static int scan_presets(void);

/* Function to manipulate all yesno dialogues.
   This function needs the output text as an argument. */
static bool yesno_pop(char* text)
{
    int i;
    char *lines[]={text};
    struct text_message message={lines, 1};
    bool ret = (gui_syncyesno_run(&message,NULL,NULL)== YESNO_YES);
    FOR_NB_SCREENS(i)
        gui_textarea_clear(&screens[i]);
    return ret;
}

void radio_init(void)
{
    tuner_init();
    radio_stop();
}

int get_radio_status(void)
{
    return radio_status;
}

bool in_radio_screen(void)   
{   
    return in_screen;   
} 

/* secret flag for starting paused - prevents unmute */
#define FMRADIO_START_PAUSED 0x8000
void radio_start(void)
{
    const struct fm_region_setting *fmr;
    bool start_paused;
    int mute_timeout;

    if(radio_status == FMRADIO_PLAYING)
        return;

    fmr = &fm_region[global_settings.fm_region];

    start_paused = radio_status & FMRADIO_START_PAUSED;
    /* clear flag before any yielding */
    radio_status &= ~FMRADIO_START_PAUSED;

    if(radio_status == FMRADIO_OFF)
        radio_power(true);

    curr_freq = global_status.last_frequency 
        * fmr->freq_step + fmr->freq_min;

    radio_set(RADIO_SLEEP, 0); /* wake up the tuner */
    radio_set(RADIO_FREQUENCY, curr_freq);

    if(radio_status == FMRADIO_OFF)
    {
#if (CONFIG_TUNER & S1A0903X01)
        radio_set(RADIO_IF_MEASUREMENT, 0);
        radio_set(RADIO_SENSITIVITY, 0);
#endif
        radio_set(RADIO_FORCE_MONO, global_settings.fm_force_mono);
#if (CONFIG_TUNER & TEA5767)
        radio_set(RADIO_SET_DEEMPHASIS, fmr->deemphasis);
        radio_set(RADIO_SET_BAND, fmr->band);
#endif
        mute_timeout = current_tick + 1*HZ;
    }
    else
    {
        /* paused */
        mute_timeout = current_tick + 2*HZ;
    }

    while(!radio_get(RADIO_STEREO) && !radio_get(RADIO_TUNED))
    {
        if(TIME_AFTER(current_tick, mute_timeout))
             break;
        yield();
    }

    /* keep radio from sounding initially */
    if(!start_paused)
        radio_set(RADIO_MUTE, 0);

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

    radio_set(RADIO_MUTE, 1);
    radio_set(RADIO_SLEEP, 1);

    radio_status = FMRADIO_PAUSED;
} /* radio_pause */

void radio_stop(void)
{
    if(radio_status == FMRADIO_OFF)
        return;

    radio_set(RADIO_MUTE, 1);
    radio_set(RADIO_SLEEP, 1); /* low power mode, if available */
    radio_status = FMRADIO_OFF;
    radio_power(false); /* status update, power off if avail. */
} /* radio_stop */

bool radio_hardware_present(void)
{
#ifdef HAVE_TUNER_PWR_CTRL
    bool ret;
    bool fmstatus = radio_power(true); /* power it up */
    ret = radio_get(RADIO_PRESENT);
    radio_power(fmstatus); /* restore previous state */
    return ret;
#else
    return radio_get(RADIO_PRESENT);
#endif
}

/* Keep freq on the grid for the current region */
static int snap_freq_to_grid(int freq)
{
    const struct fm_region_setting * const fmr =
        &fm_region[global_settings.fm_region];

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

/* Find a matching preset to freq */
static int find_preset(int freq)
{
    int i;
    if(num_presets < 1)
        return -1;
    for(i = 0;i < MAX_PRESETS;i++)
    {
        if(freq == presets[i].frequency)
            return i;
    }

    return -1;
}

/* Return the first preset encountered in the search direction with
   wraparound. */
static int find_closest_preset(int freq, int direction)
{
    int i;

    if (direction == 0) /* direction == 0 isn't really used */
        return 0;

    for (i = 0; i < MAX_PRESETS; i++)
    {
        int preset_frequency = presets[i].frequency;

        if (preset_frequency == freq)
            return i; /* Exact match = stop */
        /* Stop when the preset frequency exeeds freq so that we can
           pick the correct one based on direction */
        if (preset_frequency > freq)
            break;
    }

    /* wrap around depending on direction */
    if (i == 0 || i >= num_presets - 1)
        i = direction < 0 ? num_presets - 1 : 0;
    else if (direction < 0)
        i--; /* use previous */

    return i;
}

static void remember_frequency(void)
{
    const struct fm_region_setting * const fmr =
        &fm_region[global_settings.fm_region];

    global_status.last_frequency = (curr_freq - fmr->freq_min) 
                                   / fmr->freq_step;
    status_save();
}

static void next_preset(int direction)
{
    if (num_presets < 1)
        return;
            
    if (curr_preset == -1)
        curr_preset = find_closest_preset(curr_freq, direction);
    else
        curr_preset = (curr_preset + direction + num_presets) % num_presets;

    /* Must stay on the current grid for the region */
    curr_freq = snap_freq_to_grid(presets[curr_preset].frequency);

    radio_set(RADIO_FREQUENCY, curr_freq);
    remember_frequency();
}

/* Step to the next or previous frequency */
static int step_freq(int freq, int direction)
{
    const struct fm_region_setting * const fmr =
        &fm_region[global_settings.fm_region];

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
static void next_station(int direction)
{
    if (direction != 0 && radio_mode != RADIO_SCAN_MODE)
    {
        next_preset(direction);
        return;
    }

    curr_freq = step_freq(curr_freq, direction);

    if (radio_status == FMRADIO_PLAYING)
        radio_set(RADIO_MUTE, 1);

    radio_set(RADIO_FREQUENCY, curr_freq);

    if (radio_status == FMRADIO_PLAYING)
        radio_set(RADIO_MUTE, 0);

    curr_preset = find_preset(curr_freq);
    remember_frequency();
}

int radio_screen(void)
{
    char buf[MAX_PATH];
    bool done = false;
    int ret_val = GO_TO_ROOT;
    int button;
    int i;
    int search_dir = 0;
    bool stereo = false, last_stereo = false;
    int fh;
    int top_of_screen = 0;
    bool update_screen = true;
    bool screen_freeze = false;
    bool keep_playing = false;
    bool statusbar = global_settings.statusbar;
#ifdef FM_RECORD_DBLPRE
    int lastbutton = BUTTON_NONE;
    unsigned long rec_lastclick = 0;
#endif
#if CONFIG_CODEC != SWCODEC
    bool have_recorded = false;
    int timeout = current_tick + HZ/10;
    unsigned int seconds = 0;
    unsigned int last_seconds = 0;
    int hours, minutes;
    struct audio_recording_options rec_options;
#endif /* CONFIG_CODEC != SWCODEC */
#ifndef HAVE_NOISY_IDLE_MODE
    int button_timeout = current_tick + (2*HZ);
#endif
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
#endif

    /* Ends an in-progress search - needs access to search_dir */
    void end_search(void)
    {
        if (search_dir != 0 && radio_status == FMRADIO_PLAYING)
            radio_set(RADIO_MUTE, 0);
        search_dir = 0;
    }

    /* change status to "in screen" */
    in_screen = true;

    /* always display status bar in radio screen for now */
    global_settings.statusbar = true;
    FOR_NB_SCREENS(i)
    {
        gui_textarea_clear(&screens[i]);
        screen_set_xmargin(&screens[i],0);
    }
    
    gui_syncstatusbar_draw(&statusbars,true);

    fh = font_get(FONT_UI)->height;
    
    /* Adjust for font size, trying to center the information vertically */
    if(fh < 10)
        top_of_screen = 1;
    
    if(num_presets <= 0)
    {
        memset(presets, 0, sizeof(presets));
        radio_load_presets(global_settings.fmr_file);
    }
                                       
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

    peak_meter_enabled = true;

    rec_init_recording_options(&rec_options);
    rec_options.rec_source = AUDIO_SRC_LINEIN;
    rec_set_recording_options(&rec_options);

    audio_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
            sound_default(SOUND_RIGHT_GAIN), AUDIO_GAIN_LINEIN);

#endif /* CONFIG_CODEC != SWCODEC */
#endif /* ndef SIMULATOR */

    /* turn on radio */
#if CONFIG_CODEC == SWCODEC
    rec_set_source(AUDIO_SRC_FMRADIO,
                   (radio_status == FMRADIO_PAUSED) ?
                       SRCF_FMRADIO_PAUSED : SRCF_FMRADIO_PLAYING);
#else
    if (radio_status == FMRADIO_OFF)
        radio_start();
#endif

   if(num_presets < 1 && yesno_pop(str(LANG_FM_FIRST_AUTOSCAN)))
        scan_presets();
    
    curr_preset = find_preset(curr_freq);
    if(curr_preset != -1)
         radio_mode = RADIO_PRESET_MODE;

#ifdef HAS_BUTTONBAR
    gui_buttonbar_set(&buttonbar, str(LANG_BUTTONBAR_MENU),
        str(LANG_FM_BUTTONBAR_PRESETS), str(LANG_FM_BUTTONBAR_RECORD));
#endif

#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(true);
#endif

    while(!done)
    {
        if(search_dir != 0)
        {
            curr_freq = step_freq(curr_freq, search_dir);
            update_screen = true;

            if(radio_set(RADIO_SCAN_FREQUENCY, curr_freq))
            {
                curr_preset = find_preset(curr_freq);
                remember_frequency();
                end_search();
            }

            trigger_cpu_boost();
        }

#if CONFIG_CODEC != SWCODEC
        /* TODO: Can we timeout at HZ when recording since peaks aren't
           displayed? This should quiet recordings too. */
        button = get_action(CONTEXT_FM,
            update_screen ? TIMEOUT_NOBLOCK : HZ / PEAK_METER_FPS);
#else
        button = get_action(CONTEXT_FM,
            update_screen ? TIMEOUT_NOBLOCK : HZ);
#endif

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
                    if(presets_changed)
                    {
                        if(yesno_pop(str(LANG_FM_SAVE_CHANGES)))
                        {
                            if(filepreset[0] == '\0')
                                save_preset_list();
                            else
                                radio_save_presets();
                        }
                    }
                    /* Clear the preset list on exit. */
                    clear_preset_list();
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
                    rec_new_file();
                    update_screen = true;
                }
                else
                {
                    have_recorded = true;
                    rec_record();
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
                if(presets_changed)
                {
                    if(yesno_pop(str(LANG_FM_SAVE_CHANGES)))
                    {
                        if(filepreset[0] == '\0')
                            save_preset_list();
                        else
                            radio_save_presets();
                    }                    
                }
                
                /* Clear the preset list on exit. */
                clear_preset_list();
                
                break;

            case ACTION_STD_PREV:
            case ACTION_STD_NEXT:
                next_station(button == ACTION_STD_PREV ? -1 : 1);
                end_search();
                update_screen = true;
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
                }
                else if (dir == 0)
                {
                    /* Starting auto scan */
                    radio_set(RADIO_MUTE, 1);
                    update_screen = true;
                }
                break;
                }

            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_INCREPEAT:
                global_settings.volume++;
                if(global_settings.volume > sound_max(SOUND_VOLUME))
                    global_settings.volume = sound_max(SOUND_VOLUME);
                sound_set_volume(global_settings.volume);
                update_screen = true;
                settings_save();
                break;

            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                global_settings.volume--;
                if(global_settings.volume < sound_min(SOUND_VOLUME))
                    global_settings.volume = sound_min(SOUND_VOLUME);
                sound_set_volume(global_settings.volume);
                update_screen = true;
                settings_save();
                break;

            case ACTION_FM_PLAY:
                if (radio_status == FMRADIO_PLAYING)
                    radio_pause();
                else
                    radio_start();

                update_screen = true;
                break;

            case ACTION_FM_MENU:
                radio_menu();
                curr_preset = find_preset(curr_freq);
                FOR_NB_SCREENS(i){
                    struct screen *sc = &screens[i];
                    gui_textarea_clear(sc);
                    screen_set_xmargin(sc, 0);
                }
#ifdef HAS_BUTTONBAR
                gui_buttonbar_set(&buttonbar, str(LANG_BUTTONBAR_MENU),
                                  str(LANG_FM_BUTTONBAR_PRESETS),
                                  str(LANG_FM_BUTTONBAR_RECORD));
#endif
                update_screen = true;
                break;
      
#ifdef FM_PRESET
            case ACTION_FM_PRESET:
                if(num_presets < 1)
                {
                    gui_syncsplash(HZ, str(LANG_FM_NO_PRESETS));
                    update_screen = true;
                    FOR_NB_SCREENS(i)
                    {
                        struct screen *sc = &screens[i];
                        gui_textarea_clear(sc);
                        screen_set_xmargin(sc, 0);
                        gui_textarea_update(sc);
                    }

                    break;
                }
                handle_radio_presets();
                FOR_NB_SCREENS(i)
                {
                    struct screen *sc = &screens[i];
                    gui_textarea_clear(sc);
                    screen_set_xmargin(sc, 0);
                    gui_textarea_update(sc);
                }
#ifdef HAS_BUTTONBAR
                gui_buttonbar_set(&buttonbar,
                                  str(LANG_BUTTONBAR_MENU),
                                  str(LANG_FM_BUTTONBAR_PRESETS),
                                  str(LANG_FM_BUTTONBAR_RECORD));
#endif
                update_screen = true;
                break;
#endif /* FM_PRESET */
                
#ifdef FM_FREEZE
            case ACTION_FM_FREEZE:
                if(!screen_freeze)
                {
                    gui_syncsplash(HZ, str(LANG_FM_FREEZE));
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
                    if(num_presets > 0)
                        radio_mode = RADIO_PRESET_MODE;
                }
                else
                    radio_mode = RADIO_SCAN_MODE;
                update_screen = true;
                break;
#endif /* FM_MODE */

#ifdef FM_NEXT_PRESET
            case ACTION_FM_NEXT_PRESET:
                next_preset(1);
                end_search();
                update_screen = true;
                break;
#endif

#ifdef FM_PREV_PRESET
            case ACTION_FM_PREV_PRESET:
                next_preset(-1);
                end_search();
                update_screen = true;
                break;
#endif
                
            default:
                default_event_handler(button);
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
            if(!audio_status())
            {
                FOR_NB_SCREENS(i)
                {
                    peak_meter_screen(&screens[i],0,
                                          STATUSBAR_HEIGHT + fh*(top_of_screen + 4), fh);
                    screens[i].update_rect(0, STATUSBAR_HEIGHT + fh*(top_of_screen + 4),
                                               screens[i].width, fh);
                }
            }

            if(TIME_AFTER(current_tick, timeout))
            {
                timeout = current_tick + HZ;
#else  /* SWCODEC */
            {
#endif /* CONFIG_CODEC == SWCODEC */

                /* keep "mono" from always being displayed when paused */
                if (radio_status != FMRADIO_PAUSED)
                {
                    stereo = radio_get(RADIO_STEREO) &&
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
            if (update_screen || seconds > last_seconds)
            {
                last_seconds = seconds;
#else
            if (update_screen)
            {
#endif
                int freq;

                FOR_NB_SCREENS(i)
                    screens[i].setfont(FONT_UI);
                
                snprintf(buf, 128, curr_preset >= 0 ? "%d. %s" : " ",
                         curr_preset + 1, presets[curr_preset].name);

                FOR_NB_SCREENS(i)
                    screens[i].puts_scroll(0, top_of_screen, buf);
                
                freq = curr_freq / 10000;
                snprintf(buf, 128, str(LANG_FM_STATION), freq / 100, freq % 100);
                FOR_NB_SCREENS(i)
                    screens[i].puts_scroll(0, top_of_screen + 1, buf);
                
                snprintf(buf, 128, stereo?str(LANG_CHANNEL_STEREO):
                                          str(LANG_CHANNEL_MONO));
                FOR_NB_SCREENS(i)
                    screens[i].puts_scroll(0, top_of_screen + 2, buf);

                snprintf(buf, 128, "%s %s", str(LANG_FM_TUNE_MODE),
                         radio_mode ? str(LANG_RADIO_PRESET_MODE) :
                                      str(LANG_RADIO_SCAN_MODE));
                FOR_NB_SCREENS(i)
                    screens[i].puts_scroll(0, top_of_screen + 3, buf);

#if CONFIG_CODEC != SWCODEC
                if(audio_status() == AUDIO_STATUS_RECORD)
                {
                    hours = seconds / 3600;
                    minutes = (seconds - (hours * 3600)) / 60;
                    snprintf(buf, 32, "%s %02d:%02d:%02d",
                             str(LANG_RECORDING_TIME),
                             hours, minutes, seconds%60);
                    FOR_NB_SCREENS(i)
                        screens[i].puts_scroll(0, top_of_screen + 4, buf);
                }
                else
                {
                    if(rec_options.rec_prerecord_time)
                    {
                        snprintf(buf, 32, "%s %02d",
                                 str(LANG_RECORD_PRERECORD), seconds%60);
                        FOR_NB_SCREENS(i)
                            screens[i].puts_scroll(0, top_of_screen + 4, buf);
                    }
                }
#endif /* CONFIG_CODEC != SWCODEC */

#ifdef HAS_BUTTONBAR
                gui_buttonbar_draw(&buttonbar);
#endif
                FOR_NB_SCREENS(i)
                    gui_textarea_update(&screens[i]);
            }
            /* Only force the redraw if update_screen is true */
            gui_syncstatusbar_draw(&statusbars,true);
        }

        update_screen = false;

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
        gui_syncsplash(0, str(LANG_DISK_FULL));
        gui_syncstatusbar_draw(&statusbars,true);
        FOR_NB_SCREENS(i)
            gui_textarea_update(&screens[i]);
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
        rec_set_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
#else
        radio_stop();
#endif
    }

#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(false);
#endif

    /* restore status bar settings */
    global_settings.statusbar = statusbar;

    in_screen = false;
#if CONFIG_CODEC != SWCODEC
    return have_recorded;
#else
    return false;
#endif
} /* radio_screen */

static void radio_save_presets(void)
{
    int fd;
    int i;
          
    fd = creat(filepreset);
    if(fd >= 0)
    {
        for(i = 0;i < num_presets;i++)
        {
            fdprintf(fd, "%d:%s\n", presets[i].frequency, presets[i].name);
        }
        close(fd);
        
        if(!strncasecmp(FMPRESET_PATH, filepreset, strlen(FMPRESET_PATH)))
            set_file(filepreset, global_settings.fmr_file, MAX_FILENAME);
        presets_changed = false;
    }
    else
    {
        gui_syncsplash(HZ, str(LANG_FM_PRESET_SAVE_FAILED));
    }    
}

void radio_load_presets(char *filename)
{
    int fd;
    int rc;
    char buf[128];
    char *freq;
    char *name;
    bool done = false;
    int f;

    memset(presets, 0, sizeof(presets));
    num_presets = 0;

/* No Preset in configuration. */
    if(filename[0] == '\0')
    {
        filepreset[0] = '\0';
        return;
    }
/* Temporary preset, loaded until player shuts down. */
    else if(filename[0] == '/') 
        strncpy(filepreset, filename, sizeof(filepreset));
/* Preset from default directory. */
    else
        snprintf(filepreset, sizeof(filepreset), "%s/%s.fmr",
            FMPRESET_PATH, filename);
    
    fd = open(filepreset, O_RDONLY);
    if(fd >= 0)
    {
        while(!done && num_presets < MAX_PRESETS)
        {
            rc = read_line(fd, buf, 128);
            if(rc > 0)
            {
                if(settings_parseline(buf, &freq, &name))
                {
                    f = atoi(freq);
                    if(f) /* For backwards compatibility */
                    {
                        struct fmstation * const fms = &presets[num_presets];
                        fms->frequency = f;
                        strncpy(fms->name, name, MAX_FMPRESET_LEN);
                        fms->name[MAX_FMPRESET_LEN] = '\0';
                        num_presets++;
                    }
                }
            }
            else
                done = true;
        }
        close(fd);
    }
    else /* invalid file name? */
        filepreset[0] = '\0';

    presets_loaded = num_presets > 0;
    presets_changed = false;
}


static int radio_add_preset(void)
{
    char buf[MAX_FMPRESET_LEN];

    if(num_presets < MAX_PRESETS)
    {
        memset(buf, 0, MAX_FMPRESET_LEN);
        
        if (!kbd_input(buf, MAX_FMPRESET_LEN))
        {
            struct fmstation * const fms = &presets[num_presets];
            buf[MAX_FMPRESET_LEN] = '\0';
            strcpy(fms->name, buf);
            fms->frequency = curr_freq;
            num_presets++;
            presets_changed = true;
            presets_loaded = num_presets > 0;
        }
    }
    else
    {
        gui_syncsplash(HZ, str(LANG_FM_NO_FREE_PRESETS));
    }
    return true;
}

/* needed to know which preset we are edit/delete-ing */
static int selected_preset = -1;
static int radio_edit_preset(void)
{
    char buf[MAX_FMPRESET_LEN];

    if (num_presets > 0)
    {
        struct fmstation * const fms = &presets[selected_preset];

        strncpy(buf, fms->name, MAX_FMPRESET_LEN);

        if (!kbd_input(buf, MAX_FMPRESET_LEN))
        {
            buf[MAX_FMPRESET_LEN] = '\0';
            strcpy(fms->name, buf);
            presets_changed = true;
        }
    }

    return true;
}

static int radio_delete_preset(void)
{
    if (num_presets > 0)
    {
        struct fmstation * const fms = &presets[selected_preset];

        if (selected_preset >= --num_presets)
            selected_preset = num_presets - 1;

        memmove(fms, fms + 1, (uintptr_t)(fms + num_presets) -
                              (uintptr_t)fms);

    }

     /* Don't ask to save when all presets are deleted. */
    presets_changed = num_presets > 0;

    if (!presets_changed)
    {
        /* The preset list will be cleared, switch to Scan Mode. */
        radio_mode = RADIO_SCAN_MODE;
        presets_loaded = false;
    }
        
    return true;
}

static int load_preset_list(void)
{
    return !rockbox_browse(FMPRESET_PATH, SHOW_FMR);
}

static int save_preset_list(void)
{   
    if(num_presets > 0)
    { 
        bool bad_file_name = true;

        if(!opendir(FMPRESET_PATH)) /* Check if there is preset folder */
            mkdir(FMPRESET_PATH); 
    
        create_numbered_filename(filepreset, FMPRESET_PATH, "preset",
                                 ".fmr", 2 IF_CNFN_NUM_(, NULL));

        while(bad_file_name)
        {
            if(!kbd_input(filepreset, sizeof(filepreset)))
            {
                /* check the name: max MAX_FILENAME (20) chars */
                char* p2;
                char* p1;
                int len;
                p1 = strrchr(filepreset, '/');
                p2 = p1;
                while((p1) && (*p2) && (*p2 != '.'))
                    p2++;
                len = (int)(p2-p1) - 1;
                if((!p1) || (len > MAX_FILENAME) || (len == 0))
                {
                    /* no slash, too long or too short */
                    gui_syncsplash(HZ, str(LANG_INVALID_FILENAME));                    
                }
                else
                {
                    /* add correct extension (easier to always write)
                       at this point, p2 points to 0 or the extension dot */
                    *p2 = '\0';
                    strcat(filepreset,".fmr");
                    bad_file_name = false;
                    radio_save_presets();
                }
            }
            else
            {
                /* user aborted */
                return false;
            }
        }
    }
    else
        gui_syncsplash(HZ, str(LANG_FM_NO_PRESETS));
        
    return true;
}

static int clear_preset_list(void)
{
    /* Clear all the preset entries */
    memset(presets, 0, sizeof (presets));
    
    num_presets = 0;
    presets_loaded = false;
    /* The preset list will be cleared switch to Scan Mode. */
    radio_mode = RADIO_SCAN_MODE;
         
    presets_changed = false; /* Don't ask to save when clearing the list. */
    
    return true;
}

MENUITEM_FUNCTION(radio_edit_preset_item, 0, 
                    ID2P(LANG_FM_EDIT_PRESET), 
                    radio_edit_preset, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(radio_delete_preset_item, 0,
                    ID2P(LANG_FM_DELETE_PRESET), 
                    radio_delete_preset, NULL, NULL, Icon_NOICON);
int radio_preset_callback(int action, const struct menu_item_ex *this_item)
{
    if (action == ACTION_STD_OK)
        action = ACTION_EXIT_AFTER_THIS_MENUITEM;
    return action;
    (void)this_item;
}
MAKE_MENU(handle_radio_preset_menu, ID2P(LANG_FM_BUTTONBAR_PRESETS),
            radio_preset_callback, Icon_NOICON, &radio_edit_preset_item, 
            &radio_delete_preset_item);
/* present a list of preset stations */
char * presets_get_name(int selected_item, void * data, char *buffer)
{
    (void)data;
    (void)buffer;
    return presets[selected_item].name;
}

static int handle_radio_presets(void)
{
    struct gui_synclist lists;
    int result = 0;
    int action = ACTION_NONE;
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
#endif

    if(presets_loaded == false)
        return result;

#ifdef HAS_BUTTONBAR
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
    gui_buttonbar_set(&buttonbar, str(LANG_FM_BUTTONBAR_ADD),
                                 str(LANG_FM_BUTTONBAR_EXIT),
                                 str(LANG_FM_BUTTONBAR_ACTION));
    gui_buttonbar_draw(&buttonbar);
#endif
    gui_synclist_init(&lists, presets_get_name, NULL, false, 1);
    gui_synclist_set_title(&lists, str(LANG_FM_BUTTONBAR_PRESETS), NOICON);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_nb_items(&lists, num_presets);
    gui_synclist_select_item(&lists, curr_preset<0 ? 0 : curr_preset);

    action_signalscreenchange();
    while (result == 0)
    {
        gui_synclist_draw(&lists);
        gui_syncstatusbar_draw(&statusbars, true);
        action = get_action(CONTEXT_STD, HZ);

        gui_synclist_do_button(&lists, action, LIST_WRAP_UNLESS_HELD);
        switch (action)
        {
            case ACTION_STD_MENU:
                radio_add_preset();
                break;
            case ACTION_STD_CANCEL:
                result = 1;
                break;
            case ACTION_STD_OK:
                curr_preset = gui_synclist_get_sel_pos(&lists);
                curr_freq = presets[curr_preset].frequency;
                next_station(0);
                remember_frequency();
                result = 1;
                break;
            case ACTION_F3:
            case ACTION_STD_CONTEXT:
                selected_preset = gui_synclist_get_sel_pos(&lists);
                do_menu(&handle_radio_preset_menu, NULL);
                break;
            default:
                if(default_event_handler(action) == SYS_USB_CONNECTED)
                    result = 2;
        }
    }
    action_signalscreenchange();
    return result - 1;
}

void toggle_mono_mode(bool mono)
{
    radio_set(RADIO_FORCE_MONO, mono);
}

void set_radio_region(int region)
{
#if (CONFIG_TUNER & TEA5767)
    radio_set(RADIO_SET_DEEMPHASIS, 
        fm_region[region].deemphasis);
    radio_set(RADIO_SET_BAND, fm_region[region].band);
#else
    (void)region;
#endif
    next_station(0);
    remember_frequency();
}

MENUITEM_SETTING(set_region, &global_settings.fm_region, NULL);
MENUITEM_SETTING(force_mono, &global_settings.fm_force_mono, NULL);

#ifndef FM_MODE
char* get_mode_text(int selected_item, void * data, char *buffer)
{
    (void)selected_item;
    (void)data;
    snprintf(buffer, MAX_PATH, "%s %s", str(LANG_FM_TUNE_MODE),
             radio_mode ? str(LANG_RADIO_PRESET_MODE) :
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
                                 get_mode_text, NULL, NULL, Icon_NOICON);
#endif

static int scan_presets(void)
{
    bool do_scan = true;
    
    if(num_presets > 0) /* Do that to avoid 2 questions. */
        do_scan = yesno_pop(str(LANG_FM_CLEAR_PRESETS));
        
    if(do_scan)
    {
        const struct fm_region_setting * const fmr =
            &fm_region[global_settings.fm_region];
        char buf[MAX_FMPRESET_LEN];
        int i;

        curr_freq = fmr->freq_min;
        num_presets = 0;
        memset(presets, 0, sizeof(presets));
        radio_set(RADIO_MUTE, 1);

        while(curr_freq <= fmr->freq_max)
        {
            int freq, frac;
            if (num_presets >= MAX_PRESETS || action_userabort(TIMEOUT_NOBLOCK))
                break;

            freq = curr_freq / 10000;
            frac = freq % 100;
            freq /= 100;

            snprintf(buf, MAX_FMPRESET_LEN, str(LANG_FM_SCANNING), freq, frac);
            gui_syncsplash(0, buf);

            if(radio_set(RADIO_SCAN_FREQUENCY, curr_freq))
            {
                /* add preset */
                snprintf(buf, MAX_FMPRESET_LEN, 
                    str(LANG_FM_DEFAULT_PRESET_NAME), freq, frac);
                strcpy(presets[num_presets].name,buf);
                presets[num_presets].frequency = curr_freq;
                num_presets++;
            }

            curr_freq += fmr->freq_step;
        }

        if (radio_status == FMRADIO_PLAYING)
            radio_set(RADIO_MUTE, 0);

        presets_changed = true;
        
        FOR_NB_SCREENS(i)
        {
            gui_textarea_clear(&screens[i]);
            screen_set_xmargin(&screens[i],0);
            gui_textarea_update(&screens[i]);
        }

        if(num_presets > 0)
        {
            curr_freq = presets[0].frequency;
            radio_mode = RADIO_PRESET_MODE;
            presets_loaded = true;
            next_station(0);
        }
        else
        {
            /* Wrap it to beginning or we'll be past end of band */
            presets_loaded = false;
            next_station(1);
        }
    }
    return true;
}


#ifdef HAVE_RECORDING

#if defined(HAVE_FMRADIO_IN) && CONFIG_CODEC == SWCODEC
#define FM_RECORDING_SCREEN
static int fm_recording_screen(void)
{
    bool ret;

    /* switch recording source to FMRADIO for the duration */
    int rec_source = global_settings.rec_source;
    global_settings.rec_source = AUDIO_SRC_FMRADIO;

    /* clearing queue seems to cure a spontaneous abort during record */
    action_signalscreenchange();

    ret = recording_screen(true);

    /* safe to reset as changing sources is prohibited here */
    global_settings.rec_source = rec_source;

    return ret;
}

#endif /* defined(HAVE_FMRADIO_IN) && CONFIG_CODEC == SWCODEC */

#if defined(HAVE_FMRADIO_IN) || CONFIG_CODEC != SWCODEC
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

#endif /* defined(HAVE_FMRADIO_IN) || CONFIG_CODEC != SWCODEC */
#endif /* HAVE_RECORDING */

#ifdef FM_RECORDING_SCREEN
MENUITEM_FUNCTION(recscreen_item, 0, ID2P(LANG_RECORDING_MENU), 
                    fm_recording_screen, NULL, NULL, Icon_NOICON);
#endif
#ifdef FM_RECORDING_SETTINGS
MENUITEM_FUNCTION(recsettings_item, 0, ID2P(LANG_RECORDING_SETTINGS), 
                    fm_recording_settings, NULL, NULL, Icon_NOICON);
#endif
#ifndef FM_PRESET
MENUITEM_FUNCTION(radio_presets_item, 0, ID2P(LANG_FM_BUTTONBAR_PRESETS), 
                    handle_radio_presets, NULL, NULL, Icon_NOICON);
#endif
#ifndef FM_PRESET_ADD
MENUITEM_FUNCTION(radio_addpreset_item, 0, ID2P(LANG_FM_ADD_PRESET), 
                    radio_add_preset, NULL, NULL, Icon_NOICON);
#endif


MENUITEM_FUNCTION(presetload_item, 0, ID2P(LANG_FM_PRESET_LOAD), 
                    load_preset_list, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(presetsave_item, 0, ID2P(LANG_FM_PRESET_SAVE), 
                    save_preset_list, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(presetclear_item, 0, ID2P(LANG_FM_PRESET_CLEAR), 
                    clear_preset_list, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(scan_presets_item, 0, ID2P(LANG_FM_SCAN_PRESETS), 
                    scan_presets, NULL, NULL, Icon_NOICON);

MAKE_MENU(radio_menu_items, ID2P(LANG_FM_MENU), NULL, 
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
    return do_menu(&radio_menu_items, NULL) == MENU_ATTACHED_USB;
}

#endif
