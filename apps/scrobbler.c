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

#include "file.h"
#include "sprintf.h"
#include "playback.h"
#include "logf.h"
#include "id3.h"
#include "kernel.h"
#include "audio.h"
#include "buffer.h"
#include "settings.h"
#include "ata_idle_notify.h"
#include "misc.h"

#if CONFIG_RTC
#include "time.h"
#include "timefuncs.h"
#endif

#include "scrobbler.h"

#define SCROBBLER_VERSION "1.1"

#if CONFIG_RTC
#define SCROBBLER_FILE "/.scrobbler.log"
#else
#define SCROBBLER_FILE "/.scrobbler-timeless.log"
#endif

/* increment this on any code change that effects output */
#define SCROBBLER_REVISION " $Revision$"

#define SCROBBLER_MAX_CACHE 32
/* longest entry I've had is 323, add a safety margin */
#define SCROBBLER_CACHE_LEN 512

static char* scrobbler_cache;

static int cache_pos;
static struct mp3entry scrobbler_entry;
static bool pending = false;
static bool scrobbler_initialised = false;
#if CONFIG_RTC
static time_t timestamp;
#else
static unsigned long timestamp;
#endif

/* Crude work-around for Archos Sims - return a set amount */
#if (CONFIG_CODEC != SWCODEC) && defined(SIMULATOR)
unsigned long audio_prev_elapsed(void)
{
    return 120000;
}
#endif

static void write_cache(void)
{
    int i;
    int fd;

    /* If the file doesn't exist, create it.
    Check at each write since file may be deleted at any time */
    if(!file_exists(SCROBBLER_FILE))
    {
        fd = open(SCROBBLER_FILE, O_RDWR | O_CREAT);
        if(fd >= 0)
        {
            fdprintf(fd, "#AUDIOSCROBBLER/" SCROBBLER_VERSION "\n"
                         "#TZ/UNKNOWN\n"
#if CONFIG_RTC
                         "#CLIENT/Rockbox " TARGET_NAME SCROBBLER_REVISION "\n");
#else
                         "#CLIENT/Rockbox " TARGET_NAME SCROBBLER_REVISION " Timeless\n");
#endif

            close(fd);
        }
        else
        {
            logf("SCROBBLER: cannot create log file");
        }
    }

    /* write the cache entries */
    fd = open(SCROBBLER_FILE, O_WRONLY | O_APPEND);
    if(fd >= 0)
    {
        logf("SCROBBLER: writing %d entries", cache_pos); 

        for ( i=0; i < cache_pos; i++ )
        {
            logf("SCROBBLER: write %d", i);
            fdprintf(fd, "%s", scrobbler_cache+(SCROBBLER_CACHE_LEN*i));
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

static bool scrobbler_flush_callback(void)
{
    if (scrobbler_initialised && cache_pos)
        write_cache();
    return true;
}

static void add_to_cache(unsigned long play_length)
{
    if ( cache_pos >= SCROBBLER_MAX_CACHE )
        write_cache();

    int ret;
    char rating = 'S'; /* Skipped */

    logf("SCROBBLER: add_to_cache[%d]", cache_pos);

    if ( play_length > (scrobbler_entry.length/2) )
        rating = 'L'; /* Listened */

    if (scrobbler_entry.tracknum > 0)
    {
        ret = snprintf(scrobbler_cache+(SCROBBLER_CACHE_LEN*cache_pos),
                SCROBBLER_CACHE_LEN,
                "%s\t%s\t%s\t%d\t%d\t%c\t%ld\t%s\n",
                scrobbler_entry.artist,
                scrobbler_entry.album?scrobbler_entry.album:"",
                scrobbler_entry.title,
                scrobbler_entry.tracknum,
                (int)scrobbler_entry.length/1000,
                rating,
                (long)timestamp,
                scrobbler_entry.mb_track_id?scrobbler_entry.mb_track_id:"");
    } else {
        ret = snprintf(scrobbler_cache+(SCROBBLER_CACHE_LEN*cache_pos),
                SCROBBLER_CACHE_LEN,
                "%s\t%s\t%s\t\t%d\t%c\t%ld\t%s\n",
                scrobbler_entry.artist,
                scrobbler_entry.album?scrobbler_entry.album:"",
                scrobbler_entry.title,
                (int)scrobbler_entry.length/1000,
                rating,
                (long)timestamp,
                scrobbler_entry.mb_track_id?scrobbler_entry.mb_track_id:"");
    }

    if ( ret >= SCROBBLER_CACHE_LEN )
    {
        logf("SCROBBLER: entry too long:");
        logf("SCROBBLER: %s", scrobbler_entry.path);
    } else {
        cache_pos++;
        register_ata_idle_func(scrobbler_flush_callback);
    }

}

void scrobbler_change_event(struct mp3entry *id)
{
    /* add entry using the previous scrobbler_entry and timestamp */
    if (pending)
        add_to_cache(audio_prev_elapsed());

    /*  check if track was resumed > %50 played
        check for blank artist or track name */
    if ((id->elapsed > (id->length/2)) ||
        (!id->artist ) || (!id->title ) )
    {
        pending = false;
        logf("SCROBBLER: skipping file %s", id->path);
    }
    else
    {
        logf("SCROBBLER: add pending");
        copy_mp3entry(&scrobbler_entry, id);
#if CONFIG_RTC
        timestamp = mktime(get_time());
#else
        timestamp = 0;
#endif
        pending = true;
    }
}

int scrobbler_init(void)
{
    logf("SCROBBLER: init %d", global_settings.audioscrobbler);

    if(!global_settings.audioscrobbler)
        return -1;

    scrobbler_cache = buffer_alloc(SCROBBLER_MAX_CACHE*SCROBBLER_CACHE_LEN);

    add_event(PLAYBACK_EVENT_TRACK_CHANGE, false, scrobbler_change_event);
    cache_pos = 0;
    pending = false;
    scrobbler_initialised = true;

    return 1;
}

void scrobbler_flush_cache(void)
{
    if (scrobbler_initialised)
    {
        /* Add any pending entries to the cache */
        if(pending)
            add_to_cache(audio_prev_elapsed());

        /* Write the cache to disk if needed */
        if (cache_pos)
            write_cache();

        pending = false;
    }
}

void scrobbler_shutdown(void)
{
    scrobbler_flush_cache();

    if (scrobbler_initialised)
    {
        remove_event(PLAYBACK_EVENT_TRACK_CHANGE, scrobbler_change_event);
        scrobbler_initialised = false;
    }
}

void scrobbler_poweroff(void)
{
    if (scrobbler_initialised && pending)
    {
        if ( audio_status() )
            add_to_cache(audio_current_track()->elapsed);
        else
            add_to_cache(audio_prev_elapsed());

        /* scrobbler_shutdown is called later, the cache will be written
        *  make sure the final track isn't added twice when that happens */
        pending = false;
    }
}

bool scrobbler_is_enabled(void)
{
    return scrobbler_initialised;
}
