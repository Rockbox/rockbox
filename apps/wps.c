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
#include "powermgmt.h"
#include "status.h"
#include "main_menu.h"
#include "ata.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

#ifdef LOADABLE_FONTS
#include "ajf.h"
#endif

#ifdef HAVE_LCD_BITMAP
    #define PLAY_DISPLAY_2LINEID3        0 
    #define PLAY_DISPLAY_FILENAME_SCROLL 1 
    #define PLAY_DISPLAY_TRACK_TITLE     2 
#else
    #define PLAY_DISPLAY_1LINEID3        0
    #define PLAY_DISPLAY_2LINEID3        1 
    #define PLAY_DISPLAY_FILENAME_SCROLL 2 
    #define PLAY_DISPLAY_TRACK_TITLE     3 
    #define PLAY_DISPLAY_CUSTOM_WPS      4
#endif

#ifdef HAVE_RECORDER_KEYPAD
#define RELEASE_MASK (BUTTON_F1 | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP)
#else
#define RELEASE_MASK (BUTTON_MENU | BUTTON_STOP | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_PLAY)
#endif

bool keys_locked = false;
bool device_muted = false;
static bool ff_rewind = false;
static bool paused = false;

static void draw_screen(struct mp3entry* id3)
{
    int font_height;
#ifdef LOADABLE_FONTS
    unsigned char *font = lcd_getcurrentldfont();
    font_height  = ajf_get_fontheight(font);
#else
    font_height = 8;
#endif

    lcd_clear_display();
    if(!id3)
    {
#ifdef HAVE_LCD_CHARCELLS
        lcd_puts(0, 0, "End of list");
        lcd_puts(0, 1, "<Press ON>");
#else
        lcd_puts(0, 2, "<End of song list>");
        lcd_puts(5, 4, "Press ON");
#endif
    }
    else
    {
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
                lcd_puts(0, 0, szArtist?szArtist:"<nothing>");

                // removes the .mp3 from the end of the display buffer
                szPeriod = strrchr(szDelimit, '.');
                if (szPeriod != NULL)
                    *szPeriod = 0;

                lcd_puts_scroll(0, 1, (++szDelimit));
                break;
            }
            case PLAY_DISPLAY_FILENAME_SCROLL:
            {
                char buffer[64];
                char ch = '/';
                char* szLast = strrchr(id3->path, ch);

                if (szLast)
                {
                    snprintf(buffer, sizeof(buffer), "%d/%d: %s",
                            id3->index + 1,
                            playlist.amount,
                            ++szLast);
                }
                else
                {
                    snprintf(buffer, sizeof(buffer), "%d/%d: %s",
                            id3->index + 1,
                            playlist.amount,
                            id3->path);
                }
                lcd_puts_scroll(0, 0, buffer);
                break;
            }
            case PLAY_DISPLAY_2LINEID3:
            {
                int l = 0;
#ifdef HAVE_LCD_BITMAP
                char buffer[64];

                lcd_puts_scroll(0, l++, id3->path);
                lcd_puts(0, l++, id3->title?id3->title:"");
                lcd_puts(0, l++, id3->album?id3->album:"");
                lcd_puts(0, l++, id3->artist?id3->artist:"");

                if(!global_settings.statusbar && font_height <= 8) {
                    if(id3->vbr)
                        snprintf(buffer, sizeof(buffer), "%d kbit (avg)",
                                 id3->bitrate);
                    else
                        snprintf(buffer, sizeof(buffer), "%d kbit", id3->bitrate);

                    lcd_puts(0, l++, buffer);
                    snprintf(buffer,sizeof(buffer), "%d Hz", id3->frequency);
                    lcd_puts(0, l++, buffer);
                }
                else {
                    if(id3->vbr)
                        snprintf(buffer, sizeof(buffer), "%dkbit(a) %dHz",
                                 id3->bitrate, id3->frequency);
                    else
                        snprintf(buffer, sizeof(buffer), "%dkbit    %dHz",
                                 id3->bitrate, id3->frequency);

                    lcd_puts(0, l++, buffer);
                }
#else
                lcd_puts(0, l++, id3->artist?id3->artist:"<no artist>");
                lcd_puts_scroll(0, l++, id3->title?id3->title:"<no title>");
#endif
                break;
            }
#ifdef HAVE_LCD_CHARCELLS
            case PLAY_DISPLAY_1LINEID3:
            {
                char buffer[64];
                char ch = '/';
                char* szLast = strrchr(id3->path, ch);

                if(id3->artist && id3->title)
                    snprintf(buffer, sizeof(buffer), "%d/%d: %s - %s",
                            id3->index + 1, playlist.amount,
                            id3->artist?id3->artist:"<no artist>",
                            id3->title?id3->title:"<no title>");
                else
                    snprintf(buffer, sizeof(buffer), "%d/%d: %s",
                            id3->index + 1, playlist.amount,
                            szLast?++szLast:id3->path);
                lcd_puts_scroll(0, 0, buffer);
                break;
            }
            case PLAY_DISPLAY_CUSTOM_WPS:
            {
                wps_load_custom_config();
            }
#endif
        }
    }
    status_draw();
    lcd_update();
}

int wps_load_custom_config(void)
{
    char buffer[128];
    char tmpbuf[64];
    char cchr1[0] = "";
    char cchr2[0] = "";
    int i;
    int fd;
    struct mp3entry* id3 = NULL;
    char ch = '/';
    char* szLast;

    szLast = strrchr(id3->path, ch);
    id3 = mpeg_current_track();
    snprintf(buffer, sizeof(buffer), "");
    lcd_stop_scroll();
    fd = open("/wps.config", O_RDONLY);
    if(-1 == fd)
    {
        lcd_puts(0, 0, "  *Error*  ");
        sleep(HZ);
        lcd_puts_scroll(0, 1, "--Couldn't Load wps.config--");
        sleep(HZ*5);
        global_settings.wps_display = 0;
        settings_save();
        draw_screen(id3);
        return(-1);
    }
    while(1)
    {
        i = read(fd, cchr1, 1);
        if(i <= 0)
        {
            close(fd);
            lcd_puts_scroll(0, 0, buffer);
            return(1);
        }
        switch(cchr1[0])
        {
            case '%':
                i = read(fd, cchr2, 1);
                if(i <= 0)
                {
                    close(fd);
                    lcd_puts_scroll(0, 0, buffer);
                    return(1);
                }
                switch(cchr2[0])
                {
                    case 't':  /* ID3 Title */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%s", id3->title);
                        break;
                    case 'a':  /* ID3 Artist */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%s", id3->artist);
                        break;
                    case 'n':  /* ID3 Track Number */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d", id3->tracknum);
                        break;
                    case 'u':  /* ID3 Album */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%s", id3->album);
                        break;
                    case 'c':  /* Conditional Filename \ ID3 Artist-Title */
                        if(id3->artist && id3->title)
                            snprintf(tmpbuf, sizeof(tmpbuf), "%s - %s",
                                    id3->artist?id3->artist:"<no artist>",
                                    id3->title?id3->title:"<no title>");
                        else
                            snprintf(tmpbuf, sizeof(tmpbuf), "%s",
                                    szLast?++szLast:id3->path);
                        break;
                    case 'h':  /* Conditional Filename \ ID3 Title-Artist */
                        if(id3->artist && id3->title)
                            snprintf(tmpbuf, sizeof(tmpbuf), "%s - %s",
                                     id3->title?id3->title:"<no title>",
                                     id3->artist?id3->artist:"<no artist>");
                        else
                            snprintf(tmpbuf, sizeof(tmpbuf), "%s",
                                    szLast?++szLast:id3->path);
                        break;
                    case 'b':  /* File Bitrate */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d", id3->bitrate);
                        break;
                    case 'f':  /* File Frequency */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d", id3->frequency);
                        break;
                    case 'p':  /* File Path */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%s", id3->path);
                        break;
                    case 'm':  /* File Name */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%s", 
                                szLast?++szLast:id3->path);
                        break;
                    case 's':  /* File Size (In Kilobytes) */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d",
                                id3->filesize / 1024);
                        break;
                    case 'i':  /* Playlist Position */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d", id3->index + 1);
                        break;
                    case 'l':  /* Playlist Total Entries */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d", playlist.amount);
                        break;
                    case 'e':  /* Elapsed Time */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d:%02d",
                                id3->elapsed / 60000,
                                id3->elapsed % 60000 / 1000);
                        break;
                    case 'o':  /* Total Time */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%d:%02d",
                                id3->elapsed / 60000,
                                id3->elapsed % 60000 / 1000);
                        break;
                    case '%':  /* Displays % */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%%");
                        break;                    
                }
                break;
            default:
                snprintf(tmpbuf, sizeof(tmpbuf), "%s", cchr1); 
                break;
        }
    snprintf(buffer, sizeof(buffer), "%s%s", buffer, tmpbuf);
    }
}


int player_id3_show(void)
{
#ifdef HAVE_PLAYER_KEYPAD
    int button;
    int menu_pos = 0;
    int menu_max = 6;
    bool menu_changed = true;
    char scroll_text[MAX_PATH];
    struct mp3entry* id3 = NULL;

    lcd_stop_scroll();
    lcd_clear_display();
    lcd_puts(0, 0, "-ID3 Info- ");
    lcd_puts(0, 1, "--Screen-- ");
    sleep(HZ*1.5);
 
    while(1)
    {
        id3 = mpeg_current_track();
        button = button_get(false);

        switch(button)
        {
            case BUTTON_LEFT:
                menu_changed = true;
                if(menu_pos > 0)
                    menu_pos--;
                else
                    menu_pos = menu_max;
                break;

            case BUTTON_RIGHT:
                menu_changed = true;
                if(menu_pos < menu_max)
                    menu_pos++;
                else
                    menu_pos = 0;
                break;
            
            case BUTTON_REPEAT:
                break;

            case BUTTON_STOP:
            case BUTTON_PLAY:
                lcd_stop_scroll();
                return(0);
                break;

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

        switch(menu_pos)
        {
            case 0:
                lcd_puts(0, 0, "Title");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->title);
                break;
            case 1:
                lcd_puts(0, 0, "Artist");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->artist);
                break;
            case 2:
                lcd_puts(0, 0, "Album");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->album);
                break;
            case 3:
                lcd_puts(0, 0, "Length");
                snprintf(scroll_text,sizeof(scroll_text), "%d:%02d",
                   id3->length / 60000,
                   id3->length % 60000 / 1000 );
                break;
            case 4:
                lcd_puts(0, 0, "Bitrate");
                snprintf(scroll_text,sizeof(scroll_text), "%d kbps", 
                        id3->bitrate);
                break;
            case 5:
                lcd_puts(0, 0, "Frequency");
                snprintf(scroll_text,sizeof(scroll_text), "%d kHz",
                        id3->frequency);
                break;
            case 6:
                lcd_puts(0, 0, "Path");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->path);
                break;
        }

        if(menu_changed == true)
        {
            menu_changed = false;
            lcd_stop_scroll();
            lcd_clear_display();
            lcd_puts_scroll(0, 1, scroll_text);
        }

        status_draw();
        lcd_update();
        yield();
    }
#endif
    return(0);
}

static void display_file_time(unsigned int elapsed, unsigned int length)
{
    char buffer[32];

#ifdef HAVE_LCD_BITMAP
    int line;
    if(global_settings.statusbar)
        line = 5;
    else
        line = 6;
    snprintf(buffer,sizeof(buffer),
             "Time:%3d:%02d/%d:%02d",
             elapsed / 60000,
             elapsed % 60000 / 1000,
             length / 60000,
             length % 60000 / 1000 );

    lcd_puts(0, line, buffer);
    slidebar(0, LCD_HEIGHT-6, LCD_WIDTH, 6, elapsed*100/length, Grow_Right);
    lcd_update();
#else
    /* Display time with the filename scroll only because
       the screen has room. */
    if ((global_settings.wps_display == PLAY_DISPLAY_FILENAME_SCROLL) ||
        global_settings.wps_display == PLAY_DISPLAY_1LINEID3 ||
        global_settings.wps_display == PLAY_DISPLAY_CUSTOM_WPS ||
        ff_rewind)
    {
        snprintf(buffer,sizeof(buffer), "%d:%02d/%d:%02d  ",
                 elapsed / 60000,
                 elapsed % 60000 / 1000,
                 length / 60000,
                 length % 60000 / 1000 );

        lcd_puts(0, 1, buffer);
        lcd_update();
    }
#endif
}

void display_volume_level(int vol_level)
{
    char buffer[32];

    lcd_stop_scroll();
    snprintf(buffer,sizeof(buffer),"Vol: %d %s       ", vol_level * 2, "%");

#ifdef HAVE_LCD_CHARCELLS
    lcd_puts(0, 0, buffer);
#else
    lcd_puts(2, 3, buffer);
    lcd_update();
#endif

    sleep(HZ/6);
}

void display_keylock_text(bool locked)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_CHARCELLS
    if(locked)
        lcd_puts(0, 0, "Keylock ON");
    else
        lcd_puts(0, 0, "Keylock OFF");
#else
    if(locked)
    {
        lcd_puts(2, 3, "Key lock is ON");
    }
    else
    {
        lcd_puts(2, 3, "Key lock is OFF");
    }
    status_draw();
    lcd_update();
#endif
    
    sleep(HZ);
}

void display_mute_text(bool muted)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_CHARCELLS
    if(muted)
        lcd_puts(0, 0, "Mute ON");
    else
        lcd_puts(0, 0, "Mute OFF");
#else
    if(muted)
    {
        lcd_puts(2, 3, "Mute is ON");
    }
    else
    {
        lcd_puts(2, 3, "Mute is OFF");
    }
    status_draw();
    lcd_update();
#endif
    
    sleep(HZ);
}

/* demonstrates showing different formats from playtune */
int wps_show(void)
{
    struct mp3entry* id3 = NULL;

#ifdef HAVE_PLAYER_KEYPAD
    int retval;
#endif
    bool dont_go_to_menu = false;
    bool menu_button_is_down = false;
    bool pending_keylock = true; /* Keylock will go ON next time */
    bool pending_mute = true; /* Mute will go ON next time */
    int old_release_mask;
    int button;
    int ff_rewind_count = 0;
    bool ignore_keyup = true;

    old_release_mask = button_set_release(RELEASE_MASK);

#ifdef HAVE_LCD_CHARCELLS
    lcd_icon(ICON_AUDIO, true);
    lcd_icon(ICON_PARAM, false);
#else
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    ff_rewind = false;
    
    if(mpeg_is_playing())
    {
        id3 = mpeg_current_track();
        draw_screen(id3);
    }

    while ( 1 )
    {
        bool restore = false;

        button = button_get_w_tmo(HZ/5);

        /* discard first event if it's a button release */
        if (button && ignore_keyup)
        {
            ignore_keyup = false;
            if (button & BUTTON_REL && button != SYS_USB_CONNECTED)
                continue;
        }
        
        if(mpeg_has_changed_track())
        {
            lcd_stop_scroll();
            id3 = mpeg_current_track();
            draw_screen(id3);
        }

        switch(button)
        {
            case BUTTON_ON:
                if (keys_locked)
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }

#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
                lcd_icon(ICON_AUDIO, false);
#endif
                button_set_release(old_release_mask);
                return 0;
                
            case BUTTON_PLAY:
                if (keys_locked)
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }

                if ( !paused )
                {
                    mpeg_pause();
                    paused = true;
                    status_set_playmode(STATUS_PAUSE);
                    status_draw();
                    if (global_settings.resume) {
                        settings_save();
#ifndef HAVE_RTC
                        ata_flush();
#endif
                    }
                }
                else
                {
                    mpeg_resume();
                    paused = false;
                    status_set_playmode(STATUS_PLAY);
                    status_draw();
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#endif
            case BUTTON_VOL_UP:
                if (keys_locked)
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }

                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw();
                settings_save();
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#endif
            case BUTTON_VOL_DOWN:
                if (keys_locked)
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }

                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw();
                settings_save();
                break;

            case BUTTON_LEFT | BUTTON_REPEAT:
                if (!keys_locked)
                {
                    if (ff_rewind)
                    {
                        ff_rewind_count -= global_settings.ff_rewind*1000;
                    }
                    else
                    {
                        if ( mpeg_is_playing() && id3 && id3->length )
                        {
                            if (!paused)
                                mpeg_pause();
#ifdef HAVE_PLAYER_KEYPAD
                            lcd_stop_scroll();
#endif
                            status_set_playmode(STATUS_FASTBACKWARD);
                            status_draw();
                            ff_rewind = true;
                            ff_rewind_count = -global_settings.ff_rewind*1000;
                        }
                        else
                            break;
                    }

                    if ((int)(id3->elapsed + ff_rewind_count) < 0)
                        ff_rewind_count = -id3->elapsed;

                    display_file_time(id3->elapsed + ff_rewind_count,
                                      id3->length);
                }
                break;

            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (!keys_locked)
                {
                    if (ff_rewind)
                    {
                        ff_rewind_count += global_settings.ff_rewind*1000;
                    }
                    else
                    {
                        if ( mpeg_is_playing() && id3 && id3->length )
                        {
                            if (!paused)
                                mpeg_pause();
#ifdef HAVE_PLAYER_KEYPAD
                            lcd_stop_scroll();
#endif
                            status_set_playmode(STATUS_FASTFORWARD);
                            status_draw();
                            ff_rewind = true;
                            ff_rewind_count = global_settings.ff_rewind*1000;
                        }
                        else
                            break;
                    }

                    if ((id3->elapsed + ff_rewind_count) > id3->length)
                        ff_rewind_count = id3->length - id3->elapsed;

                    display_file_time(id3->elapsed + ff_rewind_count,
                                      id3->length);
                }
                break;

            case BUTTON_LEFT | BUTTON_REL:
                if (menu_button_is_down && keys_locked)
                {
                    sleep(HZ/6);
                    draw_screen(id3);
                }
                if (!keys_locked)
                {
                    if (ff_rewind)
                    {
                        /* rewind */
                        mpeg_ff_rewind(ff_rewind_count);
                        ff_rewind_count = 0;
                        ff_rewind = false;
                        if (paused)
                            status_set_playmode(STATUS_PAUSE);
                        else {
                            mpeg_resume();
                            status_set_playmode(STATUS_PLAY);
                        }
#ifdef HAVE_LCD_CHARCELLS
                        draw_screen(id3);
#endif
                    }
#ifdef HAVE_PLAYER_KEYPAD
                    else if(!menu_button_is_down)
#else
                    else
#endif
                    {
                        if (id3->elapsed < 3*1000)
                            mpeg_prev();
                        else {
                            mpeg_pause();
                            mpeg_ff_rewind(-(id3->elapsed));
                            mpeg_resume();
                        }
                    }
                }
#ifdef HAVE_PLAYER_KEYPAD
                else if(!menu_button_is_down)
#else
                else
#endif
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                }
                break;

            case BUTTON_RIGHT | BUTTON_REL:
                if (menu_button_is_down && keys_locked)
                {
                    sleep(HZ/6);
                    draw_screen(id3);
                }
                if (!keys_locked)
                {
                    if (ff_rewind)
                    {
                        /* fast forward */
                        mpeg_ff_rewind(ff_rewind_count);
                        ff_rewind_count = 0;
                        ff_rewind = false;
                        if (paused)
                            status_set_playmode(STATUS_PAUSE);
                        else {
                            mpeg_resume();
                            status_set_playmode(STATUS_PLAY);
                        }
#ifdef HAVE_LCD_CHARCELLS
                        draw_screen(id3);
#endif
                    }
#ifdef HAVE_PLAYER_KEYPAD
                    else if(!menu_button_is_down)
#else
                    else
#endif
                    {
                        mpeg_next();
                    }
                }
#ifdef HAVE_PLAYER_KEYPAD
                else if(!menu_button_is_down)
#else
                else
#endif
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }
                break;

#ifdef HAVE_PLAYER_KEYPAD
            case BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
                dont_go_to_menu = true;
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                display_volume_level(global_settings.volume);
                draw_screen(id3);
                status_draw();
                settings_save();
                break;

            case BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
                dont_go_to_menu = true;
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                display_volume_level(global_settings.volume);
                draw_screen(id3);
                status_draw();
                settings_save();
                break;

            case BUTTON_MENU | BUTTON_ON:
                if (keys_locked)
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }
                lcd_stop_scroll();
                dont_go_to_menu = true;
                menu_button_is_down = false;
                lcd_icon(ICON_PARAM, true);
                lcd_icon(ICON_AUDIO, true);
                retval = player_id3_show();
                if(retval == SYS_USB_CONNECTED)
                    return SYS_USB_CONNECTED;
                lcd_icon(ICON_PARAM, false);
                lcd_icon(ICON_AUDIO, true);
                draw_screen(id3);
                break;

            case BUTTON_MENU:
                lcd_icon(ICON_PARAM, true);
                lcd_icon(ICON_AUDIO, false);
                menu_button_is_down = true;
                break;

            case BUTTON_PLAY | BUTTON_REL:
                /* The PLAY key has been release while the MENU key
                   was held */
                if(menu_button_is_down)
                    pending_mute = !pending_mute;
                break;

            case BUTTON_STOP | BUTTON_REL:
                /* The STOP key has been release while the MENU key
                   was held */
                if(menu_button_is_down)
                    pending_keylock = !pending_keylock;
                break;
#else
            case BUTTON_F1:
                menu_button_is_down = true;
                break;
 
            case BUTTON_UP | BUTTON_REL:
                /* The UP key has been release while the F1 key
                   was held */
                if(menu_button_is_down)
                {
                    pending_mute = !pending_mute;
                    debugf("pending: %d\n", pending_mute);
                }
                break; 
              
            case BUTTON_DOWN | BUTTON_REL:
                /* The DOWN key has been release while the F1 key
                   was held */
                if(menu_button_is_down)
                {
                    pending_keylock = !pending_keylock;
                    debugf("pending: %d\n", pending_keylock);
                }
                break;
#endif

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_UP:
#else
            case BUTTON_MENU | BUTTON_PLAY:
#endif
                if(keys_locked)
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }

                if(device_muted != pending_mute)
                {
                    device_muted = pending_mute;

                    if(device_muted)
                        mpeg_sound_set(SOUND_VOLUME, 0);
                    else
                        mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                    display_mute_text(device_muted);
                    restore = true;
                }
                dont_go_to_menu = true;
                break;
                    
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_DOWN:
#else
            case BUTTON_MENU | BUTTON_STOP:
#endif
                if(keys_locked != pending_keylock)
                {
                    keys_locked = pending_keylock;

#ifdef HAVE_LCD_CHARCELLS
                    if(keys_locked)
                        lcd_icon(ICON_RECORD, true);
                    else
                        lcd_icon(ICON_RECORD, false);
#endif
                    display_keylock_text(keys_locked);
                    restore = true;
                }

                dont_go_to_menu = true;
                break;
                    
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_REL:
#else
            case BUTTON_MENU | BUTTON_REL:
#endif
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_PARAM, false);
#endif
                if(!keys_locked && !dont_go_to_menu && menu_button_is_down)
                {
                    lcd_stop_scroll();
                    button_set_release(old_release_mask);
                    main_menu();
                    old_release_mask = button_set_release(RELEASE_MASK);
                    ignore_keyup = true;
                    id3 = mpeg_current_track();
                    restore = true;
                }
                else
                {
                    dont_go_to_menu = false;
                }
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_AUDIO, true);
#endif
                menu_button_is_down = false;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F3:
#ifdef HAVE_LCD_BITMAP
                global_settings.statusbar = !global_settings.statusbar;
                settings_save();
                if(global_settings.statusbar)
                    lcd_setmargins(0, STATUSBAR_HEIGHT);
                else
                    lcd_setmargins(0, 0);
                restore = true;
#endif
                break;
#endif

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
#else
            case BUTTON_STOP:
#endif
                if (keys_locked)
                {
                    display_keylock_text(keys_locked);
                    restore = true;
                    break;
                }

#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
                lcd_icon(ICON_AUDIO, false);
#endif
                mpeg_stop();
                status_set_playmode(STATUS_STOP);
                button_set_release(old_release_mask);
                return 0;
                    
#ifndef SIMULATOR
            case SYS_USB_CONNECTED: {
#ifdef HAVE_LCD_BITMAP
                bool laststate=statusbar(false);
#endif
                /* Tell the USB thread that we are safe */
                DEBUGF("wps got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    
                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&button_queue);

#ifdef HAVE_LCD_BITMAP
                statusbar(laststate);
#endif
                /* Signal to our caller that we have been in USB mode */
                return SYS_USB_CONNECTED;
                break;
            }
#endif
            case BUTTON_NONE: /* Timeout */
                if (mpeg_is_playing() && id3)
                    display_file_time(id3->elapsed, id3->length);
                
                /* save resume data */
                if ( id3 && 
                     global_settings.resume &&
                     global_settings.resume_offset != id3->offset ) {
                    DEBUGF("R%X,%X (%X)\n",global_settings.resume_offset,id3->offset,id3);
                    global_settings.resume_index = id3->index;
                    global_settings.resume_offset = id3->offset;
                    settings_save();
                }

                status_draw();
                break;
        }

        if(restore) {
            draw_screen(id3);
            if (mpeg_is_playing() && id3)
                display_file_time(id3->elapsed, id3->length);
        }
    }
}
