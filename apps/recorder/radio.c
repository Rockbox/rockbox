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
#include "sprintf.h"
#include "mas.h"
#include "settings.h"
#include "button.h"
#include "status.h"
#include "kernel.h"
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
#include "hwcompat.h"
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

#ifdef CONFIG_TUNER

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

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define FM_PRESET
#define FM_MODE

#elif CONFIG_KEYPAD == ONDIO_PAD
#define FM_RECORD_DBLPRE
#define FM_RECORD
#endif

#define RADIO_SCAN_MODE 0
#define RADIO_PRESET_MODE 1

#if (CONFIG_TUNER & TEA5767)
#define DEEMPH_50 0,
#define DEEMPH_75 1,
#define BAND_LIM_EU 0
#define BAND_LIM_JP 1
#else
#define DEEMPH_50
#define DEEMPH_75
#define BAND_LIM_EU
#define BAND_LIM_JP
#endif
static struct fm_region_setting fm_region[] = {
    /* Europe */
    { LANG_FM_EUROPE, 87500000, 108000000,   50000, DEEMPH_50 BAND_LIM_EU },
    /* US / Canada */
    { LANG_FM_US,     87900000, 107900000,  200000, DEEMPH_75 BAND_LIM_EU },
    /* Japan */
    { LANG_FM_JAPAN,  76000000,  90000000,  100000, DEEMPH_50 BAND_LIM_JP },
    /* Korea */
    { LANG_FM_KOREA,  87500000, 108000000,  100000, DEEMPH_50 BAND_LIM_EU },
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

static int preset_menu; /* The menu index of the preset list */
static struct menu_item preset_menu_items[MAX_PRESETS];
static int num_presets = 0; /* The number of presets in the preset list */

static void radio_save_presets(void);
static bool handle_radio_presets(void);
static bool radio_menu(void);
static bool radio_add_preset(void);
static bool save_preset_list(void);
static bool load_preset_list(void);
static bool clear_preset_list(void);

static bool scan_presets(void);

#ifdef SIMULATOR
void radio_set(int setting, int value);
int radio_get(int setting);
#else
#if CONFIG_TUNER == S1A0903X01 /* FM recorder */
#define radio_set samsung_set
#define radio_get samsung_get
#elif CONFIG_TUNER == TEA5767 /* iriver, iaudio */
#define radio_set philips_set
#define radio_get philips_get
#elif CONFIG_TUNER == (S1A0903X01 | TEA5767) /* OndioFM */
static void (*radio_set)(int setting, int value);
static int (*radio_get)(int setting);
#endif
#endif

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
#ifndef SIMULATOR
#if CONFIG_TUNER == (S1A0903X01 | TEA5767)
    if (read_hw_mask() & TUNER_MODEL)
    {
        radio_set = philips_set;
        radio_get = philips_get;
    }
    else
    {
        radio_set = samsung_set;
        radio_get = samsung_get;
    }
#endif
#endif
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
    bool start_paused;
    int mute_timeout;

    if(radio_status == FMRADIO_PLAYING)
        return;

    start_paused = radio_status & FMRADIO_START_PAUSED;
    /* clear flag before any yielding */
    radio_status &= ~FMRADIO_START_PAUSED;

    if(radio_status == FMRADIO_OFF)
        radio_power(true);

    curr_freq = global_status.last_frequency 
        * fm_region[global_settings.fm_region].freq_step 
        + fm_region[global_settings.fm_region].freq_min;

    radio_set(RADIO_SLEEP, 0); /* wake up the tuner */
    radio_set(RADIO_FREQUENCY, curr_freq);

    if(radio_status == FMRADIO_OFF)
    {
        radio_set(RADIO_IF_MEASUREMENT, 0);
        radio_set(RADIO_SENSITIVITY, 0);
        radio_set(RADIO_FORCE_MONO, global_settings.fm_force_mono);
#if (CONFIG_TUNER & TEA5767)
        radio_set(RADIO_SET_DEEMPHASIS, 
            fm_region[global_settings.fm_region].deemphasis);
        radio_set(RADIO_SET_BAND, fm_region[global_settings.fm_region].band);
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

static int find_closest_preset(int freq)
{
    int i; 
    int diff;
    int min_diff = fm_region[global_settings.fm_region].freq_min;
    int preset = -1;

    for(i = 0;i < MAX_PRESETS;i++)
    {
        diff = freq - presets[i].frequency;
        if(diff==0)
            return i;
        if(diff < 0)
            diff = -diff;
        if(diff < min_diff)
        {
            preset = i;
            min_diff = diff;
        }
    }

    return preset;
}

static void remember_frequency(void)
{
    global_status.last_frequency = (curr_freq 
        - fm_region[global_settings.fm_region].freq_min) 
        / fm_region[global_settings.fm_region].freq_step;
    status_save();
}

static void next_preset(int direction)
{
    if (num_presets < 1)
        return;
            
    if(curr_preset == -1)
        curr_preset = find_closest_preset(curr_freq);
    
    if(direction > 0)
        if(curr_preset == num_presets - 1)
            curr_preset = 0;
        else
            curr_preset++;
    else
        if(curr_preset == 0)
            curr_preset = num_presets - 1;
        else
            curr_preset--;
            
    curr_freq = presets[curr_preset].frequency;
    radio_set(RADIO_FREQUENCY, curr_freq);
    remember_frequency();
}


bool radio_screen(void)
{
    char buf[MAX_PATH];
    bool done = false;
    int button, lastbutton = BUTTON_NONE;
#ifdef FM_RECORD_DBLPRE
    unsigned long rec_lastclick = 0;
#endif
    int freq, i;
    bool tuned;
    bool stereo = false;
    int search_dir = 0;
    int fh;
    bool last_stereo_status = false;
    int top_of_screen = 0;
    bool update_screen = true;
    int timeout = current_tick + HZ/10;
    bool screen_freeze = false;
    bool have_recorded = false;
    unsigned int seconds = 0;
    unsigned int last_seconds = 0;
#if CONFIG_CODEC != SWCODEC
    int hours, minutes;
    struct audio_recording_options rec_options;
#endif
    bool keep_playing = false;
    bool statusbar = global_settings.statusbar;
    int button_timeout = current_tick + (2*HZ);
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
#endif
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
    
    if(!num_presets)
    {
        memset(presets, 0, sizeof(presets));
        radio_load_presets(global_settings.fmr_file);
    }
                                       
#ifndef SIMULATOR
    if(radio_status == FMRADIO_OFF)    
        audio_stop();

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
    rec_set_source(AUDIO_SRC_FMRADIO, (radio_status == FMRADIO_PAUSED) ?
                       SRCF_FMRADIO_PAUSED : SRCF_FMRADIO_PLAYING);
#else
    if (radio_status == FMRADIO_OFF)
        radio_start();
#endif

    /* I hate this thing with vehement passion (jhMikeS): */
   if(num_presets == 0 && yesno_pop(str(LANG_FM_FIRST_AUTOSCAN)))
        scan_presets();
    
    curr_preset = find_preset(curr_freq);
    if(curr_preset != -1)
         radio_mode = RADIO_PRESET_MODE;

#ifdef HAS_BUTTONBAR
    gui_buttonbar_set(&buttonbar, str(LANG_BUTTONBAR_MENU),
        str(LANG_FM_BUTTONBAR_PRESETS), str(LANG_FM_BUTTONBAR_RECORD));
#endif

    cpu_idle_mode(true);
        
    while(!done)
    {
        if(search_dir)
        {
            curr_freq += search_dir 
                * fm_region[global_settings.fm_region].freq_step;
            if(curr_freq < fm_region[global_settings.fm_region].freq_min)
                curr_freq = fm_region[global_settings.fm_region].freq_max;
            if(curr_freq > fm_region[global_settings.fm_region].freq_max)
                curr_freq = fm_region[global_settings.fm_region].freq_min;

            /* Tune in and delay */
            radio_set(RADIO_FREQUENCY, curr_freq);
            sleep(1);
            
            /* Start IF measurement */
            radio_set(RADIO_IF_MEASUREMENT, 1);
            sleep(1);

            /* Now check how close to the IF frequency we are */
            tuned = radio_get(RADIO_TUNED);

            /* Stop searching if the tuning is close */
            if(tuned)
            {
                search_dir = 0;
                curr_preset = find_preset(curr_freq);
                remember_frequency();
            }
            
            update_screen = true;
        }

        if(search_dir)
            button = button_get(false);
        else
            button = get_action(CONTEXT_FM, HZ / PEAK_METER_FPS);
        if (button != ACTION_NONE)
        {
            cpu_idle_mode(false);
            button_timeout = current_tick + (2*HZ);
        }
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
#endif
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
#endif
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
                if(radio_mode == RADIO_SCAN_MODE)
                {
                     curr_freq 
                         -= fm_region[global_settings.fm_region].freq_step;
                     if(curr_freq < fm_region[global_settings.fm_region].freq_min)
                          curr_freq 
                          = fm_region[global_settings.fm_region].freq_max;
                     radio_set(RADIO_FREQUENCY, curr_freq);
                     curr_preset = find_preset(curr_freq);
                     remember_frequency();
                }
                else
                     next_preset(-1);
                search_dir = 0;
                update_screen = true;
                break;

            case ACTION_STD_NEXT:
                if(radio_mode == RADIO_SCAN_MODE)
                {
                     curr_freq 
                         += fm_region[global_settings.fm_region].freq_step;
                     if(curr_freq > fm_region[global_settings.fm_region].freq_max)
                           curr_freq 
                            = fm_region[global_settings.fm_region].freq_min;
                     radio_set(RADIO_FREQUENCY, curr_freq);
                     curr_preset = find_preset(curr_freq);
                     remember_frequency();
                }
                else
                     next_preset(1);
                search_dir = 0;
                update_screen = true;
                break;

            case ACTION_STD_PREVREPEAT:
                if(radio_mode == RADIO_SCAN_MODE)
                    search_dir = -1;
                else
                {
                    next_preset(-1);
                    update_screen = true;
                }

                break;

            case ACTION_STD_NEXTREPEAT:
                if(radio_mode == RADIO_SCAN_MODE)
                    search_dir = 1;
                else
                {
                    next_preset(1);
                    update_screen = true;
                }
                
                break;


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
                    gui_textarea_clear(&screens[i]);
                    screen_set_xmargin(&screens[i],0);
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
                    gui_syncsplash(HZ, true, str(LANG_FM_NO_PRESETS));
                    update_screen = true;
                    FOR_NB_SCREENS(i)
                    {
                        gui_textarea_clear(&screens[i]);
                        screen_set_xmargin(&screens[i],0);
                        gui_textarea_update(&screens[i]);
                    }

                    break;
                }
                handle_radio_presets();
                FOR_NB_SCREENS(i)
                {
                    gui_textarea_clear(&screens[i]);
                    screen_set_xmargin(&screens[i],0);
                    gui_textarea_update(&screens[i]);
                }
#ifdef HAS_BUTTONBAR
                gui_buttonbar_set(&buttonbar,
                                  str(LANG_BUTTONBAR_MENU),
                                  str(LANG_FM_BUTTONBAR_PRESETS),
                                  str(LANG_FM_BUTTONBAR_RECORD));
#endif
                update_screen = true;
                break;
#endif
                
#ifdef FM_FREEZE
            case ACTION_FM_FREEZE:
                if(!screen_freeze)
                {
                    gui_syncsplash(HZ, true, str(LANG_FM_FREEZE));
                    screen_freeze = true;
                }
                else
                {
                    update_screen = true;
                    screen_freeze = false;
                }
                break;
#endif                
            case SYS_USB_CONNECTED:
                /* Only accept USB connection when not recording */
                if(audio_status() != AUDIO_STATUS_RECORD)
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
#endif
#ifdef FM_NEXT_PRESET
            case ACTION_FM_NEXT_PRESET:
                next_preset(1);
                search_dir = 0;
                update_screen = true;
                break;
#endif
#ifdef FM_PREV_PRESET
            case ACTION_FM_PREV_PRESET:
                next_preset(-1);
                search_dir = 0;
                update_screen = true;
                break;
#endif
                
            default:
                default_event_handler(button);
                break;
        } /*switch(button)*/

        if (button != ACTION_NONE)
            lastbutton = button;

#if CONFIG_CODEC != SWCODEC
        peak_meter_peek();
#endif

        if(!screen_freeze)
        {           
            /* Only display the peak meter when not recording */
            if(!audio_status())
            {

#if CONFIG_CODEC != SWCODEC
                FOR_NB_SCREENS(i)
                {
                    peak_meter_screen(&screens[i],0,
                                          STATUSBAR_HEIGHT + fh*(top_of_screen + 4), fh);
                    screens[i].update_rect(0, STATUSBAR_HEIGHT + fh*(top_of_screen + 4),
                                               screens[i].width, fh);
                }
#endif

            }

            if(TIME_AFTER(current_tick, timeout))
            {
                timeout = current_tick + HZ;

                /* keep "mono" from always being displayed when paused */
                if (radio_status != FMRADIO_PAUSED)
                {
                    stereo = radio_get(RADIO_STEREO) &&
                        !global_settings.fm_force_mono;
                    if(stereo != last_stereo_status)
                    {
                        update_screen = true;
                        last_stereo_status = stereo;
                    }
                }
            }
            
#ifndef SIMULATOR
#if CONFIG_CODEC != SWCODEC
            seconds = audio_recorded_time() / HZ;
#endif
#endif
            if(update_screen || seconds > last_seconds)
            {
                last_seconds = seconds;

                FOR_NB_SCREENS(i)
                    screens[i].setfont(FONT_UI);
                
                if (curr_preset >= 0 )
                   snprintf(buf, 128, "%d. %s",curr_preset + 1,
                                presets[curr_preset].name);
                else
                   snprintf(buf, 128, " ");
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
#endif

#ifdef HAS_BUTTONBAR
                gui_buttonbar_draw(&buttonbar);
#endif
                FOR_NB_SCREENS(i)
                    gui_textarea_update(&screens[i]);
            }
            /* Only force the redraw if update_screen is true */
            gui_syncstatusbar_draw(&statusbars,true);

            update_screen = false;
        }

        if(audio_status() & AUDIO_STATUS_ERROR)
        {
            done = true;
        }
        if (TIME_AFTER(current_tick, button_timeout))
        {
            cpu_idle_mode(true);
        }
    } /*while(!done)*/

#ifndef SIMULATOR
    if(audio_status() & AUDIO_STATUS_ERROR)
    {
        gui_syncsplash(0, true, str(LANG_DISK_FULL));
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
    
#if CONFIG_CODEC != SWCODEC
    audio_init_playback();
#endif

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
#endif
    }
    else
    {
#if CONFIG_CODEC == SWCODEC
        rec_set_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
#else
        radio_stop();
#endif
    }
    
    cpu_idle_mode(false);

    /* restore status bar settings */
    global_settings.statusbar = statusbar;

    in_screen = false;
    
    return have_recorded;
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
        gui_syncsplash(HZ, true, str(LANG_FM_PRESET_SAVE_FAILED));
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
                        presets[num_presets].frequency = f;
                        strncpy(presets[num_presets].name, name, MAX_FMPRESET_LEN);
                        presets[num_presets].name[MAX_FMPRESET_LEN] = 0;
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
    
    if(num_presets > 0)
        presets_loaded = true;
    else
        presets_loaded = false;
        
    presets_changed = false;
}

static void rebuild_preset_menu(void)
{
    int i;
    for(i = 0;i < num_presets;i++)
    {
        preset_menu_items[i].desc = presets[i].name;
    }
}

static bool radio_add_preset(void)
{
    char buf[MAX_FMPRESET_LEN];

    if(num_presets < MAX_PRESETS)
    {
        memset(buf, 0, MAX_FMPRESET_LEN);
        
        if (!kbd_input(buf, MAX_FMPRESET_LEN))
        {
            buf[MAX_FMPRESET_LEN] = 0;
            strcpy(presets[num_presets].name, buf);
            presets[num_presets].frequency = curr_freq;
#ifdef FM_PRESET_ADD  /* only for archos */         
            menu_insert(preset_menu, -1,
                        presets[num_presets].name, 0);
            /* We must still rebuild the menu table, since the
               item name pointers must be updated */
            rebuild_preset_menu();
#endif 
            num_presets++;
            presets_changed = true;
            if(num_presets > 0)
                presets_loaded = true;
        }
    }
    else
    {
        gui_syncsplash(HZ, true, str(LANG_FM_NO_FREE_PRESETS));
    }
    return true;
}

/* button preprocessor for preset option submenu */
static int handle_radio_presets_menu_cb(int key, int m)
{
    (void)m;
#ifdef FM_PRESET_ACTION
    switch(key)
    {
        case ACTION_F3:
            key = ACTION_STD_CANCEL; /* Fake an exit */
            action_signalscreenchange();
            break;
    }
#endif
    return key;
}

static bool radio_edit_preset(void)
{
    int pos = menu_cursor(preset_menu);
    char buf[MAX_FMPRESET_LEN];

    strncpy(buf, menu_description(preset_menu, pos), MAX_FMPRESET_LEN);
        
    if (!kbd_input(buf, MAX_FMPRESET_LEN))
    {
        buf[MAX_FMPRESET_LEN] = 0;
        strcpy(presets[pos].name, buf);
        presets_changed = true;
    }
    return true;
}

static bool radio_delete_preset(void)
{
    int pos = menu_cursor(preset_menu);
    int i;

    for(i = pos;i < num_presets;i++)
        presets[i] = presets[i+1];
    num_presets--;
    
    menu_delete(preset_menu, pos);
    /* We must still rebuild the menu table, since the
       item name pointers must be updated */
    rebuild_preset_menu();
    
     /* Don't ask to save when all presets are deleted. */
    if(num_presets > 0)
        presets_changed = true;
    else
    {
        presets_changed = false;
        /* The preset list will be cleared, switch to Scan Mode. */
        radio_mode = RADIO_SCAN_MODE;
        presets_loaded = false;
    }
        
    return true; /* Make the menu return immediately */
}

static bool load_preset_list(void)
{
    return !rockbox_browse(FMPRESET_PATH, SHOW_FMR);
}

static bool save_preset_list(void)
{   
    if(num_presets != 0)
    { 
        bool bad_file_name = true;

        if(!opendir(FMPRESET_PATH)) /* Check if there is preset folder */
            mkdir(FMPRESET_PATH, 0); 
    
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
                    gui_syncsplash(HZ,true,str(LANG_INVALID_FILENAME));                    
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
        gui_syncsplash(HZ,true,str(LANG_FM_NO_PRESETS));
        
    return true;
}

static bool clear_preset_list(void)
{
    int i;
    
    /* Clear all the preset entries */
    for(i = 0;i <= num_presets;i++){
        presets[i].name[0] = '\0';
        presets[i].frequency = 0;
    }
    
    num_presets = 0;
    presets_loaded = false;
    /* The preset list will be cleared switch to Scan Mode. */
    radio_mode = RADIO_SCAN_MODE;
         
    presets_changed = false; /* Don't ask to save when clearing the list. */
    
    return true;
}

/* little menu on what to do with a preset entry */
static bool handle_radio_presets_menu(void)
{
    static const struct menu_item preset_menu_items[] = {
        { ID2P(LANG_FM_EDIT_PRESET), radio_edit_preset },
        { ID2P(LANG_FM_DELETE_PRESET), radio_delete_preset },
    };
    int m;

    m = menu_init( preset_menu_items,
                   sizeof preset_menu_items / sizeof(struct menu_item),
                   handle_radio_presets_menu_cb,
                   NULL, NULL, str(LANG_FM_BUTTONBAR_EXIT));
    menu_run(m);
    menu_exit(m);
    return false;
}

/* button preprocessor for list of preset stations menu */
static int handle_radio_presets_cb(int key, int m)
{
    (void)m;

    switch(key)
    {
#ifdef FM_PRESET_ADD
        case ACTION_STD_MENU:
            radio_add_preset();
            menu_draw(m);
            key = BUTTON_NONE;
            break;
#endif
#if (CONFIG_KEYPAD != IRIVER_H100_PAD) && (CONFIG_KEYPAD != IRIVER_H300_PAD) && (CONFIG_KEYPAD != IAUDIO_X5_PAD)
#ifdef FM_PRESET
        case ACTION_STD_QUICKSCREEN:
            menu_draw(m);
            key = ACTION_STD_CANCEL; /* Fake an exit */
            break;
#endif
#endif
        case ACTION_F3:
        case ACTION_STD_CONTEXT:
        {
            bool ret;
            ret = handle_radio_presets_menu();
            menu_draw(m);
            if(ret)
                key = SYS_USB_CONNECTED;
            else
                key = ACTION_NONE;
            break;
        }
    }
    return key;
}

/* present a list of preset stations */
static bool handle_radio_presets(void)
{
    int result;
    bool reload_dir = false;

    if(presets_loaded)
    {
        rebuild_preset_menu();

        /* DIY menu handling, since we want to exit after selection */
        preset_menu = menu_init( preset_menu_items, num_presets,
                                 handle_radio_presets_cb,
                                 str(LANG_FM_BUTTONBAR_ADD),
                                 str(LANG_FM_BUTTONBAR_EXIT),
                                 str(LANG_FM_BUTTONBAR_ACTION));
        if (curr_preset >= 0)
            menu_set_cursor(preset_menu, curr_preset);
        result = menu_show(preset_menu);
        menu_exit(preset_menu);
        if (result == MENU_SELECTED_EXIT)
            return false;
        else if (result == MENU_ATTACHED_USB)
            reload_dir = true;
        
        if (result >= 0) /* A preset was selected */
        {
            curr_preset = menu_cursor(preset_menu);
            curr_freq = presets[curr_preset].frequency;
            radio_set(RADIO_FREQUENCY, curr_freq);
            remember_frequency();
        }
    }
    
    return reload_dir;
}

static char monomode_menu_string[32];

static void create_monomode_menu(void)
{
    snprintf(monomode_menu_string, sizeof monomode_menu_string,
             "%s: %s", str(LANG_FM_MONO_MODE),
             global_settings.fm_force_mono?
             str(LANG_SET_BOOL_YES):str(LANG_SET_BOOL_NO));
}

static bool toggle_mono_mode(void)
{
    global_settings.fm_force_mono = !global_settings.fm_force_mono;
    radio_set(RADIO_FORCE_MONO, global_settings.fm_force_mono);
    settings_save();
    create_monomode_menu();
    return false;
}

static char region_menu_string[32];
static void create_region_menu(void)
{
    snprintf(region_menu_string, sizeof(region_menu_string),
    "%s: %s", str(LANG_FM_REGION),
        str(fm_region[global_settings.fm_region].lang));
}

static bool toggle_region_mode(void)
{
    global_settings.fm_region++;
    if(global_settings.fm_region >= 
        (int)(sizeof(fm_region) / sizeof(struct fm_region_setting)))
        global_settings.fm_region = 0;
#if (CONFIG_TUNER & TEA5767)
    radio_set(RADIO_SET_DEEMPHASIS, 
        fm_region[global_settings.fm_region].deemphasis);
    radio_set(RADIO_SET_BAND, fm_region[global_settings.fm_region].band);
#endif
    /* make sure the current frequency is in the region range */
    curr_freq -= (curr_freq - fm_region[global_settings.fm_region].freq_min)
        % fm_region[global_settings.fm_region].freq_step;
    if(curr_freq < fm_region[global_settings.fm_region].freq_min)
        curr_freq = fm_region[global_settings.fm_region].freq_min;
    if(curr_freq > fm_region[global_settings.fm_region].freq_max)
        curr_freq = fm_region[global_settings.fm_region].freq_max;
    radio_set(RADIO_FREQUENCY, curr_freq);

    remember_frequency();
    create_region_menu();
    return false;
}

#ifndef FM_MODE
static char radiomode_menu_string[32];

static void create_radiomode_menu(void)
{
    snprintf(radiomode_menu_string, 32, "%s %s", str(LANG_FM_TUNE_MODE),
             radio_mode ? str(LANG_RADIO_PRESET_MODE) :
                          str(LANG_RADIO_SCAN_MODE));
}

static bool toggle_radio_mode(void)
{
    radio_mode = (radio_mode == RADIO_SCAN_MODE) ?
                 RADIO_PRESET_MODE : RADIO_SCAN_MODE;
    create_radiomode_menu();
    return false;
}
#endif

static bool scan_presets(void)
{
    bool tuned = false, do_scan = true;
    char buf[MAX_FMPRESET_LEN];
    int freq, i;
    
    if(num_presets > 0) /* Do that to avoid 2 questions. */
        do_scan = yesno_pop(str(LANG_FM_CLEAR_PRESETS));
        
    if(do_scan)
    {
        curr_freq = fm_region[global_settings.fm_region].freq_min;
        num_presets = 0;
        memset(presets, 0, sizeof(presets));
        while(curr_freq <= fm_region[global_settings.fm_region].freq_max)
        {
            if (num_presets >= MAX_PRESETS || action_userabort(TIMEOUT_NOBLOCK))
                break;

            freq = curr_freq / 10000;
            snprintf(buf, MAX_FMPRESET_LEN, str(LANG_FM_SCANNING), 
                            freq/100, freq % 100);
            gui_syncsplash(0, true, buf);

            /* Tune in and delay */
            radio_set(RADIO_FREQUENCY, curr_freq);
            sleep(1);
            
            /* Start IF measurement */
            radio_set(RADIO_IF_MEASUREMENT, 1);
            sleep(1);

            /* Now check how close to the IF frequency we are */
            tuned = radio_get(RADIO_TUNED);

            /* add preset */
            if(tuned){
                 snprintf(buf, MAX_FMPRESET_LEN, 
                    str(LANG_FM_DEFAULT_PRESET_NAME),freq/100, freq % 100);
                 strcpy(presets[num_presets].name,buf);
                 presets[num_presets].frequency = curr_freq;
                 num_presets++;
            }

            curr_freq += fm_region[global_settings.fm_region].freq_step;
                   
        }

        presets_changed = true;
        
        FOR_NB_SCREENS(i)
        {
            gui_textarea_clear(&screens[i]);
            screen_set_xmargin(&screens[i],0);
            gui_textarea_update(&screens[i]);
        }

        if(num_presets > 0 )
        {
            curr_freq = presets[0].frequency;
            radio_set(RADIO_FREQUENCY, curr_freq);
            remember_frequency();
            radio_mode = RADIO_PRESET_MODE;
            presets_loaded = true;
        }
        else
            presets_loaded = false;
    }
    return true;
}

/* button preprocessor for the main menu */
static int radio_menu_cb(int key, int m)
{
    (void)m;
#if 0 /* this screen needs fixing! */
    switch(key)
    {
#if (CONFIG_KEYPAD != IRIVER_H100_PAD) && (CONFIG_KEYPAD != IRIVER_H300_PAD) && (CONFIG_KEYPAD != IAUDIO_X5_PAD)
#ifdef MENU_ENTER2
    case MENU_ENTER2:
#endif
#endif
    case MENU_ENTER:
        key = BUTTON_NONE; /* eat the downpress, next menu reacts on release */
        break;

#if (CONFIG_KEYPAD != IRIVER_H100_PAD) && (CONFIG_KEYPAD != IRIVER_H300_PAD) && (CONFIG_KEYPAD != IAUDIO_X5_PAD)
#ifdef MENU_ENTER2
    case MENU_ENTER2 | BUTTON_REL:
#endif
#endif
    case MENU_ENTER | BUTTON_REL:
        key = MENU_ENTER; /* fake downpress, next menu doesn't like release */
        break;
    }
#endif
    return key;
}

#ifndef SIMULATOR
#ifdef HAVE_RECORDING

#if defined(HAVE_FMRADIO_IN) && CONFIG_CODEC == SWCODEC
#define FM_RECORDING_SCREEN
static bool fm_recording_screen(void)
{
    bool ret;

    /* switch recording source to FMRADIO for the duration */
    int rec_source = global_settings.rec_source;
    global_settings.rec_source = AUDIO_SRC_FMRADIO;

    /* clearing queue seems to cure a spontaneous abort during record */
    while (button_get(false) != BUTTON_NONE);

    ret = recording_screen(true);

    /* safe to reset as changing sources is prohibited here */
    global_settings.rec_source = rec_source;

    return ret;
}
#endif /* defined(HAVE_FMRADIO_IN) && CONFIG_CODEC == SWCODEC */

#if defined(HAVE_FMRADIO_IN) || CONFIG_CODEC != SWCODEC
#define FM_RECORDING_SETTINGS
static bool fm_recording_settings(void)
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
#endif /* SIMULATOR */


/* main menu of the radio screen */
static bool radio_menu(void)
{
    int m;
    bool result;
    
    static const struct menu_item items[] = {
/* Add functions not accessible via buttons */
#ifndef FM_PRESET
        { ID2P(LANG_FM_BUTTONBAR_PRESETS), handle_radio_presets  },
#endif
#ifndef FM_PRESET_ADD
        { ID2P(LANG_FM_ADD_PRESET)       , radio_add_preset      },
#endif
        { ID2P(LANG_FM_PRESET_LOAD)      , load_preset_list      },
        { ID2P(LANG_FM_PRESET_SAVE)      , save_preset_list      },
        { ID2P(LANG_FM_PRESET_CLEAR)     , clear_preset_list     },

        { monomode_menu_string           , toggle_mono_mode      },
#ifndef FM_MODE
        { radiomode_menu_string          , toggle_radio_mode     },
#endif
        { region_menu_string             , toggle_region_mode    },
        { ID2P(LANG_SOUND_SETTINGS)      , sound_menu            },
#ifdef FM_RECORDING_SCREEN
        { ID2P(LANG_RECORDING_MENU)      , fm_recording_screen   },
#endif
#ifdef FM_RECORDING_SETTINGS
        { ID2P(LANG_RECORDING_SETTINGS)  , fm_recording_settings },
#endif
        { ID2P(LANG_FM_SCAN_PRESETS)     , scan_presets          },
    };

    create_monomode_menu();
    create_region_menu();
#ifndef FM_MODE
    create_radiomode_menu();
#endif
    m = menu_init(items, sizeof(items) / sizeof(*items),
                  radio_menu_cb, NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

#endif
