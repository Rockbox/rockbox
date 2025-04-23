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
/* Scrobbler Plugin
Audioscrobbler spec at: (use wayback machine)
http://www.audioscrobbler.net/wiki/Portable_Player_Logging
* EXCERPT:
* The first lines of .scrobbler.log should be header lines, indicated by the leading '#' character:

#AUDIOSCROBBLER/1.1
#TZ/[UNKNOWN|UTC]
#CLIENT/<IDENTIFICATION STRING>

Where 1.1 is the version for this file format

    If the device knows what timezone it is in,
        it must convert all logged times to UTC (aka GMT+0)
        eg: #TZ/UTC
    If the device knows the time, but not the timezone
        eg: #TZ/UNKNOWN

<IDENTIFICATION STRING> should be replaced by the name/model of the hardware device
 and the revision of the software producing the log file.

After the header lines, simply append one line of text for every song
 that is played or skipped.

The following fields comprise each line, and are tab (\t)
 separated (strip any tab characters from the data):

    - artist name
    - album name (optional)
    - track name
    - track position on album (optional)
    - song duration in seconds
    - rating (L if listened at least 50% or S if skipped)
    - unix timestamp when song started playing
    - MusicBrainz Track ID (optional)
lines should be terminated with \n
Example
(listened to enter sandman, skipped cowboys, listened to the pusher) :
 #AUDIOSCROBBLER/1.0
 #TZ/UTC
 #CLIENT/Rockbox h3xx 1.1
 Metallica        Metallica        Enter Sandman        1        365        L        1143374412        62c2e20a?-559e-422f-a44c-9afa7882f0c4?
 Portishead        Roseland NYC Live        Cowboys        2        312        S        1143374777        db45ed76-f5bf-430f-a19f-fbe3cd1c77d3
 Steppenwolf        Live        The Pusher        12        350        L        1143374779        58ddd581-0fcc-45ed-9352-25255bf80bfb?
    If the data for optional fields is not available to you, leave the field blank (\t\t).
    All strings should be written as UTF-8, although the file does not use a BOM.
    All fields except those marked (optional) above are required.
*/

#include "plugin.h"
#include "lib/configfile.h"

#define MIN_ELAPSED_MS (500) /*should not be lower than value found in playback.c*/

#ifndef UNTAGGED
    #define UNTAGGED "<UNTAGGED>"
#endif

#ifdef ROCKBOX_HAS_LOGF
#define logf rb->logf
#else
#define logf(...) do { } while(0)
#endif

/****************** constants ******************/

/* increment this on any code change that effects output */
#define SCROBBLER_VERSION "1.1"

#define SCROBBLER_REVISION " $Revision$"

#define SCROBBLER_BAD_ENTRY "# FAILED - "

/* longest entry I've had is 323, add a safety margin */
#define SCROBBLER_MAXENTRY_LEN (MAX_PATH + 60 + 10)

#define ITEM_HDR "#ARTIST\t#ALBUM\t#TITLE\t#TRACKNUM\t#LENGTH\t#RATING\t#TIMESTAMP\t#MUSICBRAINZ_TRACKID\n"

#define CFG_FILE "/lastfm_scrobbler.cfg"
#define CFG_VER  3

#define SCROBBLER_MENU (PLUGIN_OK + 1)
#define SCROBBLER_LOG_OK (PLUGIN_OK + 2)
#define SCROBBLER_LOG_NOMETADATA (PLUGIN_OK + 3)
#define SCROBBLER_LOG_SKIPTRACK  (PLUGIN_OK + 4)
#define SCROBBLER_LOG_ERROR (PLUGIN_ERROR)

#if CONFIG_RTC
#define BASE_FILENAME       HOME_DIR "/.scrobbler.log"
#define HDR_STR_TIMELESS
#else /* !CONFIG_RTC */
#define HDR_STR_TIMELESS    " Timeless"
#define BASE_FILENAME       HOME_DIR "/.scrobbler-timeless.log"
#endif /* CONFIG_RTC */

#define PLAYBACK_LOG "playback"
#define PLAYBACK_LOG_DIR ROCKBOX_DIR

/****************** prototypes / globals ******************/
enum plugin_status plugin_start(const void* parameter); /* entry */
static int view_playback_log(void);
static int sbl_export(void);

struct scrobbler_entry
{
     unsigned long timestamp;
     unsigned long elapsed;
     unsigned long length;
     const char *path;
};

static struct scrobbler_cfg
{
    int  savepct;
    int  minms;
    int  tracknfo;
    int  remove_dup;
    bool delete_log;
} gConfig;

static struct opt_items tracknfo_option[3] = {
    {ID2P(LANG_ALL), -1 },
    { "Skip if Missing", -1},
    {ID2P(LANG_DISPLAY_TRACK_NAME_ONLY), -1},
};
static char *tracknfo_strs[] = {"Save All", "Skip if Missing", "Filename Only"};

static struct opt_items dup_option[3] = {
    {ID2P(LANG_OFF), -1 },
    {ID2P(LANG_RESUME_PLAYBACK), -1},
    {ID2P(LANG_ALL), -1},
};
static char *dup_strs[] = {"Save All", "No Resume Duplicates", "No Duplicates"};

static struct configdata config[] =
{
    {TYPE_ENUM, 0, 3, { .int_p = &gConfig.tracknfo },              "TrackInfo", tracknfo_strs},
    {TYPE_ENUM, 0, 3, { .int_p =  &gConfig.remove_dup },          "RemoveDupes", dup_strs},
    {TYPE_BOOL, 0, 1, { .bool_p =  &gConfig.delete_log },          "DeleteLog", NULL},
    {TYPE_INT, 0, 100, { .int_p = &gConfig.savepct },              "SavePct", NULL},
    {TYPE_INT, MIN_ELAPSED_MS, 10000, { .int_p = &gConfig.minms }, "MinMs", NULL},
};
const int gCfg_sz = sizeof(config)/sizeof(*config);
static long yield_tick = 0;

/****************** config functions *****************/
static void config_set_defaults(void)
{
    gConfig.savepct = 50;
    gConfig.minms = MIN_ELAPSED_MS;
    gConfig.remove_dup = 0; /* save all*/
    gConfig.delete_log = true;
    gConfig.tracknfo = 0; /* save all */
}

static int scrobbler_menu_action(int selection, bool has_log)
{
    logf("%s sel: %d log: %d", __func__, selection, has_log);

    int res;
    static uint32_t crc = 0;
    if (crc == 0)
    {
        crc = rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF);
    }

    switch(selection)
    {
        case 0: /* 0: all saved 1: no resume duplicates 2: no duplicates */
            rb->set_option("Remove log duplicates", &gConfig.remove_dup, RB_INT,
                           dup_option, 3, NULL);
            break;
        case 1: /* delete log */
            rb->set_bool("Delete playback log", &gConfig.delete_log);
            break;
        case 2: /* % of track played to indicate listened status */
            rb->set_int(ID2P(LANG_COMPRESSOR_THRESHOLD), "%", UNIT_PERCENT,
                        &gConfig.savepct, NULL, 10, 0, 100, NULL );
            break;
        case 3: /* tracks played less than this will not be logged */
            rb->set_int("Minimum Elapsed", "ms", UNIT_MS,
                        &gConfig.minms, NULL, 100, MIN_ELAPSED_MS, 10000, NULL );
            break;
        case 4: /* 0: all saved 1: without metadata skipped 2: only filename saved*/
            rb->set_option(ID2P(LANG_TRACK_INFO), &gConfig.tracknfo, RB_INT,
                           tracknfo_option, 3, NULL);
            break;
        case 5: /* sep */
            break;
        case 6: /* view playback log */
            if (crc != rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF))
            {
                /* there are changes to save */
                if (!rb->yesno_pop(ID2P(LANG_SAVE_CHANGES)))
                {
                    return view_playback_log();
                }
            }
            res = configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
            if (res >= 0)
            {
                crc = rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF);
                logf("SCROBBLER: cfg saved %s %d bytes", CFG_FILE, gCfg_sz);
            }
            return view_playback_log();
            break;
        case 7: /* set defaults */
        {
            const struct text_message prompt = {
                (const char*[]){ ID2P(LANG_AUDIOSCROBBLER),
                                 ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS)}, 2};
            if(rb->gui_syncyesno_run(&prompt, NULL, NULL) == YESNO_YES)
            {
                config_set_defaults();
                rb->splash(HZ, ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS));
            }
            break;
        }
        case 8: /*sep*/
            break;
        case 9: /* Cancel */
            has_log = false;
            if (crc != rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF))
            {
                /* there are changes to save */
                if (!rb->yesno_pop(ID2P(LANG_SAVE_CHANGES)))
                {
                    return -1;
                }
            }
            /* fallthrough */
        case 10: /* Export & exit */
        {
            res = configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
            if (res >= 0)
            {
                logf("SCROBBLER: cfg saved %s %d bytes", CFG_FILE, gCfg_sz);
            }
            else
            {
                rb->splash(HZ*2, ID2P(LANG_ERROR_WRITING_CONFIG));
                logf("SCROBBLER: %s (%d) %s", "Error writing config", res, CFG_FILE);
            }
    #if defined(HAVE_ADJUSTABLE_CPU_FREQ)
            if (has_log)
            {
                rb->cpu_boost(true);
                return sbl_export();
                rb->cpu_boost(false);
            }
    #else
            if (has_log)
                return sbl_export();
    #endif
            return PLUGIN_OK;
        }
        case MENU_ATTACHED_USB:
            return PLUGIN_USB_CONNECTED;
        default:
            return PLUGIN_OK;
    }
    return SCROBBLER_MENU;
}

static int scrobbler_menu(bool resume)
{
    int selection = resume ? 5 : 0; /* if resume we are returning from log view */

    struct viewport parentvp[NB_SCREENS];
    FOR_NB_SCREENS(l)
    {
        rb->viewport_set_defaults(&parentvp[l], l);
        rb->viewport_set_fullscreen(&parentvp[l], l);
    }

    #define MENUITEM_STRINGLIST_CUSTOM(name, str, callback, ... )           \
        static const char *name##_[] = {__VA_ARGS__};                       \
        static const struct menu_callback_with_desc name##__ =              \
                {callback,str, Icon_NOICON};                                \
        struct menu_item_ex name =                                          \
            {MT_RETURN_ID|MENU_HAS_DESC|                                    \
             MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
                { .strings = name##_},{.callback_and_desc = & name##__}};

    MENUITEM_STRINGLIST_CUSTOM(settings_menu, ID2P(LANG_AUDIOSCROBBLER), NULL,
                        "Remove duplicates",
                        "Delete playback log",
                        ID2P(LANG_COMPRESSOR_THRESHOLD),
                        "Minimum elapsed",
                        ID2P(LANG_TRACK_INFO), //Skip tracks without metadata
                        ID2P(VOICE_BLANK),
                        ID2P(LANG_VIEWLOG),
                        ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS),
                        ID2P(VOICE_BLANK),
                        ID2P(LANG_CANCEL_0),
                        ID2P(LANG_EXPORT));

    #undef MENUITEM_STRINGLIST_CUSTOM

    int res;
    const int items = MENU_GET_COUNT(settings_menu.flags);
    const unsigned int flags = settings_menu.flags & (~MENU_ITEM_COUNT(MENU_COUNT_MASK));


    bool has_log = (rb->file_exists(ROCKBOX_DIR "/playback.log")
                 || rb->file_exists(ROCKBOX_DIR "/playback_0001.log"));

    do {
        if (!has_log)
        {
            /* hide save item -- there is no log to export */
            settings_menu.flags = flags|MENU_ITEM_COUNT((items - 1));
        }
        else
        {
            settings_menu.flags = flags|MENU_ITEM_COUNT(items);
        }
        selection=rb->do_menu(&settings_menu,&selection, parentvp, true);

        res = scrobbler_menu_action(selection, has_log);
        if (res != SCROBBLER_MENU)
            return res;

    } while ( selection >= 0 );
    return 0;
}

/****************** helper fuctions ******************/
static void ask_enable_playbacklog(void)
{
    const char *lines[]={"LastFm", "Playback logging required", "Enable?"};
    const char *response[]= {
        "Playback Settings:", "Logging: Enabled",
        "Playback Settings:", "Logging: Disabled"
    };
    const struct text_message message= {lines, 3};
    const struct text_message yes_msg= {&response[0], 2};
    const struct text_message no_msg=  {&response[2], 2};
    if(rb->gui_syncyesno_run(&message, &yes_msg, &no_msg) == YESNO_YES)
    {
        rb->global_settings->playback_log = true;
        rb->settings_save();
        rb->sleep(HZ * 2);
    }
}

static int view_playback_log(void)
{
    const char* plugin = VIEWERS_DIR "/lastfm_scrobbler_viewer.rock";
    /*rb->splashf(100, "Opening %s", plugin);*/
    if (rb->file_exists(plugin))
    {
        return rb->plugin_open(plugin, "-scrobbler_view_pbl");
    }
    return PLUGIN_ERROR;
}

static inline bool do_timed_yield(void)
{
    /* Exporting can lock up for quite a while, so yield occasionally */
    if (TIME_AFTER(*rb->current_tick, yield_tick))
    {
        rb->yield();

        yield_tick = *rb->current_tick + (HZ/5);
        return true;
    }
    return false;
}

static void clear_display(void)
{
    static struct gui_synclist lists = {0};
    rb->lcd_clear_display();
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_clear_display();
#endif

    if (!lists.title) /* initialize the list, only used to display the title..*/
    {
        rb->gui_synclist_init(&lists, NULL, NULL, false,1, NULL);
        rb->gui_synclist_set_title(&lists, rb->str(LANG_AUDIOSCROBBLER), Icon_Moving);
    }

    rb->gui_synclist_draw(&lists);
}

static inline const char* str_chk_valid(char *s, const char *alt)
{
    if (s == NULL || *s == '\0')
        return alt;
    char *sep = s;
    /* strip tabs from the incoming string */
    while ((sep = rb->strchr(sep, '\t')) != NULL)
        *sep = ' ';

    return s;
}

static unsigned long sbl_get_threshold(unsigned long length_ms)
{
    /* length is assumed to be in miliseconds */
    return length_ms / 100 * gConfig.savepct;
}

static int sbl_create_entry(struct scrobbler_entry *entry, int output_fd)
{
    /* creates a scrobbler log entry and writes it to the (opened) output_fd */
    #define SEP "\t"
    #define EOL "\n"
    struct mp3entry id3, *id;
    const char *path = rb->strrchr(entry->path, '/');
    if (!path)
        path = entry->path;
    else
        path++; /* remove slash */
    char rating = 'S'; /* Skipped */
    if (entry->elapsed >= sbl_get_threshold(entry->length))
        rating = 'L'; /* Listened */

#if (CONFIG_RTC)
    unsigned long timestamp = entry->timestamp;
#else
    unsigned long timestamp = 0U;
#endif
    /*lllllllllllllllllllllSlllllllllllllllllllllSrSlllllllllllllllllllllSgggggggg-uuuu-iiii-dddd-eeeeeeeeeeeeN0*/
    static char track_len_rate_timestamp_mbid[128];
    /*106 chars enough for 3 64 bit numbers (l), 4 (S)eparators,
     *  1 (r)ating char and 36 char GUIDe :-), (N)ewline and NULL (0) */

    if (output_fd < 0)
        return SCROBBLER_LOG_ERROR;

    /* if we are saving filename only then we don't need metadata */
    if (gConfig.tracknfo == 2 || !rb->get_metadata(&id3, -1, entry->path))
    {
        if (gConfig.tracknfo == 1) /* user doesn't want tracks missing metadata*/
            return SCROBBLER_LOG_SKIPTRACK;
        /* failure to read metadata not fatal, write what we have */
        rb->snprintf(track_len_rate_timestamp_mbid, sizeof(track_len_rate_timestamp_mbid),
            "%d"SEP"%ld"SEP"%c"SEP"%lu"SEP"%s"EOL"",
            -1, (long)(entry->length / 1000), rating, timestamp, "");

        rb->fdprintf(output_fd,"%s"SEP"%s"SEP"%s"SEP"%s",
                     UNTAGGED,
                     "",
                     path,
                     track_len_rate_timestamp_mbid);
        return SCROBBLER_LOG_NOMETADATA;
    }

    id = &id3;

    char* artist = id->artist ? id->artist : id->albumartist;

    int track = id->tracknum;

    if (track < -1)
        track = -1;

    rb->snprintf(track_len_rate_timestamp_mbid, sizeof(track_len_rate_timestamp_mbid),
        "%d"SEP"%ld"SEP"%c"SEP"%lu"SEP"%s"EOL"", track, (long)(entry->length / 1000),
        rating, timestamp, str_chk_valid(id->mb_track_id, ""));

    rb->fdprintf(output_fd,"%s"SEP"%s"SEP"%s"SEP"%s",
                 str_chk_valid(artist, UNTAGGED),
                 str_chk_valid(id->album, ""),
                 str_chk_valid(id->title, path),
                 track_len_rate_timestamp_mbid);
    #undef SEP
    #undef EOL
    return SCROBBLER_LOG_OK;
}

static int sbl_check_or_open(bool check_only)
{
    /* checks if scrobbler log exists and if !check_only creates it (if needed)
     * and returns handle */
    char scrobbler_file[MAX_PATH];
    int fd;
    int used;
    used = rb->snprintf(scrobbler_file, sizeof(scrobbler_file), "/%s", BASE_FILENAME);

    if (used <= 0 || used >= (int)sizeof(scrobbler_file))
    {
        logf("%s: not enough buffer space for log filename", __func__);
        rb->memset(scrobbler_file, 0, sizeof(scrobbler_file));
    }
    else
    {
        char *p = scrobbler_file;
        while(p[0] == '/' && p[1] == '/')
            p++;
        if (p != scrobbler_file)
            rb->memmove(scrobbler_file, p, rb->strlen(p) + 1);
    }

    logf("%s: log filename '%s'", __func__, scrobbler_file);

    bool exists = rb->file_exists(scrobbler_file);

    if (check_only)
        return exists ? 1: 0;

    /* If the file doesn't exist, create it. */
    if(!exists)
    {
        fd = rb->open(scrobbler_file, O_WRONLY | O_CREAT, 0666);
        if(fd >= 0)
        {
            /* write file header */
            rb->fdprintf(fd, "#AUDIOSCROBBLER/" SCROBBLER_VERSION "\n"
                         "#TZ/UNKNOWN\n" "#CLIENT/Rockbox "
                         TARGET_NAME SCROBBLER_REVISION
                         HDR_STR_TIMELESS "\n");
            rb->fdprintf(fd, ITEM_HDR);
        }
        else
        {
            logf("SCROBBLER: cannot create log file (%s)", scrobbler_file);
        }
    }
    else
        fd = rb->open_utf8(scrobbler_file, O_WRONLY | O_APPEND);

    return fd;
}

static int sbl_open_create(void)
{
    return sbl_check_or_open(false);
}

unsigned long pbl_parse_atoul_wlen(const char *str, size_t *len)
{

    /* we don't need much for atoi no negatives & minimal leading zeros
       there are no checks for NULL pointer either */
    unsigned long d, value = 0;
    *len = 0;
    while (str[*len])
    {
        d = str[*len] - '0';
        if (d < 10)
        {
            value = (value * 10) + d;
        }
        else if (value != 0)
            break;
        *len = *len + 1;
    }
    /*logf("atoul\n in '%s'\nout: %lu len: %lu", str, value, *len);*/
    return value;
}

unsigned long pbl_parse_atoul(const char *str)
{
    /* we don't need much for atoi no negatives & minimal leading zeros
       there are no checks for NULL pointer either */
    size_t len;
    return pbl_parse_atoul_wlen(str, &len);
}

static bool pbl_parse_entry(struct scrobbler_entry *entry, const char *begin)
{
    /* get a playbacklog entry returns true if valid, false otherwise */
    const char *p_time = begin;
    if (!p_time)
        goto failure;
    const char *p_elapsed = rb->strchr(p_time, ':');
    if (!p_elapsed)
        goto failure;
    const char *p_length = rb->strchr(++p_elapsed, ':');
    if (!p_length)
        goto failure;
    const char *p_path = rb->strchr(++p_length, ':');
    if (!p_path || *(++p_path) == '\0')
        goto failure;

    entry->timestamp = pbl_parse_atoul(p_time);
    entry->elapsed = pbl_parse_atoul(p_elapsed);
    entry->length = pbl_parse_atoul(p_length);
    entry->path = p_path;

    if (entry->length == 0 || entry->elapsed > entry->length)
    {
        goto failure;
    }
    return true; /* success */

failure:
    memset(entry, 0, sizeof(*entry));
    return false;
}

static void pbl_parse_valid_entry(struct scrobbler_entry *entry, const char *begin)
{
    /* Same as above but --  No error checking - only for pre-parsed entries */
    size_t len;
    entry->timestamp = pbl_parse_atoul_wlen(begin, &len);
    begin += len + 1; /* skip ':' */
    entry->elapsed = pbl_parse_atoul_wlen(begin, &len);
    begin += len + 1; /* skip ':' */
    entry->length = pbl_parse_atoul_wlen(begin, &len);
    begin += len + 1; /* skip ':' */
    entry->path = begin;
    /* success */
}

static inline bool pbl_cull_duplicates(int fd, struct scrobbler_entry *curentry,
                        int pos, int cur_line, char*buf, size_t bufsz, int *dup)
{
    /* worker for remove_duplicates */
    int line_num = cur_line; /* Note count skips empty lines\comments */
    int rd, start_pos;
    struct scrobbler_entry compare;
    bool b2b = gConfig.remove_dup == 2; /* No duplicates */

    while(1)
    {
        if ((rd = rb->read_line(fd, buf, bufsz)) <= 0)
            break; /* EOF */

        if (buf[0] != ' ') /* skip comments and empty lines */
        {
            rb->yield();
            pos += rd;
            continue;
        }

        /* save start of entry in case we need to remove it */
        start_pos = pos;
        pos += rd;
        line_num++;

        pbl_parse_valid_entry(&compare, buf);

        unsigned long length = compare.length;
        if (curentry->length != length
            || rb->strcmp(curentry->path, compare.path) != 0)
        {
            rb->yield();
            continue; /* different track */
        }
        b2b |= (cur_line + 1 == line_num);
        /* if this is two distinct plays keep both */
        if  (b2b /* unless back to back then its probably a resume */
            || (curentry->timestamp <= compare.timestamp + length
            && compare.timestamp <= curentry->timestamp + length))
        {
            if (curentry->elapsed >= compare.elapsed)
            {
                /* compare entry is not the greatest elapsed */
                /*logf("entry %s (%lu) @ %d culled\n", compare.path, compare.elapsed, line_num);*/
                rb->lseek(fd, start_pos, SEEK_SET);
                rb->write(fd, "@", 1); /* skip this entry */
                rb->lseek(fd, pos, SEEK_SET);
                (*dup)++;
                rb->yield();
            }
            else
            {
                /*current entry is not the greatest elapsed*/
                return false;
            }
        }
    }
    return true; /* this item is unique or the greatest elapsed */
}

static void pbl_remove_duplicates(int fd, char *buf, size_t bufsz, int lines)
{
    /* walks the log and removes duplicate tracks */
    logf("%s() lines: %d\n", __func__, lines);
    struct scrobbler_entry entry;
    static char tmp_buf[SCROBBLER_MAXENTRY_LEN];
    int start_pos, pos = 0;
    int rd;
    int line_num = 0; /* count includes empty lines\comments */
    int dup = 0;

    rb->button_clear_queue();
    rb->lseek(fd, 0, SEEK_SET);

    clear_display();

    rb->splash_progress(line_num, lines,
                   ID2P(LANG_PLAYLIST_SEARCH_MSG), dup, rb->str(LANG_REPEAT));
    while(1)
    {
        if ((rd = rb->read_line(fd, buf, bufsz)) <= 0)
            break;  /* EOF */
        if (do_timed_yield())
        {
            rb->splash_progress(line_num, lines,
               ID2P(LANG_PLAYLIST_SEARCH_MSG), dup, rb->str(LANG_REPEAT));

            if (rb->action_userabort(TIMEOUT_NOBLOCK))
            {
                if (rb->yesno_pop(ID2P(LANG_CANCEL_0)))
                {
                    logf("User canceled");
                    break;
                }
                clear_display();
            }
        }

        line_num++;
        if (buf[0] != ' ') /* skip comments and empty lines */
        {
            rb->splash_progress(line_num, lines,
               ID2P(LANG_PLAYLIST_SEARCH_MSG), dup, rb->str(LANG_REPEAT));
            pos += rd;
            continue;
        }
        pbl_parse_valid_entry(&entry, buf);

        /* save start of entry in case we need to remove it */
        start_pos = pos;
        pos += rd;
        /*logf("current entry %s (%lu) @ %d", entry.path, entry.elapsed, line_num);*/
        if (!pbl_cull_duplicates(fd, &entry, pos, line_num, tmp_buf, sizeof(tmp_buf), &dup))
        {
            rb->lseek(fd, start_pos, SEEK_SET);
            /*logf("entry: %s @ %d is a duplicate", entry.path, line_num);*/
            rb->write(fd, "@", 1); /* skip this entry */
            dup++;
        }
        rb->lseek(fd, pos, SEEK_SET); /* cull moves the file pos.. set it back */
    }

    logf("%s() lines: %d / %d\n", __func__, line_num, lines);
}

static int pbl_copyloop(int fd_copy, const char *src_filename,
                        char *buf, size_t buf_sz, int *total, int *lines)
{
    /* worker for copymerge_logs - copies valid entries from playback logs */
    struct scrobbler_entry entry;
    long next_tick = *rb->current_tick;
    int count = 0;
    int line_num = 0;
    int lines_copied = 0;
    int fd_src = rb->open_utf8(src_filename, O_RDONLY);
    if (fd_src < 0)
    {
        logf("Scrobbler Error opening: %s\n", src_filename);
        rb->splashf(0, "Scrobbler Error opening: %s", src_filename);
        return -1;
    }

    while(rb->read_line(fd_src, buf, buf_sz) > 0)
    {
        char skipch = ' ';
        line_num++;
        do_timed_yield();
        if (buf[0] == '\0') /* skip empty lines */
            continue;
        if (buf[0] == '#') /*copy comments*/
        {
            lines_copied++;
            rb->fdprintf(fd_copy, "%s\n", buf);
            continue;
        }
        /* parse entry will fail if elapsed > length or other invalid entry */
        if (!pbl_parse_entry(&entry, buf))
        {
            logf("%s failed parsing entry @ line: %d\n", __func__, line_num);
            continue;
        }

        /* comment out entries that do not meet users minimum play length */
        if ((int) entry.elapsed < gConfig.minms)
        {
            logf("Skipping path:'%s' @ line: %d\nelapsed: %ld length: %ld\nmin: %d\n",
             entry.path, line_num, entry.elapsed, entry.length, gConfig.minms);
            skipch = '!'; /* ignore this entry */
        }

        /* add a space (or !) to beginning of every line pbl_remove_duplicates
        * will use this to prepend '@' to entries that will be ignored
        * rewrite entry to ensure valid formatting of the parsed fields*/
        rb->fdprintf(fd_copy, "%c%lu:%lu:%lu:%s\n", skipch,
                     entry.timestamp, entry.elapsed, entry.length, entry.path);
        count++;
        lines_copied++;
        if (TIME_AFTER(*rb->current_tick, next_tick))
        {
            rb->splashf(0, ID2P(LANG_PLAYLIST_SAVE_COUNT), *total + count, PLAYBACK_LOG "_old.log");
            next_tick = *rb->current_tick + HZ;
        }
    }

    logf("Scrobbler: copied %d entries from %s", count, src_filename);

    *total += count;
    *lines += lines_copied;
    rb->close(fd_src);
    return count;
}

static int pbl_copymerge_logs(int fd_copy, char *buf, size_t buf_sz, int *lines)
{
    /* copies valid entries from playback.log and playback_####.log */
    int copied;
    int total_tracks = 0;
    int total_lines = 0;
    char filename[MAX_PATH];

    rb->add_playbacklog(NULL); /* ensure the log has been flushed */

    /* We don't want any writes while copying and (possibly) deleting the log */
    bool log_enabled = rb->global_settings->playback_log;
    rb->global_settings->playback_log = false;

    copied = pbl_copyloop(fd_copy, PLAYBACK_LOG_DIR "/"PLAYBACK_LOG".log",
                       buf, buf_sz, &total_tracks, &total_lines);

    if (gConfig.delete_log && copied > 0)
    {
        rb->remove(PLAYBACK_LOG_DIR "/"PLAYBACK_LOG".log");
    }
    rb->global_settings->playback_log = log_enabled;

    if (copied < 0)
    {
        rb->sleep(HZ * 2); /* to let user see fail message */
    }

    /* now check any sequential numbered log files */
    for (int i = 1; i < 9999; i++)
    {
        rb->snprintf(filename, sizeof(filename), "%s/%s_%0*d%s",
                 PLAYBACK_LOG_DIR, PLAYBACK_LOG, 4, i, ".log");

        if (!rb->file_exists(filename))
        {
            logf("Scrobbler: %s doesn't exist", filename);
            break;
        }
        copied = pbl_copyloop(fd_copy, filename, buf, buf_sz, &total_tracks, &total_lines);
        if (copied > 0)
        {
            if (gConfig.delete_log)
            {
                rb->remove(filename);
            }
        }
        else
        {
            rb->sleep(HZ*2); /* to let user see fail message */
        }
    }
    *lines = total_lines;
    return total_tracks;
}

static int sbl_export(void)
{
    /* Export playbacklog to scrobbler.log */
    rb->lcd_clear_display();

    rb->splash(0, ID2P(LANG_WAIT));

    static char buf[SCROBBLER_MAXENTRY_LEN];
    int tracks_total;
    int lines_total  = 0;
    int tracks_saved = 0;
    int missing_meta = 0;
    int scrobbler_fd;
    int fd_copy = rb->open(PLAYBACK_LOG_DIR "/"PLAYBACK_LOG"_old.log", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd_copy < 0)
    {
        logf("Scrobbler Error opening: %s\n", PLAYBACK_LOG_DIR "/"PLAYBACK_LOG"_old.log");
        rb->splashf(HZ *2, "Scrobbler Error opening: %s", PLAYBACK_LOG_DIR "/"PLAYBACK_LOG"_old.log");
        return PLUGIN_ERROR;
    }

    rb->fdprintf(fd_copy, "#Parsed Playback log tags: '#' comment, '!' too short, '@' duplicate\n");

    tracks_total = pbl_copymerge_logs(fd_copy, buf, sizeof(buf), &lines_total);
    logf("%s %d tracks copied %d lines copied\n", __func__, tracks_total, lines_total);

    if (tracks_total > 0)
    {
        if (gConfig.remove_dup > 0)
            pbl_remove_duplicates(fd_copy, buf, sizeof(buf), lines_total);

        rb->lseek(fd_copy, 0, SEEK_SET);

        struct scrobbler_entry entry;
        int rd = 0;
        int line_num = 0;

        scrobbler_fd = sbl_open_create();
        if (scrobbler_fd >= 0)
        {
            clear_display();
            while (1)
            {
                if ((rd = rb->read_line(fd_copy, buf, sizeof(buf))) <= 0)
                    break;
                line_num++;
                if (buf[0] != ' ') /* skip culled entries comments and empty lines */
                    continue;
                pbl_parse_valid_entry(&entry, buf);

                logf("Read (%d) @ line: %d: timestamp: %lu\nelapsed: %ld\nlength: %ld\npath: '%s'\n",
                     rd, line_num, entry.timestamp, entry.elapsed, entry.length, entry.path);

                int ret = sbl_create_entry(&entry, scrobbler_fd);
                if (ret == SCROBBLER_LOG_ERROR)
                    goto entry_error;
                if (ret == SCROBBLER_LOG_SKIPTRACK)
                {
                    missing_meta++;
                    continue;
                }
                if (ret == SCROBBLER_LOG_NOMETADATA)
                    missing_meta++;

                if (do_timed_yield())
                {
                    rb->splash_progress(tracks_saved, tracks_total,
                    "%s %s", rb->str(LANG_EXPORT), rb->str(LANG_TRACKS));
                }
                tracks_saved++;
                /* process our valid entry */
            }
            rb->close(scrobbler_fd);
        }
        logf("%s %d tracks saved", __func__, tracks_saved);
    }
    rb->close(fd_copy);

    if (gConfig.tracknfo == 1)
        rb->snprintf(buf, sizeof(buf), "%d %s", missing_meta, rb->str(LANG_ID3_NO_INFO));
    else
        rb->snprintf(buf, sizeof(buf), "%d %s", missing_meta, rb->str(LANG_DISPLAY_TRACK_NAME_ONLY));

    clear_display();
    rb->splashf(HZ *5, ID2P(LANG_PLAYLIST_SAVE_COUNT),tracks_saved, buf);
    return PLUGIN_OK;
entry_error:
    if (scrobbler_fd >= 0)
        rb->close(scrobbler_fd);
    rb->close(fd_copy);
    return PLUGIN_ERROR;
}

/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(const void* parameter)
{
    bool resume;
    const char * param_str = (const char*) parameter;
    resume = (parameter && param_str[0] == '-'
             && rb->strcasecmp(param_str, "-resume") == 0);

    logf("Resume %s", resume ? "YES" : "NO");

    if (!parameter)
        clear_display();

    if (!resume && !rb->global_settings->playback_log)
        ask_enable_playbacklog();

    /* now go ahead and have fun! */
    if (rb->usb_inserted() == true)
        return PLUGIN_USB_CONNECTED;

    config_set_defaults();

    if (configfile_load(CFG_FILE, config, gCfg_sz, CFG_VER) < 0)
    {
        /* If the loading failed, save a new config file */
        configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
        rb->splash(HZ, ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS));
    }
    else if (!parameter && rb->global_settings->playback_log
             && rb->global_status->last_screen != GO_TO_PLUGIN)
    {
        if (rb->strcasestr(rb->tree_get_context()->currdir, PLUGIN_APPS_DIR) == NULL)
        {
            logf("Auto Export - Last screen: %d", rb->global_status->last_screen);
            if (rb->file_exists(ROCKBOX_DIR "/playback.log")
             || rb->file_exists(ROCKBOX_DIR "/playback_0001.log"))
            {
                return scrobbler_menu_action(10, true); /* export scrobbler file */
            }
            else
            {
                rb->splashf(HZ, "0 %s", rb->str(LANG_TRACKS));
                return PLUGIN_OK;
            }
        }
    }

    return scrobbler_menu(resume);
}
