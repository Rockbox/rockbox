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

#ifdef HAVE_FMRADIO

#define MAX_FREQ (108000000)
#define MIN_FREQ (87500000)
#define PLL_FREQ_STEP 10000
#define FREQ_STEP 100000

#define RADIO_FREQUENCY 0
#define RADIO_MUTE 1
#define RADIO_IF_MEASUREMENT 2
#define RADIO_SENSITIVITY 3
#define RADIO_FORCE_MONO 4

#define DEFAULT_IN1 0x100003 /* Mute */
#define DEFAULT_IN2 0x140884 /* 5kHz, 7.2MHz crystal */

static int fm_in1 = DEFAULT_IN1;
static int fm_in2 = DEFAULT_IN2;

static int curr_preset = -1;
static int curr_freq;
static int pll_cnt;

#define MAX_PRESETS 32
static bool presets_loaded = false;
static struct fmstation presets[MAX_PRESETS];

static const char default_filename[] = "/.rockbox/fm-presets-default.fmr";

int debug_fm_detection;

static int preset_menu; /* The menu index of the preset list */
static struct menu_item preset_menu_items[MAX_PRESETS];
static int num_presets; /* The number of presets in the preset list */

void radio_load_presets(void);
bool handle_radio_presets(void);
bool radio_menu(void);

void radio_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_FREQUENCY:
            /* We add the standard Intermediate Frequency 10.7MHz
            ** before calculating the divisor
            ** The reference frequency is set to 50kHz, and the VCO
            ** output is prescaled by 2.
            */
    
            pll_cnt = (value + 10700000) / (PLL_FREQ_STEP/2) / 2;

            /* 0x100000 == FM mode
            ** 0x000002 == Microprocessor controlled Mute
            */
            fm_in1 = (fm_in1 & 0xfff00007) | (pll_cnt << 3);
            fmradio_set(1, fm_in1);
            break;

        case RADIO_MUTE:
            fm_in1 = (fm_in1 & 0xfffffffe) | (value?1:0);
            fmradio_set(1, fm_in1);
            break;

        case RADIO_IF_MEASUREMENT:
            fm_in1 = (fm_in1 & 0xfffffffb) | (value?4:0);
            fmradio_set(1, fm_in1);
            break;

        case RADIO_SENSITIVITY:
            fm_in2 = (fm_in2 & 0xffff9fff) | ((value & 3) << 13);
            fmradio_set(2, fm_in2);
            break;

        case RADIO_FORCE_MONO:
            fm_in2 = (fm_in2 & 0xfffffffb) | (value?0:4);
            fmradio_set(2, fm_in2);
            break;
    }
}

void radio_stop(void)
{
    radio_set(RADIO_MUTE, 1);
}

bool radio_hardware_present(void)
{
    int val;
    
    fmradio_set(2, 0x140885); /* 5kHz, 7.2MHz crystal, test mode 1 */
    val = fmradio_read(0);
    debug_fm_detection = val;
    if(val == 0x140885)
        return true;
    else
        return false;
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
    int button;
    int val;
    int freq;
    int i_freq;
    bool stereo = false;
    int search_dir = 0;
    int fw, fh;
    bool last_stereo_status = false;
    int top_of_screen = 0;
    bool update_screen = true;
    int timeout = current_tick + HZ/10;
    bool screen_freeze = false;
    bool have_recorded = false;
    unsigned int seconds;
    unsigned int last_seconds = 0;
    int hours, minutes;
    bool keep_playing = false;

    lcd_clear_display();
    lcd_setmargins(0, 8);
    status_draw(true);
    fmradio_set_status(FMRADIO_PLAYING);

    font_get(FONT_UI);
    lcd_getstringsize("M", &fw, &fh);

    /* Adjust for font size, trying to center the information vertically */
    if(fh < 10)
        top_of_screen = 1;
    
    radio_load_presets();

#ifndef SIMULATOR
    if(rec_create_directory() > 0)
        have_recorded = true;
    
    mpeg_stop();
    
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

    
    mpeg_set_recording_gain(mpeg_sound_default(SOUND_LEFT_GAIN),
                            mpeg_sound_default(SOUND_RIGHT_GAIN), false);
#endif

    curr_freq = global_settings.last_frequency * FREQ_STEP + MIN_FREQ;
    
    fmradio_set(1, DEFAULT_IN1);
    fmradio_set(2, DEFAULT_IN2);

    radio_set(RADIO_FREQUENCY, curr_freq);
    radio_set(RADIO_IF_MEASUREMENT, 0);
    radio_set(RADIO_SENSITIVITY, 0);
    radio_set(RADIO_FORCE_MONO, global_settings.fm_force_mono);
    radio_set(RADIO_MUTE, 0);
    
    curr_preset = find_preset(curr_freq);

    buttonbar_set(str(LANG_BUTTONBAR_MENU), str(LANG_FM_BUTTONBAR_PRESETS),
                  str(LANG_FM_BUTTONBAR_RECORD));

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
            val = fmradio_read(3);
            i_freq = (val & 0x7ffff) / 80;

            /* Stop searching if the IF frequency is close to 10.7MHz */
            if(i_freq > 1065 && i_freq < 1075)
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
            button = button_get_w_tmo(HZ / peak_meter_fps);
        switch(button)
        {
            case BUTTON_OFF:
#ifndef SIMULATOR
                if(mpeg_status() == MPEG_STATUS_RECORD)
                {
                    mpeg_stop();
                }
                else
#endif
                {
                    radio_stop();
                    done = true;
                }
                update_screen = true;
                break;

            case BUTTON_F3:
#ifndef SIMULATOR
                if(mpeg_status() == MPEG_STATUS_RECORD)
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

            case BUTTON_ON | BUTTON_REL:
                done = true;
                keep_playing = true;
                break;
                
            case BUTTON_LEFT:
                curr_freq -= FREQ_STEP;
                if(curr_freq < MIN_FREQ)
                    curr_freq = MIN_FREQ;

                radio_set(RADIO_FREQUENCY, curr_freq);
                curr_preset = find_preset(curr_freq);
                remember_frequency();
                search_dir = 0;
                update_screen = true;
                break;

            case BUTTON_RIGHT:
                curr_freq += FREQ_STEP;
                if(curr_freq > MAX_FREQ)
                    curr_freq = MAX_FREQ;
                
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
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                update_screen = true;
                settings_save();
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                update_screen = true;
                settings_save();
                break;

            case BUTTON_F1:
                radio_menu();
                curr_preset = find_preset(curr_freq);
                lcd_clear_display();
                lcd_setmargins(0, 8);
                buttonbar_set(str(LANG_BUTTONBAR_MENU),
                              str(LANG_FM_BUTTONBAR_PRESETS),
                              str(LANG_FM_BUTTONBAR_RECORD));
                update_screen = true;
                break;
                
            case BUTTON_F2:
                handle_radio_presets();
                curr_preset = find_preset(curr_freq);
                lcd_clear_display();
                lcd_setmargins(0, 8);
                buttonbar_set(str(LANG_BUTTONBAR_MENU),
                              str(LANG_FM_BUTTONBAR_PRESETS),
                              str(LANG_FM_BUTTONBAR_RECORD));
                update_screen = true;
                break;
                
#if (BUTTON_UP != BUTTON_PLAY) /* FixMe, this is just to make the Ondio compile */
            case BUTTON_PLAY:
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
                if(mpeg_status() != MPEG_STATUS_RECORD)
                {
                    default_event_handler(SYS_USB_CONNECTED);
                    fmradio_set_status(0);
                    screen_freeze = true; /* Cosmetic: makes sure the
                                             radio screen doesn't redraw */
                    done = true;
                }
                break;
                
            default:
                default_event_handler(button);
                break;
        }

        peak_meter_peek();

        if(!screen_freeze)
        {
            lcd_setmargins(0, 8);
            
            /* Only display the peak meter when not recording */
            if(!mpeg_status())
            {
                lcd_clearrect(0, 8 + fh*(top_of_screen + 3), LCD_WIDTH, fh);
                peak_meter_draw(0, 8 + fh*(top_of_screen + 3), LCD_WIDTH, fh);
                lcd_update_rect(0, 8 + fh*(top_of_screen + 3), LCD_WIDTH, fh);
            }
            
            if(TIME_AFTER(current_tick, timeout))
            {
                timeout = current_tick + HZ;
                
                val = fmradio_read(3);
                stereo = ((val & 0x100000)?true:false) &
                    !global_settings.fm_force_mono;
                if(stereo != last_stereo_status)
                {
                    update_screen = true;
                    last_stereo_status = stereo;
                }
            }
            
#ifndef SIMULATOR
            seconds = mpeg_recorded_time() / HZ;
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
                    lcd_clearrect(0, 8 + top_of_screen*fh, LCD_WIDTH, fh);
                }
                
                freq = curr_freq / 100000;
                snprintf(buf, 128, str(LANG_FM_STATION), freq / 10, freq % 10);
                lcd_puts(0, top_of_screen + 1, buf);
                
                snprintf(buf, 128,
                         stereo?str(LANG_CHANNEL_STEREO):
                         str(LANG_CHANNEL_MONO));
                lcd_puts(0, top_of_screen + 2, buf);

                if(mpeg_status() == MPEG_STATUS_RECORD)
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
                
                /* Only force the redraw if update_screen is true */
                status_draw(update_screen);
                
                buttonbar_draw();
                
                lcd_update();
                
                update_screen = false;
            }
        }

        if(mpeg_status() & MPEG_STATUS_ERROR)
        {
            done = true;
        }
    }

#ifndef SIMULATOR
    if(mpeg_status() & MPEG_STATUS_ERROR)
    {
        splash(0, true, str(LANG_DISK_FULL));
        status_draw(true);
        lcd_update();
        mpeg_error_clear();

        while(1)
        {
            button = button_get(true);
            if(button == (BUTTON_OFF | BUTTON_REL))
                break;
        }
    }
    
    mpeg_init_playback();

    sound_settings_apply();

    fmradio_set_status(0);

    if(keep_playing)
    {
        /* Enable the Left and right A/D Converter */
        mpeg_set_recording_gain(mpeg_sound_default(SOUND_LEFT_GAIN),
                                mpeg_sound_default(SOUND_RIGHT_GAIN), false);
        mas_codec_writereg(6, 0x4000);
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
            fprintf(fd, "%d:%s\n", presets[i].frequency, presets[i].name);
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
    int i;
    int f;

    if(!presets_loaded)
    {
        memset(presets, 0, sizeof(presets));
        num_presets = 0;
    
        fd = open(default_filename, O_RDONLY);
        if(fd >= 0)
        {
            i = 0;
            while(!done && i < MAX_PRESETS)
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

static bool radio_add_preset(void)
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

static int handle_radio_presets_menu_cb(int key, int m)
{
    (void)m;
    switch(key)
    {
        case BUTTON_F3:
            key = BUTTON_LEFT; /* Fake an exit */
            break;
    }
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

int handle_radio_presets_cb(int key, int m)
{
    bool ret;
    
    switch(key)
    {
        case BUTTON_F1:
            radio_add_preset();
            menu_draw(m);
            key = BUTTON_NONE;
            break;
            
        case BUTTON_F2:
            menu_draw(m);
            key = BUTTON_LEFT; /* Fake an exit */
            break;
            
        case BUTTON_F3:
            ret = handle_radio_presets_menu();
            menu_draw(m);
            if(ret)
                key = MENU_ATTACHED_USB;
            else
                key = BUTTON_NONE;
            break;
    }
    return key;
}

bool handle_radio_presets(void)
{
    int result;
    int i;
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
        result = menu_show(preset_menu);
        menu_exit(preset_menu);
        if (result == MENU_SELECTED_EXIT)
            return false;
        else if (result == MENU_ATTACHED_USB)
            reload_dir = true;
        
        if (result >= 0) /* A preset was selected */
        {
            i = menu_cursor(preset_menu);
            curr_freq = presets[i].frequency;
            radio_set(RADIO_FREQUENCY, curr_freq);
            remember_frequency();
        }
    }
    
    return reload_dir;
}

#ifndef SIMULATOR
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

bool radio_menu(void)
{
    struct menu_item items[3];
    int m;
    bool result;

    m = menu_init(items, 0, NULL, NULL, NULL, NULL);

    create_monomode_menu();
    menu_insert(m, -1, monomode_menu_string, toggle_mono_mode);
    menu_insert(m, -1, ID2P(LANG_SOUND_SETTINGS), sound_menu);
    
#ifndef SIMULATOR
    menu_insert(m, -1, ID2P(LANG_RECORDING_SETTINGS), fm_recording_settings);
#endif

    result = menu_run(m);
    menu_exit(m);
    return result;
}

#endif
