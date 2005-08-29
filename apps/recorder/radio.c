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
#include "lcd.h"
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
#define FM_EXIT (BUTTON_ON | BUTTON_REL)
#define FM_PRESET_ADD BUTTON_F1
#define FM_PRESET_ACTION BUTTON_F3
#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define FM_MENU BUTTON_MODE
#define FM_STOP BUTTON_OFF
#define FM_EXIT_PRE BUTTON_SELECT
#define FM_EXIT (BUTTON_SELECT | BUTTON_REL)
#elif CONFIG_KEYPAD == ONDIO_PAD /* restricted keypad */
#define FM_MENU (BUTTON_MENU | BUTTON_REPEAT)
#define FM_RECORD (BUTTON_MENU | BUTTON_REL)
#define FM_STOP (BUTTON_OFF | BUTTON_REL)
#define FM_EXIT (BUTTON_OFF | BUTTON_REPEAT)
#endif

#define MAX_FREQ (108000000)
#define MIN_FREQ (87500000)
#define FREQ_STEP 100000

static int curr_preset = -1;
static int curr_freq;

#define MAX_PRESETS 32
static bool presets_loaded = false;
static struct fmstation presets[MAX_PRESETS];

static const char default_filename[] = "/.rockbox/fm-presets-default.fmr";

static int preset_menu; /* The menu index of the preset list */
static struct menu_item preset_menu_items[MAX_PRESETS];
static int num_presets; /* The number of presets in the preset list */

void radio_load_presets(void);
bool handle_radio_presets(void);
bool radio_menu(void);
bool radio_add_preset(void);

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

void radio_stop(void)
{
    radio_set(RADIO_MUTE, 1);
    radio_set(RADIO_SLEEP, 1); /* low power mode, if available */
    radio_set_status(FMRADIO_OFF); /* status update, power off if avail. */
}

bool radio_hardware_present(void)
{
#ifdef HAVE_TUNER_PWR_CTRL
    bool ret;
    int fmstatus = radio_get_status(); /* get current state */
    radio_set_status(FMRADIO_POWERED); /* power it up */
    ret = radio_get(RADIO_PRESENT);
    radio_set_status(fmstatus); /* restore previous state */
    return ret;
#else
    return radio_get(RADIO_PRESENT);
#endif
}

static int find_preset(int freq)
{
    int i;
    for(i = 0;i < MAX_PRESETS;i++)
    {
        if(freq == presets[i].frequency)
            return i;
    }

    return -1;
}

static void remember_frequency(void)
{
    global_settings.last_frequency = (curr_freq - MIN_FREQ) / FREQ_STEP;
    settings_save();
}

bool radio_screen(void)
{
    char buf[MAX_PATH];
    bool done = false;
    int button, lastbutton = BUTTON_NONE;
    int freq;
    bool tuned;
    bool stereo = false;
    int search_dir = 0;
    int fw, fh;
    bool last_stereo_status = false;
    int top_of_screen = 0;
    bool update_screen = true;
    int timeout = current_tick + HZ/10;
    bool screen_freeze = false;
    bool have_recorded = false;
    unsigned int seconds = 0;
    unsigned int last_seconds = 0;
    int hours, minutes;
    bool keep_playing = false;

    lcd_clear_display();
    lcd_setmargins(0, 8);
    status_draw(true);
    radio_set_status(FMRADIO_PLAYING);

    font_get(FONT_UI);
    lcd_getstringsize("M", &fw, &fh);

    /* Adjust for font size, trying to center the information vertically */
    if(fh < 10)
        top_of_screen = 1;
    
    radio_load_presets();

#ifndef SIMULATOR
#if CONFIG_CODEC != SWCODEC
    if(rec_create_directory() > 0)
        have_recorded = true;
#endif
    
    audio_stop();

#if CONFIG_CODEC != SWCODEC
    mpeg_init_recording();

    sound_settings_apply();

    /* Yes, we use the D/A for monitoring */
    peak_meter_playback(true);
    
    peak_meter_enabled = true;

    if (global_settings.rec_prerecord_time)
        talk_buffer_steal(); /* will use the mp3 buffer */

    mpeg_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               1, /* Line In */
                               global_settings.rec_channels,
                               global_settings.rec_editable,
                               global_settings.rec_prerecord_time);

    
    mpeg_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
                            sound_default(SOUND_RIGHT_GAIN), false);
#else
    uda1380_enable_recording(false);
    uda1380_set_recvol(0, 0, 10);
    uda1380_set_monitor(true);

    /* Set the input multiplexer to FM */
    pcmrec_set_mux(1);
#endif
#endif

    curr_freq = global_settings.last_frequency * FREQ_STEP + MIN_FREQ;
    
    radio_set(RADIO_SLEEP, 0); /* wake up the tuner */
    radio_set(RADIO_FREQUENCY, curr_freq);
    radio_set(RADIO_IF_MEASUREMENT, 0);
    radio_set(RADIO_SENSITIVITY, 0);
    radio_set(RADIO_FORCE_MONO, global_settings.fm_force_mono);
    radio_set(RADIO_MUTE, 0);
    
    curr_preset = find_preset(curr_freq);

#if CONFIG_KEYPAD == RECORDER_PAD
    buttonbar_set(str(LANG_BUTTONBAR_MENU), str(LANG_FM_BUTTONBAR_PRESETS),
                  str(LANG_FM_BUTTONBAR_RECORD));
#endif

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
        switch(button)
        {
            case FM_STOP:
#ifndef SIMULATOR
                if(audio_status() == AUDIO_STATUS_RECORD)
                {
                    audio_stop();
                }
                else
#endif
                {
                    done = true;
                }
                update_screen = true;
                break;

#ifdef FM_RECORD
            case FM_RECORD:
#ifndef SIMULATOR
                if(audio_status() == AUDIO_STATUS_RECORD)
                {
                    mpeg_new_file(rec_create_filename(buf));
                    update_screen = true;
                }
                else
                {
                    have_recorded = true;
                    talk_buffer_steal(); /* we use the mp3 buffer */
                    mpeg_record(rec_create_filename(buf));
                    update_screen = true;
                }
#endif
                last_seconds = 0;
                break;
#endif /* #ifdef FM_RECORD */

            case FM_EXIT:
#ifdef FM_EXIT_PRE
                if(lastbutton != FM_EXIT_PRE)
                    break;
#endif
#ifndef SIMULATOR
                if(audio_status() == AUDIO_STATUS_RECORD)
                    audio_stop();
#endif
                done = true;
                keep_playing = true;
                break;
                
            case BUTTON_LEFT:
                curr_freq -= FREQ_STEP;
                if(curr_freq < MIN_FREQ)
                    curr_freq = MAX_FREQ;

                radio_set(RADIO_FREQUENCY, curr_freq);
                curr_preset = find_preset(curr_freq);
                remember_frequency();
                search_dir = 0;
                update_screen = true;
                break;

            case BUTTON_RIGHT:
                curr_freq += FREQ_STEP;
                if(curr_freq > MAX_FREQ)
                    curr_freq = MIN_FREQ;
                
                radio_set(RADIO_FREQUENCY, curr_freq);
                curr_preset = find_preset(curr_freq);
                remember_frequency();
                search_dir = 0;
                update_screen = true;
                break;

            case BUTTON_LEFT | BUTTON_REPEAT:
                search_dir = -1;
                break;
                
            case BUTTON_RIGHT | BUTTON_REPEAT:
                search_dir = 1;
                break;

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                global_settings.volume++;
                if(global_settings.volume > sound_max(SOUND_VOLUME))
                    global_settings.volume = sound_max(SOUND_VOLUME);
                sound_set(SOUND_VOLUME, global_settings.volume);
                update_screen = true;
                settings_save();
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                global_settings.volume--;
                if(global_settings.volume < sound_min(SOUND_VOLUME))
                    global_settings.volume = sound_min(SOUND_VOLUME);
                sound_set(SOUND_VOLUME, global_settings.volume);
                update_screen = true;
                settings_save();
                break;

#ifdef FM_MENU
            case FM_MENU:
                radio_menu();
                curr_preset = find_preset(curr_freq);
                lcd_clear_display();
                lcd_setmargins(0, 8);
#if CONFIG_KEYPAD == RECORDER_PAD
                buttonbar_set(str(LANG_BUTTONBAR_MENU),
                              str(LANG_FM_BUTTONBAR_PRESETS),
                              str(LANG_FM_BUTTONBAR_RECORD));
#endif
                update_screen = true;
                break;
#endif
                
#ifdef FM_PRESET
            case FM_PRESET:
                handle_radio_presets();
                curr_preset = find_preset(curr_freq);
                lcd_clear_display();
                lcd_setmargins(0, 8);
#if CONFIG_KEYPAD == RECORDER_PAD
                buttonbar_set(str(LANG_BUTTONBAR_MENU),
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
                    splash(0, true, "Screen frozen");
                    lcd_update();
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
                
            default:
                default_event_handler(button);
                break;
        }

        if (button != BUTTON_NONE)
            lastbutton = button;
        
        peak_meter_peek();

        if(!screen_freeze)
        {
            lcd_setmargins(0, 8);
            
            /* Only display the peak meter when not recording */
            if(!audio_status())
            {
                peak_meter_draw(0, 8 + fh*(top_of_screen + 3), LCD_WIDTH, fh);
                lcd_update_rect(0, 8 + fh*(top_of_screen + 3), LCD_WIDTH, fh);
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
            seconds = mpeg_recorded_time() / HZ;
#endif
#endif
            if(update_screen || seconds > last_seconds)
            {
                last_seconds = seconds;
                
                lcd_setfont(FONT_UI);
                
                if(curr_preset >= 0)
                {
                    lcd_puts_scroll(0, top_of_screen,
                                    presets[curr_preset].name);
                }
                else
                {
                    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    lcd_fillrect(0, 8 + top_of_screen*fh, LCD_WIDTH, fh);
                    lcd_set_drawmode(DRMODE_SOLID);
                }
                
                freq = curr_freq / 100000;
                snprintf(buf, 128, str(LANG_FM_STATION), freq / 10, freq % 10);
                lcd_puts(0, top_of_screen + 1, buf);
                
                snprintf(buf, 128,
                         stereo?str(LANG_CHANNEL_STEREO):
                         str(LANG_CHANNEL_MONO));
                lcd_puts(0, top_of_screen + 2, buf);

                if(audio_status() == AUDIO_STATUS_RECORD)
                {
                    hours = seconds / 3600;
                    minutes = (seconds - (hours * 3600)) / 60;
                    snprintf(buf, 32, "%s %02d:%02d:%02d",
                             str(LANG_RECORDING_TIME),
                             hours, minutes, seconds%60);
                    lcd_puts(0, top_of_screen + 3, buf);
                }
                else
                {
                    if(global_settings.rec_prerecord_time)
                    {
                        snprintf(buf, 32, "%s %02d",
                                 str(LANG_RECORD_PRERECORD), seconds%60);
                        lcd_puts(0, top_of_screen + 3, buf);
                    }
                }
                
#if CONFIG_KEYPAD == RECORDER_PAD
                buttonbar_draw();
#endif                
                lcd_update();
                
                update_screen = false;
            }
            /* Only force the redraw if update_screen is true */
            status_draw(update_screen);
        }

        if(audio_status() & AUDIO_STATUS_ERROR)
        {
            done = true;
        }
    }

#ifndef SIMULATOR
    if(audio_status() & AUDIO_STATUS_ERROR)
    {
        splash(0, true, str(LANG_DISK_FULL));
        status_draw(true);
        lcd_update();
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

    if(keep_playing)
    {
#if CONFIG_CODEC != SWCODEC
        /* Enable the Left and right A/D Converter */
        mpeg_set_recording_gain(sound_default(SOUND_LEFT_GAIN),
                                sound_default(SOUND_RIGHT_GAIN), false);
        mas_codec_writereg(6, 0x4000);
#endif
        radio_set_status(FMRADIO_POWERED); /* leave it powered */
    }
    else
    {
        radio_stop();
#if CONFIG_CODEC == SWCODEC
        pcmrec_set_mux(0); /* Line In */
#endif
    }

#endif
    return have_recorded;
}

void radio_save_presets(void)
{
    int fd;
    int i;
    
    fd = creat(default_filename, O_WRONLY);
    if(fd >= 0)
    {
        for(i = 0;i < num_presets;i++)
        {
            fdprintf(fd, "%d:%s\n", presets[i].frequency, presets[i].name);
        }
        close(fd);
    }
    else
    {
        splash(HZ*2, true, str(LANG_FM_PRESET_SAVE_FAILED));
    }
}

void radio_load_presets(void)
{
    int fd;
    int rc;
    char buf[128];
    char *freq;
    char *name;
    bool done = false;
    int f;

    if(!presets_loaded)
    {
        memset(presets, 0, sizeof(presets));
        num_presets = 0;
    
        fd = open(default_filename, O_RDONLY);
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
                            strncpy(presets[num_presets].name, name, 27);
                            presets[num_presets].name[27] = 0;
                            num_presets++;
                        }
                    }
                }
                else
                    done = true;
            }
            close(fd);
        }
    }
    presets_loaded = true;
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
    char buf[27];

    if(num_presets < MAX_PRESETS)
    {
        memset(buf, 0, 27);
        
        if (!kbd_input(buf, 27))
        {
            buf[27] = 0;
            strcpy(presets[num_presets].name, buf);
            presets[num_presets].frequency = curr_freq;
            menu_insert(preset_menu, -1,
                        presets[num_presets].name, 0);
            /* We must still rebuild the menu table, since the
               item name pointers must be updated */
            rebuild_preset_menu();
            num_presets++;
            radio_save_presets();
        }
    }
    else
    {
        splash(HZ*2, true, str(LANG_FM_NO_FREE_PRESETS));
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
    char buf[27];

    strncpy(buf, menu_description(preset_menu, pos), 27);
        
    if (!kbd_input(buf, 27))
    {
        buf[27] = 0;
        strcpy(presets[pos].name, buf);
        radio_save_presets();
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
    radio_save_presets();

    return true; /* Make the menu return immediately */
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
#ifdef FM_PRESET
        case FM_PRESET:
            menu_draw(m);
            key = MENU_EXIT; /* Fake an exit */
            break;
#endif
#ifdef FM_PRESET_ACTION
        case FM_PRESET_ACTION:
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
#ifdef MENU_ENTER2
        case MENU_ENTER2 | BUTTON_REL:
#endif
        case MENU_ENTER | BUTTON_REL:
            key = MENU_ENTER; /* fake enter for short press */
            break;

#ifdef MENU_ENTER2
        case MENU_ENTER2:
#endif
        case MENU_ENTER: /* ignore down event */
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

        mpeg_set_recording_options(global_settings.rec_frequency,
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
    snprintf(monomode_menu_string, 32, "%s: %s", str(LANG_FM_MONO_MODE),
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


/* button preprocessor for the main menu */
int radio_menu_cb(int key, int m)
{
    (void)m;
    switch(key)
    {
#ifdef MENU_ENTER2
    case MENU_ENTER2:
#endif
    case MENU_ENTER:
        key = BUTTON_NONE; /* eat the downpress, next menu reacts on release */
        break;

#ifdef MENU_ENTER2
    case MENU_ENTER2 | BUTTON_REL:
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
#if CONFIG_KEYPAD == ONDIO_PAD /* Ondio has no key for presets, put it in menu */
        { ID2P(LANG_FM_BUTTONBAR_PRESETS), handle_radio_presets },
        { ID2P(LANG_FM_BUTTONBAR_ADD)    , radio_add_preset     },
#endif
        { monomode_menu_string           , toggle_mono_mode     },
        { ID2P(LANG_SOUND_SETTINGS)      , sound_menu           },
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
        { ID2P(LANG_RECORDING_SETTINGS)  , fm_recording_settings},
#endif
    };

    create_monomode_menu();

    m = menu_init(items, sizeof(items) / sizeof(*items),
                  radio_menu_cb, NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

#endif
