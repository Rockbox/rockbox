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

bool f2_rec_screen(void);
bool f3_rec_screen(void);

extern int recording_peak_left;
extern int recording_peak_right;

extern int mp3buf_write;
extern int mp3buf_read;
extern bool recording;

int mic_gain = 8;
int left_gain = 2;
int right_gain = 2;
unsigned int recording_quality = 5;
unsigned int recording_frequency = 0;
unsigned int recording_channel_mode = 0;
unsigned int recording_source = 0;

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
    if(recording_source == SOURCE_MIC)
    {
        mpeg_set_recording_gain(left_gain, 0, mic_gain);
    }
    else
    {
        mpeg_set_recording_gain(left_gain, right_gain, 0);
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
    if(recording_source == SOURCE_LINE)
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

    mpeg_set_recording_options(recording_frequency, recording_quality,
                               recording_source, recording_channel_mode);

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
                mpeg_record("");
                status_set_playmode(STATUS_RECORD);
                update_countdown = 1; /* Update immediately */
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
                        if(recording_source == SOURCE_MIC)
                        {
                            mic_gain++;
                            if(mic_gain > mpeg_sound_max(SOUND_MIC_GAIN))
                                mic_gain = mpeg_sound_max(SOUND_MIC_GAIN);
                        }
                        else
                        {
                            gain = MAX(left_gain, right_gain) + 1;
                            if(gain > mpeg_sound_max(SOUND_MIC_GAIN))
                                gain = mpeg_sound_max(SOUND_MIC_GAIN);
                            left_gain = gain;
                            right_gain = gain;
                        }
                        break;
                    case 1:
                        left_gain++;
                        if(left_gain > mpeg_sound_max(SOUND_LEFT_GAIN))
                            left_gain = mpeg_sound_max(SOUND_LEFT_GAIN);
                        break;
                    case 2:
                        right_gain++;
                        if(right_gain > mpeg_sound_max(SOUND_RIGHT_GAIN))
                            right_gain = mpeg_sound_max(SOUND_RIGHT_GAIN);
                        break;
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case BUTTON_LEFT:
                switch(cursor)
                {
                    case 0:
                        if(recording_source == SOURCE_MIC)
                        {
                            mic_gain--;
                            if(mic_gain < mpeg_sound_min(SOUND_MIC_GAIN))
                                mic_gain = mpeg_sound_min(SOUND_MIC_GAIN);
                        }
                        else
                        {
                            gain = MAX(left_gain, right_gain) - 1;
                            if(gain < mpeg_sound_min(SOUND_LEFT_GAIN))
                                gain = mpeg_sound_min(SOUND_LEFT_GAIN);
                            left_gain = gain;
                            right_gain = gain;
                        }
                        break;
                    case 1:
                        left_gain--;
                        if(left_gain < mpeg_sound_min(SOUND_LEFT_GAIN))
                            left_gain = mpeg_sound_min(SOUND_LEFT_GAIN);
                        break;
                    case 2:
                        right_gain--;
                        if(right_gain < mpeg_sound_min(SOUND_MIC_GAIN))
                            right_gain = mpeg_sound_min(SOUND_MIC_GAIN);
                        break;
                }
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
                if(recording_source == 0)
                {
                    snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_GAIN),
                             fmt_gain(SOUND_MIC_GAIN, mic_gain,
                                      buf2, sizeof(buf2)));
                    lcd_puts(0, 3, buf);
                }
                else
                {
                    if(recording_source == SOURCE_LINE)
                    {
                        gain = MAX(left_gain, right_gain);
                        
                        snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_GAIN),
                                 fmt_gain(SOUND_LEFT_GAIN, gain,
                                          buf2, sizeof(buf2)));
                        lcd_puts(0, 3, buf);
                        
                        snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_LEFT),
                                 fmt_gain(SOUND_LEFT_GAIN, left_gain,
                                          buf2, sizeof(buf2)));
                        lcd_puts(0, 4, buf);
                        
                        snprintf(buf, 32, "%s: %s", str(LANG_RECORDING_RIGHT),
                                 fmt_gain(SOUND_RIGHT_GAIN, right_gain,
                                          buf2, sizeof(buf2)));
                        lcd_puts(0, 5, buf);
                    }
                }

                status_draw();

                if(recording_source != SOURCE_SPDIF)
                    put_cursorxy(0, 3 + cursor, true);

                snprintf(buf, 32, "%s %s [%d]",
                         freq_str[recording_frequency],
                         recording_channel_mode?
                         str(LANG_CHANNEL_MONO):str(LANG_CHANNEL_STEREO),
                         recording_quality);
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
        snprintf(buf, 32, "%d", recording_quality);
        lcd_putsxy(0, LCD_HEIGHT/2-h, buf);
        lcd_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                   LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8, true);

        /* Frequency */
        snprintf(buf, sizeof buf, "%s:", str(LANG_RECORDING_FREQUENCY));
        lcd_getstringsize(buf,&w,&h);
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, buf);
        ptr = freq_str[recording_frequency];
        lcd_getstringsize(ptr, &w, &h);
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
        lcd_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                   LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8, true);

        /* Channel mode */
        switch ( recording_channel_mode ) {
            case 0:
                ptr = str(LANG_CHANNEL_STEREO);
                break;

            case 1:
                ptr = str(LANG_CHANNEL_MONO);
                break;
        }

        lcd_getstringsize(str(LANG_RECORDING_CHANNEL), &w, &h);
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2,
                   str(LANG_RECORDING_CHANNEL));
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
                recording_quality++;
                if(recording_quality > 7)
                    recording_quality = 0;
                used = true;
                break;

            case BUTTON_DOWN:
            case BUTTON_F2 | BUTTON_DOWN:
                recording_frequency++;
                if(recording_frequency > 5)
                    recording_frequency = 0;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F2 | BUTTON_RIGHT:
                recording_channel_mode++;
                if(recording_channel_mode > 1)
                    recording_channel_mode = 0;
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

    mpeg_set_recording_options(recording_frequency, recording_quality,
                               recording_source, recording_channel_mode);

//    settings_save();
    lcd_setfont(FONT_UI);

    return false;
}

char *src_str[] =
{
    "Mic",
    "Line In",
    "Digital"
};

bool f3_rec_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h;

    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize("A",&w,&h);
    lcd_stop_scroll();

    while (!exit) {
        char* ptr=NULL;

        lcd_clear_display();

        /* Recording source */
        lcd_putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_RECORDING_SOURCE));
        ptr = src_str[recording_source];
        lcd_getstringsize(ptr, &w, &h);
        lcd_putsxy(0, LCD_HEIGHT/2-h, ptr);
        lcd_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                   LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8, true);

        lcd_update();

        switch (button_get(true)) {
            case BUTTON_LEFT:
            case BUTTON_F3 | BUTTON_LEFT:
                recording_source++;
                if(recording_source > 2)
                    recording_source = 0;
                used = true;
                break;

            case BUTTON_DOWN:
            case BUTTON_F3 | BUTTON_DOWN:
                recording_frequency++;
                if(recording_frequency > 5)
                    recording_frequency = 0;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F3 | BUTTON_RIGHT:
                recording_channel_mode++;
                if(recording_channel_mode > 1)
                    recording_channel_mode = 0;
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

    mpeg_set_recording_options(recording_frequency, recording_quality,
                               recording_source, recording_channel_mode);

//    settings_save();
    lcd_setfont(FONT_UI);

    return false;
}
