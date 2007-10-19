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
#include <ctype.h>
#include "playlist.h"
#include "file.h"
#include "action.h"
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
#include "thread.h"
#include "usb.h"
#include "filetypes.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

#include "lang.h"
#include "talk.h"
#include "splash.h"
#include "rbunicode.h"
#include "root_menu.h"

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

struct directory_search_context {
    struct playlist_info* playlist;
    int position;
    bool queue;
    int count;
};

static bool changing_dir = false;

static struct playlist_info current_playlist;
static char now_playing[MAX_PATH+1];

static void empty_playlist(struct playlist_info* playlist, bool resume);
static void new_playlist(struct playlist_info* playlist, const char *dir,
                         const char *file);
static void create_control(struct playlist_info* playlist);
static int  check_control(struct playlist_info* playlist);
static int  recreate_control(struct playlist_info* playlist);
static void update_playlist_filename(struct playlist_info* playlist,
                                     const char *dir, const char *file);
static int add_indices_to_playlist(struct playlist_info* playlist,
                                   char* buffer, size_t buflen);
static int add_track_to_playlist(struct playlist_info* playlist,
                                 const char *filename, int position,
                                 bool queue, int seek_pos);
static int directory_search_callback(char* filename, void* context);
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
static int get_filename(struct playlist_info* playlist, int index, int seek,
                        bool control_file, char *buf, int buf_length);
static int get_next_directory(char *dir);
static int get_next_dir(char *dir, bool is_forward, bool recursion);
static int get_previous_directory(char *dir);
static int check_subdir_for_music(char *dir, char *subdir);
static int format_track_path(char *dest, char *src, int buf_length, int max,
                             char *dir);
static void display_playlist_count(int count, const unsigned char *fmt,
                                   bool final);
static void display_buffer_full(void);
static int flush_cached_control(struct playlist_info* playlist);
static int update_control(struct playlist_info* playlist,
                          enum playlist_command command, int i1, int i2,
                          const char* s1, const char* s2, void* data);
static void sync_control(struct playlist_info* playlist, bool force);
static int rotate_index(const struct playlist_info* playlist, int index);

#ifdef HAVE_DIRCACHE
#define PLAYLIST_LOAD_POINTERS 1

static struct event_queue playlist_queue;
static long playlist_stack[(DEFAULT_STACK_SIZE + 0x800)/sizeof(long)];
static const char playlist_thread_name[] = "playlist cachectrl";
#endif

/*
 * remove any files and indices associated with the playlist
 */
static void empty_playlist(struct playlist_info* playlist, bool resume)
{
    playlist->filename[0] = '\0';
    playlist->utf8 = true;

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
    playlist->started = false;

    playlist->num_cached = 0;
    playlist->pending_control_sync = false;

    if (!resume && playlist->current)
    {
        /* start with fresh playlist control file when starting new
           playlist */
        create_control(playlist);

        /* Reset resume settings */
        global_status.resume_first_index = 0;
        global_status.resume_seed = -1;
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
        update_control(playlist, PLAYLIST_COMMAND_PLAYLIST,
            PLAYLIST_CONTROL_FILE_VERSION, -1, dir, file, NULL);
        sync_control(playlist, false);
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
            cond_talk_ids_fq(LANG_PLAYLIST_CONTROL_ACCESS_ERROR);
            gui_syncsplash(HZ*2, (unsigned char *)"%s (%d)",
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

            update_control(playlist, PLAYLIST_COMMAND_PLAYLIST,
                PLAYLIST_CONTROL_FILE_VERSION, -1, dir, file, NULL);
            sync_control(playlist, false);
            playlist->filename[playlist->dirlen-1] = c;
        }
    }

    if (playlist->control_fd < 0)
        return -1;

    return 0;
}

/*
 * recreate the control file based on current playlist entries
 */
static int recreate_control(struct playlist_info* playlist)
{
    char temp_file[MAX_PATH+1];
    int  temp_fd = -1;
    int  i;
    int  result = 0;

    if(playlist->control_fd >= 0)
    {
        char* dir = playlist->filename;
        char* file = playlist->filename+playlist->dirlen;
        char c = playlist->filename[playlist->dirlen-1];

        close(playlist->control_fd);

        snprintf(temp_file, sizeof(temp_file), "%s_temp",
            playlist->control_filename);

        if (rename(playlist->control_filename, temp_file) < 0)
            return -1;

        temp_fd = open(temp_file, O_RDONLY);
        if (temp_fd < 0)
            return -1;

        playlist->control_fd = open(playlist->control_filename,
            O_CREAT|O_RDWR|O_TRUNC);
        if (playlist->control_fd < 0)
            return -1;

        playlist->filename[playlist->dirlen-1] = '\0';
        
        /* cannot call update_control() because of mutex */
        result = fdprintf(playlist->control_fd, "P:%d:%s:%s\n",
            PLAYLIST_CONTROL_FILE_VERSION, dir, file);

        playlist->filename[playlist->dirlen-1] = c;

        if (result < 0)
        {
            close(temp_fd);
            return result;
        }
    }

    playlist->seed = 0;
    playlist->shuffle_modified = false;
    playlist->deleted = false;
    playlist->num_inserted_tracks = 0;

    if (playlist->current)
    {
        global_status.resume_seed = -1;
        status_save();
    }

    for (i=0; i<playlist->amount; i++)
    {
        if (playlist->indices[i] & PLAYLIST_INSERT_TYPE_MASK)
        {
            bool queue = playlist->indices[i] & PLAYLIST_QUEUE_MASK;
            char inserted_file[MAX_PATH+1];

            lseek(temp_fd, playlist->indices[i] & PLAYLIST_SEEK_MASK,
                SEEK_SET);
            read_line(temp_fd, inserted_file, sizeof(inserted_file));

            result = fdprintf(playlist->control_fd, "%c:%d:%d:",
                queue?'Q':'A', i, playlist->last_insert_pos);
            if (result > 0)
            {
                /* save the position in file where name is written */
                int seek_pos = lseek(playlist->control_fd, 0, SEEK_CUR);

                result = fdprintf(playlist->control_fd, "%s\n",
                    inserted_file);

                playlist->indices[i] =
                    (playlist->indices[i] & ~PLAYLIST_SEEK_MASK) | seek_pos;
            }

            if (result < 0)
                break;

            playlist->num_inserted_tracks++;
        }
    }

    close(temp_fd);
    remove(temp_file);
    fsync(playlist->control_fd);

    if (result < 0)
        return result;

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
    int filelen = strlen(file);

    /* Default to utf8 unless explicitly told otherwise. */
    playlist->utf8 = !(filelen > 4 && strcasecmp(&file[filelen - 4], ".m3u") == 0);
    
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
                                   char* buffer, size_t buflen)
{
    unsigned int nread;
    unsigned int i = 0;
    unsigned int count = 0;
    bool store_index;
    unsigned char *p;
    int result = 0;

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
    gui_syncsplash(0, ID2P(LANG_WAIT));

    if (!buffer)
    {
        /* use mp3 buffer for maximum load speed */
        audio_stop();
#if CONFIG_CODEC != SWCODEC
        talk_buffer_steal(); /* we use the mp3 buffer, need to tell */
        buflen = (audiobufend - audiobuf);
        buffer = (char *)audiobuf;
#else
        buffer = (char *)audio_get_buffer(false, &buflen);
#endif
    }
    
    store_index = true;

    while(1)
    {
        nread = read(playlist->fd, buffer, buflen);
        /* Terminate on EOF */
        if(nread <= 0)
            break;
        
        p = (unsigned char *)buffer;

        /* utf8 BOM at beginning of file? */
        if(i == 0 && nread > 3 
           && *p == 0xef && *(p+1) == 0xbb && *(p+2) == 0xbf) {
            nread -= 3;
            p += 3;
            i += 3;
            playlist->utf8 = true;  /* Override any earlier indication. */
        }

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
                    if ( playlist->amount >= playlist->max_playlist_size ) {
                        display_buffer_full();
                        result = -1;
                        goto exit;
                    }

                    /* Store a new entry */
                    playlist->indices[ playlist->amount ] = i+count;
#ifdef HAVE_DIRCACHE
                    if (playlist->filenames)
                        playlist->filenames[ playlist->amount ] = NULL;
#endif
                    playlist->amount++;
                }
            }
        }

        i+= count;
    }

exit:
#ifdef HAVE_DIRCACHE
    queue_post(&playlist_queue, PLAYLIST_LOAD_POINTERS, 0);
#endif

    return result;
}

/*
 * Removes all tracks, from the playlist, leaving the presently playing
 * track queued.
 */
int remove_all_tracks(struct playlist_info *playlist)
{
    int result;

    if (playlist == NULL)
        playlist = &current_playlist;

    while (playlist->index > 0)
        if ((result = remove_track_from_playlist(playlist, 0, true)) < 0)
            return result;

    while (playlist->amount > 1)
        if ((result = remove_track_from_playlist(playlist, 1, true)) < 0)
            return result;

    if (playlist->amount == 1) {
        playlist->indices[0] |= PLAYLIST_QUEUED;
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
 *     PLAYLIST_REPLACE         - Erase current playlist, Cue the current track
 *                                and inster this track at the end.
 */
static int add_track_to_playlist(struct playlist_info* playlist,
                                 const char *filename, int position,
                                 bool queue, int seek_pos)
{
    int insert_position, orig_position;
    unsigned long flags = PLAYLIST_INSERT_TYPE_INSERT;
    int i;

    insert_position = orig_position = position;

    if (playlist->amount >= playlist->max_playlist_size)
    {
        display_buffer_full();
        return -1;
    }

    switch (position)
    {
        case PLAYLIST_PREPEND:
            position = insert_position = playlist->first_index;
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

            if (playlist->started)
                playlist->last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_FIRST:
            if (playlist->amount > 0)
                position = insert_position = playlist->index + 1;
            else
                position = insert_position = 0;

            if (playlist->last_insert_pos < 0 && playlist->started)
                playlist->last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_LAST:
            if (playlist->first_index > 0)
                position = insert_position = playlist->first_index;
            else
                position = insert_position = playlist->amount;
            break;
        case PLAYLIST_INSERT_SHUFFLED:
        {
            if (playlist->started)
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
            }
            else
                position = insert_position = (rand() % (playlist->amount+1));
            break;
        }
        case PLAYLIST_REPLACE:
            if (remove_all_tracks(playlist) < 0)
                return -1;
    
            position = insert_position = playlist->index + 1;
            break;
    }
    
    if (queue)
        flags |= PLAYLIST_QUEUED;

    /* shift indices so that track can be added */
    for (i=playlist->amount; i>insert_position; i--)
    {
        playlist->indices[i] = playlist->indices[i-1];
#ifdef HAVE_DIRCACHE
        if (playlist->filenames)
            playlist->filenames[i] = playlist->filenames[i-1];
#endif
    }
    
    /* update stored indices if needed */
    if (playlist->amount > 0 && insert_position <= playlist->index &&
        playlist->started)
        playlist->index++;

    if (playlist->amount > 0 && insert_position <= playlist->first_index &&
        orig_position != PLAYLIST_PREPEND && playlist->started)
    {
        playlist->first_index++;

        if (seek_pos < 0 && playlist->current)
        {
            global_status.resume_first_index = playlist->first_index;
            status_save();
        }
    }

    if (insert_position < playlist->last_insert_pos ||
        (insert_position == playlist->last_insert_pos && position < 0))
        playlist->last_insert_pos++;

    if (seek_pos < 0 && playlist->control_fd >= 0)
    {
        int result = update_control(playlist,
            (queue?PLAYLIST_COMMAND_QUEUE:PLAYLIST_COMMAND_ADD), position,
            playlist->last_insert_pos, filename, NULL, &seek_pos);

        if (result < 0)
            return result;
    }

    playlist->indices[insert_position] = flags | seek_pos;

#ifdef HAVE_DIRCACHE
    if (playlist->filenames)
        playlist->filenames[insert_position] = NULL;
#endif

    playlist->amount++;
    playlist->num_inserted_tracks++;

    return insert_position;
}

/*
 * Callback for playlist_directory_tracksearch to insert track into
 * playlist.
 */
static int directory_search_callback(char* filename, void* context)
{
    struct directory_search_context* c =
        (struct directory_search_context*) context;
    int insert_pos;

    insert_pos = add_track_to_playlist(c->playlist, filename, c->position,
        c->queue, -1);

    if (insert_pos < 0)
        return -1;
    
    (c->count)++;
    
    /* Make sure tracks are inserted in correct order if user requests
       INSERT_FIRST */
    if (c->position == PLAYLIST_INSERT_FIRST || c->position >= 0)
        c->position = insert_pos + 1;
    
    if (((c->count)%PLAYLIST_DISPLAY_COUNT) == 0)
    {
        unsigned char* count_str;

        if (c->queue)
            count_str = ID2P(LANG_PLAYLIST_QUEUE_COUNT);
        else
            count_str = ID2P(LANG_PLAYLIST_INSERT_COUNT);

        display_playlist_count(c->count, count_str, false);
        
        if ((c->count) == PLAYLIST_DISPLAY_COUNT &&
            (audio_status() & AUDIO_STATUS_PLAY) &&
            c->playlist->started)
            audio_flush_and_reload_tracks();
    }

    return 0;
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
    {
        playlist->indices[i] = playlist->indices[i+1];
#ifdef HAVE_DIRCACHE
        if (playlist->filenames)
            playlist->filenames[i] = playlist->filenames[i+1];
#endif
    }

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
            global_status.resume_first_index = playlist->first_index;
            status_save();
        }
    }

    if (position <= playlist->last_insert_pos)
        playlist->last_insert_pos--;

    if (write && playlist->control_fd >= 0)
    {
        int result = update_control(playlist, PLAYLIST_COMMAND_DELETE,
            position, -1, NULL, NULL, NULL);

        if (result < 0)
            return result;

        sync_control(playlist, false);
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
    long store;
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
#ifdef HAVE_DIRCACHE
        if (playlist->filenames)
        {
            store = (long)playlist->filenames[candidate];
            playlist->filenames[candidate] = playlist->filenames[count];
            playlist->filenames[count] = (struct dircache_entry *)store;
        }
#endif
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
        update_control(playlist, PLAYLIST_COMMAND_SHUFFLE, seed,
            playlist->first_index, NULL, NULL, NULL);
        global_status.resume_seed = seed;
        status_save();
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

#ifdef HAVE_DIRCACHE
    /** We need to re-check the song names from disk because qsort can't
     * sort two arrays at once :/
     * FIXME: Please implement a better way to do this. */
    memset(playlist->filenames, 0, playlist->max_playlist_size * sizeof(int));
    queue_post(&playlist_queue, PLAYLIST_LOAD_POINTERS, 0);
#endif

    if (start_current)
        find_and_set_playlist_index(playlist, current);

    /* indices have been moved so last insert position is no longer valid */
    playlist->last_insert_pos = -1;

    if (!playlist->num_inserted_tracks && !playlist->deleted)
        playlist->shuffle_modified = false;
    if (write && playlist->control_fd >= 0)
    {
        update_control(playlist, PLAYLIST_COMMAND_UNSHUFFLE,
            playlist->first_index, -1, NULL, NULL, NULL);
        global_status.resume_seed = 0;
        status_save();
    }

    return 0;
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
    do {
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

        index += direction;
    } while (i <= count);

    return steps;
}

/* Marks the index of the track to be skipped that is "steps" away from
 * current playing track.
 */
void playlist_skip_entry(struct playlist_info *playlist, int steps)
{
    int index;

    if (playlist == NULL)
        playlist = &current_playlist;
    
    /* need to account for already skipped tracks */
    steps = calculate_step_count(playlist, steps);

    index = playlist->index + steps;
    if (index < 0)
        index += playlist->amount;
    else if (index >= playlist->amount)
        index -= playlist->amount;

    playlist->indices[index] |= PLAYLIST_SKIPPED;
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

    if (repeat_mode == REPEAT_SHUFFLE && playlist->amount <= 1)
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
                global_status.resume_first_index = i;
                status_save();
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

#ifdef HAVE_DIRCACHE
/**
 * Thread to update filename pointers to dircache on background
 * without affecting playlist load up performance.  This thread also flushes
 * any pending control commands when the disk spins up.
 */
static void playlist_thread(void)
{
    struct queue_event ev;
    bool dirty_pointers = false;
    static char tmp[MAX_PATH+1];

    struct playlist_info *playlist;
    int index;
    int seek;
    bool control_file;

    int sleep_time = 5;

#ifndef HAVE_FLASH_STORAGE
    if (global_settings.disk_spindown > 1 &&
        global_settings.disk_spindown <= 5)
        sleep_time = global_settings.disk_spindown - 1;
#endif

    while (1)
    {
        queue_wait_w_tmo(&playlist_queue, &ev, HZ*sleep_time);

        switch (ev.id)
        {
            case PLAYLIST_LOAD_POINTERS:
                dirty_pointers = true;
                break ;

            /* Start the background scanning after either the disk spindown
               timeout or 5s, whichever is less */
            case SYS_TIMEOUT:
                playlist = &current_playlist;

                if (playlist->control_fd >= 0
# ifndef SIMULATOR
                    && ata_disk_is_active()
# endif
                    )
                {
                    if (playlist->num_cached > 0)
                    {
                        mutex_lock(&playlist->control_mutex);
                        flush_cached_control(playlist);
                        mutex_unlock(&playlist->control_mutex);
                    }
                    
                    sync_control(playlist, true);
                }

                if (!dirty_pointers)
                    break ;

                if (!dircache_is_enabled() || !playlist->filenames
                     || playlist->amount <= 0)
                    break ;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                cpu_boost(true);
#endif
                for (index = 0; index < playlist->amount
                     && queue_empty(&playlist_queue); index++)
                {
                    /* Process only pointers that are not already loaded. */
                    if (playlist->filenames[index])
                        continue ;
                    
                    control_file = playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK;
                    seek = playlist->indices[index] & PLAYLIST_SEEK_MASK;

                    /* Load the filename from playlist file. */
                    if (get_filename(playlist, index, seek, control_file, tmp,
                        sizeof(tmp)) < 0)
                        break ;

                    /* Set the dircache entry pointer. */
                    playlist->filenames[index] = dircache_get_entry_ptr(tmp);

                    /* And be on background so user doesn't notice any delays. */
                    yield();
                }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                cpu_boost(false);
#endif
                dirty_pointers = false;
                break ;
            
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&playlist_queue);
                break ;
#endif
        }
    }
}
#endif

/*
 * gets pathname for track at seek index
 */
static int get_filename(struct playlist_info* playlist, int index, int seek,
                        bool control_file, char *buf, int buf_length)
{
    int fd;
    int max = -1;
    char tmp_buf[MAX_PATH+1];
    char dir_buf[MAX_PATH+1];

    if (buf_length > MAX_PATH+1)
        buf_length = MAX_PATH+1;

#ifdef HAVE_DIRCACHE
    if (dircache_is_enabled() && playlist->filenames)
    {
        if (playlist->filenames[index] != NULL)
        {
            dircache_copy_path(playlist->filenames[index], tmp_buf, sizeof(tmp_buf)-1);
            max = strlen(tmp_buf) + 1;
        }
    }
#else
    (void)index;
#endif
    
    if (playlist->in_ram && !control_file && max < 0)
    {
        strncpy(tmp_buf, &playlist->buffer[seek], sizeof(tmp_buf));
        tmp_buf[MAX_PATH] = '\0';
        max = strlen(tmp_buf) + 1;
    }
    else if (max < 0)
    {
        mutex_lock(&playlist->control_mutex);

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
            
            if (lseek(fd, seek, SEEK_SET) != seek)
                max = -1;
            else
            {
                max = read(fd, tmp_buf, MIN((size_t) buf_length, sizeof(tmp_buf)));
                
                if ((max > 0) && !playlist->utf8)
                {
                    char* end;
                    int i = 0;

                    /* Locate EOL. */
                    while ((tmp_buf[i] != '\n') && (tmp_buf[i] != '\r')
                        && (i < max))
                    {
                        i++;
                    }
                
                    /* Now work back killing white space. */
                    while ((i > 0) && isspace(tmp_buf[i - 1]))
                    {
                        i--;
                    }

                    /* Borrow dir_buf a little... */
                    /* TODO: iso_decode can overflow dir_buf; it really 
                     * should take a dest size argument.
                     */
                    end = iso_decode(tmp_buf, dir_buf, -1, i);
                    *end = 0;
                    strncpy(tmp_buf, dir_buf, sizeof(tmp_buf));
                    tmp_buf[sizeof(tmp_buf) - 1] = 0;
                    max = strlen(tmp_buf);
                }
            }
        }

        mutex_unlock(&playlist->control_mutex);

        if (max < 0)
        {
            if (control_file)
                gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
            else
                gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_ACCESS_ERROR));

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
    int sort_dir = global_settings.sort_dir;
    char *start_dir = NULL;
    bool exit = false;
    struct tree_context* tc = tree_get_context();
    int dirfilter = *(tc->dirfilter);

    if (global_settings.next_folder == FOLDER_ADVANCE_RANDOM)
    {
        int fd = open(ROCKBOX_DIR "/folder_advance_list.dat",O_RDONLY);
        char buffer[MAX_PATH];
        int folder_count = 0,i;
        srand(current_tick);
        *(tc->dirfilter) = SHOW_MUSIC;
        if (fd >= 0)
        {
            read(fd,&folder_count,sizeof(int));
            while (!exit)
            {
                i = rand()%folder_count;
                lseek(fd,sizeof(int) + (MAX_PATH*i),SEEK_SET);
                read(fd,buffer,MAX_PATH);
                if (check_subdir_for_music(buffer,"") ==0)
                    exit = true;
            }
            strcpy(dir,buffer);
            close(fd);
            *(tc->dirfilter) = dirfilter;
            reload_directory();
            return 0;
        }
    }
    /* not random folder advance */
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
    *(tc->dirfilter) = SHOW_ALL;

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
            gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
            exit = true;
            result = -1;
            break;
        }
        
        files = (struct entry*) tc->dircache;
        num_files = tc->filesindir;

        for (i=0; i<num_files; i++)
        {
            /* user abort */
            if (action_userabort(TIMEOUT_NOBLOCK))
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
    *(tc->dirfilter) = dirfilter;
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
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
        return -2;
    }
    
    files = (struct entry*) tc->dircache;
    num_files = tc->filesindir;
    
    for (i=0; i<num_files; i++)
    {
        if (files[i].attr & ATTR_DIRECTORY)
            has_subdir = true;
        else if ((files[i].attr & FILE_ATTR_MASK) == FILE_ATTR_AUDIO)
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
            if (action_userabort(TIMEOUT_NOBLOCK))
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
            gui_syncsplash(HZ*2,
                ID2P(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));        
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
static void display_playlist_count(int count, const unsigned char *fmt,
                                   bool final)
{
    static long talked_tick = 0;
    long id = P2ID(fmt);
    if(global_settings.talk_menu && id>=0)
    {
        if(final || (count && (talked_tick == 0
                               || TIME_AFTER(current_tick, talked_tick+5*HZ))))
        {
            talked_tick = current_tick;
            talk_number(count, false);
            talk_id(id, true);
        }
    }
    fmt = P2STR(fmt);

    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    gui_syncsplash(0, fmt, count, str(LANG_OFF_ABORT));
}

/*
 * Display buffer full message
 */
static void display_buffer_full(void)
{
    gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_BUFFER_FULL));
}

/*
 * Flush any cached control commands to disk.  Called when playlist is being
 * modified.  Returns 0 on success and -1 on failure.
 */
static int flush_cached_control(struct playlist_info* playlist)
{
    int result = 0;
    int i;

    if (!playlist->num_cached)
        return 0;

    lseek(playlist->control_fd, 0, SEEK_END);

    for (i=0; i<playlist->num_cached; i++)
    {
        struct playlist_control_cache* cache =
            &(playlist->control_cache[i]);

        switch (cache->command)
        {
            case PLAYLIST_COMMAND_PLAYLIST:
                result = fdprintf(playlist->control_fd, "P:%d:%s:%s\n",
                    cache->i1, cache->s1, cache->s2);
                break;
            case PLAYLIST_COMMAND_ADD:
            case PLAYLIST_COMMAND_QUEUE:
                result = fdprintf(playlist->control_fd, "%c:%d:%d:",
                    (cache->command == PLAYLIST_COMMAND_ADD)?'A':'Q',
                    cache->i1, cache->i2);
                if (result > 0)
                {
                    /* save the position in file where name is written */
                    int* seek_pos = (int *)cache->data;
                    *seek_pos = lseek(playlist->control_fd, 0, SEEK_CUR);
                    result = fdprintf(playlist->control_fd, "%s\n",
                        cache->s1);
                }
                break;
            case PLAYLIST_COMMAND_DELETE:
                result = fdprintf(playlist->control_fd, "D:%d\n", cache->i1);
                break;
            case PLAYLIST_COMMAND_SHUFFLE:
                result = fdprintf(playlist->control_fd, "S:%d:%d\n",
                    cache->i1, cache->i2);
                break;
            case PLAYLIST_COMMAND_UNSHUFFLE:
                result = fdprintf(playlist->control_fd, "U:%d\n", cache->i1);
                break;
            case PLAYLIST_COMMAND_RESET:
                result = fdprintf(playlist->control_fd, "R\n");
                break;
            default:
                break;
        }

        if (result <= 0)
            break;
    }

    if (result > 0)
    {
        if (global_status.resume_seed >= 0)
        {
            global_status.resume_seed = -1;
            status_save();
        }

        playlist->num_cached = 0;
        playlist->pending_control_sync = true;

        result = 0;
    }
    else
    {
        result = -1;
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
    }

    return result;
}

/*
 * Update control data with new command.  Depending on the command, it may be
 * cached or flushed to disk.
 */
static int update_control(struct playlist_info* playlist,
                          enum playlist_command command, int i1, int i2,
                          const char* s1, const char* s2, void* data)
{
    int result = 0;
    struct playlist_control_cache* cache;
    bool flush = false;

    mutex_lock(&playlist->control_mutex);

    cache = &(playlist->control_cache[playlist->num_cached++]);

    cache->command = command;
    cache->i1 = i1;
    cache->i2 = i2;
    cache->s1 = s1;
    cache->s2 = s2;
    cache->data = data;

    switch (command)
    {
        case PLAYLIST_COMMAND_PLAYLIST:
        case PLAYLIST_COMMAND_ADD:
        case PLAYLIST_COMMAND_QUEUE:
#ifndef HAVE_DIRCACHE
        case PLAYLIST_COMMAND_DELETE:
        case PLAYLIST_COMMAND_RESET:
#endif
            flush = true;
            break;
        case PLAYLIST_COMMAND_SHUFFLE:
        case PLAYLIST_COMMAND_UNSHUFFLE:
        default:
            /* only flush when needed */
            break;
    }

    if (flush || playlist->num_cached == PLAYLIST_MAX_CACHE)
        result = flush_cached_control(playlist);

    mutex_unlock(&playlist->control_mutex);
        
    return result;
}

/*
 * sync control file to disk
 */
static void sync_control(struct playlist_info* playlist, bool force)
{
#ifdef HAVE_DIRCACHE
    if (playlist->started && force)
#else
    (void) force;

    if (playlist->started)
#endif
    {
        if (playlist->pending_control_sync)
        {
            mutex_lock(&playlist->control_mutex);
            fsync(playlist->control_fd);
            playlist->pending_control_sync = false;
            mutex_unlock(&playlist->control_mutex);
        }
    }
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

#ifdef HAVE_DIRCACHE
    playlist->filenames = buffer_alloc(
        playlist->max_playlist_size * sizeof(int));
    memset(playlist->filenames, 0,
           playlist->max_playlist_size * sizeof(int));
    create_thread(playlist_thread, playlist_stack, sizeof(playlist_stack),
                  0, playlist_thread_name IF_PRIO(, PRIORITY_BACKGROUND)
		          IF_COP(, CPU));
    queue_init(&playlist_queue, true);
#endif
}

/*
 * Clean playlist at shutdown
 */
void playlist_shutdown(void)
{
    struct playlist_info* playlist = &current_playlist;

    if (playlist->control_fd >= 0)
    {
        mutex_lock(&playlist->control_mutex);

        if (playlist->num_cached > 0)
            flush_cached_control(playlist);

        close(playlist->control_fd);

        mutex_unlock(&playlist->control_mutex);
    }
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
    size_t buflen;
    int nread;
    int total_read = 0;
    int control_file_size = 0;
    bool first = true;
    bool sorted = true;

    /* use mp3 buffer for maximum load speed */
#if CONFIG_CODEC != SWCODEC
    talk_buffer_steal(); /* we use the mp3 buffer, need to tell */
    buflen = (audiobufend - audiobuf);
    buffer = (char *)audiobuf;
#else
    buffer = (char *)audio_get_buffer(false, &buflen);
#endif

    empty_playlist(playlist, true);

    gui_syncsplash(0, ID2P(LANG_WAIT));
    playlist->control_fd = open(playlist->control_filename, O_RDWR);
    if (playlist->control_fd < 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }
    playlist->control_created = true;

    control_file_size = filesize(playlist->control_fd);
    if (control_file_size <= 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    /* read a small amount first to get the header */
    nread = read(playlist->control_fd, buffer,
        PLAYLIST_COMMAND_SIZE<buflen?PLAYLIST_COMMAND_SIZE:buflen);
    if(nread <= 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    playlist->started = true;

    while (1)
    {
        int result = 0;
        int count;
        enum playlist_command current_command = PLAYLIST_COMMAND_COMMENT;
        int last_newline = 0;
        int str_count = -1;
        bool newline = true;
        bool exit_loop = false;
        char *p = buffer;
        char *str1 = NULL;
        char *str2 = NULL;
        char *str3 = NULL;
        unsigned long last_tick = current_tick;
        
        for(count=0; count<nread && !exit_loop; count++,p++)
        {
            /* So a splash while we are loading. */
            if (current_tick - last_tick > HZ/4)
            {
                gui_syncsplash(0, str(LANG_LOADING_PERCENT), 
                               (total_read+count)*100/control_file_size,
                               str(LANG_OFF_ABORT));
                if (action_userabort(TIMEOUT_NOBLOCK))
                {
                    /* FIXME: 
                     * Not sure how to implement this, somebody more familiar
                     * with the code, please fix this. */
                }
                last_tick = current_tick;
            }
            
            /* Are we on a new line? */
            if((*p == '\n') || (*p == '\r'))
            {
                *p = '\0';

                /* save last_newline in case we need to load more data */
                last_newline = count;

                switch (current_command)
                {
                    case PLAYLIST_COMMAND_PLAYLIST:
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
                    case PLAYLIST_COMMAND_ADD:
                    case PLAYLIST_COMMAND_QUEUE:
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
                        
                        queue = (current_command == PLAYLIST_COMMAND_ADD)?
                            false:true;

                        /* seek position is based on str3's position in
                           buffer */
                        if (add_track_to_playlist(playlist, str3, position,
                                queue, total_read+(str3-buffer)) < 0)
                        return -1;
                        
                        playlist->last_insert_pos = last_position;

                        break;
                    }
                    case PLAYLIST_COMMAND_DELETE:
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
                    case PLAYLIST_COMMAND_SHUFFLE:
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
                    case PLAYLIST_COMMAND_UNSHUFFLE:
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
                    case PLAYLIST_COMMAND_RESET:
                    {
                        playlist->last_insert_pos = -1;
                        break;
                    }
                    case PLAYLIST_COMMAND_COMMENT:
                    default:
                        break;
                }

                newline = true;

                /* to ignore any extra newlines */
                current_command = PLAYLIST_COMMAND_COMMENT;
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

                        current_command = PLAYLIST_COMMAND_PLAYLIST;
                        break;
                    case 'A':
                        current_command = PLAYLIST_COMMAND_ADD;
                        break;
                    case 'Q':
                        current_command = PLAYLIST_COMMAND_QUEUE;
                        break;
                    case 'D':
                        current_command = PLAYLIST_COMMAND_DELETE;
                        break;
                    case 'S':
                        current_command = PLAYLIST_COMMAND_SHUFFLE;
                        break;
                    case 'U':
                        current_command = PLAYLIST_COMMAND_UNSHUFFLE;
                        break;
                    case 'R':
                        current_command = PLAYLIST_COMMAND_RESET;
                        break;
                    case '#':
                        current_command = PLAYLIST_COMMAND_COMMENT;
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
            else if(current_command != PLAYLIST_COMMAND_COMMENT)
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
            gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_INVALID));
            return result;
        }

        if (!newline || (exit_loop && count<nread))
        {
            if ((total_read + count) >= control_file_size)
            {
                /* no newline at end of control file */
                gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_INVALID));
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
            if (global_status.resume_seed >= 0)
            {
                /* Apply shuffle command saved in settings */
                if (global_status.resume_seed == 0)
                    sort_playlist(playlist, false, true);
                else
                {
                    if (!sorted)
                        sort_playlist(playlist, false, false);

                    randomise_playlist(playlist, global_status.resume_seed,
                        false, true);
                }
            }

            playlist->first_index = global_status.resume_first_index;
            break;
        }
    }

#ifdef HAVE_DIRCACHE
    queue_post(&playlist_queue, PLAYLIST_LOAD_POINTERS, 0);
#endif

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

    playlist->indices[playlist->amount] = playlist->buffer_end_pos;
#ifdef HAVE_DIRCACHE
    playlist->filenames[playlist->amount] = NULL;
#endif
    playlist->amount++;
    
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
        playlist->index = global_status.resume_first_index =
            playlist->first_index = start_index;
        start_current = true;
    }
    
    randomise_playlist(playlist, random_seed, start_current, true);

    return playlist->index;
}

/* start playing current playlist at specified index/offset */
int playlist_start(int start_index, int offset)
{
    struct playlist_info* playlist = &current_playlist;

    /* Cancel FM radio selection as previous music. For cases where we start
       playback without going to the WPS, such as playlist insert.. or
       playlist catalog. */
    previous_music_is_wps();

    playlist->index = start_index;

#if CONFIG_CODEC != SWCODEC
    talk_buffer_steal(); /* will use the mp3 buffer */
#endif

    playlist->started = true;
    sync_control(playlist, false);
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

    if (get_filename(playlist, index, seek, control_file, now_playing,
            MAX_PATH+1) < 0)
        return NULL;

    temp_ptr = now_playing;

    if (!playlist->in_ram || control_file)
    {
        /* remove bogus dirs from beginning of path
           (workaround for buggy playlist creation tools) */
        while (temp_ptr)
        {
#ifdef HAVE_DIRCACHE
            if (dircache_is_enabled())
            {
                if (dircache_get_entry_ptr(temp_ptr))
                    break;
            }
            else
#endif
            {
                fd = open(temp_ptr, O_RDONLY);
                if (fd >= 0)
                {
                    close(fd);
                    break;
                }
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
            playlist->amount > 1)
        {
            /* Repeat shuffle mode.  Re-shuffle playlist and resume play */
            playlist->first_index = global_status.resume_first_index = 0;
            sort_playlist(playlist, false, false);
            randomise_playlist(playlist, current_tick, false, true);
#if CONFIG_CODEC != SWCODEC
            playlist_start(0, 0);
#endif
            playlist->index = 0;
            index = 0;
        }
        else if (playlist->in_ram && global_settings.next_folder)
        {
            char dir[MAX_PATH+1];

            changing_dir = true;
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
#if CONFIG_CODEC != SWCODEC
                        playlist_start(0, 0);
#endif
                        playlist->index = index = 0;
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
#if CONFIG_CODEC != SWCODEC
                        playlist_start(current_playlist.amount-1, 0);
#endif
                        playlist->index = index = current_playlist.amount - 1;
                    }
                }
            }
            changing_dir = false;
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
                int result = update_control(playlist, PLAYLIST_COMMAND_RESET,
                    -1, -1, NULL, NULL, NULL);

                if (result < 0)
                    return result;

                sync_control(playlist, false);
            }
        }
    }

    return index;
}

/* try playing next or previous folder */
bool playlist_next_dir(int direction)
{
    char dir[MAX_PATH+1];
    bool result;
    int res;

    /* not to mess up real playlists */
    if(!current_playlist.in_ram)
       return false;

    if(changing_dir)
       return false;

    changing_dir = true;
    if(direction > 0)
      res = get_next_directory(dir);
    else
      res = get_previous_directory(dir);
    if (!res)
    {
        if (playlist_create(dir, NULL) != -1)
        {
            ft_build_playlist(tree_get_context(), 0);
            if (global_settings.playlist_shuffle)
                 playlist_shuffle(current_tick, -1);
#if (CONFIG_CODEC != SWCODEC)
            playlist_start(0,0);
#endif
            result = true;
        }
        else
            result = false;
    }
    else
        result = false;

    changing_dir = false;

    return result;    
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
        if (global_status.resume_index != playlist->index ||
            global_status.resume_offset != id3->offset)
        {
            global_status.resume_index = playlist->index;
            global_status.resume_offset = id3->offset;
            status_save();
        }
    }
    else
    {
        global_status.resume_index = -1;
        global_status.resume_offset = -1;
        status_save();
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

#ifdef HAVE_DIRCACHE
            num_indices /= 2;
#endif
            if (num_indices > global_settings.max_files_in_playlist)
                num_indices = global_settings.max_files_in_playlist;

            playlist->max_playlist_size = num_indices;
            playlist->indices = index_buffer;
#ifdef HAVE_DIRCACHE
            playlist->filenames = (const struct dircache_entry **)
                &playlist->indices[num_indices];
#endif
        }
        else
        {
            playlist->max_playlist_size = current_playlist.max_playlist_size;
            playlist->indices = current_playlist.indices;
#ifdef HAVE_DIRCACHE
            playlist->filenames = current_playlist.filenames;
#endif
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

    current_playlist.utf8 = playlist->utf8;
    current_playlist.fd = playlist->fd;

    close(playlist->control_fd);
    close(current_playlist.control_fd);
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
    {
        memcpy(current_playlist.indices, playlist->indices,
               playlist->max_playlist_size*sizeof(int));
#ifdef HAVE_DIRCACHE
        memcpy(current_playlist.filenames, playlist->filenames,
               playlist->max_playlist_size*sizeof(int));
#endif
    }
    
    current_playlist.first_index = playlist->first_index;
    current_playlist.amount = playlist->amount;
    current_playlist.last_insert_pos = playlist->last_insert_pos;
    current_playlist.seed = playlist->seed;
    current_playlist.shuffle_modified = playlist->shuffle_modified;
    current_playlist.deleted = playlist->deleted;
    current_playlist.num_inserted_tracks = playlist->num_inserted_tracks;
    
    memcpy(current_playlist.control_cache, playlist->control_cache,
        sizeof(current_playlist.control_cache));
    current_playlist.num_cached = playlist->num_cached;
    current_playlist.pending_control_sync = playlist->pending_control_sync;

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

void playlist_sync(struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;
    
    sync_control(playlist, false);
    if ((audio_status() & AUDIO_STATUS_PLAY) && playlist->started)
        audio_flush_and_reload_tracks();

#ifdef HAVE_DIRCACHE
    queue_post(&playlist_queue, PLAYLIST_LOAD_POINTERS, 0);
#endif
}

/*
 * Insert track into playlist at specified position (or one of the special
 * positions).  Returns position where track was inserted or -1 if error.
 */
int playlist_insert_track(struct playlist_info* playlist, const char *filename,
                          int position, bool queue, bool sync)
{
    int result;
    
    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    result = add_track_to_playlist(playlist, filename, position, queue, -1);

    /* Check if we want manually sync later. For example when adding
     * bunch of files from tagcache, syncing after every file wouldn't be
     * a good thing to do. */
    if (sync && result >= 0)
        playlist_sync(playlist);

    return result;
}

/*
 * Insert all tracks from specified directory into playlist.
 */
int playlist_insert_directory(struct playlist_info* playlist,
                              const char *dirname, int position, bool queue,
                              bool recurse)
{
    int result;
    unsigned char *count_str;
    struct directory_search_context context;

    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    if (position == PLAYLIST_REPLACE)
    {
        if (remove_all_tracks(playlist) == 0)
            position = PLAYLIST_INSERT_LAST;
        else
            return -1;
    }

    if (queue)
        count_str = ID2P(LANG_PLAYLIST_QUEUE_COUNT);
    else
      count_str = ID2P(LANG_PLAYLIST_INSERT_COUNT);

    display_playlist_count(0, count_str, false);

    context.playlist = playlist;
    context.position = position;
    context.queue = queue;
    context.count = 0;
    
    cpu_boost(true);

    result = playlist_directory_tracksearch(dirname, recurse,
        directory_search_callback, &context);

    sync_control(playlist, false);

    cpu_boost(false);

    display_playlist_count(context.count, count_str, true);

    if ((audio_status() & AUDIO_STATUS_PLAY) && playlist->started)
        audio_flush_and_reload_tracks();

#ifdef HAVE_DIRCACHE
    queue_post(&playlist_queue, PLAYLIST_LOAD_POINTERS, 0);
#endif

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
    unsigned char *count_str;
    char temp_buf[MAX_PATH+1];
    char trackname[MAX_PATH+1];
    int count = 0;
    int result = 0;

    if (!playlist)
        playlist = &current_playlist;

    if (check_control(playlist) < 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_ACCESS_ERROR));
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
        count_str = ID2P(LANG_PLAYLIST_QUEUE_COUNT);
    else
        count_str = ID2P(LANG_PLAYLIST_INSERT_COUNT);

    display_playlist_count(count, count_str, false);

    if (position == PLAYLIST_REPLACE)
    {
        if (remove_all_tracks(playlist) == 0)
            position = PLAYLIST_INSERT_LAST;
        else return -1;
    }

    cpu_boost(true);

    while ((max = read_line(fd, temp_buf, sizeof(temp_buf))) > 0)
    {
        /* user abort */
        if (action_userabort(TIMEOUT_NOBLOCK))
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
                display_playlist_count(count, count_str, false);

                if (count == PLAYLIST_DISPLAY_COUNT &&
                    (audio_status() & AUDIO_STATUS_PLAY) &&
                    playlist->started)
                    audio_flush_and_reload_tracks();
            }
        }

        /* let the other threads work */
        yield();
    }

    close(fd);

    if (temp_ptr)
        *temp_ptr = '/';

    sync_control(playlist, false);

    cpu_boost(false);

    display_playlist_count(count, count_str, true);

    if ((audio_status() & AUDIO_STATUS_PLAY) && playlist->started)
        audio_flush_and_reload_tracks();

#ifdef HAVE_DIRCACHE
    queue_post(&playlist_queue, PLAYLIST_LOAD_POINTERS, 0);
#endif

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
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    if (index == PLAYLIST_DELETE_CURRENT)
        index = playlist->index;

    result = remove_track_from_playlist(playlist, index, true);
    
    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY) &&
        playlist->started)
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
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
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

    if (get_filename(playlist, index, seek, control_file, filename,
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

            if ((audio_status() & AUDIO_STATUS_PLAY) && playlist->started)
                audio_flush_and_reload_tracks();
        }
    }

#ifdef HAVE_DIRCACHE
    queue_post(&playlist_queue, PLAYLIST_LOAD_POINTERS, 0);
#endif

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

    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY) &&
        playlist->started)
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

    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY) &&
        playlist->started)
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

    if (get_filename(playlist, index, seek, control_file, info->filename,
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
    char path[MAX_PATH+1];
    char tmp_buf[MAX_PATH+1];
    int result = 0;
    bool overwrite_current = false;
    int* index_buf = NULL;

    if (!playlist)
        playlist = &current_playlist;

    if (playlist->amount <= 0)
        return -1;

    /* use current working directory as base for pathname */
    if (format_track_path(path, filename, sizeof(tmp_buf),
                          strlen(filename)+1, getcwd(NULL, -1)) < 0)
        return -1;

    if (!strncmp(playlist->filename, path, strlen(path)))
    {
        /* Attempting to overwrite current playlist file.*/

        if (playlist->buffer_size < (int)(playlist->amount * sizeof(int)))
        {
            /* not enough buffer space to store updated indices */
            gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_ACCESS_ERROR));
            return -1;
        }

        /* in_ram buffer is unused for m3u files so we'll use for storing
           updated indices */
        index_buf = (int*)playlist->buffer;

        /* use temporary pathname */
        snprintf(path, sizeof(path), "%s_temp", playlist->filename);
        overwrite_current = true;
    }

    fd = open(path, O_CREAT|O_WRONLY|O_TRUNC);
    if (fd < 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_ACCESS_ERROR));
        return -1;
    }

    display_playlist_count(count, ID2P(LANG_PLAYLIST_SAVE_COUNT), false);

    cpu_boost(true);

    index = playlist->first_index;
    for (i=0; i<playlist->amount; i++)
    {
        bool control_file;
        bool queue;
        int seek;

        /* user abort */
        if (action_userabort(TIMEOUT_NOBLOCK))
        {
            result = -1;
            break;
        }

        control_file = playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK;
        queue = playlist->indices[index] & PLAYLIST_QUEUE_MASK;
        seek = playlist->indices[index] & PLAYLIST_SEEK_MASK;

        /* Don't save queued files */
        if (!queue)
        {
            if (get_filename(playlist, index, seek, control_file, tmp_buf,
                    MAX_PATH+1) < 0)
            {
                result = -1;
                break;
            }

            if (overwrite_current)
                index_buf[count] = lseek(fd, 0, SEEK_CUR);

            if (fdprintf(fd, "%s\n", tmp_buf) < 0)
            {
                gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_ACCESS_ERROR));
                result = -1;
                break;
            }

            count++;

            if ((count % PLAYLIST_DISPLAY_COUNT) == 0)
                display_playlist_count(count, ID2P(LANG_PLAYLIST_SAVE_COUNT),
                                       false);

            yield();
        }

        index = (index+1)%playlist->amount;
    }

    display_playlist_count(count, ID2P(LANG_PLAYLIST_SAVE_COUNT), true);

    close(fd);

    if (overwrite_current && result >= 0)
    {
        result = -1;

        mutex_lock(&playlist->control_mutex);

        /* Replace the current playlist with the new one and update indices */
        close(playlist->fd);
        if (remove(playlist->filename) >= 0)
        {
            if (rename(path, playlist->filename) >= 0)
            {
                playlist->fd = open(playlist->filename, O_RDONLY);
                if (playlist->fd >= 0)
                {
                    index = playlist->first_index;
                    for (i=0, count=0; i<playlist->amount; i++)
                    {
                        if (!(playlist->indices[index] & PLAYLIST_QUEUE_MASK))
                        {
                            playlist->indices[index] = index_buf[count];
                            count++;
                        }
                        index = (index+1)%playlist->amount;
                    }

                    /* we need to recreate control because inserted tracks are
                       now part of the playlist and shuffle has been
                       invalidated */
                    result = recreate_control(playlist);
                }
            }
       }

       mutex_unlock(&playlist->control_mutex);

    }

    cpu_boost(false);

    return result;
}

/*
 * Search specified directory for tracks and notify via callback.  May be
 * called recursively.
 */
int playlist_directory_tracksearch(const char* dirname, bool recurse,
                                   int (*callback)(char*, void*),
                                   void* context)
{
    char buf[MAX_PATH+1];
    int result = 0;
    int num_files = 0;
    int i;
    struct entry *files;
    struct tree_context* tc = tree_get_context();
    int old_dirfilter = *(tc->dirfilter);

    if (!callback)
        return -1;

    /* use the tree browser dircache to load files */
    *(tc->dirfilter) = SHOW_ALL;

    if (ft_load(tc, dirname) < 0)
    {
        gui_syncsplash(HZ*2, ID2P(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
        *(tc->dirfilter) = old_dirfilter;
        return -1;
    }

    files = (struct entry*) tc->dircache;
    num_files = tc->filesindir;

    /* we've overwritten the dircache so tree browser will need to be
       reloaded */
    reload_directory();

    for (i=0; i<num_files; i++)
    {
        /* user abort */
        if (action_userabort(TIMEOUT_NOBLOCK))
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
                result = playlist_directory_tracksearch(buf, recurse,
                    callback, context);
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
        else if ((files[i].attr & FILE_ATTR_MASK) == FILE_ATTR_AUDIO)
        {
            snprintf(buf, sizeof(buf), "%s/%s", dirname, files[i].name);

            if (callback(buf, context) != 0)
            {
                result = -1;
                break;
            }

            /* let the other threads work */
            yield();
        }
    }

    /* restore dirfilter */
    *(tc->dirfilter) = old_dirfilter;

    return result;
}
