/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "backlight.h"
#include "button.h"
#include "lcd.h"
#include "lang.h"
#include "icons.h"
#include "font.h"
#include "mpeg.h"
#include "usb.h"
#include "settings.h"
#include "playlist.h"

void usb_screen(void)
{
#ifndef SIMULATOR
    backlight_on();
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    usb_wait_for_disconnect(&button_queue);
    backlight_on();
#endif
}

#ifdef HAVE_RECORDER_KEYPAD
/* returns:
   0 if no key was pressed
   1 if a key was pressed (or if ON was held down long enough to repeat)
   2 if USB was connected */
int on_screen(void)
{
    static int pitch = 100;
    bool exit = false;
    bool used = false;

    lcd_setfont(FONT_SYSFIXED);

    while (!exit) {

        if ( used ) {
            char* ptr;
            char buf[32];
            int w, h;

            lcd_scroll_pause();
            lcd_clear_display();

            ptr = str(LANG_PITCH_UP);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, 0, ptr);
            lcd_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                       LCD_WIDTH/2 - 3, h*2, 7, 8, true);

            snprintf(buf, sizeof buf, "%d%%", pitch);
            lcd_getstringsize(buf,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, h, buf);

            ptr = str(LANG_PITCH_DOWN);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
            lcd_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                       LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8, true);

            ptr = str(LANG_PAUSE);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-(w/2))/2, LCD_HEIGHT/2 - h/2, ptr);
            lcd_bitmap(bitmap_icons_7x8[Icon_Pause],
                       (LCD_WIDTH-(w/2))/2-10, LCD_HEIGHT/2 - h/2, 7, 8, true);

            lcd_update();
        }

        /* use lastbutton, so the main loop can decide whether to
           exit to browser or not */
        switch (button_get(true)) {
            case BUTTON_UP:
            case BUTTON_ON | BUTTON_UP:
            case BUTTON_ON | BUTTON_UP | BUTTON_REPEAT:
                used = true;
                pitch++;
                if ( pitch > 200 )
                    pitch = 200;
#ifdef HAVE_MAS3587F
                mpeg_set_pitch(pitch);
#endif
                break;

            case BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REPEAT:
                used = true;
                pitch--;
                if ( pitch < 50 )
                    pitch = 50;
#ifdef HAVE_MAS3587F
                mpeg_set_pitch(pitch);
#endif
                break;

            case BUTTON_ON | BUTTON_PLAY:
                mpeg_pause();
                used = true;
                break;

            case BUTTON_PLAY | BUTTON_REL:
                mpeg_resume();
                used = true;
                break;

            case BUTTON_ON | BUTTON_PLAY | BUTTON_REL:
                mpeg_resume();
                exit = true;
                break;

#ifdef SIMULATOR
            case BUTTON_ON:
#else
            case BUTTON_ON | BUTTON_REL:
            case BUTTON_ON | BUTTON_UP | BUTTON_REL:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REL:
#endif
                exit = true;
                break;

            case BUTTON_ON | BUTTON_REPEAT:
                used = true;
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                return 2;
        }
    }

    lcd_setfont(FONT_UI);

    if ( used )
        return 1;
    else
        return 0;
}

bool f2_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h;
    char buf[32];
    int oldrepeat = global_settings.repeat_mode;

    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize("A",&w,&h);
    lcd_stop_scroll();

    while (!exit) {
        char* ptr=NULL;

        lcd_clear_display();

        /* Shuffle mode */
        lcd_putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_SHUFFLE));
        lcd_putsxy(0, LCD_HEIGHT/2 - h, str(LANG_F2_MODE));
        lcd_putsxy(0, LCD_HEIGHT/2, 
                   global_settings.playlist_shuffle ? 
                   str(LANG_ON) : str(LANG_OFF));
        lcd_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                   LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8, true);

        /* Directory Filter */
        switch ( global_settings.dirfilter ) {
            case SHOW_ALL:
                ptr = str(LANG_FILTER_ALL);
                break;

            case SHOW_SUPPORTED:
                ptr = str(LANG_FILTER_SUPPORTED);
                break;

            case SHOW_MUSIC:
                ptr = str(LANG_FILTER_MUSIC);
                break;
        }

        snprintf(buf, sizeof buf, "%s:", str(LANG_FILTER));
        lcd_getstringsize(buf,&w,&h);
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, buf);
        lcd_getstringsize(ptr,&w,&h);
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
        lcd_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                   LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8, true);

        /* Repeat Mode */
        switch ( global_settings.repeat_mode ) {
            case REPEAT_OFF:
                ptr = str(LANG_OFF);
                break;

            case REPEAT_ALL:
                ptr = str(LANG_REPEAT_ALL);
                break;

            case REPEAT_ONE:
                ptr = str(LANG_REPEAT_ONE);
                break;
        }

        lcd_getstringsize(str(LANG_REPEAT),&w,&h);
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2, str(LANG_REPEAT));
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h, str(LANG_F2_MODE));
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2, ptr);
        lcd_bitmap(bitmap_icons_7x8[Icon_FastForward], 
                   LCD_WIDTH/2 + 8, LCD_HEIGHT/2 - 4, 7, 8, true);

        lcd_update();

        switch (button_get(true)) {
            case BUTTON_LEFT:
            case BUTTON_F2 | BUTTON_LEFT:
                global_settings.playlist_shuffle =
                    !global_settings.playlist_shuffle;

                if (global_settings.playlist_shuffle)
                    randomise_playlist(current_tick);
                else
                    sort_playlist(true);
                used = true;
                break;

            case BUTTON_DOWN:
            case BUTTON_F2 | BUTTON_DOWN:
                global_settings.dirfilter++;
                if ( global_settings.dirfilter >= NUM_FILTER_MODES )
                    global_settings.dirfilter = 0;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F2 | BUTTON_RIGHT:
                global_settings.repeat_mode++;
                if ( global_settings.repeat_mode >= NUM_REPEAT_MODES )
                    global_settings.repeat_mode = 0;
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

    settings_save();
    lcd_setfont(FONT_UI);
    if ( oldrepeat != global_settings.repeat_mode )
        mpeg_flush_and_reload_tracks();

    return false;
}

bool f3_screen(void)
{
    bool exit = false;
    bool used = false;

    lcd_stop_scroll();
    lcd_setfont(FONT_SYSFIXED);

    while (!exit) {
        int w,h;
        char* ptr;

        ptr = str(LANG_F3_STATUS);
        lcd_getstringsize(ptr,&w,&h);
        lcd_clear_display();

        lcd_putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_F3_SCROLL));
        lcd_putsxy(0, LCD_HEIGHT/2 - h, str(LANG_F3_BAR));
        lcd_putsxy(0, LCD_HEIGHT/2, 
                   global_settings.scrollbar ? str(LANG_ON) : str(LANG_OFF));
        lcd_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                   LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8, true);

        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2, ptr);
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h, str(LANG_F3_BAR));
        lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2, 
                   global_settings.statusbar ? str(LANG_ON) : str(LANG_OFF));
        lcd_bitmap(bitmap_icons_7x8[Icon_FastForward], 
                   LCD_WIDTH/2 + 8, LCD_HEIGHT/2 - 4, 7, 8, true);
        lcd_update();

        switch (button_get(true)) {
            case BUTTON_LEFT:
            case BUTTON_F3 | BUTTON_LEFT:
                global_settings.scrollbar = !global_settings.scrollbar;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F3 | BUTTON_RIGHT:
                global_settings.statusbar = !global_settings.statusbar;
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

    settings_save();
    if (global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
    lcd_setfont(FONT_UI);

    return false;
}
#endif
