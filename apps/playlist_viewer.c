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
/*
 * Kevin Ferrare 2005/10/16
 * multi-screen support, rewrote a lot of code
 */
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
#include "debug.h"

#include "lang.h"

#include "playlist_viewer.h"
#include "icon.h"
#include "list.h"
#include "statusbar.h"
#include "splash.h"
#include "playlist_menu.h"

/* Maximum number of tracks we can have loaded at one time */
#define MAX_PLAYLIST_ENTRIES 200

/* The number of items between the selected one and the end/start of
 * the buffer under which the buffer must reload */
#define MIN_BUFFER_MARGIN screens[0].nb_lines

/* Default playlist name for saving */
#define DEFAULT_VIEWER_PLAYLIST_NAME "/viewer.m3u"


/* Information about a specific track */
struct playlist_entry {
    char *name;                 /* Formatted track name                     */
    int index;                  /* Playlist index                           */
    int display_index;          /* Display index                            */
    bool queued;                /* Is track queued?                         */
    bool skipped;               /* Is track marked as bad?                  */
};

enum direction
{
    FORWARD,
    BACKWARD
};

struct playlist_buffer
{
    char *name_buffer;        /* Buffer used to store track names */
    int buffer_size;          /* Size of name buffer */

    int first_index;          /* Real index of first track loaded inside
                                 the buffer */

    enum direction direction; /* Direction of the buffer (if the buffer
                                 was loaded BACKWARD, the last track in
                                 the buffer has a real index < to the
                                 real index of the the first track)*/

    struct playlist_entry tracks[MAX_PLAYLIST_ENTRIES];
    int num_loaded;           /* Number of track entries loaded in buffer */
};

/* Global playlist viewer settings */
struct playlist_viewer {
    struct playlist_info* playlist; /* playlist being viewed                */
    int num_tracks;             /* Number of tracks in playlist             */
    int current_playing_track;  /* Index of current playing track           */
    int selected_track;         /* The selected track, relative (first is 0)*/
    int move_track;             /* Playlist index of track to move or -1    */
    struct playlist_buffer buffer;
};

static struct playlist_viewer  viewer;

/* Used when viewing playlists on disk */
static struct playlist_info temp_playlist;

void playlist_buffer_init(struct playlist_buffer * pb, char * names_buffer,
                          int names_buffer_size);
void playlist_buffer_load_entries(struct playlist_buffer * pb, int index,
                                  enum direction direction);
int playlist_entry_load(struct playlist_entry *entry, int index,
                        char* name_buffer, int remaining_size);

struct playlist_entry * playlist_buffer_get_track(struct playlist_buffer * pb,
                                                  int index);

static bool playlist_viewer_init(struct playlist_viewer * viewer,
                                 char* filename, bool reload);

static void format_name(char* dest, const char* src);
static void format_line(const struct playlist_entry* track, char* str,
                        int len);

static bool update_playlist(bool force);
static int  onplay_menu(int index);
static bool viewer_menu(void);
static bool show_icons(void);
static bool show_indices(void);
static bool track_display(void);
static bool save_playlist(void);

void playlist_buffer_init(struct playlist_buffer * pb, char * names_buffer,
                          int names_buffer_size)
{
    pb->name_buffer=names_buffer;
    pb->buffer_size=names_buffer_size;
    pb->first_index=0;
    pb->num_loaded=0;
}

/*
 * Loads the entries following 'index' in the playlist buffer
 */
void playlist_buffer_load_entries(struct playlist_buffer * pb, int index,
                                  enum direction direction)
{
    int num_entries = viewer.num_tracks;
    char* p = pb->name_buffer;
    int remaining = pb->buffer_size;
    int i;

    pb->first_index = index;
    if (num_entries > MAX_PLAYLIST_ENTRIES)
        num_entries = MAX_PLAYLIST_ENTRIES;

    for(i=0; i<num_entries; i++)
    {
        int len = playlist_entry_load(&(pb->tracks[i]), index, p, remaining);
        if (len < 0)
        {
            /* Out of name buffer space */
            num_entries = i;
            break;
        }

        p += len;
        remaining -= len;

        if(direction==FORWARD)
            index++;
        else
            index--;
        index+=viewer.num_tracks;
        index%=viewer.num_tracks;
    }
    pb->direction=direction;
    pb->num_loaded = i;
}

void playlist_buffer_load_entries_screen(struct playlist_buffer * pb,
                                         enum direction direction)
{
    if(direction==FORWARD)
    {
        int min_start=viewer.selected_track-2*screens[0].nb_lines;
        while(min_start<0)
            min_start+=viewer.num_tracks;
        min_start %= viewer.num_tracks;
        playlist_buffer_load_entries(pb, min_start, FORWARD);
    }
     else
    {
        int max_start=viewer.selected_track+2*screens[0].nb_lines;
        max_start%=viewer.num_tracks;
        playlist_buffer_load_entries(pb, max_start, BACKWARD);
    }
}

int playlist_entry_load(struct playlist_entry *entry, int index,
                        char* name_buffer, int remaining_size)
{
    struct playlist_track_info info;
    int len;

    /* Playlist viewer orders songs based on display index.  We need to
       convert to real playlist index to access track */
    index = (index + playlist_get_first_index(viewer.playlist)) %
        viewer.num_tracks;
    if (playlist_get_track_info(viewer.playlist, index, &info) < 0)
        return -1;

    len = strlen(info.filename) + 1;

    if (len <= remaining_size)
    {
        strcpy(name_buffer, info.filename);

        entry->name = name_buffer;
        entry->index = info.index;
        entry->display_index = info.display_index;
        entry->queued = info.attr & PLAYLIST_ATTR_QUEUED;
        entry->skipped = info.attr & PLAYLIST_ATTR_SKIPPED;
        return len;
    }
    return -1;
}

int playlist_buffer_get_index(struct playlist_buffer * pb, int index )
{
    int buffer_index;
    if(pb->direction==FORWARD)
    {
        if(index>=pb->first_index)
            buffer_index=index-pb->first_index;
        else /* rotation : track0 in buffer + requested track */
            buffer_index=(viewer.num_tracks-pb->first_index)+(index);
    }
    else
    {
        if(index<=pb->first_index)
            buffer_index=pb->first_index-index;
        else /* rotation : track0 in buffer + dist from the last track
                to the requested track (num_tracks-requested track) */
            buffer_index=(pb->first_index)+(viewer.num_tracks-index);
    }
    return(buffer_index);
}

#define distance(a, b) \
    a>b? (a) - (b) : (b) - (a)
bool playlist_buffer_needs_reload(struct playlist_buffer* pb, int track_index)
{
    if(pb->num_loaded==viewer.num_tracks)
        return(false);
    int selected_index=playlist_buffer_get_index(pb, track_index);
    int first_buffer_index=playlist_buffer_get_index(pb, pb->first_index);
    int distance_beginning=distance(selected_index, first_buffer_index);
    if(distance_beginning<MIN_BUFFER_MARGIN)
        return(true);

    if(pb->num_loaded - distance_beginning < MIN_BUFFER_MARGIN)
       return(true);
    return(false);
}

struct playlist_entry * playlist_buffer_get_track(struct playlist_buffer * pb,
                                                  int index)
{
    int buffer_index=playlist_buffer_get_index(pb, index);
    return(&(pb->tracks[buffer_index]));
}

/* Initialize the playlist viewer. */
static bool playlist_viewer_init(struct playlist_viewer * viewer,
                                 char* filename, bool reload)
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
        viewer->playlist = NULL;
    else
    {
        /* Viewing playlist on disk */
        char *dir, *file, *temp_ptr;
        char *index_buffer = NULL;
        int  index_buffer_size = 0;

        viewer->playlist = &temp_playlist;

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

        playlist_create_ex(viewer->playlist, dir, file, index_buffer,
            index_buffer_size, buffer+index_buffer_size,
            buffer_size-index_buffer_size);

        if (temp_ptr)
            *temp_ptr = '/';

        buffer += index_buffer_size;
        buffer_size -= index_buffer_size;
    }
    playlist_buffer_init(&viewer->buffer, buffer, buffer_size );

    viewer->move_track = -1;

    if (!reload)
    {
        if (viewer->playlist)
            viewer->selected_track = 0;
        else
            viewer->selected_track = playlist_get_display_index() - 1;
    }

    if (!update_playlist(true))
        return false;
    return true;
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
static void format_line(const struct playlist_entry* track, char* str,
                        int len)
{
    char name[MAX_PATH];
    char *skipped = "";

    format_name(name, track->name);

    if (track->skipped)
        skipped = "(ERR) ";
    
    if (global_settings.playlist_viewer_indices)
        /* Display playlist index */
        snprintf(str, len, "%d. %s%s", track->display_index, skipped, name);
    else
        snprintf(str, len, "%s%s", skipped, name);

}

/* Update playlist in case something has changed or forced */
static bool update_playlist(bool force)
{
    if (!viewer.playlist)
        playlist_get_resume_info(&viewer.current_playing_track);
    else
        viewer.current_playing_track = -1;
    int nb_tracks=playlist_amount_ex(viewer.playlist);
    force=force || nb_tracks != viewer.num_tracks;
    if (force)
    {
        /* Reload tracks */
        viewer.num_tracks = nb_tracks;
        if (viewer.num_tracks < 0)
            return false;
        playlist_buffer_load_entries_screen(&viewer.buffer, FORWARD);
        if (viewer.buffer.num_loaded <= 0)
            return false;
    }
    return true;
}

/* Menu of playlist commands.  Invoked via ON+PLAY on main viewer screen.
   Returns -1 if USB attached, 0 if no playlist change, and 1 if playlist
   changed. */
static int onplay_menu(int index)
{
    struct menu_item items[3]; /* increase this if you add entries! */
    int m, i=0, result, ret = 0;
    struct playlist_entry * current_track=
        playlist_buffer_get_track(&viewer.buffer, index);
    bool current = (current_track->index == viewer.current_playing_track);

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
                playlist_delete(viewer.playlist, current_track->index);
                if (current)
                {
                    /* Start playing new track except if it's the last track
                       in the playlist and repeat mode is disabled */
                    current_track=
                        playlist_buffer_get_track(&viewer.buffer, index);
                    if (current_track->display_index != viewer.num_tracks ||
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
                viewer.move_track = current_track->index;
                ret = 0;
                break;
            case 2:
            {
                onplay(current_track->name, TREE_ATTR_MPA, CONTEXT_TREE);

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
    return set_bool((char *)str(LANG_SHOW_ICONS),
                    &global_settings.playlist_viewer_icons);
}

/* Show indices of tracks? */
static bool show_indices(void)
{
    return set_bool((char *)str(LANG_SHOW_INDICES),
                    &global_settings.playlist_viewer_indices);
}

/* How to display a track */
static bool track_display(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_DISPLAY_TRACK_NAME_ONLY) },
        { STR(LANG_DISPLAY_FULL_PATH) }
    };

    return set_option((char *)str(LANG_TRACK_DISPLAY), 
                      &global_settings.playlist_viewer_track_display, INT, names, 2,
                      NULL);
}

/* Save playlist to disk */
static bool save_playlist(void)
{
    save_playlist_screen(viewer.playlist);
    return false;
}

/* View current playlist */
bool playlist_viewer(void)
{
    return playlist_viewer_ex(NULL);
}

char * playlist_callback_name(int selected_item, void * data, char *buffer)
{
    struct playlist_viewer * local_viewer = (struct playlist_viewer *)data;
    struct playlist_entry *track=
        playlist_buffer_get_track(&(local_viewer->buffer), selected_item);
    format_line(track, buffer, MAX_PATH);
    return(buffer);
}


void playlist_callback_icons(int selected_item, void * data, ICON * icon)
{
    struct playlist_viewer * local_viewer=(struct playlist_viewer *)data;
    struct playlist_entry *track=
        playlist_buffer_get_track(&(local_viewer->buffer), selected_item);
    if (track->index == local_viewer->current_playing_track)
    {
        /* Current playing track */
#ifdef HAVE_LCD_BITMAP
        *icon=bitmap_icons_6x8[Icon_Audio];
#else
        *icon=Icon_Audio;
#endif
    }
    else if (track->index == local_viewer->move_track)
    {
        /* Track we are moving */
#ifdef HAVE_LCD_BITMAP
        *icon=bitmap_icons_6x8[Icon_Moving];
#else
        *icon=Icon_Moving;
#endif
    }
    else if (track->queued)
    {
        /* Queued track */
#ifdef HAVE_LCD_BITMAP
        *icon=bitmap_icons_6x8[Icon_Queued];
#else
        *icon=Icon_Queued;
#endif
    }
    else
#ifdef HAVE_LCD_BITMAP
        *icon=0;
#else
        *icon=-1;
#endif
}

/* Main viewer function.  Filename identifies playlist to be viewed.  If NULL,
   view current playlist. */
bool playlist_viewer_ex(char* filename)
{
    bool ret = false;       /* return value */
    bool exit=false;        /* exit viewer */
    int button, lastbutton = BUTTON_NONE;
    struct gui_synclist playlist_lists;
    if (!playlist_viewer_init(&viewer, filename, false))
        goto exit;

    gui_synclist_init(&playlist_lists, playlist_callback_name, &viewer);
    gui_synclist_set_icon_callback(&playlist_lists,
                  global_settings.playlist_viewer_icons?
                  &playlist_callback_icons:NULL);
    gui_synclist_set_nb_items(&playlist_lists, viewer.num_tracks);
    gui_synclist_select_item(&playlist_lists, viewer.selected_track);
    gui_synclist_draw(&playlist_lists);
    while (!exit)
    {
        int track;
        if (!viewer.playlist && !(audio_status() & AUDIO_STATUS_PLAY))
        {
            /* Play has stopped */
#ifdef HAVE_LCD_CHARCELLS
            gui_syncsplash(HZ, true, str(LANG_END_PLAYLIST_PLAYER));
#else
            gui_syncsplash(HZ, true, str(LANG_END_PLAYLIST_RECORDER));
#endif
            goto exit;
        }

        if (viewer.move_track != -1)
            gui_synclist_flash(&playlist_lists);

        if (!viewer.playlist)
            playlist_get_resume_info(&track);
        else
            track = -1;

        if (track != viewer.current_playing_track ||
            playlist_amount_ex(viewer.playlist) != viewer.num_tracks)
        {
            /* Playlist has changed (new track started?) */
            if (!update_playlist(false))
                goto exit;
            gui_synclist_set_nb_items(&playlist_lists, viewer.num_tracks);
            /* Abort move on playlist change */
            viewer.move_track = -1;
        }

        /* Timeout so we can determine if play status has changed */
        button = button_get_w_tmo(HZ/2);
        int list_action;
        if( (list_action=gui_synclist_do_button(&playlist_lists, button))!=0 )
        {
            viewer.selected_track=gui_synclist_get_sel_pos(&playlist_lists);
            if(playlist_buffer_needs_reload(&viewer.buffer,
                viewer.selected_track))
                playlist_buffer_load_entries_screen(&viewer.buffer,
                    list_action==LIST_NEXT?
                        FORWARD
                        :
                        BACKWARD
                    );
        }
        switch (button)
        {
            case TREE_EXIT:
#ifdef TREE_RC_EXIT
            case TREE_RC_EXIT:
#endif
#ifdef TREE_OFF
            case TREE_OFF:
#endif
                exit = true;
                break;

#ifdef TREE_ENTER
            case TREE_ENTER:
            case TREE_ENTER | BUTTON_REPEAT:
#endif
#ifdef TREE_RC_RUN
            case TREE_RC_RUN:
#endif
            case TREE_RUN:
#ifdef TREE_RUN_PRE
                if (((button == TREE_RUN)
#ifdef TREE_RC_RUN_PRE
                    || (button == TREE_RC_RUN))
                        && ((lastbutton != TREE_RC_RUN_PRE)
#endif
                    && (lastbutton != TREE_RUN_PRE)))
                    break;
#endif
                struct playlist_entry * current_track =
                    playlist_buffer_get_track(&viewer.buffer,
                    viewer.selected_track);
                if (viewer.move_track >= 0)
                {
                    /* Move track */
                    int ret;

                    ret = playlist_move(viewer.playlist, viewer.move_track,
                        current_track->index);
                    if (ret < 0)
                        gui_syncsplash(HZ, true, str(LANG_MOVE_FAILED));

                    update_playlist(true);
                    viewer.move_track = -1;
                }
                else if (!viewer.playlist)
                {
                    /* play new track */
                    playlist_start(current_track->index, 0);
                    update_playlist(false);
                }
                else
                {
                    /* New playlist */
                    if (playlist_set_current(viewer.playlist) < 0)
                        goto exit;

                    playlist_start(current_track->index, 0);

                    /* Our playlist is now the current list */
                    if (!playlist_viewer_init(&viewer, NULL, true))
                        goto exit;
                }
                gui_synclist_draw(&playlist_lists);

                break;

            case TREE_CONTEXT:
#ifdef TREE_CONTEXT2
            case TREE_CONTEXT2:
#endif
#ifdef TREE_RC_CONTEXT
            case TREE_RC_CONTEXT:
#endif
            {
                /* ON+PLAY menu */
                int ret;

                ret = onplay_menu(viewer.selected_track);

                if (ret < 0)
                {
                    ret = true;
                    goto exit;
                }
                else if (ret > 0)
                {
                    /* Playlist changed */
                    gui_synclist_del_item(&playlist_lists);
                    update_playlist(true);
                    if (viewer.num_tracks <= 0)
                        exit = true;
                }
                gui_synclist_draw(&playlist_lists);
                break;
            }

            case TREE_MENU:
#ifdef TREE_RC_MENU
            case TREE_RC_MENU:
#endif
                if (viewer_menu())
                {
                    ret = true;
                    goto exit;
                }
                gui_synclist_set_icon_callback(
                    &playlist_lists,
                    global_settings.playlist_viewer_icons?
                        &playlist_callback_icons:NULL
                    );
                gui_synclist_draw(&playlist_lists);
                break;

            case BUTTON_NONE:
                gui_syncstatusbar_draw(&statusbars, false);
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    ret = true;
                    goto exit;
                }
                break;
        }
        lastbutton = button;
    }

exit:
    if (viewer.playlist)
        playlist_close(viewer.playlist);
    return ret;
}
char * playlist_search_callback_name(int selected_item, void * data, char *buffer)
{
    int *found_indicies = (int*)data;
    static struct playlist_track_info track;    
    playlist_get_track_info(viewer.playlist,found_indicies[selected_item],&track);
    format_name(buffer,track.filename);
    return(buffer);
}


void playlist_search_callback_icons(int selected_item, void * data, ICON * icon)
{
    (void)selected_item;
    (void)data;
#ifdef HAVE_LCD_BITMAP
        *icon=0;
#else
        *icon=-1;
#endif
}
bool search_playlist(void)
{
    char search_str[32] = "";
    bool ret = false, exit = false;
    int i, playlist_count;
    int found_indicies[MAX_PLAYLIST_ENTRIES],found_indicies_count = 0;
    int button;
    struct gui_synclist playlist_lists;
    struct playlist_track_info track;
    
    if (!playlist_viewer_init(&viewer, 0, false))
        return ret;
    if (kbd_input(search_str,sizeof(search_str)) == -1)
        return ret; 
    lcd_clear_display();  
    playlist_count = playlist_amount_ex(viewer.playlist);
    for (i=0;(i<playlist_count)&&(found_indicies_count<MAX_PLAYLIST_ENTRIES);i++)
    {
        gui_syncsplash(0, true, str(LANG_PLAYLIST_SEARCH_MSG),found_indicies_count,
#if CONFIG_KEYPAD == PLAYER_PAD
                   str(LANG_STOP_ABORT)
#else
                   str(LANG_OFF_ABORT)
#endif
        );
        if (SETTINGS_CANCEL == button_get(false))
            return ret;
        playlist_get_track_info(viewer.playlist,i,&track);
        if (strcasestr(track.filename,search_str))
        {
            found_indicies[found_indicies_count++] = track.index;
        }
    }
    if (!found_indicies_count)
    {
        return ret;
    }
    backlight_on();
    gui_synclist_init(&playlist_lists, playlist_search_callback_name,
                                found_indicies);
    gui_synclist_set_icon_callback(&playlist_lists,
                  global_settings.playlist_viewer_icons?
                  &playlist_search_callback_icons:NULL);
    gui_synclist_set_nb_items(&playlist_lists, found_indicies_count);
    gui_synclist_select_item(&playlist_lists, 0);
    gui_synclist_draw(&playlist_lists);
    while (!exit)
    {
        button = button_get(true);
        if (gui_synclist_do_button(&playlist_lists, button))
            continue;
        switch (button)
        {
            case TREE_EXIT:
#ifdef TREE_RC_EXIT
            case TREE_RC_EXIT:
#endif
#ifdef TREE_OFF
            case TREE_OFF:
#endif
                exit = true;
                break;

#ifdef TREE_ENTER
            case TREE_ENTER:
            case TREE_ENTER | BUTTON_REPEAT:
#endif
#ifdef TREE_RC_RUN
            case TREE_RC_RUN:
#endif
            case TREE_RUN:
                playlist_start(
                    found_indicies[gui_synclist_get_sel_pos(&playlist_lists)]
                    ,0);
                exit = 1;
            break;
            case BUTTON_NONE:
                break;
            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    ret = true;
                    exit = true;
                }
                break;
        }
    }
    return ret;
}

