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
      exactly the same as before shutdown.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playlist.h"
#include "file.h"
#include "dir.h"
#include "sprintf.h"
#include "debug.h"
#include "mpeg.h"
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
#include "tree.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

#include "lang.h"

static struct playlist_info playlist;

#define PLAYLIST_CONTROL_FILE ROCKBOX_DIR "/.playlist_control"
#define PLAYLIST_CONTROL_FILE_VERSION 1

/*
    Each playlist index has a flag associated with it which identifies what
    type of track it is.  These flags are stored in the 3 high order bits of
    the index.
    
    NOTE: This limits the playlist file size to a max of 512K.

    Bits 31-30:
        00 = Playlist track
        01 = Track was prepended into playlist
        10 = Track was inserted into playlist
        11 = Track was appended into playlist
    Bit 29:
        0 = Added track
        1 = Queued track
 */
#define PLAYLIST_SEEK_MASK              0x1FFFFFFF
#define PLAYLIST_INSERT_TYPE_MASK       0xC0000000
#define PLAYLIST_QUEUE_MASK             0x20000000

#define PLAYLIST_INSERT_TYPE_PREPEND    0x40000000
#define PLAYLIST_INSERT_TYPE_INSERT     0x80000000
#define PLAYLIST_INSERT_TYPE_APPEND     0xC0000000

#define PLAYLIST_QUEUED                 0x20000000

#define PLAYLIST_DISPLAY_COUNT          10

static char now_playing[MAX_PATH+1];

static void empty_playlist(bool resume);
static void update_playlist_filename(char *dir, char *file);
static int add_indices_to_playlist(void);
static int add_track_to_playlist(char *filename, int position, bool queue,
                                 int seek_pos);
static int add_directory_to_playlist(char *dirname, int *position, bool queue,
                                     int *count, bool recurse);
static int remove_track_from_playlist(int position, bool write);
static int randomise_playlist(unsigned int seed, bool start_current,
                              bool write);
static int sort_playlist(bool start_current, bool write);
static int get_next_index(int steps);
static void find_and_set_playlist_index(unsigned int seek);
static int compare(const void* p1, const void* p2);
static int get_filename(int seek, bool control_file, char *buf,
                        int buf_length);
static int format_track_path(char *dest, char *src, int buf_length, int max,
                             char *dir);
static void display_playlist_count(int count, char *fmt);
static void display_buffer_full(void);

/*
 * remove any files and indices associated with the playlist
 */
static void empty_playlist(bool resume)
{
    playlist.filename[0] = '\0';

    if(-1 != playlist.fd)
        /* If there is an already open playlist, close it. */
        close(playlist.fd);
    playlist.fd = -1;

    if(-1 != playlist.control_fd)
        close(playlist.control_fd);
    playlist.control_fd = -1;

    playlist.in_ram = false;
    playlist.buffer[0] = 0;
    playlist.buffer_end_pos = 0;

    playlist.index = 0;
    playlist.first_index = 0;
    playlist.amount = 0;
    playlist.last_insert_pos = -1;

    if (!resume)
    {
        int fd;

        /* start with fresh playlist control file when starting new
           playlist */
        fd = creat(PLAYLIST_CONTROL_FILE, 0000200);
        if (fd >= 0)
            close(fd);
    }
}

/*
 * store directory and name of playlist file
 */
static void update_playlist_filename(char *dir, char *file)
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
    
    playlist.dirlen = dirlen;
    
    snprintf(playlist.filename, sizeof(playlist.filename),
        "%s%s%s", 
        dir, sep, file);
}

/*
 * calculate track offsets within a playlist file
 */
static int add_indices_to_playlist(void)
{
    unsigned int nread;
    unsigned int i = 0;
    unsigned int count = 0;
    int buflen;
    bool store_index;
    char *buffer;
    unsigned char *p;

    if(-1 == playlist.fd)
        playlist.fd = open(playlist.filename, O_RDONLY);
    if(-1 == playlist.fd)
        return -1; /* failure */
    
#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    splash(0, 0, true, str(LANG_PLAYLIST_LOAD));

    /* use mp3 buffer for maximum load speed */
    buflen = (mp3end - mp3buf);
    buffer = mp3buf;

    store_index = true;

    mpeg_stop();
    
    while(1)
    {
        nread = read(playlist.fd, buffer, buflen);
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
                    playlist.indices[ playlist.amount ] = i+count;
                    playlist.amount++;
                    if ( playlist.amount >= playlist.max_playlist_size ) {
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
 * Add track to playlist at specified position.  There are three special
 * positions that can be specified:
 *     PLAYLIST_PREPEND      - Add track at beginning of playlist
 *     PLAYLIST_INSERT       - Add track after current song.  NOTE: If there
 *                             are already inserted tracks then track is added
 *                             to the end of the insertion list.
 *     PLAYLIST_INSERT_FIRST - Add track immediately after current song, no
 *                             matter what other tracks have been inserted.
 *     PLAYLIST_INSERT_LAST  - Add track to end of playlist
 */
static int add_track_to_playlist(char *filename, int position, bool queue,
                                 int seek_pos)
{
    int insert_position = position;
    unsigned int flags = PLAYLIST_INSERT_TYPE_INSERT;
    int i;

    if (playlist.amount >= playlist.max_playlist_size)
    {
        display_buffer_full();
        return -1;
    }

    switch (position)
    {
        case PLAYLIST_PREPEND:
            insert_position = playlist.first_index;
            flags = PLAYLIST_INSERT_TYPE_PREPEND;
            break;
        case PLAYLIST_INSERT:
            /* if there are already inserted tracks then add track to end of
               insertion list else add after current playing track */
            if (playlist.last_insert_pos >= 0 &&
                playlist.last_insert_pos < playlist.amount &&
                (playlist.indices[playlist.last_insert_pos]&
                    PLAYLIST_INSERT_TYPE_MASK) == PLAYLIST_INSERT_TYPE_INSERT)
                position = insert_position = playlist.last_insert_pos+1;
            else if (playlist.amount > 0)
                position = insert_position = playlist.index + 1;
            else
                position = insert_position = 0;

            playlist.last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_FIRST:
            if (playlist.amount > 0)
                position = insert_position = playlist.index + 1;
            else
                position = insert_position = 0;

            if (playlist.last_insert_pos < 0)
                playlist.last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_LAST:
            if (playlist.first_index > 0)
                insert_position = playlist.first_index;
            else
                insert_position = playlist.amount;

            flags = PLAYLIST_INSERT_TYPE_APPEND;
            break;
    }
    
    if (queue)
        flags |= PLAYLIST_QUEUED;

    /* shift indices so that track can be added */
    for (i=playlist.amount; i>insert_position; i--)
        playlist.indices[i] = playlist.indices[i-1];

    /* update stored indices if needed */
    if (playlist.amount > 0 && insert_position <= playlist.index)
        playlist.index++;

    if (playlist.amount > 0 && insert_position <= playlist.first_index &&
        position != PLAYLIST_PREPEND)
        playlist.first_index++;

    if (insert_position < playlist.last_insert_pos ||
        (insert_position == playlist.last_insert_pos && position < 0))
        playlist.last_insert_pos++;

    if (seek_pos < 0 && playlist.control_fd >= 0)
    {
        int result = -1;

        mutex_lock(&playlist.control_mutex);

        if (lseek(playlist.control_fd, 0, SEEK_END) >= 0)
        {
            if (fprintf(playlist.control_fd, "%c:%d:%d:", (queue?'Q':'A'),
                    position, playlist.last_insert_pos) > 0)
            {
                /* save the position in file where track name is written */
                seek_pos = lseek(playlist.control_fd, 0, SEEK_CUR);

                if (fprintf(playlist.control_fd, "%s\n", filename) > 0)
                    result = 0;
            }
        }

        mutex_unlock(&playlist.control_mutex);

        if (result < 0)
        {
            splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
            return result;
        }
    }

    playlist.indices[insert_position] = flags | seek_pos;

    playlist.amount++;

    return insert_position;
}

/*
 * Insert directory into playlist.  May be called recursively.
 */
static int add_directory_to_playlist(char *dirname, int *position, bool queue,
                                     int *count, bool recurse)
{
    char buf[MAX_PATH+1];
    char *count_str;
    int result = 0;
    int num_files = 0;
    bool buffer_full = false;
    int i;
    struct entry *files;

    /* use the tree browser dircache to load files */
    files = load_and_sort_directory(dirname, SHOW_ALL, &num_files,
        &buffer_full);

    if(!files)
    {
        splash(HZ*2, 0, true, str(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
        return 0;
    }

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
#ifdef HAVE_PLAYER_KEYPAD
        if (button_get(false) == BUTTON_STOP)
#else
        if (button_get(false) == BUTTON_OFF)
#endif
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
                result = add_directory_to_playlist(buf, position, queue,
                    count, recurse);
                if (result < 0)
                    break;

                /* we now need to reload our current directory */
                files = load_and_sort_directory(dirname, SHOW_ALL, &num_files,
                    &buffer_full);
                if (!files)
                {
                    result = -1;
                    break;
                }
            }
            else
                continue;
        }
        else if (files[i].attr & TREE_ATTR_MPA)
        {
            int insert_pos;

            snprintf(buf, sizeof(buf), "%s/%s", dirname, files[i].name);
            
            insert_pos = add_track_to_playlist(buf, *position, queue, -1);
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

                if (*count == PLAYLIST_DISPLAY_COUNT)
                    mpeg_flush_and_reload_tracks();
            }
            
            /* let the other threads work */
            yield();
        }
    }

    return result;
}

/*
 * remove track at specified position
 */
static int remove_track_from_playlist(int position, bool write)
{
    int i;

    if (playlist.amount <= 0)
        return -1;

    /* shift indices now that track has been removed */
    for (i=position; i<playlist.amount; i++)
        playlist.indices[i] = playlist.indices[i+1];

    playlist.amount--;

    /* update stored indices if needed */
    if (position < playlist.index)
        playlist.index--;

    if (position < playlist.first_index)
        playlist.first_index--;

    if (position <= playlist.last_insert_pos)
        playlist.last_insert_pos--;

    if (write && playlist.control_fd >= 0)
    {
        int result = -1;

        mutex_lock(&playlist.control_mutex);

        if (lseek(playlist.control_fd, 0, SEEK_END) >= 0)
        {
            if (fprintf(playlist.control_fd, "D:%d\n", position) > 0)
            {
                fsync(playlist.control_fd);
                result = 0;
            }
        }

        mutex_unlock(&playlist.control_mutex);

        if (result < 0)
        {
            splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
            return result;
        }
    }

    return 0;
}

/*
 * randomly rearrange the array of indices for the playlist.  If start_current
 * is true then update the index to the new index of the current playing track
 */
static int randomise_playlist(unsigned int seed, bool start_current, bool write)
{
    int count;
    int candidate;
    int store;
    unsigned int current = playlist.indices[playlist.index];
    
    /* seed with the given seed */
    srand(seed);

    /* randomise entire indices list */
    for(count = playlist.amount - 1; count >= 0; count--)
    {
        /* the rand is from 0 to RAND_MAX, so adjust to our value range */
        candidate = rand() % (count + 1);

        /* now swap the values at the 'count' and 'candidate' positions */
        store = playlist.indices[candidate];
        playlist.indices[candidate] = playlist.indices[count];
        playlist.indices[count] = store;
    }

    if (start_current)
        find_and_set_playlist_index(current);

    /* indices have been moved so last insert position is no longer valid */
    playlist.last_insert_pos = -1;

    if (write && playlist.control_fd >= 0)
    {
        int result = -1;

        mutex_lock(&playlist.control_mutex);

        if (lseek(playlist.control_fd, 0, SEEK_END) >= 0)
        {
            if (fprintf(playlist.control_fd, "S:%d:%d\n", seed,
                    playlist.first_index) > 0)
            {
                fsync(playlist.control_fd);
                result = 0;
            }
        }

        mutex_unlock(&playlist.control_mutex);

        if (result < 0)
        {
            splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
            return result;
        }
    }

    return 0;
}

/*
 * Sort the array of indices for the playlist. If start_current is true then
 * set the index to the new index of the current song.
 */
static int sort_playlist(bool start_current, bool write)
{
    unsigned int current = playlist.indices[playlist.index];

    if (playlist.amount > 0)
        qsort(playlist.indices, playlist.amount, sizeof(playlist.indices[0]),
            compare);

    if (start_current)
        find_and_set_playlist_index(current);

    /* indices have been moved so last insert position is no longer valid */
    playlist.last_insert_pos = -1;

    if (write && playlist.control_fd >= 0)
    {
        int result = -1;

        mutex_lock(&playlist.control_mutex);

        if (lseek(playlist.control_fd, 0, SEEK_END) >= 0)
        {
            if (fprintf(playlist.control_fd, "U:%d\n", playlist.first_index) >
                    0)
            {
                fsync(playlist.control_fd);
                result = 0;
            }
        }

        mutex_unlock(&playlist.control_mutex);

        if (result < 0)
        {
            splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
            return result;
        }
    }

    return 0;
}

/*
 * returns the index of the track that is "steps" away from current playing
 * track.
 */
static int get_next_index(int steps)
{
    int current_index = playlist.index;
    int next_index    = -1;

    if (playlist.amount <= 0)
        return -1;

    switch (global_settings.repeat_mode)
    {
        case REPEAT_OFF:
        {
            /* Rotate indices such that first_index is considered index 0 to
               simplify next calculation */
            current_index -= playlist.first_index;
            if (current_index < 0)
                current_index += playlist.amount;
            
            next_index = current_index+steps;
            if ((next_index < 0) || (next_index >= playlist.amount))
                next_index = -1;
            else
                next_index = (next_index+playlist.first_index) %
                    playlist.amount;

            break;
        }

        case REPEAT_ONE:
            next_index = current_index;
            break;

        case REPEAT_ALL:
        default:
        {
            next_index = (current_index+steps) % playlist.amount;
            while (next_index < 0)
                next_index += playlist.amount;

            if (steps >= playlist.amount)
            {
                int i, index;

                index = next_index;
                next_index = -1;

                /* second time around so skip the queued files */
                for (i=0; i<playlist.amount; i++)
                {
                    if (playlist.indices[index] & PLAYLIST_QUEUE_MASK)
                        index = (index+1) % playlist.amount;
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

    return next_index;
}

/*
 * Search for the seek track and set appropriate indices.  Used after shuffle
 * to make sure the current index is still pointing to correct track.
 */
static void find_and_set_playlist_index(unsigned int seek)
{
    int i;
    
    /* Set the index to the current song */
    for (i=0; i<playlist.amount; i++)
    {
        if (playlist.indices[i] == seek)
        {
            playlist.index = playlist.first_index = i;
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
    unsigned int* e1 = (unsigned int*) p1;
    unsigned int* e2 = (unsigned int*) p2;
    unsigned int flags1 = *e1 & PLAYLIST_INSERT_TYPE_MASK;
    unsigned int flags2 = *e2 & PLAYLIST_INSERT_TYPE_MASK;

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
static int get_filename(int seek, bool control_file, char *buf,
                        int buf_length)
{
    int fd;
    int max = -1;
    char tmp_buf[MAX_PATH+1];
    char dir_buf[MAX_PATH+1];

    if (buf_length > MAX_PATH+1)
        buf_length = MAX_PATH+1;

    if (playlist.in_ram && !control_file)
    {
        strncpy(tmp_buf, &playlist.buffer[seek], sizeof(tmp_buf));
        tmp_buf[MAX_PATH] = '\0';
        max = strlen(tmp_buf) + 1;
    }
    else
    {
        if (control_file)
            fd = playlist.control_fd;
        else
        {
            if(-1 == playlist.fd)
                playlist.fd = open(playlist.filename, O_RDONLY);
            
            fd = playlist.fd;
        }
        
        if(-1 != fd)
        {
            if (control_file)
                mutex_lock(&playlist.control_mutex);
            
            lseek(fd, seek, SEEK_SET);
            max = read(fd, tmp_buf, buf_length);
            
            if (control_file)
                mutex_unlock(&playlist.control_mutex);            
        }

        if (max < 0)
        {
            if (control_file)
                splash(HZ*2, 0, true,
                    str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
            else
                splash(HZ*2, 0, true, str(LANG_PLAYLIST_ACCESS_ERROR));

            return max;
        }
    }

    strncpy(dir_buf, playlist.filename, playlist.dirlen-1);
    dir_buf[playlist.dirlen-1] = 0;

    return (format_track_path(buf, tmp_buf, buf_length, max, dir_buf));
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
        else if ('.' == src[0] && '.' == src[1] && '/' == src[2])
        {
            /* handle relative paths */
            i=3;
            while(src[i] == '.' &&
                  src[i] == '.' &&
                  src[i] == '/')
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
static void display_playlist_count(int count, char *fmt)
{
    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

#ifdef HAVE_PLAYER_KEYPAD
    splash(0, 0, true, fmt, count, str(LANG_STOP_ABORT));
#else
    splash(0, 0, true, fmt, count, str(LANG_OFF_ABORT));
#endif
}

/*
 * Display buffer full message
 */
static void display_buffer_full(void)
{
    lcd_clear_display();
    lcd_puts(0,0,str(LANG_PLAYINDICES_PLAYLIST));
    lcd_puts(0,1,str(LANG_PLAYINDICES_BUFFER));
    lcd_update();
    sleep(HZ*2);
    lcd_clear_display();
}

/*
 * Initialize playlist entries at startup
 */
void playlist_init(void)
{
    playlist.fd = -1;
    playlist.control_fd = -1;
    playlist.max_playlist_size = global_settings.max_files_in_playlist;
    playlist.indices = buffer_alloc(playlist.max_playlist_size * sizeof(int));
    playlist.buffer_size =
        AVERAGE_FILENAME_LENGTH * global_settings.max_files_in_dir;
    playlist.buffer = buffer_alloc(playlist.buffer_size);
    mutex_init(&playlist.control_mutex);
    empty_playlist(true);
}

/*
 * Create new playlist
 */
int playlist_create(char *dir, char *file)
{
    empty_playlist(false);

    playlist.control_fd = open(PLAYLIST_CONTROL_FILE, O_RDWR);
    if (-1 == playlist.control_fd)
    {
        splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    if (!file)
    {
        file = "";

        if (dir)
            playlist.in_ram = true;
        else
            dir = ""; /* empty playlist */
    }
    
    update_playlist_filename(dir, file);

    if (fprintf(playlist.control_fd, "P:%d:%s:%s\n",
            PLAYLIST_CONTROL_FILE_VERSION, dir, file) > 0)
        fsync(playlist.control_fd);
    else
    {
        splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
        return -1;
    }

    /* load the playlist file */
    if (file[0] != '\0')
        add_indices_to_playlist();

    return 0;
}

#define PLAYLIST_COMMAND_SIZE (MAX_PATH+12)

/*
 * Restore the playlist state based on control file commands.  Called to
 * resume playback after shutdown.
 */
int playlist_resume(void)
{
    char *buffer;
    int buflen;
    int nread;
    int total_read = 0;
    bool first = true;

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
    buflen = (mp3end - mp3buf);
    buffer = mp3buf;

    empty_playlist(true);

    playlist.control_fd = open(PLAYLIST_CONTROL_FILE, O_RDWR);
    if (-1 == playlist.control_fd)
    {
        splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
        return -1;
    }

    /* read a small amount first to get the header */
    nread = read(playlist.control_fd, buffer,
        PLAYLIST_COMMAND_SIZE<buflen?PLAYLIST_COMMAND_SIZE:buflen);
    if(nread <= 0)
    {
        splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
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
                        {
                            result = -1;
                            exit_loop = true;
                            break;
                        }
                        
                        update_playlist_filename(str2, str3);
                        
                        if (str3[0] != '\0')
                        {
                            /* NOTE: add_indices_to_playlist() overwrites the
                               mp3buf so we need to reload control file
                               data */
                            add_indices_to_playlist();
                        }
                        else if (str2[0] != '\0')
                        {
                            playlist.in_ram = true;
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
                        if (add_track_to_playlist(str3, position, queue,
                            total_read+(str3-buffer)) < 0)
                            return -1;
                        
                        playlist.last_insert_pos = last_position;

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
                        
                        if (remove_track_from_playlist(position,
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
                        
                        seed = atoi(str1);
                        playlist.first_index = atoi(str2);
                        
                        if (randomise_playlist(seed, false, false) < 0)
                            return -1;

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
                        
                        playlist.first_index = atoi(str1);
                        
                        if (sort_playlist(false, false) < 0)
                            return -1;

                        break;
                    }
                    case resume_reset:
                    {
                        playlist.last_insert_pos = -1;
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
            splash(HZ*2, 0, true, str(LANG_PLAYLIST_CONTROL_INVALID));
            return result;
        }

        if (!newline || (exit_loop && count<nread))
        {
            /* We didn't end on a newline or we exited loop prematurely.
               Either way, re-read the remainder.
               NOTE: because of this, control file must always end with a
                     newline */
            count = last_newline;
            lseek(playlist.control_fd, total_read+count, SEEK_SET);
        }

        total_read += count;

        if (first)
            /* still looking for header */
            nread = read(playlist.control_fd, buffer,
                PLAYLIST_COMMAND_SIZE<buflen?PLAYLIST_COMMAND_SIZE:buflen);
        else
            nread = read(playlist.control_fd, buffer, buflen);

        /* Terminate on EOF */
        if(nread <= 0)
            break;
    }

    return 0;
}

/* 
 * Add track to in_ram playlist.  Used when playing directories.
 */
int playlist_add(char *filename)
{
    int len = strlen(filename);
    
    if((len+1 > playlist.buffer_size - playlist.buffer_end_pos) || 
       (playlist.amount >= playlist.max_playlist_size))
    {
        display_buffer_full();
        return -1;
    }

    playlist.indices[playlist.amount++] = playlist.buffer_end_pos;

    strcpy(&playlist.buffer[playlist.buffer_end_pos], filename);
    playlist.buffer_end_pos += len;
    playlist.buffer[playlist.buffer_end_pos++] = '\0';

    return 0;
}

/*
 * Insert track into playlist at specified position (or one of the special
 * positions).  Returns position where track was inserted or -1 if error.
 */
int playlist_insert_track(char *filename, int position, bool queue)
{
    int result = add_track_to_playlist(filename, position, queue, -1);

    if (result != -1)
    {
        fsync(playlist.control_fd);
        mpeg_flush_and_reload_tracks();
    }

    return result;
}

/*
 * Insert all tracks from specified directory into playlist.
 */
int playlist_insert_directory(char *dirname, int position, bool queue,
                              bool recurse)
{
    int count = 0;
    int result;
    char *count_str;

    if (queue)
        count_str = str(LANG_PLAYLIST_QUEUE_COUNT);
    else
        count_str = str(LANG_PLAYLIST_INSERT_COUNT);

    display_playlist_count(count, count_str);

    result = add_directory_to_playlist(dirname, &position, queue, &count,
        recurse);
    fsync(playlist.control_fd);

    display_playlist_count(count, count_str);
    mpeg_flush_and_reload_tracks();

    return result;
}

/*
 * Insert all tracks from specified playlist into dynamic playlist
 */
int playlist_insert_playlist(char *filename, int position, bool queue)
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

    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        splash(HZ*2, 0, true, str(LANG_PLAYLIST_ACCESS_ERROR));
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
#ifdef HAVE_PLAYER_KEYPAD
        if (button_get(false) == BUTTON_STOP)
#else
        if (button_get(false) == BUTTON_OFF)
#endif
            break;

        if (temp_buf[0] != '#' || temp_buf[0] != '\0')
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
            
            insert_pos = add_track_to_playlist(trackname, position, queue,
                                               -1);

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

                if (count == PLAYLIST_DISPLAY_COUNT)
                    mpeg_flush_and_reload_tracks();
            }            
        }

        /* let the other threads work */
        yield();
    }

    close(fd);
    fsync(playlist.control_fd);

    *temp_ptr = '/';

    display_playlist_count(count, count_str);
    mpeg_flush_and_reload_tracks();

    return result;
}

/* delete track at specified index */
int playlist_delete(int index)
{
    int result = remove_track_from_playlist(index, true);

    if (result != -1)
        mpeg_flush_and_reload_tracks();

    return result;
}

/* shuffle newly created playlist using random seed. */
int playlist_shuffle(int random_seed, int start_index)
{
    unsigned int seek_pos = 0;
    bool start_current = false;

    if (start_index >= 0 && global_settings.play_selected)
    {
        /* store the seek position before the shuffle */
        seek_pos = playlist.indices[start_index];        
        playlist.index = playlist.first_index = start_index;
        start_current = true;
    }

    splash(0, 0, true, str(LANG_PLAYLIST_SHUFFLE));
    
    randomise_playlist(random_seed, start_current, true);

    return playlist.index;
}

/* shuffle currently playing playlist */
int playlist_randomise(unsigned int seed, bool start_current)
{
    int result = randomise_playlist(seed, start_current, true);

    if (result != -1)
        mpeg_flush_and_reload_tracks();

    return result;
}

/* sort currently playing playlist */
int playlist_sort(bool start_current)
{
    int result = sort_playlist(start_current, true);

    if (result != -1)
        mpeg_flush_and_reload_tracks();

    return result;
}

/* start playing current playlist at specified index/offset */
int playlist_start(int start_index, int offset)
{
    playlist.index = start_index;
    mpeg_play(offset);

    return 0;
}

/* Returns false if 'steps' is out of bounds, else true */
bool playlist_check(int steps)
{
    int index = get_next_index(steps);
    return (index >= 0);
}

/* get trackname of track that is "steps" away from current playing track.
   NULL is used to identify end of playlist */
char* playlist_peek(int steps)
{
    int seek;
    int fd;
    char *temp_ptr;
    int index;
    bool control_file;

    index = get_next_index(steps);
    if (index < 0)
        return NULL;

    control_file = playlist.indices[index] & PLAYLIST_INSERT_TYPE_MASK;
    seek = playlist.indices[index] & PLAYLIST_SEEK_MASK;

    if (get_filename(seek, control_file, now_playing, MAX_PATH+1) < 0)
        return NULL;

    temp_ptr = now_playing;

    if (!playlist.in_ram || control_file)
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
    int index;

    if (steps > 0)
    {
        int i, j;

        /* We need to delete all the queued songs */
        for (i=0, j=steps; i<j; i++)
        {
            index = get_next_index(i);
            
            if (playlist.indices[index] & PLAYLIST_QUEUE_MASK)
            {
                remove_track_from_playlist(index, true);
                steps--; /* one less track */
            }
        }
    }

    index = get_next_index(steps);
    playlist.index = index;

    if (playlist.last_insert_pos >= 0)
    {
        /* check to see if we've gone beyond the last inserted track */
        int rot_index = index;
        int rot_last_pos = playlist.last_insert_pos;

        rot_index -= playlist.first_index;
        if (rot_index < 0)
            rot_index += playlist.amount;

        rot_last_pos -= playlist.first_index;
        if (rot_last_pos < 0)
            rot_last_pos += playlist.amount;

        if (rot_index > rot_last_pos)
        {
            /* reset last inserted track */
            playlist.last_insert_pos = -1;

            if (playlist.control_fd >= 0)
            {
                int result = -1;

                mutex_lock(&playlist.control_mutex);
            
                if (lseek(playlist.control_fd, 0, SEEK_END) >= 0)
                {
                    if (fprintf(playlist.control_fd, "R\n") > 0)
                    {
                        fsync(playlist.control_fd);
                        result = 0;
                    }
                }

                mutex_unlock(&playlist.control_mutex);

                if (result < 0)
                {
                    splash(HZ*2, 0, true,
                        str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
                    return result;
                }
            }
        }
    }

    return index;
}

/* Get resume info for current playing song.  If return value is -1 then
   settings shouldn't be saved. */
int playlist_get_resume_info(int *resume_index)
{
    *resume_index = playlist.index;

    return 0;
}

/* Returns index of current playing track for display purposes.  This value
   should not be used for resume purposes as it doesn't represent the actual
   index into the playlist */
int playlist_get_display_index(void)
{
    int index = playlist.index;

    /* first_index should always be index 0 for display purposes */
    index -= playlist.first_index;
    if (index < 0)
        index += playlist.amount;

    return (index+1);
}

/* returns number of tracks in playlist (includes queued/inserted tracks) */
int playlist_amount(void)
{
    return playlist.amount;
}

/* returns playlist name */
char *playlist_name(char *buf, int buf_size)
{
    char *sep;

    snprintf(buf, buf_size, "%s", playlist.filename+playlist.dirlen);

    if (0 == buf[0])
        return NULL;

    /* Remove extension */
    sep = strrchr(buf, '.');
    if (NULL != sep)
        *sep = 0;
    
    return buf;
}

/* save the current dynamic playlist to specified file */
int playlist_save(char *filename)
{
    int fd;
    int i, index;
    int count = 0;
    char tmp_buf[MAX_PATH+1];
    int result = 0;

    if (playlist.amount <= 0)
        return -1;

    /* use current working directory as base for pathname */
    if (format_track_path(tmp_buf, filename, sizeof(tmp_buf),
            strlen(filename)+1, getcwd(NULL, -1)) < 0)
        return -1;

    fd = open(tmp_buf, O_CREAT|O_WRONLY|O_TRUNC);
    if (fd < 0)
    {
        splash(HZ*2, 0, true, str(LANG_PLAYLIST_ACCESS_ERROR));
        return -1;
    }

    display_playlist_count(count, str(LANG_PLAYLIST_SAVE_COUNT));

    index = playlist.first_index;
    for (i=0; i<playlist.amount; i++)
    {
        bool control_file;
        bool queue;
        int seek;

        /* user abort */
#ifdef HAVE_PLAYER_KEYPAD
        if (button_get(false) == BUTTON_STOP)
#else
        if (button_get(false) == BUTTON_OFF)
#endif
            break;

        control_file = playlist.indices[index] & PLAYLIST_INSERT_TYPE_MASK;
        queue = playlist.indices[index] & PLAYLIST_QUEUE_MASK;
        seek = playlist.indices[index] & PLAYLIST_SEEK_MASK;

        /* Don't save queued files */
        if (!queue)
        {
            if (get_filename(seek, control_file, tmp_buf, MAX_PATH+1) < 0)
            {
                result = -1;
                break;
            }

            if (fprintf(fd, "%s\n", tmp_buf) < 0)
            {
                splash(HZ*2, 0, true,
                    str(LANG_PLAYLIST_CONTROL_UPDATE_ERROR));
                result = -1;
                break;
            }

            count++;

            if ((count%PLAYLIST_DISPLAY_COUNT) == 0)
                display_playlist_count(count, str(LANG_PLAYLIST_SAVE_COUNT));

            yield();
        }

        index = (index+1)%playlist.amount;
    }

    display_playlist_count(count, str(LANG_PLAYLIST_SAVE_COUNT));

    close(fd);

    return result;
}
