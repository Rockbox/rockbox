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

#define FF_REWIND_MIN_STEP 1000 /* minimum ff/rewind step is 1 second */
#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */ 

#ifdef HAVE_LCD_BITMAP
    #define PLAY_DISPLAY_2LINEID3        0 
    #define PLAY_DISPLAY_FILENAME_SCROLL 1 
    #define PLAY_DISPLAY_TRACK_TITLE     2 
    #define PLAY_DISPLAY_CUSTOM_WPS      3 
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
static bool ff_rewind = false;
static bool paused = false;
static int ff_rewind_count = 0;
static struct mp3entry* id3 = NULL;
static int old_release_mask;

#ifdef CUSTOM_WPS
static char custom_wps[5][64];
static char wps_display[5][64];
static int scroll_line;
static int scroll_line_custom;
#endif

static void draw_screen(void)
{
    int font_height;

#ifdef LOADABLE_FONTS
    unsigned char *font = lcd_getcurrentldfont();
    font_height = ajf_get_fontheight(font);
#else
    font_height = 8;
#endif

    lcd_clear_display();
    if(!id3 && !mpeg_is_playing())
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
#ifdef CUSTOM_WPS
        static int last_wps = -1;
        if ((last_wps != global_settings.wps_display
            && global_settings.wps_display == PLAY_DISPLAY_CUSTOM_WPS))
        {
            load_custom_wps();
            last_wps = global_settings.wps_display;
        }
#endif
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
#ifdef CUSTOM_WPS
                int tmpcnt = 0;
#endif

                szBuff[sizeof(szBuff)-1] = 0;
                strncpy(szBuff, id3->path, sizeof(szBuff));

                szTok = strtok_r(szBuff, "/", &end);
                szTok = strtok_r(NULL, "/", &end);

                /* Assume path format of: Genre/Artist/Album/Mp3_file */
                strncpy(szArtist,szTok,sizeof(szArtist));
                szArtist[sizeof(szArtist)-1] = 0;
                szDelimit = strrchr(id3->path, ch);
                lcd_puts(0, 0, szArtist?szArtist:"<nothing>");

                /* removes the .mp3 from the end of the display buffer */
                szPeriod = strrchr(szDelimit, '.');
                if (szPeriod != NULL)
                    *szPeriod = 0;
#ifdef CUSTOM_WPS
                snprintf(wps_display[0],sizeof(wps_display[0]),"%s",
                        (++szDelimit));
#ifdef HAVE_LCD_CHARCELLS
                snprintf(wps_display[1],sizeof(wps_display[1]),"%s",
                        "%pc/%pt");
#endif
                for(tmpcnt=2;tmpcnt<=5;tmpcnt++)
                    wps_display[tmpcnt][0] = 0;
                scroll_line = 0;
                refresh_wps(false);
#else
                lcd_puts_scroll(0, 1, (++szDelimit));
#endif
                break;
            }
            case PLAY_DISPLAY_FILENAME_SCROLL:
            {
#ifdef CUSTOM_WPS
                snprintf(wps_display[0],sizeof(wps_display[0]),"%s",
                        "%pp/%pe: %fn");
#ifdef HAVE_LCD_CHARCELLS
                snprintf(wps_display[1],sizeof(wps_display[1]),"%s",
                        "%pc/%pt");
#endif
                scroll_line = 0;
                refresh_wps(false);
#else
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
#endif
                break;
            }
            case PLAY_DISPLAY_2LINEID3:
            {
#ifdef HAVE_LCD_BITMAP
                int l = 0;

#ifdef CUSTOM_WPS
                snprintf(wps_display[l],sizeof(wps_display[l]),"%s","%fn");
                snprintf(wps_display[l++],sizeof(wps_display[l]),"%s","%it");
                snprintf(wps_display[l++],sizeof(wps_display[l]),"%s","%id");
                snprintf(wps_display[l++],sizeof(wps_display[l]),"%s","%ia");
                if(!global_settings.statusbar && font_height <= 8)
                {
                    if(id3->vbr)
                        snprintf(wps_display[l++],sizeof(wps_display[l]),"%s",
                                "%fb kbit (avg)");
                    else
                        snprintf(wps_display[l],sizeof(wps_display[l]),"%s",
                                "%fb kbit");
                    snprintf(wps_display[l],sizeof(wps_display[l]),"%s",
                            "%ff Hz");
                }
                else
                {
                    if(id3->vbr)
                        snprintf(wps_display[l++],sizeof(wps_display[l]),"%s",
                                "%fb kbit(a) %ffHz");
                    else
                        snprintf(wps_display[l],sizeof(wps_display[l]),"%s",
                                "%fb kbit    %ffHz");
                }
                scroll_line = 0;
                refresh_wps(false);
#else
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
                        snprintf(buffer, sizeof(buffer), "%d kbit", 
                                 id3->bitrate);

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
#endif
#else
#ifdef CUSTOM_WPS
                snprintf(wps_display[0],sizeof(wps_display[0]),"%s","%ia");
                snprintf(wps_display[1],sizeof(wps_display[1]),"%s","%it");
                scroll_line = 1;
                refresh_wps(false);
#else
                lcd_puts(0, 0, id3->artist?id3->artist:"<no artist>");
                lcd_puts_scroll(0, 1, id3->title?id3->title:"<no title>");
#endif
#endif
                break;
            }
#ifdef HAVE_LCD_CHARCELLS
            case PLAY_DISPLAY_1LINEID3:
            {
#ifdef CUSTOM_WPS
                snprintf(wps_display[0],sizeof(wps_display[0]),"%s",
                        "%pp/%pe: %fc");
                snprintf(wps_display[1],sizeof(wps_display[1]),"%s",
                        "%pc/%pt");
                scroll_line = 0;
                refresh_wps(false);
#else
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
#endif
                break;
            }
#endif
#ifdef CUSTOM_WPS
            case PLAY_DISPLAY_CUSTOM_WPS:
            {
                if(custom_wps[0] == 0)
                {
                    snprintf(wps_display[0],sizeof(wps_display[0]),"%s",
                        "Couldn't Load Custom WPS");
                    snprintf(wps_display[1],sizeof(wps_display[1]),"%s",
                            "%pc/%pt");
                }
                refresh_wps(false);
                break;
            }
#endif
        }
    }
    status_draw();
    lcd_update();
}

#ifdef CUSTOM_WPS
bool refresh_wps(bool refresh_scroll)
{
    int l;
#ifdef CUSTOM_WPS
#ifdef HAVE_LCD_BITMAP
    int bmp_time_line;
#endif
#endif

    if(!id3)
    {
        lcd_stop_scroll();
        lcd_clear_display();
        return(false);
    }

#ifdef HAVE_LCD_CHARCELL
    for(l = 0; l <= 1; l++)
#else
    for(l = 0; l <= 5; l++)
#endif
    {
        if(global_settings.wps_display == PLAY_DISPLAY_CUSTOM_WPS)
        {
            scroll_line = scroll_line_custom;
            if(scroll_line != l)
                    display_custom_wps(0, l, false, custom_wps[l]);
            else
                if(refresh_scroll)
                        display_custom_wps(0, l, true, custom_wps[l]);
        }
        else
        {
            if(scroll_line != l)
                    display_custom_wps(0, l, false, wps_display[l]);
            if(scroll_line == l && refresh_scroll)
                    display_custom_wps(0, l, true, wps_display[l]);                
        }
    }
#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        bmp_time_line = 5;
    else
        bmp_time_line = 6;
    snprintf(wps_display[bmp_time_line],sizeof(wps_display[bmp_time_line]),
             "%s","Time: %pc/%pt");

    slidebar(0, LCD_HEIGHT-6, LCD_WIDTH, 6, id3->elapsed*100/id3->length, Grow_Right);
    lcd_update();
#endif
    return(true);
}

bool load_custom_wps(void)
{
#ifdef SIMULATOR
    snprintf(custom_wps[0],sizeof(custom_wps[0]),"%s","%s%pp/%pe: %?%it - %ia%:%fn%?");
    snprintf(custom_wps[1],sizeof(custom_wps[1]),"%s","%pc/%pt");
    snprintf(custom_wps[2],sizeof(custom_wps[2]),"%s","%it");
    snprintf(custom_wps[3],sizeof(custom_wps[3]),"%s","%id");
    snprintf(custom_wps[4],sizeof(custom_wps[4]),"%s","%ia");
    snprintf(custom_wps[5],sizeof(custom_wps[5]),"%s","%id");
    scroll_line_custom = 0;
    return(true);
#else
    int fd;
    int l = 0;
    int numread = 1;
    char cchr[0];

    for(l=0;l<=5;l++)
    {
         custom_wps[l][0] = 0;
    }
    l = 0;

    fd = open("/wps.config", O_RDONLY);
    if(-1 == fd)
    {
        close(fd);
        return(false);
    }

    while(l<=5)
    {
        numread = read(fd, cchr, 1);
        if(numread==0)
            break;
        switch(cchr[0])
        {
            case 10: /* LF */
                l++;
                break;
            case 13: /* CR ... Ignore it */
                break;                
            default:
                snprintf(custom_wps[l], sizeof(custom_wps[l]), "%s%c", custom_wps[l], cchr[0]);
                break;
        }
    }
    close(fd);

    scroll_line_custom = 0;
    for(l=0;l<=5;l++)
    {
        if(custom_wps[l][0] == '%' && custom_wps[l][1] == 's')
            scroll_line_custom = l;
    }
    return(true);
#endif
}

bool display_custom_wps(int x_val, int y_val, bool do_scroll, char *wps_string)
{
    char buffer[128];
    char tmpbuf[64];
    int con_flag = 0;  /* (0)Not inside of if/else
                          (1)Inside of If
                          (2)Inside of Else */
    char con_if[64];
    char con_else[64];
    char cchr1;
    char cchr2;
    char cchr3;
    unsigned int seek;

    char* szLast;

    szLast = strrchr(id3->path, '/');
    if(szLast)
        /* point to the first letter in the file name */
        szLast++;

    buffer[0] = 0;

    seek = -1;
    while(1)
    {
        seek++;
        cchr1 = wps_string[seek];
        tmpbuf[0] = 0;
        switch(cchr1)
        {
            case '%':
                seek++;
                cchr2 = wps_string[seek];
                switch(cchr2)
                {
                    case 'i':  /* ID3 Information */
                        seek++;
                        cchr3 = wps_string[seek];
                        switch(cchr3)
                        {
                            case 't':  /* ID3 Title */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%s", 
                                         id3->title?id3->title:"<no title>");
                                break;
                            case 'a':  /* ID3 Artist */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%s", 
                                         id3->artist?id3->artist:"<no artist>");
                                break;
                            case 'n':  /* ID3 Track Number */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%d", id3->tracknum);
                                break;
                            case 'd':  /* ID3 Album/Disc */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%s", id3->album);
                                break;
                        }
                        break;
                    case 'f':  /* File Information */
                        seek++;
                        cchr3 = wps_string[seek];
                        switch(cchr3)
                        {
                            case 'c':  /* Conditional Filename \ ID3 Artist-Title */
                                if(id3->artist && id3->title)
                                    snprintf(tmpbuf, sizeof(tmpbuf), "%s - %s",
                                             id3->artist?id3->artist:"<no artist>",
                                             id3->title?id3->title:"<no title>");
                                else
                                    snprintf(tmpbuf, sizeof(tmpbuf), "%s",
                                             szLast?szLast:id3->path);
                                break;
                            case 'd':  /* Conditional Filename \ ID3 Title-Artist */
                                if(id3->artist && id3->title)
                                    snprintf(tmpbuf, sizeof(tmpbuf), "%s - %s",
                                             id3->title?id3->title:"<no title>",
                                             id3->artist?id3->artist:"<no artist>");
                                else
                                    snprintf(tmpbuf, sizeof(tmpbuf), "%s",
                                             szLast?szLast:id3->path);
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
                            case 'n':  /* File Name */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%s",
                                         szLast?szLast:id3->path);
                                break;
                            case 's':  /* File Size (In Kilobytes) */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%d",
                                         id3->filesize / 1024);
                                break;
                        }
                        break;
                    case 'p':  /* Playlist/Song Information */
                        seek++;
                        cchr3 = wps_string[seek];
                        switch(cchr3)
                        {
                            case 'p':  /* Playlist Position */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%d", id3->index + 1);
                                break;
                            case 'e':  /* Playlist Total Entries */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%d", playlist.amount);
                                break;
                            case 'c':  /* Current Time in Song */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%d:%02d",
                                         (id3->elapsed + ff_rewind_count) / 60000,
                                         (id3->elapsed + ff_rewind_count) % 60000 / 1000);
                                break;
                            case 't':  /* Total Time */
                                snprintf(tmpbuf, sizeof(tmpbuf), "%d:%02d",
                                         id3->length / 60000,
                                         id3->length % 60000 / 1000);
                                break;
                        }
                        break;
                    case '%':  /* Displays % */
                        snprintf(tmpbuf, sizeof(tmpbuf), "%%");
                        break;
                    case '?':  /* Conditional Display of ID3/File */
                        switch(con_flag)
                        {
                            case 0:
                                con_if[0] = 0;
                                con_else[0] = 0;
                                con_flag = 1;
                                break;
                            default:
                                if(id3->artist && id3->title)
                                    snprintf(tmpbuf, sizeof(tmpbuf), "%s", con_if);
                                else
                                    snprintf(tmpbuf, sizeof(tmpbuf), "%s", con_else);
                                con_flag = 0;
                                break;
                        }
                        break;
                    case ':': /* Seperator for Conditional ID3/File Display */
                        con_flag = 2;
                        break;
                }
                switch(con_flag)
                {
                    case 0:
                        snprintf(buffer, sizeof(buffer), "%s%s", buffer, tmpbuf);
                        break;
                    case 1:
                        snprintf(con_if, sizeof(con_if), "%s%s", con_if, tmpbuf);
                        break;
                    case 2:
                        snprintf(con_else, sizeof(con_else), "%s%s", con_else, tmpbuf);
                        break;
                }
                break;
            default:
                switch(con_flag)
                {
                    case 0:
                        snprintf(buffer, sizeof(buffer), "%s%c", buffer, cchr1);
                        break;
                    case 1:
                        snprintf(con_if, sizeof(con_if), "%s%c", con_if, cchr1);
                        break;
                    case 2:
                        snprintf(con_else, sizeof(con_else), "%s%c", con_else, cchr1);
                        break;
                }
                break;
        }
        if(seek >= strlen(wps_string))
        {
            if(do_scroll)
            {
                lcd_stop_scroll();
                lcd_puts_scroll(x_val, y_val, buffer);
            }
            else
                lcd_puts(x_val, y_val, buffer);
            return(true);
        }
    }
    return(true);
}
#endif

int player_id3_show(void)
{
#ifdef HAVE_PLAYER_KEYPAD
    int button;
    int menu_pos = 0;
    int menu_max = 6;
    bool menu_changed = true;
    char scroll_text[MAX_PATH];

    lcd_stop_scroll();
    lcd_clear_display();
    lcd_puts(0, 0, "-ID3 Info- ");
    lcd_puts(0, 1, "--Screen-- ");
    sleep(HZ*1.5);
 
    while(1)
    {
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
                draw_screen();
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
                lcd_puts(0, 0, "[Title]");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->title?id3->title:"<no title>");
                break;
            case 1:
                lcd_puts(0, 0, "[Artist]");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->artist?id3->artist:"<no artist>");
                break;
            case 2:
                lcd_puts(0, 0, "[Album]");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->album?id3->album:"<no album>");
                break;
            case 3:
                lcd_puts(0, 0, "[Length]");
                snprintf(scroll_text,sizeof(scroll_text), "%d:%02d",
                   id3->length / 60000,
                   id3->length % 60000 / 1000 );
                break;
            case 4:
                lcd_puts(0, 0, "[Bitrate]");
                snprintf(scroll_text,sizeof(scroll_text), "%d kbps", 
                        id3->bitrate);
                break;
            case 5:
                lcd_puts(0, 0, "[Frequency]");
                snprintf(scroll_text,sizeof(scroll_text), "%d kHz",
                        id3->frequency);
                break;
            case 6:
                lcd_puts(0, 0, "[Path]");
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

        lcd_update();
        yield();
    }
#endif
    return(0);
}

#ifndef CUSTOM_WPS
static void display_file_time(void)
{
    char buffer[32];
    int elapsed = id3->elapsed + ff_rewind_count;

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
             id3->length / 60000,
             id3->length % 60000 / 1000 );

    lcd_puts(0, line, buffer);
    slidebar(0, LCD_HEIGHT-6, LCD_WIDTH, 6, 
             elapsed*100/id3->length, Grow_Right);
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
                 id3->length / 60000,
                 id3->length % 60000 / 1000 );

        lcd_puts(0, 1, buffer);
        lcd_update();
    }
#endif
}
#endif

void display_volume_level(int vol_level)
{
    char buffer[32];

    lcd_stop_scroll();
    snprintf(buffer,sizeof(buffer),"Vol: %d %%       ", vol_level * 2);

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
    lcd_update();
#endif
    
    sleep(HZ);
}

static void handle_usb(void)
{
#ifndef SIMULATOR
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
#endif
}

static bool ffwd_rew(int button)
{
    unsigned int ff_rewind_step = 0; /* current rewind step size */ 
    unsigned int ff_rewind_max_step = 0; /* max rewind step size */ 
    long ff_rewind_accel_tick = 0; /* next time to bump ff/rewind step size */ 
    bool exit = false;
    bool usb = false;

    while (!exit) {
        switch ( button ) {
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (ff_rewind)
                {
                    ff_rewind_count -= ff_rewind_step; 
                    if (global_settings.ff_rewind_accel != 0 && 
                        current_tick >= ff_rewind_accel_tick) 
                    { 
                        ff_rewind_step *= 2; 
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    } 
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
                        ff_rewind = true;
                        ff_rewind_max_step = 
                            id3->length * FF_REWIND_MAX_PERCENT / 100; 
                        ff_rewind_step = FF_REWIND_MIN_STEP;
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_count = -ff_rewind_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    }
                    else
                        break;
                }

                if ((int)(id3->elapsed + ff_rewind_count) < 0)
                    ff_rewind_count = -id3->elapsed;

#ifdef CUSTOM_WPS
                refresh_wps(false);
#else
                display_file_time();
#endif
                break;

            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (ff_rewind)
                {
                    ff_rewind_count += ff_rewind_step; 
                    if (global_settings.ff_rewind_accel != 0 && 
                        current_tick >= ff_rewind_accel_tick) 
                    { 
                        ff_rewind_step *= 2; 
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    } 
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
                        ff_rewind = true;
                        ff_rewind_max_step = 
                            id3->length * FF_REWIND_MAX_PERCENT / 100; 
                        ff_rewind_step = FF_REWIND_MIN_STEP; 
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_count = ff_rewind_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    }
                    else
                        break;
                }

                if ((id3->elapsed + ff_rewind_count) > id3->length)
                    ff_rewind_count = id3->length - id3->elapsed;

#ifdef CUSTOM_WPS
                refresh_wps(false);
#else
                display_file_time();
#endif
                break;

            case BUTTON_LEFT | BUTTON_REL:
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
                draw_screen();
#endif
                exit = true;
                break;

            case BUTTON_RIGHT | BUTTON_REL: 
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
                draw_screen();
#endif
                exit = true;
                break;

            case SYS_USB_CONNECTED:
                handle_usb();
                usb = true;
                exit = true;
                break;
        }
        if (!exit)
            button = button_get(true);
    }

    return usb;
}

static void update(void)
{
    if (mpeg_has_changed_track())
    {
        lcd_stop_scroll();
        id3 = mpeg_current_track();
        draw_screen();
#ifdef CUSTOM_WPS
        refresh_wps(true);
#endif
    }

    if (id3) {
#ifdef CUSTOM_WPS
        refresh_wps(false);
#else
        display_file_time();
#endif
    }

    status_draw();

    /* save resume data */
    if ( id3 && 
         global_settings.resume &&
         global_settings.resume_offset != id3->offset ) {
        DEBUGF("R%X,%X (%X)\n", global_settings.resume_offset,
               id3->offset,id3);
        global_settings.resume_index = id3->index;
        global_settings.resume_offset = id3->offset;
        settings_save();
    }
}


static bool keylock(void)
{
    bool exit = false;

#ifdef HAVE_LCD_CHARCELLS
    lcd_icon(ICON_RECORD, true);
    lcd_icon(ICON_PARAM, false);
#endif
    display_keylock_text(true);
    keys_locked = true;
#ifdef CUSTOM_WPS
    refresh_wps(true);
#endif
    draw_screen();
    status_draw();
    while (button_get(false)); /* clear button queue */

    while (!exit) {
        switch ( button_get_w_tmo(HZ/5) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_DOWN:
#else
            case BUTTON_MENU | BUTTON_STOP:
#endif
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
#endif
                display_keylock_text(false);
                keys_locked = false;
                exit = true;
                while (button_get(false)); /* clear button queue */
                break;

            case SYS_USB_CONNECTED:
                handle_usb();
                return true;

            case BUTTON_NONE:
                update();
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1:
#else
            case BUTTON_MENU:
#endif
                /* ignore menu key, to avoid displaying "Keylock ON"
                   every time we unlock the keys */
                break;

            default:
                display_keylock_text(true);
                while (button_get(false)); /* clear button queue */
#ifdef CUSTOM_WPS
                refresh_wps(true);
#endif
                draw_screen();
                break;
        }
    }
    return false;
}

static bool menu(void)
{
    static bool muted = false;
    bool exit = false;
    int last_button = 0;

#ifdef HAVE_LCD_CHARCELLS
    lcd_icon(ICON_PARAM, true);
#endif

    while (!exit) {
        int button = button_get(true);
        
        switch ( button ) {
            /* go into menu */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_REL:
#else
            case BUTTON_MENU | BUTTON_REL:
#endif
                exit = true;
                if ( !last_button ) {
                    lcd_stop_scroll();
                    button_set_release(old_release_mask);
                    main_menu();
                    old_release_mask = button_set_release(RELEASE_MASK);
                }
                break;

                /* mute */
#ifdef HAVE_PLAYER_KEYPAD
            case BUTTON_MENU | BUTTON_PLAY:
#else
            case BUTTON_MENU | BUTTON_UP:
#endif
                if ( muted )
                    mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                else
                    mpeg_sound_set(SOUND_VOLUME, 0);
                muted = !muted;
                lcd_icon(ICON_PARAM, false);
                display_mute_text(muted);
                break;

                /* key lock */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_DOWN:
#else
            case BUTTON_MENU | BUTTON_STOP:
#endif
                if (keylock())
                    return true;
                exit = true;
                break;

#ifdef HAVE_PLAYER_KEYPAD
                /* change volume */
            case BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                display_volume_level(global_settings.volume);
                draw_screen();
                status_draw();
                settings_save();
                break;

                /* change volume */
            case BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                display_volume_level(global_settings.volume);
                draw_screen();
                status_draw();
                settings_save();
                break;

                /* show id3 tags */
            case BUTTON_MENU | BUTTON_ON:
                lcd_stop_scroll();
                lcd_icon(ICON_PARAM, true);
                lcd_icon(ICON_AUDIO, true);
                if(player_id3_show() == SYS_USB_CONNECTED)
                    return true;
                lcd_icon(ICON_PARAM, false);
                lcd_icon(ICON_AUDIO, true);
                draw_screen();
                exit = true;
                break;
#endif

            case SYS_USB_CONNECTED:
                handle_usb();
                return true;
        }
        last_button = button;
    }

#ifdef HAVE_LCD_CHARCELLS
    lcd_icon(ICON_PARAM, false);
#endif

    draw_screen();
#ifdef CUSTOM_WPS
    refresh_wps(true);
#endif
    return false;
}

/* demonstrates showing different formats from playtune */
int wps_show(void)
{
    int button;
    bool ignore_keyup = true;
    bool restore = false;

    id3 = NULL;

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
    ff_rewind_count = 0;

#ifdef CUSTOM_WPS
    load_custom_wps();  /* Load the Custom WPS file, if there is one */
#endif

    if(mpeg_is_playing())
    {
        id3 = mpeg_current_track();
        if (id3) {
            draw_screen();
#ifdef CUSTOM_WPS
            refresh_wps(true);
#else
            display_file_time();
#endif
        }
        restore = true;
    }

    while ( 1 )
    {
        button = button_get_w_tmo(HZ/5);

        /* discard first event if it's a button release */
        if (button && ignore_keyup)
        {
            ignore_keyup = false;
            if (button & BUTTON_REL && button != SYS_USB_CONNECTED)
                continue;
        }
        
        switch(button)
        {
            /* exit to dir browser */
            case BUTTON_ON:
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
                lcd_icon(ICON_AUDIO, false);
#endif
                button_set_release(old_release_mask);
                return 0;
                
                /* play/pause */
            case BUTTON_PLAY:
                if ( paused )
                {
                    mpeg_resume();
                    paused = false;
                    status_set_playmode(STATUS_PLAY);
                }
                else
                {
                    mpeg_pause();
                    paused = true;
                    status_set_playmode(STATUS_PAUSE);
                    if (global_settings.resume) {
                        settings_save();
#ifndef HAVE_RTC
                        ata_flush();
#endif
                    }
                }
                break;

                /* volume up */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#endif
            case BUTTON_VOL_UP:
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw();
                settings_save();
                break;

                /* volume down */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#endif
            case BUTTON_VOL_DOWN:
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw();
                settings_save();
                break;

                /* fast forward / rewind */
            case BUTTON_LEFT | BUTTON_REPEAT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                ffwd_rew(button);
                break;

                /* prev / restart */
            case BUTTON_LEFT | BUTTON_REL:
                if (!id3 || (id3->elapsed < 3*1000))
                    mpeg_prev();
                else {
                    if (!paused)
                        mpeg_pause();

                    mpeg_ff_rewind(-(id3->elapsed));

                    if (!paused)
                        mpeg_resume();
                }
                break;

                /* next */
            case BUTTON_RIGHT | BUTTON_REL:
                mpeg_next();
                break;

                /* menu key functions */
#ifdef HAVE_PLAYER_KEYPAD
            case BUTTON_MENU:
#else
            case BUTTON_F1:
#endif
                if (menu())
                    return SYS_USB_CONNECTED;
                break;

                /* toggle status bar */
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

                /* stop and exit wps */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
#else
            case BUTTON_STOP:
#endif
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
                lcd_icon(ICON_AUDIO, false);
#endif
                mpeg_stop();
                status_set_playmode(STATUS_STOP);
                button_set_release(old_release_mask);
                return 0;
                    
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                handle_usb();
                return SYS_USB_CONNECTED;
#endif

            case BUTTON_NONE: /* Timeout */
                update();
                break;
        }

        if ( button )
            ata_spin();

        if (restore) {
            restore = false;
            draw_screen();
            if (id3)
#ifdef CUSTOM_WPS
                refresh_wps(false);
#else
                display_file_time();
#endif
        }
    }
}
