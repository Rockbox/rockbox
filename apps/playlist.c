/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
    Dynamic playlist design (based on design originally proposed by ricII)

    There are two files associated with a dynamic playlist:
    1. Playlist file : This file contains the initial songs in the playlist.
                       The file is created by the user and stored on the hard
                       drive.  NOTE: If we are playing the contents of a
                       directory, there will be no playlist file.
    2. Control file :  This file is automatically created when a playlist is
                       started and contains all the commands done to it.
    
    The first non-comment line in a control file must begin with
    "P:VERSION:DIR:FILE" where VERSION is the playlist control file version,
    DIR is the directory where the playlist is located and FILE is the
    playlist filename.  For dirplay, FILE will be empty.  An empty playlist
    will have both entries as null.

    Control file commands:
        a. Add track (A:<position>:<last position>:<path to track>)
            - Insert a track at the specified position in the current
              playlist.  Last position is used to specify where last insertion
              occurred.
        b. Queue track (Q:<position>:<last position>:<path to track>)
            - Queue a track at the specified position in the current
              playlist.  Queued tracks differ from added tracks in that they
              are deleted from the playlist as soon as they are played and
              they are not saved to disk as part of the playlist.
        c. Delete track (D:<position>)
            - Delete track from specified position in the current playlist.
        d. Shuffle playlist (S:<seed>:<index>)
            - Shuffle entire playlist with specified seed.  The index
              identifies the first index in the newly shuffled playlist
              (needed for repeat mode).
        e. Unshuffle playlist (U:<index>)
            - Unshuffle entire playlist.  The index identifies the first index
              in the newly unshuffled playlist.
        f. Reset last insert position (R)
            - Needed so that insertions work properly after resume

  Resume:
      The only resume info that needs to be saved is the current index in the
      playlist and the position in the track.  When resuming, all the commands
      in the control file will be reapplied so that the playlist indices are
      exactly the same as before shutdown.  To avoid unnecessary disk
      accesses, the shuffle mode settings are also saved in settings and only
      flushed to disk when required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playlist.h"
#include "file.h"
#include "dir.h"
#include "sprintf.h"
#include "debug.h"
#include "audio.h"
#include "lcd.h"
#include "kernel.h"
#include "settings.h"
#include "status.h"
#include "applimits.h"
#include "screens.h"
#include "buffer.h"
#include "atoi.h"
#include "misc.h"
#include "button.h"
#include "filetree.h"
#include "abrepeat.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

#include "lang.h"
#include "talk.h"

#define PLAYLIST_CONTROL_FILE ROCKBOX_DIR "/.playlist_control"
#define PLAYLIST_CONTROL_FILE_VERSION 2

/*
    Each playlist index has a flag associated with it which identifies what
    type of track it is.  These flags are stored in the 4 high order bits of
    the index.
    
    NOTE: This limits the playlist file size to a max of 256M.

    Bits 31-30:
        00 = Playlist track
        01 = Track was prepended into playlist
        10 = Track was inserted into playlist
        11 = Track was appended into playlist
    Bit 29:
        0 = Added track
        1 = Queued track
    Bit 28:
        0 = Track entry is valid
        1 = Track does not exist on disk and should be skipped
 */
#define PLAYLIST_SEEK_MASK              0x0FFFFFFF
#define PLAYLIST_INSERT_TYPE_MASK       0xC0000000
#define PLAYLIST_QUEUE_MASK             0x20000000

#define PLAYLIST_INSERT_TYPE_PREPEND    0x40000000
#define PLAYLIST_INSERT_TYPE_INSERT     0x80000000
#define PLAYLIST_INSERT_TYPE_APPEND     0xC0000000

#define PLAYLIST_QUEUED                 0x20000000
#define PLAYLIST_SKIPPED                0x10000000

#define PLAYLIST_DISPLAY_COUNT          10

static struct playlist_info current_playlist;
static char now_playing[MAX_PATH+1];

static void empty_playlist(struct playlist_info* playlist, bool resume);
static void new_playlist(struct playlist_info* playlist, const char *dir,
                         const char *file);
static void create_control(struct playlist_info* playlist);
static int  check_control(struct playlist_info* playlist);
static void update_playlist_filename(struct playlist_info* playlist,
                                     const char *dir, const char *file);
static int add_indices_to_playlist(struct playlist_info* playlist,
                                   char* buffer, int buflen);
static int add_track_to_playlist(struct playlist_info* playlist,
                                 const char *filename, int position,
                                 bool queue, int seek_pos);
static int add_directory_to_playlist(struct playlist_info* playlist,
                                     const char *dirname, int *position,
                                     bool queue, int *count, bool recurse);
static int remove_track_from_playlist(struct playlist_info* playlist,
                                      int position, bool write);
static int randomise_playlist(struct playlist_info* playlist,
                              unsigned int seed, bool start_current,
                              bool write);
static int sort_playlist(struct playlist_info* playlist, bool start_current,
                         bool write);
static int get_next_index(const struct playlist_info* playlist, int steps,
                          int repeat_mode);
static void find_and_set_playlist_index(struct playlist_info* playlist,
                                        unsigned int seek);
static int compare(const void* p1, const void* p2);
static int get_filename(struct playlist_info* playlist, int seek,
                        bool control_file, char *buf, int buf_length);
static int get_next_directory(char *dir);
static int get_next_dir(char *dir, bool is_forward, bool recursion);
static int get_previous_directory(char *dir);
static int check_subdir_for_music(char *dir, char *subdir);
static int format_track_path(char *dest, char *src, int buf_length, int max,
                             char *dir);
static void display_playlist_count(int count, const char *fmt);
static void display_buffer_full(void);
static int flush_pending_control(struct playlist_info* playlist);
static int rotate_index(const struct playlist_info* playlist, int index);

/*
 * remove any files and indices associated with the playlist
 */
static void empty_playlist(struct playlist_info* playlist, bool resume)
{
    playlist->filename[0] = '\0';

    if(playlist->fd >= 0)
        /* If there is an already open playlist, close it. */
        close(playlist->fd);
    playlist->fd = -1;

    if(playlist->control_fd >= 0)
        close(playlist->control_fd);
    playlist->control_fd = -1;
    playlist->control_created = false;

    playlist->in_ram = false;

    if (playlist->buffer)
        playlist->buffer[0] = 0;

    playlist->buffer_end_pos = 0;

    playlist->index = 0;
    playlist->first_index = 0;
    playlist->amount = 0;
    playlist->last_insert_pos = -1;
    playlist->seed = 0;
    playlist->shuffle_modified = false;
    playlist->deleted = false;
    playlist->num_inserted_tracks = 0;
    playlist->shuffle_flush = false;

    if (!resume && playlist->current)
    {
        /* start with fresh playlist control file when starting new
           playlist */
        create_control(playlist);

        /* Reset resume settings */
        global_settings.resume_first_index = 0;
        global_settings.resume_seed = -1;
    }
}

/*
 * Initialize a new playlist for viewing/editing/playing.  dir is the
 * directory where the playlist is located and file is the filename.
 */
static void new_playlist(struct playlist_info* playlist, const char *dir,
                         const char *file)
{
    empty_playlist(playlist, false);

    if (!file)
    {
        file = "";

        if (dir && playlist->current) /* !current cannot be in_ram */
            playlist->in_ram = true;
        else
            dir = ""; /* empty playlist */
    }
    
    update_playlist_filename(playlist, dir, file);

    if (playlist->control_fd >= 0)
    {
        if (fdprintf(playlist->control_fd, "P:%d:%s:%s\n",
            PLAYLIST_CONTROL_FILE_VERSION, dir, file) > 0)
            fsync(playlist->control_fd);
        else
            splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
    }
}

/*
 * create control file for playlist
 */
static void create_control(struct playlist_info* playlist)
{
    playlist->control_fd = open(playlist->control_filename,
                                O_CREAT|O_RDWR|O_TRUNC);
    if (playlist->control_fd < 0)
    {
        if (check_rockboxdir())
        {
            splash(HZ*2, true, "%s (%d)",
                str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR),
                playlist->control_fd);
        }
        playlist->control_created = false;
    }
    else
    {
        playlist->control_created = true;
    }
}

/*
 * validate the control file.  This may include creating/initializing it if
 * necessary;
 */
static int check_control(struct playlist_info* playlist)
{
    if (!playlist->control_created)
    {
        create_control(playlist);

        if (playlist->control_fd >= 0)
        {
            char* dir = playlist->filename;
            char* file = playlist->filename+playlist->dirlen;
            char c = playlist->filename[playlist->dirlen-1];

            playlist->filename[playlist->dirlen-1] = '\0';

            if (fdprintf(playlist->control_fd, "P:%d:%s:%s\n",
                PLAYLIST_CONTROL_FILE_VERSION, dir, file) > 0)
                fsync(playlist->control_fd);
            else
                splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));

            playlist->filename[playlist->dirlen-1] = c;
        }
    }

    if (playlist->control_fd < 0)
        return -1;

    return 0;
}

/*
 * store directory and name of playlist file
 */
static void update_playlist_filename(struct playlist_info* playlist,
                                     const char *dir, const char *file)
{
    char *sep="";
    int dirlen = strlen(dir);
    
    /* If the dir does not end in trailing slash, we use a separator.
       Otherwise we don't. */
    if('/' != dir[dirlen-1])
    {
        sep="/";
        dirlen++;
    }

    playlist->dirlen = dirlen;
    
    snprintf(playlist->filename, sizeof(playlist->filename),
        "%s%s%s", dir, sep, file);
}

/*
 * calculate track offsets within a playlist file
 */
static int add_indices_to_playlist(struct playlist_info* playlist,
                                   char* buffer, int buflen)
{
    unsigned int nread;
    unsigned int i = 0;
    unsigned int count = 0;
    bool store_index;
    unsigned char *p;

    if(-1 == playlist->fd)
        playlist->fd = open(playlist->filename, O_RDONLY);
    if(playlist->fd < 0)
        return -1; /* failure */
    
#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    splash(0, true, str(LANG_PLAYLIST_LOAD));

    if (!buffer)
    {
        /* use mp3 buffer for maximum load speed */
        audio_stop();
        talk_buffer_steal(); /* we use the mp3 buffer, need to tell */

        buffer = audiobuf;
        buflen = (audiobufend - audiobuf);
    }
    
    store_index = true;

    while(1)
    {
        nread = read(playlist->fd, buffer, buflen);
        /* Terminate on EOF */
        if(nread <= 0)
            break;
        
        p = buffer;

        for(count=0; count < nread; count++,p++) {

            /* Are we on a new line? */
            if((*p == '\n') || (*p == '\r'))
            {
                store_index = true;
            }
            else if(store_index)
            {
                store_index = false;

                if(*p != '#')
                {
                    /* Store a new entry */
                    playlist->indices[ playlist->amount ] = i+count;
                    playlist->amount++;
                    if ( playlist->amount >= playlist->max_playlist_size ) {
                        display_buffer_full();
                        return -1;
                    }
                }
            }
        }

        i+= count;
    }

    return 0;
}

/*
 * Add track to playlist at specified position.  There are five special
 * positions that can be specified:
 *     PLAYLIST_PREPEND         - Add track at beginning of playlist
 *     PLAYLIST_INSERT          - Add track after current song.  NOTE: If
 *                                there are already inserted tracks then track
 *                                is added to the end of the insertion list
 *     PLAYLIST_INSERT_FIRST    - Add track immediately after current song, no
 *                                matter what other tracks have been inserted
 *     PLAYLIST_INSERT_LAST     - Add track to end of playlist
 *     PLAYLIST_INSERT_SHUFFLED - Add track at some random point between the
 *                                current playing track and end of playlist
 */
static int add_track_to_playlist(struct playlist_info* playlist,
                                 const char *filename, int position,
                                 bool queue, int seek_pos)
{
    int insert_position = position;
    unsigned long flags = PLAYLIST_INSERT_TYPE_INSERT;
    int i;

    if (playlist->amount >= playlist->max_playlist_size)
    {
        display_buffer_full();
        return -1;
    }

    switch (position)
    {
        case PLAYLIST_PREPEND:
            insert_position = playlist->first_index;
            flags = PLAYLIST_INSERT_TYPE_PREPEND;
            break;
        case PLAYLIST_INSERT:
            /* if there are already inserted tracks then add track to end of
               insertion list else add after current playing track */
            if (playlist->last_insert_pos >= 0 &&
                playlist->last_insert_pos < playlist->amount &&
                (playlist->indices[playlist->last_insert_pos]&
                    PLAYLIST_INSERT_TYPE_MASK) == PLAYLIST_INSERT_TYPE_INSERT)
                position = insert_position = playlist->last_insert_pos+1;
            else if (playlist->amount > 0)
                position = insert_position = playlist->index + 1;
            else
                position = insert_position = 0;

            playlist->last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_FIRST:
            if (playlist->amount > 0)
                position = insert_position = playlist->index + 1;
            else
                position = insert_position = 0;

            if (playlist->last_insert_pos < 0)
                playlist->last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_LAST:
            if (playlist->first_index > 0)
                insert_position = playlist->first_index;
            else
                insert_position = playlist->amount;

            flags = PLAYLIST_INSERT_TYPE_APPEND;
            break;
        case PLAYLIST_INSERT_SHUFFLED:
        {
            int offset;
            int n = playlist->amount -
                rotate_index(playlist, playlist->index);
            
            if (n > 0)
                offset = rand() % n;
            else
                offset = 0;

            position = playlist->index + offset + 1;
            if (position >= playlist->amount)
                position -= playlist->amount;

            insert_position = position;
            break;
        }
    }
    
    if (queue)
        flags |= PLAYLIST_QUEUED;

    /* shift indices so that track can be added */
    for (i=playlist->amount; i>insert_position; i--)
        playlist->indices[i] = playlist->indices[i-1];

    /* update stored indices if needed */
    if (playlist->amount > 0 && insert_position <= playlist->index)
        playlist->index++;

    if (playlist->amount > 0 && insert_position <= playlist->first_index &&
        position != PLAYLIST_PREPEND)
    {
        playlist->first_index++;

        if (seek_pos < 0 && playlist->current)
        {
            global_settings.resume_first_index = playlist->first_index;
            settings_save();
        }
    }

    if (insert_position < playlist->last_insert_pos ||
        (insert_position == playlist->last_insert_pos && position < 0))
        playlist->last_insert_pos++;

    if (seek_pos < 0 && playlist->control_fd >= 0)
    {
        int result = -1;

        if (flush_pending_control(playlist) < 0)
            return -1;

        mutex_lock(&playlist->control_mutex);

        if (lseek(playlist->control_fd, 0, SEEK_END) >= 0)
        {
            if (fdprintf(playlist->control_fd, "%c:%d:%d:", (queue?'Q':'A'),
                    position, playlist->last_insert_pos) > 0)
            {
                /* save the position in file where track name is written */
                seek_pos = lseek(playlist->control_fd, 0, SEEK_CUR);

                if (fdprintf(playlist->control_fd, "%s\n", filename) > 0)
                    result = 0;
            }
        }

        mutex_unlock(&playlist->control_mutex);

        if (result < 0)
        {
            splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
            return result;
        }
    }

    playlist->indices[insert_position] = flags | seek_pos;

    playlist->amount++;
    playlist->num_inserted_tracks++;

    return insert_position;
}

/*
 * Insert directory into playlist.  May be called recursively.
 */
static int add_directory_to_playlist(struct playlist_info* playlist,
                                     const char *dirname, int *position,
                                     bool queue, int *count, bool recurse)
{
    char buf[MAX_PATH+1];
    char *count_str;
    int result = 0;
    int num_files = 0;
    int i;
    int dirfilter = global_settings.dirfilter;
    struct entry *files;
    struct tree_context* tc = tree_get_context();

    /* use the tree browser dircache to load files */
    global_settings.dirfilter = SHOW_ALL;

    if (ft_load(tc, dirname) < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
        global_settings.dirfilter = dirfilter;
        return -1;
    }

    files = (struct entry*) tc->dircache;
    num_files = tc->filesindir;

    /* we've overwritten the dircache so tree browser will need to be
       reloaded */
    reload_directory();

    if (queue)
        count_str = str(LANG_PLAYLIST_QUEUE_COUNT);
    else
        count_str = str(LANG_PLAYLIST_INSERT_COUNT);

    for (i=0; i<num_files; i++)
    {
        /* user abort */
        if (button_get(false) == SETTINGS_CANCEL)
        {
            result = -1;
            break;
        }

        if (files[i].attr & ATTR_DIRECTORY)
        {
            if (recurse)
            {
                /* recursively add directories */
                snprintf(buf, sizeof(buf), "%s/%s", dirname, files[i].name);
                result = add_directory_to_playlist(playlist, buf, position,
                    queue, count, recurse);
                if (result < 0)
                    break;

                /* we now need to reload our current directory */
                if(ft_load(tc, dirname) < 0)
                {
                    result = -1;
                    break;
                }
                    
                files = (struct entry*) tc->dircache;
                num_files = tc->filesindir;
                if (!num_files)
                {
                    result = -1;
                    break;
                }
            }
            else
                continue;
        }
        else if ((files[i].attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
        {
            int insert_pos;

            snprintf(buf, sizeof(buf), "%s/%s", dirname, files[i].name);
            
            insert_pos = add_track_to_playlist(playlist, buf, *position,
                queue, -1);
            if (insert_pos < 0)
            {
                result = -1;
                break;
            }

            (*count)++;

            /* Make sure tracks are inserted in correct order if user requests
               INSERT_FIRST */
            if (*position == PLAYLIST_INSERT_FIRST || *position >= 0)
                *position = insert_pos + 1;

            if ((*count%PLAYLIST_DISPLAY_COUNT) == 0)
            {
                display_playlist_count(*count, count_str);

                if (*count == PLAYLIST_DISPLAY_COUNT &&
                    (audio_status() & AUDIO_STATUS_PLAY))
                    audio_flush_and_reload_tracks();
            }
            
            /* let the other threads work */
            yield();
        }
    }

    /* restore dirfilter */
    global_settings.dirfilter = dirfilter;

    return result;
}

/*
 * remove track at specified position
 */
static int remove_track_from_playlist(struct playlist_info* playlist,
                                      int position, bool write)
{
    int i;
    bool inserted;

    if (playlist->amount <= 0)
        return -1;

    inserted = playlist->indices[position] & PLAYLIST_INSERT_TYPE_MASK;

    /* shift indices now that track has been removed */
    for (i=position; i<playlist->amount; i++)
        playlist->indices[i] = playlist->indices[i+1];

    playlist->amount--;

    if (inserted)
        playlist->num_inserted_tracks--;
    else
        playlist->deleted = true;

    /* update stored indices if needed */
    if (position < playlist->index)
        playlist->index--;

    if (position < playlist->first_index)
    {
        playlist->first_index--;

        if (write)
        {
            global_settings.resume_first_index = playlist->first_index;
            settings_save();
        }
    }

    if (position <= playlist->last_insert_pos)
        playlist->last_insert_pos--;

    if (write && playlist->control_fd >= 0)
    {
        int result = -1;

        if (flush_pending_control(playlist) < 0)
            return -1;

        mutex_lock(&playlist->control_mutex);

        if (lseek(playlist->control_fd, 0, SEEK_END) >= 0)
        {
            if (fdprintf(playlist->control_fd, "D:%d\n", position) > 0)
            {
                fsync(playlist->control_fd);
                result = 0;
            }
        }

        mutex_unlock(&playlist->control_mutex);

        if (result < 0)
        {
            splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
            return result;
        }
    }

    return 0;
}

/*
 * randomly rearrange the array of indices for the playlist.  If start_current
 * is true then update the index to the new index of the current playing track
 */
static int randomise_playlist(struct playlist_info* playlist,
                              unsigned int seed, bool start_current,
                              bool write)
{
    int count;
    int candidate;
    int store;
    unsigned int current = playlist->indices[playlist->index];
    
    /* seed 0 is used to identify sorted playlist for resume purposes */
    if (seed == 0)
        seed = 1;

    /* seed with the given seed */
    srand(seed);

    /* randomise entire indices list */
    for(count = playlist->amount - 1; count >= 0; count--)
    {
        /* the rand is from 0 to RAND_MAX, so adjust to our value range */
        candidate = rand() % (count + 1);

        /* now swap the values at the 'count' and 'candidate' positions */
        store = playlist->indices[candidate];
        playlist->indices[candidate] = playlist->indices[count];
        playlist->indices[count] = store;
    }

    if (start_current)
        find_and_set_playlist_index(playlist, current);

    /* indices have been moved so last insert position is no longer valid */
    playlist->last_insert_pos = -1;

    playlist->seed = seed;
    if (playlist->num_inserted_tracks > 0 || playlist->deleted)
        playlist->shuffle_modified = true;

    if (write)
    {
        /* Don't write to disk immediately.  Instead, save in settings and
           only flush if playlist is modified (insertion/deletion) */
        playlist->shuffle_flush = true;
        global_settings.resume_seed = seed;
        settings_save();
    }

    return 0;
}

/*
 * Sort the array of indices for the playlist. If start_current is true then
 * set the index to the new index of the current song.
 */
static int sort_playlist(struct playlist_info* playlist, bool start_current,
                         bool write)
{
    unsigned int current = playlist->indices[playlist->index];

    if (playlist->amount > 0)
        qsort(playlist->indices, playlist->amount,
            sizeof(playlist->indices[0]), compare);

    if (start_current)
        find_and_set_playlist_index(playlist, current);

    /* indices have been moved so last insert position is no longer valid */
    playlist->last_insert_pos = -1;

    if (!playlist->num_inserted_tracks && !playlist->deleted)
        playlist->shuffle_modified = false;
    if (write && playlist->control_fd >= 0)
    {
        /* Don't write to disk immediately.  Instead, save in settings and
           only flush if playlist is modified (insertion/deletion) */
        playlist->shuffle_flush = true;
        global_settings.resume_seed = 0;
        settings_save();
    }

    return 0;
}

/* Marks the index of the track to be skipped that is "steps" away from
 * current playing track.
 */
void playlist_skip_entry(struct playlist_info *playlist, int steps)
{
    int index;

    if (playlist == NULL)
        playlist = &current_playlist;
    
    index = rotate_index(playlist, playlist->index);
    index += steps;
    if (index < 0 || index >= playlist->amount)
        return ;
    
    index = (index+playlist->first_index) % playlist->amount;
    playlist->indices[index] |= PLAYLIST_SKIPPED;
}

/* Calculate how many steps we have to really step when skipping entries
 * marked as bad.
 */
static int calculate_step_count(const struct playlist_info *playlist, int steps)
{
    int i, count, direction;
    int index;
    int stepped_count = 0;
    
    if (steps < 0)
    {
        direction = -1;
        count = -steps;
    }
    else
    {
        direction = 1;
        count = steps;
    }

    index = playlist->index;
    i = 0;
    while (i < count)
    {
        index += direction;
        /* Boundary check */
        if (index < 0)
            index += playlist->amount;
        if (index >= playlist->amount)
            index -= playlist->amount;

        /* Check if we found a bad entry. */
        if (playlist->indices[index] & PLAYLIST_SKIPPED)
        {
            steps += direction;
            /* Are all entries bad? */
            if (stepped_count++ > playlist->amount)
                break ;
        }
        else
            i++;
    }

    return steps;
}


/*
 * returns the index of the track that is "steps" away from current playing
 * track.
 */
static int get_next_index(const struct playlist_info* playlist, int steps,
                          int repeat_mode)
{
    int current_index = playlist->index;
    int next_index    = -1;

    if (playlist->amount <= 0)
        return -1;

    if (repeat_mode == -1)
        repeat_mode = global_settings.repeat_mode;

    if (repeat_mode == REPEAT_SHUFFLE &&
        (!global_settings.playlist_shuffle || playlist->amount <= 1))
        repeat_mode = REPEAT_ALL;

    steps = calculate_step_count(playlist, steps);
    switch (repeat_mode)
    {
        case REPEAT_SHUFFLE:
            /* Treat repeat shuffle just like repeat off.  At end of playlist,
               play will be resumed in playlist_next() */
        case REPEAT_OFF:
        {
            current_index = rotate_index(playlist, current_index);
            next_index = current_index+steps;
            if ((next_index < 0) || (next_index >= playlist->amount))
                next_index = -1;
            else
                next_index = (next_index+playlist->first_index) %
                    playlist->amount;

            break;
        }

        case REPEAT_ONE:
#ifdef AB_REPEAT_ENABLE
        case REPEAT_AB:
#endif
            next_index = current_index;
            break;

        case REPEAT_ALL:
        default:
        {
            next_index = (current_index+steps) % playlist->amount;
            while (next_index < 0)
                next_index += playlist->amount;

            if (steps >= playlist->amount)
            {
                int i, index;

                index = next_index;
                next_index = -1;

                /* second time around so skip the queued files */
                for (i=0; i<playlist->amount; i++)
                {
                    if (playlist->indices[index] & PLAYLIST_QUEUE_MASK)
                        index = (index+1) % playlist->amount;
                    else
                    {
                        next_index = index;
                        break;
                    }
                }
            }
            break;
        }
    }

    /* No luck if the whole playlist was bad. */
    if (playlist->indices[next_index] & PLAYLIST_SKIPPED)
        return -1;
    
    return next_index;
}

/*
 * Search for the seek track and set appropriate indices.  Used after shuffle
 * to make sure the current index is still pointing to correct track.
 */
static void find_and_set_playlist_index(struct playlist_info* playlist,
                                        unsigned int seek)
{
    int i;
    
    /* Set the index to the current song */
    for (i=0; i<playlist->amount; i++)
    {
        if (playlist->indices[i] == seek)
        {
            playlist->index = playlist->first_index = i;

            if (playlist->current)
            {
                global_settings.resume_first_index = i;
                settings_save();
            }

            break;
        }
    }
}

/*
 * used to sort track indices.  Sort order is as follows:
 * 1. Prepended tracks (in prepend order)
 * 2. Playlist/directory tracks (in playlist order)
 * 3. Inserted/Appended tracks (in insert order)
 */
static int compare(const void* p1, const void* p2)
{
    unsigned long* e1 = (unsigned long*) p1;
    unsigned long* e2 = (unsigned long*) p2;
    unsigned long flags1 = *e1 & PLAYLIST_INSERT_TYPE_MASK;
    unsigned long flags2 = *e2 & PLAYLIST_INSERT_TYPE_MASK;

    if (flags1 == flags2)
        return (*e1 & PLAYLIST_SEEK_MASK) - (*e2 & PLAYLIST_SEEK_MASK);
    else if (flags1 == PLAYLIST_INSERT_TYPE_PREPEND ||
        flags2 == PLAYLIST_INSERT_TYPE_APPEND)
        return -1;
    else if (flags1 == PLAYLIST_INSERT_TYPE_APPEND ||
        flags2 == PLAYLIST_INSERT_TYPE_PREPEND)
        return 1;
    else if (flags1 && flags2)
        return (*e1 & PLAYLIST_SEEK_MASK) - (*e2 & PLAYLIST_SEEK_MASK);
    else
        return *e1 - *e2;
}

/*
 * gets pathname for track at seek index
 */
static int get_filename(struct playlist_info* playlist, int seek,
                        bool control_file, char *buf, int buf_length)
{
    int fd;
    int max = -1;
    char tmp_buf[MAX_PATH+1];
    char dir_buf[MAX_PATH+1];

    if (buf_length > MAX_PATH+1)
        buf_length = MAX_PATH+1;

    if (playlist->in_ram && !control_file)
    {
        strncpy(tmp_buf, &playlist->buffer[seek], sizeof(tmp_buf));
        tmp_buf[MAX_PATH] = '\0';
        max = strlen(tmp_buf) + 1;
    }
    else
    {
        if (control_file)
            fd = playlist->control_fd;
        else
        {
            if(-1 == playlist->fd)
                playlist->fd = open(playlist->filename, O_RDONLY);
            
            fd = playlist->fd;
        }
        
        if(-1 != fd)
        {
            if (control_file)
                mutex_lock(&playlist->control_mutex);
            
            if (lseek(fd, seek, SEEK_SET) != seek)
                max = -1;
            else
                max = read(fd, tmp_buf, buf_length);
            
            if (control_file)
                mutex_unlock(&playlist->control_mutex);
        }

        if (max < 0)
        {
            if (control_file)
                splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
            else
                splash(HZ*2, true, str(LANG_PLAYLIST_ACCESS_ERROR));

            return max;
        }
    }

    strncpy(dir_buf, playlist->filename, playlist->dirlen-1);
    dir_buf[playlist->dirlen-1] = 0;

    return (format_track_path(buf, tmp_buf, buf_length, max, dir_buf));
}

static int get_next_directory(char *dir){
  return get_next_dir(dir,true,false);
}

static int get_previous_directory(char *dir){
  return get_next_dir(dir,false,false);
}

/*
 * search through all the directories (starting with the current) to find
 * one that has tracks to play
 */
static int get_next_dir(char *dir, bool is_forward, bool recursion)
{
    struct playlist_info* playlist = &current_playlist;
    int result = -1;
    int dirfilter = global_settings.dirfilter;
    int sort_dir = global_settings.sort_dir;
    char *start_dir = NULL;
    bool exit = false;
    struct tree_context* tc = tree_get_context();

    if (recursion){
       /* start with root */
       dir[0] = '\0';
    }
    else{
        /* start with current directory */
        strncpy(dir, playlist->filename, playlist->dirlen-1);
        dir[playlist->dirlen-1] = '\0';
    }

    /* use the tree browser dircache to load files */
    global_settings.dirfilter = SHOW_ALL;

    /* sort in another direction if previous dir is requested */
    if(!is_forward){
       if ((global_settings.sort_dir == 0) || (global_settings.sort_dir == 3))
           global_settings.sort_dir = 4;
       else if (global_settings.sort_dir == 1)
           global_settings.sort_dir = 2;
       else if (global_settings.sort_dir == 2)
           global_settings.sort_dir = 1;
       else if (global_settings.sort_dir == 4)
           global_settings.sort_dir = 0;
    }

    while (!exit)
    {
        struct entry *files;
        int num_files = 0;
        int i;

        if (ft_load(tc, (dir[0]=='\0')?"/":dir) < 0)
        {
            splash(HZ*2, true, str(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
            exit = true;
            result = -1;
            break;
        }
        
        files = (struct entry*) tc->dircache;
        num_files = tc->filesindir;

        for (i=0; i<num_files; i++)
        {
            /* user abort */
            if (button_get(false) == SETTINGS_CANCEL)
            {
                result = -1;
                exit = true;
                break;
            }
            
            if (files[i].attr & ATTR_DIRECTORY)
            {
                if (!start_dir)
                {
                    result = check_subdir_for_music(dir, files[i].name);
                    if (result != -1)
                    {
                        exit = true;
                        break;
                    }
                }
                else if (!strcmp(start_dir, files[i].name))
                    start_dir = NULL;
            }
        }

        if (!exit)
        {
            /* move down to parent directory.  current directory name is
               stored as the starting point for the search in parent */
            start_dir = strrchr(dir, '/');
            if (start_dir)
            {
                *start_dir = '\0';
                start_dir++;
            }
            else
                break;
        }
    }

    /* we've overwritten the dircache so tree browser will need to be
       reloaded */
    reload_directory();

    /* restore dirfilter & sort_dir */
    global_settings.dirfilter = dirfilter;
    global_settings.sort_dir = sort_dir;

    /* special case if nothing found: try start searching again from root */
    if (result == -1 && !recursion){
        result = get_next_dir(dir,is_forward, true);
    }

    return result;
}

/*
 * Checks if there are any music files in the dir or any of its
 * subdirectories.  May be called recursively.
 */
static int check_subdir_for_music(char *dir, char *subdir)
{
    int result = -1;
    int dirlen = strlen(dir);
    int num_files = 0;
    int i;
    struct entry *files;
    bool has_music = false;
    bool has_subdir = false;
    struct tree_context* tc = tree_get_context();

    snprintf(dir+dirlen, MAX_PATH-dirlen, "/%s", subdir);
    
    if (ft_load(tc, dir) < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
        return -2;
    }
    
    files = (struct entry*) tc->dircache;
    num_files = tc->filesindir;
    
    for (i=0; i<num_files; i++)
    {
        if (files[i].attr & ATTR_DIRECTORY)
            has_subdir = true;
        else if ((files[i].attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
        {
            has_music = true;
            break;
        }
    }

    if (has_music)
        return 0;

    if (has_subdir)
    {
        for (i=0; i<num_files; i++)
        {
            if (button_get(false) == SETTINGS_CANCEL)
            {
                result = -2;
                break;
            }

            if (files[i].attr & ATTR_DIRECTORY)
            {
                result = check_subdir_for_music(dir, files[i].name);
                if (!result)
                    break;
            }
        }
    }

    if (result < 0)
    {
        if (dirlen)
        {
            dir[dirlen] = '\0';
        }
        else
        {
            strcpy(dir, "/");
        }

        /* we now need to reload our current directory */
        if(ft_load(tc, dir) < 0)
            splash(HZ*2, true,
                str(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));        
    }

    return result;
}

/*
 * Returns absolute path of track
 */
static int format_track_path(char *dest, char *src, int buf_length, int max,
                             char *dir)
{
    int i = 0;
    int j;
    char *temp_ptr;

    /* Zero-terminate the file name */
    while((src[i] != '\n') &&
          (src[i] != '\r') &&
          (i < max))
        i++;

    /* Now work back killing white space */
    while((src[i-1] == ' ') ||
          (src[i-1] == '\t'))
        i--;

    src[i]=0;
      
    /* replace backslashes with forward slashes */
    for ( j=0; j<i; j++ )
        if ( src[j] == '\\' )
            src[j] = '/';

    if('/' == src[0])
    {
        strncpy(dest, src, buf_length);
    }
    else
    {
        /* handle dos style drive letter */
        if (':' == src[1])
            strncpy(dest, &src[2], buf_length);
        else if (!strncmp(src, "../", 3))
        {
            /* handle relative paths */
            i=3;
            while(!strncmp(&src[i], "../", 3))
                i += 3;
            for (j=0; j<i/3; j++) {
                temp_ptr = strrchr(dir, '/');
                if (temp_ptr)
                    *temp_ptr = '\0';
                else
                    break;
            }
            snprintf(dest, buf_length, "%s/%s", dir, &src[i]);
        }
        else if ( '.' == src[0] && '/' == src[1] ) {
            snprintf(dest, buf_length, "%s/%s", dir, &src[2]);
        }
        else {
            snprintf(dest, buf_length, "%s/%s", dir, src);
        }
    }

    return 0;
}

/*
 * Display splash message showing progress of playlist/directory insertion or
 * save.
 */
static void display_playlist_count(int count, const char *fmt)
{
    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    splash(0, true, fmt, count,
#if CONFIG_KEYPAD == PLAYER_PAD
           str(LANG_STOP_ABORT)
#else
           str(LANG_OFF_ABORT)
#endif
        );
}

/*
 * Display buffer full message
 */
static void display_buffer_full(void)
{
    splash(HZ*2, true, "%s %s",
           str(LANG_PLAYINDICES_PLAYLIST),
           str(LANG_PLAYINDICES_BUFFER));
}

/*
 * Flush any pending control commands to disk.  Called when playlist is being
 * modified.  Returns 0 on success and -1 on failure.
 */
static int flush_pending_control(struct playlist_info* playlist)
{
    int result = 0;
        
    if (playlist->shuffle_flush && global_settings.resume_seed >= 0)
    {
        /* pending shuffle */
        mutex_lock(&playlist->control_mutex);
        
        if (lseek(playlist->control_fd, 0, SEEK_END) >= 0)
        {
            if (global_settings.resume_seed == 0)
                result = fdprintf(playlist->control_fd, "U:%d\n",
                    playlist->first_index);
            else
                result = fdprintf(playlist->control_fd, "S:%d:%d\n",
                    global_settings.resume_seed, playlist->first_index);

            if (result > 0)
            {
                fsync(playlist->control_fd);

                playlist->shuffle_flush = false;
                global_settings.resume_seed = -1;
                settings_save();

                result = 0;
            }
            else
                result = -1;
        }
        else
            result = -1;
        
        mutex_unlock(&playlist->control_mutex);
        
        if (result < 0)
        {
            splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
            return result;
        }
    }

    return result;
}

/*
 * Rotate indices such that first_index is index 0
 */
static int rotate_index(const struct playlist_info* playlist, int index)
{
    index -= playlist->first_index;
    if (index < 0)
        index += playlist->amount;

    return index;
}

/*
 * Initialize playlist entries at startup
 */
void playlist_init(void)
{
    struct playlist_info* playlist = &current_playlist;

    playlist->current = true;
    snprintf(playlist->control_filename, sizeof(playlist->control_filename),
        "%s", PLAYLIST_CONTROL_FILE);
    playlist->fd = -1;
    playlist->control_fd = -1;
    playlist->max_playlist_size = global_settings.max_files_in_playlist;
    playlist->indices = buffer_alloc(
        playlist->max_playlist_size * sizeof(int));
    playlist->buffer_size =
        AVERAGE_FILENAME_LENGTH * global_settings.max_files_in_dir;
    playlist->buffer = buffer_alloc(playlist->buffer_size);
    mutex_init(&playlist->control_mutex);
    empty_playlist(playlist, true);
}

/*
 * Create new playlist
 */
int playlist_create(const char *dir, const char *file)
{
    struct playlist_info* playlist = &current_playlist;

    new_playlist(playlist, dir, file);

    if (file)
        /* load the playlist file */
        add_indices_to_playlist(playlist, NULL, 0);

    return 0;
}

#define PLAYLIST_COMMAND_SIZE (MAX_PATH+12)

/*
 * Restore the playlist state based on control file commands.  Called to
 * resume playback after shutdown.
 */
int playlist_resume(void)
{
    struct playlist_info* playlist = &current_playlist;
    char *buffer;
    int buflen;
    int nread;
    int total_read = 0;
    int control_file_size = 0;
    bool first = true;
    bool sorted = true;

    enum {
        resume_playlist,
        resume_add,
        resume_queue,
        resume_delete,
        resume_shuffle,
        resume_unshuffle,
        resume_reset,
        resume_comment
    };

    /* use mp3 buffer for maximum load speed */
#if CONFIG_CODEC != SWCODEC
    talk_buffer_steal(); /* we use the mp3 buffer, need to tell */
    buflen = (audiobufend - audiobuf);
    buffer = audiobuf;
#else
    buflen = (audiobufend - audiobuf - talk_get_bufsize());
    buffer = &audiobuf[talk_get_bufsize()];
#endif

    empty_playlist(playlist, true);

    playlist->control_fd = open(playlist->control_filename, O_RDWR);
    if (playlist->control_fd < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }
    playlist->control_created = true;

    control_file_size = filesize(playlist->control_fd);
    if (control_file_size <= 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    /* read a small amount first to get the header */
    nread = read(playlist->control_fd, buffer,
        PLAYLIST_COMMAND_SIZE<buflen?PLAYLIST_COMMAND_SIZE:buflen);
    if(nread <= 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    while (1)
    {
        int result = 0;
        int count;
        int current_command = resume_comment;
        int last_newline = 0;
        int str_count = -1;
        bool newline = true;
        bool exit_loop = false;
        char *p = buffer;
        char *str1 = NULL;
        char *str2 = NULL;
        char *str3 = NULL;

        for(count=0; count<nread && !exit_loop; count++,p++)
        {
            /* Are we on a new line? */
            if((*p == '\n') || (*p == '\r'))
            {
                *p = '\0';

                /* save last_newline in case we need to load more data */
                last_newline = count;

                switch (current_command)
                {
                    case resume_playlist:
                    {
                        /* str1=version str2=dir str3=file */
                        int version;

                        if (!str1)
                        {
                            result = -1;
                            exit_loop = true;
                            break;
                        }
                        
                        if (!str2)
                            str2 = "";
                        
                        if (!str3)
                            str3 = "";
                        
                        version = atoi(str1);
                        
                        if (version != PLAYLIST_CONTROL_FILE_VERSION)
                            return -1;
                        
                        update_playlist_filename(playlist, str2, str3);
                        
                        if (str3[0] != '\0')
                        {
                            /* NOTE: add_indices_to_playlist() overwrites the
                               audiobuf so we need to reload control file
                               data */
                            add_indices_to_playlist(playlist, NULL, 0);
                        }
                        else if (str2[0] != '\0')
                        {
                            playlist->in_ram = true;
                            resume_directory(str2);
                        }
                        
                        /* load the rest of the data */
                        first = false;
                        exit_loop = true;

                        break;
                    }
                    case resume_add:
                    case resume_queue:
                    {
                        /* str1=position str2=last_position str3=file */
                        int position, last_position;
                        bool queue;
                        
                        if (!str1 || !str2 || !str3)
                        {
                            result = -1;
                            exit_loop = true;
                            break;
                        }
                        
                        position = atoi(str1);
                        last_position = atoi(str2);
                        
                        queue = (current_command == resume_add)?false:true;

                        /* seek position is based on str3's position in
                           buffer */
                        if (add_track_to_playlist(playlist, str3, position,
                                queue, total_read+(str3-buffer)) < 0)
                            return -1;
                        
                        playlist->last_insert_pos = last_position;

                        break;
                    }
                    case resume_delete:
                    {
                        /* str1=position */
                        int position;
                        
                        if (!str1)
                        {
                            result = -1;
                            exit_loop = true;
                            break;
                        }
                        
                        position = atoi(str1);
                        
                        if (remove_track_from_playlist(playlist, position,
                                false) < 0)
                            return -1;

                        break;
                    }
                    case resume_shuffle:
                    {
                        /* str1=seed str2=first_index */
                        int seed;
                        
                        if (!str1 || !str2)
                        {
                            result = -1;
                            exit_loop = true;
                            break;
                        }
                        
                        if (!sorted)
                        {
                            /* Always sort list before shuffling */
                            sort_playlist(playlist, false, false);
                        }

                        seed = atoi(str1);
                        playlist->first_index = atoi(str2);
                        
                        if (randomise_playlist(playlist, seed, false,
                                false) < 0)
                            return -1;

                        sorted = false;
                        break;
                    }
                    case resume_unshuffle:
                    {
                        /* str1=first_index */
                        if (!str1)
                        {
                            result = -1;
                            exit_loop = true;
                            break;
                        }
                        
                        playlist->first_index = atoi(str1);
                        
                        if (sort_playlist(playlist, false, false) < 0)
                            return -1;

                        sorted = true;
                        break;
                    }
                    case resume_reset:
                    {
                        playlist->last_insert_pos = -1;
                        break;
                    }
                    case resume_comment:
                    default:
                        break;
                }

                newline = true;

                /* to ignore any extra newlines */
                current_command = resume_comment;
            }
            else if(newline)
            {
                newline = false;

                /* first non-comment line must always specify playlist */
                if (first && *p != 'P' && *p != '#')
                {
                    result = -1;
                    exit_loop = true;
                    break;
                }

                switch (*p)
                {
                    case 'P':
                        /* playlist can only be specified once */
                        if (!first)
                        {
                            result = -1;
                            exit_loop = true;
                            break;
                        }

                        current_command = resume_playlist;
                        break;
                    case 'A':
                        current_command = resume_add;
                        break;
                    case 'Q':
                        current_command = resume_queue;
                        break;
                    case 'D':
                        current_command = resume_delete;
                        break;
                    case 'S':
                        current_command = resume_shuffle;
                        break;
                    case 'U':
                        current_command = resume_unshuffle;
                        break;
                    case 'R':
                        current_command = resume_reset;
                        break;
                    case '#':
                        current_command = resume_comment;
                        break;
                    default:
                        result = -1;
                        exit_loop = true;
                        break;
                }

                str_count = -1;
                str1 = NULL;
                str2 = NULL;
                str3 = NULL;
            }
            else if(current_command != resume_comment)
            {
                /* all control file strings are separated with a colon.
                   Replace the colon with 0 to get proper strings that can be
                   used by commands above */
                if (*p == ':')
                {
                    *p = '\0';
                    str_count++;

                    if ((count+1) < nread)
                    {
                        switch (str_count)
                        {
                        case 0:
                            str1 = p+1;
                            break;
                        case 1:
                            str2 = p+1;
                            break;
                        case 2:
                            str3 = p+1;
                            break;
                        default:
                            /* allow last string to contain colons */
                            *p = ':';
                            break;
                        }
                    }
                }
            }
        }

        if (result < 0)
        {
            splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_INVALID));
            return result;
        }

        if (!newline || (exit_loop && count<nread))
        {
            if ((total_read + count) >= control_file_size)
            {
                /* no newline at end of control file */
                splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_INVALID));
                return -1;
            }

            /* We didn't end on a newline or we exited loop prematurely.
               Either way, re-read the remainder. */
            count = last_newline;
            lseek(playlist->control_fd, total_read+count, SEEK_SET);
        }

        total_read += count;

        if (first)
            /* still looking for header */
            nread = read(playlist->control_fd, buffer,
                PLAYLIST_COMMAND_SIZE<buflen?PLAYLIST_COMMAND_SIZE:buflen);
        else
            nread = read(playlist->control_fd, buffer, buflen);

        /* Terminate on EOF */
        if(nread <= 0)
        {
            if (global_settings.resume_seed >= 0)
            {
                /* Apply shuffle command saved in settings */
                if (global_settings.resume_seed == 0)
                    sort_playlist(playlist, false, true);
                else
                {
                    if (!sorted)
                        sort_playlist(playlist, false, false);

                    randomise_playlist(playlist, global_settings.resume_seed,
                        false, true);
                }

                playlist->first_index = global_settings.resume_first_index;
            }

            break;
        }
    }

    return 0;
}

/*
 * Add track to in_ram playlist.  Used when playing directories.
 */
int playlist_add(const char *filename)
{
    struct playlist_info* playlist = &current_playlist;
    int len = strlen(filename);
    
    if((len+1 > playlist->buffer_size - playlist->buffer_end_pos) ||
       (playlist->amount >= playlist->max_playlist_size))
    {
        display_buffer_full();
        return -1;
    }

    playlist->indices[playlist->amount++] = playlist->buffer_end_pos;

    strcpy(&playlist->buffer[playlist->buffer_end_pos], filename);
    playlist->buffer_end_pos += len;
    playlist->buffer[playlist->buffer_end_pos++] = '\0';

    return 0;
}

/* shuffle newly created playlist using random seed. */
int playlist_shuffle(int random_seed, int start_index)
{
    struct playlist_info* playlist = &current_playlist;

    unsigned int seek_pos = 0;
    bool start_current = false;

    if (start_index >= 0 && global_settings.play_selected)
    {
        /* store the seek position before the shuffle */
        seek_pos = playlist->indices[start_index];
        playlist->index = global_settings.resume_first_index =
            playlist->first_index = start_index;
        start_current = true;
    }

    splash(0, true, str(LANG_PLAYLIST_SHUFFLE));
    
    randomise_playlist(playlist, random_seed, start_current, true);

    /* Flush shuffle command to disk */
    flush_pending_control(playlist);

    return playlist->index;
}

/* start playing current playlist at specified index/offset */
int playlist_start(int start_index, int offset)
{
    struct playlist_info* playlist = &current_playlist;

    playlist->index = start_index;
#if CONFIG_CODEC != SWCODEC
    talk_buffer_steal(); /* will use the mp3 buffer */
#endif
    audio_play(offset);

    return 0;
}

/* Returns false if 'steps' is out of bounds, else true */
bool playlist_check(int steps)
{
    struct playlist_info* playlist = &current_playlist;

    /* always allow folder navigation */
    if (global_settings.next_folder && playlist->in_ram)
        return true;

    int index = get_next_index(playlist, steps, -1);

    if (index < 0 && steps >= 0 && global_settings.repeat_mode == REPEAT_SHUFFLE)
        index = get_next_index(playlist, steps, REPEAT_ALL);

    return (index >= 0);
}

/* get trackname of track that is "steps" away from current playing track.
   NULL is used to identify end of playlist */
char* playlist_peek(int steps)
{
    struct playlist_info* playlist = &current_playlist;
    int seek;
    int fd;
    char *temp_ptr;
    int index;
    bool control_file;

    index = get_next_index(playlist, steps, -1);
    if (index < 0)
        return NULL;

    control_file = playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK;
    seek = playlist->indices[index] & PLAYLIST_SEEK_MASK;

    if (get_filename(playlist, seek, control_file, now_playing,
            MAX_PATH+1) < 0)
        return NULL;

    temp_ptr = now_playing;

    if (!playlist->in_ram || control_file)
    {
        /* remove bogus dirs from beginning of path
           (workaround for buggy playlist creation tools) */
        while (temp_ptr)
        {
            fd = open(temp_ptr, O_RDONLY);
            if (fd >= 0)
            {
                close(fd);
                break;
            }
            
            temp_ptr = strchr(temp_ptr+1, '/');
        }
        
        if (!temp_ptr)
        {
            /* Even though this is an invalid file, we still need to pass a
               file name to the caller because NULL is used to indicate end
               of playlist */
            return now_playing;
        }
    }

    return temp_ptr;
}

/*
 * Update indices as track has changed
 */
int playlist_next(int steps)
{
    struct playlist_info* playlist = &current_playlist;
    int index;

    if ( (steps > 0)
#ifdef AB_REPEAT_ENABLE
    && (global_settings.repeat_mode != REPEAT_AB)
#endif
    && (global_settings.repeat_mode != REPEAT_ONE) )
    {
        int i, j;

        /* We need to delete all the queued songs */
        for (i=0, j=steps; i<j; i++)
        {
            index = get_next_index(playlist, i, -1);
            
            if (playlist->indices[index] & PLAYLIST_QUEUE_MASK)
            {
                remove_track_from_playlist(playlist, index, true);
                steps--; /* one less track */
            }
        }
    }

    index = get_next_index(playlist, steps, -1);

    if (index < 0)
    {
        /* end of playlist... or is it */
        if (global_settings.repeat_mode == REPEAT_SHUFFLE &&
            global_settings.playlist_shuffle &&
            playlist->amount > 1)
        {
            /* Repeat shuffle mode.  Re-shuffle playlist and resume play */
            playlist->first_index = global_settings.resume_first_index = 0;
            sort_playlist(playlist, false, false);
            randomise_playlist(playlist, current_tick, false, true);
            playlist_start(0, 0);
            index = 0;
        }
        else if (playlist->in_ram && global_settings.next_folder)
        {
            char dir[MAX_PATH+1];

            if (steps > 0)
            {
                if (!get_next_directory(dir))
                {
                    /* start playing next directory */
                    if (playlist_create(dir, NULL) != -1)
                    {
                        ft_build_playlist(tree_get_context(), 0);
                        if (global_settings.playlist_shuffle)
                            playlist_shuffle(current_tick, -1);                    
                        playlist_start(0, 0);
                        index = 0;
                    }
                }
            }
            else
            {
              if (!get_previous_directory(dir))
              {
                /* start playing previous directory */
                if (playlist_create(dir, NULL) != -1)
                {
                    ft_build_playlist(tree_get_context(), 0);
                    if (global_settings.playlist_shuffle)
                        playlist_shuffle(current_tick, -1);
                    playlist_start(current_playlist.amount-1,0);
                    index = current_playlist.amount-1;
                }
              }
           }
        }

        return index;
    }

    playlist->index = index;

    if (playlist->last_insert_pos >= 0 && steps > 0)
    {
        /* check to see if we've gone beyond the last inserted track */
        int cur = rotate_index(playlist, index);
        int last_pos = rotate_index(playlist, playlist->last_insert_pos);

        if (cur > last_pos)
        {
            /* reset last inserted track */
            playlist->last_insert_pos = -1;

            if (playlist->control_fd >= 0)
            {
                int result = -1;

                mutex_lock(&playlist->control_mutex);
            
                if (lseek(playlist->control_fd, 0, SEEK_END) >= 0)
                {
                    if (fdprintf(playlist->control_fd, "R\n") > 0)
                    {
                        fsync(playlist->control_fd);
                        result = 0;
                    }
                }

                mutex_unlock(&playlist->control_mutex);

                if (result < 0)
                {
                    splash(HZ*2, true,
                        str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
                    return result;
                }
            }
        }
    }

    return index;
}

/* try playing next or previous folder */
bool playlist_next_dir(int direction)
{
    char dir[MAX_PATH+1];

    if (((direction > 0) && !get_next_directory(dir)) ||
       ((direction < 0) && !get_previous_directory(dir)))
    {
        if (playlist_create(dir, NULL) != -1)
        {
            ft_build_playlist(tree_get_context(), 0);
            if (global_settings.playlist_shuffle)
                 playlist_shuffle(current_tick, -1);
            playlist_start(0,0);
            return true;
        }
        else
            return false;
    }
    else
        return false;
}

/* Get resume info for current playing song.  If return value is -1 then
   settings shouldn't be saved. */
int playlist_get_resume_info(int *resume_index)
{
    struct playlist_info* playlist = &current_playlist;

    *resume_index = playlist->index;

    return 0;
}

/* Update resume info for current playing song.  Returns -1 on error. */
int playlist_update_resume_info(const struct mp3entry* id3)
{
    struct playlist_info* playlist = &current_playlist;

    if (id3)
    {
        if (global_settings.resume_index != playlist->index ||
            global_settings.resume_offset != id3->offset)
        {
            global_settings.resume_index = playlist->index;
            global_settings.resume_offset = id3->offset;
            settings_save();
        }
    }
    else
    {
        global_settings.resume_index = -1;
        global_settings.resume_offset = -1;
        settings_save();
    }

    return 0;
}

/* Returns index of current playing track for display purposes.  This value
   should not be used for resume purposes as it doesn't represent the actual
   index into the playlist */
int playlist_get_display_index(void)
{
    struct playlist_info* playlist = &current_playlist;

    /* first_index should always be index 0 for display purposes */
    int index = rotate_index(playlist, playlist->index);

    return (index+1);
}

/* returns number of tracks in current playlist */
int playlist_amount(void)
{
    return playlist_amount_ex(NULL);
}

/*
 * Create a new playlist  If playlist is not NULL then we're loading a
 * playlist off disk for viewing/editing.  The index_buffer is used to store
 * playlist indices (required for and only used if !current playlist).  The
 * temp_buffer (if not NULL) is used as a scratchpad when loading indices.
 */
int playlist_create_ex(struct playlist_info* playlist,
                       const char* dir, const char* file,
                       void* index_buffer, int index_buffer_size,
                       void* temp_buffer, int temp_buffer_size)
{
    if (!playlist)
        playlist = &current_playlist;
    else
    {
        /* Initialize playlist structure */
        int r = rand() % 10;
        playlist->current = false;

        /* Use random name for control file */
        snprintf(playlist->control_filename, sizeof(playlist->control_filename),
            "%s.%d", PLAYLIST_CONTROL_FILE, r);
        playlist->fd = -1;
        playlist->control_fd = -1;

        if (index_buffer)
        {
            int num_indices = index_buffer_size / sizeof(int);

            if (num_indices > global_settings.max_files_in_playlist)
                num_indices = global_settings.max_files_in_playlist;

            playlist->max_playlist_size = num_indices;
            playlist->indices = index_buffer;
        }
        else
        {
            playlist->max_playlist_size = current_playlist.max_playlist_size;
            playlist->indices = current_playlist.indices;
        }

        playlist->buffer_size = 0;
        playlist->buffer = NULL;
        mutex_init(&playlist->control_mutex);
    }

    new_playlist(playlist, dir, file);

    if (file)
        /* load the playlist file */
        add_indices_to_playlist(playlist, temp_buffer, temp_buffer_size);

    return 0;
}

/*
 * Set the specified playlist as the current.
 * NOTE: You will get undefined behaviour if something is already playing so
 *       remember to stop before calling this.  Also, this call will
 *       effectively close your playlist, making it unusable.
 */
int playlist_set_current(struct playlist_info* playlist)
{
    if (!playlist || (check_control(playlist) < 0))
        return -1;

    empty_playlist(&current_playlist, false);

    strncpy(current_playlist.filename, playlist->filename,
        sizeof(current_playlist.filename));

    current_playlist.fd = playlist->fd;

    close(playlist->control_fd);
    remove(current_playlist.control_filename);
    if (rename(playlist->control_filename,
            current_playlist.control_filename) < 0)
        return -1;
    current_playlist.control_fd = open(current_playlist.control_filename,
        O_RDWR);
    if (current_playlist.control_fd < 0)
        return -1;
    current_playlist.control_created = true;

    current_playlist.dirlen = playlist->dirlen;

    if (playlist->indices && playlist->indices != current_playlist.indices)
        memcpy(current_playlist.indices, playlist->indices,
            playlist->max_playlist_size*sizeof(int));

    current_playlist.first_index = playlist->first_index;
    current_playlist.amount = playlist->amount;
    current_playlist.last_insert_pos = playlist->last_insert_pos;
    current_playlist.seed = playlist->seed;
    current_playlist.shuffle_modified = playlist->shuffle_modified;
    current_playlist.deleted = playlist->deleted;
    current_playlist.num_inserted_tracks = playlist->num_inserted_tracks;
    current_playlist.shuffle_flush = playlist->shuffle_flush;
    
    return 0;
}

/*
 * Close files and delete control file for non-current playlist.
 */
void playlist_close(struct playlist_info* playlist)
{
    if (!playlist)
        return;

    if (playlist->fd >= 0)
        close(playlist->fd);

    if (playlist->control_fd >= 0)
        close(playlist->control_fd);

    if (playlist->control_created)
        remove(playlist->control_filename);
}

/*
 * Insert track into playlist at specified position (or one of the special
 * positions).  Returns position where track was inserted or -1 if error.
 */
int playlist_insert_track(struct playlist_info* playlist,
                          const char *filename, int position, bool queue)
{
    int result;
    
    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    result = add_track_to_playlist(playlist, filename, position, queue, -1);

    if (result != -1)
    {
        mutex_lock(&playlist->control_mutex);
        fsync(playlist->control_fd);
        mutex_unlock(&playlist->control_mutex);

        if (audio_status() & AUDIO_STATUS_PLAY)
            audio_flush_and_reload_tracks();
    }

    return result;
}

/*
 * Insert all tracks from specified directory into playlist.
 */
int playlist_insert_directory(struct playlist_info* playlist,
                              const char *dirname, int position, bool queue,
                              bool recurse)
{
    int count = 0;
    int result;
    char *count_str;

    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    if (queue)
        count_str = str(LANG_PLAYLIST_QUEUE_COUNT);
    else
        count_str = str(LANG_PLAYLIST_INSERT_COUNT);

    display_playlist_count(count, count_str);

    result = add_directory_to_playlist(playlist, dirname, &position, queue,
        &count, recurse);

    mutex_lock(&playlist->control_mutex);
    fsync(playlist->control_fd);
    mutex_unlock(&playlist->control_mutex);

    display_playlist_count(count, count_str);

    if (audio_status() & AUDIO_STATUS_PLAY)
        audio_flush_and_reload_tracks();

    return result;
}

/*
 * Insert all tracks from specified playlist into dynamic playlist.
 */
int playlist_insert_playlist(struct playlist_info* playlist, char *filename,
                             int position, bool queue)
{
    int fd;
    int max;
    char *temp_ptr;
    char *dir;
    char *count_str;
    char temp_buf[MAX_PATH+1];
    char trackname[MAX_PATH+1];
    int count = 0;
    int result = 0;

    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_ACCESS_ERROR));
        return -1;
    }

    /* we need the directory name for formatting purposes */
    dir = filename;

    temp_ptr = strrchr(filename+1,'/');
    if (temp_ptr)
        *temp_ptr = 0;
    else
        dir = "/";

    if (queue)
        count_str = str(LANG_PLAYLIST_QUEUE_COUNT);
    else
        count_str = str(LANG_PLAYLIST_INSERT_COUNT);

    display_playlist_count(count, count_str);

    while ((max = read_line(fd, temp_buf, sizeof(temp_buf))) > 0)
    {
        /* user abort */
        if (button_get(false) == SETTINGS_CANCEL)
            break;

        if (temp_buf[0] != '#' && temp_buf[0] != '\0')
        {
            int insert_pos;

            /* we need to format so that relative paths are correctly
               handled */
            if (format_track_path(trackname, temp_buf, sizeof(trackname), max,
                    dir) < 0)
            {
                result = -1;
                break;
            }
            
            insert_pos = add_track_to_playlist(playlist, trackname, position,
                queue, -1);

            if (insert_pos < 0)
            {
                result = -1;
                break;
            }

            /* Make sure tracks are inserted in correct order if user
               requests INSERT_FIRST */
            if (position == PLAYLIST_INSERT_FIRST || position >= 0)
                position = insert_pos + 1;

            count++;
            
            if ((count%PLAYLIST_DISPLAY_COUNT) == 0)
            {
                display_playlist_count(count, count_str);

                if (count == PLAYLIST_DISPLAY_COUNT &&
                    (audio_status() & AUDIO_STATUS_PLAY))
                    audio_flush_and_reload_tracks();
            }
        }

        /* let the other threads work */
        yield();
    }

    close(fd);

    mutex_lock(&playlist->control_mutex);
    fsync(playlist->control_fd);
    mutex_unlock(&playlist->control_mutex);

    if (temp_ptr)
        *temp_ptr = '/';

    display_playlist_count(count, count_str);

    if (audio_status() & AUDIO_STATUS_PLAY)
        audio_flush_and_reload_tracks();

    return result;
}

/*
 * Delete track at specified index.  If index is PLAYLIST_DELETE_CURRENT then
 * we want to delete the current playing track.
 */
int playlist_delete(struct playlist_info* playlist, int index)
{
    int result = 0;

    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    if (index == PLAYLIST_DELETE_CURRENT)
        index = playlist->index;

    result = remove_track_from_playlist(playlist, index, true);
    
    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY))
        audio_flush_and_reload_tracks();

    return result;
}

/*
 * Move track at index to new_index.  Tracks between the two are shifted
 * appropriately.  Returns 0 on success and -1 on failure.
 */
int playlist_move(struct playlist_info* playlist, int index, int new_index)
{
    int result;
    int seek;
    bool control_file;
    bool queue;
    bool current = false;
    int r;
    char filename[MAX_PATH];

    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    if (index == new_index)
        return -1;

    if (index == playlist->index)
        /* Moving the current track */
        current = true;

    control_file = playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK;
    queue = playlist->indices[index] & PLAYLIST_QUEUE_MASK;
    seek = playlist->indices[index] & PLAYLIST_SEEK_MASK;

    if (get_filename(playlist, seek, control_file, filename,
            sizeof(filename)) < 0)
        return -1;

    /* Delete track from original position */
    result = remove_track_from_playlist(playlist, index, true);

    if (result != -1)
    {
        /* We want to insert the track at the position that was specified by
           new_index.  This may be different then new_index because of the
           shifting that occurred after the delete */
        r = rotate_index(playlist, new_index);

        if (r == 0)
            /* First index */
            new_index = PLAYLIST_PREPEND;
        else if (r == playlist->amount)
            /* Append */
            new_index = PLAYLIST_INSERT_LAST;
        else
            /* Calculate index of desired position */
            new_index = (r+playlist->first_index)%playlist->amount;

        result = add_track_to_playlist(playlist, filename, new_index, queue,
            -1);
        
        if (result != -1)
        {
            if (current)
            {
                /* Moved the current track */
                switch (new_index)
                {
                    case PLAYLIST_PREPEND:
                        playlist->index = playlist->first_index;
                        break;
                    case PLAYLIST_INSERT_LAST:
                        playlist->index = playlist->first_index - 1;
                        if (playlist->index < 0)
                            playlist->index += playlist->amount;
                        break;
                    default:
                        playlist->index = new_index;
                        break;
                }
            }

            mutex_lock(&playlist->control_mutex);
            fsync(playlist->control_fd);
            mutex_unlock(&playlist->control_mutex);

            if (audio_status() & AUDIO_STATUS_PLAY)
                audio_flush_and_reload_tracks();
        }
    }

    return result;
}

/* shuffle currently playing playlist */
int playlist_randomise(struct playlist_info* playlist, unsigned int seed,
                       bool start_current)
{
    int result;

    if (!playlist)
        playlist = &current_playlist;

    check_control(playlist);

    result = randomise_playlist(playlist, seed, start_current, true);

    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY))
        audio_flush_and_reload_tracks();

    return result;
}

/* sort currently playing playlist */
int playlist_sort(struct playlist_info* playlist, bool start_current)
{
    int result;

    if (!playlist)
        playlist = &current_playlist;

    check_control(playlist);

    result = sort_playlist(playlist, start_current, true);

    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY))
        audio_flush_and_reload_tracks();

    return result;
}

/* returns true if playlist has been modified */
bool playlist_modified(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    if (playlist->shuffle_modified ||
        playlist->deleted ||
        playlist->num_inserted_tracks > 0)
        return true;

    return false;
}

/* returns index of first track in playlist */
int playlist_get_first_index(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return playlist->first_index;
}

/* returns shuffle seed of playlist */
int playlist_get_seed(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return playlist->seed;
}

/* returns number of tracks in playlist (includes queued/inserted tracks) */
int playlist_amount_ex(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return playlist->amount;
}

/* returns full path of playlist (minus extension) */
char *playlist_name(const struct playlist_info* playlist, char *buf,
                    int buf_size)
{
    char *sep;

    if (!playlist)
        playlist = &current_playlist;

    snprintf(buf, buf_size, "%s", playlist->filename+playlist->dirlen);
  
    if (!buf[0])
        return NULL;

    /* Remove extension */
    sep = strrchr(buf, '.');
    if (sep)
        *sep = 0;

    return buf;
}

/* returns the playlist filename */
char *playlist_get_name(const struct playlist_info* playlist, char *buf,
                        int buf_size)
{
    if (!playlist)
        playlist = &current_playlist;

    snprintf(buf, buf_size, "%s", playlist->filename);

    if (!buf[0])
        return NULL;

    return buf;
}

/* Fills info structure with information about track at specified index.
   Returns 0 on success and -1 on failure */
int playlist_get_track_info(struct playlist_info* playlist, int index,
                            struct playlist_track_info* info)
{
    int seek;
    bool control_file;

    if (!playlist)
        playlist = &current_playlist;

    if (index < 0 || index >= playlist->amount)
        return -1;

    control_file = playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK;
    seek = playlist->indices[index] & PLAYLIST_SEEK_MASK;

    if (get_filename(playlist, seek, control_file, info->filename,
            sizeof(info->filename)) < 0)
        return -1;

    info->attr = 0;

    if (control_file)
    {
        if (playlist->indices[index] & PLAYLIST_QUEUE_MASK)
            info->attr |= PLAYLIST_ATTR_QUEUED;
        else
            info->attr |= PLAYLIST_ATTR_INSERTED;
        
    }

    if (playlist->indices[index] & PLAYLIST_SKIPPED)
        info->attr |= PLAYLIST_ATTR_SKIPPED;
    
    info->index = index;
    info->display_index = rotate_index(playlist, index) + 1;

    return 0;
}

/* save the current dynamic playlist to specified file */
int playlist_save(struct playlist_info* playlist, char *filename)
{
    int fd;
    int i, index;
    int count = 0;
    char tmp_buf[MAX_PATH+1];
    int result = 0;

    if (!playlist)
        playlist = &current_playlist;

    if (playlist->amount <= 0)
        return -1;

    /* use current working directory as base for pathname */
    if (format_track_path(tmp_buf, filename, sizeof(tmp_buf),
                          strlen(filename)+1, getcwd(NULL, -1)) < 0)
        return -1;

    fd = open(tmp_buf, O_CREAT|O_WRONLY|O_TRUNC);
    if (fd < 0)
    {
        splash(HZ*2, true, str(LANG_PLAYLIST_ACCESS_ERROR));
        return -1;
    }

    display_playlist_count(count, str(LANG_PLAYLIST_SAVE_COUNT));

    index = playlist->first_index;
    for (i=0; i<playlist->amount; i++)
    {
        bool control_file;
        bool queue;
        int seek;

        /* user abort */
        if (button_get(false) == SETTINGS_CANCEL)
            break;

        control_file = playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK;
        queue = playlist->indices[index] & PLAYLIST_QUEUE_MASK;
        seek = playlist->indices[index] & PLAYLIST_SEEK_MASK;

        /* Don't save queued files */
        if (!queue)
        {
            if (get_filename(playlist, seek, control_file, tmp_buf,
                    MAX_PATH+1) < 0)
            {
                result = -1;
                break;
            }

            if (fdprintf(fd, "%s\n", tmp_buf) < 0)
            {
                splash(HZ*2, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
                result = -1;
                break;
            }

            count++;

            if ((count % PLAYLIST_DISPLAY_COUNT) == 0)
                display_playlist_count(count, str(LANG_PLAYLIST_SAVE_COUNT));

            yield();
        }

        index = (index+1)%playlist->amount;
    }

    display_playlist_count(count, str(LANG_PLAYLIST_SAVE_COUNT));

    close(fd);

    return result;
}
