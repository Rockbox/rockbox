/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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

#include "system.h"
#include "lcd.h"
#include "mpeg.h"
#include "button.h"
#include "kernel.h"
#include "settings.h"
#include "lang.h"
#include "font.h"
#include "icons.h"
#include "screens.h"
#include "peakmeter.h"
#include "status.h"
#include "menu.h"
#include "sound_menu.h"

bool f2_rec_screen(void);
bool f3_rec_screen(void);

extern int recording_peak_left;
extern int recording_peak_right;

extern int mp3buf_write;
extern int mp3buf_read;
extern bool recording;

#define SOURCE_MIC 0
#define SOURCE_LINE 1
#define SOURCE_SPDIF 2

char *freq_str[6] =
{
    "44.1kHz",
    "48kHz",
    "32kHz",
    "22.05kHz",
    "24kHz",
    "16kHz"
};

static void set_gain(void)
{
    if(global_settings.rec_source == SOURCE_MIC)
    {
        mpeg_set_recording_gain(global_settings.rec_left_gain, 0,
                                global_settings.rec_mic_gain);
    }
    else
    {
        mpeg_set_recording_gain(global_settings.rec_left_gain,
                                global_settings.rec_right_gain, 0);
    }
}

static char *fmtstr[] =
{
    "",                 /* no decimals */
    "%d.%d %s  ",       /* 1 decimal */
    "%d.%02d %s  "      /* 2 decimals */
};

char *fmt_gain(int snd, int val, char *str, int len)
{
    int tmp, i, d, numdec;
    char *unit;
    
    tmp = mpeg_val2phys(snd, val);
    numdec = mpeg_sound_numdecimals(snd);
    unit = mpeg_sound_unit(snd);
    
    i = tmp / (10*numdec);
    d = tmp % (10*numdec);
    
    snprintf(str, len, fmtstr[numdec], i, d, unit);
    return str;
}

int cursor;

void adjust_cursor(void)
{
    if(global_settings.rec_source == SOURCE_LINE)
    {
        if(cursor < 0)
            cursor = 0;
    
        if(cursor > 2)
            cursor = 2;
    }
    else
    {
        cursor = 0;
    }
}

bool recording_screen(void)
{
    int button;
    bool done = false;
    char buf[32];
    char buf2[32];
    long timeout = current_tick + HZ/10;
    int gain;
    int w, h;
    int update_countdown = 1;

    cursor = 0;
    mpeg_init_recording();
    status_set_playmode(STATUS_STOP);
    
    peak_meter_playback(false);
    
    peak_meter_enabled = true;

    mpeg_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               global_settings.rec_source,
                               global_settings.rec_channels);

    lcd_setfont(FONT_UI);
    lcd_getstringsize("M", &w, &h);
    lcd_setmargins(w, 8);
    
    while(!done)
    {
        button = button_get(false);
        switch(button)
        {
            case BUTTON_OFF:
                if(mpeg_status())
                {
                    mpeg_stop();
                    status_set_playmode(STATUS_STOP);
                }
                else
                {
                    peak_meter_playback(true);
                    peak_meter_enabled = false;
                    done = true;
                }
                update_countdown = 1; /* Update immediately */
                break;

            case BUTTON_PLAY:
                /* Only act if the mpeg is stopped */
                if(!mpeg_status())
                {
                    mpeg_record("");
                    status_set_playmode(STATUS_RECORD);
                    update_countdown = 1; /* Update immediately */
                }
                break;

            case BUTTON_UP:
                cursor--;
                adjust_cursor();
                update_countdown = 1; /* Update immediately */
                break;
                
            case BUTTON_DOWN:
                cursor++;
                adjust_cursor();
                update_countdown = 1; /* Update immediately */
                break;

            case BUTTON_RIGHT:
                switch(cursor)
                {
                    case 0:
                        if(global_settings.rec_source == SOURCE_MIC)
                        {
                            global_settings.rec_mic_gain++;
                            if(global_settings.rec_mic_gain >
                               mpeg_sound_max(SOUND_MIC_GAIN))
                                global_settings.rec_mic_gain =
                                    mpeg_sound_max(SOUND_MIC_GAIN);
                        }
                        else
                        {
                            gain = MAX(global_settings.rec_left_gain,
                                       global_settings.rec_right_gain) + 1;
                            if(gain > mpeg_sound_max(SOUND_MIC_GAIN))
                                gain = mpeg_sound_max(SOUND_MIC_GAIN);
                            global_settings.rec_left_gain = gain;
                            global_settings.rec_right_gain = gain;
                        }
                        break;
                    case 1:
                        global_settings.rec_left_gain++;
                        if(global_settings.rec_left_gain >
                           mpeg_sound_max(SOUND_LEFT_GAIN))
                            global_settings.rec_left_gain =
                                mpeg_sound_max(SOUND_LEFT_GAIN);
                        break;
                    case 2:
                        global_settings.rec_right_gain++;
                        if(global_settings.rec_right_gain >
                           mpeg_sound_max(SOUND_RIGHT_GAIN))
                            global_settings.rec_right_gain =
                                mpeg_sound_max(SOUND_RIGHT_GAIN);
                        break;
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case BUTTON_LEFT:
                switch(cursor)
                {
                    case 0:
                        if(global_settings.rec_source == SOURCE_MIC)
                        {
                            global_settings.rec_mic_gain--;
                            if(global_settings.rec_mic_gain <
                               mpeg_sound_min(SOUND_MIC_GAIN))
                                global_settings.rec_mic_gain =
                                    mpeg_sound_min(SOUND_MIC_GAIN);
                        }
                        else
                        {
                            gain = MAX(global_settings.rec_left_gain,
                                       global_settings.rec_right_gain) - 1;
                            if(gain < mpeg_sound_min(SOUND_LEFT_GAIN))
                                gain = mpeg_sound_min(SOUND_LEFT_GAIN);
                            global_settings.rec_left_gain = gain;
                            global_settings.rec_right_gain = gain;
                        }
                        break;
                    case 1:
                        global_settings.rec_left_gain--;
                        if(global_settings.rec_left_gain <
                           mpeg_sound_min(SOUND_LEFT_GAIN))
                            global_settings.rec_left_gain =
                                mpeg_sound_min(SOUND_LEFT_GAIN);
                        break;
                    case 2:
                        global_settings.rec_right_gain--;
                        if(global_settings.rec_right_gain <
                           mpeg_sound_min(SOUND_MIC_GAIN))
                            global_settings.rec_right_gain =
                                mpeg_sound_min(SOUND_MIC_GAIN);
                        break;
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case BUTTON_F1:
                if (recording_menu())
                    return SYS_USB_CONNECTED;
                settings_save();
                mpeg_set_recording_options(global_settings.rec_frequency,
                                           global_settings.rec_quality,
                                           global_settings.rec_source,
                                           global_settings.rec_channels);
                
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;

            case BUTTON_F2:
                if (f2_rec_screen())
                    return SYS_USB_CONNECTED;
                update_countdown = 1; /* Update immediately */
                break;

            case BUTTON_F3:
                if (f3_rec_screen())
                    return SYS_USB_CONNECTED;
                update_countdown = 1; /* Update immediately */
                break;

        }

        peak_meter_peek();
        
        if(TIME_AFTER(current_tick, timeout))
        {
            timeout = current_tick + HZ/10;

            update_countdown--;
            if(update_countdown == 0)
            {
                update_countdown = 10;
                
                lcd_clear_display();
                peak_meter_draw(0, 8 + h, LCD_WIDTH, h);

                /* Show mic gain if input source is Mic */
                if(global_settings.rec_source == 0)
                {
                    snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_GAIN),
                             fmt_gain(SOUND_MIC_GAIN,
                                      global_settings.rec_mic_gain,
                                      buf2, sizeof(buf2)));
                    lcd_puts(0, 3, buf);
                }
                else
                {
                    if(global_settings.rec_source == SOURCE_LINE)
                    {
                        gain = MAX(global_settings.rec_left_gain,
                                   global_settings.rec_right_gain);
                        
                        snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_GAIN),
                                 fmt_gain(SOUND_LEFT_GAIN, gain,
                                          buf2, sizeof(buf2)));
                        lcd_puts(0, 3, buf);
                        
                        snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_LEFT),
                                 fmt_gain(SOUND_LEFT_GAIN,
                                          global_settings.rec_left_gain,
                                          buf2, sizeof(buf2)));
                        lcd_puts(0, 4, buf);
                        
                        snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_RIGHT),
                                 fmt_gain(SOUND_RIGHT_GAIN,
                                          global_settings.rec_right_gain,
                                          buf2, sizeof(buf2)));
                        lcd_puts(0, 5, buf);
                    }
                }

                status_draw();

                if(global_settings.rec_source != SOURCE_SPDIF)
                    put_cursorxy(0, 3 + cursor, true);

                snprintf(buf, 32, "%s %s [%d]",
                         freq_str[global_settings.rec_frequency],
                         global_settings.rec_channels?
                         str(LANG_CHANNEL_MONO):str(LANG_CHANNEL_STEREO),
                         global_settings.rec_quality);
                lcd_puts(0, 6, buf);
            }
            else
            {
                lcd_clearrect(0, 8 + h, LCD_WIDTH, h);
                peak_meter_draw(0, 8 + h, LCD_WIDTH, h);
                lcd_update_rect(0, 8 + h, LCD_WIDTH, h);
            }
            lcd_update();
        }
    }
    
    mpeg_init_playback();

    return false;
}

bool f2_rec_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h;
    char buf[32];

    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize("A",&w,&h);
    lcd_stop_scroll();

    while (!exit) {
        char* ptr=NULL;

        lcd_clear_display();

        /* Recording quality */
        lcd_putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_RECORDING_QUALITY));
        snprintf(buf, 32, "%d", global_settings.rec_quality);
        lcd_putsxy(0, LCD_HEIGHT/2-h, buf);
        lcd_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                   LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8, true);

        /* Frequency */
        snprintf(buf, sizeof buf, "%s:", str(LANG_RECORDING_FREQUENCY));
        lcd_getstringsize(buf,&w,&h);
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, buf);
        ptr = freq_str[global_settings.rec_frequency];
        lcd_getstringsize(ptr, &w, &h);
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
        lcd_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                   LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8, true);

        /* Channel mode */
        switch ( global_settings.rec_channels ) {
            case 0:
                ptr = str(LANG_CHANNEL_STEREO);
                break;

            case 1:
                ptr = str(LANG_CHANNEL_MONO);
                break;
        }

        lcd_getstringsize(str(LANG_RECORDING_CHANNELS), &w, &h);
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2,
                   str(LANG_RECORDING_CHANNELS));
        lcd_getstringsize(str(LANG_F2_MODE), &w, &h);
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h, str(LANG_F2_MODE));
        lcd_getstringsize(ptr, &w, &h);
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2, ptr);
        lcd_bitmap(bitmap_icons_7x8[Icon_FastForward], 
                   LCD_WIDTH/2 + 8, LCD_HEIGHT/2 - 4, 7, 8, true);

        lcd_update();

        switch (button_get(true)) {
            case BUTTON_LEFT:
            case BUTTON_F2 | BUTTON_LEFT:
                global_settings.rec_quality++;
                if(global_settings.rec_quality > 7)
                    global_settings.rec_quality = 0;
                used = true;
                break;

            case BUTTON_DOWN:
            case BUTTON_F2 | BUTTON_DOWN:
                global_settings.rec_frequency++;
                if(global_settings.rec_frequency > 5)
                    global_settings.rec_frequency = 0;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F2 | BUTTON_RIGHT:
                global_settings.rec_channels++;
                if(global_settings.rec_channels > 1)
                    global_settings.rec_channels = 0;
                used = true;
                break;

            case BUTTON_F2 | BUTTON_REL:
                if ( used )
                    exit = true;
                used = true;
                break;

            case BUTTON_F2 | BUTTON_REPEAT:
                used = true;
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                return true;
        }
    }

    mpeg_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               global_settings.rec_source,
                               global_settings.rec_channels);

    set_gain();
    
    settings_save();
    lcd_setfont(FONT_UI);

    return false;
}

bool f3_rec_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h;
    char *src_str[] =
    {
        str(LANG_RECORDING_SRC_MIC),
        str(LANG_RECORDING_SRC_LINE),
        str(LANG_RECORDING_SRC_DIGITAL)
    };

    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize("A",&w,&h);
    lcd_stop_scroll();

    while (!exit) {
        char* ptr=NULL;

        lcd_clear_display();

        /* Recording source */
        lcd_putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_RECORDING_SOURCE));
        ptr = src_str[global_settings.rec_source];
        lcd_getstringsize(ptr, &w, &h);
        lcd_putsxy(0, LCD_HEIGHT/2-h, ptr);
        lcd_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                   LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8, true);

        lcd_update();

        switch (button_get(true)) {
            case BUTTON_LEFT:
            case BUTTON_F3 | BUTTON_LEFT:
                global_settings.rec_source++;
                if(global_settings.rec_source > 2)
                    global_settings.rec_source = 0;
                used = true;
                break;

            case BUTTON_DOWN:
            case BUTTON_F3 | BUTTON_DOWN:
                global_settings.rec_frequency++;
                if(global_settings.rec_frequency > 5)
                    global_settings.rec_frequency = 0;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F3 | BUTTON_RIGHT:
                global_settings.rec_channels++;
                if(global_settings.rec_channels > 1)
                    global_settings.rec_channels = 0;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_REL:
                if ( used )
                    exit = true;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_REPEAT:
                used = true;
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                return true;
        }
    }

    mpeg_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               global_settings.rec_source,
                               global_settings.rec_channels);

    set_gain();
    
    settings_save();
    lcd_setfont(FONT_UI);

    return false;
}
