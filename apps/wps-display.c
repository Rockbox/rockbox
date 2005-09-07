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
#include "hwcompat.h"
#include "font.h"
#include "audio.h"
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
#include "backlight.h"
#include "button.h"
#include "abrepeat.h"

#ifdef HAVE_LCD_BITMAP
#include <ctype.h>
#include "icons.h"
#include "widgets.h"
#include "peakmeter.h"
#include "screens.h"

/* Image stuff */
#include "bmp.h"
#include "atoi.h"
#define MAX_IMAGES 26 /* a-z */
#define IMG_BUFSIZE (LCD_HEIGHT * LCD_WIDTH * MAX_IMAGES/10) / 8
static unsigned char img_buf[IMG_BUFSIZE]; /* image buffer */
static unsigned char* img_buf_ptr = img_buf; /* where are in image buffer? */

static int img_buf_free = IMG_BUFSIZE; /* free space in image buffer */

struct {
    unsigned char* ptr;     /* pointer */
    int x;                  /* x-pos */
    int y;                  /* y-pos */
    int w;                  /* width */
    int h;                  /* height */
    bool loaded;            /* load state */
    bool display;           /* is to be displayed */
    bool always_display;    /* not using the preload/display mechanism */
} img[MAX_IMAGES] ;


#endif

#define WPS_CONFIG ROCKBOX_DIR "/default.wps"

#ifdef HAVE_LCD_BITMAP
#define MAX_LINES (LCD_HEIGHT/5+1)
#define FORMAT_BUFFER_SIZE 800
#else
#define MAX_LINES 2
#define FORMAT_BUFFER_SIZE 400
#endif
#define MAX_SUBLINES 12
#define DEFAULT_SUBLINE_TIME_MULTIPLIER 20 /* (10ths of sec) */
#define BASE_SUBLINE_TIME 10 /* base time that multiplier is applied to
                                (1/HZ sec, or 100ths of sec) */
#define SUBLINE_RESET -1

#ifdef HAVE_LCD_CHARCELLS
static unsigned char wps_progress_pat[8]={0,0,0,0,0,0,0,0};
static bool full_line_progressbar=0;
static bool draw_player_progress(const struct mp3entry* id3,
                                 int ff_rewwind_count);
static void draw_player_fullbar(char* buf, int buf_size,
                                const struct mp3entry* id3,
                                int ff_rewwind_count);
static char map_fullbar_char(char ascii_val);
#endif

struct align_pos {
    char* left;
    char* center;
    char* right;
};
static char format_buffer[FORMAT_BUFFER_SIZE];
static char* format_lines[MAX_LINES][MAX_SUBLINES];
static struct align_pos format_align[MAX_LINES][MAX_SUBLINES];
static unsigned char line_type[MAX_LINES][MAX_SUBLINES];
static unsigned short time_mult[MAX_LINES][MAX_SUBLINES];
static long subline_expire_time[MAX_LINES];
static int curr_subline[MAX_LINES];

static int ff_rewind_count;
bool wps_time_countup = true;
static bool wps_loaded = false;

#ifdef HAVE_LCD_BITMAP
/* Display images */
static void wps_display_images(void) {
    int n;
    lcd_set_drawmode(DRMODE_FG);
    for (n = 0; n < MAX_IMAGES; n++) {
        if (img[n].loaded && img[n].display) {
            lcd_mono_bitmap(img[n].ptr, img[n].x, img[n].y, img[n].w, img[n].h);
            lcd_update_rect(img[n].x, img[n].y, img[n].w, img[n].h);
        }
    }
    lcd_set_drawmode(DRMODE_SOLID);
}
#endif

/* Set format string to use for WPS, splitting it into lines */
static void wps_format(const char* fmt)
{
    char* buf = format_buffer;
    char* start_of_line = format_buffer;
    int line = 0;
    int subline;

    strncpy(format_buffer, fmt, sizeof(format_buffer));
    format_buffer[sizeof(format_buffer) - 1] = 0;

    for (line=0; line<MAX_LINES; line++)
    {
        for (subline=0; subline<MAX_SUBLINES; subline++)
        {
            format_lines[line][subline] = 0;
            time_mult[line][subline] = 0;
        }
        subline_expire_time[line] = 0;
        curr_subline[line] = SUBLINE_RESET;
    }

    line = 0;
    subline = 0;
    format_lines[line][subline] = buf;

    while ((*buf) && (line < MAX_LINES))
    {
        switch (*buf)
        {
            /*
             * skip % sequences so "%;" doesn't start a new subline
             * don't skip %x lines (pre-load bitmaps)
             */
            case '%':
                if (*(buf+1) != 'x')
                    buf++;
                break;

            case '\r': /* CR */
                *buf = 0;
                break;

            case '\n': /* LF */
                *buf = 0;

                if (*start_of_line != '#') /* A comment? */
                    line++;

                if (line < MAX_LINES)
                {
                    /* the next line starts on the next byte */
                    subline = 0;
                    format_lines[line][subline] = buf+1;
                    start_of_line = format_lines[line][subline];
                }
                break;

            case ';': /* start a new subline */
                *buf = 0;
                subline++;
                if (subline < MAX_SUBLINES)
                {
                    format_lines[line][subline] = buf+1;
                }
                else /* exceeded max sublines, skip rest of line */
                {
                    while (*(++buf))
                    {
                        if  ((*buf == '\r') || (*buf == '\n'))
                        {
                            break;
                        }
                    }
                    buf--;
                    subline = 0;
                }
                break;

            case 'x':
#ifdef HAVE_LCD_BITMAP
                /* Preload images so the %xd# tag can display it */
            {
                int ret = 0;
                int n;
                char *ptr = buf+1;
                char *pos = NULL;
                char imgname[MAX_PATH];
                char qual = *ptr;
                if (qual == 'l' || qual == '|')  /* format:
                                                    %x|n|filename.bmp|x|y|
                                                    or
                                                    %xl|n|filename.bmp|x|y|
                                                 */
                {
                    ptr = strchr(ptr, '|') + 1;
                    pos = strchr(ptr, '|');
                    if (pos)
                    {
                        /* get the image number */
                        n = tolower(*ptr) - 'a';
                        if(n < 0 || n >= MAX_IMAGES)
                        {
                            buf++;
                            break;
                        }
                        ptr = pos+1;

                        /* check the image number and load state */
                        if ((n < MAX_IMAGES) && (!img[n].loaded))
                        {
                            /* get filename */
                            pos = strchr(ptr, '|');
                            if ((pos - ptr) <
                                (int)sizeof(imgname)-ROCKBOX_DIR_LEN-2)
                            {
                                memcpy(imgname, ROCKBOX_DIR, ROCKBOX_DIR_LEN);
                                imgname[ROCKBOX_DIR_LEN] = '/';
                                memcpy(&imgname[ROCKBOX_DIR_LEN+1],
                                       ptr, pos - ptr);
                                imgname[ROCKBOX_DIR_LEN+1+pos-ptr] = 0;
                            }
                            else
                                /* filename too long */
                                imgname[0] = 0;

                            ptr = pos+1;

                            /* get x-position */
                            pos = strchr(ptr, '|');
                            if (pos)
                                img[n].x = atoi(ptr);
                            else
                            {
                                /* weird syntax, bail out */
                                buf++;
                                break;
                            }

                            /* get y-position */
                            ptr = pos+1;
                            pos = strchr(ptr, '|');
                            if (pos)
                                img[n].y = atoi(ptr);
                            else
                            {
                                /* weird syntax, bail out */
                                buf++;
                                break;
                            }

                            pos++;
                            
                            /* reposition buf pointer to next WPS element */
                            while (*pos && *pos != ';' &&
                                   *pos != '\r' && *pos != '\n')
                                pos++;

                            buf = pos;

                            /* load the image */
                            ret = read_bmp_file(imgname, &img[n].w, &img[n].h,
                                                img_buf_ptr, img_buf_free);
                            if (ret > 0)
                            {
                                img[n].ptr = img_buf_ptr;
                                img_buf_ptr += ret;
                                img_buf_free -= ret;
                                img[n].loaded = true;
                                if(qual == '|')
                                    img[n].always_display = true;
                            }
                        }
                        buf++;
                    }
                }
              }
#endif
              break;
        }
        buf++;
    }
}

void wps_reset(void)
{
    wps_loaded = false;
    memset(&format_buffer, 0, sizeof format_buffer);
}

bool wps_load(const char* file, bool display)
{
    int i, s;
    char buffer[FORMAT_BUFFER_SIZE];
    int fd;

    fd = open(file, O_RDONLY);

    if (fd >= 0)
    {
        int numread = read(fd, buffer, sizeof(buffer) - 1);

        if (numread > 0)
        {
#ifdef HAVE_LCD_BITMAP
            /* reset image buffer */
            img_buf_ptr = img_buf;
            img_buf_free = IMG_BUFSIZE;

            /* set images to unloaded and not displayed */
            for (i = 0; i < MAX_IMAGES; i++) {
                img[i].loaded = false;
                img[i].display = false;
                img[i].always_display = false;
            }
#endif
            buffer[numread] = 0;
            wps_format(buffer);
        }

        close(fd);

        if ( display ) {
            bool any_defined_line;
            lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
            lcd_setmargins(0,0);
#endif
            for (s=0; s<MAX_SUBLINES; s++)
            {
                any_defined_line = false;
                for (i=0; i<MAX_LINES; i++)
                {
                    if (format_lines[i][s])
                    {
                        if (*format_lines[i][s] == 0)
                            lcd_puts(0,i," ");
                        else
                            lcd_puts(0,i,format_lines[i][s]);
                        any_defined_line = true;
                    }
                    else
                    {
                        lcd_puts(0,i," ");
                    }
                }
                if (any_defined_line)
                {
#ifdef HAVE_LCD_BITMAP
                    wps_display_images();
#endif
                    lcd_update();
                    sleep(HZ/2);
                }
            }
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
void wps_format_time(char* buf, int buf_size, long time)
{
    if ( time < 3600000 ) {
      snprintf(buf, buf_size, "%d:%02d",
               (int) (time % 3600000 / 60000), (int) (time % 60000 / 1000));
    } else {
      snprintf(buf, buf_size, "%d:%02d:%02d",
               (int) (time / 3600000), (int) (time % 3600000 / 60000),
               (int) (time % 60000 / 1000));
    }
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
static char* get_dir(char* buf, int buf_size, const char* path, int level)
{
    const char* sep;
    const char* last_sep;
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
 * cid3      - ID3 data to get tag values from.
 * nid3     - next-song ID3 data to get tag values from.
 * tag      - string (of two characters) specifying the tag to get.
 * buf      - buffer to certain tags, such as track number, play time or
 *           directory name.
 * buf_size - size of buffer.
 * flags    - returns the type of the line. See constants i wps-display.h
 *
 * Returns the tag. NULL indicates the tag wasn't available.
 */
static char* get_tag(struct mp3entry* cid3,
                     struct mp3entry* nid3,
                     const char* tag,
                     char* buf,
                     int buf_size,
                     unsigned char* tag_len,
                     unsigned short* subline_time_mult,
                     unsigned char* flags,
                     int *intval)
{
    struct mp3entry *id3 = cid3; /* default to current song */

    if ((0 == tag[0]) || (0 == tag[1]))
    {
        *tag_len = 0;
        return NULL;
    }

    *tag_len = 2;

    *intval = 0;
    
    switch (tag[0])
    {
        case 'I':  /* ID3 Information */
            id3 = nid3; /* display next-song data */
            *flags |= WPS_REFRESH_DYNAMIC;
            if(!id3)
                return NULL; /* no such info (yet) */
            /* fall-through */
        case 'i':  /* ID3 Information */
            *flags |= WPS_REFRESH_STATIC;
            switch (tag[1])
            {
                case 't':  /* ID3 Title */
                    return id3->title;

                case 'a':  /* ID3 Artist */
                    return id3->artist;

                case 'n':  /* ID3 Track Number */
                    if (id3->track_string)
                        return id3->track_string;

                    if (id3->tracknum) {
                        snprintf(buf, buf_size, "%d", id3->tracknum);
                        return buf;
                    }
                    return NULL;

                case 'd':  /* ID3 Album/Disc */
                    return id3->album;

                case 'c':  /* ID3 Composer */
                    return id3->composer;

                case 'y':  /* year */
                    if( id3->year_string )
                        return id3->year_string;

                    if (id3->year) {
                        snprintf(buf, buf_size, "%d", id3->year);
                        return buf;
                    }
                    return NULL;

                case 'g':  /* genre */
                    return id3_get_genre(id3);

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

        case 'F':  /* File Information */
            id3 = nid3;
            *flags |= WPS_REFRESH_DYNAMIC;
            if(!id3)
                return NULL; /* no such info (yet) */
            /* fall-through */
        case 'f':  /* File Information */
            *flags |= WPS_REFRESH_STATIC;
            switch(tag[1])
            {
                case 'v':  /* VBR file? */
                    return id3->vbr ? "(avg)" : NULL;

                case 'b':  /* File Bitrate */
                    if(id3->bitrate)
                        snprintf(buf, buf_size, "%d", id3->bitrate);
                    else
                        snprintf(buf, buf_size, "?");
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

                case 'c':  /* File Codec */
                    if(id3->codectype == AFMT_UNKNOWN)
                        *intval = AFMT_NUM_CODECS;
                    else
                        *intval = id3->codectype;
                    return id3_get_codec(id3);
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
                    if(is_new_player()) {
                        *flags |= WPS_REFRESH_PLAYER_PROGRESS;
                        *flags |= WPS_REFRESH_DYNAMIC;
                        full_line_progressbar=1;
                        /* we need 11 characters (full line) for
                           progress-bar */
                        snprintf(buf, buf_size, "           ");
                    }
                    else
                    {
                        /* Tell the user if we have an OldPlayer */
                        snprintf(buf, buf_size, " <Old LCD> ");
                    }
                    return buf;
#endif
                case 'p':  /* Playlist Position */
                    *flags |= WPS_REFRESH_STATIC;
                    snprintf(buf, buf_size, "%d", playlist_get_display_index());
                    return buf;

                case 'n':  /* Playlist Name (without path) */
                    *flags |= WPS_REFRESH_STATIC;
                    return playlist_name(NULL, buf, buf_size);

                case 'e':  /* Playlist Total Entries */
                    *flags |= WPS_REFRESH_STATIC;
                    snprintf(buf, buf_size, "%d", playlist_amount());
                    return buf;

                case 'c':  /* Current Time in Song */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    wps_format_time(buf, buf_size,
                                    id3->elapsed + ff_rewind_count);
                    return buf;

                case 'r': /* Remaining Time in Song */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    wps_format_time(buf, buf_size,
                                    id3->length - id3->elapsed - ff_rewind_count);
                    return buf;

                case 't':  /* Total Time */
                    *flags |= WPS_REFRESH_STATIC;
                    wps_format_time(buf, buf_size, id3->length);
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

                case 'v': /* volume */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    snprintf(buf, buf_size, "%d%%", global_settings.volume);
                    *intval = global_settings.volume / 10 + 1;
                    return buf;

            }
            break;
            
        case 'm':
            switch (tag[1])
            {
                case 'm': /* playback repeat mode */
                    *intval = global_settings.repeat_mode + 1;
                    snprintf(buf, buf_size, "%d", *intval);
                    return buf;
                    
                /* playback status */
                case 'p': /* play */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    int status = audio_status();
                    *intval = 1;
                    if (status == AUDIO_STATUS_PLAY && \
                        !(status & AUDIO_STATUS_PAUSE))
                        *intval = 2;
                    if (audio_status() & AUDIO_STATUS_PAUSE && \
                        (! status_get_ffmode()))
                        *intval = 3;
                    if (status_get_ffmode() == STATUS_FASTFORWARD)
                        *intval = 4;
                    if (status_get_ffmode() == STATUS_FASTBACKWARD)
                        *intval = 5;
                    snprintf(buf, buf_size, "%d", *intval);
                    return buf;

#if CONFIG_KEYPAD == IRIVER_H100_PAD
                case 'h': /* hold */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    if (button_hold())
                        return "h";
                    else
                        return NULL;
                case 'r': /* remote hold */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    if (remote_button_hold())
                        return "r";
                    else
                        return NULL;
#endif
            }
            break;

        case 'b': /* battery info */
            *flags |= WPS_REFRESH_DYNAMIC;
            switch (tag[1]) {
                case 'l': /* battery level */
                {
                    int l = battery_level();
                    if (l > -1)
                    {
                        snprintf(buf, buf_size, "%d%%", l);
                        *intval = l / 20 + 1;
                    }
                    else
                    {
                        *intval = 6;
                        return "?%";
                    }
                    return buf;
                }

                case 't': /* estimated battery time */
                {
                    int t = battery_time();
                    if (t >= 0)
                        snprintf(buf, buf_size, "%dh %dm", t / 60, t % 60);
                    else
                        strncpy(buf, "?h ?m", buf_size);
                    return buf;
                }
            }
            break;

        case 'D': /* Directory path information */
            id3 = nid3; /* next song please! */
            *flags |= WPS_REFRESH_DYNAMIC;
            if(!id3)
                return NULL; /* no such info (yet) */
            /* fall-through */
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

        case 't': /* set sub line time multiplier */
            {
                int d = 1;
                int time_mult = 0;
                bool have_point = false;
                bool have_tenth = false;

                while (((tag[d] >= '0') &&
                        (tag[d] <= '9')) ||
                       (tag[d] == '.'))
                {
                    if (tag[d] != '.')
                    {
                        time_mult = time_mult * 10;
                        time_mult = time_mult + tag[d] - '0';
                        if (have_point)
                        {
                            have_tenth = true;
                            d++;
                            break;
                        }
                    }
                    else
                    {
                        have_point = true;
                    }
                    d++;
                }

                if (have_tenth == false)
                    time_mult *= 10;

                *subline_time_mult = time_mult;
                *tag_len = d;

                buf[0] = 0;
                return buf;
            }
            break;
       case 'r':  /* Runtime database Information */
            switch(tag[1])
                {
                     case 'p':  /* Playcount */
                         *flags |= WPS_REFRESH_STATIC;
                         snprintf(buf, buf_size, "%ld", cid3->playcount);
                         return buf;
                     case 'r':  /* Rating */
                         *flags |= WPS_REFRESH_STATIC;
                         *intval = cid3->rating+1;
                         snprintf(buf, buf_size, "%d", cid3->rating);
                         return buf;
                }
            break;
    }
    return NULL;
}

/* Skip to the end of the current %? conditional.
 *
 * fmt     - string to skip it. Should point to somewhere after the leading
 *           "<" char (and before or at the last ">").
 * num     - number of |'s to skip, or 0 to skip to the end (the ">").
 *
 * Returns the new position in fmt.
 */
static const char* skip_conditional(const char* fmt, int num)
{
    int level = 1;
    int count = num;
    const char *last_alternative = NULL;

    while (*fmt)
    {
        switch (*fmt++)
        {
            case '%':
                break;

            case '|':
                if(1 == level) {
                    last_alternative = fmt;
                    if(num) {
                        count--;
                        if(count == 0)
                            return fmt;
                        continue;
                    }
                }
                continue;

            case '>':
                if (0 == --level)
                {
                    /* We're just skipping to the end */
                    if(num == 0)
                        return fmt;

                    /* If we are parsing an enum, we'll return the selected
                       item. If there weren't enough items in the enum, we'll
                       return the last one found. */
                    if(count && last_alternative)
                    {
                        return last_alternative;
                    }
                    return fmt - 1;
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
 * nid3     - the ID3 data of the next song (might by NULL)
 * fmt      - format description.
 * flags    - returns the type of the line. See constants i wps-display.h
 */
static void format_display(char* buf,
                           int buf_size,
                           struct mp3entry* id3,
                           struct mp3entry* nid3, /* next song's id3 */
                           const char* fmt,
                           struct align_pos* align,
                           unsigned short* subline_time_mult,
                           unsigned char* flags)
{
    char temp_buf[128];
    char* buf_start = buf;
    char* buf_end = buf + buf_size - 1;   /* Leave room for end null */
    char* value = NULL;
    int level = 0;
    unsigned char tag_length;
    int intval;

    int cur_align;
    char* cur_align_start;
#ifdef HAVE_LCD_BITMAP
    int n;
    /* Set images to not to be displayed */
    for (n = 0; n < MAX_IMAGES; n++) {
        img[n].display = false;
    }
#endif
    
    cur_align_start = buf;
    cur_align = WPS_ALIGN_LEFT;
    *subline_time_mult = DEFAULT_SUBLINE_TIME_MULTIPLIER;

    align->left = 0;
    align->center = 0;
    align->right = 0;

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
                    fmt = skip_conditional(fmt, 0);
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
            case 'a':
                ++fmt;
                /* remember where the current aligned text started */
                switch (cur_align)
                {
                    case WPS_ALIGN_LEFT:
                        align->left = cur_align_start;
                        break;

                    case WPS_ALIGN_CENTER:
                        align->center = cur_align_start;
                        break;

                    case WPS_ALIGN_RIGHT:
                        align->right = cur_align_start;
                        break;
                }
                /* start a new alignment */
                switch (*fmt)
                {
                    case 'l':
                        cur_align = WPS_ALIGN_LEFT;
                        break;
                    case 'c':
                        cur_align = WPS_ALIGN_CENTER;
                        break;
                    case 'r':
                        cur_align = WPS_ALIGN_RIGHT;
                        break;
                } 
                *buf++=0;
                cur_align_start = buf;
                ++fmt;
                break;
            case 's':
                *flags |= WPS_REFRESH_SCROLL;
                ++fmt;
                break;

            case 'x': /* image support */
#ifdef HAVE_LCD_BITMAP
                /* skip preload or regular image tag */
                if ('l' == *(fmt+1) || '|' == *(fmt+1))
                {
                    while (*fmt && *fmt != '\n')
                        fmt++;
                }
                else if ('d' == *(fmt+1))
                {
                    fmt+=2;
                    n = tolower(*fmt) - 'a';
                    if (n >= 0 && n < MAX_IMAGES && img[n].loaded) {
                        img[n].display = true;
                    }
                }
#endif
                fmt++;
                break;


            case '%':
            case '|':
            case '<':
            case '>':
            case ';':
                *buf++ = *fmt++;
                break;

            case '?':
                fmt++;
                value = get_tag(id3, nid3, fmt, temp_buf, sizeof(temp_buf),
                                &tag_length, subline_time_mult, flags,
                                &intval);

                while (*fmt && ('<' != *fmt))
                    fmt++;

                if ('<' == *fmt)
                    fmt++;

                /* No value, so skip to else part, using a sufficiently high
                   value to "hit" the last part of the conditional */
                if ((!value) || (!strlen(value)))
                    fmt = skip_conditional(fmt, 1000);
                else
                    if(intval > 1) /* enum */
                        fmt = skip_conditional(fmt, intval - 1);

                level++;
                break;

            default:
                value = get_tag(id3, nid3, fmt, temp_buf, sizeof(temp_buf),
                                &tag_length, subline_time_mult, flags,
                                &intval);
                fmt += tag_length;

                if (value)
                {
                    while (*value && (buf < buf_end))
                        *buf++ = *value++;
                }
        }
    }

    /* remember where the current aligned text started */
    switch (cur_align)
    {
        case WPS_ALIGN_LEFT:
            align->left = cur_align_start;
            break;

        case WPS_ALIGN_CENTER:
            align->center = cur_align_start;
            break;

        case WPS_ALIGN_RIGHT:
            align->right = cur_align_start;
            break;
    }

    *buf = 0;

    /* if resulting line is an empty line, set the subline time to 0 */
    if (buf - buf_start == 0)
        *subline_time_mult = 0;

    /* If no flags have been set, the line didn't contain any format codes.
       We still want to refresh it. */
    if(*flags == 0)
        *flags = WPS_REFRESH_STATIC;
}

bool wps_refresh(struct mp3entry* id3,
                 struct mp3entry* nid3,
                 int ffwd_offset,
                 unsigned char refresh_mode)
{
    char buf[MAX_PATH];
    unsigned char flags;
    int i;
    bool update_line;
    bool only_one_subline;
    bool new_subline_refresh;
    int search;
    int search_start;
#ifdef HAVE_LCD_BITMAP
    int h = font_get(FONT_UI)->height;
    int offset = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;
    /* to find out wether the peak meter is enabled we
       assume it wasn't until we find a line that contains
       the peak meter. We can't use peak_meter_enabled itself
       because that would mean to turn off the meter thread
       temporarily. (That shouldn't matter unless yield
       or sleep is called but who knows...)
    */
    bool enable_pm = false;
#endif

    /* reset to first subline if refresh all flag is set */
    if (refresh_mode == WPS_REFRESH_ALL)
    {
        for (i=0; i<MAX_LINES; i++)
        {
            curr_subline[i] = SUBLINE_RESET;
        }
    }

#ifdef HAVE_LCD_CHARCELLS
    for (i=0; i<8; i++) {
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
        new_subline_refresh = false;
        only_one_subline = false;

        /* if time to advance to next sub-line  */
        if (TIME_AFTER(current_tick,  subline_expire_time[i] - 1) ||
           (curr_subline[i] == SUBLINE_RESET))
        {
            /* search all sublines until the next subline with time > 0
               is found or we get back to the subline we started with */
            if (curr_subline[i] == SUBLINE_RESET)
                search_start = 0;
            else
                search_start = curr_subline[i];
            for (search=0; search<MAX_SUBLINES; search++)
            {
                curr_subline[i]++;

                /* wrap around if beyond last defined subline or MAX_SUBLINES */
                if ((!format_lines[i][curr_subline[i]]) ||
                    (curr_subline[i] == MAX_SUBLINES))
                {
                    if (curr_subline[i] == 1)
                        only_one_subline = true;
                    curr_subline[i] = 0;
                }

                /* if back where we started after search or
                   only one subline is defined on the line */
                if (((search > 0) && (curr_subline[i] == search_start)) ||
                    only_one_subline)
                {
                    /* no other subline with a time > 0 exists */
                    subline_expire_time[i] = current_tick  + 100 * HZ;
                    break;
                }
                else
                {
                    /* get initial time multiplier and
                       line type flags for this subline */
                    format_display(buf, sizeof(buf), id3, nid3,
                                   format_lines[i][curr_subline[i]],
                                   &format_align[i][curr_subline[i]],
                                   &time_mult[i][curr_subline[i]],
                                   &line_type[i][curr_subline[i]]);

                    /* only use this subline if subline time > 0 */
                    if (time_mult[i][curr_subline[i]] > 0)
                    {
                        new_subline_refresh = true;
                        subline_expire_time[i] = current_tick  +
                            BASE_SUBLINE_TIME * time_mult[i][curr_subline[i]];
                        break;
                    }
                }
            }

        }

        update_line = false;

        if ( !format_lines[i][curr_subline[i]] )
            break;

        if ((line_type[i][curr_subline[i]] & refresh_mode) ||
            (refresh_mode == WPS_REFRESH_ALL) ||
            new_subline_refresh)
        {
            flags = 0;
#ifdef HAVE_LCD_BITMAP
            int left_width,   left_xpos;
            int center_width, center_xpos;
            int right_width,  right_xpos;
            int space_width;
            int string_height;
            int ypos;
#endif

            format_display(buf, sizeof(buf), id3, nid3,
                           format_lines[i][curr_subline[i]],
                           &format_align[i][curr_subline[i]],
                           &time_mult[i][curr_subline[i]],
                           &flags);
            line_type[i][curr_subline[i]] = flags;

#ifdef HAVE_LCD_BITMAP
            /* progress */
            if (flags & refresh_mode & WPS_REFRESH_PLAYER_PROGRESS) 
            {
#define PROGRESS_BAR_HEIGHT 6 /* this should probably be defined elsewhere; config-*.h perhaps? */
                int sby = i*h + offset + (h > 7 ? (h - 6) / 2 : 1);
                scrollbar(0, sby, LCD_WIDTH, PROGRESS_BAR_HEIGHT,
                    id3->length?id3->length:1, 0,
                    id3->length?id3->elapsed + ff_rewind_count:0,
                    HORIZONTAL);
#ifdef AB_REPEAT_ENABLE
                if ( ab_repeat_mode_enabled() )
                    ab_draw_markers(id3->length, 0, sby, LCD_WIDTH, PROGRESS_BAR_HEIGHT);
#endif
                update_line = true;
            }
            if (flags & refresh_mode & WPS_REFRESH_PEAK_METER) {
                /* peak meter */
                int peak_meter_y;

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
#ifdef HAVE_LCD_BITMAP
            /* calculate different string sizes and positions */
            lcd_getstringsize(" ", &space_width, &string_height);
            if (format_align[i][curr_subline[i]].left != 0) {
                lcd_getstringsize(format_align[i][curr_subline[i]].left,
                                  &left_width, &string_height);
            }
            else {
                left_width = 0;
            }
            left_xpos = 0;

            if (format_align[i][curr_subline[i]].center != 0) {
                lcd_getstringsize(format_align[i][curr_subline[i]].center,
                                  &center_width, &string_height);
            }
            else {
                center_width = 0;
            }
            center_xpos=(LCD_WIDTH - center_width) / 2;

            if (format_align[i][curr_subline[i]].right != 0) {
                lcd_getstringsize(format_align[i][curr_subline[i]].right,
                                  &right_width, &string_height);
            }
            else {
                right_width = 0;
            }
            right_xpos = (LCD_WIDTH - right_width);

            /* Checks for overlapping strings.
               If needed the overlapping strings will be merged, separated by a
               space */

            /* CASE 1: left and centered string overlap */
            /* there is a left string, need to merge left and center */
            if ((left_width != 0 && center_width != 0) &&
                (left_xpos + left_width + space_width > center_xpos)) {
                /* replace the former separator '\0' of left and 
                   center string with a space */
                *(--format_align[i][curr_subline[i]].center) = ' ';
                /* calculate the new width and position of the merged string */
                left_width = left_width + space_width + center_width;
                left_xpos = 0;
                /* there is no centered string anymore */
                center_width = 0;
            }
            /* there is no left string, move center to left */
            if ((left_width == 0 && center_width != 0) &&
                (left_xpos + left_width > center_xpos)) {
                /* move the center string to the left string */
                format_align[i][curr_subline[i]].left = format_align[i][curr_subline[i]].center;
                /* calculate the new width and position of the string */
                left_width = center_width;
                left_xpos = 0;
                /* there is no centered string anymore */
                center_width = 0;
            }

            /* CASE 2: centered and right string overlap */
            /* there is a right string, need to merge center and right */
            if ((center_width != 0 && right_width != 0) &&
                (center_xpos + center_width + space_width > right_xpos)) {
                /* replace the former separator '\0' of center and 
                   right string with a space */
                *(--format_align[i][curr_subline[i]].right) = ' ';
                /* move the center string to the right after merge */
                format_align[i][curr_subline[i]].right = format_align[i][curr_subline[i]].center;
                /* calculate the new width and position of the merged string */
                right_width = center_width + space_width + right_width;
                right_xpos = (LCD_WIDTH - right_width);
                /* there is no centered string anymore */
                center_width = 0;
            }
            /* there is no right string, move center to right */
            if ((center_width != 0 && right_width == 0) &&
                (center_xpos + center_width > right_xpos)) {
                /* move the center string to the right string */
                format_align[i][curr_subline[i]].right = format_align[i][curr_subline[i]].center;
                /* calculate the new width and position of the string */
                right_width = center_width;
                right_xpos = (LCD_WIDTH - right_width);
                /* there is no centered string anymore */
                center_width = 0;
            }

            /* CASE 3: left and right overlap
               There is no center string anymore, either there never
               was one or it has been merged in case 1 or 2 */
            /* there is a left string, need to merge left and right */
            if ((left_width != 0 && center_width == 0 && right_width != 0) &&
                (left_xpos + left_width + space_width > right_xpos)) {
                /* replace the former separator '\0' of left and 
                   right string with a space */
                *(--format_align[i][curr_subline[i]].right) = ' ';
                /* calculate the new width and position of the string */
                left_width = left_width + space_width + right_width;
                left_xpos = 0;
                /* there is no right string anymore */
                right_width = 0;
            }
            /* there is no left string, move right to left */
            if ((left_width == 0 && center_width == 0 && right_width != 0) &&
                (left_xpos + left_width > right_xpos)) {
                /* move the right string to the left string */
                format_align[i][curr_subline[i]].left = format_align[i][curr_subline[i]].right;
                /* calculate the new width and position of the string */
                left_width = right_width;
                left_xpos = 0;
                /* there is no right string anymore */
                right_width = 0;
            }

#endif

            if (flags & WPS_REFRESH_SCROLL) {

                /* scroll line */
                if ((refresh_mode & WPS_REFRESH_SCROLL) ||
                    new_subline_refresh) {
#ifdef HAVE_LCD_BITMAP
                    ypos = (i*string_height)+lcd_getymargin();
                    update_line = true;

                    if (left_width>LCD_WIDTH) {
                        lcd_puts_scroll(0, i, format_align[i][curr_subline[i]].left);
                    } else {
                        /* clear the line first */
                        lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        lcd_fillrect(0, ypos, LCD_WIDTH, string_height);
                        lcd_set_drawmode(DRMODE_SOLID);

                        /* Nasty hack: we output an empty scrolling string,
                           which will reset the scroller for that line */
                        lcd_puts_scroll(0, i, "");
                        
                        /* print aligned strings */
                        if (left_width != 0)
                        {
                            lcd_putsxy(left_xpos, ypos, format_align[i][curr_subline[i]].left);
                        }
                        if (center_width != 0)
                        {
                            lcd_putsxy(center_xpos, ypos, format_align[i][curr_subline[i]].center);
                        }
                        if (right_width != 0)
                        {
                            lcd_putsxy(right_xpos, ypos, format_align[i][curr_subline[i]].right);
                        }
                    }
#else
                    lcd_puts_scroll(0, i, buf);
                    update_line = true;
#endif
                }
            }
            else if (flags & (WPS_REFRESH_DYNAMIC | WPS_REFRESH_STATIC))
            {
                /* dynamic / static line */
                if ((refresh_mode & (WPS_REFRESH_DYNAMIC|WPS_REFRESH_STATIC)) ||
                    new_subline_refresh)
                {
#ifdef HAVE_LCD_BITMAP
                    ypos = (i*string_height)+lcd_getymargin();
                    update_line = true;

                    /* clear the line first */
                    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    lcd_fillrect(0, ypos, LCD_WIDTH, string_height);
                    lcd_set_drawmode(DRMODE_SOLID);

                    /* Nasty hack: we output an empty scrolling string,
                       which will reset the scroller for that line */
                    lcd_puts_scroll(0, i, "");
                        
                    /* print aligned strings */
                    if (left_width != 0)
                    {
                        lcd_putsxy(left_xpos, ypos, format_align[i][curr_subline[i]].left);
                    }
                    if (center_width != 0)
                    {
                        lcd_putsxy(center_xpos, ypos, format_align[i][curr_subline[i]].center);
                    }
                    if (right_width != 0)
                    {
                        lcd_putsxy(right_xpos, ypos, format_align[i][curr_subline[i]].right);
                    }
#else
                    update_line = true;
                    lcd_puts(0, i, buf);
#endif
                }
            }
        }
#ifdef HAVE_LCD_BITMAP
        if (update_line) {
            lcd_update_rect(0, i*h + offset, LCD_WIDTH, h);
            wps_display_images();
        }
#endif
    }

#ifdef HAVE_LCD_BITMAP
    /* Display images marked as "always display" */
    for (i = 0; i < MAX_IMAGES; i++) {
        img[i].display = img[i].always_display;
    }
    wps_display_images();
    
    /* Now we know wether the peak meter is used.
       So we can enable / disable the peak meter thread */
    peak_meter_enabled = enable_pm;
#endif

#if defined(CONFIG_BACKLIGHT) && !defined(SIMULATOR)
    if (global_settings.caption_backlight && id3) {
        /* turn on backlight n seconds before track ends, and turn it off n
           seconds into the new track. n == backlight_timeout, or 5s */
        int n =
            backlight_timeout_value[global_settings.backlight_timeout] * 1000;

        if ( n < 1000 )
            n = 5000; /* use 5s if backlight is always on or off */

        if ((id3->elapsed < 1000) ||
            ((id3->length - id3->elapsed) < (unsigned)n))
            backlight_on();
    }
#endif
    return true;
}

bool wps_display(struct mp3entry* id3,
                 struct mp3entry* nid3)
{
    if (!id3 && !(audio_status() & AUDIO_STATUS_PLAY))
    {
        global_settings.resume_index = -1;
#ifdef HAVE_LCD_CHARCELLS
        splash(HZ, true, str(LANG_END_PLAYLIST_PLAYER));
#else
        status_draw(true);
        splash(HZ, true, str(LANG_END_PLAYLIST_RECORDER));
#endif
        return true;
    }
    else
    {
        lcd_clear_display();

        if (!wps_loaded) {
            if ( !format_buffer[0] ) {
#ifdef HAVE_LCD_BITMAP
                wps_format("%s%?it<%?in<%in. |>%it|%fn>\n"
                           "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
                           "%s%?id<%id|%?d1<%d1|(root)>> %?iy<(%iy)|>\n"
                           "\n"
                           "%al%pc/%pt%ar[%pp:%pe]\n"
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
    wps_refresh(id3, nid3, 0, WPS_REFRESH_ALL);
    status_draw(true);
#ifdef HAVE_LCD_BITMAP
    wps_display_images();
#endif
    lcd_update();
    return false;
}

#ifdef HAVE_LCD_CHARCELLS
static bool draw_player_progress(const struct mp3entry* id3,
                                 int ff_rewwind_count)
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
                                const struct mp3entry* id3,
                                int ff_rewwind_count)
{
    int i,j,lcd_char_pos;

    char player_progressbar[7];
    char binline[36];
    static const char numbers[12][4][3]={
        {{1,1,1},{1,0,1},{1,0,1},{1,1,1}},/*0*/
        {{0,1,0},{1,1,0},{0,1,0},{0,1,0}},/*1*/
        {{1,1,1},{0,0,1},{0,1,0},{1,1,1}},/*2*/
        {{1,1,1},{0,0,1},{0,1,1},{1,1,1}},/*3*/
        {{1,0,0},{1,1,0},{1,1,1},{0,1,0}},/*4*/
        {{1,1,1},{1,1,0},{0,0,1},{1,1,0}},/*5*/
        {{1,1,1},{1,0,0},{1,1,1},{1,1,1}},/*6*/
        {{1,1,1},{0,0,1},{0,1,0},{1,0,0}},/*7*/
        {{1,1,1},{1,1,1},{1,0,1},{1,1,1}},/*8*/
        {{1,1,1},{1,1,1},{0,0,1},{1,1,1}},/*9*/
        {{0,0,0},{0,1,0},{0,0,0},{0,1,0}},/*:*/
        {{0,0,0},{0,0,0},{0,0,0},{0,0,0}} /*<blank>*/
    };

    int songpos = 0;
    int digits[6];
    int time;
    char timestr[7];

    for (i=0; i < buf_size; i++)
        buf[i] = ' ';

    if(id3->elapsed >= id3->length)
        songpos = 55;
    else {
        if(wps_time_countup == false)
            songpos = ((id3->elapsed - ff_rewwind_count) * 55) / id3->length;
        else
            songpos = ((id3->elapsed + ff_rewwind_count) * 55) / id3->length;
    }

    time=(id3->elapsed + ff_rewind_count);

    memset(timestr, 0, sizeof(timestr));
    wps_format_time(timestr, sizeof(timestr), time);
    for(lcd_char_pos=0; lcd_char_pos<6; lcd_char_pos++) {
        digits[lcd_char_pos] = map_fullbar_char(timestr[lcd_char_pos]);
    }

    /* build the progressbar-icons */
    for (lcd_char_pos=0; lcd_char_pos<6; lcd_char_pos++) {
        memset(binline, 0, sizeof binline);
        memset(player_progressbar, 0, sizeof player_progressbar);

        /* make the character (progressbar & digit)*/
        for (i=0; i<7; i++) {
            for (j=0;j<5;j++) {
                /* make the progressbar */
                if (lcd_char_pos==(songpos/5)) {
                    /* partial */
                    if ((j<(songpos%5))&&(i>4))
                        binline[i*5+j] = 1;
                    else
                        binline[i*5+j] = 0;
                }
                else {
                    if (lcd_char_pos<(songpos/5)) {
                        /* full character */
                        if (i>4)
                            binline[i*5+j] = 1;
                    }
                }
                /* insert the digit */
                if ((j<3)&&(i<4)) {
                    if (numbers[digits[lcd_char_pos]][i][j]==1)
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

        lcd_define_pattern(wps_progress_pat[lcd_char_pos+1],player_progressbar);
        buf[lcd_char_pos]=wps_progress_pat[lcd_char_pos+1];

    }

    /* make rest of the progressbar if necessary */
    if (songpos/5>5) {

        /* set the characters positions that use the full 5 pixel wide bar */
        for (lcd_char_pos=6; lcd_char_pos < (songpos/5); lcd_char_pos++)
            buf[lcd_char_pos] = 0x86; /* '_' */

        /* build the partial bar character for the tail character position */
        memset(binline, 0, sizeof binline);
        memset(player_progressbar, 0, sizeof player_progressbar);

        for (i=5; i<7; i++) {
            for (j=0;j<5;j++) {
                if (j<(songpos%5)) {
                    binline[i*5+j] = 1;
                }
            }
        }

        for (i=0; i<7; i++) {
            for (j=0;j<5;j++) {
                player_progressbar[i] <<= 1;
                player_progressbar[i] += binline[i*5+j];
            }
        }

        lcd_define_pattern(wps_progress_pat[7],player_progressbar);

        buf[songpos/5]=wps_progress_pat[7];
    }
}

static char map_fullbar_char(char ascii_val)
{
    if (ascii_val >= '0' && ascii_val <= '9') {
        return(ascii_val - '0');
    }
    else if (ascii_val == ':') {
        return(10);
    }
    else
        return(11); /* anything besides a number or ':' is mapped to <blank> */
}


#endif

/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
