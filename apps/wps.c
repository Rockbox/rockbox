/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Jerome Kuptz
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "file.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "tree.h"
#include "debug.h"
#include "sprintf.h"
#include "settings.h"
#include "wps.h"
#include "mpeg.h"
#include "usb.h"
#include "power.h"
#include "status.h"
#include "main_menu.h"

#define LINE_Y      1 /* initial line */

#define PLAY_DISPLAY_DEFAULT         0 
#define PLAY_DISPLAY_FILENAME_SCROLL 1 
#define PLAY_DISPLAY_TRACK_TITLE     2 

static void draw_screen(struct mp3entry* id3)
{
    lcd_clear_display();
    switch ( global_settings.wps_display ) {
        case PLAY_DISPLAY_TRACK_TITLE:
        {
            char ch = '/';
            char* end;
            char* szTok;
            char* szDelimit;
            char* szPeriod;
            char szArtist[26];
            char szBuff[257];
            szBuff[sizeof(szBuff)-1] = 0;

            strncpy(szBuff, id3->path, sizeof(szBuff));

            szTok = strtok_r(szBuff, "/", &end);
            szTok = strtok_r(NULL, "/", &end);

            // Assume path format of: Genre/Artist/Album/Mp3_file
            strncpy(szArtist,szTok,sizeof(szArtist));
            szArtist[sizeof(szArtist)-1] = 0;
            szDelimit = strrchr(id3->path, ch);
            lcd_puts(0,0, szArtist?szArtist:"<nothing>");

            // removes the .mp3 from the end of the display buffer
            szPeriod = strrchr(szDelimit, '.');
            if (szPeriod != NULL)
                *szPeriod = 0;

            lcd_puts_scroll(0,LINE_Y,(++szDelimit));
            break;
        }
        case PLAY_DISPLAY_FILENAME_SCROLL:
        {
            char ch = '/';
            char* szLast = strrchr(id3->path, ch);

            if (szLast)
                lcd_puts_scroll(0,0, (++szLast));
            else
                lcd_puts_scroll(0,0, id3->path);
            break;
        }
        case PLAY_DISPLAY_DEFAULT:
        {
            int l = 0;
#ifdef HAVE_LCD_BITMAP
            char buffer[64];

            lcd_puts_scroll(0, l++, id3->path);
            lcd_puts(0, l++, id3->title?id3->title:"");
            lcd_puts(0, l++, id3->album?id3->album:"");
            lcd_puts(0, l++, id3->artist?id3->artist:"");

            if(id3->vbr)
                snprintf(buffer, sizeof(buffer), "%d kbit (avg)",
                         id3->bitrate);
            else
                snprintf(buffer, sizeof(buffer), "%d kbit", id3->bitrate);

            lcd_puts(0, l++, buffer);

            snprintf(buffer,sizeof(buffer), "%d Hz", id3->frequency);
            lcd_puts(0, l++, buffer);
#else

            lcd_puts(0, l++, id3->artist?id3->artist:"<no artist>");
            lcd_puts_scroll(0, l++, id3->title?id3->title:"<no title>");
#endif
            break;
        }
    }
    status_draw();
    lcd_update();
}

/* demonstrates showing different formats from playtune */
int wps_show(void)
{
    static bool playing = true;
    struct mp3entry* id3 = mpeg_current_track();
    unsigned int lastlength=0, lastsize=0, lastrate=0;
    int lastartist=0, lastalbum=0, lasttitle=0;
    bool lastvbr = false;

    lcd_clear_display();

    while ( 1 ) {
        int i;
        char buffer[32];

        if ( ( id3->length != lastlength ) ||
             ( id3->filesize != lastsize ) ||
             ( id3->bitrate != lastrate ) ||
             ( id3->vbr != lastvbr ) ||
             ( (id3->artist?id3->artist[0]:0) != lastartist ) ||
             ( (id3->album?id3->album[0]:0) != lastalbum ) ||
             ( (id3->title?id3->title[0]:0) != lasttitle ) )
        {
	    lcd_stop_scroll();
            draw_screen(id3);
            lastlength = id3->length;
            lastsize = id3->filesize;
            lastrate = id3->bitrate;
            lastvbr = id3->vbr;
            lastartist = id3->artist?id3->artist[0]:0;
            lastalbum = id3->album?id3->album[0]:0;
            lasttitle = id3->title?id3->title[0]:0;
        }
        
        if (playing)
        {
#ifdef HAVE_LCD_BITMAP
            snprintf(buffer,sizeof(buffer), "Time: %d:%02d / %d:%02d",
                     id3->elapsed / 60000,
                     id3->elapsed % 60000 / 1000,
                     id3->length / 60000,
                     id3->length % 60000 / 1000 );

            lcd_puts(0, 6, buffer);
            lcd_update();
#else
            // Display time with the filename scroll only because the screen has room. 
            if (global_settings.wps_display == PLAY_DISPLAY_FILENAME_SCROLL)
            { 

                snprintf(buffer,sizeof(buffer), "Time: %d:%02d / %d:%02d",
                         id3->elapsed / 60000,
                         id3->elapsed % 60000 / 1000,
                         id3->length / 60000,
                         id3->length % 60000 / 1000 );

                lcd_puts(0, 1, buffer);
                lcd_update();
           }
#endif
        } 

        status_draw();
        
#ifdef HAVE_LCD_BITMAP
        /* draw battery indicator line */
        lcd_clearline(0,LCD_HEIGHT-1,LCD_WIDTH-1, LCD_HEIGHT-1);
        lcd_drawline(0,LCD_HEIGHT-1,battery_level() * (LCD_WIDTH-1) / 100, LCD_HEIGHT-1);
#endif

        for ( i=0;i<5;i++ ) {
            switch ( button_get(false) ) {
                case BUTTON_ON:
                    return 0;

#ifdef HAVE_RECORDER_KEYPAD
                case BUTTON_PLAY:
#else
                case BUTTON_UP:
#endif
                    if (global_settings.hold)
                        break;

                    if ( playing )
                    {
                        mpeg_pause();
                        status_set_playmode(STATUS_PAUSE);
                    }
                    else
                    {
                        mpeg_resume();
                        status_set_playmode(STATUS_PLAY);
                    }

                    playing = !playing;
                    break;

#ifdef HAVE_RECORDER_KEYPAD
                case BUTTON_UP:
                    if (global_settings.hold)
                        break;
                    global_settings.volume++;
                    if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                        global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                    mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                    break;

                case BUTTON_DOWN:
                    if (global_settings.hold)
                        break;
                    global_settings.volume--;
                    if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                        global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                    mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                    break;
#endif

                case BUTTON_LEFT:
                    if (global_settings.hold)
                        break;
                    mpeg_prev();
                    break;

                case BUTTON_RIGHT:
                    if (global_settings.hold)
                        break;
                    mpeg_next();
                    break;

#ifdef HAVE_RECORDER_KEYPAD
                case BUTTON_F1:
#else
                case BUTTON_MENU:
#endif
                    lcd_stop_scroll();
                    main_menu();
                    draw_screen(id3);
                    break;

#ifdef HAVE_RECORDER_KEYPAD                
                case BUTTON_OFF:
#else
                case BUTTON_DOWN:
#endif
                    if (global_settings.hold)
                        break;
                    mpeg_stop();
                    status_set_playmode(STATUS_STOP);
                    return 0;
                    
#ifndef SIMULATOR
                case SYS_USB_CONNECTED:
                    /* Tell the USB thread that we are safe */
                    DEBUGF("wps got SYS_USB_CONNECTED\n");
                    usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    
                    /* Wait until the USB cable is extracted again */
                    usb_wait_for_disconnect(&button_queue);

                    /* Signal to our caller that we have been in USB mode */
                    return SYS_USB_CONNECTED;
                    break;
#endif
            }
            sleep(HZ/10);
        }
    }
}
