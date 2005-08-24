/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Hardeep Sidhu
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>
#include <sprintf.h>
#include "playlist.h"
#include "audio.h"
#include "screens.h"
#include "status.h"
#include "settings.h"
#include "icons.h"
#include "menu.h"
#include "plugin.h"
#include "keyboard.h"
#include "tree.h"
#include "onplay.h"
#include "talk.h"
#include "misc.h"
#include "action.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#endif

#include "lang.h"

#include "playlist_viewer.h"

/* Defines for LCD display purposes.  Taken from tree.c */
#ifdef HAVE_LCD_BITMAP
    #define CURSOR_X        (global_settings.scrollbar && \
                             viewer.num_tracks>viewer.num_display_lines?1:0)
    #define CURSOR_Y        0 
    #define CURSOR_WIDTH    (global_settings.invert_cursor ? 0 : 4)

    #define ICON_WIDTH      ((viewer.char_width > 6) ? viewer.char_width : 6)

    #define MARGIN_X        ((global_settings.scrollbar && \
                             viewer.num_tracks > viewer.num_display_lines ? \
                             SCROLLBAR_WIDTH : 0) + CURSOR_WIDTH + \
                             (global_settings.playlist_viewer_icons ? \
                                ICON_WIDTH : 0))
    #define MARGIN_Y        (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)

    #define LINE_X          0
    #define LINE_Y          (global_settings.statusbar ? 1 : 0)

    #define SCROLLBAR_X     0
    #define SCROLLBAR_Y     lcd_getymargin()
    #define SCROLLBAR_WIDTH 6
#else
    #define MARGIN_X        0
    #define MARGIN_Y        0
    #define LINE_X          2
    #define LINE_Y          0
    #define CURSOR_X        0
    #define CURSOR_Y        0
#endif

/* Maximum number of tracks we can have loaded at one time */
#define MAX_PLAYLIST_ENTRIES 200

/* Default playlist name for saving */
#define DEFAULT_VIEWER_PLAYLIST_NAME "/viewer.m3u"

/* Index of track on display line _pos */
#define INDEX(_pos) (viewer.first_display_index - viewer.first_index + (_pos))

/* Global playlist viewer settings */
struct playlist_viewer_info {
    struct playlist_info* playlist; /* playlist being viewed                */
    char *name_buffer;          /* Buffer used to store track names         */
    int buffer_size;            /* Size of name buffer                      */

    int num_display_lines;      /* Number of lines on lcd                   */
    int line_height;            /* Height (in pixels) of display line       */
    int char_width;             /* Width (in pixels) of a character         */

    int num_tracks;             /* Number of tracks in playlist             */
    int current_playing_track;  /* Index of current playing track           */

    int num_loaded;             /* Number of track entries loaded in viewer */
    int first_index;            /* Index of first loaded track              */
    int last_index;             /* Index of last loaded track               */
    int first_display_index;    /* Index of first track on display          */
    int last_display_index;     /* Index of last track on display           */
    int cursor_pos;             /* Line number of cursor                    */

    int move_track;             /* Playlist index of track to move or -1    */
};

/* Information about a specific track */
struct playlist_entry {
    char *name;                 /* Formatted track name                     */
    int index;                  /* Playlist index                           */
    int display_index;          /* Display index                            */
    bool queued;                /* Is track queued?                         */
};

static struct playlist_viewer_info  viewer;
static struct playlist_entry        tracks[MAX_PLAYLIST_ENTRIES];

/* Used when viewing playlists on disk */
static struct playlist_info         temp_playlist;

static bool initialize(char* filename, bool reload);
static void load_playlist_entries(int start_index);
static void load_playlist_entries_r(int end_index);
static int  load_entry(int index, int pos, char* p, int size);
static void format_name(char* dest, const char* src);
static void format_line(const struct playlist_entry* track, char* str, int len);
static void display_playlist(void);
static void update_display_line(int line, bool scroll);
static void scroll_display(int lines);
static void update_first_index(void);
static bool update_playlist(bool force);
static int  onplay_menu(int index);
static bool viewer_menu(void);
static bool show_icons(void);
static bool show_indices(void);
static bool track_display(void);
static bool save_playlist(void);

/* Initialize the playlist viewer. */
static bool initialize(char* filename, bool reload)
{
    char* buffer;
    int buffer_size;
    bool is_playing = audio_status() & AUDIO_STATUS_PLAY;

    if (!filename && !is_playing)
        /* Nothing is playing, exit */
        return false;

    buffer = plugin_get_buffer(&buffer_size);
    if (!buffer)
        return false;

    if (!filename)
        viewer.playlist = NULL;
    else
    {
        /* Viewing playlist on disk */
        char *dir, *file, *temp_ptr;
        char *index_buffer = NULL;
        int  index_buffer_size = 0;
        
        viewer.playlist = &temp_playlist;

        /* Separate directory from filename */
        temp_ptr = strrchr(filename+1,'/');
        if (temp_ptr)
        {
            *temp_ptr = 0;
            dir = filename;
            file = temp_ptr + 1;
        }
        else
        {
            dir = "/";
            file = filename+1;
        }

        if (is_playing)
        {
            /* Something is playing, use half the plugin buffer for playlist
               indices */
            index_buffer_size = buffer_size / 2;
            index_buffer = buffer;
        }

        playlist_create_ex(viewer.playlist, dir, file, index_buffer,
            index_buffer_size, buffer+index_buffer_size,
            buffer_size-index_buffer_size);

        if (temp_ptr)
            *temp_ptr = '/';

        buffer += index_buffer_size;
        buffer_size -= index_buffer_size;
    }

    viewer.name_buffer = buffer;
    viewer.buffer_size = buffer_size;

#ifdef HAVE_LCD_BITMAP
    {
        char icon_chars[] = "MQ"; /* characters used as icons */
        unsigned int i;

        viewer.char_width = 0;
        viewer.line_height = 0;

        /* Use icon characters to calculate largest possible width/height so
           that we set proper margins */ 
        for (i=0; i<sizeof(icon_chars); i++)
        {
            char str[2];
            int w, h;

            snprintf(str, sizeof(str), "%c", icon_chars[i]);
            lcd_getstringsize(str, &w, &h);

            if (w > viewer.char_width)
                viewer.char_width = w;

            if (h > viewer.line_height)
            {
                viewer.line_height = h;
                viewer.num_display_lines = (LCD_HEIGHT - MARGIN_Y)/h;
            }
        }
    }
#else
    viewer.num_display_lines = 2;
    viewer.char_width = 1;
    viewer.line_height = 1;
#endif

    viewer.move_track = -1;

    if (!reload)
    {
        viewer.cursor_pos = 0;

        if (!viewer.playlist)
            /* Start displaying at current playing track */
            viewer.first_display_index = playlist_get_display_index() - 1;
        else
            viewer.first_display_index = 0;

        update_first_index();
    }

    if (!update_playlist(true))
        return false;

    return true;
}

/* Load tracks starting at start_index */
static void load_playlist_entries(int start_index)
{
    int num_entries = viewer.num_tracks - start_index;
    char* p = viewer.name_buffer;
    int remaining = viewer.buffer_size;
    int i;

    viewer.first_index = start_index;

    if (num_entries > MAX_PLAYLIST_ENTRIES)
        num_entries = MAX_PLAYLIST_ENTRIES;

    for(i=0; i<num_entries; i++, start_index++)
    {
        int len = load_entry(start_index, i, p, remaining);
        if (len < 0)
        {
            /* Out of name buffer space */
            num_entries = i;
            break;
        }

        p += len;
        remaining -= len;
    }

    viewer.num_loaded = num_entries;
    viewer.last_index = viewer.first_index + (viewer.num_loaded - 1);
}

/* Load tracks in reverse, ending at end_index */
static void load_playlist_entries_r(int end_index)
{
    int num_entries = end_index;
    char* p = viewer.name_buffer;
    int remaining = viewer.buffer_size;
    int i;

    viewer.last_index = end_index;

    if (num_entries >= MAX_PLAYLIST_ENTRIES)
        num_entries = MAX_PLAYLIST_ENTRIES-1;

    for(i=num_entries; i>=0; i--, end_index--)
    {
        int len = load_entry(end_index, i, p, remaining);
        if (len < 0)
        {
            int j;

            /* Out of name buffer space */
            num_entries -= i;

            /* Shift loaded tracks up such that first track is index 0 */
            for (j=0; j<num_entries; j++, i++)
            {
                tracks[j].name = tracks[i].name;
                tracks[j].index = tracks[i].index;
                tracks[j].display_index = tracks[i].display_index;
                tracks[j].queued = tracks[i].queued;
            }

            break;
        }

        p += len;
        remaining -= len;
    }

    viewer.first_index = viewer.last_index - num_entries;

    num_entries++;
    if (!viewer.first_index &&
         num_entries < viewer.num_tracks &&
         num_entries < MAX_PLAYLIST_ENTRIES)
    {
        /* Lets see if we can load more data at the end of the list */
        int max = viewer.num_tracks;
        if (max > MAX_PLAYLIST_ENTRIES)
            max = MAX_PLAYLIST_ENTRIES;

        for (i = num_entries; i<max; i++)
        {
            int len = load_entry(num_entries, num_entries, p, remaining);
            if (len < 0)
                /* Out of name buffer space */
                break;
            
            p += len;
            remaining -= len;

            num_entries++;
            viewer.last_index++;
        }
    }

    viewer.num_loaded = num_entries;
}

/* Load track at playlist index.  pos is the position in the tracks array and
   p is a pointer to the name buffer (max size),  Returns -1 if buffer is
   full. */
static int load_entry(int index, int pos, char* p, int size)
{
    struct playlist_track_info info;
    int len;
    int result = 0;

    /* Playlist viewer orders songs based on display index.  We need to
       convert to real playlist index to access track */
    index = (index + playlist_get_first_index(viewer.playlist)) %
        viewer.num_tracks;
    if (playlist_get_track_info(viewer.playlist, index, &info) < 0)
        return -1;
    
    len = strlen(info.filename) + 1;
    
    if (len <= size)
    {
        strcpy(p, info.filename);
        
        tracks[pos].name = p;
        tracks[pos].index = info.index;
        tracks[pos].display_index = info.display_index;
        tracks[pos].queued = info.attr & PLAYLIST_ATTR_QUEUED;
        
        result = len;
    }
    else
        result = -1;

    return result;
}

/* Format trackname for display purposes */
static void format_name(char* dest, const char* src)
{
    switch (global_settings.playlist_viewer_track_display)
    {
        case 0:
        default:
        {
            /* Only display the mp3 filename */
            char* p = strrchr(src, '/');
            int len;
            
            strcpy(dest, p+1);
            len = strlen(dest);
            
            /* Remove the extension */
            if (!strcasecmp(&dest[len-4], ".mp3") ||
                !strcasecmp(&dest[len-4], ".mp2") ||
                !strcasecmp(&dest[len-4], ".mpa"))
                dest[len-4] = '\0';

            break;
        }
        case 1:
            /* Full path */
            strcpy(dest, src);
            break;
    }
}

/* Format display line */
static void format_line(const struct playlist_entry* track, char* str, int len)
{
    char name[MAX_PATH];

    format_name(name, track->name);

    if (global_settings.playlist_viewer_indices)
        /* Display playlist index */
        snprintf(str, len, "%d. %s", track->display_index, name);
    else
        snprintf(str, len, "%s", name);

}

/* Display tracks on screen */
static void display_playlist(void)
{
    int i;
    int num_display_tracks =
        viewer.last_display_index - viewer.first_display_index;

    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(MARGIN_X, MARGIN_Y);
    lcd_setfont(FONT_UI);
#endif

    for (i=0; i<=num_display_tracks; i++)
    {
        if (global_settings.playlist_viewer_icons)
        {
            /* Icons */
            if (tracks[INDEX(i)].index == viewer.current_playing_track)
            {
                /* Current playing track */
#ifdef HAVE_LCD_BITMAP
                int offset=0;
                if ( viewer.line_height > 8 )
                    offset = (viewer.line_height - 8) / 2;
                lcd_mono_bitmap(bitmap_icons_6x8[Icon_Audio],
                    CURSOR_X * 6 + CURSOR_WIDTH,
                    MARGIN_Y+(i*viewer.line_height) + offset, 6, 8);
#else
                lcd_putc(LINE_X-1, i, Icon_Audio);
#endif
            }
            else if (tracks[INDEX(i)].index == viewer.move_track)
            {
                /* Track we are moving */
#ifdef HAVE_LCD_BITMAP
                lcd_putsxy(CURSOR_X * 6 + CURSOR_WIDTH,
                    MARGIN_Y+(i*viewer.line_height), "M");
#else
                lcd_putc(LINE_X-1, i, 'M');
#endif
            }
            else if (tracks[INDEX(i)].queued)
            {
                /* Queued track */
#ifdef HAVE_LCD_BITMAP
                lcd_putsxy(CURSOR_X * 6 + CURSOR_WIDTH,
                    MARGIN_Y+(i*viewer.line_height), "Q");
#else
                lcd_putc(LINE_X-1, i, 'Q');
#endif
            }
        }

        update_display_line(i, false);
    }

#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar &&
        (viewer.num_tracks > viewer.num_display_lines))
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  LCD_HEIGHT - SCROLLBAR_Y, viewer.num_tracks-1,
                  viewer.first_display_index, viewer.last_display_index,
                  VERTICAL);
#endif

    put_cursorxy(CURSOR_X, CURSOR_Y + viewer.cursor_pos, true);
    status_draw(true);
}

/* Scroll cursor or display by num lines */
static void scroll_display(int lines)
{
    int new_index = viewer.first_display_index + viewer.cursor_pos + lines;
    bool pagescroll = false;
    bool wrap = false;

    put_cursorxy(CURSOR_X, CURSOR_Y + viewer.cursor_pos, false);

    if (lines > 1 || lines < -1)
        pagescroll = true;

    if (new_index < 0)
    {
        /* Wrap around if not pageup */
        if (pagescroll)
            new_index = 0;
        else
        {
            new_index += viewer.num_tracks;
            viewer.cursor_pos = viewer.num_display_lines-1;
            wrap = true;
        }
    }
    else if (new_index >= viewer.num_tracks)
    {
        /* Wrap around if not pagedown */
        if (pagescroll)
            new_index = viewer.num_tracks - 1;
        else
        {
            new_index -= viewer.num_tracks;
            viewer.cursor_pos = 0;
            wrap = true;
        }
    }

    if (new_index >= viewer.first_display_index &&
        new_index <= viewer.last_display_index)
    {
        /* Just update the cursor */
        viewer.cursor_pos = new_index - viewer.first_display_index;
    }
    else
    {
        /* New track is outside of display */
        if (wrap)
            viewer.first_display_index = new_index;
        else
            viewer.first_display_index = viewer.first_display_index + lines;

        if (viewer.first_display_index < 0)
            viewer.first_display_index = 0;

        viewer.last_display_index =
            viewer.first_display_index + (viewer.num_display_lines - 1);
        if (viewer.last_display_index >= viewer.num_tracks)
        {
            /* display as many tracks as possible on screen */
            if (viewer.first_display_index > 0)
            {
                viewer.first_display_index -=
                    (viewer.last_display_index - viewer.num_tracks + 1);
                if (viewer.first_display_index < 0)
                    viewer.first_display_index = 0;
            }

            viewer.last_display_index = viewer.num_tracks - 1;
        }

        if (viewer.cursor_pos >
                (viewer.last_display_index - viewer.first_display_index))
            viewer.cursor_pos =
                viewer.last_display_index - viewer.first_display_index;

        /* Load more data if needed */
        if (viewer.first_display_index < viewer.first_index)
            load_playlist_entries_r(viewer.last_display_index);
        else if (viewer.last_display_index > viewer.last_index)
            load_playlist_entries(viewer.first_display_index);

        display_playlist();
    }

    put_cursorxy(CURSOR_X, CURSOR_Y + viewer.cursor_pos, true);
}

/* Update lcd line.  Scroll line if requested */
static void update_display_line(int line, bool scroll)
{
    char str[MAX_PATH + 16];
    
    format_line(&tracks[INDEX(line)], str, sizeof(str));

    if (scroll)
    {
#ifdef HAVE_LCD_BITMAP
        if (global_settings.invert_cursor)
            lcd_puts_scroll_style(LINE_X, line, str, STYLE_INVERT);
        else
#endif
            lcd_puts_scroll(LINE_X, line, str);
    }
    else
        lcd_puts(LINE_X, line, str);
}

/* Update first index, if necessary, to put as much as possible on the
   screen */
static void update_first_index(void)
{
    /* viewer.num_tracks may be invalid at this point */
    int num_tracks = playlist_amount_ex(viewer.playlist);

    if ((num_tracks - viewer.first_display_index) < viewer.num_display_lines)
    {
        /* Try to display as much as possible */
        int old_index = viewer.first_display_index;

        viewer.first_display_index = num_tracks - viewer.num_display_lines;
        if (viewer.first_display_index < 0)
            viewer.first_display_index = 0;

        /* Cursor should still point at current track */
        viewer.cursor_pos += old_index - viewer.first_display_index;
    }
}

/* Update playlist in case something has changed or forced */
static bool update_playlist(bool force)
{
    if (!viewer.playlist)
        playlist_get_resume_info(&viewer.current_playing_track);
    else
        viewer.current_playing_track = -1;
        
    if (force || playlist_amount_ex(viewer.playlist) != viewer.num_tracks)
    {
        int index;

        /* Reload tracks */
        viewer.num_tracks = playlist_amount_ex(viewer.playlist);
        if (viewer.num_tracks < 0)
            return false;
        
        index = viewer.first_display_index;

        load_playlist_entries(index);

        if (viewer.num_loaded <= 0)
            return false;
        
        viewer.first_display_index = viewer.first_index;
        viewer.last_display_index =
            viewer.first_index + viewer.num_display_lines - 1;
        if (viewer.last_display_index >= viewer.num_tracks)
            viewer.last_display_index = viewer.num_tracks - 1;
    }

    display_playlist();

    return true;
}

/* Menu of playlist commands.  Invoked via ON+PLAY on main viewer screen.
   Returns -1 if USB attached, 0 if no playlist change, and 1 if playlist
   changed. */
static int onplay_menu(int index)
{
    struct menu_item items[3]; /* increase this if you add entries! */
    int m, i=0, result, ret = 0;
    bool current = (tracks[index].index == viewer.current_playing_track);

    items[i].desc = ID2P(LANG_REMOVE);
    i++;

    items[i].desc = ID2P(LANG_MOVE);
    i++;

    items[i].desc = ID2P(LANG_FILE_OPTIONS);
    i++;

    m = menu_init(items, i, NULL, NULL, NULL, NULL);
    result = menu_show(m);
    if (result == MENU_ATTACHED_USB)
        ret = -1;
    else if (result >= 0)
    {
        /* Abort current move */
        viewer.move_track = -1;

        switch (result)
        {
            case 0:
                /* delete track */
                playlist_delete(viewer.playlist, tracks[index].index);

                if (current)
                {
                    /* Start playing new track except if it's the last track
                       in the playlist and repeat mode is disabled */
                    if (tracks[index].display_index != viewer.num_tracks ||
                        global_settings.repeat_mode == REPEAT_ALL)
                    {
                        talk_buffer_steal(); /* will use the mp3 buffer */
                        audio_play(0);
                        viewer.current_playing_track = -1;
                    }
                }

                ret = 1;
                break;
            case 1:
                /* move track */
                viewer.move_track = tracks[index].index;
                ret = 0;
                break;
            case 2:
            {
                onplay(tracks[index].name, TREE_ATTR_MPA, CONTEXT_TREE);

                if (!viewer.playlist)
                    ret = 1;
                else
                    ret = 0;

                break;
            }
        }
    }

    menu_exit(m);

    return ret;
}

/* Menu of viewer options.  Invoked via F1(r) or Menu(p). */
static bool viewer_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SHOW_ICONS),             show_icons },
        { ID2P(LANG_SHOW_INDICES),           show_indices },
        { ID2P(LANG_TRACK_DISPLAY),          track_display },
        { ID2P(LANG_SAVE_DYNAMIC_PLAYLIST),  save_playlist },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL );
    result = menu_run(m);
    menu_exit(m);

    settings_save();

    return result;
}

/* Show icons in viewer? */
static bool show_icons(void)
{
    return set_bool(str(LANG_SHOW_ICONS),
        &global_settings.playlist_viewer_icons);
}

/* Show indices of tracks? */
static bool show_indices(void)
{
    return set_bool(str(LANG_SHOW_INDICES),
        &global_settings.playlist_viewer_indices);
}

/* How to display a track */
static bool track_display(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_DISPLAY_TRACK_NAME_ONLY) },
        { STR(LANG_DISPLAY_FULL_PATH) }
    };
    
    return set_option(str(LANG_TRACK_DISPLAY), 
        &global_settings.playlist_viewer_track_display, INT, names, 2, NULL);
}

/* Save playlist to disk */
static bool save_playlist(void)
{
    char filename[MAX_PATH+1];

    strncpy(filename, DEFAULT_VIEWER_PLAYLIST_NAME, sizeof(filename));

    if (!kbd_input(filename, sizeof(filename)))
    {
        playlist_save(viewer.playlist, filename);
        
        /* reload in case playlist was saved to cwd */
        reload_directory();
    }

    return false;
}

/* View current playlist */
bool playlist_viewer(void)
{
    return playlist_viewer_ex(NULL);
}

/* Main viewer function.  Filename identifies playlist to be viewed.  If NULL,
   view current playlist. */
bool playlist_viewer_ex(char* filename)
{
    bool ret = false;       /* return value */
    bool exit=false;        /* exit viewer */
    bool update=true;       /* update display */
    bool cursor_on=true;    /* used for flashing cursor */
    int old_cursor_pos;     /* last cursor position */
    int button, lastbutton = BUTTON_NONE;

    if (!initialize(filename, false))
        goto exit;

    old_cursor_pos = viewer.cursor_pos;

    while (!exit)
    {
        int track;

        if (!viewer.playlist && !(audio_status() & AUDIO_STATUS_PLAY))
        {
            /* Play has stopped */
#ifdef HAVE_LCD_CHARCELLS
            splash(HZ, true, str(LANG_END_PLAYLIST_PLAYER));
#else
            splash(HZ, true, str(LANG_END_PLAYLIST_RECORDER));
#endif
            goto exit;
        }

        if (viewer.move_track != -1 || !cursor_on)
        {
            /* Flash cursor to identify that we are moving a track */
            cursor_on = !cursor_on;
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor)
            {
                lcd_set_drawmode(DRMODE_COMPLEMENT);
                lcd_fillrect(
                    MARGIN_X, MARGIN_Y+(viewer.cursor_pos*viewer.line_height),
                    LCD_WIDTH, viewer.line_height);
                lcd_set_drawmode(DRMODE_SOLID);
                lcd_invertscroll(LINE_X, viewer.cursor_pos);
            }
            else
                put_cursorxy(CURSOR_X, CURSOR_Y + viewer.cursor_pos,
                    cursor_on);

            lcd_update_rect(
                0, MARGIN_Y + (viewer.cursor_pos * viewer.line_height),
                LCD_WIDTH, viewer.line_height);
#else
            put_cursorxy(CURSOR_X, CURSOR_Y + viewer.cursor_pos, cursor_on);
            lcd_update();
#endif
        }

        if (!viewer.playlist)
            playlist_get_resume_info(&track);
        else
            track = -1;

        if (track != viewer.current_playing_track ||
            playlist_amount_ex(viewer.playlist) != viewer.num_tracks)
        {
            /* Playlist has changed (new track started?) */
            update_first_index();
            if (!update_playlist(false))
                goto exit;
            else
                update = true;

            /* Abort move on playlist change */
            viewer.move_track = -1;
        }

        /* Timeout so we can determine if play status has changed */
        button = button_get_w_tmo(HZ/2);

        switch (button)
        {
            case TREE_EXIT:
#ifdef TREE_OFF
           case TREE_OFF:
#endif
                exit = true;
                break;

            case TREE_PREV:
            case TREE_PREV | BUTTON_REPEAT:
                scroll_display(-1);
                update = true;
                break;

            case TREE_NEXT:
            case TREE_NEXT | BUTTON_REPEAT:
                scroll_display(1);
                update = true;
                break;

#ifdef TREE_PGUP
            case TREE_PGUP:
            case TREE_PGUP | BUTTON_REPEAT:
                /* Pageup */
                scroll_display(-viewer.num_display_lines);
                update = true;
                break;

            case TREE_PGDN:
            case TREE_PGDN | BUTTON_REPEAT:
                /* Pagedown */
                scroll_display(viewer.num_display_lines);
                update = true;
                break;
#endif

            case TREE_RUN:
#ifdef TREE_RUN_PRE
                if (lastbutton != TREE_RUN_PRE)
                    break;
#endif
                if (viewer.move_track >= 0)
                {
                    /* Move track */
                    int ret;

                    ret = playlist_move(viewer.playlist, viewer.move_track,
                        tracks[INDEX(viewer.cursor_pos)].index);
                    if (ret < 0)
                        splash(HZ, true, str(LANG_MOVE_FAILED));

                    update_playlist(true);
                    viewer.move_track = -1;
                }
                else if (!viewer.playlist)
                {
                    /* play new track */
                    playlist_start(tracks[INDEX(viewer.cursor_pos)].index, 0);
                    update_playlist(false);
                }
                else
                {
                    /* New playlist */
                    if (playlist_set_current(viewer.playlist) < 0)
                        goto exit;

                    playlist_start(tracks[INDEX(viewer.cursor_pos)].index, 0);

                    /* Our playlist is now the current list */
                    if (!initialize(NULL, true))
                        goto exit;
                }

                display_playlist();
                update = true;
                break;

            case TREE_CONTEXT:
#ifdef TREE_CONTEXT2
            case TREE_CONTEXT2:
#endif
            {
                /* ON+PLAY menu */
                int ret;

                ret = onplay_menu(INDEX(viewer.cursor_pos));

                if (ret < 0)
                {
                    ret = true;
                    goto exit;
                }
                else if (ret > 0)
                {
                    /* Playlist changed */
                    update_first_index();
                    update_playlist(false);
                    if (viewer.num_tracks <= 0)
                        exit = true;
                }
                else
                    display_playlist();

                update = true;
                break;
            }

            case TREE_MENU:
                if (viewer_menu())
                {
                    ret = true;
                    goto exit;
                }
               
                display_playlist();
                update = true;
                break;

            case BUTTON_NONE:
                status_draw(false);
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    ret = true;
                    goto exit;
                }
                break;
        }

        if (update && !exit)
        {
            lcd_stop_scroll();

            if (viewer.cursor_pos >
                (viewer.last_display_index - viewer.first_display_index))
            {
                /* Cursor position is invalid */
                put_cursorxy(CURSOR_X, CURSOR_Y + viewer.cursor_pos, false);
                viewer.cursor_pos =
                    viewer.last_display_index - viewer.first_display_index;
                put_cursorxy(CURSOR_X, CURSOR_Y + viewer.cursor_pos, true);
            }

            if (viewer.cursor_pos != old_cursor_pos &&
                old_cursor_pos <=
                    (viewer.last_display_index - viewer.first_display_index))
                /* Stop scrolling previous line */
                update_display_line(old_cursor_pos, false);

            /* Scroll line at new cursor position */
            update_display_line(viewer.cursor_pos, true);

            lcd_update();

            old_cursor_pos = viewer.cursor_pos;
            cursor_on = true;
            update = false;
        }
        lastbutton = button;
    }

exit:
    if (viewer.playlist)
        playlist_close(viewer.playlist);
    return ret;
}
