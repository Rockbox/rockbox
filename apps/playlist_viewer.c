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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "playlist.h"
#include "audio.h"
#include "screens.h"
#include "settings.h"
#include "icons.h"
#include "menu.h"
#include "plugin.h"
#include "keyboard.h"
#include "filetypes.h"
#include "onplay.h"
#include "talk.h"
#include "misc.h"
#include "action.h"
#include "debug.h"
#include "backlight.h"

#include "lang.h"

#include "playlist_viewer.h"
#include "playlist_catalog.h"
#include "icon.h"
#include "list.h"
#include "splash.h"
#include "playlist_menu.h"
#include "menus/exported_menus.h"
#include "yesno.h"
#include "playback.h"

/* Maximum number of tracks we can have loaded at one time */
#define MAX_PLAYLIST_ENTRIES 200

/* The number of items between the selected one and the end/start of
 * the buffer under which the buffer must reload */
#define MIN_BUFFER_MARGIN (screens[0].getnblines()+1)

/* Information about a specific track */
struct playlist_entry {
    char *name;                 /* track path                               */
    int  index;                 /* Playlist index                           */
    int  display_index;         /* Display index                            */
    int  attr;                  /* Is track queued?; Is track marked as bad?*/
};

enum direction
{
    FORWARD,
    BACKWARD
};

enum pv_onplay_result {
    PV_ONPLAY_USB,
    PV_ONPLAY_USB_CLOSED,
    PV_ONPLAY_WPS_CLOSED,
    PV_ONPLAY_CLOSED,
    PV_ONPLAY_ITEM_REMOVED,
    PV_ONPLAY_CHANGED,
    PV_ONPLAY_UNCHANGED,
    PV_ONPLAY_SAVED,
};

struct playlist_buffer
{
    struct playlist_entry tracks[MAX_PLAYLIST_ENTRIES];

    char *name_buffer;        /* Buffer used to store track names */
    int buffer_size;          /* Size of name buffer */

    int first_index;          /* Real index of first track loaded inside
                                 the buffer */

    enum direction direction; /* Direction of the buffer (if the buffer
                                 was loaded BACKWARD, the last track in
                                 the buffer has a real index < to the
                                 real index of the the first track)*/

    int num_loaded;           /* Number of track entries loaded in buffer */
};

/* Global playlist viewer settings */
struct playlist_viewer {
    const char *title;          /* Playlist Viewer list title                */
    struct playlist_info* playlist; /* Playlist being viewed                 */
    int num_tracks;             /* Number of tracks in playlist              */
    int *initial_selection;     /* The initially selected track              */
    int current_playing_track;  /* Index of current playing track            */
    int selected_track;         /* The selected track, relative (first is 0) */
    int moving_track;           /* The track to move, relative (first is 0)
                                   or -1 if nothing is currently being moved */
    int moving_playlist_index;  /* Playlist-relative index (as opposed to
                                   viewer-relative index) of moving track    */
    struct playlist_buffer buffer;
};

struct playlist_search_data
{
    struct playlist_track_info *track;
    int *found_indicies;
};

static struct playlist_viewer  viewer;

/* Used when viewing playlists on disk */
static struct playlist_info temp_playlist;
static bool temp_playlist_init = false;

static void playlist_buffer_init(struct playlist_buffer *pb, char *names_buffer,
                                 int names_buffer_size);
static void playlist_buffer_load_entries(struct playlist_buffer * pb, int index,
                                         enum direction direction);
static int playlist_entry_load(struct playlist_entry *entry, int index,
                               char* name_buffer, int remaining_size);

static struct playlist_entry * playlist_buffer_get_track(struct playlist_buffer *pb,
                                                         int index);

static bool playlist_viewer_init(struct playlist_viewer * viewer,
                                 const char* filename, bool reload,
                                 int *most_recent_selection);

static void format_line(struct playlist_entry* track, char* str,
                        int len);

static bool update_playlist(bool force);
static enum pv_onplay_result onplay_menu(int index);

static void close_playlist_viewer(void);

static void playlist_buffer_init(struct playlist_buffer *pb, char *names_buffer,
                                 int names_buffer_size)
{
    pb->name_buffer = names_buffer;
    pb->buffer_size = names_buffer_size;
    pb->first_index = 0;
    pb->num_loaded = 0;
}

static int playlist_buffer_get_index(struct playlist_buffer *pb, int index)
{
    int buffer_index;
    if (pb->direction == FORWARD)
    {
        if (index >= pb->first_index)
            buffer_index = index-pb->first_index;
        else /* rotation : track0 in buffer + requested track */
            buffer_index = viewer.num_tracks-pb->first_index+index;
    }
    else
    {
        if (index <= pb->first_index)
            buffer_index = pb->first_index-index;
        else /* rotation : track0 in buffer + dist from the last track
                to the requested track (num_tracks-requested track) */
            buffer_index = pb->first_index+viewer.num_tracks-index;
    }
    return buffer_index;
}

/*
 * Loads the entries following 'index' in the playlist buffer
 */
static void playlist_buffer_load_entries(struct playlist_buffer *pb, int index,
                                         enum direction direction)
{
    int num_entries = viewer.num_tracks;
    char* p = pb->name_buffer;
    int remaining = pb->buffer_size;
    int i;

    pb->first_index = index;
    if (num_entries > MAX_PLAYLIST_ENTRIES)
        num_entries = MAX_PLAYLIST_ENTRIES;

    for (i = 0; i < num_entries; i++)
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

        if(direction == FORWARD)
            index++;
        else
            index--;
        index += viewer.num_tracks;
        index %= viewer.num_tracks;
    }
    pb->direction = direction;
    pb->num_loaded = i;
}

/* playlist_buffer_load_entries_screen()
 *      This function is called when the currently selected item gets too close
 *      to the start or the end of the loaded part of the playlist, or when
 *      the list callback requests a playlist item that has not been loaded yet
 *
 *      reference_track is either the currently selected track, or the track that
 *      has been requested by the callback, and has not been loaded yet.
 */
static void playlist_buffer_load_entries_screen(struct playlist_buffer * pb,
                                                enum direction direction,
                                                int reference_track)
{
    int start;
    if (direction == FORWARD)
    {
        int min_start = reference_track-2*screens[0].getnblines();
        while (min_start < 0)
            min_start += viewer.num_tracks;
        start = min_start % viewer.num_tracks;
    }
    else
    {
        int max_start = reference_track+2*screens[0].getnblines();
        start = max_start % viewer.num_tracks;
    }

    playlist_buffer_load_entries(pb, start, direction);
}

static bool retrieve_id3_tags(const int index, const char* name, struct mp3entry *id3, int flags)
{
    bool id3_retrieval_successful = false;

    if (!viewer.playlist &&
        (audio_status() & AUDIO_STATUS_PLAY) &&
        (playlist_get_resume_info(&viewer.current_playing_track) == index))
    {
        copy_mp3entry(id3, audio_current_track()); /* retrieve id3 from RAM */
        id3_retrieval_successful = true;
    }
    else
    {
    /* Read from disk, the database, doesn't store frequency, file size or codec (g4470) ChrisS*/
        id3_retrieval_successful = get_metadata_ex(id3, -1, name, flags);
    }
    return id3_retrieval_successful;
}

static int playlist_entry_load(struct playlist_entry *entry, int index,
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

    len = strlcpy(name_buffer, info.filename, remaining_size) + 1;

    if (global_settings.playlist_viewer_track_display >
        PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH && len <= remaining_size)
    {
        /* Allocate space for the id3viewc if the option is enabled */
        len += MAX_PATH + 1;
    }

    if (len <= remaining_size)
    {
        entry->name = name_buffer;
        entry->index = info.index;
        entry->display_index = info.display_index;
        entry->attr = info.attr & (PLAYLIST_ATTR_SKIPPED | PLAYLIST_ATTR_QUEUED);
        return len;
    }
    return -1;
}

#define distance(a, b) \
    a>b? (a) - (b) : (b) - (a)
static bool playlist_buffer_needs_reload(struct playlist_buffer* pb,
                                         int track_index)
{
    if (pb->num_loaded == viewer.num_tracks)
        return false;
    int selected_index = playlist_buffer_get_index(pb, track_index);
    int first_buffer_index = playlist_buffer_get_index(pb, pb->first_index);
    int distance_beginning = distance(selected_index, first_buffer_index);
    if (distance_beginning < MIN_BUFFER_MARGIN)
        return true;

    if (pb->num_loaded - distance_beginning < MIN_BUFFER_MARGIN)
       return true;
    return false;
}

static struct playlist_entry * playlist_buffer_get_track(struct playlist_buffer *pb,
                                                         int index)
{
    int buffer_index = playlist_buffer_get_index(pb, index);
    /* Make sure that we are not returning an invalid pointer.
       In some cases, when scrolling really fast, it could happen that a reqested track
       has not been pre-loaded */
    if (buffer_index < 0) {
        playlist_buffer_load_entries_screen(&viewer.buffer,
                    pb->direction == FORWARD ? BACKWARD : FORWARD,
                    index);

    } else if (buffer_index >= pb->num_loaded) {
        playlist_buffer_load_entries_screen(&viewer.buffer,
                    pb->direction,
                    index);
    }
    buffer_index = playlist_buffer_get_index(pb, index);
    if (buffer_index < 0 || buffer_index >= pb->num_loaded) {
        /* This really shouldn't happen. If this happens, then
           the name_buffer is probably too small to store enough
           titles to fill the screen, and preload data in the short
           direction.

           If this happens then scrolling performance will probably
           be quite low, but it's better then having Data Abort errors */
        playlist_buffer_load_entries(pb, index, FORWARD);
        buffer_index = playlist_buffer_get_index(pb, index);
    }
    return &(pb->tracks[buffer_index]);
}

/* Initialize the playlist viewer. */
static bool playlist_viewer_init(struct playlist_viewer * viewer,
                                 const char* filename, bool reload,
                                 int *most_recent_selection)
{
    char* buffer;
    size_t buffer_size;
    bool is_playing = audio_status() & (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE);
    bool have_list = filename || is_playing;
    if (!have_list && (global_status.resume_index != -1))
    {
        /* Try to restore the list from control file */
        have_list = (playlist_resume() != -1);
    }
    if (!have_list && (playlist_amount() > 0))
    {
         /*If dynamic playlist still exists, view it anyway even
        if playback has reached the end of the playlist */
        have_list = true;
    }
    if (!have_list)
    {
        /* Nothing to view, exit */
        splash(HZ, ID2P(LANG_CATALOG_NO_PLAYLISTS));
        return false;
    }

    buffer = plugin_get_buffer(&buffer_size);
    if (!buffer)
        return false;

    if (!filename)
    {
        viewer->playlist = NULL;
        viewer->title = (char *) str(LANG_PLAYLIST);
    }
    else
    {
        /* Viewing playlist on disk */
        const char *dir, *file;
        char *temp_ptr;
        char *index_buffer = NULL;
        ssize_t index_buffer_size = 0;

        /* Initialize temp playlist
         * TODO - move this to playlist.c */
        if (!temp_playlist_init)
        {
            mutex_init(&temp_playlist.mutex);
            temp_playlist_init = true;
        }

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
        viewer->title = file;

        if (is_playing)
        {
            /* Something is playing, try to accommodate
            *  global_settings.max_files_in_playlist entries */
            index_buffer_size = playlist_get_required_bufsz(viewer->playlist,
                                   false, global_settings.max_files_in_playlist);

            if ((unsigned)index_buffer_size >= buffer_size - MAX_PATH)
                index_buffer_size = buffer_size - (MAX_PATH + 1);

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
    playlist_buffer_init(&viewer->buffer, buffer, buffer_size);

    viewer->moving_track = -1;
    viewer->moving_playlist_index = -1;
    viewer->initial_selection = most_recent_selection;

    if (!reload)
    {
        if (viewer->playlist)
            viewer->selected_track = most_recent_selection ? *most_recent_selection : 0;
        else
            viewer->selected_track = playlist_get_display_index() - 1;
    }

    if (!update_playlist(true))
        return false;
    return true;
}

/* Format trackname for display purposes */
static void format_name(char* dest, const char* src, size_t bufsz)
{
    switch (global_settings.playlist_viewer_track_display)
    {
        case PLAYLIST_VIEWER_ENTRY_SHOW_FILE_NAME:
        case PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE_AND_ALBUM: /* If loading from tags failed, only display the file name */
        case PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE: /* If loading from tags failed, only display the file name */
        default:
        {
            /* Only display the filename */
            char* p = strrchr(src, '/');
            strlcpy(dest, p+1, bufsz);
            /* Remove the extension */
            strrsplt(dest, '.');
            break;
        }
        case PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH:
            /* Full path */
            strlcpy(dest, src, bufsz);
            break;
    }
}

/* Format display line */
static void format_line(struct playlist_entry* track, char* str,
                        int len)
{
    char *id3viewc = NULL;
    char *skipped = "";
    if (track->attr & PLAYLIST_ATTR_SKIPPED)
        skipped = "(ERR) ";
    if (!(track->attr & PLAYLIST_ATTR_RETRIEVE_ID3_ATTEMPTED) &&
        (global_settings.playlist_viewer_track_display ==
            PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE_AND_ALBUM ||
        global_settings.playlist_viewer_track_display ==
            PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE
    ))
    {
        track->attr |= PLAYLIST_ATTR_RETRIEVE_ID3_ATTEMPTED;
        struct mp3entry id3;
        bool retrieve_success = retrieve_id3_tags(track->index, track->name,
                                            &id3, METADATA_EXCLUDE_ID3_PATH);
        if (retrieve_success)
        {
            if (!id3viewc)
            {
                id3viewc = track->name + strlen(track->name) + 1;
            }
            struct mp3entry * pid3 = &id3;
            id3viewc[0] = '\0';
            if (global_settings.playlist_viewer_track_display ==
                PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE_AND_ALBUM)
            {
                /* Title & Album */
                if (pid3->title && pid3->title[0] != '\0')
                {
                    char* cur_str = id3viewc;
                    int title_len = strlen(pid3->title);
                    int rem_space = MAX_PATH;
                    for (int i = 0; i < title_len && rem_space > 0; i++)
                    {
                        cur_str[0] = pid3->title[i];
                        cur_str++;
                        rem_space--;
                    }
                    if (rem_space > 10)
                    {
                        cur_str[0] = (char) ' ';
                        cur_str[1] = (char) '-';
                        cur_str[2] = (char) ' ';
                        cur_str += 3;
                        rem_space -= 3;
                        cur_str = strmemccpy(cur_str, pid3->album && pid3->album[0] != '\0' ?
                            pid3->album : (char*) str(LANG_TAGNAVI_UNTAGGED), rem_space);
                        if (cur_str)
                            track->attr |= PLAYLIST_ATTR_RETRIEVE_ID3_SUCCEEDED;
                    }
                }
            }
            else if (global_settings.playlist_viewer_track_display ==
                PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE)
            {
                /* Just the title */
                if (pid3->title && pid3->title[0] != '\0' &&
                    strmemccpy(id3viewc, pid3->title, MAX_PATH)
                )
                    track->attr |= PLAYLIST_ATTR_RETRIEVE_ID3_SUCCEEDED;
            }
            /* Yield to reduce as much as possible the perceived UI lag, 
            because retrieving id3 tags is an expensive operation */
            yield();
        }
    }
    
    if (!(track->attr & PLAYLIST_ATTR_RETRIEVE_ID3_SUCCEEDED))
    {
        /* Simply use a formatted file name */
        char name[MAX_PATH];
        format_name(name, track->name, sizeof(name));
        if (global_settings.playlist_viewer_indices)
            /* Display playlist index */
            snprintf(str, len, "%d. %s%s", track->display_index, skipped, name);
        else
            snprintf(str, len, "%s%s", skipped, name);
    }
    else
    {
        if (!id3viewc)
        {
            id3viewc = track->name + strlen(track->name) + 1;
        }
        if (global_settings.playlist_viewer_indices)
            /* Display playlist index */
            snprintf(str, len, "%d. %s%s", track->display_index, skipped, id3viewc);
        else
            snprintf(str, len, "%s%s", skipped, id3viewc);
    }
}

/* Update playlist in case something has changed or forced */
static bool update_playlist(bool force)
{
    if (!viewer.playlist)
        playlist_get_resume_info(&viewer.current_playing_track);
    else
        viewer.current_playing_track = -1;
    int nb_tracks = playlist_amount_ex(viewer.playlist);

    if (force || nb_tracks != viewer.num_tracks)
    {
        /* Reload tracks */
        viewer.num_tracks = nb_tracks;
        if (viewer.num_tracks <= 0)
        {
            global_status.resume_index = -1;
            global_status.resume_offset = -1;
            global_status.resume_elapsed = -1;
            return false;
        }
        playlist_buffer_load_entries_screen(&viewer.buffer, FORWARD,
                          viewer.selected_track);
        if (viewer.buffer.num_loaded <= 0)
        {
            global_status.resume_index = -1;
            global_status.resume_offset = -1;
            global_status.resume_elapsed = -1;
            return false;
        }
    }
    return true;
}

static enum pv_onplay_result show_track_info(const struct playlist_entry *current_track)
{
    struct mp3entry id3;
    bool id3_retrieval_successful = retrieve_id3_tags(current_track->index, current_track->name, &id3, 0);

    return id3_retrieval_successful &&
            browse_id3_ex(&id3, viewer.playlist, current_track->display_index,
            viewer.num_tracks, NULL, 1) ? PV_ONPLAY_USB : PV_ONPLAY_UNCHANGED;
}

#if defined(HAVE_HOTKEY) || defined(HAVE_TAGCACHE)
static enum pv_onplay_result
    open_with_plugin(const struct playlist_entry *current_track,
                     const char* plugin_name,
                     int (*loadplugin)(const char* plugin, const char* file))
{
    char selected_track[MAX_PATH];
    close_playlist_viewer(); /* don't pop activity yet – relevant for plugin_load */

    strmemccpy(selected_track, current_track->name, sizeof(selected_track));

    int plugin_return = loadplugin(plugin_name, selected_track);
    pop_current_activity_without_refresh();

    switch (plugin_return)
    {
        case PLUGIN_USB_CONNECTED:
            return PV_ONPLAY_USB_CLOSED;
        case PLUGIN_GOTO_WPS:
            return PV_ONPLAY_WPS_CLOSED;
        default:
            return PV_ONPLAY_CLOSED;
    }
}

#ifdef HAVE_HOTKEY
static int list_viewers(const char* plugin, const char* file)
{
    /* dummy function to match prototype with filetype_load_plugin */
    (void)plugin;
    return filetype_list_viewers(file);
}
static enum pv_onplay_result open_with(const struct playlist_entry *current_track)
{
    return open_with_plugin(current_track, "", &list_viewers);
}
#endif /* HAVE_HOTKEY */

#ifdef HAVE_TAGCACHE
static enum pv_onplay_result open_pictureflow(const struct playlist_entry *current_track)
{
    return open_with_plugin(current_track, "pictureflow", &filetype_load_plugin);
}
#endif
#endif /*defined(HAVE_HOTKEY) || defined(HAVE_TAGCACHE)*/

static enum pv_onplay_result delete_track(int current_track_index,
                                          int index, bool current_was_playing)
{
    playlist_delete(viewer.playlist, current_track_index);
    if (current_was_playing)
    {
        if (playlist_amount_ex(viewer.playlist) <= 0)
            audio_stop();
        else
        {
           /* Start playing new track except if it's the lasttrack
              track in the playlist and repeat mode is disabled */
            struct playlist_entry *current_track =
                playlist_buffer_get_track(&viewer.buffer, index);
            if (current_track->display_index != viewer.num_tracks ||
                global_settings.repeat_mode == REPEAT_ALL)
            {
                audio_play(0, 0);
                viewer.current_playing_track = -1;
            }
        }
    }
    return PV_ONPLAY_ITEM_REMOVED;
}

/* Menu of playlist commands.  Invoked via ON+PLAY on main viewer screen. */
static enum pv_onplay_result onplay_menu(int index)
{
    int result, ret = PV_ONPLAY_UNCHANGED;
    struct playlist_entry * current_track =
        playlist_buffer_get_track(&viewer.buffer, index);
    MENUITEM_STRINGLIST(menu_items, ID2P(LANG_PLAYLIST), NULL,
                        ID2P(LANG_PLAYING_NEXT), ID2P(LANG_ADD_TO_PL),
                        ID2P(LANG_REMOVE), ID2P(LANG_MOVE), ID2P(LANG_MENU_SHOW_ID3_INFO),
                        ID2P(LANG_SHUFFLE),
                        ID2P(LANG_SAVE),
                        ID2P(LANG_PLAYLISTVIEWER_SETTINGS)
#ifdef HAVE_TAGCACHE
                        ,ID2P(LANG_ONPLAY_PICTUREFLOW)
#endif
                        );

    bool current_was_playing = (current_track->index == viewer.current_playing_track);

    result = do_menu(&menu_items, NULL, NULL, false);
    if (result == MENU_ATTACHED_USB)
    {
        ret = PV_ONPLAY_USB;
    }
    else if (result >= 0)
    {
        /* Abort current move */
        viewer.moving_track = -1;
        viewer.moving_playlist_index = -1;

        switch (result)
        {
            case 0:
                /* playlist */
                onplay_show_playlist_menu(current_track->name, FILE_ATTR_AUDIO, NULL);
                ret = PV_ONPLAY_UNCHANGED;
                break;
            case 1:
                /* add to catalog */
                onplay_show_playlist_cat_menu(current_track->name, FILE_ATTR_AUDIO, NULL);
                ret = PV_ONPLAY_UNCHANGED;
                break;
            case 2:
                ret = delete_track(current_track->index, index, current_was_playing);
                break;
            case 3:
                /* move track */
                viewer.moving_track = index;
                viewer.moving_playlist_index = current_track->index;
                ret = PV_ONPLAY_UNCHANGED;
                break;
            case 4:
                ret = show_track_info(current_track);
                break;
            case 5:
                /* shuffle */
                playlist_sort(viewer.playlist, !viewer.playlist);
                playlist_randomise(viewer.playlist, current_tick, !viewer.playlist);
                viewer.selected_track = 0;
                ret = PV_ONPLAY_CHANGED;
                break;
            case 6:
                save_playlist_screen(viewer.playlist);
                /* playlist indices of current playlist may have changed */
                ret = viewer.playlist ? PV_ONPLAY_UNCHANGED : PV_ONPLAY_SAVED;
                break;
            case 7:
            {
                int last_display = global_settings.playlist_viewer_track_display;
                /* playlist viewer settings */
                result = do_menu(&viewer_settings_menu, NULL, NULL, false);


                if (result == MENU_ATTACHED_USB)
                    ret = PV_ONPLAY_USB;
                else
                {
                    if (last_display != global_settings.playlist_viewer_track_display)
                        update_playlist(true);/* reload buffer */

                    ret = PV_ONPLAY_UNCHANGED;
                }
                break;
            }
#ifdef HAVE_TAGCACHE
            case 8:
                ret = open_pictureflow(current_track);
                break;
#endif
        }
    }
    return ret;
}

/* View current playlist */
enum playlist_viewer_result playlist_viewer(void)
{
    return playlist_viewer_ex(NULL, NULL);
}

static int get_track_num(struct playlist_viewer *local_viewer,
                         int selected_item)
{
    if (local_viewer->moving_track >= 0)
    {
        if (local_viewer->selected_track == selected_item)
        {
            return local_viewer->moving_track;
        }
        else if (local_viewer->selected_track > selected_item
            && selected_item >= local_viewer->moving_track)
        {
            return selected_item+1; /* move down */
        }
        else if (local_viewer->selected_track < selected_item
            && selected_item <= local_viewer->moving_track)
        {
            return selected_item-1; /* move up */
        }
    }
    return selected_item;
}

static struct playlist_entry* pv_get_track(struct playlist_viewer *local_viewer, int selected_item)
{
    int track_num = get_track_num(local_viewer, selected_item);
    return playlist_buffer_get_track(&(local_viewer->buffer), track_num);
}

static const char* playlist_callback_name(int selected_item,
                                          void *data,
                                          char *buffer,
                                          size_t buffer_len)
{
    struct playlist_entry *track = pv_get_track(data, selected_item);

    format_line(track, buffer, buffer_len);

    return(buffer);
}


static enum themable_icons playlist_callback_icons(int selected_item,
                                                   void *data)
{
    struct playlist_viewer *local_viewer = (struct playlist_viewer *)data;
    struct playlist_entry *track = pv_get_track(local_viewer, selected_item);

    if (track->index == local_viewer->current_playing_track)
    {
        /* Current playing track */
        return Icon_Audio;
    }
    else if (track->index == local_viewer->moving_playlist_index)
    {
        /* Track we are moving */
        return Icon_Moving;
    }
    else if (track->attr & PLAYLIST_ATTR_QUEUED)
    {
        /* Queued track */
        return Icon_Queued;
    }
    else
        return Icon_NOICON;
}

static int playlist_callback_voice(int selected_item, void *data)
{
    struct playlist_viewer *local_viewer = (struct playlist_viewer *)data;
    struct playlist_entry *track = pv_get_track(local_viewer, selected_item);

    if(global_settings.playlist_viewer_icons) {
        if (track->index == local_viewer->current_playing_track)
            talk_id(LANG_NOW_PLAYING, true);
        if (track->index == local_viewer->moving_track)
            talk_id(VOICE_TRACK_TO_MOVE, true);
        if (track->attr & PLAYLIST_ATTR_QUEUED)
            talk_id(VOICE_QUEUED, true);
    }
    if (track->attr & PLAYLIST_ATTR_SKIPPED)
        talk_id(VOICE_BAD_TRACK, true);
    if (global_settings.playlist_viewer_indices)
        talk_number(track->display_index, true);

    switch(global_settings.playlist_viewer_track_display)
    {
        case PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH:
            /*full path*/
            talk_fullpath(track->name, true);
            break;
        default:
        case PLAYLIST_VIEWER_ENTRY_SHOW_FILE_NAME:
            /*filename only*/
        case PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE_AND_ALBUM:
            /* If loading from tags failed, only talk the file name */
        case PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE:
            /* If loading from tags failed, only talk the file name */
            talk_file_or_spell(NULL, track->name, NULL, true);
            break;
    }
    if (viewer.moving_track != -1)
        talk_ids(true,VOICE_PAUSE, VOICE_MOVING_TRACK);

    return 0;
}

static void update_lists(struct gui_synclist * playlist_lists, bool init)
{
    if (init)
        gui_synclist_init(playlist_lists, playlist_callback_name,
                          &viewer, false, 1, NULL);
    gui_synclist_set_nb_items(playlist_lists, viewer.num_tracks);
    gui_synclist_set_voice_callback(playlist_lists,
                                    global_settings.talk_file?
                                    &playlist_callback_voice:NULL);
    gui_synclist_set_icon_callback(playlist_lists,
                  global_settings.playlist_viewer_icons?
                  &playlist_callback_icons:NULL);
    gui_synclist_set_title(playlist_lists, viewer.title, Icon_Playlist);
    gui_synclist_select_item(playlist_lists, viewer.selected_track);
    gui_synclist_draw(playlist_lists);
    gui_synclist_speak_item(playlist_lists);
}

static bool update_viewer_with_changes(struct gui_synclist *playlist_lists, enum pv_onplay_result res)
{
    bool exit = false;
    if (res == PV_ONPLAY_CHANGED ||
        res == PV_ONPLAY_SAVED ||
        res == PV_ONPLAY_ITEM_REMOVED)
    {
        if (res != PV_ONPLAY_SAVED)
            playlist_set_modified(viewer.playlist, true);

        if (res == PV_ONPLAY_ITEM_REMOVED)
            gui_synclist_del_item(playlist_lists);

        update_playlist(true);

        if (viewer.num_tracks <= 0)
            exit = true;

        if (viewer.selected_track >= viewer.num_tracks)
            viewer.selected_track = viewer.num_tracks-1;
    }

    /* the show_icons option in the playlist viewer settings
     * menu might have changed */
    update_lists(playlist_lists, false);
    return exit;
}

static bool open_playlist_viewer(const char* filename,
                                  struct gui_synclist *playlist_lists,
                                  bool reload, int *most_recent_selection)
{
    push_current_activity(ACTIVITY_PLAYLISTVIEWER);

    if (!playlist_viewer_init(&viewer, filename, reload, most_recent_selection))
        return false;

    update_lists(playlist_lists, true);

    return true;
}

/* Main viewer function.  Filename identifies playlist to be viewed.  If NULL,
   view current playlist. */
enum playlist_viewer_result playlist_viewer_ex(const char* filename,
                                               int* most_recent_selection)
{
    enum playlist_viewer_result ret = PLAYLIST_VIEWER_OK;
    bool exit = false;        /* exit viewer */
    int button;
    struct gui_synclist playlist_lists;

    if (!open_playlist_viewer(filename, &playlist_lists, false, most_recent_selection))
    {
        ret = PLAYLIST_VIEWER_CANCEL;
        goto exit;
    }

    while (!exit)
    {
        int track;

        if (global_status.resume_index != -1 && !viewer.playlist)
            playlist_get_resume_info(&track);
        else
            track = -1;

        if (track != viewer.current_playing_track ||
            playlist_amount_ex(viewer.playlist) != viewer.num_tracks)
        {
            /* Playlist has changed (new track started?) */
            if (!update_playlist(false))
                goto exit;
            /*Needed because update_playlist gives wrong value when
                                                            playing is stopped*/
            viewer.current_playing_track = track;
            gui_synclist_set_nb_items(&playlist_lists, viewer.num_tracks);

            gui_synclist_draw(&playlist_lists);
            gui_synclist_speak_item(&playlist_lists);
        }

        /* Timeout so we can determine if play status has changed */
        bool res = list_do_action(CONTEXT_TREE, HZ/2, &playlist_lists, &button);
        /* during moving, another redraw is going to be needed,
         * since viewer.selected_track is updated too late (after the first draw)
         * drawing the moving item needs it */
        viewer.selected_track=gui_synclist_get_sel_pos(&playlist_lists);
        if (res)
        {
            bool reload = playlist_buffer_needs_reload(&viewer.buffer,
                    viewer.selected_track);
            if (reload)
                playlist_buffer_load_entries_screen(&viewer.buffer,
                    button == ACTION_STD_NEXT ? FORWARD : BACKWARD,
                    viewer.selected_track);
            if (reload || viewer.moving_track >= 0)
                gui_synclist_draw(&playlist_lists);
        }
        switch (button)
        {
            case ACTION_TREE_WPS:
            case ACTION_STD_CANCEL:
            {
                if (viewer.moving_track >= 0)
                {
                    viewer.selected_track = viewer.moving_track;
                    gui_synclist_select_item(&playlist_lists, viewer.moving_track);
                    viewer.moving_track = -1;
                    viewer.moving_playlist_index = -1;
                    gui_synclist_draw(&playlist_lists);
                    gui_synclist_speak_item(&playlist_lists);
                }
                else
                {
                    exit = true;
                    ret = button == ACTION_TREE_WPS ?
                                    PLAYLIST_VIEWER_OK : PLAYLIST_VIEWER_CANCEL;
                }
                break;
            }
            case ACTION_STD_OK:
            {
                struct playlist_entry * current_track =
                            playlist_buffer_get_track(&viewer.buffer,
                                                      viewer.selected_track);

                if (viewer.moving_track >= 0)
                {
                    /* Move track */
                    int ret_val;

                    ret_val = playlist_move(viewer.playlist,
                                            viewer.moving_playlist_index,
                                            current_track->index);
                    if (ret_val < 0)
                    {
                         cond_talk_ids_fq(LANG_MOVE, LANG_FAILED);
                         splashf(HZ, (unsigned char *)"%s %s", str(LANG_MOVE),
                                                               str(LANG_FAILED));
                    }

                    playlist_set_modified(viewer.playlist, true);

                    update_playlist(true);
                    viewer.moving_track = -1;
                    viewer.moving_playlist_index = -1;
                }
                else if (global_settings.party_mode)
                {
                    /* Nothing to do */
                }
                else if (!viewer.playlist)
                {
                    /* play new track */
                    playlist_start(current_track->index, 0, 0);
                    update_playlist(false);
                }
                else
                {
                    int start_index = current_track->index;
                    if (!warn_on_pl_erase())
                    {
                        gui_synclist_set_title(&playlist_lists, playlist_lists.title, playlist_lists.title_icon);
                        gui_synclist_draw(&playlist_lists);
                        break;
                    }
                    /* New playlist */
                    if (playlist_set_current(viewer.playlist) < 0)
                        goto exit;
                    if (global_settings.playlist_shuffle)
                        start_index = playlist_shuffle(current_tick, start_index);
                    playlist_start(start_index, 0, 0);

                    if (viewer.initial_selection)
                        *(viewer.initial_selection) = viewer.selected_track;

                    /* Our playlist is now the current list */
                    if (!playlist_viewer_init(&viewer, NULL, true, NULL))
                        goto exit;
                    exit = true;
                }
                gui_synclist_draw(&playlist_lists);
                gui_synclist_speak_item(&playlist_lists);

                break;
            }
            case ACTION_STD_CONTEXT:
            {
                int pv_onplay_result = onplay_menu(viewer.selected_track);

                if (pv_onplay_result == PV_ONPLAY_USB)
                {
                    ret = PLAYLIST_VIEWER_USB;
                    goto exit;
                }
                else if (pv_onplay_result == PV_ONPLAY_USB_CLOSED)
                    return PLAYLIST_VIEWER_USB;
                else if (pv_onplay_result == PV_ONPLAY_WPS_CLOSED)
                    return PLAYLIST_VIEWER_OK;
                else if (pv_onplay_result == PV_ONPLAY_CLOSED)
                {
                    if (!open_playlist_viewer(filename, &playlist_lists, true, NULL))
                    {
                        ret = PLAYLIST_VIEWER_CANCEL;
                        goto exit;
                    }
                    break;
                }
                if (update_viewer_with_changes(&playlist_lists, pv_onplay_result))
                {
                    exit = true;
                    ret = PLAYLIST_VIEWER_CANCEL;
                }
                break;
            }
            case ACTION_STD_MENU:
                ret = PLAYLIST_VIEWER_MAINMENU;
                goto exit;
#ifdef HAVE_QUICKSCREEN
            case ACTION_STD_QUICKSCREEN:
                    if (!global_settings.shortcuts_replaces_qs)
                    {
                        if (quick_screen_quick(button) ==
                            QUICKSCREEN_GOTO_SHORTCUTS_MENU) /* currently disabled */
                        {
                            /* QuickScreen defers skin updates when popping its activity
                               to switch to Shortcuts Menu, so make up for that here: */
                            FOR_NB_SCREENS(i)
                                skin_update(CUSTOM_STATUSBAR, i, SKIN_REFRESH_ALL);
                        }
                        update_playlist(true);
                        update_lists(&playlist_lists, true);
                    }
                    break;
#endif
#ifdef HAVE_HOTKEY
            case ACTION_TREE_HOTKEY:
            {
                struct playlist_entry *current_track = playlist_buffer_get_track(
                                                            &viewer.buffer,
                                                            viewer.selected_track);
                enum pv_onplay_result (*do_plugin)(const struct playlist_entry *) = NULL;
#ifdef HAVE_TAGCACHE
                if (global_settings.hotkey_tree == HOTKEY_PICTUREFLOW)
                    do_plugin = &open_pictureflow;
#endif
                if (global_settings.hotkey_tree == HOTKEY_OPEN_WITH)
                    do_plugin = &open_with;

                if (do_plugin != NULL)
                {
                    int plugin_result = do_plugin(current_track);

                    if (plugin_result == PV_ONPLAY_USB_CLOSED)
                        return PLAYLIST_VIEWER_USB;
                    else if (plugin_result == PV_ONPLAY_WPS_CLOSED)
                        return PLAYLIST_VIEWER_OK;
                    else if (!open_playlist_viewer(filename, &playlist_lists, true, NULL))
                    {
                        ret = PLAYLIST_VIEWER_CANCEL;
                        goto exit;
                    }
                }
                else if (global_settings.hotkey_tree == HOTKEY_PROPERTIES)
                {
                    if (show_track_info(current_track) == PV_ONPLAY_USB)
                    {
                        ret = PLAYLIST_VIEWER_USB;
                        goto exit;
                    }
                    update_lists(&playlist_lists, false);
                }
                else if (global_settings.hotkey_tree == HOTKEY_DELETE)
                {
                    if (update_viewer_with_changes(&playlist_lists,
                            delete_track(current_track->index,
                            viewer.selected_track,
                            (current_track->index == viewer.current_playing_track))))
                    {
                        ret = PLAYLIST_VIEWER_CANCEL;
                        exit = true;
                    }
                }
                else
                    onplay(current_track->name, FILE_ATTR_AUDIO, CONTEXT_STD, true, ONPLAY_NO_CUSTOMACTION);
                break;
            }
#endif /* HAVE_HOTKEY */
            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    ret = PLAYLIST_VIEWER_USB;
                    goto exit;
                }
                break;
        }
    }

exit:
    pop_current_activity_without_refresh();
    close_playlist_viewer();
    return ret;
}

static void close_playlist_viewer(void)
{
    talk_shutup();
    if (viewer.playlist)
    {
        if (viewer.initial_selection)
            *(viewer.initial_selection) = viewer.selected_track;

        if(playlist_modified(viewer.playlist) && yesno_pop(ID2P(LANG_SAVE_CHANGES)))
            save_playlist_screen(viewer.playlist);
        playlist_close(viewer.playlist);
    }
}

static const char* playlist_search_callback_name(int selected_item, void * data,
                                                 char *buffer, size_t buffer_len)
{
    struct playlist_search_data *s_data = data;
    playlist_get_track_info(viewer.playlist, s_data->found_indicies[selected_item], s_data->track);
    format_name(buffer, s_data->track->filename, buffer_len);
    return buffer;
}

static int say_search_item(int selected_item, void *data)
{
    struct playlist_search_data *s_data = data;
    playlist_get_track_info(viewer.playlist, s_data->found_indicies[selected_item], s_data->track);
    if(global_settings.playlist_viewer_track_display == PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH)
        /* full path*/
        talk_fullpath(s_data->track->filename, false);
    else talk_file_or_spell(NULL, s_data->track->filename, NULL, false);
    return 0;
}

bool search_playlist(void)
{
    char search_str[32] = "";
    bool ret = false, exit = false;
    int i, playlist_count;
    int found_indicies[MAX_PLAYLIST_ENTRIES];
    int found_indicies_count = 0, last_found_count = -1;
    int button;
    int track_display = global_settings.playlist_viewer_track_display;
    struct gui_synclist playlist_lists;
    struct playlist_track_info track;

    if (!playlist_viewer_init(&viewer, 0, false, NULL))
        return ret;
    if (kbd_input(search_str, sizeof(search_str), NULL) < 0)
        return ret;
    lcd_clear_display();
    playlist_count = playlist_amount_ex(viewer.playlist);
    cond_talk_ids_fq(LANG_WAIT);

    cpu_boost(true);

    for (i = 0; i < playlist_count &&
        found_indicies_count < MAX_PLAYLIST_ENTRIES; i++)
    {
        if (found_indicies_count != last_found_count)
        {
            splashf(0, str(LANG_PLAYLIST_SEARCH_MSG), found_indicies_count,
                       str(LANG_OFF_ABORT));
            last_found_count = found_indicies_count;
        }

        if (action_userabort(TIMEOUT_NOBLOCK))
            break;

        playlist_get_track_info(viewer.playlist, i, &track);
        const char *trackname = track.filename;
        if (track_display != PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH)
            /* if we only display filename only search filename */
            trackname = strrchr(track.filename, '/');

        if (trackname && strcasestr(trackname, search_str))
            found_indicies[found_indicies_count++] = track.index;

        yield();
    }

    cpu_boost(false);

    cond_talk_ids_fq(TALK_ID(found_indicies_count, UNIT_INT),
                     LANG_PLAYLIST_SEARCH_MSG);
    if (!found_indicies_count)
    {
        return ret;
    }
    backlight_on();
    struct playlist_search_data s_data = {.track = &track, .found_indicies = found_indicies}; 
    gui_synclist_init(&playlist_lists, playlist_search_callback_name,
                      &s_data, false, 1, NULL);
    gui_synclist_set_title(&playlist_lists, str(LANG_SEARCH_RESULTS), NOICON);
    if(global_settings.talk_file)
        gui_synclist_set_voice_callback(&playlist_lists,
                                        global_settings.talk_file?
                                        &say_search_item:NULL);
    gui_synclist_set_nb_items(&playlist_lists, found_indicies_count);
    gui_synclist_select_item(&playlist_lists, 0);
    gui_synclist_draw(&playlist_lists);
    gui_synclist_speak_item(&playlist_lists);
    while (!exit)
    {
        if (list_do_action(CONTEXT_LIST, HZ/4, &playlist_lists, &button))
            continue;
        switch (button)
        {
            case ACTION_STD_CANCEL:
                exit = true;
                break;

            case ACTION_STD_OK:
            {
                int sel = gui_synclist_get_sel_pos(&playlist_lists);
                playlist_start(found_indicies[sel], 0, 0);
                exit = 1;
            }
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    ret = true;
                    exit = true;
                }
                break;
        }
    }
    talk_shutup();
    return ret;
}
