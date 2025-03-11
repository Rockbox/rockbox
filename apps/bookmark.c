/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C)2003 by Benjamin Metzler
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "action.h"
#include "audio.h"
#include "playlist.h"
#include "settings.h"
#include "tree.h"
#include "bookmark.h"
#include "system.h"
#include "icons.h"
#include "menu.h"
#include "lang.h"
#include "talk.h"
#include "misc.h"
#include "splash.h"
#include "yesno.h"
#include "list.h"
#include "plugin.h"
#include "file.h"
#include "pathfuncs.h"
#include "playlist_menu.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#define MAX_BOOKMARKS 10
#define MAX_BOOKMARK_SIZE  350
#define RECENT_BOOKMARK_FILE ROCKBOX_DIR "/most-recent.bmark"
#define BOOKMARK_IGNORE "/bookmark.ignore"
#define BOOKMARK_UNIGNORE "/bookmark.unignore"

/* flags for write_bookmark() */
#define BMARK_WRITE        0x0
#define BMARK_ASK_USER     0x1
#define BMARK_CREATE_FILE  0x2
#define BMARK_CHECK_IGNORE 0x4

/* Used to buffer bookmarks while displaying the bookmark list. */
struct bookmark_list
{
    const char* filename;
    size_t buffer_size;
    int start;
    int count;
    int total_count;
    bool show_dont_resume;
    bool reload;
    bool show_playlist_name;
    char* items[];
};

/* flags for optional bookmark tokens */
#define BM_PITCH    0x01
#define BM_SPEED    0x02

/* bookmark values */
struct resume_info{
    const struct mp3entry *id3;
    int  resume_index;
    unsigned long resume_offset;
    int  resume_seed;
    long resume_elapsed;
    int  repeat_mode;
    bool shuffle;
    /* optional values */
    int  pitch;
    int  speed;
};

/* Temp buffer used for reading, create_bookmark and filename creation */
#define TEMP_BUF_SIZE (MAX(MAX_BOOKMARK_SIZE, MAX_PATH + 1))
static char global_temp_buffer[TEMP_BUF_SIZE];

static inline void get_hash(const char *key, uint32_t *hash, int len)
{
    *hash = crc_32(key, len, *hash); /* this is probably sufficient */
}

static const char* skip_tokens(const char* s, int ntokens)
{
    for (int i = 0; i < ntokens; i++)
    {
        while (*s && *s != ';')
        {
            s++;
        }

        if (*s)
        {
            s++;
        }
    }
    return s;
}

static int int_token(const char **s)
{
    int ret = atoi(*s);
    *s = skip_tokens(*s, 1);
    return ret;
}

static long long_token(const char **s)
{
    /* Should be atol, but we don't have it. */
    return int_token(s);
}

/*-------------------------------------------------------------------------*/
/* Get the name of the playlist and the name of the track from a bookmark. */
/* Returns true iff both were extracted.                                   */
/*-------------------------------------------------------------------------*/
static bool bookmark_get_playlist_and_track_hash(const char *bookmark,
                                            uint32_t *pl_hash,
                                            uint32_t *track_hash)
{
    *pl_hash = 0;
    *track_hash = 0;
    int  pl_len;
    const char *pl_start, *pl_end, *track;

    logf("%s", __func__);

    pl_start = strchr(bookmark,'/');
    if (!(pl_start))
        return false;

    pl_end = skip_tokens(pl_start, 1) - 1;
    pl_len = pl_end - pl_start;

    track = pl_end + 1;
    get_hash(pl_start, pl_hash, pl_len);

    if (global_settings.usemrb == BOOKMARK_ONE_PER_TRACK)
    {
        get_hash(track, track_hash, strlen(track));
    }


    return true;
}

/* ----------------------------------------------------------------------- */
/* This function takes a bookmark and parses it.  This function also       */
/* validates the bookmark.  Valid filenamebuf indicates whether            */
/* the filename tokens are to be extracted.                                */
/* Returns true on successful bookmark parse.                              */
/* ----------------------------------------------------------------------- */
static bool parse_bookmark(char *filenamebuf,
                           size_t filenamebufsz,
                           const char *bookmark,
                           struct resume_info *resume_info,
                           const bool strip_dir)
{
    const char* s = bookmark;
    const char* end;

#define GET_INT_TOKEN(var)   var = int_token(&s)
#define GET_LONG_TOKEN(var)  var = long_token(&s)
#define GET_BOOL_TOKEN(var) var = (int_token(&s) != 0)

    /* if new format bookmark, extract the optional content flags,
       otherwise treat as an original format bookmark */
    int opt_flags = 0;
    int opt_pitch = 0;
    int opt_speed = 0;
    int old_format = ((strchr(s, '>') == s) ? 0 : 1);
    if (old_format == 0) /* this is a new format bookmark */
    {
        s++;
        GET_INT_TOKEN(opt_flags);
        opt_pitch = (opt_flags & BM_PITCH) ? 1:0;
        opt_speed = (opt_flags & BM_SPEED) ? 1:0;
    }

    /* extract all original bookmark tokens */
    if (resume_info)
    {
        GET_INT_TOKEN(resume_info->resume_index);
        GET_LONG_TOKEN(resume_info->resume_offset);
        GET_INT_TOKEN(resume_info->resume_seed);

        s = skip_tokens(s, old_format); /* skip deprecated token */

        GET_LONG_TOKEN(resume_info->resume_elapsed);
        GET_INT_TOKEN(resume_info->repeat_mode);
        GET_BOOL_TOKEN(resume_info->shuffle);

        /* extract all optional bookmark tokens */
        if (opt_pitch != 0)
            GET_INT_TOKEN(resume_info->pitch);
        if (opt_speed != 0)
            GET_INT_TOKEN(resume_info->speed);
    }
    else /* no resume info we just want the file name strings */
    {
        #define DEFAULT_BM_TOKENS 6
        int skipct = DEFAULT_BM_TOKENS + old_format + opt_pitch + opt_speed;
        s = skip_tokens(s, skipct);
        #undef DEFAULT_BM_TOKENS
    }

    if (*s == 0)
    {
        return false;
    }

    end = strchr(s, ';');

    /* extract file names */
    if(filenamebuf)
    {
        size_t len = (end == NULL) ? strlen(s) : (size_t) (end - s);
        len = MIN(TEMP_BUF_SIZE - 1, len);
        strmemccpy(global_temp_buffer, s, len + 1);

        if (end != NULL)
        {
            end++;
            if (strip_dir)
            {
                s = strrchr(end, '/');
                if (s)
                {
                    end = s;
                    end++;
                }
            }
            strmemccpy(filenamebuf, end, filenamebufsz);
        }
     }

    return true;
}

/* ------------------------------------------------------------------------- */
/* This function takes a filename and appends .tmp. This function also opens */
/* the resulting file based on oflags, filename will be in buf on return     */
/* Returns file descriptor                                                   */
/* --------------------------------------------------------------------------*/
static int open_temp_bookmark(char *buf,
                              size_t bufsz,
                              int oflags,
                              const char* filename)
{
    if(filename[0] == '/')
        filename++;
    /* Opening up a temp bookmark file */
    int fd  = open_pathfmt(buf, bufsz, oflags, "/%s.tmp", filename);
#ifdef LOGF_ENABLE
    if (oflags & O_PATH)
        logf("tempfile path %s", buf);
    else
        logf("opening tempfile %s", buf);
#endif
    return fd;
}

/* ----------------------------------------------------------------------- */
/* This function adds a bookmark to a file.                                */
/* Returns true on successful bookmark add.                                */
/* ------------------------------------------------------------------------*/
static bool add_bookmark(const char* bookmark_file_name,
                         const char* bookmark,
                         bool most_recent)
{
    char fnamebuf[MAX_PATH];
    int  temp_bookmark_file = 0;
    int  bookmark_file = 0;
    int  bookmark_count = 0;
    bool comp_playlist = false;
    bool comp_track = false;
    bool equal;
    uint32_t pl_hash, pl_track_hash;
    uint32_t bm_pl_hash, bm_pl_track_hash;

    if (!bookmark)
        return false; /* no bookmark */

    /* Opening up a temp bookmark file */
    temp_bookmark_file = open_temp_bookmark(fnamebuf,
                                            sizeof(fnamebuf),
                                            O_WRONLY | O_CREAT | O_TRUNC,
                                            bookmark_file_name);

    if (temp_bookmark_file < 0)
        return false; /* can't open the temp file */

    if (most_recent && ((global_settings.usemrb == BOOKMARK_ONE_PER_PLAYLIST)
                      || (global_settings.usemrb == BOOKMARK_ONE_PER_TRACK)))
    {

        if (bookmark_get_playlist_and_track_hash(bookmark, &pl_hash, &pl_track_hash))
        {
            comp_playlist = true;
            comp_track = (global_settings.usemrb == BOOKMARK_ONE_PER_TRACK);
        }
    }

    logf("adding bookmark to %s [%s]", fnamebuf, bookmark);
    /* Writing the new bookmark to the begining of the temp file */
    write(temp_bookmark_file, bookmark, strlen(bookmark));
    write(temp_bookmark_file, "\n", 1);
    bookmark_count++;

    /* WARNING underlying buffer to *bookmrk gets overwritten after this point! */

    /* Reading in the previous bookmarks and writing them to the temp file */
    logf("opening old bookmark %s", bookmark_file_name);
    bookmark_file = open(bookmark_file_name, O_RDONLY);
    if (bookmark_file >= 0)
    {
        while (read_line(bookmark_file, global_temp_buffer,
                         sizeof(global_temp_buffer)) > 0)
        {
            /* The MRB has a max of MAX_BOOKMARKS in it */
            /* This keeps it from getting too large */
            if (most_recent && (bookmark_count >= MAX_BOOKMARKS))
                break;

            if (!parse_bookmark(NULL, 0, global_temp_buffer, NULL, false))
                break;

            equal = false;
            if (comp_playlist)
            {
                if (bookmark_get_playlist_and_track_hash(global_temp_buffer,
                                                &bm_pl_hash, &bm_pl_track_hash))
                {
                    equal = (pl_hash == bm_pl_hash);
                    if (equal && comp_track)
                    {
                         equal = (pl_track_hash == bm_pl_track_hash);
                    }
                }
            }
            if (!equal)
            {
                bookmark_count++;
                /*logf("copying old bookmark [%s]", global_temp_buffer);*/
                write(temp_bookmark_file, global_temp_buffer,
                      strlen(global_temp_buffer));
                write(temp_bookmark_file, "\n", 1);
            }
        }
        close(bookmark_file);
    }
    close(temp_bookmark_file);

    remove(bookmark_file_name);
    rename(fnamebuf, bookmark_file_name);

    return true;
}

/* ----------------------------------------------------------------------- */
/* This function is used by multiple functions and is used to generate a   */
/* bookmark named based off of the input.                                  */
/* Changing this function could result in how the bookmarks are stored.    */
/* it would be here that the centralized/decentralized bookmark code       */
/* could be placed.                                                        */
/* Returns true if the file name is generated, false if it was too long    */
/* ----------------------------------------------------------------------- */
static bool generate_bookmark_file_name(char *filenamebuf,
                                        size_t filenamebufsz,
                                        const char *bmarknamein,
                                        size_t bmarknamelen)
{
    /* if this is a root dir MP3, rename the bookmark file root_dir.bmark */
    /* otherwise, name it based on the bmarknamein variable */
    if (!strncmp("/", bmarknamein, bmarknamelen))
        strmemccpy(filenamebuf, "/root_dir.bmark", filenamebufsz);
    else
    {
        size_t buflen, len;
        /* strmemccpy considers the NULL so bmarknamelen is one off */
        buflen = MIN(filenamebufsz -1 , bmarknamelen);
        if (buflen >= filenamebufsz)
            return false;

        strmemccpy(filenamebuf, bmarknamein, buflen + 1);

        len = strlen(filenamebuf);

#ifdef HAVE_MULTIVOLUME
        /* The "root" of an extra volume need special handling too. */
        const char *filename;
        path_strip_volume(filenamebuf, &filename, true);
        bool volume_root = *filename == '\0';
#endif
        if(filenamebuf[len-1] == '/') {
            filenamebuf[len-1] = '\0';
        }

        const char *name = ".bmark";
#ifdef HAVE_MULTIVOLUME
        if (volume_root)
            name = "/volume_dir.bmark";
#endif
        len = strlcat(filenamebuf, name, filenamebufsz);

        if(len >= filenamebufsz)
            return false;
    }
    logf ("generated name '%s' from '%.*s'",
          filenamebuf, (int)bmarknamelen, bmarknamein);
    return true;
}

/* GCC 7 and up complain about the snprintf in create_bookmark() when
   compiled with -D_FORTIFY_SOURCE or -Wformat-truncation
   This is a false positive, so disable it here only */
/* SHOULD NO LONGER BE NEEDED --Bilgus 11-2022 */
#if 0 /* __GNUC__ >= 7 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
/* ----------------------------------------------------------------------- */
/* This function takes the system resume data and formats it into a valid  */
/* bookmark.                                                               */
/* playlist name and  name len are passed back through the name/namelen    */
/* Return is not NULL on successful bookmark format.                      */
/* ----------------------------------------------------------------------- */
static char* create_bookmark(char **name,
                             size_t *namelen,
                             struct resume_info *resume_info)
{
    const char *file;
    char *buf = global_temp_buffer;
    size_t bufsz = sizeof(global_temp_buffer);

    if(!resume_info->id3)
        return NULL;

    size_t bmarksz= snprintf(buf, bufsz,
                             /* new optional bookmark token descriptors should
                                be inserted just after ';"' in this line... */
#if defined(HAVE_PITCHCONTROL)
                             ">%d;%d;%ld;%d;%ld;%d;%d;%ld;%ld;",
#else
                             ">%d;%d;%ld;%d;%ld;%d;%d;",
#endif
                             /* ... their flags should go here ... */
#if defined(HAVE_PITCHCONTROL)
                             BM_PITCH | BM_SPEED,
#else
                             0,
#endif
                             resume_info->resume_index,
                             resume_info->id3->offset,
                             resume_info->resume_seed,
                             resume_info->id3->elapsed,
                             resume_info->repeat_mode,
                             resume_info->shuffle,
                             /* ...and their values should go here */
#if defined(HAVE_PITCHCONTROL)
                             (long)resume_info->pitch,
                             (long)resume_info->speed
#endif
                    ); /*sprintf*/
/* mandatory tokens */
    if (bmarksz >= bufsz) /* include NULL*/
        return NULL;
    buf += bmarksz;
    bufsz -= bmarksz;

    /* create the bookmark */
    playlist_get_name(NULL, buf, bufsz);
    bmarksz = strlen(buf);

    if (bmarksz == 0 || (bmarksz + 1) >= bufsz) /* include the separator & NULL*/
        return NULL;

    *name = buf;     /* return the playlist name through the *pointer */
    *namelen = bmarksz; /* return the name length through the pointer */

    /* Get the currently playing file minus the path */
    /* This is used when displaying the available bookmarks */
    file = strrchr(resume_info->id3->path,'/');
    if(NULL == file)
        return NULL;

    if (buf[bmarksz - 1] != '/')
        file = resume_info->id3->path;
    else file++;

    buf += bmarksz;
    bufsz -= (bmarksz + 1);
    buf[0] = ';';
    buf[1] = '\0';

    strlcat(buf, file, bufsz);

    logf("%s [%s]", __func__, global_temp_buffer);
    /* checking to see if the bookmark is valid */
    if (parse_bookmark(NULL, 0, global_temp_buffer, NULL, false))
        return global_temp_buffer;
    else
        return NULL;
}
#if 0/* __GNUC__ >= 7*/
#pragma GCC diagnostic pop /* -Wformat-truncation */
#endif

/* ----------------------------------------------------------------------- */
/* This function gets some basic resume information for the current song   */
/* from rockbox,  */
/* ----------------------------------------------------------------------- */
static void get_track_resume_info(struct resume_info *resume_info)
{
    if (global_settings.playlist_shuffle)
        playlist_get_resume_info(&(resume_info->resume_index));
    else
        resume_info->resume_index = playlist_get_display_index() - 1;

    resume_info->resume_seed = playlist_get_seed(NULL);
    resume_info->id3 = audio_current_track();
    resume_info->repeat_mode = global_settings.repeat_mode;
    resume_info->shuffle = global_settings.playlist_shuffle;
#if defined(HAVE_PITCHCONTROL)
    resume_info->pitch = sound_get_pitch();
    resume_info->speed = dsp_get_timestretch();
#endif
}

/* ----------------------------------------------------------------------- */
/* This function checks for bookmark ignore and unignore files to allow    */
/* directories to be ignored or included in bookmarks */
/* ----------------------------------------------------------------------- */
static bool bookmark_has_ignore(struct resume_info *resume_info)
{
    if (!resume_info->id3)
        return false;

    char *buf = global_temp_buffer;
    size_t bufsz = sizeof(global_temp_buffer);

    strmemccpy(buf, resume_info->id3->path, bufsz);

    char *slash;
    while ((slash = strrchr(buf, '/')))
    {
        size_t rem = bufsz - (slash - buf);
        if (strmemccpy(slash, BOOKMARK_UNIGNORE, rem) != NULL && file_exists(buf))
        {
            /* unignore exists we want bookmarks */
            logf("unignore bookmark found %s\n", buf);
            return false;
        }
        if (strmemccpy(slash, BOOKMARK_IGNORE, rem) != NULL && file_exists(buf))
        {
            /* ignore exists we do not want bookmarks */
            logf("ignore bookmark found %s\n", buf);
            return true;
        }
        *slash = '\0';
    }
    return false;
}

/* ----------------------------------------------------------------------- */
/* This function takes the current current resume information and writes   */
/* that to the beginning of the bookmark file.                             */
/* This file will contain N number of bookmarks in the following format:   */
/* resume_index*resume_offset*resume_seed*resume_first_index*              */
/* resume_file*milliseconds*MP3 Title*                                     */
/* Returns true on successful bookmark write.                              */
/* Returns false if any part of the bookmarking process fails.  It is      */
/* possible that a bookmark is successfully added to the most recent       */
/* bookmark list but fails to be added to the bookmark file or vice versa. */
/* ------------------------------------------------------------------------*/
static bool write_bookmark(unsigned int flags)
{
    logf("%s flags: %d", __func__, flags);
    char bm_filename[MAX_PATH];
    bool ret=true;

    bool create_bookmark_file = flags & BMARK_CREATE_FILE;
    bool check_ignore = flags & BMARK_CHECK_IGNORE;
    bool ask_user = flags & BMARK_ASK_USER;
    bool usemrb = global_settings.usemrb;

    char *name = NULL;
    size_t namelen = 0;
    char* bm;
    struct resume_info resume_info;

    if (bookmark_is_bookmarkable_state())
    {
        get_track_resume_info(&resume_info);

        if (check_ignore
            && (create_bookmark_file || ask_user || usemrb))
        {
            if (bookmark_has_ignore(&resume_info))
                return false;
        }

        if (ask_user)
        {
            if (yesno_pop(ID2P(LANG_AUTO_BOOKMARK_QUERY)))
            {
                if (global_settings.autocreatebookmark != BOOKMARK_RECENT_ONLY_ASK)
                    create_bookmark_file = true;
            }
            else
                return false;
        }

        /* writing the most recent bookmark */
        if (usemrb)
        {
            /* since we use the same buffer bookmark needs created each time */
            bm = create_bookmark(&name, &namelen, &resume_info);
            ret = add_bookmark(RECENT_BOOKMARK_FILE, bm, true);
        }

        /* writing the directory bookmark */
        if (create_bookmark_file)
        {
            bm = create_bookmark(&name, &namelen, &resume_info);
            if (generate_bookmark_file_name(bm_filename,
                                            sizeof(bm_filename), name, namelen))
            {
                ret &= add_bookmark(bm_filename, bm, false);
            }
            else
            {
                ret = false; /* generating bookmark file failed */
            }
        }
    }
    else
        ret = false;

    splash(HZ, ret ? ID2P(LANG_BOOKMARK_CREATE_SUCCESS)
           : ID2P(LANG_BOOKMARK_CREATE_FAILURE));

    return ret;
}

static int get_bookmark_count(const char* bookmark_file_name)
{
    int read_count = 0;
    int file = open(bookmark_file_name, O_RDONLY);

    if(file < 0)
        return -1;

    while(read_line(file, global_temp_buffer, sizeof(global_temp_buffer)) > 0)
    {
        read_count++;
    }

    close(file);
    return read_count;
}

static int buffer_bookmarks(struct bookmark_list* bookmarks, int first_line)
{
    char* dest = ((char*) bookmarks) + bookmarks->buffer_size - 1;
    int read_count = 0;
    int file = open(bookmarks->filename, O_RDONLY);

    if (file < 0)
    {
        return -1;
    }

    if ((first_line != 0) && ((size_t) filesize(file) < bookmarks->buffer_size
            - sizeof(*bookmarks) - (sizeof(char*) * bookmarks->total_count)))
    {
        /* Entire file fits in buffer */
        first_line = 0;
    }

    bookmarks->start = first_line;
    bookmarks->count = 0;
    bookmarks->reload = false;

    while(read_line(file, global_temp_buffer, sizeof(global_temp_buffer)) > 0)
    {
        read_count++;

        if (read_count >= first_line)
        {
            dest -= strlen(global_temp_buffer) + 1;

            if (dest < ((char*) bookmarks) + sizeof(*bookmarks)
                + (sizeof(char*) * (bookmarks->count + 1)))
            {
                break;
            }

            strcpy(dest, global_temp_buffer);
            bookmarks->items[bookmarks->count] = dest;
            bookmarks->count++;
        }
    }

    close(file);
    return bookmarks->start + bookmarks->count;
}

static const char* get_bookmark_info(int list_index,
                                     void* data,
                                     char *buffer,
                                     size_t buffer_len)
{
    char fnamebuf[MAX_PATH];
    struct resume_info resume_info;
    struct bookmark_list* bookmarks = (struct bookmark_list*) data;
    int     index = list_index / 2;

    if (bookmarks->show_dont_resume)
    {
        if (index == 0)
        {
            return list_index % 2 == 0
                ? (char*) str(LANG_BOOKMARK_DONT_RESUME) : " ";
        }

        index--;
    }

    if (bookmarks->reload || (index >= bookmarks->start + bookmarks->count)
        || (index < bookmarks->start))
    {
        int read_index = index;

        /* Using count as a guide on how far to move could possibly fail
         * sometimes. Use byte count if that is a problem?
         */

        if (read_index != 0)
        {
            /* Move count * 3 / 4 items in the direction the user is moving,
             * but don't go too close to the end.
             */
            int offset = bookmarks->count;
            int max = bookmarks->total_count - (bookmarks->count / 2);

            if (read_index < bookmarks->start)
            {
                offset *= 3;
            }

            read_index = index - offset / 4;

            if (read_index > max)
            {
                read_index = max;
            }

            if (read_index < 0)
            {
                read_index = 0;
            }
        }

        if (buffer_bookmarks(bookmarks, read_index) <= index)
        {
            return "";
        }
    }

    if (!parse_bookmark(fnamebuf, sizeof(fnamebuf),
            bookmarks->items[index - bookmarks->start], &resume_info, true))
    {
        return list_index % 2 == 0 ? (char*) str(LANG_BOOKMARK_INVALID) : " ";
    }

    if (list_index % 2 == 0)
    {
        char *name;
        char *format;
        int len = strlen(global_temp_buffer);

        if (bookmarks->show_playlist_name && len > 0)
        {
            name = global_temp_buffer;
            len--;

            if (name[len] != '/')
            {
                strrsplt(name, '.');
            }
            else if (len > 1)
            {
                name[len] = '\0';
            }

            if (len > 1)
            {
                name = strrsplt(name, '/');
            }

            format = "%s : %s";
        }
        else
        {
            name = fnamebuf;
            format = "%s";
        }

        strrsplt(fnamebuf, '.');
        snprintf(buffer, buffer_len, format, name, fnamebuf);
        return buffer;
    }
    else
    {
        char time_buf[32];

        format_time(time_buf, sizeof(time_buf), resume_info.resume_elapsed);
        snprintf(buffer, buffer_len, "%s, %d%s", time_buf,
                 resume_info.resume_index + 1,
                 resume_info.shuffle ? (char*) str(LANG_BOOKMARK_SHUFFLE) : "");
        return buffer;
    }
}

/* ----------------------------------------------------------------------- */
/* This function parses a bookmark, says the voice UI part of it.          */
/* ------------------------------------------------------------------------*/
static void say_bookmark(const char* bookmark,
                         int bookmark_id,
                         bool show_playlist_name)
{
    char fnamebuf[MAX_PATH];
    struct resume_info resume_info;
    if (!parse_bookmark(fnamebuf, sizeof(fnamebuf), bookmark, &resume_info, false))
    {
        talk_id(LANG_BOOKMARK_INVALID, false);
        return;
    }

    talk_number(bookmark_id + 1, false);

    bool is_dir = (global_temp_buffer[0]
              && global_temp_buffer[strlen(global_temp_buffer)-1] == '/');

    /* HWCODEC cannot enqueue voice file entries and .talk thumbnails
       together, because there is no guarantee that the same mp3
       parameters are used. */
    if(show_playlist_name)
    {   /* It's useful to know which playlist this is */
        if(is_dir)
            talk_dir_or_spell(global_temp_buffer,
                              TALK_IDARRAY(VOICE_DIR), true);
        else talk_file_or_spell(NULL, global_temp_buffer,
                                TALK_IDARRAY(LANG_PLAYLIST), true);
    }

    if(resume_info.shuffle)
        talk_id(LANG_SHUFFLE, true);

    talk_id(VOICE_BOOKMARK_SELECT_INDEX_TEXT, true);
    talk_number(resume_info.resume_index + 1, true);
    talk_id(LANG_TIME, true);
    talk_value(resume_info.resume_elapsed / 1000, UNIT_TIME, true);

    /* Track filename */
    if(!is_dir)
        global_temp_buffer[0] = 0;
    talk_file_or_spell(global_temp_buffer, fnamebuf,
                       TALK_IDARRAY(VOICE_FILE), true);
}

static int bookmark_list_voice_cb(int list_index, void* data)
{
    struct bookmark_list* bookmarks = (struct bookmark_list*) data;
    int index = list_index / 2;

    if (bookmarks->show_dont_resume)
    {
        if (index == 0)
            return talk_id(LANG_BOOKMARK_DONT_RESUME, false);
        index--;
    }
    say_bookmark(bookmarks->items[index - bookmarks->start], index,
                 bookmarks->show_playlist_name);
    return 0;
}

/* ----------------------------------------------------------------------- */
/* This function takes a location in a bookmark file and deletes that      */
/* bookmark.                                                               */
/* Returns true on successful bookmark deletion.                           */
/* ------------------------------------------------------------------------*/
static bool delete_bookmark(const char* bookmark_file_name, int bookmark_id)
{
    int temp_bookmark_file = 0;
    int bookmark_file = 0;
    int bookmark_count = 0;

    /* Opening up a temp bookmark file */
    temp_bookmark_file = open_temp_bookmark(global_temp_buffer,
                                            sizeof(global_temp_buffer),
                                            O_WRONLY | O_CREAT | O_TRUNC,
                                            bookmark_file_name);

    if (temp_bookmark_file < 0)
        return false; /* can't open the temp file */

    /* Reading in the previous bookmarks and writing them to the temp file */
    bookmark_file = open(bookmark_file_name, O_RDONLY);
    if (bookmark_file >= 0)
    {
        while (read_line(bookmark_file, global_temp_buffer,
                         sizeof(global_temp_buffer)) > 0)
        {
            if (bookmark_id != bookmark_count)
            {
                write(temp_bookmark_file, global_temp_buffer,
                      strlen(global_temp_buffer));
                write(temp_bookmark_file, "\n", 1);
            }
            bookmark_count++;
        }
        close(bookmark_file);
    }
    close(temp_bookmark_file);

    /* only retrieve the path*/
    close(open_temp_bookmark(global_temp_buffer,
                       sizeof(global_temp_buffer),
                       O_PATH,
                       bookmark_file_name));

    remove(bookmark_file_name);
    rename(global_temp_buffer, bookmark_file_name);

    return true;
}

/* ----------------------------------------------------------------------- */
/* This displays the bookmarks in a file and allows the user to            */
/* select one to play.                                                     */
/* *selected_bookmark contains a non NULL value on successful bookmark     */
/* selection.                                                              */
/* Returns BOOKMARK_SUCCESS on successful bookmark selection, BOOKMARK_FAIL*/
/* if no selection was made and BOOKMARK_USB_CONNECTED if the selection    */
/* menu is forced to exit due to a USB connection.                         */
/* ------------------------------------------------------------------------*/
static int select_bookmark(const char* bookmark_file_name,
                           bool show_dont_resume,
                           char** selected_bookmark)
{
    struct bookmark_list* bookmarks;
    struct gui_synclist list;
    int item = 0;
    int action;
    size_t size;
    bool exit = false;
    bool refresh = true;
    int ret = BOOKMARK_FAIL;

    bookmarks = plugin_get_buffer(&size);
    bookmarks->buffer_size = size;
    bookmarks->show_dont_resume = show_dont_resume;
    bookmarks->filename = bookmark_file_name;
    bookmarks->start = 0;
    bookmarks->show_playlist_name
        = (strcmp(bookmark_file_name, RECENT_BOOKMARK_FILE) == 0);

    gui_synclist_init(&list, &get_bookmark_info,
                      (void*) bookmarks, false, 2, NULL);

    if(global_settings.talk_menu)
        gui_synclist_set_voice_callback(&list, bookmark_list_voice_cb);

    while (!exit)
    {

        if (refresh)
        {
            int count = get_bookmark_count(bookmark_file_name);
            bookmarks->total_count = count;

            if (bookmarks->total_count < 1)
            {
                /* No more bookmarks, delete file and exit */
                splash(HZ, ID2P(LANG_BOOKMARK_LOAD_EMPTY));
                remove(bookmark_file_name);
                *selected_bookmark = NULL;
                return BOOKMARK_FAIL;
            }

            if (bookmarks->show_dont_resume)
            {
                count++;
                item++;
            }

            gui_synclist_set_nb_items(&list, count * 2);

            if (item >= count)
            {
                /* Selected item has been deleted */
                item = count - 1;
                gui_synclist_select_item(&list, item * 2);
            }

            buffer_bookmarks(bookmarks, bookmarks->start);
            gui_synclist_set_title(&list, str(LANG_BOOKMARK_SELECT_BOOKMARK),
                                   Icon_Bookmark);
            gui_synclist_draw(&list);
            cond_talk_ids_fq(VOICE_EXT_BMARK);
            gui_synclist_speak_item(&list);
            refresh = false;
        }

        list_do_action(CONTEXT_BOOKMARKSCREEN, HZ / 2, &list, &action);
        item = gui_synclist_get_sel_pos(&list) / 2;

        if (bookmarks->show_dont_resume)
        {
            item--;
        }

        if (action == ACTION_STD_CONTEXT)
        {
            MENUITEM_STRINGLIST(menu_items, ID2P(LANG_BOOKMARK_CONTEXT_MENU),
                NULL, ID2P(LANG_BOOKMARK_CONTEXT_RESUME),
                ID2P(LANG_DELETE));
            static const int menu_actions[] =
            {
                ACTION_STD_OK, ACTION_BMS_DELETE
            };
            int selection = do_menu(&menu_items, NULL, NULL, false);

            refresh = true;

            if (selection >= 0 && selection <=
                (int) (sizeof(menu_actions) / sizeof(menu_actions[0])))
            {
                action = menu_actions[selection];
            }
        }

        switch (action)
        {
        case ACTION_STD_OK:
            if (item >= 0)
            {
                talk_shutup();
                *selected_bookmark = bookmarks->items[item - bookmarks->start];
                return BOOKMARK_SUCCESS;
            }
            exit = true;
            ret = BOOKMARK_SUCCESS;
            break;

        case ACTION_TREE_WPS:
        case ACTION_STD_CANCEL:
            exit = true;
            break;

        case ACTION_BMS_DELETE:
            if (item >= 0)
            {
                if (confirm_delete_yesno("") == YESNO_YES)
                {
                    delete_bookmark(bookmark_file_name, item);
                    bookmarks->reload = true;
                }
                refresh = true;
            }
            break;

        default:
            if (default_event_handler(action) == SYS_USB_CONNECTED)
            {
                ret = BOOKMARK_USB_CONNECTED;
                exit = true;
            }

            break;
        }
    }

    talk_shutup();
    *selected_bookmark = NULL;
    return ret;
}

/* ----------------------------------------------------------------------- */
/* This function parses a bookmark and then plays it.                      */
/* Returns true on successful bookmark play.                               */
/* ------------------------------------------------------------------------*/
static bool play_bookmark(const char* bookmark)
{
    char fnamebuf[MAX_PATH];
    struct resume_info resume_info;
#if defined(HAVE_PITCHCONTROL)
    /* preset pitch and speed to 100% in case bookmark doesn't have info */
    resume_info.pitch = sound_get_pitch();
    resume_info.speed = dsp_get_timestretch();
#endif

    if (parse_bookmark(fnamebuf, sizeof(fnamebuf), bookmark, &resume_info, true))
    {
        global_settings.repeat_mode = resume_info.repeat_mode;
        global_settings.playlist_shuffle = resume_info.shuffle;
#if defined(HAVE_PITCHCONTROL)
        sound_set_pitch(resume_info.pitch);
        dsp_set_timestretch(resume_info.speed);
#endif
        if (!warn_on_pl_erase())
            return false;
        bool success = bookmark_play(global_temp_buffer, resume_info.resume_index,
                             resume_info.resume_elapsed, resume_info.resume_offset,
                             resume_info.resume_seed, fnamebuf);
        if (success) /* verify we loaded the correct track */
        {
            const struct mp3entry *id3 = audio_current_track();
            if (id3)
            {
                const char *path;
                const char *track;
                path_basename(id3->path, &path);
                path_basename(fnamebuf, &track);
                if (strcmp(path, track) == 0)
                {
                    return true;
                }
            }
            audio_stop();
        }
    }

    return false;
}

/*-------------------------------------------------------------------------*/
/* PUBLIC INTERFACE -------------------------------------------------------*/
/*-------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------- */
/* This is an interface function from the context menu.                    */
/* Returns true on successful bookmark creation.                           */
/* ----------------------------------------------------------------------- */
bool bookmark_create_menu(void)
{
    if (!bookmark_is_bookmarkable_state())
        save_playlist_screen(NULL);

    return write_bookmark(BMARK_CREATE_FILE);
}
/* ----------------------------------------------------------------------- */
/* This function acts as the load interface from the context menu.         */
/* This function determines the bookmark file name and then loads that file*/
/* for the user.  The user can then select or delete previous bookmarks.   */
/* This function returns BOOKMARK_SUCCESS on the selection of a track to   */
/* resume, BOOKMARK_FAIL if the menu is exited without a selection and     */
/* BOOKMARK_USB_CONNECTED if the menu is forced to exit due to a USB       */
/* connection.                                                             */
/* ----------------------------------------------------------------------- */
int bookmark_load_menu(void)
{
    char bm_filename[MAX_PATH];
    char* bookmark;
    int ret = BOOKMARK_FAIL;

    /* To prevent root dir ("/") bookmarks  from being displayed
       when 'List Bookmarks' hotkey or button is pressed, check:
    */
    if (playlist_dynamic_only())
    {
        splash(HZ, ID2P(LANG_BOOKMARK_LOAD_EMPTY));
        return ret;
    }

    push_current_activity(ACTIVITY_BOOKMARKSLIST);

    char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
    if (generate_bookmark_file_name(bm_filename, sizeof(bm_filename), name, -1))
    {
        ret = select_bookmark(bm_filename, false, &bookmark);
        if (bookmark != NULL)
        {
            ret = play_bookmark(bookmark) ? BOOKMARK_SUCCESS : BOOKMARK_FAIL;
        }
    }

    pop_current_activity();
    return ret;
}

/* ----------------------------------------------------------------------- */
/* Gives the user a list of the Most Recent Bookmarks.  This is an         */
/* interface function                                                      */
/* Returns true on the successful selection of a recent bookmark.          */
/* ----------------------------------------------------------------------- */
bool bookmark_mrb_load()
{
    char* bookmark;
    bool ret = false;

    push_current_activity(ACTIVITY_BOOKMARKSLIST);
    select_bookmark(RECENT_BOOKMARK_FILE, false, &bookmark);
    if (bookmark != NULL)
    {
        ret = play_bookmark(bookmark);
    }

    pop_current_activity();
    return ret;
}

/* ----------------------------------------------------------------------- */
/* This function handles an autobookmark creation.  This is an interface   */
/* function.                                                               */
/* Returns true on successful bookmark creation.                           */
/* ----------------------------------------------------------------------- */
bool bookmark_autobookmark(bool prompt_ok)
{
    logf("%s", __func__);
    bool update;

    if (!bookmark_is_bookmarkable_state())
        return false;

    audio_pause();    /* first pause playback */
    update = (global_settings.autoupdatebookmark && bookmark_exists());

    if (update)
        return write_bookmark(BMARK_CREATE_FILE | BMARK_CHECK_IGNORE);

    switch (global_settings.autocreatebookmark)
    {
        case BOOKMARK_YES:
            return write_bookmark(BMARK_CREATE_FILE | BMARK_CHECK_IGNORE);

        case BOOKMARK_NO:
            return false;

        case BOOKMARK_RECENT_ONLY_YES:
            return write_bookmark(BMARK_CHECK_IGNORE);
    }

    if(prompt_ok)
        return write_bookmark(BMARK_ASK_USER | BMARK_CHECK_IGNORE);

    return false;
}

/* ----------------------------------------------------------------------- */
/* This function will determine if an autoload is necessary.  This is an   */
/* interface function.                                                     */
/* Returns                                                                 */
/* BOOKMARK_DO_RESUME    on bookmark load or bookmark selection.           */
/* BOOKMARK_DONT_RESUME  if we're not going to resume                      */
/* BOOKMARK_CANCEL       if user canceled                                  */
/* ------------------------------------------------------------------------*/
int bookmark_autoload(const char* file)
{
    logf("%s", __func__);
    char bm_filename[MAX_PATH];
    char* bookmark;

    if(global_settings.autoloadbookmark == BOOKMARK_NO)
        return BOOKMARK_DONT_RESUME;

    /*Checking to see if a bookmark file exists.*/
    if(!generate_bookmark_file_name(bm_filename, sizeof(bm_filename), file, -1))
    {
        return BOOKMARK_DONT_RESUME;
    }

    if(!file_exists(bm_filename))
        return BOOKMARK_DONT_RESUME;

    if(global_settings.autoloadbookmark == BOOKMARK_YES)
    {
        return (bookmark_load(bm_filename, true)
                ? BOOKMARK_DO_RESUME : BOOKMARK_DONT_RESUME);
    }
    else
    {
        int ret = select_bookmark(bm_filename, true, &bookmark);

        if (bookmark != NULL)
        {
            if (!play_bookmark(bookmark))
                return BOOKMARK_CANCEL;
            return BOOKMARK_DO_RESUME;
        }

        return (ret != BOOKMARK_SUCCESS) ? BOOKMARK_CANCEL : BOOKMARK_DONT_RESUME;
    }
}

/* ----------------------------------------------------------------------- */
/* This function loads the bookmark information into the resume memory.    */
/* This is an interface function.                                          */
/* Returns true on successful bookmark load.                               */
/* ------------------------------------------------------------------------*/
bool bookmark_load(const char* file, bool autoload)
{
    logf("%s", __func__);
    int  fd;
    char* bookmark = NULL;

    if(autoload)
    {
        fd = open(file, O_RDONLY);
        if(fd >= 0)
        {
            if(read_line(fd, global_temp_buffer, sizeof(global_temp_buffer)) > 0)
                bookmark=global_temp_buffer;
            close(fd);
        }
    }
    else
    {
        /* This is not an auto-load, so list the bookmarks */
        select_bookmark(file, false, &bookmark);
    }

    if (bookmark != NULL)
    {
        if (!play_bookmark(bookmark))
        {
            /* Selected bookmark not found. */
            if (!autoload)
            {
                splash(HZ*2, ID2P(LANG_NOTHING_TO_RESUME));
            }

            return false;
        }
    }

    return true;
}

/* ----------------------------------------------------------------------- */
/* Returns true if a bookmark file exists for the current playlist.        */
/* This is an interface function.                                          */
/* ----------------------------------------------------------------------- */
bool bookmark_exists(void)
{
    char bm_filename[MAX_PATH];
    bool exist=false;

    char* name = playlist_get_name(NULL, global_temp_buffer,
                                   sizeof(global_temp_buffer));
    if (!playlist_dynamic_only() &&
        generate_bookmark_file_name(bm_filename, sizeof(bm_filename), name, -1))
    {
        exist = file_exists(bm_filename);
    }
    return exist;
}

/* ----------------------------------------------------------------------- */
/* Checks the current state of the system and returns true if it is in a   */
/* bookmarkable state.                                                     */
/* This is an interface funtion.                                           */
/* ----------------------------------------------------------------------- */
bool bookmark_is_bookmarkable_state(void)
{
    int resume_index = 0;

    if (!(audio_status() && audio_current_track()) ||
        /* no track playing */
       (playlist_get_resume_info(&resume_index) == -1) ||
        /* invalid queue info */
       (playlist_modified(NULL)) ||
        /* can't bookmark playlists modified by user */
       (playlist_dynamic_only()))
        /* can't bookmark playlists without associated folder or playlist file */
    {
        return false;
    }

    return true;
}
