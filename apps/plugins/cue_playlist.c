/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2024 William Wilgus
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

/* convert supplied playlist file to a .cue file */

#include "plugin.h"

#if defined(DEBUG) || defined(SIMULATOR)
    #define logf(...) rb->debugf(__VA_ARGS__); rb->debugf("\n")
#elif defined(ROCKBOX_HAS_LOGF)
    #define logf rb->logf
#else
    #define logf(...) do { } while(0)
#endif

#define CPS_MAX_ENTRY_SZ (4 *1024)
#define TDINDENT "   " /* prepend spaces for track data formatting */

static struct cps
{
    char *buffer;
    size_t buffer_sz;
    size_t buffer_index;
    int cue_fd;
    int entries;
} cps;

static int sprfunc(void *ptr, int letter)
{
    /* callback for vuprintf */
    (void) ptr;
    if (cps.buffer_index < cps.buffer_sz - 1)
    {
        cps.buffer[cps.buffer_index++] = letter;
        return 1;
    }
    return -1;
}

void cps_printf(const char *fmt, ...)
{
    /* NOTE! this is made for flushing the buffer to disk -- WARNING
     * Nothing is NULL terminated here unless explicitly made so.. \0 */
    va_list ap;
    va_start(ap, fmt);
    rb->vuprintf(sprfunc, NULL, fmt, ap);
    va_end(ap);
}

static uint32_t write_metadata_tags(struct mp3entry *id3)
{
/* check an ID3 and write any numeric tags and valid string tags (non empty) */
#define ISVALID(s) (s != NULL && s[0] != '\0')
    uint32_t tag_flags = 0;

    const char *performer = rb->str(LANG_TAGNAVI_UNTAGGED);
    if (ISVALID(id3->artist))
        performer = id3->artist;
    else if (ISVALID(id3->albumartist))
        performer = id3->albumartist;

    const char *title = rb->str(LANG_TAGNAVI_UNTAGGED);
    if (ISVALID(id3->title))
        title = id3->title;

#define PERFORMER_TITLE_SZ TDINDENT "PERFORMER \"%s\"\n" \
                           TDINDENT "TITLE \"%s\"\n" \
                           TDINDENT "SIZE_INFO %ld\n"

    cps_printf(PERFORMER_TITLE_SZ, performer, title, id3->filesize);
    if (ISVALID(id3->composer))
        cps_printf(TDINDENT "COMPOSER \"%s\"\n", id3->composer);

    cps_printf(TDINDENT "INDEX 01 00:00:00\n");

#define ID3_TAG_NUM(theid3, TAGID, flag) \
    {cps_printf(TDINDENT "REM %s %lu\n", TAGID, (unsigned long)theid3);tag_flags |= flag;}
#define ID3_TAG_STR(theid3, TAGID, flag) if (ISVALID(theid3)) \
    {cps_printf(TDINDENT "REM %s \"%s\"\n", TAGID, theid3);tag_flags |= flag;}

    ID3_TAG_STR(id3->album, "ALBUM", 0x01);
    ID3_TAG_STR(id3->albumartist, "ALBUMARTIST", 0x02);
    ID3_TAG_STR(id3->comment, "COMMENT", 0x04);
    ID3_TAG_STR(id3->genre_string, "GENRE", 0x08);
    ID3_TAG_STR(id3->disc_string, "DISC", 0x10);
    ID3_TAG_STR(id3->track_string, "TRACK", 0x20);
    ID3_TAG_STR(id3->grouping, "GROUPING", 0x40);
    ID3_TAG_STR(id3->mb_track_id, "MB_TRACK_ID", 0x80);
    ID3_TAG_STR(rb->get_codec_string(id3->codectype), "ID3_CODEC", 0x100);

    ID3_TAG_NUM(id3->discnum, "DISCNUM", 0x200);
    ID3_TAG_NUM(id3->tracknum, "TRACKNUM", 0x400);
    ID3_TAG_NUM(id3->length, "LENGTH", 0x800);
    ID3_TAG_NUM(id3->bitrate, "BITRATE", 0x1000);
    ID3_TAG_NUM(id3->frequency, "FREQUENCY", 0x2000);
    ID3_TAG_NUM(id3->track_level, "TRACK_LEVEL",0x4000);
    ID3_TAG_NUM(id3->album_level, "ALBUM_LEVEL", 0x8000);
#undef ID3_TAG_STR
#undef ID3_TAG_NUM
#undef IS_VALID
    return tag_flags;
}

static bool current_playlist_filename_cb(const char *filename, int attr, int index, int display_index)
{
    /* worker function for writing the actual cue data */
    int szpos = 0; /* records position of the size string */
    int namepos = 0; /* records position of the end of filename string */
    struct mp3entry id3;

    logf("found: %s", filename);

    uint32_t id3_flags = 0;
    bool have_metadata = rb->get_metadata(&id3, -1, filename);

    if (!have_metadata && !rb->file_exists(filename))
        return false;
#define RB_ENTRY_DATA_FMT "REM RB_ENTRY_DATA " \
                          "\"DISPLAY_INDEX %012u " \
                          "PLAYLIST_INDEX %012u " \
                          "SIZE %n%012zu TAGS %012lu\"\n"

    const char *audiotype = "WAVE"; /* everything except MP3 */
    const char *skipped = "";;
    const char *queued = "";

    size_t entry_start = cps.buffer_index; /* get start to calculate final size */

    cps_printf(RB_ENTRY_DATA_FMT, display_index, index, &szpos, 0, 0UL);

    size_t file_start = cps.buffer_index;
    cps_printf("FILE \"%s%n\"", filename, &namepos);

    if (cps.buffer[file_start + namepos - 1] == '3')
        audiotype = "MP3";

    cps_printf(" %s\n", audiotype);

    if (attr & PLAYLIST_ATTR_SKIPPED)
        skipped = TDINDENT "REM SKIPPED\n";
    if (attr & PLAYLIST_ATTR_QUEUED)
        queued = TDINDENT "REM QUEUED\n";

    cps_printf(" TRACK %d AUDIO\n%s%s", display_index, skipped, queued);

    if (have_metadata)
        id3_flags = write_metadata_tags(&id3);

    if (cps.buffer_index - entry_start < CPS_MAX_ENTRY_SZ)
    {
        /* place the write pointer at the size entry so we can update size + tags*/
        size_t index = cps.buffer_index;
        cps.buffer_index = entry_start + szpos;
        cps_printf("%012zu TAGS %012lu", (index - entry_start), id3_flags);
        cps.buffer_index = index; /* set the write pointer back at the end */
    }
    else
    {
        rb->splashf(HZ * 3, "Entry too large %s", filename);
        cps.buffer_index = entry_start;
        return false;
    }

    cps.entries++;
    return true;
}

static bool playlist_filename_cb(const char *filename)
{
    /* get entries from an on-disk playlist */
    return current_playlist_filename_cb(filename, 0,
                                        cps.entries, cps.entries);
}

static bool current_playlist_get_entries(void)
{
    /* get entries from a loaded playlist ( may have queued or skipped tracks ) */
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
#define cpuboost(enable) rb->cpu_boost(enable);
#else
#define cpuboost(enable) do{ } while(0)
#endif
    struct playlist_track_info info;
    int count = rb->playlist_amount();
    int i, res = 0;
    logf("current playlist contains %d entries", count);

    cpuboost(true);

    long next_progress_tick = *rb->current_tick;
    for (i = 0; i < count; i++)
    {
        res = rb->playlist_get_track_info(NULL, i, &info);
        int attr = info.attr;
        int index = info.index;
        int display_index = info.display_index;
        if (res < 0 || !current_playlist_filename_cb(info.filename, attr, index, display_index))
            break;

        if (cps.buffer_index >= (cps.buffer_sz - CPS_MAX_ENTRY_SZ))
        {
            logf("Buffer full, writing to disk");
            rb->write(cps.cue_fd, cps.buffer, cps.buffer_index);
            cps.buffer_index = 0;
        }

        if (TIME_AFTER(*rb->current_tick, next_progress_tick))
        {
            rb->splash_progress(i, count, "Processing current playlist %d", i);
            int action = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
            if (action == ACTION_STD_CANCEL)
            {
                res = -10;
                break;
            }
            if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
            {
                cpuboost(false);
                return PLUGIN_USB_CONNECTED;
            }
            next_progress_tick = *rb->current_tick + HZ / 2;
        }
        rb->yield();
    }

    cpuboost(false);

    return res >= 0;
#undef cpuboost
}

static void init_new_cue(const char *playlist_filename)
{
    if (cps.cue_fd >= 0)
    {
        rb->lseek(cps.cue_fd, 0, SEEK_SET);
        rb->fdprintf(cps.cue_fd, "REM COMMENT \"generated by Rockbox version: " \
                                 "%s\"\n", rb->rbversion);

        rb->fdprintf(cps.cue_fd, "TITLE \"%s\"\n", playlist_filename); /* top level TITLE */
    }
}

static void finalize_new_cue(void)
{
    rb->write(cps.cue_fd, "\n", 1);
    rb->close(cps.cue_fd);
}

static int create_new_cue(const char *filename)
{
    char buf[MAX_PATH];
    if (!filename)
        filename = "/Playlists/current.cue";
    const char *dot = rb->strrchr(filename, '.');
    int dotpos = 0;
    if (dot)
        dotpos = dot - filename;
    rb->snprintf(buf, sizeof(buf), "%.*s.cue", dotpos, filename);
    cps.cue_fd = rb->open(buf, O_WRONLY|O_CREAT|O_TRUNC, 0666);

    init_new_cue(filename);

    return cps.cue_fd;
}

enum plugin_status plugin_start(const void* parameter)
{

    bool res;
    rb->splash(HZ*2, ID2P(LANG_WAIT));

    const char *filename = parameter;

    if (create_new_cue(filename) < 0)
    {
        rb->splashf(HZ, "creat() failed: %d", cps.cue_fd);
        return PLUGIN_ERROR;
    }

    cps.buffer = rb->plugin_get_buffer(&cps.buffer_sz);
    if (cps.buffer != NULL)
    {
        cps.buffer_index = 0;
#ifdef STORAGE_WANTS_ALIGN
        /* align start and length for DMA */
        STORAGE_ALIGN_BUFFER(cps.buffer, cps.buffer_sz);
#else
        /* align start and length to 32 bit */
        ALIGN_BUFFER(cps.buffer, cps.buffer_sz, 4);
#endif
    }
    if (cps.buffer == NULL|| cps.buffer_sz < CPS_MAX_ENTRY_SZ)
    {
        rb->splashf(HZ, "No Buffers Available :( ");
        return PLUGIN_ERROR;
    }

    if (filename && filename[0])
        res = rb->playlist_entries_iterate(filename, NULL, &playlist_filename_cb);
    else
        res = current_playlist_get_entries();

    if (res)
    {

        if (cps.buffer_index > 0)
        {
            rb->write(cps.cue_fd, cps.buffer, cps.buffer_index);
            cps.buffer_index = 0;

        }
        rb->splashf(HZ * 2,
                    "Playist parsing SUCCESS %d entries written", cps.entries);
    }
    else
    {
        rb->splashf(HZ * 2, "Playist parsing FAILED after %d entries", cps.entries);
    }

    finalize_new_cue();

    if (!res)
        return PLUGIN_ERROR;
    return PLUGIN_OK;
}

/*
#CUE FORMAT
    CATALOG
    CDTEXTFILE
    FILE
    FLAGS
    INDEX
    ISRC
    PERFORMER
    POSTGAP
    PREGAP
    REM
    SONGWRITER
    TITLE
    TRACK
#CD-TEXT https://wyday.com/cuesharp/specification.php
    ARRANGER
    COMPOSER
    DISC_ID
    GENRE
    ISRC
    MESSAGE
    PERFORMER
    SONGWRITER
    TITLE
    TOC_INFO
    TOC_INFO2
    UPC_EAN
    SIZE_INFO
*/
