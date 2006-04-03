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
#include "fmradio.h"
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

#ifdef CONFIG_TUNER

#if CONFIG_CODEC == SWCODEC
#include "uda1380.h"
#include "pcm_record.h"
#endif

#if CONFIG_KEYPAD == RECORDER_PAD
#define FM_MENU BUTTON_F1
#define FM_PRESET BUTTON_F2
#define FM_RECORD BUTTON_F3
#define FM_FREEZE BUTTON_PLAY
#define FM_STOP BUTTON_OFF
#define FM_MODE (BUTTON_ON | BUTTON_REPEAT)
#define FM_EXIT_PRE BUTTON_ON
#define FM_EXIT (BUTTON_ON | BUTTON_REL)
#define FM_PRESET_ADD BUTTON_F1
#define FM_PRESET_ACTION BUTTON_F3

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
/* pause/play - short PLAY */
#define FM_PLAY_PRE BUTTON_ON
#define FM_RC_PLAY_PRE BUTTON_RC_ON
#define FM_PLAY (BUTTON_ON | BUTTON_REL)
#define FM_RC_PLAY (BUTTON_RC_ON | BUTTON_REL)
/* preset/scan mode - long PLAY */
#define FM_MODE (BUTTON_ON | BUTTON_REPEAT)
#define FM_RC_MODE (BUTTON_RC_ON | BUTTON_REPEAT)
/* preset menu - short SELECT */
#define FM_PRESET_PRE BUTTON_SELECT
#define FM_RC_PRESET_PRE BUTTON_RC_MENU
#define FM_PRESET (BUTTON_SELECT | BUTTON_REL)
#define FM_RC_PRESET (BUTTON_RC_MENU | BUTTON_REL)
/* fm menu - long SELECT */
#define FM_MENU (BUTTON_SELECT | BUTTON_REPEAT)
#define FM_RC_MENU (BUTTON_RC_MENU | BUTTON_REPEAT)
/* main menu(exit radio while playing) - A-B */
#define FM_EXIT_PRE BUTTON_MODE
#define FM_EXIT (BUTTON_MODE | BUTTON_REL)
#define FM_RC_EXIT_PRE BUTTON_RC_MODE
#define FM_RC_EXIT (BUTTON_RC_MODE | BUTTON_REL)
/* prev/next preset on the remote - BITRATE/SOURCE */
#define FM_NEXT_PRESET (BUTTON_RC_BITRATE | BUTTON_REL)
#define FM_PREV_PRESET (BUTTON_RC_SOURCE | BUTTON_REL)
/* stop and exit radio - STOP */
#define FM_STOP BUTTON_OFF
#define FM_RC_STOP BUTTON_RC_STOP

#elif CONFIG_KEYPAD == ONDIO_PAD /* restricted keypad */
#define FM_MENU (BUTTON_MENU | BUTTON_REPEAT)
#define FM_RECORD_DBLPRE BUTTON_MENU
#define FM_RECORD (BUTTON_MENU | BUTTON_REL)
#define FM_STOP_PRE BUTTON_OFF
#define FM_STOP (BUTTON_OFF | BUTTON_REL)
#define FM_EXIT (BUTTON_OFF | BUTTON_REPEAT)
#endif

#define MAX_FREQ (108000000)
#define MIN_FREQ (87500000)
#define FREQ_STEP 100000

#define RADIO_SCAN_MODE 0
#define RADIO_PRESET_MODE 1

static int curr_preset = -1;
static int curr_freq;
static int radio_mode = RADIO_SCAN_MODE;

static int radio_status = FMRADIO_OFF;

#define MAX_PRESETS 64
static bool presets_loaded = false, presets_changed = false;
static struct fmstation presets[MAX_PRESETS];

static char filepreset[MAX_PATH]; /* preset filename variable */

static int preset_menu; /* The menu index of the preset list */
static struct menu_item preset_menu_items[MAX_PRESETS];
static int num_presets = 0; /* The number of presets in the preset list */

void radio_save_presets(void);
bool handle_radio_presets(void);
bool radio_menu(void);
bool radio_add_preset(void);
bool save_preset_list(void);
bool load_preset_list(void);
bool clear_preset_list(void);

static bool scan_presets(void);

#ifdef SIMULATOR
void radio_set(int setting, int value);
int radio_get(int setting);
#else
#if CONFIG_TUNER == S1A0903X01 /* FM recorder */
#define radio_set samsung_set
#define radio_get samsung_get
#elif CONFIG_TUNER == TEA5767 /* Iriver */
#define radio_set philips_set
#define radio_get philips_get
#elif CONFIG_TUNER == (S1A0903X01 | TEA5767) /* OndioFM */
void (*radio_set)(int setting, int value);
int (*radio_get)(int setting);
#endif
#endif

/* Function to manipulate all yesno dialogues.
   This function needs the output text as an argument. */
bool yesno_pop(char* text)
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

void radio_stop(void)
{             
    radio_set(RADIO_MUTE, 1);
    radio_set(RADIO_SLEEP, 1); /* low power mode, if available */
    radio_status = FMRADIO_OFF;
    radio_power(false); /* status update, power off if avail. */
}

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
    int min_diff = MAX_FREQ;
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
    global_settings.last_frequency = (curr_freq - MIN_FREQ) / FREQ_STEP;
    settings_save();
}

void next_preset(int direction)
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
#endif
    bool keep_playing = false;
    bool statusbar = global_settings.statusbar;
    int mute_timeout = current_tick;
    int button_timeout = current_tick + (2*HZ);
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
#endif
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
#if CONFIG_CODEC != SWCODEC
    if(rec_create_directory() > 0)
        have_recorded = true;
#endif

    if(radio_status == FMRADIO_PLAYING_OUT)
        radio_status = FMRADIO_PLAYING;
    else if(radio_status == FMRADIO_PAUSED_OUT)
        radio_status = FMRADIO_PAUSED;

    if(radio_status == FMRADIO_OFF)    
        audio_stop();
        
#if CONFIG_CODEC != SWCODEC
    audio_init_recording();

    sound_settings_apply();
    /* Yes, we use the D/A for monitoring */
    peak_meter_playback(true);
    
    peak_meter_enabled = true;

    if (global_settings.rec_prerecord_time)
        talk_buffer_steal(); /* will use the mp3 buffer */

    audio_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               1, /* Line In */
                               global_settings.rec_channels,
                               global_settings.rec_editable,
                               global_settings.rec_prerecord_time);

    
    audio_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
                            sound_default(SOUND_RIGHT_GAIN), AUDIO_GAIN_LINEIN);
#else
    peak_meter_enabled = false;
    uda1380_enable_recording(false);
    uda1380_set_recvol(10, 10, AUDIO_GAIN_DECIMATOR);
    uda1380_set_recvol(0, 0, AUDIO_GAIN_LINEIN);
    uda1380_set_monitor(true);

    /* Set the input multiplexer to FM */
    pcm_rec_mux(1);
#endif
#endif

    curr_freq = global_settings.last_frequency * FREQ_STEP + MIN_FREQ;
    
    if(radio_status == FMRADIO_OFF)
    {
        radio_power(true);
        radio_set(RADIO_SLEEP, 0); /* wake up the tuner */
        radio_set(RADIO_FREQUENCY, curr_freq);
        radio_set(RADIO_IF_MEASUREMENT, 0);
        radio_set(RADIO_SENSITIVITY, 0);
        radio_set(RADIO_FORCE_MONO, global_settings.fm_force_mono);
        mute_timeout = current_tick + (1*HZ);
        while( !radio_get(RADIO_STEREO)
             &&!radio_get(RADIO_TUNED) )
        {
            if(TIME_AFTER(current_tick, mute_timeout))
                 break;
            yield();
        }
        radio_set(RADIO_MUTE, 0);
        radio_status = FMRADIO_PLAYING;
    }

   if(num_presets == 0 && yesno_pop(str(LANG_FM_FIRST_AUTOSCAN)))
        scan_presets();
    
    curr_preset = find_preset(curr_freq);
    if(curr_preset != -1)
         radio_mode = RADIO_PRESET_MODE;

#ifdef HAS_BUTTONBAR
    gui_buttonbar_set(&buttonbar, str(LANG_BUTTONBAR_MENU), str(LANG_FM_BUTTONBAR_PRESETS),
                  str(LANG_FM_BUTTONBAR_RECORD));
#endif

    cpu_idle_mode(true);
        
    while(!done)
    {
        if(search_dir)
        {
            curr_freq += search_dir * FREQ_STEP;
            if(curr_freq < MIN_FREQ)
                curr_freq = MAX_FREQ;
            if(curr_freq > MAX_FREQ)
                curr_freq = MIN_FREQ;

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
            button = button_get_w_tmo(HZ / PEAK_METER_FPS);
        if (button != BUTTON_NONE)
        {
            cpu_idle_mode(false);
            button_timeout = current_tick + (2*HZ);
        }
        switch(button)
        {
#ifdef FM_RC_STOP
            case FM_RC_STOP:
#endif
            case FM_STOP:
#ifdef FM_STOP_PRE
                if (lastbutton != FM_STOP_PRE)
                    break;
#endif
#ifndef SIMULATOR
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
            case FM_RECORD:
#ifdef FM_RECORD_DBLPRE
                if (lastbutton != FM_RECORD_DBLPRE)
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
                    audio_new_file(rec_create_filename(buf));
                    update_screen = true;
                }
                else
                {
                    have_recorded = true;
                    talk_buffer_steal(); /* we use the mp3 buffer */
                    audio_record(rec_create_filename(buf));
                    update_screen = true;
                }
#endif
                last_seconds = 0;
                break;
#endif /* #ifdef FM_RECORD */

#ifdef FM_RC_EXIT
            case FM_RC_EXIT:
#endif
            case FM_EXIT:
#ifdef FM_EXIT_PRE
                if(lastbutton != FM_EXIT_PRE
#ifdef FM_RC_EXIT_PRE
                   && lastbutton != FM_RC_EXIT_PRE
#endif                    
                    )
                    break;
#endif
#ifndef SIMULATOR
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
                
#ifdef BUTTON_RC_REW
            case BUTTON_RC_REW:
#endif
            case BUTTON_LEFT:
                if(radio_mode == RADIO_SCAN_MODE)
                {
                     curr_freq -= FREQ_STEP;
                     if(curr_freq < MIN_FREQ)
                          curr_freq = MAX_FREQ;
                     radio_set(RADIO_FREQUENCY, curr_freq);
                     curr_preset = find_preset(curr_freq);
                     remember_frequency();
                }
                else
                     next_preset(-1);
                search_dir = 0;
                update_screen = true;
                break;

#ifdef BUTTON_RC_FF
            case BUTTON_RC_FF:
#endif
            case BUTTON_RIGHT:
                if(radio_mode == RADIO_SCAN_MODE)
                {
                     curr_freq += FREQ_STEP;
                     if(curr_freq > MAX_FREQ)
                           curr_freq = MIN_FREQ;
                     radio_set(RADIO_FREQUENCY, curr_freq);
                     curr_preset = find_preset(curr_freq);
                     remember_frequency();
                }
                else
                     next_preset(1);
                search_dir = 0;
                update_screen = true;
                break;

#ifdef BUTTON_RC_REW
            case BUTTON_RC_REW | BUTTON_REPEAT:
#endif
            case BUTTON_LEFT | BUTTON_REPEAT:
                if(radio_mode == RADIO_SCAN_MODE)
                    search_dir = -1;
                else
                {
                    next_preset(-1);
                    update_screen = true;
                }

                break;
                
#ifdef BUTTON_RC_FF
            case BUTTON_RC_FF | BUTTON_REPEAT:
#endif
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if(radio_mode == RADIO_SCAN_MODE)
                    search_dir = 1;
                else
                {
                    next_preset(1);
                    update_screen = true;
                }
                
                break;

#ifdef BUTTON_RC_VOL_UP
            case BUTTON_RC_VOL_UP:
            case BUTTON_RC_VOL_UP | BUTTON_REPEAT:
#endif
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                global_settings.volume++;
                if(global_settings.volume > sound_max(SOUND_VOLUME))
                    global_settings.volume = sound_max(SOUND_VOLUME);
                sound_set_volume(global_settings.volume);
                update_screen = true;
                settings_save();
                break;

#ifdef BUTTON_RC_VOL_DOWN
            case BUTTON_RC_VOL_DOWN:
            case BUTTON_RC_VOL_DOWN | BUTTON_REPEAT:
#endif
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                global_settings.volume--;
                if(global_settings.volume < sound_min(SOUND_VOLUME))
                    global_settings.volume = sound_min(SOUND_VOLUME);
                sound_set_volume(global_settings.volume);
                update_screen = true;
                settings_save();
                break;

#ifdef FM_PLAY
#ifdef FM_RC_PLAY
            case FM_RC_PLAY:
#endif
            case FM_PLAY:
#ifdef FM_PLAY_PRE
                if(lastbutton != FM_PLAY_PRE
#ifdef FM_RC_PLAY_PRE
                    && lastbutton != FM_RC_PLAY_PRE
#endif
                  )
                     break;
#endif
                if(radio_status != FMRADIO_PLAYING)
                {
                     radio_set(RADIO_SLEEP, 0);
                     radio_set(RADIO_FREQUENCY, curr_freq);
                     mute_timeout = current_tick + (2*HZ);
                     while( !radio_get(RADIO_STEREO)
                          &&!radio_get(RADIO_TUNED) )
                     {
                         if(TIME_AFTER(current_tick, mute_timeout))
                             break;
                         yield();
                     }
                     radio_set(RADIO_MUTE, 0);
                     radio_status = FMRADIO_PLAYING;
                }
                else
                {
                     radio_set(RADIO_MUTE, 1);
                     radio_set(RADIO_SLEEP, 1);
                     radio_status = FMRADIO_PAUSED;
                }
                update_screen = true;
                break;
#endif
#ifdef FM_MENU
#ifdef FM_RC_MENU
            case FM_RC_MENU:
#endif
            case FM_MENU:
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
#endif

#ifdef FM_RC_PRESET
            case FM_RC_PRESET:
#endif                    
#ifdef FM_PRESET
            case FM_PRESET:
#ifdef FM_PRESET_PRE
                if(lastbutton != FM_PRESET_PRE
#ifdef FM_RC_PRESET_PRE
                    && lastbutton != FM_RC_PRESET_PRE
#endif
                  )
                     break;
#endif
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
            case FM_FREEZE:
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

#ifdef FM_RC_MODE
           case FM_RC_MODE:
#endif
#ifdef FM_MODE
           case FM_MODE:
                if(lastbutton != FM_MODE 
#ifdef FM_RC_MODE
                   && lastbutton != FM_RC_MODE
#endif                    
                   )
                {
                    if(radio_mode == RADIO_SCAN_MODE)
                    {
                        /* Force scan mode if there are no presets. */
                        if(num_presets > 0)
                            radio_mode = RADIO_PRESET_MODE;
                    }
                    else
                        radio_mode = RADIO_SCAN_MODE;
                    update_screen = true;
                }
                break;
#endif
#ifdef FM_NEXT_PRESET
            case FM_NEXT_PRESET:
                next_preset(1);
                search_dir = 0;
                update_screen = true;
                break;
#endif
#ifdef FM_PREV_PRESET
            case FM_PREV_PRESET:
                next_preset(-1);
                search_dir = 0;
                update_screen = true;
                break;
#endif
                
            default:
                default_event_handler(button);
                break;
        } /*switch(button)*/

        if (button != BUTTON_NONE)
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
                
                stereo = radio_get(RADIO_STEREO) &&
                    !global_settings.fm_force_mono;
                if(stereo != last_stereo_status)
                {
                    update_screen = true;
                    last_stereo_status = stereo;
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
                
                freq = curr_freq / 100000;
                snprintf(buf, 128, str(LANG_FM_STATION), freq / 10, freq % 10);
                FOR_NB_SCREENS(i)
                    screens[i].puts_scroll(0, top_of_screen + 1, buf);
                
                strcat(buf, stereo?str(LANG_CHANNEL_STEREO):
                                   str(LANG_CHANNEL_MONO));

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
                    if(global_settings.rec_prerecord_time)
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
            button = button_get(true);
            if(button == (BUTTON_OFF | BUTTON_REL))
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
/* Catch FMRADIO_PLAYING_OUT status for the sim. */ 
#ifndef SIMULATOR
#if CONFIG_CODEC != SWCODEC
        /* Enable the Left and right A/D Converter */
        audio_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
                                sound_default(SOUND_RIGHT_GAIN), AUDIO_GAIN_LINEIN);
        mas_codec_writereg(6, 0x4000);
#endif
#endif
        if(radio_status == FMRADIO_PAUSED)
            radio_status = FMRADIO_PAUSED_OUT;
        else
            radio_status = FMRADIO_PLAYING_OUT;

    }
    else
    {
        radio_stop();
#ifndef SIMULATOR /* SIMULATOR. Catch FMRADIO_OFF status for the sim. */ 
#if CONFIG_CODEC == SWCODEC
        pcm_rec_mux(0); /* Line In */
        peak_meter_enabled = true;
#endif
#endif /* SIMULATOR */
    }
    
    cpu_idle_mode(false);

    /* restore status bar settings */
    global_settings.statusbar = statusbar;
    
    return have_recorded;
}

void radio_save_presets(void)
{
    int fd;
    int i;
          
    fd = creat(filepreset, O_WRONLY);
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

bool radio_add_preset(void)
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
        case FM_PRESET_ACTION:
            key = MENU_EXIT; /* Fake an exit */
            break;
            
        case FM_PRESET_ACTION | BUTTON_REL:
            /* Ignore the release events */
            key = BUTTON_NONE;
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

bool radio_delete_preset(void)
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

bool load_preset_list(void)
{
    return !rockbox_browse(FMPRESET_PATH, SHOW_FMR);
}

bool save_preset_list(void)
{   
    if(num_presets != 0)
    { 
        if(!opendir(FMPRESET_PATH)) /* Check if there is preset folder */
            mkdir(FMPRESET_PATH, 0); 
    
        create_numbered_filename(filepreset, FMPRESET_PATH, "preset", ".fmr", 2);
    
        if (!kbd_input(filepreset, sizeof(filepreset)))
           radio_save_presets();
    }
    else
        gui_syncsplash(HZ,true,str(LANG_FM_NO_PRESETS));
        
    return true;
}

bool clear_preset_list(void)
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
bool handle_radio_presets_menu(void)
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
int handle_radio_presets_cb(int key, int m)
{
    (void)m;
    switch(key)
    {
#ifdef FM_PRESET_ADD
        case FM_PRESET_ADD:
            radio_add_preset();
            menu_draw(m);
            key = BUTTON_NONE;
            break;
#endif
#if (CONFIG_KEYPAD != IRIVER_H100_PAD) && (CONFIG_KEYPAD != IRIVER_H300_PAD)
#ifdef FM_PRESET
        case FM_PRESET:
            menu_draw(m);
            key = MENU_EXIT; /* Fake an exit */
            break;
#endif
#endif
#ifdef FM_PRESET_ACTION
        case FM_PRESET_ACTION:
#endif
#ifdef MENU_RC_ENTER
        case MENU_RC_ENTER | BUTTON_REPEAT:
#endif
#ifdef MENU_RC_ENTER2
        case MENU_RC_ENTER2 | BUTTON_REPEAT:
#endif
#ifdef MENU_ENTER2
        case MENU_ENTER2 | BUTTON_REPEAT:
#endif
        case MENU_ENTER | BUTTON_REPEAT: /* long gives options */
        {
            bool ret;
            ret = handle_radio_presets_menu();
            menu_draw(m);
            if(ret)
                key = MENU_ATTACHED_USB;
            else
                key = BUTTON_NONE;
            break;
        }
#ifdef MENU_RC_ENTER
        case MENU_RC_ENTER | BUTTON_REL:
#endif
#ifdef MENU_RC_ENTER2
        case MENU_RC_ENTER2 | BUTTON_REL:
#endif
#ifdef MENU_ENTER2
        case MENU_ENTER2 | BUTTON_REL:
#endif
        case MENU_ENTER | BUTTON_REL:
            key = MENU_ENTER; /* fake enter for short press */
            break;
            
/* ignore down events */
#ifdef MENU_RC_ENTER
        case MENU_RC_ENTER:
#endif
#ifdef MENU_RC_ENTER2
        case MENU_RC_ENTER2:
#endif

#ifdef MENU_ENTER2
        case MENU_ENTER2:
#endif
        case MENU_ENTER: 
            /* Ignore the release events */
#ifdef FM_PRESET_ADD
        case FM_PRESET_ADD | BUTTON_REL:
#endif
#ifdef FM_PRESET_ACTION
        case FM_PRESET_ACTION | BUTTON_REL:
#endif
            key = BUTTON_NONE;
            break;
    }
    return key;
}

/* present a list of preset stations */
bool handle_radio_presets(void)
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

#ifndef SIMULATOR
#if CONFIG_CODEC != SWCODEC
static bool fm_recording_settings(void)
{
    bool ret;
    
    ret = recording_menu(true);
    if(!ret)
    {
        if (global_settings.rec_prerecord_time)
            talk_buffer_steal(); /* will use the mp3 buffer */

        audio_set_recording_options(global_settings.rec_frequency,
                                   global_settings.rec_quality,
                                   1, /* Line In */
                                   global_settings.rec_channels,
                                   global_settings.rec_editable,
                                   global_settings.rec_prerecord_time);
    }
    return ret;
}
#endif
#endif

char monomode_menu_string[32];

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

#ifndef FM_MODE
char radiomode_menu_string[32];

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
        curr_freq = MIN_FREQ;
        num_presets = 0;
        memset(presets, 0, sizeof(presets));
        while(curr_freq <= MAX_FREQ)
        {
            if (num_presets >= MAX_PRESETS)
                break;

            freq = curr_freq /100000;
            snprintf(buf, MAX_FMPRESET_LEN, str(LANG_FM_SCANNING), 
                            freq/10, freq % 10);
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
                    str(LANG_FM_DEFAULT_PRESET_NAME),freq/10, freq % 10);
                 strcpy(presets[num_presets].name,buf);
                 presets[num_presets].frequency = curr_freq;
                 num_presets++;
            }

            curr_freq += FREQ_STEP;
                   
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
int radio_menu_cb(int key, int m)
{
    (void)m;
    switch(key)
    {
#if (CONFIG_KEYPAD != IRIVER_H100_PAD) && (CONFIG_KEYPAD != IRIVER_H300_PAD)
#ifdef MENU_ENTER2
    case MENU_ENTER2:
#endif
#endif
    case MENU_ENTER:
        key = BUTTON_NONE; /* eat the downpress, next menu reacts on release */
        break;

#if (CONFIG_KEYPAD != IRIVER_H100_PAD) && (CONFIG_KEYPAD != IRIVER_H300_PAD)
#ifdef MENU_ENTER2
    case MENU_ENTER2 | BUTTON_REL:
#endif
#endif
    case MENU_ENTER | BUTTON_REL:
        key = MENU_ENTER; /* fake downpress, next menu doesn't like release */
        break;
    }

    return key;
}

/* main menu of the radio screen */
bool radio_menu(void)
{
    int m;
    bool result;
    
    static const struct menu_item items[] = {
/* Add functions not accessible via buttons */
#ifndef FM_PRESET
        { ID2P(LANG_FM_BUTTONBAR_PRESETS), handle_radio_presets },
#endif
#ifndef FM_PRESET_ADD
        { ID2P(LANG_FM_ADD_PRESET)       , radio_add_preset     },
#endif
        { ID2P(LANG_FM_PRESET_LOAD)      , load_preset_list     },
        { ID2P(LANG_FM_PRESET_SAVE)      , save_preset_list     },
        { ID2P(LANG_FM_PRESET_CLEAR)     , clear_preset_list    },

        { monomode_menu_string           , toggle_mono_mode     },
#ifndef FM_MODE
        { radiomode_menu_string          , toggle_radio_mode    },
#endif
        { ID2P(LANG_SOUND_SETTINGS)      , sound_menu           },
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
        { ID2P(LANG_RECORDING_SETTINGS)  , fm_recording_settings},
#endif
        { ID2P(LANG_FM_SCAN_PRESETS)     , scan_presets         },
    };

    create_monomode_menu();
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
