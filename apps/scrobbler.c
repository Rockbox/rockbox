/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2008 Robert Keevil
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
Audioscrobbler spec at:
http://www.audioscrobbler.net/wiki/Portable_Player_Logging
*/

#include <stdio.h>
#include <config.h>
#include "file.h"
#include "logf.h"
#include "metadata.h"
#include "kernel.h"
#include "audio.h"
#include "core_alloc.h"
#include "rbpaths.h"
#include "ata_idle_notify.h"
#include "pathfuncs.h"
#include "appevents.h"
#include "string-extra.h"
#if CONFIG_RTC
#include "time.h"
#include "timefuncs.h"
#endif

#include "scrobbler.h"

#define SCROBBLER_VERSION "1.1"

/* increment this on any code change that effects output */
#define SCROBBLER_REVISION " $Revision$"

#define SCROBBLER_MAX_CACHE 32
/* longest entry I've had is 323, add a safety margin */
#define SCROBBLER_CACHE_LEN 512

static bool scrobbler_initialised = false;
static int scrobbler_cache = 0;
static int cache_pos = 0;
static bool pending = false;
#if CONFIG_RTC
static time_t timestamp;
#define BASE_FILENAME       HOME_DIR "/.scrobbler.log"
#define HDR_STR_TIMELESS
#define get_timestamp()     ((long)timestamp)
#define record_timestamp()  ((void)(timestamp = mktime(get_time())))
#else /* !CONFIG_RTC */
#define HDR_STR_TIMELESS    " Timeless"
#define BASE_FILENAME       ".scrobbler-timeless.log"
#define get_timestamp()     (0l)
#define record_timestamp()  ({})
#endif /* CONFIG_RTC */

static void get_scrobbler_filename(char *path, size_t size)
{
    int used;

    used = snprintf(path, size, "/%s", BASE_FILENAME);

    if (used >= (int)size)
    {
        logf("SCROBBLER: not enough buffer space for log file");
        memset(path, 0, size);
    }
}

static void write_cache(void)
{
    int i;
    int fd;

    char scrobbler_file[MAX_PATH];
    get_scrobbler_filename(scrobbler_file, MAX_PATH);

    /* If the file doesn't exist, create it.
    Check at each write since file may be deleted at any time */
    if(!file_exists(scrobbler_file))
    {
        fd = open(scrobbler_file, O_RDWR | O_CREAT, 0666);
        if(fd >= 0)
        {
            fdprintf(fd, "#AUDIOSCROBBLER/" SCROBBLER_VERSION "\n"
                         "#TZ/UNKNOWN\n" "#CLIENT/Rockbox "
                         TARGET_NAME SCROBBLER_REVISION
                         HDR_STR_TIMELESS "\n");

            close(fd);
        }
        else
        {
            logf("SCROBBLER: cannot create log file (%s)", scrobbler_file);
        }
    }

    /* write the cache entries */
    fd = open(scrobbler_file, O_WRONLY | O_APPEND);
    if(fd >= 0)
    {
        logf("SCROBBLER: writing %d entries", cache_pos);
        /* copy data to temporary storage in case data moves during I/O */
        char temp_buf[SCROBBLER_CACHE_LEN];
        for ( i=0; i < cache_pos; i++ )
        {
            logf("SCROBBLER: write %d", i);
            char* scrobbler_buf = core_get_data(scrobbler_cache);
            ssize_t len = strlcpy(temp_buf, scrobbler_buf+(SCROBBLER_CACHE_LEN*i),
                                        sizeof(temp_buf));
            if (write(fd, temp_buf, len) != len)
                break;
        }
        close(fd);
    }
    else
    {
        logf("SCROBBLER: error writing file");
    }

    /* clear even if unsuccessful - don't want to overflow the buffer */
    cache_pos = 0;
}

static void scrobbler_flush_callback(void)
{
    if (scrobbler_initialised && cache_pos)
        write_cache();
}

static void add_to_cache(const struct mp3entry *id)
{
    if ( cache_pos >= SCROBBLER_MAX_CACHE )
        write_cache();

    char rating = 'S'; /* Skipped */
    char* scrobbler_buf = core_get_data(scrobbler_cache);

    logf("SCROBBLER: add_to_cache[%d]", cache_pos);

    if (id->elapsed > id->length / 2)
        rating = 'L'; /* Listened */

    char tracknum[11] = { "" };

    if (id->tracknum > 0)
        snprintf(tracknum, sizeof (tracknum), "%d", id->tracknum);

    int ret = snprintf(scrobbler_buf+(SCROBBLER_CACHE_LEN*cache_pos),
                       SCROBBLER_CACHE_LEN,
                       "%s\t%s\t%s\t%s\t%d\t%c\t%ld\t%s\n",
                       id->artist,
                       id->album ?: "",
                       id->title,
                       tracknum,
                       (int)(id->length / 1000),
                       rating,
                       get_timestamp(),
                       id->mb_track_id ?: "");

    if ( ret >= SCROBBLER_CACHE_LEN )
    {
        logf("SCROBBLER: entry too long:");
        logf("SCROBBLER: %s", id->path);
    }
    else
    {
        cache_pos++;
        register_storage_idle_func(scrobbler_flush_callback);
    }

}

static void scrobbler_change_event(unsigned short id, void *ev_data)
{
    (void)id;
    struct mp3entry *id3 = ((struct track_event *)ev_data)->id3;

    /*  check if track was resumed > %50 played
        check for blank artist or track name */
    if (id3->elapsed > id3->length / 2 || !id3->artist || !id3->title)
    {
        pending = false;
        logf("SCROBBLER: skipping file %s", id3->path);
    }
    else
    {
        logf("SCROBBLER: add pending");
        record_timestamp();
        pending = true;
    }
}

static void scrobbler_finish_event(unsigned short id, void *data)
{
    (void)id;
    struct track_event *te = (struct track_event *)data;

    /* add entry using the currently ending track */
    if (pending && (te->flags & TEF_CURRENT)
        && !(te->flags & TEF_REWIND)
    )
    {
        pending = false;
        add_to_cache(te->id3);
    }
}

int scrobbler_init(void)
{
    if (scrobbler_initialised)
        return 1;

    scrobbler_cache = core_alloc("scrobbler",
        SCROBBLER_MAX_CACHE*SCROBBLER_CACHE_LEN);

    if (scrobbler_cache <= 0)
    {
        logf("SCROOBLER: OOM");
        return -1;
    }

    cache_pos = 0;
    pending = false;

    scrobbler_initialised = true;

    add_event(PLAYBACK_EVENT_TRACK_CHANGE, scrobbler_change_event);
    add_event(PLAYBACK_EVENT_TRACK_FINISH, scrobbler_finish_event);

    return 1;
}

static void scrobbler_flush_cache(void)
{
        /* Add any pending entries to the cache */
    if (pending)
    {
        pending = false;
        if (audio_status())
            add_to_cache(audio_current_track());
    }

    /* Write the cache to disk if needed */
    if (cache_pos)
        write_cache();
}

void scrobbler_shutdown(bool poweroff)
{
    if (!scrobbler_initialised)
        return;

    remove_event(PLAYBACK_EVENT_TRACK_CHANGE, scrobbler_change_event);
    remove_event(PLAYBACK_EVENT_TRACK_FINISH, scrobbler_finish_event);

    scrobbler_flush_cache();

    if (!poweroff)
    {
        /* get rid of the buffer */
        core_free(scrobbler_cache);
        scrobbler_cache = 0;
    }

    scrobbler_initialised = false;
}

bool scrobbler_is_enabled(void)
{
    return scrobbler_initialised;
}
