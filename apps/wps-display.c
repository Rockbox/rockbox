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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lcd.h"
#include "mpeg.h"
#include "id3.h"
#include "settings.h"
#include "playlist.h"
#include "kernel.h"
#include "status.h"
#include "wps-display.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

#ifdef LOADABLE_FONTS
#include "ajf.h"
#endif

#define WPS_CONFIG "/wps.config"

#ifdef HAVE_LCD_BITMAP
    #define PLAY_DISPLAY_2LINEID3        0 
    #define PLAY_DISPLAY_FILENAME_SCROLL 1 
    #define PLAY_DISPLAY_TRACK_TITLE     2 
    #define PLAY_DISPLAY_CUSTOM_WPS      3 
#else
    #define PLAY_DISPLAY_1LINEID3        0 
    #define PLAY_DISPLAY_1LINEID3_PLUS   1
    #define PLAY_DISPLAY_2LINEID3        2 
    #define PLAY_DISPLAY_FILENAME_SCROLL 3 
    #define PLAY_DISPLAY_TRACK_TITLE     4 
    #define PLAY_DISPLAY_CUSTOM_WPS      5 
#endif

#define LINE_LEN 64

static int ff_rewind_count;
static char custom_wps[5][LINE_LEN];
static char display[5][LINE_LEN];
static int scroll_line;
static int scroll_line_custom;
bool wps_time_countup = true;

static bool load_custom_wps(void)
{
    int fd;
    int l = 0;
    int numread = 1;
    char cchr[0];

    for (l=0;l<=5;l++)
         custom_wps[l][0] = 0;

    l = 0;

    fd = open(WPS_CONFIG, O_RDONLY);
    if (-1 == fd)
    {
        close(fd);
        return false;
    }

    while(l<=5)
    {
        numread = read(fd, cchr, 1);
        if (numread==0)
            break;

        switch (cchr[0])
        {
            case '\n': /* LF */
                l++;
                break;

            case '\r': /* CR ... Ignore it */
                break;                

            default:
                snprintf(custom_wps[l], LINE_LEN,
                         "%s%c", custom_wps[l], cchr[0]);
                break;
        }
    }
    close(fd);

    scroll_line_custom = 0;
    for (l=0;l<=5;l++)
    {
        if (custom_wps[l][0] == '%' && custom_wps[l][1] == 's')
            scroll_line_custom = l;
    }
    return true;
}

static bool display_custom_wps( struct mp3entry* id3, 
                                int x_val, 
                                int y_val, 
                                bool do_scroll, 
                                char *wps_string)
{
    char bigbuf[LINE_LEN*2];
    char buf[LINE_LEN];
    int i;
    int con_flag = 0;  /* (0)Not inside of if/else
                          (1)Inside of If
                          (2)Inside of Else */
    char con_if[LINE_LEN];
    char con_else[LINE_LEN];
    char cchr1;
    char cchr2;
    char cchr3;
    unsigned int seek;

    char* szLast;

    szLast = strrchr(id3->path, '/');
    if (szLast)
        /* point to the first letter in the file name */
        szLast++;

    bigbuf[0] = 0;

    seek = -1;
    while(1)
    {
        seek++;
        cchr1 = wps_string[seek];
        buf[0] = 0;
        if (cchr1 == '%')
        {
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
                            strncpy(buf, 
                                    id3->title ? id3->title : "<no title>",
                                    LINE_LEN);
                            break;
                        case 'a':  /* ID3 Artist */
                            strncpy(buf, 
                                    id3->artist ? id3->artist : "<no artist>",
                                    LINE_LEN);
                            break;
                        case 'n':  /* ID3 Track Number */
                            snprintf(buf, LINE_LEN, "%d", 
                                     id3->tracknum);
                            break;
                        case 'd':  /* ID3 Album/Disc */
                            strncpy(buf, id3->album ? id3->album : "<no album>", LINE_LEN);
                            break;
                    }
                    break;
                case 'f':  /* File Information */
                    seek++;
                    cchr3 = wps_string[seek];
                    switch(cchr3)
                    {
                        case 'c': /* Conditional Filename \ ID3 Artist-Title */
                            if (id3->artist && id3->title)
                                snprintf(buf, LINE_LEN, "%s - %s",
                                         id3->artist?id3->artist:"<no artist>",
                                         id3->title?id3->title:"<no title>");
                            else
                                strncpy(buf, 
                                        szLast ? szLast : id3->path,
                                        LINE_LEN );
                            break;

                        case 'd': /* Conditional Filename \ ID3 Title-Artist */
                            if (id3->artist && id3->title)
                                snprintf(buf, LINE_LEN, "%s - %s",
                                         id3->title?id3->title:"<no title>",
                                         id3->artist?id3->artist:"<no artist>");
                            else
                                strncpy(buf, szLast ? szLast : id3->path,
                                        LINE_LEN);
                            break;

                        case 'b':  /* File Bitrate */
                            snprintf(buf, LINE_LEN, "%d", id3->bitrate);
                            break;

                        case 'f':  /* File Frequency */
                            snprintf(buf, LINE_LEN, "%d", id3->frequency);
                            break;

                        case 'p':  /* File Path */
                            strncpy(buf, id3->path, LINE_LEN );
                            break;

                        case 'n':  /* File Name */
                            strncpy(buf, szLast ? szLast : id3->path,
                                    LINE_LEN );
                            break;

                        case 's':  /* File Size (In Kilobytes) */
                            snprintf(buf, LINE_LEN, "%d",
                                     id3->filesize / 1024);
                            break;
                    }
                    break;

                case 'p':  /* Playlist/Song Information */
                    seek++;
                    cchr3 = wps_string[seek];

                    switch(cchr3)
                    {
#ifdef HAVE_LCD_CHARCELLS
                        case 'b':  /* Progress Bar (PLAYER ONLY)*/
                            draw_player_progress(id3, ff_rewind_count);
                            snprintf(buf, LINE_LEN, "\x01");
                            break;
#endif
                        case 'p':  /* Playlist Position */
                            snprintf(buf, LINE_LEN, "%d", id3->index + 1);
                            break;

                        case 'e':  /* Playlist Total Entries */
                            snprintf(buf, LINE_LEN, "%d", playlist.amount);
                            break;

                        case 'c':  /* Current Time in Song */
                            i = id3->elapsed + ff_rewind_count;
                            snprintf(buf, LINE_LEN, "%d:%02d",
                                     i / 60000,
                                     i % 60000 / 1000);
                            wps_time_countup = true;
                            break;

                        case 'r': /* Remaining Time in Song */
                            i = id3->length - id3->elapsed + ff_rewind_count;
                            snprintf(buf, LINE_LEN, "%d:%02d",
                                     i / 60000,
                                     i % 60000 / 1000 );
                            wps_time_countup = false;
                            break;

                        case 't':  /* Total Time */
                            snprintf(buf, LINE_LEN, "%d:%02d",
                                     id3->length / 60000,
                                     id3->length % 60000 / 1000);
                            break;
                    }
                    break;

                case '%':  /* Displays % */
                    buf[0] = '%';
                    buf[1] = 0;
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
                            if (id3->artist && id3->title)
                                strncpy(buf, con_if, LINE_LEN);
                            else
                                strncpy(buf, con_else, LINE_LEN);
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
                    snprintf(bigbuf, sizeof bigbuf, "%s%s", bigbuf, buf);
                    break;

                case 1:
                    snprintf(con_if, sizeof con_if, "%s%s", con_if, buf);
                    break;

                case 2:
                    snprintf(con_else, sizeof con_else, "%s%s", con_else, buf);
                    break;
            }
        }
        else
        {
            switch(con_flag)
            {
                case 0:
                    snprintf(bigbuf, sizeof bigbuf, "%s%c", bigbuf, cchr1);
                    break;

                case 1:
                    snprintf(con_if, sizeof con_if, "%s%c", con_if, cchr1);
                    break;

                case 2:
                    snprintf(con_else, sizeof con_else, "%s%c", 
                             con_else, cchr1);
                    break;
            }
        }

        if (seek >= strlen(wps_string))
        {
            if (do_scroll)
            {
                lcd_stop_scroll();
                lcd_puts_scroll(x_val, y_val, bigbuf);
            }
            else
                lcd_puts(x_val, y_val, bigbuf);

            return true;
        }
    }
    return true;
}

bool wps_refresh(struct mp3entry* id3, int ffwd_offset, bool refresh_scroll)
{
    int l;
#ifdef HAVE_LCD_BITMAP
    int bmp_time_line;
#endif

    if (!id3)
    {
        lcd_stop_scroll();
        lcd_clear_display();
        return false;
    }

    ff_rewind_count = ffwd_offset;

#ifdef HAVE_LCD_CHARCELL
    for (l = 0; l <= 1; l++)
#else
    for (l = 0; l <= 5; l++)
#endif
    {
        if (global_settings.wps_display == PLAY_DISPLAY_CUSTOM_WPS)
        {
            scroll_line = scroll_line_custom;
            if (scroll_line != l)
                    display_custom_wps(id3, 0, l, false, custom_wps[l]);
            if (scroll_line == l && refresh_scroll)
                    display_custom_wps(id3, 0, l, true, custom_wps[l]);
        }
        else
        {
            if (scroll_line != l)
                    display_custom_wps(id3, 0, l, false, display[l]);
            if (scroll_line == l && refresh_scroll)
                    display_custom_wps(id3, 0, l, true, display[l]);                
        }
    }
#ifdef HAVE_LCD_BITMAP
    if (global_settings.statusbar)
        bmp_time_line = 5;
    else
        bmp_time_line = 6;
    snprintf(display[bmp_time_line], sizeof display[bmp_time_line],
             "%s","Time: %pc/%pt");

    slidebar(0, LCD_HEIGHT-6, LCD_WIDTH, 6, id3->elapsed*100/id3->length, Grow_Right);
    lcd_update();
#endif
    return true;
}

void wps_display(struct mp3entry* id3)
{
    int font_height;

#ifdef LOADABLE_FONTS
    unsigned char *font = lcd_getcurrentldfont();
    font_height = ajf_get_fontheight(font);
#else
    font_height = 8;
#endif

    lcd_clear_display();
    if (!id3 && !mpeg_is_playing())
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
        static int last_wps = -1;
        if ((last_wps != global_settings.wps_display
            && global_settings.wps_display == PLAY_DISPLAY_CUSTOM_WPS))
        {
            load_custom_wps();
            last_wps = global_settings.wps_display;
        }

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
                int tmpcnt = 0;

                szBuff[sizeof(szBuff)-1] = 0;
                strncpy(szBuff, id3->path, sizeof szBuff);

                szTok = strtok_r(szBuff, "/", &end);
                szTok = strtok_r(NULL, "/", &end);

                /* Assume path format of: Genre/Artist/Album/Mp3_file */
                strncpy(szArtist, szTok, sizeof szArtist);
                szArtist[sizeof(szArtist)-1] = 0;
                szDelimit = strrchr(id3->path, ch);
                lcd_puts(0, 0, szArtist ? szArtist : "<nothing>");

                /* removes the .mp3 from the end of the display buffer */
                szPeriod = strrchr(szDelimit, '.');
                if (szPeriod != NULL)
                    *szPeriod = 0;

                strncpy(display[0], ++szDelimit, sizeof display[0]);
#ifdef HAVE_LCD_CHARCELLS
                snprintf(display[1], sizeof display[1], "%s", "%pc/%pt");
#endif
                for (tmpcnt=2;tmpcnt<=5;tmpcnt++)
                    display[tmpcnt][0] = 0;
                scroll_line = 0;
                break;
            }
            case PLAY_DISPLAY_FILENAME_SCROLL:
            {
                snprintf(display[0], sizeof display[0], "%s", "%pp/%pe: %fn");
#ifdef HAVE_LCD_CHARCELLS
                snprintf(display[1], sizeof display[1], "%s", "%pc/%pt");
#endif
                scroll_line = 0;
                break;
            }
            case PLAY_DISPLAY_2LINEID3:
            {
#ifdef HAVE_LCD_BITMAP
                int l = 0;

                strncpy( display[l++], "%fn", LINE_LEN );
                strncpy( display[l++], "%it", LINE_LEN );
                strncpy( display[l++], "%id", LINE_LEN );
                strncpy( display[l++], "%ia", LINE_LEN );

                if (!global_settings.statusbar && font_height <= 8)
                {
                    if (id3->vbr)
                        strncpy(display[l++], "%fb kbit (avg)", LINE_LEN);
                    else
                        strncpy(display[l++], "%fb kbit", LINE_LEN);

                    strncpy(display[l++], "%ff Hz", LINE_LEN);
                }
                else
                {
                    if (id3->vbr)
                        strncpy(display[l++], "%fb kbit(a) %ffHz", LINE_LEN);
                    else
                        strncpy(display[l++], "%fb kbit    %ffHz", LINE_LEN);
                }
                scroll_line = 0;
#else
                strncpy(display[0], "%ia", LINE_LEN);
                strncpy(display[1], "%it", LINE_LEN);
                scroll_line = 1;
#endif
                break;
            }
#ifdef HAVE_LCD_CHARCELLS
            case PLAY_DISPLAY_1LINEID3:
            {
                strncpy(display[0], "%pp/%pe: %fc", LINE_LEN);
                strncpy(display[1], "%pc/%pt", LINE_LEN);
                scroll_line = 0;
                break;
            }
            case PLAY_DISPLAY_1LINEID3_PLUS:
            {
                strncpy(display[0], "%pp/%pe: %fc", LINE_LEN);
                strncpy(display[1], "%pr%pb%fbkps", LINE_LEN);
                scroll_line = 0;
                break;
            }
#endif
            case PLAY_DISPLAY_CUSTOM_WPS:
            {
                if (custom_wps[0] == 0)
                {
                    strncpy(display[0], "Couldn't Load Custom WPS", LINE_LEN);
                    strncpy(display[1], "%pc/%pt", LINE_LEN);
                }
                break;
            }
        }
    }
    wps_refresh(id3,0,false);
    status_draw();
    lcd_update();
}

#ifdef HAVE_LCD_CHARCELLS
bool draw_player_progress(struct mp3entry* id3, int ff_rewwind_count)
{
    if(!id3)
        return(false);
    char player_progressbar[7];
    char binline[36];
    int songpos = 0;
    int i,j;

    memset(binline, 1, sizeof binline);
    memset(player_progressbar, 1, sizeof player_progressbar);
    if(wps_time_countup == false)
        songpos = ((id3->elapsed - ff_rewwind_count) * 36) / id3->length;
    else
        songpos = ((id3->elapsed + ff_rewwind_count) * 36) / id3->length;
    for (i=0; i < songpos; i++)
        binline[i] = 0;

    for (i=0; i<=6; i++) {
        for (j=0;j<5;j++) {
            player_progressbar[i] <<= 1;
            player_progressbar[i] += binline[i*5+j];
        }
    }
    lcd_define_pattern(8,player_progressbar,7);
    return(true);
}
#endif
