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
#include "status.h"
#include "playlist.h"
#include "sprintf.h"

#ifdef HAVE_LCD_BITMAP
#define BMPHEIGHT_usb_logo 32
#define BMPWIDTH_usb_logo 100
static unsigned char usb_logo[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08,
    0x04, 0x04, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0x81, 0x81, 0x81,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0xf1, 0x4f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0xc0, 
    0x00, 0x00, 0xe0, 0x1c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x06, 0x81, 0xc0, 0xe0, 0xe0, 0xe0, 0xe0,
    0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x1c,
    0x0c, 0x0e, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xc0, 0xc0, 0x80, 0x80, 0x00, 0x00,
    0x00, 0x00, 0xe0, 0x1f, 0x00, 0xf8, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x82, 0x7e, 0x00, 0xc0, 0x3e, 0x01, 
    0x70, 0x4f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x80, 0x00, 0x07, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f,
    0x0f, 0x07, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07, 0x0f,
    0x1f, 0x3f, 0x7b, 0xf3, 0xe3, 0xc3, 0x83, 0x83, 0x83, 0x83, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0x03, 0x03, 0x03, 0x3f, 0x1f, 0x1f, 0x0f, 0x0f, 0x07, 0x02,
    0xc0, 0x3e, 0x01, 0xe0, 0x9f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0xf0, 0x0f, 0x80, 0x78, 0x07, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0c, 0x10, 0x20, 0x40, 0x40, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x87, 0x87, 0x87,
    0x87, 0x87, 0x87, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf0,
    0x0f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x07, 0x00, 0x00, 0x00, 0x00, 
};
#endif

void usb_display_info(void)
{
    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    /* lcd_bitmap() only supports 16 pixels height! */
    lcd_bitmap(usb_logo, 6, 16,
               BMPWIDTH_usb_logo, 8, false);
    lcd_bitmap(usb_logo+BMPWIDTH_usb_logo, 6, 24,
               BMPWIDTH_usb_logo, 8, false);
    lcd_bitmap(usb_logo+BMPWIDTH_usb_logo*2, 6, 32,
               BMPWIDTH_usb_logo, 8, false);
    lcd_bitmap(usb_logo+BMPWIDTH_usb_logo*3, 6, 40,
               BMPWIDTH_usb_logo, 8, false);
    status_draw(true);
    lcd_update();
#else
    lcd_puts(0, 0, "[USB Mode]");
    status_set_param(false);
    status_set_audio(false);
    status_set_usb(true);
    status_draw(false);
#endif
}

void usb_screen(void)
{
#ifndef SIMULATOR
    backlight_on();
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    usb_display_info();
    while(usb_wait_for_disconnect_w_tmo(&button_queue, HZ)) {
        if(usb_inserted()) {
            status_draw(false);
        }
    }
#ifdef HAVE_LCD_CHARCELLS
    status_set_usb(false);
#endif

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
    static int pitch = 1000;
    bool exit = false;
    bool used = false;

    while (!exit) {

        if ( used ) {
            char* ptr;
            char buf[32];
            int w, h;

            lcd_clear_display();
            lcd_setfont(FONT_SYSFIXED);
    
            ptr = str(LANG_PITCH_UP);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, 0, ptr);
            lcd_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                       LCD_WIDTH/2 - 3, h*2, 7, 8, true);

            snprintf(buf, sizeof buf, "%d.%d%%", pitch / 10, pitch % 10 );
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
                if ( pitch > 2000 )
                    pitch = 2000;
                mpeg_set_pitch(pitch);
                break;

            case BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REPEAT:
                used = true;
                pitch--;
                if ( pitch < 500 )
                    pitch = 500;
                mpeg_set_pitch(pitch);
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

            case BUTTON_ON | BUTTON_RIGHT:
                if ( pitch < 2000 ) {
                    pitch += 20;
                    mpeg_set_pitch(pitch);
                }
                break;

            case BUTTON_RIGHT | BUTTON_REL:
                if ( pitch > 500 ) {
                    pitch -= 20;
                    mpeg_set_pitch(pitch);
                }
                break;

            case BUTTON_ON | BUTTON_LEFT:
                if ( pitch > 500 ) {
                    pitch -= 20;
                    mpeg_set_pitch(pitch);
                }
                break;

            case BUTTON_LEFT | BUTTON_REL:
                if ( pitch < 2000 ) {
                    pitch += 20;
                    mpeg_set_pitch(pitch);
                }
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
                
            case SHOW_PLAYLIST:
                ptr = str(LANG_FILTER_PLAYLIST);
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

        /* Invert */
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, str(LANG_INVERT));
        lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, 
                   global_settings.invert ? str(LANG_ON) : str(LANG_OFF));
        lcd_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                   LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8, true);

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

            case BUTTON_DOWN:
            case BUTTON_F3 | BUTTON_DOWN:
                global_settings.invert = !global_settings.invert;
                lcd_set_invert_display(global_settings.invert);
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

#ifdef HAVE_LCD_BITMAP
#define SPACE 3 /* pixels between words */
#define MAXLETTERS 128 /* 16*8 */
#define MAXLINES 10
#else
#define SPACE 1 /* one letter space */
#undef LCD_WIDTH
#define LCD_WIDTH 11
#undef LCD_HEIGHT
#define LCD_HEIGHT 2
#define MAXLETTERS 22 /* 11 * 2 */
#define MAXLINES 2
#endif

void splash(int ticks,   /* how long */
            int keymask, /* what keymask aborts the waiting (if any) */
            bool center, /* FALSE means left-justified, TRUE means
                            horizontal and vertical center */
            char *fmt,   /* what to say *printf style */
            ...)
{
    char *next;
    char *store=NULL;
    int x=0;
    int y=0;
    int w, h;
    unsigned char splash_buf[MAXLETTERS];
    va_list ap;
    unsigned char widths[MAXLINES];
    int line=0;
    bool first=true;
#ifdef HAVE_LCD_BITMAP
    int maxw=0;
#endif

    va_start( ap, fmt );
    vsnprintf( splash_buf, sizeof(splash_buf), fmt, ap );

    if(center) {
        
        /* first a pass to measure sizes */
        next = strtok_r(splash_buf, " ", &store);
        while (next) {
#ifdef HAVE_LCD_BITMAP
            lcd_getstringsize(next, &w, &h);
#else
            w = strlen(next);
            h = 1; /* store height in characters */
#endif
            if(!first) {
                if(x+w> LCD_WIDTH) {
                    /* Too wide, wrap */
                    y+=h;
                    line++;
                    if((y > (LCD_HEIGHT-h)) || (line > MAXLINES))
                        /* STOP */
                        break;
                    x=0;
                    first=true;
                }
            }
            else
                first = false;

            /* think of it as if the text was written here at position x,y
               being w pixels/chars wide and h high */

            x += w+SPACE;
            widths[line]=x-SPACE; /* don't count the trailing space */
#ifdef HAVE_LCD_BITMAP
            /* store the widest line */
            if(widths[line]>maxw)
                maxw = widths[line];
#endif
            next = strtok_r(NULL, " ", &store);
        }
#ifdef HAVE_LCD_BITMAP
        /* Start displaying the message at position y. The reason for the
           added h here is that it isn't added until the end of lines in the
           loop above and we always break the loop in the middle of a line. */
        y = (LCD_HEIGHT - (y+h) )/2;
#else
        y = 0; /* vertical center on 2 lines would be silly */
#endif
        first=true;

        /* Now recreate the string again since the strtok_r() above has ruined
           the one we already have! Here's room for improvements! */
        vsnprintf( splash_buf, sizeof(splash_buf), fmt, ap );
    }
    va_end( ap );

    if(center)
        x = (LCD_WIDTH-widths[0])/2;

#ifdef HAVE_LCD_BITMAP
    /* If we center the display and it wouldn't cover the full screen,
       then just clear the box we need and put a nice little frame and
       put the text in there! */
    if(center && (y > 2)) {
        if(maxw < (LCD_WIDTH -4)) {
            int xx = (LCD_WIDTH-maxw)/2 - 2;
            lcd_clearrect(xx, y-2, maxw+4, LCD_HEIGHT-y*2+4);
            lcd_drawrect(xx, y-2, maxw+4, LCD_HEIGHT-y*2+4);
        }
        else {
            lcd_clearrect(0, y-2, LCD_WIDTH, LCD_HEIGHT-y*2+4);
            lcd_drawline(0, y-2, LCD_WIDTH-1, y-2);
            lcd_drawline(0, LCD_HEIGHT-y+2, LCD_WIDTH-1, LCD_HEIGHT-y+2);
        }
    }
    else
#endif
        lcd_clear_display();
    line=0;
    next = strtok_r(splash_buf, " ", &store);
    while (next) {
#ifdef HAVE_LCD_BITMAP
        lcd_getstringsize(next, &w, &h);
#else
        w = strlen(next);
        h = 1;
#endif
        if(!first) {
            if(x+w> LCD_WIDTH) {
                /* too wide */
                y+=h;
                line++; /* goto next line */
                first=true;
                if(y > (LCD_HEIGHT-h))
                    /* STOP */
                    break;
                if(center)
                    x = (LCD_WIDTH-widths[line])/2;
                else
                    x=0;
            }
        }
        else
            first=false;
#ifdef HAVE_LCD_BITMAP
        lcd_putsxy(x, y, next);
#else
        lcd_puts(x, y, next);
#endif
        x += w+SPACE; /*  pixels space! */
        next = strtok_r(NULL, " ", &store);
    }
    lcd_update();

    if(ticks) {
        if(keymask) {
            int start = current_tick;
            int done = ticks + current_tick + 1;
            while (TIME_BEFORE( current_tick, done)) {
                int button = button_get_w_tmo(ticks - (current_tick-start));
                if((button & keymask) == keymask)
                    break;
            }
        }
        else
            /* unbreakable! */
            sleep(ticks);
    }
}
