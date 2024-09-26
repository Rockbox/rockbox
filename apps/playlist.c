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
    Dynamic playlist design (based on design originally proposed by ricII)

    There are two files associated with a dynamic playlist:
    1. Playlist file : This file contains the initial songs in the playlist.
                       The file is created by the user and stored on the hard
                       drive.  NOTE: If we are playing the contents of a
                       directory, there will be no playlist file.
    2. Control file :  This file is automatically created when a playlist is
                       started and contains all the commands done to it.

    The first non-comment line in a control file must begin with
    "P:VERSION:DIR:FILE" where VERSION is the playlist control file version
    DIR is the directory where the playlist is located and FILE is the
    playlist filename (without the directory part).

    When there is an on-disk playlist file, both DIR and FILE are nonempty.
    Dynamically generated playlists (whether by the file browser, database,
    or another means) have an empty FILE. For dirplay, DIR will be nonempty.

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

//#define LOGF_ENABLE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "string-extra.h"
#include "playlist.h"
#include "ata_idle_notify.h"
#include "file.h"
#include "action.h"
#include "mv.h"
#include "debug.h"
#include "audio.h"
#include "lcd.h"
#include "kernel.h"
#include "settings.h"
#include "status.h"
#include "applimits.h"
#include "screens.h"
#include "core_alloc.h"
#include "misc.h"
#include "pathfuncs.h"
#include "button.h"
#include "filetree.h"
#include "abrepeat.h"
#include "thread.h"
#include "usb.h"
#include "filetypes.h"
#include "icons.h"
#include "system.h"
#include "misc.h"

#include "lang.h"
#include "talk.h"
#include "splash.h"
#include "rbunicode.h"
#include "root_menu.h"
#include "plugin.h" /* To borrow a temp buffer to rewrite a .m3u8 file */
#include "logdiskf.h"
#ifdef HAVE_DIRCACHE
#include "dircache.h"
#endif
#include "logf.h"
#include "panic.h"

#if 0//def ROCKBOX_HAS_LOGDISKF
#undef DEBUGF
#undef ERRORF
#undef WARNF
#undef NOTEF
#define DEBUGF logf
#define ERRORF DEBUGF
#define WARNF DEBUGF
#define NOTEF DEBUGF
#endif

/* default load buffer size (should be at least 1 KiB) */
#define PLAYLIST_LOAD_BUFLEN    (32*1024)

/*
 * Minimum supported version and current version of the control file.
 * Any versions outside of this range will be rejected by the loader.
 *
 * v1 was the initial version when dynamic playlists were first implemented.
 * v2 was added shortly thereafter and has been used since 2003.
 * v3 added the (C)lear command and is otherwise identical to v2.
 * v4 added the (F)lags command.
 * v5 added index to the (C)lear command. Added PLAYLIST_INSERT_LAST_ROTATED (-8) as
 *    a supported position for (A)dd or (Q)eue commands.
 */
#define PLAYLIST_CONTROL_FILE_MIN_VERSION   2
#define PLAYLIST_CONTROL_FILE_VERSION       5

#define PLAYLIST_COMMAND_SIZE (MAX_PATH+12)

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

#define PLAYLIST_INSERT_TYPE_PREPEND    0x40000000
#define PLAYLIST_INSERT_TYPE_INSERT     0x80000000
#define PLAYLIST_INSERT_TYPE_APPEND     0xC0000000

#define PLAYLIST_QUEUED                 0x20000000
#define PLAYLIST_SKIPPED                0x10000000

static struct playlist_info current_playlist;
/* REPEAT_ONE support function from playback.c */
extern bool audio_pending_track_skip_is_manual(void);
static inline bool is_manual_skip(void)
{
    return audio_pending_track_skip_is_manual();
}

/* Directory Cache*/
static void dc_init_filerefs(struct playlist_info *playlist,
                             int start, int count)
{
#ifdef HAVE_DIRCACHE
    if (!playlist->dcfrefs_handle)
        return;

    struct dircache_fileref *dcfrefs = core_get_data(playlist->dcfrefs_handle);
    int end = start + count;

    for (int i = start; i < end; i++)
        dircache_fileref_init(&dcfrefs[i]);
#else
    (void)playlist;
    (void)start;
    (void)count;
#endif
}

#ifdef HAVE_DIRCACHE
#define PLAYLIST_DC_SCAN_START  1
#define PLAYLIST_DC_SCAN_STOP   2

static struct event_queue playlist_queue;
static struct queue_sender_list playlist_queue_sender_list;
static long playlist_stack[(DEFAULT_STACK_SIZE + 0x800)/sizeof(long)];
static const char dc_thread_playlist_name[] = "playlist cachectrl";
#endif

#define playlist_read_lock(p)       mutex_lock(&(p)->mutex)
#define playlist_read_unlock(p)     mutex_unlock(&(p)->mutex)
#define playlist_write_lock(p)      mutex_lock(&(p)->mutex)
#define playlist_write_unlock(p)    mutex_unlock(&(p)->mutex)

#if defined(PLAYLIST_DEBUG_ACCESS_ERRORS)
#define notify_access_error() (splashf(HZ*2, "%s %s", \
                                    __func__, ID2P(LANG_PLAYLIST_ACCESS_ERROR)))
#define notify_control_access_error() (splashf(HZ*2, "%s %s", \
                            __func__, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR)))
#else
static void notify_access_error(void) {
    splash(HZ*2, ID2P(LANG_PLAYLIST_ACCESS_ERROR));
}
static void notify_control_access_error(void) {
    splash(HZ*2, ID2P(LANG_PLAYLIST_CONTROL_ACCESS_ERROR));
}
#endif

/*
 * Display buffer full message
 */
static void notify_buffer_full(void)
{
    splash(HZ*2, ID2P(LANG_PLAYLIST_BUFFER_FULL));
}

static void dc_thread_start(struct playlist_info *playlist, bool is_dirty)
{
#ifdef HAVE_DIRCACHE
    if (playlist == &current_playlist)
        queue_post(&playlist_queue, PLAYLIST_DC_SCAN_START, is_dirty);
#else
    (void)playlist;
    (void)is_dirty;
#endif
}

static void dc_thread_stop(struct playlist_info *playlist)
{
#ifdef HAVE_DIRCACHE
    if (playlist == &current_playlist)
        queue_send(&playlist_queue, PLAYLIST_DC_SCAN_STOP, 0);
#else
    (void)playlist;
#endif
}

/*
 * Open playlist file and return file descriptor or -1 on error.
 * The fd should not be closed manually. Not thread-safe.
 */
static int pl_open_playlist(struct playlist_info *playlist)
{
    if (playlist->fd >= 0)
        return playlist->fd;

    int fd = open_utf8(playlist->filename, O_RDONLY);
    if (fd < 0)
        return fd;

    /* Presence of UTF-8 BOM forces UTF-8 encoding. */
    if (lseek(fd, 0, SEEK_CUR) > 0)
        playlist->utf8 = true;

    playlist->fd = fd;
    return fd;
}

static void pl_close_fd(int *fdptr)
{
    int fd = *fdptr;
    if (fd >= 0)
    {
        close(fd);
        *fdptr = -1;
    }
}

/*
 * Close any open file descriptor for the playlist file.
 * Not thread-safe.
 */
static void pl_close_playlist(struct playlist_info *playlist)
{
    pl_close_fd(&playlist->fd);
}

/*
 * Close any open playlist control file descriptor.
 * Not thread-safe.
 */
static void pl_close_control(struct playlist_info *playlist)
{
    pl_close_fd(&playlist->control_fd);
}

/* Check if the filename suggests M3U or M3U8 format. */
static bool is_m3u8_name(const char* filename)
{
    char *dot = strrchr(filename, '.');

    /* Default to M3U8 unless explicitly told otherwise. */
    return (!dot || strcasecmp(dot, ".m3u") != 0);
}

/* Convert a filename in an M3U playlist to UTF-8.
 *
 * buf     - the filename to convert; can contain more than one line from the
 *           playlist.
 * buf_len - amount of buf that is used.
 * buf_max - total size of buf.
 * temp    - temporary conversion buffer, at least buf_max bytes.
 *
 * Returns the length of the converted filename.
 */
static int convert_m3u_name(char* buf, int buf_len, int buf_max, char* temp)
{
    int i = 0;
    char* dest;

    /* Locate EOL. */
    while ((i < buf_len) && (buf[i] != '\n') && (buf[i] != '\r'))
    {
        i++;
    }

    /* Work back killing white space. */
    while ((i > 0) && isspace(buf[i - 1]))
    {
        i--;
    }

    buf_len = i;
    dest = temp;

    /* Convert char by char, so as to not overflow temp (iso_decode should
     * preferably handle this). No more than 4 bytes should be generated for
     * each input char.
     */
    for (i = 0; i < buf_len && dest < (temp + buf_max - 4); i++)
    {
        dest = iso_decode(&buf[i], dest, -1, 1);
    }

    *dest = 0;
    strcpy(buf, temp);
    return dest - temp;
}

/*
 * create control file for playlist
 */
static void create_control_unlocked(struct playlist_info* playlist)
{
    playlist->control_fd = open(playlist->control_filename,
                                O_CREAT|O_RDWR|O_TRUNC, 0666);

    playlist->control_created = (playlist->control_fd >= 0);

    if (!playlist->control_created)
    {
        if (!check_rockboxdir())
            return; /* No RB directory exists! */

        cond_talk_ids_fq(LANG_PLAYLIST_CONTROL_ACCESS_ERROR);
        int fd = playlist->control_fd;
        splashf(HZ*2, "%s (%d)", str(LANG_PLAYLIST_CONTROL_ACCESS_ERROR), fd);
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

static void sync_control_unlocked(struct playlist_info* playlist)
{
    if (playlist->control_fd >= 0)
        fsync(playlist->control_fd);
}

static int update_control_unlocked(struct playlist_info* playlist,
                                   enum playlist_command command, int i1, int i2,
                                   const char* s1, const char* s2, int *seekpos)
{
    int fd = playlist->control_fd;
    int result;

    lseek(fd, 0, SEEK_END);

    switch (command)
    {
    case PLAYLIST_COMMAND_PLAYLIST:
        result = fdprintf(fd, "P:%d:%s:%s\n", i1, s1, s2);
        break;
    case PLAYLIST_COMMAND_ADD:
    case PLAYLIST_COMMAND_QUEUE:
        result = fdprintf(fd, "%c:%d:%d:",
                          command == PLAYLIST_COMMAND_ADD ? 'A' : 'Q', i1, i2);
        if (result > 0)
        {
            *seekpos = lseek(fd, 0, SEEK_CUR);
            result = fdprintf(fd, "%s\n", s1);
        }
        break;
    case PLAYLIST_COMMAND_DELETE:
        result = fdprintf(fd, "D:%d\n", i1);
        break;
    case PLAYLIST_COMMAND_SHUFFLE:
        result = fdprintf(fd, "S:%d:%d\n", i1, i2);
        break;
    case PLAYLIST_COMMAND_UNSHUFFLE:
        result = fdprintf(fd, "U:%d\n", i1);
        break;
    case PLAYLIST_COMMAND_RESET:
        result = write(fd, "R\n", 2);
        break;
    case PLAYLIST_COMMAND_CLEAR:
        result = fdprintf(fd, "C:%d\n", i1);
        break;
    case PLAYLIST_COMMAND_FLAGS:
        result = fdprintf(fd, "F:%u:%u\n", i1, i2);
        break;
    default:
        return -1;
    }

    return result;
}

/*
 * store directory and name of playlist file
 */
static void update_playlist_filename_unlocked(struct playlist_info* playlist,
                                     const char *dir, const char *file)
{
    char *sep="";
    int dirlen = strlen(dir);

    playlist->utf8 = is_m3u8_name(file);

    /* If the dir does not end in trailing slash, we use a separator.
       Otherwise we don't. */
    if(!dirlen || '/' != dir[dirlen-1])
    {
        sep="/";
        dirlen++;
    }

    playlist->dirlen = dirlen;

    snprintf(playlist->filename, sizeof(playlist->filename),
        "%s%s%s", dir, sep, file);
}

/*
 * remove any files and indices associated with the playlist
 */
static void empty_playlist_unlocked(struct playlist_info* playlist, bool resume)
{
    pl_close_playlist(playlist);

    if(playlist->control_fd >= 0)
    {
        close(playlist->control_fd);
    }

    playlist->filename[0] = '\0';

    playlist->seed = 0;

    playlist->utf8 = true;
    playlist->control_created = false;
    playlist->flags = 0;

    playlist->control_fd = -1;

    playlist->index = 0;
    playlist->first_index = 0;
    playlist->amount = 0;
    playlist->last_insert_pos = -1;

    playlist->started = false;

    if (!resume && playlist == &current_playlist)
    {
        /* start with fresh playlist control file when starting new
           playlist */
        create_control_unlocked(playlist);
    }
}

int update_playlist_flags_unlocked(struct playlist_info *playlist,
                                   unsigned int setf, unsigned int clearf)
{
    unsigned int newflags = (playlist->flags & ~clearf) | setf;
    if (newflags == playlist->flags)
        return 0;

    playlist->flags = newflags;

    if (playlist->control_fd >= 0)
    {
        int res = update_control_unlocked(playlist, PLAYLIST_COMMAND_FLAGS,
                                          setf, clearf, NULL, NULL, NULL);
        if (res < 0)
            return res;

        sync_control_unlocked(playlist);
    }

    return 0;
}

/*
 * Returns absolute path of track
 *
 * dest: output buffer
 * src: the file name from the playlist
 * dir: the absolute path to the directory where the playlist resides
 * dlen used to truncate dir -- supply -1u to ignore
 *
 * The type of path in "src" determines what will be written to "dest":
 *
 * 1. UNIX-style absolute paths (/foo/bar) remain unaltered
 * 2. Windows-style absolute paths (C:/foo/bar) will be converted into an
 *    absolute path by replacing the drive letter with the volume that the
 *    *playlist* resides on, ie. the volume in "dir"
 * 3. Relative paths are converted to absolute paths by prepending "dir".
 *    This also applies to Windows-style relative paths "C:foo/bar" where
 *    the drive letter is accepted but ignored.
 */
static ssize_t format_track_path(char *dest, char *src, int buf_length,
                                 const char *dir, size_t dlen)
{
    /* Look for the end of the string (includes NULL) */

    if (!src || !dest || !dir)
    {
        DEBUGF("%s() bad pointer", __func__);
        return -2; /* bad pointers */
    }
    size_t len = strcspn(src, "\r\n");;
    /* Now work back killing white space */
    while (len > 0)
    {
        int c = src[len - 1];
        if (c != '\t' && c != ' ')
            break;
        len--;
    }

    src[len] = '\0';

    /* Replace backslashes with forward slashes */
    path_correct_separators(src, src);

    /* Handle Windows-style absolute paths */
    if (path_strip_drive(src, (const char **)&src, true) >= 0 &&
        src[-1] == PATH_SEPCH)
    {
    #ifdef HAVE_MULTIVOLUME
        const char *p;
        path_strip_last_volume(dir, &p, false);
        //dir = strmemdupa(dir, p - dir);
        dlen = (p-dir);  /* empty if no volspec on dir */
    #else
        dir = "";                       /* only volume is root */
    #endif
    }

    if (*dir == '\0')
    {
        dir = PATH_ROOTSTR;
        dlen = -1u;
    }

    len = path_append_ex(dest, dir, dlen, src, buf_length);
    if (len >= (size_t)buf_length)
        return -1; /* buffer too small */

    path_remove_dot_segments (dest, dest);
    logf("%s %s", __func__, dest);
    return strlen (dest);
}

/*
 * Initialize a new playlist for viewing/editing/playing.  dir is the
 * directory where the playlist is located and file is the filename.
 */
static void new_playlist_unlocked(struct playlist_info* playlist,
                                  const char *dir, const char *file)
{
    empty_playlist_unlocked(playlist, false);

    /* enable dirplay for the current playlist if there's a DIR but no FILE */
    if (!file && dir && playlist == &current_playlist)
        playlist->flags |= PLAYLIST_FLAG_DIRPLAY;

    dir = dir ?: "";
    file = file ?: "";

    update_playlist_filename_unlocked(playlist, dir, file);

    if (playlist->control_fd >= 0)
    {
        update_control_unlocked(playlist, PLAYLIST_COMMAND_PLAYLIST,
                        PLAYLIST_CONTROL_FILE_VERSION, -1, dir, file, NULL);
        sync_control_unlocked(playlist);
    }
}

/*
 * validate the control file.  This may include creating/initializing it if
 * necessary;
 */
static int check_control(struct playlist_info* playlist)
{
    int ret = 0;
    playlist_write_lock(playlist);

    if (!playlist->control_created)
    {
        create_control_unlocked(playlist);

        if (playlist->control_fd >= 0)
        {
            char* dir = playlist->filename;
            char* file = playlist->filename+playlist->dirlen;
            char c = playlist->filename[playlist->dirlen-1];

            playlist->filename[playlist->dirlen-1] = '\0';

            update_control_unlocked(playlist, PLAYLIST_COMMAND_PLAYLIST,
                PLAYLIST_CONTROL_FILE_VERSION, -1, dir, file, NULL);
            sync_control_unlocked(playlist);
            playlist->filename[playlist->dirlen-1] = c;
        }
    }

    if (playlist->control_fd < 0)
        ret = -1;

    playlist_write_unlock(playlist);
    return ret;
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

    if(id >= 0 && global_settings.talk_menu)
    {
        long next_tick = talked_tick + (HZ * 5);

        if (final || talked_tick == 0)
            next_tick = current_tick - 1;

        if(count && TIME_AFTER(current_tick, next_tick))
        {
            talked_tick = current_tick;
            talk_number(count, false);
            talk_id(id, true);
        }
    }

    splashf(0, P2STR(fmt), count, str(LANG_OFF_ABORT));
}

/*
 * calculate track offsets within a playlist file
 */
static int add_indices_to_playlist(struct playlist_info* playlist,
                                   char* buffer, size_t buflen)
{
    ssize_t nread;
    unsigned int i, count = 0;
    bool store_index;
    unsigned char *p;
    int result = 0;
    /* get emergency buffer so we don't fail horribly */
    if (!buflen)
        buffer = alloca((buflen = 64));

    playlist_write_lock(playlist);

    /* Close and re-open the playlist to ensure we are properly
     * positioned at the start of the file after any UTF-8 BOM. */
    pl_close_playlist(playlist);
    if (pl_open_playlist(playlist) < 0)
    {
        result = -1;
        goto exit;
    }

    i = lseek(playlist->fd, 0, SEEK_CUR);

    splash(0, ID2P(LANG_WAIT));
    store_index = true;

    while(1)
    {
        nread = read(playlist->fd, buffer, buflen);
        /* Terminate on EOF */
        if(nread <= 0)
            break;

        p = (unsigned char *)buffer;

        for(count=0; count < (unsigned int)nread; count++,p++) {

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
                        notify_buffer_full();
                        result = -1;
                        goto exit;
                    }

                    /* Store a new entry */
                    playlist->indices[ playlist->amount ] = i+count;
                    dc_init_filerefs(playlist, playlist->amount, 1);
                    playlist->amount++;
                }
            }
        }

        i+= count;
    }

exit:
    playlist_write_unlock(playlist);
    return result;
}

/*
 * Checks if there are any music files in the dir or any of its
 * subdirectories.  May be called recursively.
 */
static int check_subdir_for_music(char *dir, const char *subdir, bool recurse)
{
    int result = -1;
    size_t dirlen = strlen(dir);
    int num_files = 0;
    int i;
    struct entry *files;
    bool has_music = false;
    bool has_subdir = false;
    struct tree_context* tc = tree_get_context();

    if (path_append(dir + dirlen, PA_SEP_HARD, subdir, MAX_PATH - dirlen) >=
            MAX_PATH - dirlen)
    {
        return 0;
    }

    if (ft_load(tc, dir) < 0)
    {
        return -2;
    }

    tree_lock_cache(tc);
    files = tree_get_entries(tc);
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
    {
        tree_unlock_cache(tc);
        return 0;
    }

    if (has_subdir && recurse)
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
                result = check_subdir_for_music(dir, files[i].name, true);
                if (!result)
                    break;
            }
        }
    }
    tree_unlock_cache(tc);

    if (result < 0)
    {
        if (dirlen)
        {
            dir[dirlen] = '\0';
        }
        else
        {
            strcpy(dir, PATH_ROOTSTR);
        }

        /* we now need to reload our current directory */
        if(ft_load(tc, dir) < 0)
            splash(HZ*2, ID2P(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
    }
    return result;
}

/*
 * search through all the directories (starting with the current) to find
 * one that has tracks to play
 */
static int get_next_dir(char *dir, int direction)
{
    struct playlist_info* playlist = &current_playlist;
    int result = -1;
    char *start_dir = NULL;
    bool exit = false;
    struct tree_context* tc = tree_get_context();
    int saved_dirfilter = *(tc->dirfilter);
    unsigned int base_len;

    if (global_settings.constrain_next_folder)
    {
        /* constrain results to directories below user's start directory */
        strcpy(dir, global_settings.start_directory);
        base_len = strlen(dir);

        /* strip any trailing slash from base directory */
        if (base_len > 0 && dir[base_len - 1] == '/')
        {
            base_len--;
            dir[base_len] = '\0';
        }
    }
    else
    {
        /* start from root directory */
        dir[0] = '\0';
        base_len = 0;
    }

    /* process random folder advance */
    if (global_settings.next_folder == FOLDER_ADVANCE_RANDOM)
    {
        int fd = open(ROCKBOX_DIR "/folder_advance_list.dat", O_RDONLY);
        if (fd >= 0)
        {
            int folder_count = 0;
            ssize_t nread = read(fd,&folder_count,sizeof(int));
            if ((nread == sizeof(int)) && folder_count)
            {
                char buffer[MAX_PATH];
                /* give up looking for a directory after we've had four
                   times as many tries as there are directories. */
                unsigned long allowed_tries = folder_count * 4;
                int i;
                srand(current_tick);
                *(tc->dirfilter) = SHOW_MUSIC;
                tc->sort_dir = global_settings.sort_dir;
                while (!exit && allowed_tries--)
                {
                    i = rand() % folder_count;
                    lseek(fd, sizeof(int) + (MAX_PATH * i), SEEK_SET);
                    read(fd, buffer, MAX_PATH);
                    /* is the current dir within our base dir and has music? */
                    if ((base_len == 0 || !strncmp(buffer, dir, base_len))
                        && check_subdir_for_music(buffer, "", false) == 0)
                            exit = true;
                }
                close(fd);
                *(tc->dirfilter) = saved_dirfilter;
                tc->sort_dir = global_settings.sort_dir;
                reload_directory();
                if (exit)
                {
                    strcpy(dir,buffer);
                    return 0;
                }
            }
            else
                close(fd);
        }
    }

    /* if the current file is within our base dir, use its dir instead */
    if (base_len == 0 || !strncmp(playlist->filename, dir, base_len))
        strmemccpy(dir, playlist->filename, playlist->dirlen);

    /* use the tree browser dircache to load files */
    *(tc->dirfilter) = SHOW_ALL;

    /* set up sorting/direction */
    tc->sort_dir = global_settings.sort_dir;
    if (direction < 0)
    {
        static const char sortpairs[] =
        {
            [SORT_ALPHA] = SORT_ALPHA_REVERSED,
            [SORT_DATE] = SORT_DATE_REVERSED,
            [SORT_TYPE] = SORT_TYPE_REVERSED,
            [SORT_ALPHA_REVERSED] = SORT_ALPHA,
            [SORT_DATE_REVERSED] = SORT_DATE,
            [SORT_TYPE_REVERSED] = SORT_TYPE,
        };

        if ((unsigned)tc->sort_dir < sizeof(sortpairs))
            tc->sort_dir = sortpairs[tc->sort_dir];
    }

    while (!exit)
    {
        struct entry *files;
        int num_files = 0;
        int i;

        if (ft_load(tc, (dir[0]=='\0')?"/":dir) < 0)
        {
            exit = true;
            result = -1;
            break;
        }

        tree_lock_cache(tc);
        files = tree_get_entries(tc);
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
                    result = check_subdir_for_music(dir, files[i].name, true);
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
        tree_unlock_cache(tc);

        if (!exit)
        {
            /* we've already descended to the base dir with nothing found,
               check whether that contains music */
            if (strlen(dir) <= base_len)
            {
                result = check_subdir_for_music(dir, "", true);
                if (result == -1)
                    /* there's no music files in the base directory,
                       treat as a fatal error */
                    result = -2;
                break;
            }
            else
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
    }

    /* restore dirfilter */
    *(tc->dirfilter) = saved_dirfilter;
    tc->sort_dir = global_settings.sort_dir;

    return result;
}

/*
 * gets pathname for track at seek index
 */
static int get_track_filename(struct playlist_info* playlist, int index,
                              char *buf, int buf_length)
{
    int fd;
    int max = -1;
    char tmp_buf[MAX_PATH+1];
    char dir_buf[MAX_PATH+1];
    bool utf8 = playlist->utf8;
    if (buf_length > 0)
        buf[0] = '\0';

    if (index < 0 || index >= playlist->amount)
        return -1;

    playlist_write_lock(playlist);

    bool control_file = playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK;
    unsigned long seek = playlist->indices[index] & PLAYLIST_SEEK_MASK;

#ifdef HAVE_DIRCACHE
    if (playlist->dcfrefs_handle)
    {
        struct dircache_fileref *dcfrefs = core_get_data_pinned(playlist->dcfrefs_handle);
        max = dircache_get_fileref_path(&dcfrefs[index],
                                        tmp_buf, sizeof(tmp_buf));

        NOTEF("%s [in DCache]: 0x%x %s", __func__, dcfrefs[index], tmp_buf);
        core_put_data_pinned(dcfrefs);
    }
#endif /* HAVE_DIRCACHE */

    if (max < 0)
    {
        if (control_file)
        {
            fd = playlist->control_fd;
            utf8 = true;
        }
        else
        {
            fd = pl_open_playlist(playlist);
        }

        if(-1 != fd)
        {
            if (lseek(fd, seek, SEEK_SET) != (off_t)seek)
                max = -1;
            else
            {
                max = read(fd, tmp_buf, MIN((size_t) buf_length, sizeof(tmp_buf)));

                if (max > 0)
                {
                    /* playlist file may end without a new line - terminate buffer */
                    tmp_buf[MIN(max, (int)sizeof(tmp_buf) - 1)] = '\0';

                    /* Use dir_buf as a temporary buffer. Note that dir_buf must
                     * be as large as tmp_buf.
                     */
                    if (!utf8)
                        max = convert_m3u_name(tmp_buf, max,
                                               sizeof(tmp_buf), dir_buf);

                    NOTEF("%s [in File]: 0x%x %s", __func__, seek, tmp_buf);
                }
            }
        }

        if (max < 0)
        {
            playlist_write_unlock(playlist);

            if (usb_detect() == USB_INSERTED)
                ; /* ignore error on usb plug */
            else if (control_file)
                notify_control_access_error();
            else
                notify_access_error();

            return max;
        }
    }

    playlist_write_unlock(playlist);

    if (format_track_path(buf, tmp_buf, buf_length,
                          playlist->filename, playlist->dirlen) < 0)
        return -1;

    return 0;
}

/*
 * Utility function to create a new playlist, fill it with the next or
 * previous directory, shuffle it if needed, and start playback.
 * If play_last is true and direction zero or negative, start playing
 * the last file in the directory, otherwise start playing the first.
 */
static int create_and_play_dir(int direction, bool play_last)
{
    char dir[MAX_PATH + 1];
    int res = get_next_dir(dir, direction);
    int index = -1;

    if (res < 0) /* return the error encountered */
        return res;

    if (playlist_create(dir, NULL) != -1)
    {
        ft_build_playlist(tree_get_context(), 0);

        if (global_settings.playlist_shuffle)
             playlist_shuffle(current_tick, -1);

        if (play_last && direction <= 0)
            index = current_playlist.amount - 1;
        else
            index = 0;

        current_playlist.started = true;
    }

    /* we've overwritten the dircache when getting the next/previous dir,
       so the tree browser context will need to be reloaded */
    reload_directory();

    return index;
}

/*
 * remove all tracks, leaving the current track queued
 */
static int remove_all_tracks_unlocked(struct playlist_info *playlist, bool write)
{
    if (playlist->amount <= 0)
        return 0;

    /* Move current track down to position 0 */
    playlist->indices[0] = playlist->indices[playlist->index];
#ifdef HAVE_DIRCACHE
    if (playlist->dcfrefs_handle)
    {
        struct dircache_fileref *dcfrefs =
            core_get_data(playlist->dcfrefs_handle);
        dcfrefs[0] = dcfrefs[playlist->index];
    }
#endif

    /* Update playlist state as if by remove_track_unlocked() */
    playlist->first_index = 0;
    playlist->amount = 1;
    playlist->indices[0] |= PLAYLIST_QUEUED;

    if (playlist->last_insert_pos == 0)
        playlist->last_insert_pos = -1;
    else
        playlist->last_insert_pos = 0;

    if (write && playlist->control_fd >= 0)
    {
        update_control_unlocked(playlist, PLAYLIST_COMMAND_CLEAR,
                                playlist->index, -1, NULL, NULL, NULL);
        sync_control_unlocked(playlist);
    }

    playlist->index = 0;

    return 0;
}

/*
 * Add track to playlist at specified position. There are seven special
 * positions that can be specified:
 *  PLAYLIST_PREPEND              - Add track at beginning of playlist
 *  PLAYLIST_INSERT               - Add track after current song.  NOTE: If
 *                                  there are already inserted tracks then track
 *                                  is added to the end of the insertion list
 *  PLAYLIST_INSERT_FIRST         - Add track immediately after current song, no
 *                                  matter what other tracks have been inserted
 *  PLAYLIST_INSERT_LAST          - Add track to end of playlist
 *  PLAYLIST_INSERT_LAST_ROTATED  - Add track to end of playlist, by inserting at
 *                                  first_index, then increasing first_index by 1
 *  PLAYLIST_INSERT_SHUFFLED      - Add track at some random point between the
 *                                  current playing track and end of playlist
 *  PLAYLIST_INSERT_LAST_SHUFFLED - Add tracks in random order to the end of
 *                                  the playlist.
 *  PLAYLIST_REPLACE              - Erase current playlist, Cue the current track
 *                                  and inster this track at the end.
 */
static int add_track_to_playlist_unlocked(struct playlist_info* playlist,
                                          const char *filename, int position,
                                          bool queue, int seek_pos)
{
    int insert_position, orig_position;
    unsigned long flags = PLAYLIST_INSERT_TYPE_INSERT;
    int i;

    insert_position = orig_position = position;

    if (playlist->amount >= playlist->max_playlist_size)
    {
        notify_buffer_full();
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

            playlist->last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_FIRST:
            if (playlist->amount > 0)
                position = insert_position = playlist->index + 1;
            else
                position = insert_position = 0;

            playlist->last_insert_pos = position;
            break;
        case PLAYLIST_INSERT_LAST:
            if (playlist->first_index <= 0)
            {
                position = insert_position = playlist->amount;
                playlist->last_insert_pos = position;
                break;
            }
            /* fallthrough */
        case PLAYLIST_INSERT_LAST_ROTATED:
            position = insert_position = playlist->first_index;
            playlist->last_insert_pos = position;
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
        case PLAYLIST_INSERT_LAST_SHUFFLED:
        {
            int playlist_end = playlist->first_index > 0 ?
                               playlist->first_index : playlist->amount;

            int newpos = playlist->last_shuffled_start +
                rand() % (playlist_end - playlist->last_shuffled_start + 1);

            position = insert_position = newpos;
            break;
        }
        case PLAYLIST_REPLACE:
            if (remove_all_tracks_unlocked(playlist, true) < 0)
                return -1;
            int newpos = playlist->index + 1;
            playlist->last_insert_pos = position = insert_position = newpos;
            break;
    }

    if (queue)
        flags |= PLAYLIST_QUEUED;

#ifdef HAVE_DIRCACHE
    struct dircache_fileref *dcfrefs = NULL;
    if (playlist->dcfrefs_handle)
        dcfrefs = core_get_data(playlist->dcfrefs_handle);
#else
    int *dcfrefs = NULL;
#endif

    /* shift indices so that track can be added */
    for (i=playlist->amount; i>insert_position; i--)
    {
        playlist->indices[i] = playlist->indices[i-1];
        if (dcfrefs)
            dcfrefs[i] = dcfrefs[i-1];
    }

    /* update stored indices if needed */

    if (orig_position < 0)
    {
        if (playlist->amount > 0 && insert_position <= playlist->index &&
            playlist->started && orig_position != PLAYLIST_INSERT_LAST_ROTATED)
            playlist->index++;

        /*
         * When inserting into a playlist at positions before or equal to first_index
         * (unless PLAYLIST_PREPEND is specified explicitly), adjust first_index, so
         * that track insertion near the end does not affect the start of the playlist
         */
        if (playlist->amount > 0 && insert_position <= playlist->first_index &&
            orig_position != PLAYLIST_PREPEND && playlist->started)
        {
            /*
             * To ensure proper resuming from control file for a track that is supposed
             * to be appended, but is inserted at first_index, store position as special
             * value.
             * If we were to store the position unchanged, i.e. use first_index,
             * track would be prepended, instead, after resuming.
             */
            if (insert_position == playlist->first_index)
                position = PLAYLIST_INSERT_LAST_ROTATED;

            playlist->first_index++;
        }
    }
    else if (playlist->amount > 0 && insert_position < playlist->first_index &&
             playlist->started)
         playlist->first_index++;

    if (insert_position < playlist->last_insert_pos ||
        (insert_position == playlist->last_insert_pos && position < 0 &&
         position != PLAYLIST_INSERT_LAST_ROTATED))
        playlist->last_insert_pos++;

    if (seek_pos < 0 && playlist->control_fd >= 0)
    {
        int result = update_control_unlocked(playlist,
            (queue?PLAYLIST_COMMAND_QUEUE:PLAYLIST_COMMAND_ADD), position,
            playlist->last_insert_pos, filename, NULL, &seek_pos);

        if (result < 0)
            return result;
    }

    playlist->indices[insert_position] = flags | seek_pos;
    dc_init_filerefs(playlist, insert_position, 1);

    playlist->amount++;

    return insert_position;
}

/*
 * Callback for playlist_directory_tracksearch to insert track into
 * playlist.
 */
static int directory_search_callback(char* filename, void* context)
{
    return playlist_insert_context_add(context, filename);
}

/*
 * remove track at specified position
 */
static int remove_track_unlocked(struct playlist_info* playlist,
                                 int position, bool write)
{
    int i;
    int result = 0;

    if (playlist->amount <= 0)
        return -1;

#ifdef HAVE_DIRCACHE
    struct dircache_fileref *dcfrefs = NULL;
    if (playlist->dcfrefs_handle)
        dcfrefs = core_get_data(playlist->dcfrefs_handle);
#else
    int *dcfrefs = NULL;
#endif

    /* shift indices now that track has been removed */
    for (i=position; i<playlist->amount; i++)
    {
        playlist->indices[i] = playlist->indices[i+1];
        if (dcfrefs)
            dcfrefs[i] = dcfrefs[i+1];
    }

    playlist->amount--;

    /* update stored indices if needed */
    if (position < playlist->index)
        playlist->index--;

    if (position < playlist->first_index)
    {
        playlist->first_index--;
    }

    if (position <= playlist->last_insert_pos)
        playlist->last_insert_pos--;

    if (write && playlist->control_fd >= 0)
    {
        result = update_control_unlocked(playlist, PLAYLIST_COMMAND_DELETE,
            position, -1, NULL, NULL, NULL);
        if (result >= 0)
            sync_control_unlocked(playlist);
    }

    return result;
}

/*
 * Search for the seek track and set appropriate indices.  Used after shuffle
 * to make sure the current index is still pointing to correct track.
 */
static void find_and_set_playlist_index_unlocked(struct playlist_info* playlist,
                                                 unsigned long seek)
{
    int i;

    /* Set the index to the current song */
    for (i=0; i<playlist->amount; i++)
    {
        if (playlist->indices[i] == seek)
        {
            playlist->index = playlist->first_index = i;

            break;
        }
    }
}

/*
 * randomly rearrange the array of indices for the playlist.  If start_current
 * is true then update the index to the new index of the current playing track
 */
static int randomise_playlist_unlocked(struct playlist_info* playlist,
                                       unsigned int seed, bool start_current,
                                       bool write)
{
    int count;
    int candidate;
    unsigned long current = playlist->indices[playlist->index];

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
        unsigned long indextmp = playlist->indices[candidate];
        playlist->indices[candidate] = playlist->indices[count];
        playlist->indices[count] = indextmp;
#ifdef HAVE_DIRCACHE
        if (playlist->dcfrefs_handle)
        {
            struct dircache_fileref *dcfrefs = core_get_data(playlist->dcfrefs_handle);
            struct dircache_fileref dcftmp = dcfrefs[candidate];
            dcfrefs[candidate] = dcfrefs[count];
            dcfrefs[count] = dcftmp;
        }
#endif
    }

    if (start_current)
        find_and_set_playlist_index_unlocked(playlist, current);

    /* indices have been moved so last insert position is no longer valid */
    playlist->last_insert_pos = -1;

    playlist->seed = seed;

    if (write)
    {
        update_control_unlocked(playlist, PLAYLIST_COMMAND_SHUFFLE, seed,
            playlist->first_index, NULL, NULL, NULL);
    }

    return 0;
}

/*
 * used to sort track indices.  Sort order is as follows:
 * 1. Prepended tracks (in prepend order)
 * 2. Playlist/directory tracks (in playlist order)
 * 3. Inserted/Appended tracks (in insert order)
 */
static int sort_compare_fn(const void* p1, const void* p2)
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
 * Sort the array of indices for the playlist. If start_current is true then
 * set the index to the new index of the current song.
 * Also while going to unshuffled mode set the first_index to 0.
 */
static int sort_playlist_unlocked(struct playlist_info* playlist,
                                  bool start_current, bool write)
{
    unsigned long current = playlist->indices[playlist->index];

    if (playlist->amount > 0)
        qsort((void*)playlist->indices, playlist->amount,
            sizeof(playlist->indices[0]), sort_compare_fn);

#ifdef HAVE_DIRCACHE
    /** We need to re-check the song names from disk because qsort can't
     * sort two arrays at once :/
     * FIXME: Please implement a better way to do this. */
    dc_init_filerefs(playlist, 0, playlist->max_playlist_size);
#endif

    if (start_current)
        find_and_set_playlist_index_unlocked(playlist, current);

    /* indices have been moved so last insert position is no longer valid */
    playlist->last_insert_pos = -1;

    if (write && playlist->control_fd >= 0)
    {
        playlist->first_index = 0;
        update_control_unlocked(playlist, PLAYLIST_COMMAND_UNSHUFFLE,
            playlist->first_index, -1, NULL, NULL, NULL);
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
    {
        repeat_mode = REPEAT_ALL;
    }

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
                    if (playlist->indices[index] & PLAYLIST_QUEUED)
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
    if (next_index < 0 || next_index >= playlist->amount ||
        playlist->indices[next_index] & PLAYLIST_SKIPPED)
        return -1;

    return next_index;
}

#ifdef HAVE_DIRCACHE
/**
 * Thread to update filename pointers to dircache on background
 * without affecting playlist load up performance.
 */
static void dc_thread_playlist(void)
{
    struct queue_event ev;
    static char tmp[MAX_PATH+1];

    struct playlist_info *playlist = &current_playlist;
    struct dircache_fileref *dcfrefs;
    int index;

    /* Thread starts out stopped */
    long sleep_time = TIMEOUT_BLOCK;
    int stop_count = 1;
    bool is_dirty = false;

    while (1)
    {
        queue_wait_w_tmo(&playlist_queue, &ev, sleep_time);

        switch (ev.id)
        {
            case PLAYLIST_DC_SCAN_START:
                if (ev.data)
                    is_dirty = true;

                stop_count--;
                if (is_dirty && stop_count == 0)
                {
                    /* Start the background scanning after either the disk
                     * spindown timeout or 5s, whichever is less */
                    sleep_time = 5 * HZ;
#ifdef HAVE_DISK_STORAGE
                    if (global_settings.disk_spindown > 1 &&
                        global_settings.disk_spindown <= 5)
                        sleep_time = (global_settings.disk_spindown - 1) * HZ;
#endif
                }
                break;

            case PLAYLIST_DC_SCAN_STOP:
                stop_count++;
                sleep_time = TIMEOUT_BLOCK;
                queue_reply(&playlist_queue, 0);
                break;

            case SYS_TIMEOUT:
            {
                /* Nothing to do if there are no dcfrefs or tracks */
                if (!playlist->dcfrefs_handle || playlist->amount <= 0)
                {
                    is_dirty = false;
                    sleep_time = TIMEOUT_BLOCK;
                    logf("%s: nothing to scan", __func__);
                    break;
                }

                /* Retry at a later time if the dircache is not ready */
                struct dircache_info info;
                dircache_get_info(&info);
                if (info.status != DIRCACHE_READY)
                {
                    logf("%s: dircache not ready", __func__);
                    break;
                }

                logf("%s: scan start", __func__);
#ifdef LOGF_ENABLE
                long scan_start_tick = current_tick;
#endif

                trigger_cpu_boost();
                dcfrefs = core_get_data_pinned(playlist->dcfrefs_handle);

                for (index = 0; index < playlist->amount; index++)
                {
                    /* Process only pointers that are superficially stale. */
                    if (dircache_search(DCS_FILEREF, &dcfrefs[index], NULL) > 0)
                        continue;

                    /* Bail out if a command needs servicing. */
                    if (!queue_empty(&playlist_queue))
                    {
                        logf("%s: scan interrupted", __func__);
                        break;
                    }

                    /* Load the filename from playlist file. */
                    if (get_track_filename(playlist, index, tmp, sizeof(tmp)) != 0)
                        break;

                    /* Obtain the dircache file entry cookie. */
                    dircache_search(DCS_CACHED_PATH | DCS_UPDATE_FILEREF,
                                    &dcfrefs[index], tmp);

                    /* And be on background so user doesn't notice any delays. */
                    yield();
                }

                /* If we indexed the whole playlist without being interrupted
                 * then there are no dirty references; go to sleep. */
                if (index == playlist->amount)
                {
                    is_dirty = false;
                    sleep_time = TIMEOUT_BLOCK;
                    logf("%s: scan complete", __func__);
                }

                core_put_data_pinned(dcfrefs);
                cancel_cpu_boost();

                logf("%s: %ld ticks", __func__, current_tick - scan_start_tick);
                break;
            }

            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&playlist_queue);
                break;
        }
    }
}
#endif

/*
 * Allocate a temporary buffer for loading playlists
 */
static int alloc_tempbuf(size_t* buflen)
{
    /* request a reasonable size first */
    int handle = core_alloc_ex(PLAYLIST_LOAD_BUFLEN, &buflib_ops_locked);
    if (handle > 0)
    {
        *buflen = PLAYLIST_LOAD_BUFLEN;
        return handle;
    }

    /* otherwise, try being unreasonable */
    return core_alloc_maximum(buflen, &buflib_ops_locked);
}

/*
 * Need no movement protection since all 3 allocations are not passed to
 * other functions which can yield().
 */
static int move_callback(int handle, void* current, void* new)
{
    (void)handle;
    struct playlist_info* playlist = &current_playlist;
    if (current == playlist->indices)
        playlist->indices = new;
    return BUFLIB_CB_OK;
}


static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

/******************************************************************************/
/******************************************************************************/
/* ************************************************************************** */
/* * PUBLIC INTERFACE FUNCTIONS * *********************************************/
/* ************************************************************************** */
/******************************************************************************/
/******************************************************************************/
/*
 * Initialize playlist entries at startup
 */
void playlist_init(void)
{
    int handle;
    struct playlist_info* playlist = &current_playlist;
    mutex_init(&playlist->mutex);

    strmemccpy(playlist->control_filename, PLAYLIST_CONTROL_FILE,
            sizeof(playlist->control_filename));
    playlist->fd = -1;
    playlist->control_fd = -1;
    playlist->max_playlist_size = global_settings.max_files_in_playlist;

    handle = core_alloc_ex(playlist->max_playlist_size * sizeof(*playlist->indices), &ops);
    playlist->indices = core_get_data(handle);

    empty_playlist_unlocked(playlist, true);

#ifdef HAVE_DIRCACHE
    playlist->dcfrefs_handle = core_alloc(
        playlist->max_playlist_size * sizeof(struct dircache_fileref));
    dc_init_filerefs(playlist, 0, playlist->max_playlist_size);

    unsigned int playlist_thread_id =
        create_thread(dc_thread_playlist, playlist_stack, sizeof(playlist_stack),
                      0, dc_thread_playlist_name IF_PRIO(, PRIORITY_BACKGROUND)
                      IF_COP(, CPU));

    queue_init(&playlist_queue, true);
    queue_enable_queue_send(&playlist_queue,
                            &playlist_queue_sender_list, playlist_thread_id);

    dc_thread_start(&current_playlist, false);
#endif /* HAVE_DIRCACHE */
}

/*
 * Clean playlist at shutdown
 */
void playlist_shutdown(void)

{
    /*BugFix we need to save resume info first */
    /*if (usb_detect() == USB_INSERTED)*/
    audio_stop();
    struct playlist_info* playlist = &current_playlist;
    playlist_write_lock(playlist);
    logf("Closing Control %s", __func__);
    if (playlist->control_fd >= 0)
        pl_close_control(playlist);

    playlist_write_unlock(playlist);
}

/* returns number of tracks in playlist (includes queued/inserted tracks) */
int playlist_amount_ex(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return playlist->amount;
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
 *
 * XXX: This is really only usable by the playlist viewer. Never pass
 *      playlist == NULL, that cannot and will not work.
 */
int playlist_create_ex(struct playlist_info* playlist,
                       const char* dir, const char* file,
                       void* index_buffer, int index_buffer_size,
                       void* temp_buffer, int temp_buffer_size)
{
    /* Initialize playlist structure */
    int r = rand() % 10;

    /* Use random name for control file */
    snprintf(playlist->control_filename, sizeof(playlist->control_filename),
             "%s.%d", PLAYLIST_CONTROL_FILE, r);
    playlist->fd = -1;
    playlist->control_fd = -1;

    if (index_buffer)
    {
        int num_indices = index_buffer_size /
            playlist_get_required_bufsz(playlist, false, 1);

        if (num_indices > global_settings.max_files_in_playlist)
            num_indices = global_settings.max_files_in_playlist;

        playlist->max_playlist_size = num_indices;
        playlist->indices = index_buffer;
#ifdef HAVE_DIRCACHE
        playlist->dcfrefs_handle = 0;
#endif
    }
    else
    {
        /* FIXME not sure if it's safe to share index buffers */
        playlist->max_playlist_size = current_playlist.max_playlist_size;
        playlist->indices = current_playlist.indices;
#ifdef HAVE_DIRCACHE
        playlist->dcfrefs_handle = 0;
#endif
    }

    new_playlist_unlocked(playlist, dir, file);

    /* load the playlist file */
    if (file)
        add_indices_to_playlist(playlist, temp_buffer, temp_buffer_size);

    return 0;
}

/*
 * Create new playlist
 */
int playlist_create(const char *dir, const char *file)
{
    struct playlist_info* playlist = &current_playlist;
    int status = 0;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    new_playlist_unlocked(playlist, dir, file);

    if (file)
    {
        size_t buflen;
        int handle = alloc_tempbuf(&buflen);
        if (handle > 0)
        {
            /* align for faster load times */
            void* buf = core_get_data(handle);
            STORAGE_ALIGN_BUFFER(buf, buflen);
            buflen = ALIGN_DOWN(buflen, 512); /* to avoid partial sector I/O */
            /* load the playlist file */
            add_indices_to_playlist(playlist, buf, buflen);
            core_free(handle);
        }
        else
        {
            /* should not happen -- happens if plugin takes audio buffer */
            splashf(HZ * 2, "%s(): OOM", __func__);
            status = -1;
        }
    }

    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    return status;
}

/* Returns false if 'steps' is out of bounds, else true */
bool playlist_check(int steps)
{
    struct playlist_info* playlist = &current_playlist;

    /* always allow folder navigation */
    if (global_settings.next_folder && playlist_allow_dirplay(playlist))
        return true;

    int index = get_next_index(playlist, steps, -1);

    if (index < 0 && steps >= 0 && global_settings.repeat_mode == REPEAT_SHUFFLE)
        index = get_next_index(playlist, steps, REPEAT_ALL);

    return (index >= 0);
}

/*
 * Close files and delete control file for non-current playlist.
 */
void playlist_close(struct playlist_info* playlist)
{
    if (!playlist)
        return;

    playlist_write_lock(playlist);

    pl_close_playlist(playlist);
    pl_close_control(playlist);

    if (playlist->control_created)
    {
        remove(playlist->control_filename);
        playlist->control_created = false;
    }

    playlist_write_unlock(playlist);
}

/*
 * Delete track at specified index.
 */
int playlist_delete(struct playlist_info* playlist, int index)
{
    int result = 0;

    if (!playlist)
        playlist = &current_playlist;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    if (check_control(playlist) < 0)
    {
        notify_control_access_error();
        result = -1;
        goto out;
    }

    result = remove_track_unlocked(playlist, index, true);

out:
    playlist_write_unlock(playlist);
    dc_thread_start(playlist, false);

    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY) &&
        playlist->started)
        audio_flush_and_reload_tracks();

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
    int i;;
    struct tree_context* tc = tree_get_context();
    struct tree_cache* cache = &tc->cache;
    int old_dirfilter = *(tc->dirfilter);

    if (!callback)
        return -1;

    /* use the tree browser dircache to load files */
    *(tc->dirfilter) = SHOW_ALL;

    if (ft_load(tc, dirname) < 0)
    {
        splash(HZ*2, ID2P(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
        *(tc->dirfilter) = old_dirfilter;
        return -1;
    }

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

        struct entry *files = core_get_data(cache->entries_handle);
        if (files[i].attr & ATTR_DIRECTORY)
        {
            if (recurse)
            {
                /* recursively add directories */
                if (path_append(buf, dirname, files[i].name, sizeof(buf))
                        >= sizeof(buf))
                {
                    continue;
                }

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
            if (path_append(buf, dirname, files[i].name, sizeof(buf))
                    >= sizeof(buf))
            {
                continue;
            }

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


struct playlist_info *playlist_get_current(void)
{
    return &current_playlist;
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

/* returns the crc32 of the filename of the track at the specified index */
unsigned int playlist_get_filename_crc32(struct playlist_info *playlist,
                                         int index)
{
    const char *basename;
    char filename[MAX_PATH]; /* path name of mp3 file */
    if (!playlist)
        playlist = &current_playlist;

    if (get_track_filename(playlist, index, filename, sizeof(filename)) != 0)
        return -1;

#ifdef HAVE_MULTIVOLUME
    /* remove the volume identifier it might change just use the relative part*/
    path_strip_volume(filename, &basename, false);
    if (basename == NULL)
#endif
        basename = filename;
    NOTEF("%s: %s", __func__, basename);
    return crc_32(basename, strlen(basename), -1);
}

/* returns index of first track in playlist */
int playlist_get_first_index(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return playlist->first_index;
}

/* returns the playlist filename */
char *playlist_get_name(const struct playlist_info* playlist, char *buf,
                        int buf_size)
{
    if (!playlist)
        playlist = &current_playlist;

    strmemccpy(buf, playlist->filename, buf_size);

    if (!buf[0])
        return NULL;

    return buf;
}

/* return size of buffer needed for playlist to initialize num_indices entries */
size_t playlist_get_required_bufsz(struct playlist_info* playlist,
                                             bool include_namebuf,
                                                   int num_indices)
{
    size_t namebuf = 0;

    if (!playlist)
        playlist = &current_playlist;

    size_t unit_size = sizeof (*playlist->indices);
    if (include_namebuf)
        namebuf = AVERAGE_FILENAME_LENGTH * global_settings.max_files_in_dir;

    return (num_indices * unit_size) + namebuf;
}

/* Get resume info for current playing song.  If return value is -1 then
   settings shouldn't be saved. */
int playlist_get_resume_info(int *resume_index)
{
    struct playlist_info* playlist = &current_playlist;

    return (*resume_index = playlist->index);
}

/* returns shuffle seed of playlist */
int playlist_get_seed(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return playlist->seed;
}

/* Fills info structure with information about track at specified index.
   Returns 0 on success and -1 on failure */
int playlist_get_track_info(struct playlist_info* playlist, int index,
                            struct playlist_track_info* info)
{
    if (!playlist)
        playlist = &current_playlist;

    if (get_track_filename(playlist, index,
                           info->filename, sizeof(info->filename)) != 0)
        return -1;

    info->attr = 0;

    if (playlist->indices[index] & PLAYLIST_INSERT_TYPE_MASK)
    {
        if (playlist->indices[index] & PLAYLIST_QUEUED)
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

/*
 * initialize an insert context to add tracks to a playlist
 * don't forget to release it when finished adding files
 */
int playlist_insert_context_create(struct playlist_info* playlist,
                                   struct playlist_insert_context *context,
                                   int position, bool queue, bool progress)
{

    if (!playlist)
        playlist = &current_playlist;

    context->playlist = playlist;
    context->initialized = false;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    if (check_control(playlist) < 0)
    {
        notify_control_access_error();
        return -1;
    }

    if (position == PLAYLIST_REPLACE)
    {
        if (remove_all_tracks_unlocked(playlist, true) == 0)
            position = PLAYLIST_INSERT_LAST;
        else
        {
            return -1;
        }
    }

    context->playlist = playlist;
    context->position = position;
    context->queue = queue;
    context->count = 0;
    context->progress = progress;
    context->initialized = true;

    if (queue)
        context->count_langid = LANG_PLAYLIST_QUEUE_COUNT;
    else
        context->count_langid = LANG_PLAYLIST_INSERT_COUNT;

    return 0;
}

/*
 * add tracks to playlist using opened insert context
 */
int playlist_insert_context_add(struct playlist_insert_context *context,
                                const char *filename)
{
    struct playlist_insert_context* c = context;
    int insert_pos;

    insert_pos = add_track_to_playlist_unlocked(c->playlist, filename,
                                                c->position, c->queue, -1);

    if (insert_pos < 0)
        return -1;

    (c->count)++;

    /* After first INSERT_FIRST switch to INSERT so that all the
    rest of the tracks get inserted one after the other */
    if (c->position == PLAYLIST_INSERT_FIRST)
        c->position = PLAYLIST_INSERT;

    if (((c->count)%PLAYLIST_DISPLAY_COUNT) == 0)
    {
        if (c->progress)
            display_playlist_count(c->count, ID2P(c->count_langid), false);

        if ((c->count) == PLAYLIST_DISPLAY_COUNT &&
            (audio_status() & AUDIO_STATUS_PLAY) &&
            c->playlist->started)
            audio_flush_and_reload_tracks();
    }

    return 0;
}

/*
 * release opened insert context, sync playlist
 */
void playlist_insert_context_release(struct playlist_insert_context *context)
{

    struct playlist_info* playlist = context->playlist;
    if (context->initialized)
        sync_control_unlocked(playlist);
    if (context->progress)
        display_playlist_count(context->count, ID2P(context->count_langid), true);

    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    if ((audio_status() & AUDIO_STATUS_PLAY) && playlist->started)
        audio_flush_and_reload_tracks();
}

/*
 * Insert all tracks from specified directory into playlist.
 */
int playlist_insert_directory(struct playlist_info* playlist,
                              const char *dirname, int position, bool queue,
                              bool recurse)
{
    int result = -1;
    struct playlist_insert_context context;
    result = playlist_insert_context_create(playlist, &context,
                                            position, queue, true);
    if (result >= 0)
    {
        cpu_boost(true);

        result = playlist_directory_tracksearch(dirname, recurse,
            directory_search_callback, &context);

        cpu_boost(false);
    }
    playlist_insert_context_release(&context);
    return result;
}

/*
 * If action_cb is *not* NULL, it will be called for every track contained
 * in the playlist specified by filename. If action_cb is NULL, you must
 * instead provide a playlist insert context to use for adding each track
 * into a dynamic playlist.
 */
bool playlist_entries_iterate(const char *filename,
                              struct playlist_insert_context *pl_context,
                              bool (*action_cb)(const char *file_name))
{
    int fd = -1, i = 0;
    bool ret = false;
    int max;
    char *dir;
    off_t filesize;

    char temp_buf[MAX_PATH+1];
    char trackname[MAX_PATH+1];

    bool utf8 = is_m3u8_name(filename);

    cpu_boost(true);

    fd = open_utf8(filename, O_RDONLY);
    if (fd < 0)
    {
        notify_access_error();
        goto out;
    }
    off_t start = lseek(fd, 0, SEEK_CUR);
    filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, start, SEEK_SET);
    /* we need the directory name for formatting purposes */
    size_t dirlen = path_dirname(filename, (const char **)&dir);
    //dir = strmemdupa(dir, dirlen);


    if (action_cb)
        show_search_progress(true, 0, 0, 0);

    while ((max = read_line(fd, temp_buf, sizeof(temp_buf))) > 0)
    {
        /* user abort */
        if (!action_cb && action_userabort(TIMEOUT_NOBLOCK))
            break;

        if (temp_buf[0] != '#' && temp_buf[0] != '\0')
        {
            i++;
            if (!utf8)
            {
                /* Use trackname as a temporay buffer. Note that trackname must
                 * be as large as temp_buf.
                 */
                max = convert_m3u_name(temp_buf, max, sizeof(temp_buf), trackname);
            }

            /* we need to format so that relative paths are correctly
               handled */
            if ((max = format_track_path(trackname, temp_buf,
                                  sizeof(trackname), dir, dirlen)) < 0)
            {
                goto out;
            }
            start += max;
            if (action_cb)
            {
                if (!action_cb(trackname))
                    goto out;
                else if (!show_search_progress(false, i, start, filesize))
                    break;
            }
            else if (playlist_insert_context_add(pl_context, trackname) < 0)
                goto out;
        }

        /* let the other threads work */
        yield();
    }
    ret = true;

out:
    close(fd);
    cpu_boost(false);
    return ret;
}

/*
 * Insert all tracks from specified playlist into dynamic playlist.
 */
int playlist_insert_playlist(struct playlist_info* playlist, const char *filename,
                             int position, bool queue)
{

    int result = -1;

    struct playlist_insert_context pl_context;
    cpu_boost(true);

    if (playlist_insert_context_create(playlist, &pl_context, position, queue, true) >= 0
         && playlist_entries_iterate(filename, &pl_context, NULL))
        result = 0;

    cpu_boost(false);
    playlist_insert_context_release(&pl_context);
    return result;
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

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    if (check_control(playlist) < 0)
    {
        notify_control_access_error();
        return -1;
    }

    result = add_track_to_playlist_unlocked(playlist, filename,
                                            position, queue, -1);

    /* Check if we want manually sync later. For example when adding
     * bunch of files from tagcache, syncing after every file wouldn't be
     * a good thing to do. */
    if (sync && result >= 0)
        playlist_sync(playlist);

    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    return result;
}

/* returns true if playlist has been modified by the user */
bool playlist_modified(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return !!(playlist->flags & PLAYLIST_FLAG_MODIFIED);
}

/*
 * Set the playlist modified status. Should be called to set the flag after
 * an explicit user action that modifies the playlist. You should not clear
 * the modified flag without properly warning the user.
 */
void playlist_set_modified(struct playlist_info* playlist, bool modified)
{
    if (!playlist)
        playlist = &current_playlist;

    playlist_write_lock(playlist);

    if (modified)
        update_playlist_flags_unlocked(playlist, PLAYLIST_FLAG_MODIFIED, 0);
    else
        update_playlist_flags_unlocked(playlist, 0, PLAYLIST_FLAG_MODIFIED);

    playlist_write_unlock(playlist);
}

/* returns true if directory playback features should be enabled */
bool playlist_allow_dirplay(const struct playlist_info *playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    if (playlist_modified(playlist))
        return false;

    return !!(playlist->flags & PLAYLIST_FLAG_DIRPLAY);
}

/*
 * Returns true if the current playlist is neither
 * associated  with a folder nor with an on-disk playlist.
 */
bool playlist_dynamic_only(void)
{
    /* NOTE: New dynamic playlists currently use root dir ("/")
     *       as their placeholder filename – this could change.
     */
    if (!strcmp(current_playlist.filename, "/") &&
        !(current_playlist.flags & PLAYLIST_FLAG_DIRPLAY))
        return true;

    return false;
}

/*
 * Move track at index to new_index.  Tracks between the two are shifted
 * appropriately.  Returns 0 on success and -1 on failure.
 */
int playlist_move(struct playlist_info* playlist, int index, int new_index)
{
    int result = -1;
    bool queue;
    bool current = false;
    int r;
    struct playlist_track_info info;
    int idx_cur; /* display index of the currently playing track */
    int idx_from; /* display index of the track we're moving */
    int idx_to; /* display index of the position we're moving to */
    bool displace_current = false;
    char filename[MAX_PATH];

    if (!playlist)
        playlist = &current_playlist;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    if (check_control(playlist) < 0)
    {
        notify_control_access_error();
        goto out;
    }

    if (index == new_index)
        goto out;

    if (index == playlist->index)
    {
        /* Moving the current track */
        current = true;
    }
    else
    {
        /* Get display index of the currently playing track */
        if (playlist_get_track_info(playlist, playlist->index, &info) != -1)
        {
            idx_cur = info.display_index;
            /* Get display index of the position we're moving to */
            if (playlist_get_track_info(playlist, new_index, &info) != -1)
            {
                idx_to = info.display_index;
                /* Get display index of the track we're trying to move */
                if (playlist_get_track_info(playlist, index, &info) != -1)
                {
                    idx_from = info.display_index;
                    /* Check if moving will displace the current track.
                       Displace happens when moving from after current to
                       before, but also when moving from before to before
                       due to the removal from the original position */
                    if ( ((idx_from > idx_cur) && (idx_to <= idx_cur)) ||
                         ((idx_from < idx_cur) && (idx_to < idx_cur)) )
                        displace_current = true;
                }
            }
        }
    }

    queue = playlist->indices[index] & PLAYLIST_QUEUED;

    if (get_track_filename(playlist, index, filename, sizeof(filename)) != 0)
        goto out;

    /* We want to insert the track at the position that was specified by
       new_index.  This may be different then new_index because of the
       shifting that will occur after the delete.
       We calculate this before we do the remove as it depends on the
       size of the playlist before the track removal */
    r = rotate_index(playlist, new_index);

    /* Delete track from original position */
    result = remove_track_unlocked(playlist, index, true);
    if (result == -1)
        goto out;

    if (r == 0)/* First index */
    {
        new_index = PLAYLIST_PREPEND;
    }
    else if (r == playlist->amount)
    {
        /* Append */
        new_index = PLAYLIST_INSERT_LAST;
    }
    else /* Calculate index of desired position */
    {
        new_index = (r+playlist->first_index)%playlist->amount;

        if ((new_index < playlist->first_index) && (new_index <= playlist->index))
            displace_current = true;
        else if ((new_index >= playlist->first_index) && (playlist->index < playlist->first_index))
            displace_current = false;
    }

    result = add_track_to_playlist_unlocked(playlist, filename,
                                            new_index, queue, -1);
    if (result == -1)
        goto out;

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
    else if ((displace_current) && (new_index != PLAYLIST_PREPEND))
    {
        /* make the index point to the currently playing track */
        playlist->index++;
    }

out:
    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    if (result != -1 && playlist->started && (audio_status() & AUDIO_STATUS_PLAY))
        audio_flush_and_reload_tracks();

    return result;
}

/* returns full path of playlist (minus extension) */
char *playlist_name(const struct playlist_info* playlist, char *buf,
                    int buf_size)
{
    char *sep;

    if (!playlist)
        playlist = &current_playlist;

    strmemccpy(buf, playlist->filename+playlist->dirlen, buf_size);

    if (!buf[0])
        return NULL;

    /* Remove extension */
    sep = strrchr(buf, '.');
    if (sep)
        *sep = 0;

    return buf;
}

/*
 * Update indices as track has changed
 */
int playlist_next(int steps)
{
    struct playlist_info* playlist = &current_playlist;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    int index;
    int repeat_mode = global_settings.repeat_mode;
    if (repeat_mode == REPEAT_ONE)
    {
        if (is_manual_skip())
            repeat_mode = REPEAT_ALL;
    }
    if (steps > 0)
    {
#ifdef AB_REPEAT_ENABLE
        if (repeat_mode != REPEAT_AB && repeat_mode != REPEAT_ONE)
#else
        if (repeat_mode != REPEAT_ONE)
#endif
        {
            int i, j;
            /* We need to delete all the queued songs */
            for (i=0, j=steps; i<j; i++)
            {
                index = get_next_index(playlist, i, -1);

                if (index >= 0 && playlist->indices[index] & PLAYLIST_QUEUED)
                {
                    remove_track_unlocked(playlist, index, true);
                    steps--; /* one less track */
                }
            }
        }
    } /*steps > 0*/
    index = get_next_index(playlist, steps, repeat_mode);

    if (index < 0)
    {
        /* end of playlist... or is it */
        if (repeat_mode == REPEAT_SHUFFLE && playlist->amount > 1)
        {
            /* Repeat shuffle mode.  Re-shuffle playlist and resume play */
            playlist->first_index = 0;
            sort_playlist_unlocked(playlist, false, false);
            randomise_playlist_unlocked(playlist, current_tick, false, true);
            global_settings.playlist_shuffle = true;

            playlist->started = true;
            playlist->index = 0;
            index = 0;
        }
        else if (global_settings.next_folder && playlist_allow_dirplay(playlist))
        {
            /* we switch playlists here */
            index = create_and_play_dir(steps, true);
            if (index >= 0)
            {
                playlist->index = index;
            }
        }
        goto out;
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
                int result = update_control_unlocked(playlist,
                                                     PLAYLIST_COMMAND_RESET,
                                                     -1, -1, NULL, NULL, NULL);
                if (result < 0)
                {
                    index = result;
                    goto out;
                }

                sync_control_unlocked(playlist);
            }
        }
    }

out:
    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    return index;
}

/* try playing next or previous folder */
bool playlist_next_dir(int direction)
{
    /* not to mess up real playlists */
    if (!playlist_allow_dirplay(&current_playlist))
        return false;

    return create_and_play_dir(direction, false) >= 0;
}

/* get trackname of track that is "steps" away from current playing track.
   NULL is used to identify end of playlist */
const char* playlist_peek(int steps, char* buf, size_t buf_size)
{
    struct playlist_info* playlist = &current_playlist;
    char *temp_ptr;
    int index = get_next_index(playlist, steps, -1);

    if (index < 0)
        return NULL;

    /* Just testing - don't care about the file name */
    if (!buf || !buf_size)
        return "";

    if (get_track_filename(playlist, index, buf, buf_size) != 0)
        return NULL;

    temp_ptr = buf;

    /* remove bogus dirs from beginning of path
       (workaround for buggy playlist creation tools) */
    while (temp_ptr)
    {
        if (file_exists(temp_ptr))
            break;

        temp_ptr = strchr(temp_ptr+1, '/');
    }

    if (!temp_ptr)
    {
        /* Even though this is an invalid file, we still need to pass a
           file name to the caller because NULL is used to indicate end
           of playlist */
        return buf;
    }

    return temp_ptr;
}

/*
 * shuffle currently playing playlist
 *
 * TODO: Merge this with playlist_shuffle()?
 */
int playlist_randomise(struct playlist_info* playlist, unsigned int seed,
                       bool start_current)
{
    int result;

    if (!playlist)
        playlist = &current_playlist;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    check_control(playlist);

    result = randomise_playlist_unlocked(playlist, seed, start_current, true);
    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY) &&
        playlist->started)
    {
        audio_flush_and_reload_tracks();
    }

    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    return result;
}

/*
 * Removes all tracks, from the playlist, leaving the presently playing
 * track queued.
 */
int playlist_remove_all_tracks(struct playlist_info *playlist)
{
    int result;

    if (playlist == NULL)
        playlist = &current_playlist;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    result = remove_all_tracks_unlocked(playlist, true);

    playlist_write_unlock(playlist);
    dc_thread_start(playlist, false);
    return result;
}

/* playlist_resume helper function
 * only allows comments (#) and PLAYLIST_COMMAND_PLAYLIST (P)
 */
static enum playlist_command pl_cmds_start(char cmd)
{
    if (cmd == 'P')
        return PLAYLIST_COMMAND_PLAYLIST;
    if (cmd == '#')
        return PLAYLIST_COMMAND_COMMENT;

    return PLAYLIST_COMMAND_ERROR;
}

/* playlist resume helper function excludes PLAYLIST_COMMAND_PLAYLIST (P) */
static enum playlist_command pl_cmds_run(char cmd)
{
    switch (cmd)
    {
        case 'A':
            return PLAYLIST_COMMAND_ADD;
        case 'Q':
            return PLAYLIST_COMMAND_QUEUE;
        case 'D':
            return PLAYLIST_COMMAND_DELETE;
        case 'S':
            return PLAYLIST_COMMAND_SHUFFLE;
        case 'U':
            return PLAYLIST_COMMAND_UNSHUFFLE;
        case 'R':
            return PLAYLIST_COMMAND_RESET;
        case 'C':
            return PLAYLIST_COMMAND_CLEAR;
        case 'F':
            return PLAYLIST_COMMAND_FLAGS;
        case '#':
            return PLAYLIST_COMMAND_COMMENT;
        default: /* ERROR */
            break;
    }
    return PLAYLIST_COMMAND_ERROR;
}

/*
 * Restore the playlist state based on control file commands.  Called to
 * resume playback after shutdown.
 */
int playlist_resume(void)
{
    char *buffer;
    size_t buflen;
    size_t readsize;
    int handle;
    int nread;
    int total_read = 0;
    int control_file_size = 0;
    bool sorted = true;
    int result = -1;
    enum playlist_command (*pl_cmd)(char) = &pl_cmds_start;

    splash(0, ID2P(LANG_WAIT));
    cpu_boost(true);

    struct playlist_info* playlist = &current_playlist;
    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    if (core_allocatable() < (1 << 10))
        talk_buffer_set_policy(TALK_BUFFER_LOOSE); /* back off voice buffer */

#ifdef HAVE_DIRCACHE
    dircache_wait(); /* we need the dircache to use the files in the playlist */
#endif

    handle = alloc_tempbuf(&buflen);
    if (handle < 0)
    {
        splashf(HZ * 2, "%s(): OOM", __func__);
        goto out;
    }

    /* align buffer for faster load times */
    buffer = core_get_data(handle);
    STORAGE_ALIGN_BUFFER(buffer, buflen);
    buflen = ALIGN_DOWN(buflen, 512); /* to avoid partial sector I/O */

    empty_playlist_unlocked(playlist, true);

    playlist->control_fd = open(playlist->control_filename, O_RDWR);
    if (playlist->control_fd < 0)
    {
        notify_control_access_error();
        goto out;
    }
    playlist->control_created = true;

    control_file_size = filesize(playlist->control_fd);
    if (control_file_size <= 0)
    {
        notify_control_access_error();
        goto out;
    }

    /* read a small amount first to get the header */
    readsize = (PLAYLIST_COMMAND_SIZE<buflen?PLAYLIST_COMMAND_SIZE:buflen);
    nread = read(playlist->control_fd, buffer, readsize);
    if(nread <= 0)
    {
        notify_control_access_error();
        goto out;
    }

    playlist->started = true;

    while (1)
    {
        result = 0;
        int count;
        enum playlist_command current_command = PLAYLIST_COMMAND_COMMENT;
        int last_newline = 0;
        int str_count = -1;
        bool newline = true;
        bool exit_loop = false;
        char *p = buffer;
        char *strp[3] = {NULL};

        unsigned long last_tick = current_tick;
        splash_progress_set_delay(HZ / 2); /* wait 1/2 sec before progress */
        bool useraborted = false;
        bool queue = false;

        for(count=0; count<nread && !exit_loop; count++,p++)
        {
            /* Show a splash while we are loading. */
            splash_progress((total_read + count), control_file_size,
                         "%s (%s)", str(LANG_WAIT), str(LANG_OFF_ABORT));
            if (TIME_AFTER(current_tick, last_tick + HZ/4))
            {
                if (action_userabort(TIMEOUT_NOBLOCK))
                {
                    useraborted = true;
                    exit_loop = true;
                    break;
                }
                last_tick = current_tick;
            }
            /* Are we on a new line? */
            if((*p == '\n') || (*p == '\r'))
            {
                *p = '\0';

                switch (current_command)
                {
                    case PLAYLIST_COMMAND_ERROR:
                    {
                        /* first non-comment line does not specify playlist */
                        /* ( below handled by pl_cmds_run() ) */
                        /* OR playlist is specified more than once */
                        /* OR unknown command -- pl corrupted?? */
                        result = -12;
                        exit_loop = true;
                        break;
                    }
                    case PLAYLIST_COMMAND_PLAYLIST:
                    {
                        /* strp[0]=version strp[1]=dir strp[2]=file */
                        int version;

                        if (!strp[0])
                        {
                            result = -2;
                            exit_loop = true;
                            break;
                        }

                        if (!strp[1])
                            strp[1] = "";

                        if (!strp[2])
                            strp[2] = "";

                        version = atoi(strp[0]);

                        /*
                         * TODO: Playlist control file version upgrades
                         *
                         * If an older version control file is loaded then
                         * the header should be updated to the latest version
                         * in case any incompatible commands are written out.
                         * (It's not a big deal since the error message will
                         * be practically the same either way...)
                         */
                        if (version < PLAYLIST_CONTROL_FILE_MIN_VERSION ||
                            version > PLAYLIST_CONTROL_FILE_VERSION)
                        {
                            result = -3;
                            goto out;
                        }

                        update_playlist_filename_unlocked(playlist, strp[1], strp[2]);

                        if (strp[2][0] != '\0')
                        {
                            /* NOTE: add_indices_to_playlist() overwrites the
                               audiobuf so we need to reload control file
                               data */
                            add_indices_to_playlist(playlist, buffer, buflen);
                        }
                        else if (strp[1][0] != '\0')
                        {
                            playlist->flags |= PLAYLIST_FLAG_DIRPLAY;
                        }

                        /* load the rest of the data */
                        exit_loop = true;
                        readsize = buflen;
                        pl_cmd = &pl_cmds_run;
                        break;
                    }
                    case PLAYLIST_COMMAND_QUEUE:
                        queue = true;
                        /*Fall-through*/
                    case PLAYLIST_COMMAND_ADD:
                    {
                        /* strp[0]=position strp[1]=last_position strp[2]=file */
                        if (!strp[0] || !strp[1] || !strp[2])
                        {
                            result = -4;
                            exit_loop = true;
                            break;
                        }

                        int position = atoi(strp[0]);
                        int last_position = atoi(strp[1]);

                        /* seek position is based on strp[2]'s position in
                           buffer */
                        if (add_track_to_playlist_unlocked(playlist, strp[2],
                                position, queue, total_read+(strp[2]-buffer)) < 0)
                        {
                            result = -5;
                            goto out;
                        }

                        playlist->last_insert_pos = last_position;
                        queue = false;
                        break;
                    }
                    case PLAYLIST_COMMAND_DELETE:
                    {
                        /* strp[0]=position */
                        int position;

                        if (!strp[0])
                        {
                            result = -6;
                            exit_loop = true;
                            break;
                        }

                        position = atoi(strp[0]);

                        if (remove_track_unlocked(playlist, position, false) < 0)
                        {
                            result = -7;
                            goto out;
                        }

                        break;
                    }
                    case PLAYLIST_COMMAND_SHUFFLE:
                    {
                        /* strp[0]=seed strp[1]=first_index */
                        int seed;

                        if (!strp[0] || !strp[1])
                        {
                            result = -8;
                            exit_loop = true;
                            break;
                        }

                        if (!sorted)
                        {
                            /* Always sort list before shuffling */
                            sort_playlist_unlocked(playlist, false, false);
                        }

                        seed = atoi(strp[0]);
                        playlist->first_index = atoi(strp[1]);

                        if (randomise_playlist_unlocked(playlist, seed, false,
                                false) < 0)
                        {
                            result = -9;
                            goto out;
                        }
                        sorted = false;

                        break;
                    }
                    case PLAYLIST_COMMAND_UNSHUFFLE:
                    {
                        /* strp[0]=first_index */
                        if (!strp[0])
                        {
                            result = -10;
                            exit_loop = true;
                            break;
                        }

                        playlist->first_index = atoi(strp[0]);

                        if (sort_playlist_unlocked(playlist, false, false) < 0)
                        {
                            result = -11;
                            goto out;
                        }

                        sorted = true;
                        break;
                    }
                    case PLAYLIST_COMMAND_RESET:
                    {
                        playlist->last_insert_pos = -1;
                        break;
                    }
                    case PLAYLIST_COMMAND_CLEAR:
                    {
                        if (strp[0])
                            playlist->index = atoi(strp[0]);
                        if (remove_all_tracks_unlocked(playlist, false) < 0)
                        {
                            result = -16;
                            goto out;
                        }
                        break;
                    }
                    case PLAYLIST_COMMAND_FLAGS:
                    {
                        if (!strp[0] || !strp[1])
                        {
                            result = -18;
                            exit_loop = true;
                            break;
                        }
                        unsigned int setf = atoi(strp[0]);
                        unsigned int clearf = atoi(strp[1]);

                        playlist->flags = (playlist->flags & ~clearf) | setf;
                        break;
                    }
                    case PLAYLIST_COMMAND_COMMENT:
                    default:
                        break;
                }

                /* save last_newline in case we need to load more data */
                last_newline = count;
                newline = true;

                /* to ignore any extra newlines */
                current_command = PLAYLIST_COMMAND_COMMENT;
            }
            else if(newline)
            {
                newline = false;
                current_command = (*pl_cmd)(*p);
                str_count = -1;
                strp[0] = NULL;
                strp[1] = NULL;
                strp[2] = NULL;
            }
            else if(current_command < PLAYLIST_COMMAND_COMMENT)
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
                        case 1:
                        case 2:
                            strp[str_count] = p+1;
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

        if (result < 0 || current_command == PLAYLIST_COMMAND_ERROR)
        {
            splashf(HZ*2, "Err: %d, %s", result, str(LANG_PLAYLIST_CONTROL_INVALID));
            goto out;
        }

        if (useraborted)
        {
            splash(HZ*2, ID2P(LANG_CANCEL));
            result = -1;
            goto out;
        }

        if (!newline || (exit_loop && count<nread))
        {
            if ((total_read + count) >= control_file_size)
            {
                /* no newline at end of control file */
                splashf(HZ*2, "Err: EOF, %s", str(LANG_PLAYLIST_CONTROL_INVALID));
                result = -15;
                goto out;
            }

            /* We didn't end on a newline or we exited loop prematurely.
               Either way, re-read the remainder. */
            count = last_newline;
            lseek(playlist->control_fd, total_read+count, SEEK_SET);
        }

        total_read += count;

        nread = read(playlist->control_fd, buffer, readsize);

        /* Terminate on EOF */
        if(nread <= 0)
        {
            break;
        }
    }

out:
    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    talk_buffer_set_policy(TALK_BUFFER_DEFAULT);
    core_free(handle);
    cpu_boost(false);
    return result;
}

/* resume a playlist track with the given crc_32 of the track name. */
void playlist_resume_track(int start_index, unsigned int crc,
                           unsigned long elapsed, unsigned long offset)
{
    int i;
    unsigned int tmp_crc;
    struct playlist_info* playlist = &current_playlist;

    for (i = 0 ; i < playlist->amount; i++)
    {
        int index = (i + start_index) % playlist->amount;

        tmp_crc = playlist_get_filename_crc32(playlist, index);
        if (tmp_crc == crc)
        {
            playlist_start(index, elapsed, offset);
            return;
        }
    }

    /* If we got here the file wasnt found, so start from the beginning */
    playlist_start(0, 0, 0);
}

/*
 * Set the specified playlist as the current.
 * NOTE: You will get undefined behaviour if something is already playing so
 *       remember to stop before calling this.  Also, this call will
 *       effectively close your playlist, making it unusable.
 */
int playlist_set_current(struct playlist_info* playlist)
{
    int result = -1;

    if (!playlist || (check_control(playlist) < 0))
        return result;

    dc_thread_stop(&current_playlist);
    playlist_write_lock(&current_playlist);

    empty_playlist_unlocked(&current_playlist, false);

    strmemccpy(current_playlist.filename, playlist->filename,
        sizeof(current_playlist.filename));

    current_playlist.utf8 = playlist->utf8;
    current_playlist.fd = playlist->fd;

    pl_close_control(playlist);
    pl_close_control(&current_playlist);

    remove(current_playlist.control_filename);
    current_playlist.control_created = false;

    if (rename(playlist->control_filename, current_playlist.control_filename) < 0)
        goto out;

    current_playlist.control_fd = open(current_playlist.control_filename,
        O_RDWR);

    if (current_playlist.control_fd < 0)
        goto out;

    current_playlist.control_created = true;
    current_playlist.dirlen = playlist->dirlen;

    if (playlist->indices && playlist->indices != current_playlist.indices)
        memcpy((void*)current_playlist.indices, (void*)playlist->indices,
               playlist->max_playlist_size*sizeof(*playlist->indices));
    dc_init_filerefs(&current_playlist, 0, current_playlist.max_playlist_size);

    current_playlist.first_index = playlist->first_index;
    current_playlist.amount = playlist->amount;
    current_playlist.last_insert_pos = playlist->last_insert_pos;
    current_playlist.seed = playlist->seed;
    current_playlist.flags = playlist->flags;

    result = 0;

out:
    playlist_write_unlock(&current_playlist);
    dc_thread_start(&current_playlist, true);

    return result;
}

/* set playlist->last_shuffle_start to playlist end for
   PLAYLIST_INSERT_LAST_SHUFFLED command purposes*/
void playlist_set_last_shuffled_start(void)
{
    struct playlist_info* playlist = &current_playlist;
    playlist_write_lock(playlist);
    playlist->last_shuffled_start = playlist->first_index > 0 ?
                                    playlist->first_index : playlist->amount;
    playlist_write_unlock(playlist);
}

/* shuffle newly created playlist using random seed. */
int playlist_shuffle(int random_seed, int start_index)
{
    struct playlist_info* playlist = &current_playlist;
    bool start_current = false;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    if (start_index >= 0 && global_settings.play_selected)
    {
        /* store the seek position before the shuffle */
        playlist->index = playlist->first_index = start_index;
        start_current = true;
    }

    randomise_playlist_unlocked(playlist, random_seed, start_current, true);

    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);

    return playlist->index;
}

/* Marks the index of the track to be skipped that is "steps" away from
 * current playing track.
 */
void playlist_skip_entry(struct playlist_info *playlist, int steps)
{
    int index;

    if (playlist == NULL)
        playlist = &current_playlist;

    playlist_write_lock(playlist);

    /* need to account for already skipped tracks */
    steps = calculate_step_count(playlist, steps);

    index = playlist->index + steps;
    if (index < 0)
        index += playlist->amount;
    else if (index >= playlist->amount)
        index -= playlist->amount;

    playlist->indices[index] |= PLAYLIST_SKIPPED;
    playlist_write_unlock(playlist);
}

/* sort currently playing playlist */
int playlist_sort(struct playlist_info* playlist, bool start_current)
{
    int result;

    if (!playlist)
        playlist = &current_playlist;

    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    check_control(playlist);
    result = sort_playlist_unlocked(playlist, start_current, true);

    if (result != -1 && (audio_status() & AUDIO_STATUS_PLAY) && playlist->started)
        audio_flush_and_reload_tracks();

    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);
    return result;
}

/* start playing current playlist at specified index/offset */
void playlist_start(int start_index, unsigned long elapsed,
                    unsigned long offset)
{
    struct playlist_info* playlist = &current_playlist;

    playlist_write_lock(playlist);

    playlist->index = start_index;
    playlist->started = true;

    sync_control_unlocked(playlist);

    playlist_write_unlock(playlist);

    audio_play(elapsed, offset);
    audio_resume();
}

void playlist_sync(struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    playlist_write_lock(playlist);

    sync_control_unlocked(playlist);

    playlist_write_unlock(playlist);

    if ((audio_status() & AUDIO_STATUS_PLAY) && playlist->started)
        audio_flush_and_reload_tracks();
}

/* Update resume info for current playing song.  Returns -1 on error. */
int playlist_update_resume_info(const struct mp3entry* id3)
{
    struct playlist_info* playlist = &current_playlist;

    if (id3)
    {
        if (global_status.resume_index  != playlist->index ||
            global_status.resume_elapsed != id3->elapsed ||
            global_status.resume_offset != id3->offset)
        {
            unsigned int crc = playlist_get_filename_crc32(playlist,
                                                           playlist->index);
            global_status.resume_index  = playlist->index;
            global_status.resume_crc32 = crc;
            global_status.resume_elapsed = id3->elapsed;
            global_status.resume_offset = id3->offset;
            status_save();
        }
    }
    else
    {
        global_status.resume_index  = -1;
        global_status.resume_crc32 = -1;
        global_status.resume_elapsed = -1;
        global_status.resume_offset = -1;
        status_save();
        return -1;
    }

    return 0;
}

static int pl_get_tempname(const char *filename, char *buf, size_t bufsz)
{
    if (strlcpy(buf, filename, bufsz) >= bufsz)
        return -1;

    if (strlcat(buf, "_temp", bufsz) >= bufsz)
        return -1;

    return 0;
}

/*
 * Save all non-queued tracks to an M3U playlist with the given filename.
 * On success, the playlist is updated to point to the new playlist file.
 * On failure, the playlist filename is unchanged, but playlist indices
 * may be trashed; the current playlist should be reloaded.
 *
 * Returns 0 on success, < 0 on error, and > 0 if user canceled.
 */
static int pl_save_playlist(struct playlist_info* playlist,
                            const char *filename,
                            char *tmpbuf, size_t tmpsize)
{
    int fd, index, num_saved;
    off_t offset;
    int ret, err;

    if (pl_get_tempname(filename, tmpbuf, tmpsize))
        return -1;

    /*
     * We always save playlists as UTF-8. Add a BOM only when
     * saving to the .m3u file extension.
     */
    if (is_m3u8_name(filename))
        fd = open(tmpbuf, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    else
        fd = open_utf8(tmpbuf, O_CREAT|O_WRONLY|O_TRUNC);

    if (fd < 0)
        return -1;

    offset = lseek(fd, 0, SEEK_CUR);
    index = playlist->first_index;
    num_saved = 0;

    display_playlist_count(num_saved, ID2P(LANG_PLAYLIST_SAVE_COUNT), true);

    for (int i = 0; i < playlist->amount; ++i, ++index)
    {
        if (index == playlist->amount)
            index = 0;

        /* TODO: Disabled for now, as we can't restore the playlist state yet */
        if (false && action_userabort(TIMEOUT_NOBLOCK))
        {
            err = 1;
            goto error;
        }

        /* Do not save queued files to playlist. */
        if (playlist->indices[index] & PLAYLIST_QUEUED)
            continue;

        if (get_track_filename(playlist, index, tmpbuf, tmpsize) != 0)
        {
            err = -2;
            goto error;
        }

        /* Update seek offset so it points into the new file. */
        playlist->indices[index] &= ~PLAYLIST_INSERT_TYPE_MASK;
        playlist->indices[index] &= ~PLAYLIST_SEEK_MASK;
        playlist->indices[index] |= offset;

        ret = fdprintf(fd, "%s\n", tmpbuf);
        if (ret < 0)
        {
            err = -3;
            goto error;
        }

        offset += ret;
        num_saved++;

        if ((num_saved % PLAYLIST_DISPLAY_COUNT) == 0)
            display_playlist_count(num_saved, ID2P(LANG_PLAYLIST_SAVE_COUNT), false);
    }

    display_playlist_count(num_saved, ID2P(LANG_PLAYLIST_SAVE_COUNT), true);
    close(fd);
    pl_close_playlist(playlist);

    pl_get_tempname(filename, tmpbuf, tmpsize);
    if (rename(tmpbuf, filename))
        return -4;

    strcpy(tmpbuf, filename);
    char *dir = tmpbuf;
    char *file = strrchr(tmpbuf, '/') + 1;
    file[-1] = '\0';

    update_playlist_filename_unlocked(playlist, dir, file);
    return 0;

error:
    close(fd);
    pl_get_tempname(filename, tmpbuf, tmpsize);
    remove(tmpbuf);
    return err;
}

static void pl_reverse(struct playlist_info *playlist, int start, int end)
{
    for (; start < end; start++, end--)
    {
        unsigned long index_swap = playlist->indices[start];
        playlist->indices[start] = playlist->indices[end];
        playlist->indices[end]   = index_swap;

#ifdef HAVE_DIRCACHE
        if (playlist->dcfrefs_handle)
        {
            struct dircache_fileref *dcfrefs = core_get_data(playlist->dcfrefs_handle);
            struct dircache_fileref dcf_swap = dcfrefs[start];
            dcfrefs[start] = dcfrefs[end];
            dcfrefs[end] = dcf_swap;
        }
#endif
    }
}

/*
 * Update the control file after saving the playlist under a new name.
 * A new control file is generated, containing the new playlist filename.
 * Queued tracks are copied to the new control file.
 *
 * On success, the new control file replaces the old control file.
 * On failure, indices may be trashed and the playlist should be
 * reloaded. This may not be possible if the playlist was overwritten.
 */
static int pl_save_update_control(struct playlist_info* playlist,
                                  char *tmpbuf, size_t tmpsize)
{
    int old_fd;
    int err = 0;
    char c;
    bool any_queued = false;

    /* Nothing to update if we don't have any control file */
    if (!playlist->control_created)
        return 0;

    if (pl_get_tempname(playlist->control_filename, tmpbuf, tmpsize))
        return -1;

    /* Close the existing control file, reopen it as read-only */
    pl_close_control(playlist);
    old_fd = open(playlist->control_filename, O_RDONLY);
    if (old_fd < 0)
        return -2;

    /* Create new control file, pointing it at a tempfile */
    playlist->control_fd = open(tmpbuf, O_CREAT|O_RDWR|O_TRUNC, 0666);
    if (playlist->control_fd < 0)
    {
        err = -3;
        goto error;
    }

    /* Write out playlist filename */
    c = playlist->filename[playlist->dirlen-1];
    playlist->filename[playlist->dirlen-1] = '\0';

    err = update_control_unlocked(playlist, PLAYLIST_COMMAND_PLAYLIST,
                                  PLAYLIST_CONTROL_FILE_VERSION, -1,
                                  playlist->filename,
                                  &playlist->filename[playlist->dirlen], NULL);

    playlist->filename[playlist->dirlen-1] = c;

    if (err <= 0)
    {
        err = -4;
        goto error;
    }

    if (playlist->first_index > 0)
    {
        /* rotate indices so they'll be in sync with new control file */
        pl_reverse(playlist, 0, playlist->amount - 1);
        pl_reverse(playlist, 0, playlist->amount - playlist->first_index - 1);
        pl_reverse(playlist, playlist->amount - playlist->first_index, playlist->amount - 1);

        playlist->index = rotate_index(playlist, playlist->index);
        playlist->last_insert_pos = rotate_index(playlist, playlist->last_insert_pos);
        playlist->first_index = 0;
    }
    
    for (int index = 0; index < playlist->amount; ++index)
    {
        /* We only need to update queued files */
        if (!(playlist->indices[index] & PLAYLIST_QUEUED))
            continue;

        /* Read filename from old control file */
        lseek(old_fd, playlist->indices[index] & PLAYLIST_SEEK_MASK, SEEK_SET);
        read_line(old_fd, tmpbuf, tmpsize);

        /* Write it out to the new control file */
        int seekpos;
        err = update_control_unlocked(playlist, PLAYLIST_COMMAND_QUEUE,
                                      index, playlist->last_insert_pos,
                                      tmpbuf, NULL, &seekpos);
        if (err <= 0)
        {
            err = -5;
            goto error;
        }
        /* Update seek offset for the new control file. */
        playlist->indices[index] &= ~PLAYLIST_SEEK_MASK;
        playlist->indices[index] |= seekpos;
        any_queued = true;
    }

    /* Preserve modified state */
    if (playlist_modified(playlist))
    {
        if (any_queued)
        {
            err = update_control_unlocked(playlist, PLAYLIST_COMMAND_FLAGS,
                                          PLAYLIST_FLAG_MODIFIED, 0, NULL, NULL, NULL);
            if (err <= 0)
            {
                err = -6;
                goto error;
            }
        }
        else
        {
            playlist->flags &= ~PLAYLIST_FLAG_MODIFIED;
        }
    }

    /* Clear dirplay flag, since we now point at a playlist */
    playlist->flags &= ~PLAYLIST_FLAG_DIRPLAY;

    /* Reset shuffle seed */
    playlist->seed = 0;
    if (playlist == &current_playlist)
        global_settings.playlist_shuffle = false;

    pl_close_control(playlist);
    close(old_fd);
    remove(playlist->control_filename);

    /* TODO: Check for errors? The old control file is gone by this point... */
    pl_get_tempname(playlist->control_filename, tmpbuf, tmpsize);
    rename(tmpbuf, playlist->control_filename);

    playlist->control_fd = open(playlist->control_filename, O_RDWR);
    playlist->control_created = (playlist->control_fd >= 0);

    return 0;

error:
    close(old_fd);
    return err;
}

int playlist_save(struct playlist_info* playlist, char *filename)
{
    char save_path[MAX_PATH+1];
    char tmpbuf[MAX_PATH+1];
    ssize_t pathlen;
    int rc = 0;
    bool reload_tracks = false;

    if (!playlist)
        playlist = &current_playlist;

    pathlen = format_track_path(save_path, filename,
                                sizeof(save_path), PATH_ROOTSTR, -1u);
    if (pathlen < 0)
        return -1;

    cpu_boost(true);
    dc_thread_stop(playlist);
    playlist_write_lock(playlist);

    if (playlist->amount <= 0)
    {
        rc = -1;
        goto error;
    }

    /* Ask if queued tracks should be removed, so that
    playlist can be bookmarked after it's been saved */
    for (int i = playlist->amount - 1; i >= 0; i--)
        if (playlist->indices[i] & PLAYLIST_QUEUED)
        {
            if (reload_tracks || (reload_tracks = (confirm_remove_queued_yesno() == YESNO_YES)))
                remove_track_unlocked(playlist, i, false);
            else
                break;
        }

    rc = pl_save_playlist(playlist, save_path, tmpbuf, sizeof(tmpbuf));
    if (rc < 0)
    {
        // TODO: If we fail here, we just need to reparse the old playlist file
        panicf("Failed to save playlist: %d", rc);
        goto error;
    }

    /* User cancelled? */
    if (rc > 0)
        goto error;

    rc = pl_save_update_control(playlist, tmpbuf, sizeof(tmpbuf));
    if (rc)
    {
        // TODO: If we fail here, then there are two possibilities depending on
        // whether we overwrote the old playlist file:
        //
        // - if it still exists, we could reparse it + old control file
        // - otherwise, we need to selectively reload the old control file
        //   and somehow make use of the new playlist file
        //
        // The latter case poses other issues though, like what happens after we
        // resume, because replaying the old control file over the new playlist
        // won't work properly. We could simply choose to reset the control file,
        // seeing as by this point it only contains transient data (queued tracks).
        panicf("Failed to update control file: %d", rc);
        goto error;
    }

error:
    playlist_write_unlock(playlist);
    dc_thread_start(playlist, true);
    cpu_boost(false);
    if (reload_tracks && playlist->started &&
        (audio_status() & AUDIO_STATUS_PLAY))
        audio_flush_and_reload_tracks();
    return rc;
}
