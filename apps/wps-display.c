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

/* ID3 formatting based on code from the MAD Winamp plugin (in_mad.dll), 
 * Copyright (C) 2000-2001 Robert Leslie. 
 * See http://www.mars.org/home/rob/proj/mpeg/ for more information.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lcd.h"
#include "font.h"
#include "mpeg.h"
#include "id3.h"
#include "settings.h"
#include "playlist.h"
#include "kernel.h"
#include "system.h"
#include "status.h"
#include "wps-display.h"
#include "debug.h"
#include "mas.h"
#include "lang.h"
#include "powermgmt.h"
#include "sprintf.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#include "peakmeter.h"
#endif

#define WPS_CONFIG ROCKBOX_DIR "/default.wps"

#ifdef HAVE_LCD_BITMAP
#define MAX_LINES 10
#else
#define MAX_LINES 2
#endif

#define FORMAT_BUFFER_SIZE 300

#ifdef HAVE_LCD_CHARCELLS
static unsigned char wps_progress_pat[6]={0,0,0,0,0,0};
static bool full_line_progressbar=0;
static bool draw_player_progress(struct mp3entry* id3, int ff_rewwind_count);
static void draw_player_fullbar(char* buf, int buf_size,
                                struct mp3entry* id3, int ff_rewwind_count);
#endif

static char format_buffer[FORMAT_BUFFER_SIZE];
static char* format_lines[MAX_LINES];
static unsigned char line_type[MAX_LINES];
static int ff_rewind_count;
bool wps_time_countup = true;
static bool wps_loaded = false;

static const char* const genres[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
    "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
    "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
    "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
    "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
    "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "AlternRock",
    "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
    "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
    "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
    "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
    "Native American", "Cabaret", "New Wave", "Psychadelic", "Rave",
    "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz",
    "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock",

    /* winamp extensions */
    "Folk", "Folk-Rock", "National Folk", "Swing", "Fast Fusion", "Bebob",
    "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
    "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
    "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
    "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass",
    "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle",
    "Duet", "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall"
};

char* wps_get_genre(unsigned int genre)
{
    if (genre < sizeof(genres)/sizeof(char*))
        return (char*)genres[genre];
    return NULL;
}

/* Set format string to use for WPS, splitting it into lines */
static void wps_format(char* fmt)
{
    char* buf = format_buffer;
    int line = 0;
    
    strncpy(format_buffer, fmt, sizeof(format_buffer));
    format_buffer[sizeof(format_buffer) - 1] = 0;
    format_lines[line] = buf;
    
    while (*buf)
    {
        switch (*buf)
        {
            case '\r':
                *buf = 0;
                break;

            case '\n': /* LF */
                *buf = 0;
                
                if (++line < MAX_LINES)
                {
                    /* the next line starts on the next byte */
                    format_lines[line] = buf+1;
                }
                break;
        }
        buf++;
    }

    if(buf != format_lines[line])
        /* the last line didn't terminate with a newline */
        line++;

    for (; line < MAX_LINES; line++)
    {
        format_lines[line] = NULL;
    }
}

void wps_reset(void)
{
    wps_loaded = false;
    memset(&format_buffer, 0, sizeof format_buffer);
}

bool wps_load(char* file, bool display)
{
    char buffer[FORMAT_BUFFER_SIZE];
    int fd;

    fd = open(file, O_RDONLY);
    
    if (-1 != fd)
    {
        int numread = read(fd, buffer, sizeof(buffer) - 1);
        
        if (numread > 0)
        {
            buffer[numread] = 0;
            wps_format(buffer);
        }
        
        close(fd);

        if ( display ) {
            int i;
            lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
            lcd_setmargins(0,0);
#endif
            for (i=0; i<MAX_LINES && format_lines[i]; i++)
                lcd_puts(0,i,format_lines[i]);
            lcd_update();
            sleep(HZ);
        }

        wps_loaded = true;

        return numread > 0;
    }
    
    return false;
}

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * time     - time to format, in milliseconds.
 */
static void format_time(char* buf, int buf_size, int time)
{
    snprintf(buf, buf_size, "%d:%02d", time / 60000, time % 60000 / 1000);
}

/* Extract a part from a path.
 *
 * buf      - buffer extract part to.
 * buf_size - size of buffer.
 * path     - path to extract from.
 * level    - what to extract. 0 is file name, 1 is parent of file, 2 is 
 *            parent of parent, etc.
 *
 * Returns buf if the desired level was found, NULL otherwise.
 */
static char* get_dir(char* buf, int buf_size, char* path, int level)
{
    char* sep;
    char* last_sep;
    int len;

    sep = path + strlen(path);
    last_sep = sep;

    while (sep > path)
    {
        if ('/' == *(--sep))
        {
            if (!level)
            {
                break;
            }
            
            level--;
            last_sep = sep - 1;
        }
    }

    if (level || (last_sep <= sep))
    {
        return NULL;
    }

    len = MIN(last_sep - sep, buf_size - 1);
    strncpy(buf, sep + 1, len);
    buf[len] = 0;
    return buf;
}

/* Get the tag specified by the two characters at fmt.
 *
 * id3      - ID3 data to get tag values from.
 * tag      - string (of two characters) specifying the tag to get.
 * buf      - buffer to certain tags, such as track number, play time or 
 *           directory name.
 * buf_size - size of buffer.
 * flags    - returns the type of the line. See constants i wps-display.h
 *
 * Returns the tag. NULL indicates the tag wasn't available.
 */
static char* get_tag(struct mp3entry* id3, 
                     char* tag, 
                     char* buf, 
                     int buf_size,
                     unsigned char* flags)
{
    if ((0 == tag[0]) || (0 == tag[1]))
    {
        return NULL;
    }
    
    switch (tag[0])
    {
        case 'i':  /* ID3 Information */
            *flags |= WPS_REFRESH_STATIC;
            switch (tag[1])
            {
                case 't':  /* ID3 Title */
                    return id3->title;

                case 'a':  /* ID3 Artist */
                    return id3->artist;
            
                case 'n':  /* ID3 Track Number */
                    if (id3->tracknum)
                    {
                        snprintf(buf, buf_size, "%d", id3->tracknum);
                        return buf;
                    }
                    else
                    {
                        return NULL;
                    }

                case 'd':  /* ID3 Album/Disc */
                    return id3->album;

                case 'y':  /* year */
                    if (id3->year) {
                        snprintf(buf, buf_size, "%d", id3->year);
                        return buf;
                    }
                    else
                        return NULL;
                    break;

                case 'g':  /* genre */
                    if (id3->genre < sizeof(genres)/sizeof(char*))
                        return (char*)genres[id3->genre];
                    else
                        return NULL;
                    break;

                case 'v': /* id3 version */
                    switch (id3->id3version) {
                        case ID3_VER_1_0:
                            return "1";

                        case ID3_VER_1_1:
                            return "1.1";

                        case ID3_VER_2_2:
                            return "2.2";

                        case ID3_VER_2_3:
                            return "2.3";

                        case ID3_VER_2_4:
                            return "2.4";

                        default:
                            return NULL;
                    }
            }
            break;

        case 'f':  /* File Information */
            *flags |= WPS_REFRESH_STATIC;
            switch(tag[1])
            {
                case 'v':  /* VBR file? */
                    return id3->vbr ? "(avg)" : NULL;

                case 'b':  /* File Bitrate */
                    snprintf(buf, buf_size, "%d", id3->bitrate);
                    return buf;

                case 'f':  /* File Frequency */
                    snprintf(buf, buf_size, "%d", id3->frequency);
                    return buf;

                case 'p':  /* File Path */
                    return id3->path;

                case 'm':  /* File Name - With Extension */
                    return get_dir(buf, buf_size, id3->path, 0);

                case 'n':  /* File Name */
                    if (get_dir(buf, buf_size, id3->path, 0))
                    {
                        /* Remove extension */
                        char* sep = strrchr(buf, '.');

                        if (NULL != sep)
                        {
                            *sep = 0;
                        }

                        return buf;
                    }
                    else
                    {
                        return NULL;
                    }

                case 's':  /* File Size (in kilobytes) */
                    snprintf(buf, buf_size, "%d", id3->filesize / 1024);
                    return buf;
            }
            break;

        case 'p':  /* Playlist/Song Information */
            switch(tag[1])
            {
                case 'b':  /* progress bar */
                    *flags |= WPS_REFRESH_PLAYER_PROGRESS;
#ifdef HAVE_LCD_CHARCELLS
                    snprintf(buf, buf_size, "%c", wps_progress_pat[0]);
                    full_line_progressbar=0;
                    return buf;
#else
                    return "\x01";
#endif
                case 'f':  /* full-line progress bar */
#ifdef HAVE_LCD_CHARCELLS
                    *flags |= WPS_REFRESH_PLAYER_PROGRESS;
                    *flags |= WPS_REFRESH_DYNAMIC;
                    full_line_progressbar=1;
                    /* we need 11 characters (full line) for progress-bar */
                    snprintf(buf, buf_size, "           ");
                    return buf;
#endif
                case 'p':  /* Playlist Position */
                    *flags |= WPS_REFRESH_STATIC;
                    snprintf(buf, buf_size, "%d", id3->index + 1);
                    return buf;

                case 'n':  /* Playlist Name (without path) */
                    *flags |= WPS_REFRESH_STATIC;
                    return playlist_name(buf, buf_size);

                case 'e':  /* Playlist Total Entries */
                    *flags |= WPS_REFRESH_STATIC;
                    snprintf(buf, buf_size, "%d", playlist_amount());
                    return buf;

                case 'c':  /* Current Time in Song */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    format_time(buf, buf_size, id3->elapsed + ff_rewind_count);
                    return buf;

                case 'r': /* Remaining Time in Song */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    format_time(buf, buf_size, 
                                id3->length - id3->elapsed - ff_rewind_count);
                    return buf;

                case 't':  /* Total Time */
                    *flags |= WPS_REFRESH_STATIC;
                    format_time(buf, buf_size, id3->length);
                    return buf;

#ifdef HAVE_LCD_BITMAP
                case 'm': /* Peak Meter */
                    *flags |= WPS_REFRESH_PEAK_METER;
                    return "\x01";
#endif
                case 's': /* shuffle */
                    if ( global_settings.playlist_shuffle )
                        return "s";
                    else
                        return NULL;
                    break;
            }
            break;

        case 'b': /* battery info */
            switch (tag[1]) {
                case 'l': /* battery level */
                    snprintf(buf, buf_size, "%d%%", battery_level());
                    return buf;

                case 't': /* estimated battery time */
                    snprintf(buf, buf_size, "%dh %dm",
                             battery_time() / 60,
                             battery_time() % 60);
                    return buf;
            }
            break;

        case 'd': /* Directory path information */
            {
                int level = tag[1] - '0';
                *flags |= WPS_REFRESH_STATIC;
                /* d1 through d9 */
                if ((0 < level) && (9 > level))
                {
                    return get_dir(buf, buf_size, id3->path, level);
                }
            }
            break;
    }
    
    return NULL;
}

/* Skip to the end of the current %? conditional.
 *
 * fmt     - string to skip it. Should point to somewhere after the leading 
 *           "<" char (and before or at the last ">").
 * to_else - if true, skip to the else part (after the "|", if any), else skip
 *           to the end (the ">").
 *
 * Returns the new position in fmt.
 */
static char* skip_conditional(char* fmt, bool to_else)
{
    int level = 1;

    while (*fmt)
    {
        switch (*fmt++)
        {
            case '%':
                break;
        
            case '|':
                if (to_else && (1 == level))
                    return fmt;
            
                continue;
            
            case '>':
                if (0 == --level) 
                {
                    if (to_else)
                        fmt--;
                
                    return fmt;
                }
                continue;

            default:
                continue;
        }
        
        switch (*fmt++)
        {
            case 0:
            case '%':
            case '|':
            case '<':
            case '>':
                break;
        
            case '?':
                while (*fmt && ('<' != *fmt))
                    fmt++;
            
                if ('<' == *fmt)
                    fmt++;
            
                level++;
                break;
        
            default:
                break;
        }
    }
    
    return fmt;
}

/* Generate the display based on id3 information and format string.
 *
 * buf      - char buffer to write the display to.
 * buf_size - the size of buffer.
 * id3      - the ID3 data to format with.
 * fmt      - format description.
 * flags    - returns the type of the line. See constants i wps-display.h
 */
static void format_display(char* buf, 
                           int buf_size, 
                           struct mp3entry* id3, 
                           char* fmt, 
                           unsigned char* flags)
{
    char temp_buf[128];
    char* buf_end = buf + buf_size - 1;   /* Leave room for end null */
    char* value = NULL;
    int level = 0;
    
    while (fmt && *fmt && buf < buf_end)
    {
        switch (*fmt)
        {
            case '%':
                ++fmt;
                break;
        
            case '|':
            case '>':
                if (level > 0) 
                {
                    fmt = skip_conditional(fmt, false);
                    level--;
                    continue;
                }
                /* Else fall through */

            default:
                *buf++ = *fmt++;
                continue;
        }
        
        switch (*fmt)
        {
            case 0:
                *buf++ = '%';
                break;
        
            case 's':
                *flags |= WPS_REFRESH_SCROLL;
                ++fmt;
                break;
        
            case '%':
            case '|':
            case '<':
            case '>':
                *buf++ = *fmt++;
                break;
        
            case '?':
                fmt++;
                value = get_tag(id3, fmt, temp_buf, sizeof(temp_buf), flags);
            
                while (*fmt && ('<' != *fmt))
                    fmt++;
            
                if ('<' == *fmt)
                    fmt++;
            
                /* No value, so skip to else part */
                if (NULL == value)
                    fmt = skip_conditional(fmt, true);

                level++;
                break;
        
            default:
                value = get_tag(id3, fmt, temp_buf, sizeof(temp_buf), flags);
                fmt += 2;
            
                if (value)
                {
                    while (*value && (buf < buf_end))
                        *buf++ = *value++;
                }
        }
    }
    
    *buf = 0;

    /* If no flags have been set, the line didn't contain any format codes.
       We still want to refresh it. */
    if(*flags == 0)
        *flags = WPS_REFRESH_STATIC;
}

bool wps_refresh(struct mp3entry* id3, int ffwd_offset, unsigned char refresh_mode)
{
    char buf[MAX_PATH];
    unsigned char flags;
    int i;
    bool update_line;
#ifdef HAVE_LCD_BITMAP
    int h = font_get(FONT_UI)->height;
    /* to find out wether the peak meter is enabled we
       assume it wasn't until we find a line that contains
       the peak meter. We can't use peak_meter_enabled itself
       because that would mean to turn off the meter thread 
       temporarily. (That shouldn't matter unless yield 
       or sleep is called but who knows...)
    */
    bool enable_pm = false;
#endif
#ifdef HAVE_LCD_CHARCELLS
    for (i=0; i<6; i++) {
       if (wps_progress_pat[i]==0)
           wps_progress_pat[i]=lcd_get_locked_pattern();
    }
#endif

    if (!id3)
    {
        lcd_stop_scroll();
        return false;
    }

    ff_rewind_count = ffwd_offset;

    for (i = 0; i < MAX_LINES; i++)
    {
        update_line = false;
        if ( !format_lines[i] )
            break;

        if ((line_type[i] & refresh_mode) || 
            (refresh_mode == WPS_REFRESH_ALL))
        {
            flags = 0;
            format_display(buf, sizeof(buf), id3, format_lines[i], &flags);
            line_type[i] = flags;
            
#ifdef HAVE_LCD_BITMAP
            /* progress */
            if (flags & refresh_mode & WPS_REFRESH_PLAYER_PROGRESS) {
                int offset = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;
                int percent=
                    id3->length?
                    (id3->elapsed + ff_rewind_count) * 100 / id3->length:0;
                slidebar(0, i*h + offset + 1, LCD_WIDTH, 6, 
                         percent, Grow_Right);
                update_line = true;
            }
            if (flags & refresh_mode & WPS_REFRESH_PEAK_METER) {
                /* peak meter */
                int peak_meter_y;
                int offset = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;

                update_line = true;
                peak_meter_y = i * h + offset;

                /* The user might decide to have the peak meter in the last 
                   line so that it is only displayed if no status bar is 
                   visible. If so we neither want do draw nor enable the
                   peak meter. */
                if (peak_meter_y + h <= LCD_HEIGHT) {
                    /* found a line with a peak meter -> remember that we must
                       enable it later */
                    enable_pm = true;
                    peak_meter_draw(0, peak_meter_y, LCD_WIDTH,
                                    MIN(h, LCD_HEIGHT - peak_meter_y));
                }
            }
#else
            /* progress */
            if (flags & refresh_mode & WPS_REFRESH_PLAYER_PROGRESS) {
                if (full_line_progressbar)
                    draw_player_fullbar(buf, sizeof(buf),
                                        id3, ff_rewind_count);
                else
                    draw_player_progress(id3, ff_rewind_count);
            }
#endif
            if (flags & WPS_REFRESH_SCROLL) {
                /* scroll line */
                if (refresh_mode & WPS_REFRESH_SCROLL)  {
                    lcd_puts_scroll(0, i, buf);
                    update_line = true;
                }
            }
            /* dynamic / static line */
            else if ((flags & refresh_mode & WPS_REFRESH_DYNAMIC) ||
                     (flags & refresh_mode & WPS_REFRESH_STATIC))
            {
                update_line = true;
                lcd_puts(0, i, buf);
            }
        }
#ifdef HAVE_LCD_BITMAP
        if (update_line) {
            lcd_update_rect(0, i * h, LCD_WIDTH, h);
        }
#endif
    }
#ifdef HAVE_LCD_BITMAP
    /* Now we know wether the peak meter is used. 
       So we can enable / disable the peak meter thread */
    peak_meter_enabled = enable_pm;
#endif

    return true;
}

bool wps_display(struct mp3entry* id3)
{
    lcd_clear_display();

    if (!id3 && !(mpeg_status() & MPEG_STATUS_PLAY))
    {
#ifdef HAVE_LCD_CHARCELLS
        lcd_puts(0, 0, str(LANG_END_PLAYLIST_PLAYER));
#else
        lcd_puts(0, 2, str(LANG_END_PLAYLIST_RECORDER));
        lcd_update();
#endif
        status_set_playmode(STATUS_STOP);
        status_draw();
        sleep(HZ);
        return true;
    }
    else
    {
        if (!wps_loaded) {
            if ( !format_buffer[0] ) {
#ifdef HAVE_LCD_BITMAP
                wps_format("%s%?it<%?in<%in. |>%it|%fn>\n"
                           "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
                           "%s%?id<%id|%?d1<%d1|(root)>> %?iy<(%iy)|>\n"
                           "\n"
                           "%pc/%pt [%pp:%pe]\n"
                           "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
                           "%pb\n"
                           "%pm\n");
#else
                wps_format("%s%pp/%pe: %?it<%it|%fn> - %?ia<%ia|%d2> - %?id<%id|%d1>\n"
                           "%pc%?ps<*|/>%pt\n");
#endif
            }
        }
    }
    yield();
    wps_refresh(id3, 0, WPS_REFRESH_ALL);
    status_draw();
    lcd_update();
    return false;
}

#ifdef HAVE_LCD_CHARCELLS
static bool draw_player_progress(struct mp3entry* id3, int ff_rewwind_count)
{
    char player_progressbar[7];
    char binline[36];
    int songpos = 0;
    int i,j;

    if (!id3)
        return false;

    memset(binline, 1, sizeof binline);
    memset(player_progressbar, 1, sizeof player_progressbar);

    if(id3->elapsed >= id3->length)
        songpos = 0;
    else
    {
        if(wps_time_countup == false)
            songpos = ((id3->elapsed - ff_rewwind_count) * 36) / id3->length;
        else
            songpos = ((id3->elapsed + ff_rewwind_count) * 36) / id3->length;
    }
    for (i=0; i < songpos; i++)
        binline[i] = 0;

    for (i=0; i<=6; i++) {
        for (j=0;j<5;j++) {
            player_progressbar[i] <<= 1;
            player_progressbar[i] += binline[i*5+j];
        }
    }
    lcd_define_pattern(wps_progress_pat[0], player_progressbar);
    return true;
}

static void draw_player_fullbar(char* buf, int buf_size,
                                struct mp3entry* id3, int ff_rewwind_count)
{
    int i,j,digit;

    char player_progressbar[7];
    char binline[36];
    char numbers[11][4][3]={{{1,1,1},{1,0,1},{1,0,1},{1,1,1}},/*0*/
                            {{0,1,0},{1,1,0},{0,1,0},{0,1,0}},/*1*/
                            {{1,1,1},{0,0,1},{0,1,0},{1,1,1}},/*2*/
                            {{1,1,1},{0,0,1},{0,1,1},{1,1,1}},/*3*/
                            {{1,0,0},{1,1,0},{1,1,1},{0,1,0}},/*4*/
                            {{1,1,1},{1,1,0},{0,0,1},{1,1,0}},/*5*/
                            {{1,1,1},{1,0,0},{1,1,1},{1,1,1}},/*6*/
                            {{1,1,1},{0,0,1},{0,1,0},{1,0,0}},/*7*/
                            {{1,1,1},{1,1,1},{1,0,1},{1,1,1}},/*8*/
                            {{1,1,1},{1,1,1},{0,0,1},{1,1,1}},/*9*/
                            {{0,0,0},{0,1,0},{0,0,0},{0,1,0}}};/*:*/

    int songpos = 0;
    int digits[5];
    int time;
    
    for (i=0; i < buf_size; i++)
        buf[i] = ' ';
           
    if(id3->elapsed >= id3->length)
        songpos = 11;
    else {
        if(wps_time_countup == false)
            songpos = ((id3->elapsed - ff_rewwind_count) * 55) / id3->length;
        else
            songpos = ((id3->elapsed + ff_rewwind_count) * 55) / id3->length;
    }

    time=(id3->elapsed + ff_rewind_count);
   
    digits[4]=(time % 60000 / 1000 ) % 10;
    digits[3]=(time % 60000 / 10000) % 10;
    digits[2]=10; /*:*/
    digits[1]=(time / 60000 ) % 10;
    digits[0]=(time / 600000) % 10;
   
    /* build the progressbar-icon */
    for (digit=0; digit <=4; digit++) {
        memset(binline, 0, sizeof binline);
        memset(player_progressbar, 0, sizeof player_progressbar);

        /* make the character (progressbar & digit)*/
        for (i=0; i<=6; i++) {
            for (j=0;j<5;j++) {
                /* make the progressbar */
                if (digit==(songpos/5)) {
                    /* partial */
                    if ((j<(songpos%5))&&(i>4))
                        binline[i*5+j] = 1;
                    else
                        binline[i*5+j] = 0;
                }
                else {
                    if (digit<(songpos/5)) {
                        /* full character */
                        if (i>4)
                            binline[i*5+j] = 1;
                    }
                }
                /* insert the digit */
                if ((j<3)&&(i<4)) {
                    if (numbers[digits[digit]][i][j]==1)
                        binline[i*5+j] = 1;
                }
            }
        }
         
        for (i=0; i<=6; i++) {
            for (j=0;j<5;j++) {
                player_progressbar[i] <<= 1;
                player_progressbar[i] += binline[i*5+j];
            }
        }
       
        lcd_define_pattern(wps_progress_pat[digit],player_progressbar);
        buf[digit]=wps_progress_pat[digit];
       
    }
   
    /* make rest of the progressbar if necessary */
    if (songpos/5>4) {
   
        memset(binline, 0, sizeof binline);
        memset(player_progressbar, 0, sizeof player_progressbar);

        for (i=5; i<=6; i++) {
            for (j=0;j<5;j++) {
                if (j<(songpos%5)) {
                    binline[i*5+j] = 1;
                }
            }
        }
       
        for (i=0; i<=6; i++) {
            for (j=0;j<5;j++) {
                player_progressbar[i] <<= 1;
                player_progressbar[i] += binline[i*5+j];
            }
        }
       
        lcd_define_pattern(wps_progress_pat[5],player_progressbar);
           
        for (i=5; i < (songpos/5); i++)
            buf[i] = 0x86; /* '_' */
             
        buf[songpos/5]=wps_progress_pat[5];
    }
       
}

#endif

/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
