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
#include "gwps-common.h"
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "settings.h"
#include "audio.h"
#include "status.h"
#include "power.h"
#include "powermgmt.h"
#include "sound.h"
#ifdef HAVE_LCD_CHARCELLS
#include "hwcompat.h"
#endif
#include "mp3_playback.h"
#include "backlight.h"
#include "lang.h"
#include "misc.h"

#include "splash.h"
#include "scrollbar.h"
#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
/* Image stuff */
#include "bmp.h"
#include "atoi.h"
#endif

#ifdef HAVE_LCD_CHARCELLS
static bool draw_player_progress(struct gui_wps *gwps);
static void draw_player_fullbar(struct gui_wps *gwps,
                                char* buf, int buf_size);
#endif

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP 500

/*
 * returns the image_id between
 * a..z and A..Z
 */
#ifdef HAVE_LCD_BITMAP
static int get_image_id(int c)
{
    if(c >= 'a' && c <= 'z')
        c -= 'a';
    if(c >= 'A' && c <= 'Z')
        c = c - 'A' + 26;
    return c;
}
#endif
/*
 * parse the given buffer for following static tags:
 * %x    -   load image for always display
 * %xl   -   preload image
 * %we   -   enable statusbar on wps regardless of the global setting
 * %wd   -   disable statusbar on wps regardless of the global setting
 * and also for:
 * #     -   a comment line
 *
 * it returns true if one of these tags is found and handled
 * false otherwise
 */
bool wps_data_preload_tags(struct wps_data *data, unsigned char *buf,
                            const char *bmpdir, size_t bmpdirlen)
{
    if(!data || !buf) return false;
    
    char c;
#ifndef HAVE_LCD_BITMAP
    /* no bitmap-lcd == no bitmap loading */
    (void)bmpdir;
    (void)bmpdirlen;
#endif
    /* jump over the UTF-8 BOM(Byte Order Mark) if exist
     * the BOM for UTF-8 is 3 bytes long and looks like so:
     * 1. Byte:  0xEF
     * 2. Byte:  0xBB
     * 3. Byte:  0xBF
     */
    if(buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
        buf+=3;
    
    if(*buf == '#')
        return true;
    if('%' != *buf)
        return false;
    buf++;
    
    c  = *buf;
    switch (c)
    {
#ifdef HAVE_LCD_BITMAP
        case 'w':
            /* 
             * if tag found then return because these two tags must be on 
             * must be on their own line
             */
            if(*(buf+1) == 'd' || *(buf+1) == 'e')
            {
                data->wps_sb_tag = true;
                if( *(buf+1) == 'e' )
                    data->show_sb_on_wps = true;
                return true;
            }
        break;
        
        case 'x':
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
                    /* get the image ID */
                    n = get_image_id(*ptr);
                    
                    if(n < 0 || n >= MAX_IMAGES)
                    {
                        /* Skip the rest of the line */
                        while(*buf != '\n')
                            buf++;
                        return false;
                    }
                    ptr = pos+1;

                    /* check the image number and load state */
                    if (data->img[n].loaded)
                    {
                        /* Skip the rest of the line */
                        while(*buf != '\n')
                            buf++;
                        return false;
                    }
                    else
                    {
                        /* get filename */
                        pos = strchr(ptr, '|');
                        if ((pos - ptr) <
                            (int)sizeof(imgname)-ROCKBOX_DIR_LEN-2)
                        {
                            memcpy(imgname, bmpdir, bmpdirlen);
                            imgname[bmpdirlen] = '/';
                            memcpy(&imgname[bmpdirlen+1],
                                   ptr, pos - ptr);
                            imgname[bmpdirlen+1+pos-ptr] = 0;
                        }
                        else
                            /* filename too long */
                            imgname[0] = 0;

                        ptr = pos+1;

                        /* get x-position */
                        pos = strchr(ptr, '|');
                        if (pos)
                            data->img[n].x = atoi(ptr);
                        else
                        {
                            /* weird syntax, bail out */
                            buf++;
                            return false;
                        }

                        /* get y-position */
                        ptr = pos+1;
                        pos = strchr(ptr, '|');
                        if (pos)
                            data->img[n].y = atoi(ptr);
                        else
                        {
                            /* weird syntax, bail out */
                            buf++;
                            return false;
                        }

                        /* load the image */
                        ret = read_bmp_file(imgname, &data->img[n].w,
                                            &data->img[n].h, data->img_buf_ptr,
                                            data->img_buf_free);
                        if (ret > 0)
                        {
                            data->img[n].ptr = data->img_buf_ptr;
                            data->img_buf_ptr += ret;
                            data->img_buf_free -= ret;
                            data->img[n].loaded = true;
                            if(qual == '|')
                                data->img[n].always_display = true;
                        }
                        return true;
                    }
                }
            }
        }

        break;
#endif
    }
    /* no of these tags found */
    return false;
}


/* draws the statusbar on the given wps-screen */
#ifdef HAVE_LCD_BITMAP
static void gui_wps_statusbar_draw(struct gui_wps *wps, bool force)
{
    bool draw = global_settings.statusbar;
    
    if(wps->data->wps_sb_tag 
        && wps->data->show_sb_on_wps)
        draw = true;
    else if(wps->data->wps_sb_tag)
        draw = false;
    if(draw)
        gui_statusbar_draw(wps->statusbar, force);
}
#else
#define gui_wps_statusbar_draw(wps, force) \
    gui_statusbar_draw((wps)->statusbar, (force))
#endif

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * time     - time to format, in milliseconds.
 */
void gui_wps_format_time(char* buf, int buf_size, long time)
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
static char* get_tag(struct wps_data* wps_data,
                     struct mp3entry* cid3,
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
#ifndef HAVE_LCD_CHARCELLS
    (void)wps_data;
#endif
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
                    switch (id3->id3version)
                    {
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
                    snprintf(buf, buf_size, "%ld", id3->frequency);
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
                    snprintf(buf, buf_size, "%ld", id3->filesize / 1024);
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
                    snprintf(buf, buf_size, "%c",
                             wps_data->wps_progress_pat[0]);
                    wps_data->full_line_progressbar=0;
                    return buf;
#else
                    return "\x01";
#endif
                case 'f':  /* full-line progress bar */
#ifdef HAVE_LCD_CHARCELLS
                    if(is_new_player()) {
                        *flags |= WPS_REFRESH_PLAYER_PROGRESS;
                        *flags |= WPS_REFRESH_DYNAMIC;
                        wps_data->full_line_progressbar=1;
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
                    snprintf(buf, buf_size, "%d",
                             playlist_get_display_index());
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
                    gui_wps_format_time(buf, buf_size,
                                    id3->elapsed + wps_state.ff_rewind_count);
                    return buf;

                case 'r': /* Remaining Time in Song */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    gui_wps_format_time(buf, buf_size,
                                        id3->length - id3->elapsed -
                                        wps_state.ff_rewind_count);
                    return buf;

                case 't':  /* Total Time */
                    *flags |= WPS_REFRESH_STATIC;
                    gui_wps_format_time(buf, buf_size, id3->length);
                    return buf;

#ifdef HAVE_LCD_BITMAP
                case 'm': /* Peak Meter */
                    *flags |= WPS_REFRESH_PEAK_METER;
                    return "\x01";
#endif
                case 's': /* shuffle */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    if ( global_settings.playlist_shuffle )
                        return "s";
                    else
                        return NULL;
                    break;

                case 'v': /* volume */
                    *flags |= WPS_REFRESH_DYNAMIC;
                    snprintf(buf, buf_size, "%d", global_settings.volume);
                    *intval = global_settings.volume / 10 + 1;
                    return buf;

            }
            break;
            
        case 'm':
            switch (tag[1])
            {
                case 'm': /* playback repeat mode */
                    *flags |= WPS_REFRESH_DYNAMIC;
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
            switch (tag[1])
            {
                case 'l': /* battery level */
                {
                    int l = battery_level();
                    if (l > -1)
                    {
                        snprintf(buf, buf_size, "%d", l);
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
                
		case 'p': /* External power plugged in? */
		{
		    if(charger_inserted())
			return "p";
		    else
			return NULL;
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

#ifdef HAVE_LCD_BITMAP
/* clears the area where the image was shown */
static void clear_image_pos(struct gui_wps *gwps, int n)
{
    if(!gwps)
        return;
    struct wps_data *data = gwps->data;
    gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    gwps->display->fillrect(data->img[n].x, data->img[n].y, 
                            data->img[n].w, data->img[n].h);
    gwps->display->set_drawmode(DRMODE_SOLID);
}
#endif

/* Skip to the end of the current %? conditional.
 *
 * fmt     - string to skip it. Should point to somewhere after the leading
 *           "<" char (and before or at the last ">").
 * num     - number of |'s to skip, or 0 to skip to the end (the ">").
 *
 * Returns the new position in fmt.
 */
static const char* skip_conditional(struct gui_wps *gwps, const char* fmt,
                                    int num)
{
    int level = 1;
    int count = num;
    const char *last_alternative = NULL;
#ifdef HAVE_LCD_BITMAP
    struct wps_data *data = NULL;
    int last_x=-1, last_y=-1, last_w=-1, last_h=-1;
    if(gwps)
        data = gwps->data;
#else
    (void)gwps;
#endif
    while (*fmt)
    {
        switch (*fmt++)
        {
            case '%':
#ifdef HAVE_LCD_BITMAP
                if(data && *(fmt) == 'x' && *(fmt+1) == 'd' )
                {
                    fmt +=2;
                    int n = *fmt;
                    if(n >= 'a' && n <= 'z')
                        n -= 'a';
                    if(n >= 'A' && n <= 'Z')
                        n = n - 'A' + 26;
                    if(last_x != data->img[n].x || last_y != data->img[n].y
                       || last_w != data->img[n].w || last_h != data->img[n].h)
                    {
                        last_x = data->img[n].x;
                        last_y = data->img[n].y;
                        last_w = data->img[n].w;
                        last_h = data->img[n].h;
                        clear_image_pos(gwps,n);
                    }
                }
#endif
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
static void format_display(struct gui_wps *gwps, char* buf,
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
    struct gui_img *img = gwps->data->img;
    int n;
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
                    fmt = skip_conditional(NULL,fmt, 0);
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
                if ('d' == *(fmt+1) )
                {
                    fmt+=2;

                    /* get the image ID */
                    n = *fmt;
                    if(n >= 'a' && n <= 'z')
                        n -= 'a';
                    if(n >= 'A' && n <= 'Z')
                        n = n - 'A' + 26;
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
                value = get_tag(gwps->data, id3, nid3, fmt, temp_buf,
                                sizeof(temp_buf),&tag_length,
                                subline_time_mult, flags, &intval);

                while (*fmt && ('<' != *fmt))
                    fmt++;

                if ('<' == *fmt)
                    fmt++;

                /* No value, so skip to else part, using a sufficiently high
                   value to "hit" the last part of the conditional */
                if ((!value) || (!strlen(value)))
                    fmt = skip_conditional(gwps, fmt, 1000);
                else
                    if(intval > 1) /* enum */
                        fmt = skip_conditional(gwps, fmt, intval - 1);

                level++;
                break;

            default:
                value = get_tag(gwps->data, id3, nid3, fmt, temp_buf,
                                sizeof(temp_buf), &tag_length,
                                subline_time_mult, flags,&intval);
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

/* fades the volume */
void fade(bool fade_in)
{
    unsigned fp_global_vol = global_settings.volume << 8;
    unsigned fp_step = fp_global_vol / 30;

    if (fade_in) {
        /* fade in */
        unsigned fp_volume = 0;

        /* zero out the sound */
        sound_set_volume(0);

        sleep(HZ/10); /* let audio thread run */
        audio_resume();
        
        while (fp_volume < fp_global_vol) {
            fp_volume += fp_step;
            sleep(1);
            sound_set_volume(fp_volume >> 8);
        }
        sound_set_volume(global_settings.volume);
    }
    else {
        /* fade out */
        unsigned fp_volume = fp_global_vol;

        while (fp_volume > fp_step) {
            fp_volume -= fp_step;
            sleep(1);
            sound_set_volume(fp_volume >> 8);
        }
        audio_pause();
#ifndef SIMULATOR
        /* let audio thread run and wait for the mas to run out of data */
        while (!mp3_pause_done())
#endif
            sleep(HZ/10);

        /* reset volume to what it was before the fade */
        sound_set_volume(global_settings.volume);
    }
}

/* Set format string to use for WPS, splitting it into lines */
void gui_wps_format(struct wps_data *data)
{
    char* buf = data->format_buffer;
    char* start_of_line = data->format_buffer;
    int line = 0;
    int subline;
    char c;
    if(!data)
        return;

    for (line=0; line<WPS_MAX_LINES; line++)
    {
        for (subline=0; subline<WPS_MAX_SUBLINES; subline++)
        {
            data->format_lines[line][subline] = 0;
            data->time_mult[line][subline] = 0;
        }
        data->subline_expire_time[line] = 0;
        data->curr_subline[line] = SUBLINE_RESET;
    }
    
    line = 0;
    subline = 0;
    data->format_lines[line][subline] = buf;

    while ((*buf) && (line < WPS_MAX_LINES))
    {
        c = *buf;
        
        switch (c)
        {
            /*
             * skip % sequences so "%;" doesn't start a new subline
             * don't skip %x lines (pre-load bitmaps)
             */
            case '%':
                buf++;
                break;

            case '\r': /* CR */
                *buf = 0;
                break;

            case '\n': /* LF */
                *buf = 0;

                if (*start_of_line != '#') /* A comment? */
                    line++;

                if (line < WPS_MAX_LINES)
                {
                    /* the next line starts on the next byte */
                    subline = 0;
                    data->format_lines[line][subline] = buf+1;
                    start_of_line = data->format_lines[line][subline];
                }
                break;

            case ';': /* start a new subline */
                *buf = 0;
                subline++;
                if (subline < WPS_MAX_SUBLINES)
                {
                    data->format_lines[line][subline] = buf+1;
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
        }
        buf++;
    }
}

#ifdef HAVE_LCD_BITMAP
/* Display images */
static void wps_display_images(struct gui_wps *gwps)
{
    if(!gwps || !gwps->data || !gwps->display) return;
    int n;
    struct wps_data *data = gwps->data;
    struct screen *display = gwps->display;
    for (n = 0; n < MAX_IMAGES; n++) {
        if (data->img[n].loaded && data->img[n].display) {
            if(data->img[n].always_display)
                display->set_drawmode(DRMODE_FG);
            else
                display->set_drawmode(DRMODE_SOLID);

            display->mono_bitmap(data->img[n].ptr, data->img[n].x,
                                 data->img[n].y, data->img[n].w,
                                 data->img[n].h);
            display->update_rect(data->img[n].x, data->img[n].y,
                                 data->img[n].w, data->img[n].h);
        }
    }
    display->set_drawmode(DRMODE_SOLID);
}
#endif

void gui_wps_reset(struct gui_wps *gui_wps)
{
    if(!gui_wps || !gui_wps->data)
        return;
    gui_wps->data->wps_loaded = false;
    memset(&gui_wps->data->format_buffer, 0,
           sizeof(gui_wps->data->format_buffer));
}

bool gui_wps_refresh(struct gui_wps *gwps, int ffwd_offset,
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
    struct wps_data *data = gwps->data;
    struct wps_state *state = gwps->state;
    struct screen *display = gwps->display;
    if(!gwps || !data || !state || !display)
    {
        return false;
    }
#ifdef HAVE_LCD_BITMAP
    int h = font_get(FONT_UI)->height;
    int offset = 0;
    gui_wps_statusbar_draw(gwps, true);
    if(data->wps_sb_tag && data->show_sb_on_wps)
        offset = STATUSBAR_HEIGHT;
    else if ( global_settings.statusbar && !data->wps_sb_tag)
        offset = STATUSBAR_HEIGHT;
    
    /* to find out wether the peak meter is enabled we
       assume it wasn't until we find a line that contains
       the peak meter. We can't use peak_meter_enabled itself
       because that would mean to turn off the meter thread
       temporarily. (That shouldn't matter unless yield
       or sleep is called but who knows...)
    */
    bool enable_pm = false;

    /* Set images to not to be displayed */
    for (i = 0; i < MAX_IMAGES; i++) {
        data->img[i].display = false;
    }
#endif
    /* reset to first subline if refresh all flag is set */
    if (refresh_mode == WPS_REFRESH_ALL)
    {
        for (i=0; i<WPS_MAX_LINES; i++)
        {
            data->curr_subline[i] = SUBLINE_RESET;
        }
    }

#ifdef HAVE_LCD_CHARCELLS
    for (i=0; i<8; i++) {
       if (data->wps_progress_pat[i]==0)
           data->wps_progress_pat[i]=display->get_locked_pattern();
    }
#endif

    if (!state->id3)
    {
        display->stop_scroll();
        return false;
    }

    state->ff_rewind_count = ffwd_offset;

    for (i = 0; i < WPS_MAX_LINES; i++)
    {
        new_subline_refresh = false;
        only_one_subline = false;

        /* if time to advance to next sub-line  */
        if (TIME_AFTER(current_tick,  data->subline_expire_time[i] - 1) ||
           (data->curr_subline[i] == SUBLINE_RESET))
        {
            /* search all sublines until the next subline with time > 0
               is found or we get back to the subline we started with */
            if (data->curr_subline[i] == SUBLINE_RESET)
                search_start = 0;
            else
                search_start = data->curr_subline[i];
            for (search=0; search<WPS_MAX_SUBLINES; search++)
            {
                data->curr_subline[i]++;

                /* wrap around if beyond last defined subline or WPS_MAX_SUBLINES */
                if ((!data->format_lines[i][data->curr_subline[i]]) ||
                    (data->curr_subline[i] == WPS_MAX_SUBLINES))
                {
                    if (data->curr_subline[i] == 1)
                        only_one_subline = true;
                    data->curr_subline[i] = 0;
                }

                /* if back where we started after search or
                   only one subline is defined on the line */
                if (((search > 0) && (data->curr_subline[i] == search_start)) ||
                    only_one_subline)
                {
                    /* no other subline with a time > 0 exists */
                    data->subline_expire_time[i] = current_tick  + 100 * HZ;
                    break;
                }
                else
                {
                    /* get initial time multiplier and
                       line type flags for this subline */
                    format_display(gwps, buf, sizeof(buf),
                                   state->id3, state->nid3,
                                   data->format_lines[i][data->curr_subline[i]],
                                   &data->format_align[i][data->curr_subline[i]],
                                   &data->time_mult[i][data->curr_subline[i]],
                                   &data->line_type[i][data->curr_subline[i]]);

                    /* only use this subline if subline time > 0 */
                    if (data->time_mult[i][data->curr_subline[i]] > 0)
                    {
                        new_subline_refresh = true;
                        data->subline_expire_time[i] = current_tick  +
                            BASE_SUBLINE_TIME * data->time_mult[i][data->curr_subline[i]];
                        break;
                    }
                }
            }

        }

        update_line = false;

        if ( !data->format_lines[i][data->curr_subline[i]] )
            break;

        if ((data->line_type[i][data->curr_subline[i]] & refresh_mode) ||
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

            format_display(gwps, buf, sizeof(buf),
                           state->id3, state->nid3,
                           data->format_lines[i][data->curr_subline[i]],
                           &data->format_align[i][data->curr_subline[i]],
                           &data->time_mult[i][data->curr_subline[i]],
                           &flags);
            data->line_type[i][data->curr_subline[i]] = flags;

#ifdef HAVE_LCD_BITMAP
            /* progress */
            if (flags & refresh_mode & WPS_REFRESH_PLAYER_PROGRESS) 
            {
#define PROGRESS_BAR_HEIGHT 6 /* this should probably be defined elsewhere; config-*.h perhaps? */
                int sby = i*h + offset + (h > 7 ? (h - 6) / 2 : 1);
                gui_scrollbar_draw(display, 0, sby, display->width,
                                    PROGRESS_BAR_HEIGHT,
                                    state->id3->length?state->id3->length:1, 0,
                                    state->id3->length?state->id3->elapsed + state->ff_rewind_count:0,
                                    HORIZONTAL);
#ifdef AB_REPEAT_ENABLE
                if ( ab_repeat_mode_enabled() )
                    ab_draw_markers(state->id3->length, 0, sby, LCD_WIDTH,
                                    PROGRESS_BAR_HEIGHT);
#endif
                update_line = true;
            }
            if (flags & refresh_mode & WPS_REFRESH_PEAK_METER && display->height >= LCD_HEIGHT) {
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
                if (data->full_line_progressbar)
                    draw_player_fullbar(gwps, buf, sizeof(buf));
                else
                    draw_player_progress(gwps);
            }
#endif
#ifdef HAVE_LCD_BITMAP
            /* calculate different string sizes and positions */
            display->getstringsize((unsigned char *)" ", &space_width, &string_height);
            if (data->format_align[i][data->curr_subline[i]].left != 0) {
                display->getstringsize((unsigned char *)data->format_align[i]
                                       [data->curr_subline[i]].left,
                                       &left_width, &string_height);
            }
            else {
                left_width = 0;
            }
            left_xpos = 0;

            if (data->format_align[i][data->curr_subline[i]].center != 0) {
                display->getstringsize((unsigned char *)data->format_align[i]
                                       [data->curr_subline[i]].center,
                                       &center_width, &string_height);
            }
            else {
                center_width = 0;
            }
            center_xpos=(display->width - center_width) / 2;

            if (data->format_align[i][data->curr_subline[i]].right != 0) {
                display->getstringsize((unsigned char *)data->format_align[i]
                                       [data->curr_subline[i]].right,
                                       &right_width, &string_height);
            }
            else {
                right_width = 0;
            }
            right_xpos = (display->width - right_width);

            /* Checks for overlapping strings.
               If needed the overlapping strings will be merged, separated by a
               space */

            /* CASE 1: left and centered string overlap */
            /* there is a left string, need to merge left and center */
            if ((left_width != 0 && center_width != 0) &&
                (left_xpos + left_width + space_width > center_xpos)) {
                /* replace the former separator '\0' of left and 
                   center string with a space */
                *(--data->format_align[i][data->curr_subline[i]].center) = ' ';
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
                data->format_align[i][data->curr_subline[i]].left =
                  data->format_align[i][data->curr_subline[i]].center;
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
                *(--data->format_align[i][data->curr_subline[i]].right) = ' ';
                /* move the center string to the right after merge */
                data->format_align[i][data->curr_subline[i]].right = 
                  data->format_align[i][data->curr_subline[i]].center;
                /* calculate the new width and position of the merged string */
                right_width = center_width + space_width + right_width;
                right_xpos = (display->width - right_width);
                /* there is no centered string anymore */
                center_width = 0;
            }
            /* there is no right string, move center to right */
            if ((center_width != 0 && right_width == 0) &&
                (center_xpos + center_width > right_xpos)) {
                /* move the center string to the right string */
                data->format_align[i][data->curr_subline[i]].right =
                  data->format_align[i][data->curr_subline[i]].center;
                /* calculate the new width and position of the string */
                right_width = center_width;
                right_xpos = (display->width - right_width);
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
                *(--data->format_align[i][data->curr_subline[i]].right) = ' ';
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
                data->format_align[i][data->curr_subline[i]].left =
                  data->format_align[i][data->curr_subline[i]].right;
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
                    ypos = (i*string_height)+display->getymargin();
                    update_line = true;

                    if (left_width>display->width) {
                        display->puts_scroll(0, i,
                                             (unsigned char *)data->format_align[i]
                                             [data->curr_subline[i]].left);
                    } else {
                        /* clear the line first */
                        display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        display->fillrect(0, ypos, display->width, string_height);
                        display->set_drawmode(DRMODE_SOLID);

                        /* Nasty hack: we output an empty scrolling string,
                           which will reset the scroller for that line */
                        display->puts_scroll(0, i, (unsigned char *)"");
                        
                        /* print aligned strings */
                        if (left_width != 0)
                        {
                            display->putsxy(left_xpos, ypos,
                                            (unsigned char *)data->format_align[i]
                                            [data->curr_subline[i]].left);
                        }
                        if (center_width != 0)
                        {
                            display->putsxy(center_xpos, ypos, 
                                            (unsigned char *)data->format_align[i]
                                            [data->curr_subline[i]].center);
                        }
                        if (right_width != 0)
                        {
                            display->putsxy(right_xpos, ypos,
                                            (unsigned char *)data->format_align[i]
                                            [data->curr_subline[i]].right);
                        }
                    }
#else
                    display->puts_scroll(0, i, buf);
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
                    ypos = (i*string_height)+display->getymargin();
                    update_line = true;

                    /* clear the line first */
                    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    display->fillrect(0, ypos, display->width, string_height);
                    display->set_drawmode(DRMODE_SOLID);

                    /* Nasty hack: we output an empty scrolling string,
                       which will reset the scroller for that line */
                    display->puts_scroll(0, i, (unsigned char *)"");
                        
                    /* print aligned strings */
                    if (left_width != 0)
                    {
                        display->putsxy(left_xpos, ypos,
                                        (unsigned char *)data->format_align[i]
                                        [data->curr_subline[i]].left);
                    }
                    if (center_width != 0)
                    {
                        display->putsxy(center_xpos, ypos,
                                        (unsigned char *)data->format_align[i]
                                        [data->curr_subline[i]].center);
                    }
                    if (right_width != 0)
                    {
                        display->putsxy(right_xpos, ypos,
                                        (unsigned char *)data->format_align[i]
                                        [data->curr_subline[i]].right);
                    }
#else
                    update_line = true;
                    display->puts(0, i, buf);
#endif
                }
            }
        }
#ifdef HAVE_LCD_BITMAP
        if (update_line) {
            display->update_rect(0, i*h + offset, display->width, h);
        }
#endif
    }

#ifdef HAVE_LCD_BITMAP
    /* Display all images */
    for (i = 0; i < MAX_IMAGES; i++) {
        if(data->img[i].always_display)
           data->img[i].display = data->img[i].always_display;
    }
    wps_display_images(gwps);
    
    /* Now we know wether the peak meter is used.
       So we can enable / disable the peak meter thread */
    data->peak_meter_enabled = enable_pm;
#endif

#ifdef CONFIG_BACKLIGHT
    if (global_settings.caption_backlight && state->id3) {
        /* turn on backlight n seconds before track ends, and turn it off n
           seconds into the new track. n == backlight_timeout, or 5s */
        int n = backlight_timeout_value[global_settings.backlight_timeout] 
              * 1000;

        if ( n < 1000 )
            n = 5000; /* use 5s if backlight is always on or off */

        if ((state->id3->elapsed < 1000) ||
            ((state->id3->length - state->id3->elapsed) < (unsigned)n))
            backlight_on();
    }
#endif
#ifdef HAVE_REMOTE_LCD
    if (global_settings.remote_caption_backlight && state->id3) {
        /* turn on remote backlight n seconds before track ends, and turn it
           off n seconds into the new track. n == remote_backlight_timeout,
           or 5s */
        int n = backlight_timeout_value[global_settings.remote_backlight_timeout]
              * 1000;

        if ( n < 1000 )
            n = 5000; /* use 5s if backlight is always on or off */

        if ((state->id3->elapsed < 1000) ||
            ((state->id3->length - state->id3->elapsed) < (unsigned)n))
            remote_backlight_on();
    }
#endif
    return true;
}

#ifdef HAVE_LCD_CHARCELLS
static bool draw_player_progress(struct gui_wps *gwps)
{
    char player_progressbar[7];
    char binline[36];
    int songpos = 0;
    int i,j;
    struct wps_state *state = gwps->state;
    struct screen *display = gwps->display;
    if (!state->id3)
        return false;

    memset(binline, 1, sizeof binline);
    memset(player_progressbar, 1, sizeof player_progressbar);

    if(state->id3->elapsed >= state->id3->length)
        songpos = 0;
    else
    {
        if(state->wps_time_countup == false)
            songpos = ((state->id3->elapsed - state->ff_rewind_count) * 36) /
                            state->id3->length;
        else
            songpos = ((state->id3->elapsed + state->ff_rewind_count) * 36) / 
                            state->id3->length;
    }
    for (i=0; i < songpos; i++)
        binline[i] = 0;

    for (i=0; i<=6; i++) {
        for (j=0;j<5;j++) {
            player_progressbar[i] <<= 1;
            player_progressbar[i] += binline[i*5+j];
        }
    }
    display->define_pattern(gwps->data->wps_progress_pat[0], player_progressbar);
    return true;
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

static void draw_player_fullbar(struct gui_wps *gwps, char* buf, int buf_size)
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
    
    struct wps_state *state = gwps->state;
    struct screen *display = gwps->display;
    struct wps_data *data = gwps->data;

    for (i=0; i < buf_size; i++)
        buf[i] = ' ';

    if(state->id3->elapsed >= state->id3->length)
        songpos = 55;
    else {
        if(state->wps_time_countup == false)
            songpos = ((state->id3->elapsed - state->ff_rewind_count) * 55) / 
                                state->id3->length;
        else
            songpos = ((state->id3->elapsed + state->ff_rewind_count) * 55) / 
                                state->id3->length;
    }

    time=(state->id3->elapsed + state->ff_rewind_count);

    memset(timestr, 0, sizeof(timestr));
    gui_wps_format_time(timestr, sizeof(timestr), time);
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

        display->define_pattern(data->wps_progress_pat[lcd_char_pos+1],
                                player_progressbar);
        buf[lcd_char_pos]=data->wps_progress_pat[lcd_char_pos+1];

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

        display->define_pattern(data->wps_progress_pat[7],player_progressbar);

        buf[songpos/5]=data->wps_progress_pat[7];
    }
}
#endif

/* set volume */
void setvol(void)
{
    if (global_settings.volume < sound_min(SOUND_VOLUME))
        global_settings.volume = sound_min(SOUND_VOLUME);
    if (global_settings.volume > sound_max(SOUND_VOLUME))
        global_settings.volume = sound_max(SOUND_VOLUME);
    sound_set_volume(global_settings.volume);
    settings_save();
}
/* return true if screen restore is needed
   return false otherwise
*/
bool update_onvol_change(struct gui_wps * gwps)
{
    gui_wps_statusbar_draw(gwps, false);
    gui_wps_refresh(gwps, 0, WPS_REFRESH_NON_STATIC);

#ifdef HAVE_LCD_CHARCELLS
    gui_splash(gwps->display,0, false, "Vol: %d %%   ",
                   sound_val2phys(SOUND_VOLUME, global_settings.volume));
    return true;
#endif
    return false;
}

bool ffwd_rew(int button)
{
    static const int ff_rew_steps[] = {
      1000, 2000, 3000, 4000,
      5000, 6000, 8000, 10000,
      15000, 20000, 25000, 30000,
      45000, 60000
    };

    unsigned int step = 0;     /* current ff/rewind step */ 
    unsigned int max_step = 0; /* maximum ff/rewind step */ 
    int ff_rewind_count = 0;   /* current ff/rewind count (in ticks) */
    int direction = -1;         /* forward=1 or backward=-1 */
    long accel_tick = 0;       /* next time at which to bump the step size */
    bool exit = false;
    bool usb = false;
    int i = 0;

    while (!exit)
    {
        switch ( button )
        {
            case WPS_FFWD:
#ifdef WPS_RC_FFWD 
            case WPS_RC_FFWD:
#endif
                 direction = 1;
            case WPS_REW:
#ifdef WPS_RC_REW
            case WPS_RC_REW:
#endif
                if (wps_state.ff_rewind)
                {
                    if (direction == 1)
                    {
                        /* fast forwarding, calc max step relative to end */
                        max_step = (wps_state.id3->length - 
                                    (wps_state.id3->elapsed +
                                     ff_rewind_count)) *
                                     FF_REWIND_MAX_PERCENT / 100;
                    }
                    else
                    {
                        /* rewinding, calc max step relative to start */
                        max_step = (wps_state.id3->elapsed + ff_rewind_count) *
                                    FF_REWIND_MAX_PERCENT / 100;
                    }

                    max_step = MAX(max_step, MIN_FF_REWIND_STEP);

                    if (step > max_step)
                        step = max_step;

                    ff_rewind_count += step * direction;

                    if (global_settings.ff_rewind_accel != 0 && 
                        current_tick >= accel_tick)
                    { 
                        step *= 2;
                        accel_tick = current_tick +
                            global_settings.ff_rewind_accel*HZ; 
                    } 
                }
                else
                {
                    if ( (audio_status() & AUDIO_STATUS_PLAY) &&
                          wps_state.id3 && wps_state.id3->length )
                    {
                        if (!wps_state.paused)
                            audio_pause();
#if CONFIG_KEYPAD == PLAYER_PAD
                        FOR_NB_SCREENS(i)
                            gui_wps[i].display->stop_scroll();
#endif
                        if (direction > 0) 
                            status_set_ffmode(STATUS_FASTFORWARD);
                        else
                            status_set_ffmode(STATUS_FASTBACKWARD);

                        wps_state.ff_rewind = true;

                        step = ff_rew_steps[global_settings.ff_rewind_min_step];

                        accel_tick = current_tick +
                            global_settings.ff_rewind_accel*HZ; 
                    }
                    else
                        break;
                }

                if (direction > 0) {
                    if ((wps_state.id3->elapsed + ff_rewind_count) >
                        wps_state.id3->length)
                        ff_rewind_count = wps_state.id3->length -
                            wps_state.id3->elapsed;
                }
                else {
                    if ((int)(wps_state.id3->elapsed + ff_rewind_count) < 0)
                        ff_rewind_count = -wps_state.id3->elapsed;
                }

                FOR_NB_SCREENS(i)
                    gui_wps_refresh(&gui_wps[i],
                                    (wps_state.wps_time_countup == false)?
                                    ff_rewind_count:-ff_rewind_count,
                                    WPS_REFRESH_PLAYER_PROGRESS |
                                    WPS_REFRESH_DYNAMIC);

                break;

            case WPS_PREV:
            case WPS_NEXT: 
#ifdef WPS_RC_PREV
            case WPS_RC_PREV:
            case WPS_RC_NEXT:
#endif
                audio_ff_rewind(wps_state.id3->elapsed+ff_rewind_count);
                ff_rewind_count = 0;
                wps_state.ff_rewind = false;
                status_set_ffmode(0);
                if (!wps_state.paused)
                    audio_resume();
#ifdef HAVE_LCD_CHARCELLS
                gui_wps_display();
#endif
                exit = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED) {
                    status_set_ffmode(0);
                    usb = true;
                    exit = true;
                }
                break;
        }
        if (!exit)
            button = button_get(true);
    }

    /* let audio thread update id3->elapsed before calling wps_refresh */
    yield(); 
    FOR_NB_SCREENS(i)
        gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_ALL);
    return usb;
}

bool gui_wps_display(void)
{
    int i;
    if (!wps_state.id3 && !(audio_status() & AUDIO_STATUS_PLAY))
    {
        global_settings.resume_index = -1;
#ifdef HAVE_LCD_CHARCELLS
        gui_syncsplash(HZ, true, str(LANG_END_PLAYLIST_PLAYER));
#else
        gui_syncstatusbar_draw(&statusbars, true);
        gui_syncsplash(HZ, true, str(LANG_END_PLAYLIST_RECORDER));
#endif
        return true;
    }
    else
    {
        FOR_NB_SCREENS(i)
        {
            gui_wps[i].display->clear_display();
            if (!gui_wps[i].data->wps_loaded) {
                if ( !gui_wps[i].data->format_buffer[0] ) {
                    /* set the default wps for the main-screen */
                    if(i == 0)
                    {
#ifdef HAVE_LCD_BITMAP
                        wps_data_load(gui_wps[i].data,
                                      "%s%?it<%?in<%in. |>%it|%fn>\n"
                                      "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
                                      "%s%?id<%id|%?d1<%d1|(root)>> %?iy<(%iy)|>\n"
                                      "\n"
                                      "%al%pc/%pt%ar[%pp:%pe]\n"
                                      "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
                                      "%pb\n"
                                      "%pm\n", false, false);
#else
                        wps_data_load(gui_wps[i].data,
                                      "%s%pp/%pe: %?it<%it|%fn> - %?ia<%ia|%d2> - %?id<%id|%d1>\n"
                                      "%pc%?ps<*|/>%pt\n", false, false);
#endif
                    }
#if NB_SCREENS == 2
                     /* set the default wps for the remote-screen */
                     else if(i == 1)
                     {
                         wps_data_load(gui_wps[i].data,
                                       "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
                                       "%s%?it<%?in<%in. |>%it|%fn>\n"
                                       "%al%pc/%pt%ar[%pp:%pe]\n"
                                       "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
                                       "%pb", false, false);
                     }
#endif
                }
            }
        }
    }
    yield();
    FOR_NB_SCREENS(i)
    {
        gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_ALL);

#ifdef HAVE_LCD_BITMAP
        wps_display_images(&gui_wps[i]);
        gui_wps[i].display->update();
#endif
    }
    return false;
}

bool update(struct gui_wps *gwps)
{
    bool track_changed = audio_has_changed_track();
    bool retcode = false;

    gwps->state->nid3 = audio_next_track();
    if (track_changed)
    {
        gwps->display->stop_scroll();
        gwps->state->id3 = audio_current_track();
        if (gui_wps_display())
            retcode = true;
        else{
            gui_wps_refresh(gwps, 0, WPS_REFRESH_ALL);
        }

        if (gwps->state->id3)
            memcpy(gwps->state->current_track_path, gwps->state->id3->path,
                   sizeof(gwps->state->current_track_path));
    }

    if (gwps->state->id3)
        gui_wps_refresh(gwps, 0, WPS_REFRESH_NON_STATIC);

    gui_wps_statusbar_draw(gwps, false);
    
    return retcode;           
}

#ifdef WPS_KEYLOCK
void display_keylock_text(bool locked)
{
    char* s;
    int i;
    FOR_NB_SCREENS(i)
        gui_wps[i].display->stop_scroll();

#ifdef HAVE_LCD_CHARCELLS
    if(locked)
        s = str(LANG_KEYLOCK_ON_PLAYER);
    else
        s = str(LANG_KEYLOCK_OFF_PLAYER);
#else
    if(locked)
        s = str(LANG_KEYLOCK_ON_RECORDER);
    else
        s = str(LANG_KEYLOCK_OFF_RECORDER);
#endif
    gui_syncsplash(HZ, true, s);
}

void waitfor_nokey(void)
{
    /* wait until all keys are released */
    while (button_get(false) != BUTTON_NONE)
        yield();
}
#endif

