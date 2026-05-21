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

#ifdef HAVE_DISK_STORAGE
#include "storage.h"
#endif

#if defined (HAVE_TAGCACHE) && defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
#include "tagcache.h"
#endif

/* Maximum number of tracks we can have loaded at one time                   */
#define MAX_PLAYLIST_ENTRIES 200

/* Maximum file name length in name buffer                                   */
#define MAX_NAME_SZ (sizeof ((struct playlist_track_info *)0)->filename)

/* Maximum formatted metadata length in name buffer                          */
#define MAX_ID3_SZ MAX_PATH

/* Maximum single track info length in name buffer                           */
#define MAX_TRACK_INFO (MAX_NAME_SZ + MAX_ID3_SZ)

/* Maximum amount of space required for the name buffer.                     */
#define MAX_NAME_BUFFER_SZ (MAX_PLAYLIST_ENTRIES * MAX_TRACK_INFO)

/* Over-approximation of view_text plugin size                               */
#define VIEW_TEXT_PLUGIN_SZ 5000

/* The number of items between the selected one and the end/start of
 * the buffer under which the buffer must reload                             */
#define MIN_BUFFER_MARGIN (screens[0].getnblines()+1)

/* Information about a specific track */
struct playlist_entry {
    char *name;                 /* track path                                */
    int  index;                 /* Playlist index                            */
    int  display_index;         /* Display index                             */
    int  attr;                  /* Is track queued?; Is track marked as bad? */
};

enum direction
{
    FORWARD,
    BACKWARD
};

/* Describes possible outcomes from context (menu or hotkey) action          */
enum pv_context_result {
    PV_CONTEXT_CLOSED,          /* Playlist Viewer has been closed           */
    PV_CONTEXT_USB,             /* USB-connection initiated                  */
    PV_CONTEXT_USB_CLOSED,      /* USB-connection initiated (+viewer closed) */
    PV_CONTEXT_WPS_CLOSED,      /* WPS requested (+viewer closed)            */
    PV_CONTEXT_MODIFIED,        /* Playlist was modified in some way         */
    PV_CONTEXT_UNCHANGED,       /* No change to playlist, as far as we know  */
    PV_CONTEXT_PL_UPDATE,       /* Playlist buffer requires reloading        */
};

struct playlist_buffer
{
    struct playlist_entry tracks[MAX_PLAYLIST_ENTRIES];

    char *name_buffer;          /* Buffer used to store track names          */
    int buffer_size;            /* Size of name buffer                       */

    int first_index;            /* Real index of first track loaded inside
                                   the buffer                                */

    enum direction direction;   /* Direction of the buffer (if the buffer
                                   was loaded BACKWARD, the last track in
                                   the buffer has a real index < to the
                                   real index of the the first track)        */

    int num_loaded;             /* Number of track entries loaded in buffer  */
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
    struct mp3entry *id3;
    bool allow_view_text_plugin;
    unsigned long loading_tick;
    bool is_open;
};

struct playlist_search_data
{
    struct playlist_track_info *track;
    int *found_indicies;
};

static struct playlist_viewer  viewer;

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

    /* Allocate space for formatted metadata, if option is enabled */
    if (global_settings.playlist_viewer_track_display
        > PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH)
        len += MAX_ID3_SZ;

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
        /* provide UI feedback if opening Playlist Viewer is taking too long */
        if (!viewer.is_open && TIME_AFTER(current_tick, viewer.loading_tick))
        {
            viewer.loading_tick += HZ*10;
            splash(0, ID2P(LANG_WAIT));
        }

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
#if defined (HAVE_TAGCACHE) && defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    else if (flags & METADATA_EXCLUDE_ID3_PATH)
        /* retrieve id3 from database */
        id3_retrieval_successful = tagcache_fill_tags(id3, name);
#endif

    if (!id3_retrieval_successful)
    {
        /* Read from disk: retrieves frequency, file size, and codec */
        id3_retrieval_successful = get_metadata_ex(id3, -1, name, flags);
    }
    return id3_retrieval_successful;
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
            if (!viewer.playlist)
                playlist_update_resume_info(NULL);
            return false;
        }
        playlist_buffer_load_entries_screen(&viewer.buffer, FORWARD,
                          viewer.selected_track);
        if (viewer.buffer.num_loaded <= 0)
        {
            if (!viewer.playlist)
                playlist_update_resume_info(NULL);
            return false;
        }
    }
    return true;
}

/* Initialize the playlist viewer. */
static bool playlist_viewer_init(struct playlist_viewer * viewer,
                                 const char* dir, const char* file,
                                 bool reload, int *recent_selection)
{
    char *buffer, *index_buffer = NULL;
    size_t buffer_size, index_buffer_size = 0;
    bool is_playing = audio_status() & AUDIO_STATUS_PLAY; /* playing or paused */

    /* FIXME: On devices with a plugin buffer smaller than 512 KiB, the index buffer
              is shared with the current playlist when playback is stopped, to enable
              displaying larger playlists. This is generally unsafe, since it is possible
              to start playback of a new playlist while another list is still open in the
              viewer. Devices affected by this, as of the time of writing, appear to be:
              - 80 KiB plugin buffer: Sansa c200v2
              - 64 KiB plugin buffer: Sansa m200v4, Sansa Clip
    */
    bool require_index_buffer = file && (is_playing || PLUGIN_BUFFER_SIZE >= 0x80000);

    if (!file && !is_playing)
    {
        /* Try to restore the list from control file */
        if (playlist_resume() == -1)
        {
            /* Nothing to view, exit */
            splash(HZ, ID2P(LANG_CATALOG_NO_PLAYLISTS));
            return false;
        }
    }

    size_t id3_size = ALIGN_UP(sizeof(*viewer->id3), 4);

    buffer = plugin_get_buffer(&buffer_size);
    if (!buffer || buffer_size <= MAX_TRACK_INFO + id3_size)
        return false;

    /* Leave space to fit id3 struct and at least a single track in name buffer */
    if (require_index_buffer)
        index_buffer_size = playlist_get_index_bufsz(buffer_size - id3_size - MAX_TRACK_INFO);

    /* Check for unused space in the plugin buffer to run
       the view_text plugin used by the Track Info screen:
    ┌───────┬───────────────────────────────────────────────────────┐
    │       │<----------------- plugin_get_buffer ----------------->│
    │ (TSR  ├───────────────┬─────┬──────────────┬───────────────┐  │
    │plugin)│██ view_text ██│ id3 │ index buffer │  name buffer  │  │
    └───────┴───────────────┴─────┴──────────────┴───────────────┴──┘
    */
    viewer->allow_view_text_plugin = (buffer_size >=
        VIEW_TEXT_PLUGIN_SZ + id3_size + index_buffer_size + MAX_NAME_BUFFER_SZ);

    if (viewer->allow_view_text_plugin)
    {
        buffer += VIEW_TEXT_PLUGIN_SZ;
        buffer_size -= VIEW_TEXT_PLUGIN_SZ;
    }
    viewer->id3 = (void *) buffer;
    buffer += id3_size;
    buffer_size -= id3_size;

    /* Viewing playlist on disk */
    if (file)
    {
        if (require_index_buffer)
        {
            index_buffer = buffer;
            buffer += index_buffer_size;
            buffer_size -= index_buffer_size;
        }
        viewer->playlist = playlist_load(dir, file,
                                         index_buffer, index_buffer_size,
                                         buffer, buffer_size);
    }
    else
        viewer->playlist = NULL;

    playlist_buffer_init(&viewer->buffer, buffer, buffer_size);

    viewer->moving_track = -1;
    viewer->moving_playlist_index = -1;
    viewer->initial_selection = recent_selection;

    if (!reload)
    {
        if (viewer->playlist)
            viewer->selected_track = recent_selection ? *recent_selection : 0;
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
        /* If loading from tags failed, only display the file name */
        case PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE_AND_ALBUM:
        case PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE:
        default:
        {
            /* Only display the filename */
            const char* p = strrchr(src, '/');
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

static char* retrieve_formatted_id3(struct playlist_entry* track)
{
    struct mp3entry *id3 = viewer.id3;
    char *formatted_id3 = track->name + strlen(track->name) + 1;
    bool retrieve_success = retrieve_id3_tags(track->index, track->name,
                                              id3, METADATA_EXCLUDE_ID3_PATH);
    yield();
    track->attr |= PLAYLIST_ATTR_RETRIEVE_ID3_ATTEMPTED;
    if (!retrieve_success || !id3->title || !*id3->title)
        return NULL;
    track->attr |= PLAYLIST_ATTR_RETRIEVE_ID3_SUCCEEDED;

    size_t len = strlcpy(formatted_id3, id3->title, MAX_ID3_SZ);
    if (global_settings.playlist_viewer_track_display
        != PLAYLIST_VIEWER_ENTRY_SHOW_ID3_TITLE && id3->album && *id3->album &&
        len < MAX_ID3_SZ - 10)
    {
        formatted_id3[len++] = ' ';
        formatted_id3[len++] = '-';
        formatted_id3[len++] = ' ';
        strmemccpy(&formatted_id3[len], id3->album, MAX_ID3_SZ - len);
    }
    return formatted_id3;
}

/* Format display line */
static void format_line(struct playlist_entry* track, char* buf, int buf_sz)
{
    char name[MAX_PATH], *skipped, *prefix, *suffix, *formatted_name = NULL;
    bool show_id3 = global_settings.playlist_viewer_track_display
                    > PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH;
    skipped = prefix = suffix = "";
    if (track->attr & PLAYLIST_ATTR_SKIPPED)
        skipped = "(ERR) ";
    if (track->index == viewer.current_playing_track)
    {
        prefix = "[";
        suffix = "]";
    }

    if (show_id3 && !(track->attr & PLAYLIST_ATTR_RETRIEVE_ID3_ATTEMPTED))
        formatted_name = retrieve_formatted_id3(track);

    if (!(track->attr & PLAYLIST_ATTR_RETRIEVE_ID3_SUCCEEDED))
    {
        format_name(name, track->name, sizeof(name));
        formatted_name = name;
    }
    else if (!formatted_name)
        formatted_name = track->name + strlen(track->name) + 1;

    if (global_settings.playlist_viewer_indices)
        snprintf(buf, buf_sz, "%s%d. %s%s%s", prefix, track->display_index,
                 skipped, formatted_name, suffix);
    else
        snprintf(buf, buf_sz, "%s%s%s%s", prefix,
                 skipped, formatted_name, suffix);
}

/* Fallback for displaying fullscreen tags, in case there is not
 * enough plugin buffer space left to call the view_text plugin
 * from the Track Info screen
 */
static int view_text(const char *title, const char *text)
{
    splashf(0, "[%s]\n%s", title, text);
    action_userabort(TIMEOUT_BLOCK);
    return 0;
}

static enum pv_context_result show_track_info(const struct playlist_entry *current_track)
{
    bool id3_retrieval_successful = retrieve_id3_tags(current_track->index,
                                                      current_track->name,
                                                      viewer.id3, 0);

    return (id3_retrieval_successful &&
            browse_id3_ex(viewer.id3, viewer.playlist, current_track->display_index,
                          viewer.num_tracks, NULL, 1,
                          viewer.allow_view_text_plugin ? NULL : &view_text)) ?
           PV_CONTEXT_USB : PV_CONTEXT_UNCHANGED;
}

static void close_playlist_viewer(void)
{
    talk_shutup();
    if (viewer.playlist)
    {
        if (viewer.initial_selection)
            *(viewer.initial_selection) = viewer.selected_track;

        if(playlist_modified(viewer.playlist))
        {
            if (viewer.num_tracks && yesno_pop(ID2P(LANG_SAVE_CHANGES)))
                save_playlist_screen(viewer.playlist);
            else if (!viewer.num_tracks &&
                     confirm_delete_yesno(viewer.playlist->filename,
                                          viewer.title) == YESNO_YES)
            {
                remove(viewer.playlist->filename);
                reload_directory();
            }
        }
        playlist_close(viewer.playlist);
    }
    viewer.is_open = false;
}

#if defined(HAVE_HOTKEY) || defined(HAVE_TAGCACHE)
static enum pv_context_result
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
            return PV_CONTEXT_USB_CLOSED;
        case PLUGIN_GOTO_WPS:
            return PV_CONTEXT_WPS_CLOSED;
        default:
            return PV_CONTEXT_CLOSED;
    }
}

#ifdef HAVE_HOTKEY
static int list_viewers(const char* plugin, const char* file)
{
    /* dummy function to match prototype with filetype_load_plugin */
    (void)plugin;
    return filetype_list_viewers(file);
}
static enum pv_context_result open_with(const struct playlist_entry *current_track)
{
    return open_with_plugin(current_track, "", &list_viewers);
}
#endif /* HAVE_HOTKEY */

#ifdef HAVE_TAGCACHE
static enum pv_context_result open_pictureflow(const struct playlist_entry *current_track)
{
    return open_with_plugin(current_track, "pictureflow", &filetype_load_plugin);
}
#endif
#endif /*defined(HAVE_HOTKEY) || defined(HAVE_TAGCACHE)*/

static enum pv_context_result delete_track(int current_track_index,
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
    return PV_CONTEXT_MODIFIED;
}

static enum pv_context_result context_menu(int index)
{
    struct playlist_entry *current_track = playlist_buffer_get_track(&viewer.buffer,
                                                                     index);
    bool current_was_playing = (audio_status() & AUDIO_STATUS_PLAY) && /* or paused */
                               (current_track->index == viewer.current_playing_track);

    MENUITEM_STRINGLIST(menu_items, ID2P(LANG_PLAYLIST), NULL,
                        ID2P(LANG_PLAYING_NEXT), ID2P(LANG_ADD_TO_PL),
                        ID2P(LANG_REMOVE), ID2P(LANG_MOVE),
                        ID2P(LANG_MENU_SHOW_ID3_INFO),
                        ID2P(LANG_SHUFFLE), ID2P(LANG_SAVE),
                        ID2P(LANG_PLAYLISTVIEWER_SETTINGS)
#ifdef HAVE_TAGCACHE
                        ,ID2P(LANG_ONPLAY_PICTUREFLOW)
#endif
                        );
    int sel = do_menu(&menu_items, NULL, NULL, false);
    if (sel == MENU_ATTACHED_USB)
        return PV_CONTEXT_USB;
    else if (sel >= 0)
    {
        /* Abort current move */
        viewer.moving_track = -1;
        viewer.moving_playlist_index = -1;

        switch (sel)
        {
            case 0:
                /* Playing Next... menu */
                onplay_show_playlist_menu(current_track->name, FILE_ATTR_AUDIO, NULL);
                return PV_CONTEXT_UNCHANGED;
            case 1:
                /* Add to Playlist... menu */
                onplay_show_playlist_cat_menu(current_track->name, FILE_ATTR_AUDIO, NULL);
                return PV_CONTEXT_UNCHANGED;
            case 2:
                return delete_track(current_track->index, index, current_was_playing);
            case 3:
                /* move track */
                viewer.moving_track = index;
                viewer.moving_playlist_index = current_track->index;
                return PV_CONTEXT_UNCHANGED;
            case 4:
                return show_track_info(current_track);
            case 5:
                /* shuffle */
                if (!yesno_pop_confirm(ID2P(LANG_SHUFFLE)))
                    return PV_CONTEXT_UNCHANGED;
                playlist_sort(viewer.playlist, !viewer.playlist);
                playlist_randomise(viewer.playlist, current_tick, !viewer.playlist);
                viewer.selected_track = 0;
                return PV_CONTEXT_MODIFIED;
            case 6:
                save_playlist_screen(viewer.playlist);
                /* playlist indices of current playlist may have changed */
                return viewer.playlist ? PV_CONTEXT_UNCHANGED : PV_CONTEXT_PL_UPDATE;
            case 7:
            {
                /* playlist viewer settings */
                sel = global_settings.playlist_viewer_track_display;
                if (MENU_ATTACHED_USB == do_menu(&viewer_settings_menu, NULL, NULL, false))
                    return PV_CONTEXT_USB;
                else
                    return (sel == global_settings.playlist_viewer_track_display) ?
                           PV_CONTEXT_UNCHANGED : PV_CONTEXT_PL_UPDATE;
            }
#ifdef HAVE_TAGCACHE
            case 8:
                return open_pictureflow(current_track);
#endif
        }
    }
    return PV_CONTEXT_UNCHANGED;
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

static void update_gui(struct gui_synclist * playlist_lists, bool init)
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

static bool update_viewer(struct gui_synclist *playlist_lists, enum pv_context_result res)
{
    bool exit = false;
    if (res == PV_CONTEXT_MODIFIED)
        playlist_set_modified(viewer.playlist, true);

    if (res == PV_CONTEXT_MODIFIED || res == PV_CONTEXT_PL_UPDATE)
    {
        update_playlist(true);
        if (viewer.num_tracks <= 0)
            exit = true;

        if (viewer.selected_track >= viewer.num_tracks)
            viewer.selected_track = viewer.num_tracks-1;
    }
    update_gui(playlist_lists, false);
    return exit;
}

static bool open_playlist_viewer(const char* filename,
                                  struct gui_synclist *playlist_lists,
                                  bool reload, int *recent_selection)
{
    const char *dir = NULL, *file = NULL;
    char *sep = NULL;
    viewer.loading_tick = current_tick + HZ/3;

    /* Set viewer title */
    if (filename)
    {
        /* Separate directory from filename */
        sep = strrchr(filename + 1, '/');
        if (sep)
        {
            *sep = '\0';
            dir = filename;
            file = sep + 1;
        }
        else
        {
            dir = "/";
            file = filename + 1;
        }
        viewer.title = file;
    }
    else
        viewer.title = (char *) str(LANG_PLAYLIST);

    /* Prevent UI from feeling unresponsive when retrieving
       metadata, or on devices that use disk storage. */
    if (global_settings.playlist_viewer_track_display
        > PLAYLIST_VIEWER_ENTRY_SHOW_FULL_PATH
#ifdef HAVE_DISK_STORAGE
        || (!storage_disk_is_active()
#ifdef HAVE_DIRCACHE
            && (filename || !global_settings.dircache)
#endif /* HAVE DIRCACHE */
           )
#endif /* HAVE_DISK_STORAGE */
       )
    {
        push_activity_without_refresh(ACTIVITY_PLAYLISTVIEWER);
        clear_screen_buffer(false);
        FOR_NB_SCREENS(i)
            sb_set_title_text(viewer.title, Icon_Playlist, i);
        send_event(GUI_EVENT_ACTIONUPDATE, (void*) 1);
    }
    else
        push_current_activity(ACTIVITY_PLAYLISTVIEWER);

    viewer.is_open = playlist_viewer_init(&viewer, dir, file, reload, recent_selection);

    /* Merge separated dir and filename again */
    if (sep)
        *sep = '/';

    if (!viewer.is_open)
        return false;

    update_gui(playlist_lists, true);
    return true;
}

/* Main viewer function.  Filename identifies playlist to be viewed.  If NULL,
   view current playlist. */
enum playlist_viewer_result playlist_viewer_ex(const char* filename,
                                               int* recent_selection)
{
    enum playlist_viewer_result ret = PLAYLIST_VIEWER_OK;
    bool exit = false;        /* exit viewer */
    int button;
    struct gui_synclist playlist_lists;

    if (!open_playlist_viewer(filename, &playlist_lists, false, recent_selection))
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
                int ret_val, start_index = current_track->index;
                if (viewer.moving_track >= 0)
                {
                    /* Move track */
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
                else if (warn_on_pl_erase())
                {
                    /* Turn it into the current playlist */
                    ret_val = playlist_set_current(viewer.playlist);

                    /* Previously loaded playlist is now effectively closed */
                    viewer.playlist = NULL;

                    if (!ret_val)
                    {
                        if (global_settings.playlist_shuffle)
                            start_index = playlist_shuffle(current_tick, start_index);
                        playlist_start(start_index, 0, 0);

                        if (viewer.initial_selection)
                            *(viewer.initial_selection) = viewer.selected_track;
                    }
                    goto exit;
                }
                else
                    gui_synclist_set_title(&playlist_lists, playlist_lists.title,
                                           playlist_lists.title_icon);
                gui_synclist_draw(&playlist_lists);
                gui_synclist_speak_item(&playlist_lists);

                break;
            }
            case ACTION_STD_CONTEXT:
            {
                int pv_context_result = context_menu(viewer.selected_track);

                if (pv_context_result == PV_CONTEXT_USB)
                {
                    ret = PLAYLIST_VIEWER_USB;
                    goto exit;
                }
                else if (pv_context_result == PV_CONTEXT_USB_CLOSED)
                    return PLAYLIST_VIEWER_USB;
                else if (pv_context_result == PV_CONTEXT_WPS_CLOSED)
                    return PLAYLIST_VIEWER_OK;
                else if (pv_context_result == PV_CONTEXT_CLOSED)
                {
                    if (!open_playlist_viewer(filename, &playlist_lists, true, NULL))
                    {
                        ret = PLAYLIST_VIEWER_CANCEL;
                        goto exit;
                    }
                    break;
                }
                if (update_viewer(&playlist_lists, pv_context_result))
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
                        update_gui(&playlist_lists, true);
                    }
                    break;
#endif
#ifdef HAVE_HOTKEY
            case ACTION_TREE_HOTKEY:
            {
                struct playlist_entry *current_track = playlist_buffer_get_track(
                                                            &viewer.buffer,
                                                            viewer.selected_track);
                enum pv_context_result (*do_plugin)(const struct playlist_entry *) = NULL;
#ifdef HAVE_TAGCACHE
                if (global_settings.hotkey_tree == HOTKEY_PICTUREFLOW)
                    do_plugin = &open_pictureflow;
#endif
                if (global_settings.hotkey_tree == HOTKEY_OPEN_WITH)
                    do_plugin = &open_with;

                if (do_plugin != NULL)
                {
                    int plugin_result = do_plugin(current_track);

                    if (plugin_result == PV_CONTEXT_USB_CLOSED)
                        return PLAYLIST_VIEWER_USB;
                    else if (plugin_result == PV_CONTEXT_WPS_CLOSED)
                        return PLAYLIST_VIEWER_OK;
                    else if (!open_playlist_viewer(filename, &playlist_lists, true, NULL))
                    {
                        ret = PLAYLIST_VIEWER_CANCEL;
                        goto exit;
                    }
                }
                else if (global_settings.hotkey_tree == HOTKEY_PROPERTIES)
                {
                    if (show_track_info(current_track) == PV_CONTEXT_USB)
                    {
                        ret = PLAYLIST_VIEWER_USB;
                        goto exit;
                    }
                    update_gui(&playlist_lists, false);
                }
                else if (global_settings.hotkey_tree == HOTKEY_DELETE)
                {
                    if (update_viewer(&playlist_lists,
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

/* View current playlist */
enum playlist_viewer_result playlist_viewer(void)
{
    return playlist_viewer_ex(NULL, NULL);
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
    long talked_tick = 0;
    struct gui_synclist playlist_lists;
    struct playlist_track_info track;

    if (!playlist_viewer_init(&viewer, NULL, NULL, false, NULL))
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
            if (global_settings.talk_menu &&
                TIME_AFTER(current_tick, talked_tick + (HZ * 5)))
            {
                talked_tick = current_tick;
                talk_number(found_indicies_count, false);
                talk_id(LANG_PLAYLIST_SEARCH_MSG, true);
            }
            /* (voiced above) */
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

    cond_talk_ids_fq(LANG_ALL, TALK_ID(found_indicies_count, UNIT_INT),
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
