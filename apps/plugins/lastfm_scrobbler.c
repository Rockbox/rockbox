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
 * Converted to Plugin
 * Copyright (C) 2022 William Wilgus
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

#ifndef UNTAGGED
    #define UNTAGGED "<UNTAGGED>"
#endif

#ifdef ROCKBOX_HAS_LOGF
#define logf rb->logf
#else
#define logf(...) do { } while(0)
#endif

/****************** constants ******************/
#define EV_EXIT        MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFF)
#define EV_FLUSHCACHE  MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFE)
#define EV_USER_ERROR  MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFD)
#define EV_STARTUP     MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x01)
#define EV_TRACKCHANGE MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x02)
#define EV_TRACKFINISH MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x03)

#define ERR_NONE (0)
#define ERR_WRITING_FILE (-1)
#define ERR_ENTRY_LENGTH (-2)
#define ERR_WRITING_DATA (-3)

/* increment this on any code change that effects output */
#define SCROBBLER_VERSION "1.1"

#define SCROBBLER_REVISION " $Revision$"

#define SCROBBLER_BAD_ENTRY "# FAILED - "

/* longest entry I've had is 323, add a safety margin */
#define SCROBBLER_CACHE_LEN (512)
#define SCROBBLER_MAX_CACHE (32 * SCROBBLER_CACHE_LEN)

#define SCROBBLER_MAX_TRACK_MRU (32) /* list of hashes to detect repeats */

#define ITEM_HDR "#ARTIST #ALBUM #TITLE #TRACKNUM #LENGTH #RATING #TIMESTAMP #MUSICBRAINZ_TRACKID\n"

#define CFG_FILE "/lastfm_scrobbler.cfg"
#define CFG_VER  1

#if CONFIG_RTC
static time_t timestamp;
#define BASE_FILENAME       HOME_DIR "/.scrobbler.log"
#define HDR_STR_TIMELESS
#define get_timestamp()     ((long)timestamp)
#define record_timestamp()  ((void)(timestamp = rb->mktime(rb->get_time())))
#else /* !CONFIG_RTC */
#define HDR_STR_TIMELESS    " Timeless"
#define BASE_FILENAME       HOME_DIR "/.scrobbler-timeless.log"
#define get_timestamp()     (0l)
#define record_timestamp()  ({})
#endif /* CONFIG_RTC */

#define THREAD_STACK_SIZE 4*DEFAULT_STACK_SIZE

/****************** prototypes ******************/
enum plugin_status plugin_start(const void* parameter); /* entry */
void play_tone(unsigned int frequency, unsigned int duration);
/****************** globals ******************/
/* communication to the worker thread */
static struct
{
    bool exiting; /* signal to the thread that we want to exit */
    unsigned int id; /* worker thread id */
    struct event_queue queue; /* thread event queue */
    struct queue_sender_list queue_send;
    long stack[THREAD_STACK_SIZE / sizeof(long)];
} gThread;

struct cache_entry
{
    size_t   len;
    uint32_t crc;
    char     buf[ ];
};

static struct scrobbler_cache
{
    int    entries;
    char  *buf;
    size_t pos;
    size_t size;
    bool   pending;
    bool   force_flush;
    struct mutex mtx;
} gCache;

static struct scrobbler_cfg
{
    int  uniqct;
    int  savepct;
    int  minms;
    int  beeplvl;
    bool playback;
    bool verbose;
} gConfig;

static struct configdata config[] =
{
    #define MAX_MRU (SCROBBLER_MAX_TRACK_MRU)
    {TYPE_INT, 0, MAX_MRU, { .int_p = &gConfig.uniqct }, "UniqCt",  NULL},
    {TYPE_INT, 0, 100, { .int_p = &gConfig.savepct },    "SavePct",  NULL},
    {TYPE_INT, 0, 10000, { .int_p = &gConfig.minms },    "MinMs",    NULL},
    {TYPE_BOOL, 0, 1,  { .bool_p = &gConfig.playback },  "Playback", NULL},
    {TYPE_BOOL, 0, 1,  { .bool_p = &gConfig.verbose },   "Verbose",  NULL},
    {TYPE_INT, 0, 10,  { .int_p = &gConfig.beeplvl },    "BeepLvl",  NULL},
    #undef MAX_MRU
};
const int gCfg_sz = sizeof(config)/sizeof(*config);

/****************** config functions *****************/
static void config_set_defaults(void)
{
    gConfig.uniqct = SCROBBLER_MAX_TRACK_MRU;
    gConfig.savepct = 50;
    gConfig.minms = 500;
    gConfig.playback = false;
    gConfig.verbose = true;
    gConfig.beeplvl = 10;
}

static int config_settings_menu(void)
{
    int selection = 0;

    static uint32_t crc = 0;

    struct viewport parentvp[NB_SCREENS];
    FOR_NB_SCREENS(l)
    {
        rb->viewport_set_defaults(&parentvp[l], l);
        rb->viewport_set_fullscreen(&parentvp[l], l);
    }

    #define MENUITEM_STRINGLIST_CUSTOM(name, str, callback, ... )                  \
        static const char *name##_[] = {__VA_ARGS__};                       \
        static const struct menu_callback_with_desc name##__ =              \
                {callback,str, Icon_NOICON};                                \
        struct menu_item_ex name =                             \
            {MT_RETURN_ID|MENU_HAS_DESC|                                    \
             MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
                { .strings = name##_},{.callback_and_desc = & name##__}};

    MENUITEM_STRINGLIST_CUSTOM(settings_menu, ID2P(LANG_SETTINGS), NULL,
                        ID2P(LANG_RESUME_PLAYBACK),
                        "Save Threshold",
                        "Minimum Elapsed",
                        "Verbose",
                        "Beep Level",
                        "Unique Track MRU",
                        ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS),
                        ID2P(VOICE_BLANK),
                        ID2P(LANG_CANCEL_0),
                        ID2P(LANG_SAVE_EXIT));

    #undef MENUITEM_STRINGLIST_CUSTOM

    const int items = MENU_GET_COUNT(settings_menu.flags);
    const unsigned int flags = settings_menu.flags & (~MENU_ITEM_COUNT(MENU_COUNT_MASK));
    if (crc == 0)
    {
        crc = rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF);
    }

    do {
        if (crc == rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF))
        {
            /* hide save item -- there are no changes to save */
            settings_menu.flags = flags|MENU_ITEM_COUNT((items - 1));
        }
        else
        {
            settings_menu.flags = flags|MENU_ITEM_COUNT(items);
        }
        selection=rb->do_menu(&settings_menu,&selection, parentvp, true);
        switch(selection) {

            case 0: /* resume playback on plugin start */
                rb->set_bool(rb->str(LANG_RESUME_PLAYBACK), &gConfig.playback);
                break;
            case 1: /* % of track played to indicate listened status */
                rb->set_int("Save Threshold", "%", UNIT_PERCENT,
                            &gConfig.savepct, NULL, 10, 0, 100, NULL );
                break;
            case 2: /* tracks played less than this will not be logged */
                rb->set_int("Minimum Elapsed", "ms", UNIT_MS,
                            &gConfig.minms, NULL, 100, 0, 10000, NULL );
                break;
            case 3: /* suppress non-error messages */
                rb->set_bool("Verbose", &gConfig.verbose);
                break;
            case 4: /* set volume of start-up beep */
                rb->set_int("Beep Level", "", UNIT_INT,
                            &gConfig.beeplvl, NULL, 1, 0, 10, NULL);
                play_tone(1500, 100);
                break;
            case 5: /* keep a list of tracks to prevent repeat [Skipped] entries */
                rb->set_int("Unique Track MRU Size", "", UNIT_INT,
                            &gConfig.uniqct, NULL, 1, 0, SCROBBLER_MAX_TRACK_MRU, NULL);
                break;
            case 6: /* set defaults */
            {
                const struct text_message prompt = {
                    (const char*[]){ ID2P(LANG_AUDIOSCROBBLER),
                                     ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS)}, 2};
                if(rb->gui_syncyesno_run(&prompt, NULL, NULL) == YESNO_YES)
                {
                    config_set_defaults();
                    if (gConfig.verbose)
                        rb->splash(HZ, ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS));
                }
                break;
            }
            case 7: /*sep*/
                continue;
            case 8: /* Cancel */
                return -1;
                break;
            case 9: /* Save & exit */
            {
                int res = configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
                if (res >= 0)
                {
                    crc = rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF);
                    logf("SCROBBLER: cfg saved %s %d bytes", CFG_FILE, gCfg_sz);
                    return PLUGIN_OK;
                }
                logf("SCROBBLER: cfg FAILED (%d) %s", res, CFG_FILE);
                return PLUGIN_ERROR;
            }
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            default:
                return PLUGIN_OK;
        }
    } while ( selection >= 0 );
    return 0;
}

/****************** helper fuctions ******************/
void play_tone(unsigned int frequency, unsigned int duration)
{
    if (gConfig.beeplvl > 0)
        rb->beep_play(frequency, duration, 100 * gConfig.beeplvl);
}

int scrobbler_init_cache(void)
{
    memset(&gCache, 0, sizeof(struct scrobbler_cache));
    gCache.buf = rb->plugin_get_buffer(&gCache.size);

    /* we need to reserve the space we want for our use in TSR plugins since
     * someone else could call plugin_get_buffer() and corrupt our memory */
    size_t reqsz = SCROBBLER_MAX_CACHE;
    gCache.size = PLUGIN_BUFFER_SIZE - rb->plugin_reserve_buffer(reqsz);

    if (gCache.size < reqsz)
    {
        logf("SCROBBLER: OOM , %ld < req:%ld", gCache.size, reqsz);
        return -1;
    }
    gCache.force_flush = true;
    rb->mutex_init(&gCache.mtx);
    logf("SCROBBLER: Initialized");
    return 1;
}

static inline size_t cache_get_entry_size(int str_len)
{
    /* entry_sz consists of the cache entry + str_len + \0NULL terminator */
    return ALIGN_UP(str_len + 1 + sizeof(struct cache_entry), 0x4);
}

static inline const char* str_chk_valid(const char *s, const char *alt)
{
    return (s != NULL ? s : alt);
}

static bool track_is_unique(uint32_t hash1, uint32_t hash2)
{
    bool is_unique = false;
    static uint8_t mru_len = 0;

    struct hash64 { uint32_t hash1; uint32_t hash2; };

    static struct hash64 hash_mru[SCROBBLER_MAX_TRACK_MRU];
    struct hash64 i = {0};
    struct hash64 itmp;
    uint8_t mru;

    if (mru_len > gConfig.uniqct)
        mru_len = gConfig.uniqct;

    if (gConfig.uniqct < 1)
        return true;

    /* Search in MRU */
    for (mru = 0; mru < mru_len; mru++)
    {
        /* Items shifted >> 1 */
        itmp = i;
        i = hash_mru[mru];
            hash_mru[mru] = itmp;

        /* Found in MRU */
        if ((i.hash1 == hash1) && (i.hash2 == hash2))
        {
            logf("SCROBBLER: hash [%x, %x] found in MRU @ %d", i.hash1, i.hash2, mru);
            goto Found;
        }
    }

    /* Add MRU entry */
    is_unique = true;
    if (mru_len < SCROBBLER_MAX_TRACK_MRU && mru_len < gConfig.uniqct)
    {
        hash_mru[mru_len] = i;
        mru_len++;
    }
    else
    {
        logf("SCROBBLER: hash [%x, %x] evicted from MRU", i.hash1, i.hash2);
    }

    i = (struct hash64){.hash1 = hash1, .hash2 = hash2};
    logf("SCROBBLER: hash [%x, %x] added to  MRU[%d]", i.hash1, i.hash2, mru_len);

Found:

    /* Promote MRU item to top of MRU */
    hash_mru[0] = i;

    return is_unique;
}

static void get_scrobbler_filename(char *path, size_t size)
{
    int used;

    used = rb->snprintf(path, size, "/%s", BASE_FILENAME);

    if (used >= (int)size)
    {
        logf("%s: not enough buffer space for log filename", __func__);
        rb->memset(path, 0, size);
    }
}

static void scrobbler_write_cache(void)
{
    int i;
    int fd;
    logf("%s", __func__);
    char scrobbler_file[MAX_PATH];

    rb->mutex_lock(&gCache.mtx);

    get_scrobbler_filename(scrobbler_file, sizeof(scrobbler_file));

    /* If the file doesn't exist, create it.
    Check at each write since file may be deleted at any time */
    if(!rb->file_exists(scrobbler_file))
    {
        fd = rb->open(scrobbler_file, O_RDWR | O_CREAT, 0666);
        if(fd >= 0)
        {
            /* write file header */
            rb->fdprintf(fd, "#AUDIOSCROBBLER/" SCROBBLER_VERSION "\n"
                         "#TZ/UNKNOWN\n" "#CLIENT/Rockbox "
                         TARGET_NAME SCROBBLER_REVISION
                         HDR_STR_TIMELESS "\n");
            rb->fdprintf(fd, ITEM_HDR);

            rb->close(fd);
        }
        else
        {
            logf("SCROBBLER: cannot create log file (%s)", scrobbler_file);
        }
    }

    int entries = gCache.entries;
    size_t used = gCache.pos;
    size_t pos = 0;
    /* clear even if unsuccessful - we don't want to overflow the buffer */
    gCache.pos = 0;
    gCache.entries = 0;

    /* write the cache entries */
    fd = rb->open(scrobbler_file, O_WRONLY | O_APPEND);
    if(fd >= 0)
    {
        logf("SCROBBLER: writing %d entries", entries);
        /* copy cached data to storage */
        uint32_t prev_crc = 0x0;
        uint32_t crc;
        size_t entry_sz, len;
        bool err = false;

        for (i = 0; i < entries && pos < used; i++)
        {
            logf("SCROBBLER: write %d read pos [%ld]", i, pos);

            struct cache_entry *entry = (struct cache_entry*)&gCache.buf[pos];

            entry_sz = cache_get_entry_size(entry->len);
            crc = rb->crc_32(entry->buf, entry->len, 0xFFFFFFFF) ^ prev_crc;
            prev_crc = crc;

            len = rb->strlen(entry->buf);
            logf("SCROBBLER: write entry %d sz [%ld] len [%ld]", i, entry_sz, len);

            if (len != entry->len || crc != entry->crc) /* the entry is corrupted */
            {
                rb->write(fd, SCROBBLER_BAD_ENTRY, sizeof(SCROBBLER_BAD_ENTRY)-1);
                logf("SCROBBLER: Bad entry %d", i);
                if(!err)
                {
                    rb->queue_post(&gThread.queue, EV_USER_ERROR, ERR_WRITING_DATA);
                    err = true;
                }
            }

            logf("SCROBBLER: writing %s", entry->buf);

            if (rb->write(fd, entry->buf, len) != (ssize_t)len)
                break;

            if (entry->buf[len - 1] != '\n')
                rb->write(fd, "\n", 1); /* ensure newline termination */

            pos += entry_sz;
        }
        rb->close(fd);
    }
    else
    {
        logf("SCROBBLER: error writing file");
        rb->queue_post(&gThread.queue, EV_USER_ERROR, ERR_WRITING_FILE);
    }
    rb->mutex_unlock(&gCache.mtx);
}

#if USING_STORAGE_CALLBACK
static void scrobbler_flush_callback(void)
{
    if(gCache.pos == 0)
        return;
#if (CONFIG_STORAGE & STORAGE_ATA)
    else
#else
    if ((gCache.pos >= SCROBBLER_MAX_CACHE / 2) || gCache.force_flush == true)
#endif
    {
        gCache.force_flush = false;
        logf("%s", __func__);
        scrobbler_write_cache();
    }
}
#endif

static unsigned long scrobbler_get_threshold(unsigned long length)
{
    /* length is assumed to be in miliseconds */
    return length / 100 * gConfig.savepct;
}

static int create_log_entry(const struct mp3entry *id,
                        struct cache_entry *entry, int *trk_info_len)
{
    #define SEP "\t"
    #define EOL "\n"
    char* artist = id->artist ? id->artist : id->albumartist;
    char rating = 'S'; /* Skipped */
    if (id->elapsed >= scrobbler_get_threshold(id->length))
        rating = 'L'; /* Listened */

    char tracknum[11] = { "" };

    if (id->tracknum > 0)
        rb->snprintf(tracknum, sizeof (tracknum), "%d", id->tracknum);

    int ret = rb->snprintf(entry->buf,
                   SCROBBLER_CACHE_LEN,
                   "%s"SEP"%s"SEP"%s"SEP"%s"SEP"%d%n"SEP"%c"SEP"%ld"SEP"%s"EOL"",
                   str_chk_valid(artist, UNTAGGED),
                   str_chk_valid(id->album, ""),
                   str_chk_valid(id->title, id->path),
                   tracknum,
                   (int)(id->length / 1000),
                   trk_info_len, /* receives len of the string written so far */
                   rating,
                   get_timestamp(),
                   str_chk_valid(id->mb_track_id, ""));

    #undef SEP
    #undef EOL
    return ret;
}

static void scrobbler_add_to_cache(const struct mp3entry *id)
{
    int trk_info_len = 0;

    if (id->elapsed < (unsigned long) gConfig.minms)
    {
        logf("SCROBBLER: skipping entry < %d ms: %s", gConfig.minms, id->path);
        return;
    }

    rb->mutex_lock(&gCache.mtx);

    /* not enough room left to guarantee next entry will fit so flush the cache */
    if ( gCache.pos > SCROBBLER_MAX_CACHE - SCROBBLER_CACHE_LEN )
        scrobbler_write_cache();

    logf("SCROBBLER: add_to_cache[%d] write pos[%ld]", gCache.entries, gCache.pos);
    /* use prev_crc to allow whole buffer to be checked for consistency */
    static uint32_t prev_crc = 0x0;
    if (gCache.pos == 0)
        prev_crc = 0x0;

    void *buf = &gCache.buf[gCache.pos];
    memset(buf, 0, SCROBBLER_CACHE_LEN);

    struct cache_entry *entry = buf;

    int ret = create_log_entry(id, entry, &trk_info_len);

    if (ret <= 0 || (size_t) ret >= SCROBBLER_CACHE_LEN)
    {
        logf("SCROBBLER: entry too long:");
        logf("SCROBBLER: %s", id->path);
        rb->queue_post(&gThread.queue, EV_USER_ERROR, ERR_ENTRY_LENGTH);
    }
    else if (ret > 0)
    {
        /* first generate a crc over the static portion of the track info data
        this and a crc of the filename will be used to detect repeat entries
        */
        static uint32_t last_crc = 0;
        uint32_t crc_entry = rb->crc_32(entry->buf, trk_info_len, 0xFFFFFFFF);
        uint32_t crc_path = rb->crc_32(id->path, rb->strlen(id->path), 0xFFFFFFFF);
        bool is_unique = track_is_unique(crc_entry, crc_path);
        bool is_listened = (id->elapsed >= scrobbler_get_threshold(id->length));

        if (is_unique || is_listened)
        {
            /* finish calculating the CRC of the whole entry */
            const void *src = entry->buf + trk_info_len;
            entry->crc = rb->crc_32(src, ret - trk_info_len, crc_entry) ^ prev_crc;
            prev_crc = entry->crc;
            entry->len = ret;

            /* since Listened entries are written regardless
               make sure this isn't a direct repeat */
            if ((entry->crc ^ crc_path) != last_crc)
            {

                if (is_listened)
                    last_crc = (entry->crc ^ crc_path);
                else
                    last_crc = 0;

                size_t entry_sz = cache_get_entry_size(ret);

                logf("SCROBBLER: Added (#%d) sz[%ld] len[%d], %s",
                     gCache.entries, entry_sz, ret, entry->buf);

                gCache.entries++;
                /* increase pos by string len + null terminator + sizeof entry */
                gCache.pos += entry_sz;

#if USING_STORAGE_CALLBACK
                rb->register_storage_idle_func(scrobbler_flush_callback);
#endif
            }
        }
        else
            logf("SCROBBLER: skipping repeat entry: %s", id->path);
    }
    rb->mutex_unlock(&gCache.mtx);
}

static void scrobbler_flush_cache(void)
{
    logf("%s", __func__);
        /* Add any pending entries to the cache */
    if (gCache.pending)
    {
        gCache.pending = false;
        if (rb->audio_status())
            scrobbler_add_to_cache(rb->audio_current_track());
    }

    /* Write the cache to disk if needed */
    if (gCache.pos > 0)
    {
        scrobbler_write_cache();
    }
}

static void track_change_event(unsigned short id, void *ev_data)
{
    (void)id;
    logf("%s", __func__);
    struct mp3entry *id3 = ((struct track_event *)ev_data)->id3;

    /*  check if track was resumed > %threshold played ( likely got saved ) */
    if ((id3->elapsed > scrobbler_get_threshold(id3->length)))
    {
        gCache.pending = false;
        logf("SCROBBLER: skipping file %s", id3->path);
    }
    else
    {
        logf("SCROBBLER: add pending %s",id3->path);
        record_timestamp();
        gCache.pending = true;
    }
}

#ifdef ROCKBOX_HAS_LOGF
static const char* track_event_info(struct track_event* te)
{

    static const char *strflags[] = {"TEF_NONE", "TEF_CURRENT",
                                     "TEF_AUTOSKIP", "TEF_CUR|ASKIP",
                                     "TEF_REWIND", "TEF_CUR|REW",
                                     "TEF_ASKIP|REW", "TEF_CUR|ASKIP|REW"};
/*TEF_NONE      = 0x0,  no flags are set
* TEF_CURRENT   = 0x1,   event is for the current track
* TEF_AUTO_SKIP = 0x2,   event is sent in context of auto skip
* TEF_REWIND    = 0x4,   interpret as rewind, id3->elapsed is the
                             position before the seek back to 0
*/
    logf("SCROBBLER: flag %d", te->flags);
   return strflags[te->flags&0x7];
}
#endif

static void track_finish_event(unsigned short id, void *ev_data)
{
    (void)id;
    struct track_event *te = ((struct track_event *)ev_data);
    logf("%s %s %s", __func__, gCache.pending?"True":"False", track_event_info(te));
    /* add entry using the currently ending track */
    if (gCache.pending && (te->flags & TEF_CURRENT) && !(te->flags & TEF_REWIND))
    {
        gCache.pending = false;

        scrobbler_add_to_cache(te->id3);
    }
}

/****************** main thread + helpers ******************/
static void events_unregister(void)
{
    /* we don't want any more events */
    rb->remove_event(PLAYBACK_EVENT_TRACK_CHANGE, track_change_event);
    rb->remove_event(PLAYBACK_EVENT_TRACK_FINISH, track_finish_event);
}

static void events_register(void)
{
    rb->add_event(PLAYBACK_EVENT_TRACK_CHANGE, track_change_event);
    rb->add_event(PLAYBACK_EVENT_TRACK_FINISH, track_finish_event);
}

void thread(void)
{
    bool in_usb = false;

    struct queue_event ev;
    while (!gThread.exiting)
    {
        rb->queue_wait(&gThread.queue, &ev);

        switch (ev.id)
        {
            case SYS_USB_CONNECTED:
                scrobbler_flush_cache();
                rb->usb_acknowledge(SYS_USB_CONNECTED_ACK);
                in_usb = true;
                break;
            case SYS_USB_DISCONNECTED:
                in_usb = false;
                /*fall through*/
            case EV_STARTUP:
                events_register();
                play_tone(1500, 100);
                break;
            case SYS_POWEROFF:
            case SYS_REBOOT:
                gCache.force_flush = true;
                /*fall through*/
            case EV_EXIT:
#if USING_STORAGE_CALLBACK
                rb->unregister_storage_idle_func(scrobbler_flush_callback, false);
#else
                if (!in_usb)
                    scrobbler_flush_cache();
#endif
                events_unregister();
                return;
            case EV_FLUSHCACHE:
                scrobbler_flush_cache();
                rb->queue_reply(&gThread.queue, 0);
                break;
            case EV_USER_ERROR:
                if (!in_usb)
                {
                    if (ev.data == ERR_WRITING_FILE)
                        rb->splash(HZ, "SCROBBLER: error writing log");
                    else if (ev.data == ERR_ENTRY_LENGTH)
                        rb->splash(HZ, "SCROBBLER: error entry too long");
                    else if (ev.data == ERR_WRITING_DATA)
                        rb->splash(HZ, "SCROBBLER: error bad entry data");
                }
                break;
            default:
                logf("default %ld", ev.id);
                break;
        }
    }
}

void thread_create(void)
{
    /* put the thread's queue in the broadcast list */
    rb->queue_init(&gThread.queue, true);
    gThread.id = rb->create_thread(thread, gThread.stack, sizeof(gThread.stack),
                                      0, "Last.Fm_TSR"
                                      IF_PRIO(, PRIORITY_BACKGROUND)
                                      IF_COP(, CPU));
    rb->queue_enable_queue_send(&gThread.queue, &gThread.queue_send, gThread.id);
    rb->queue_post(&gThread.queue, EV_STARTUP, 0);
    rb->yield();
}

void thread_quit(void)
{
    if (!gThread.exiting) {
        gThread.exiting = true;
        rb->queue_post(&gThread.queue, EV_EXIT, 0);
        rb->thread_wait(gThread.id);
        /* remove the thread's queue from the broadcast list */
        rb->queue_delete(&gThread.queue);
    }
}

/* callback to end the TSR plugin, called before a new plugin gets loaded */
static int plugin_exit_tsr(bool reenter)
{
    MENUITEM_STRINGLIST(menu, ID2P(LANG_AUDIOSCROBBLER), NULL, ID2P(LANG_SETTINGS),
                        "Flush Cache", "Exit Plugin", ID2P(LANG_BACK));

    const struct text_message quit_prompt = {
        (const char*[]){ ID2P(LANG_AUDIOSCROBBLER),
                         "is currently running.",
                         "Quit scrobbler?" }, 3
    };

    while(true)
    {
        int result = reenter ? rb->do_menu(&menu, NULL, NULL, false) : 2;
        switch(result)
        {
            case 0: /* settings */
                config_settings_menu();
                break;
            case 1: /* flush cache */
                if (gCache.entries > 0)
                {
                    rb->queue_send(&gThread.queue, EV_FLUSHCACHE, 0);
                    if (gConfig.verbose)
                        rb->splashf(2*HZ, "%s Cache Flushed", rb->str(LANG_AUDIOSCROBBLER));
                }
                break;

            case 2: /* exit plugin - quit */
                if(rb->gui_syncyesno_run(&quit_prompt, NULL, NULL) == YESNO_YES)
                {
                    scrobbler_flush_cache();
                    thread_quit();
                    return (reenter ? PLUGIN_TSR_TERMINATE : PLUGIN_TSR_SUSPEND);
                }
                /* Fall Through */
            case 3: /* back to menu */
                return PLUGIN_TSR_CONTINUE;
        }
    }
}

/****************** main ******************/
static int plugin_main(const void* parameter)
{
    struct scrobbler_cfg cfg;
    rb->memcpy(&cfg, &gConfig, sizeof(struct scrobbler_cfg)); /* store settings */

    /* Resume plugin ? -- silences startup */
    if (parameter == rb->plugin_tsr)
    {
        gConfig.beeplvl = 0;
        gConfig.playback = false;
        gConfig.verbose = false;
    }

    rb->memset(&gThread, 0, sizeof(gThread));
    if (gConfig.verbose)
        rb->splashf(HZ / 2, "%s Started",rb->str(LANG_AUDIOSCROBBLER));
    logf("%s: %s Started", __func__, rb->str(LANG_AUDIOSCROBBLER));

    rb->plugin_tsr(plugin_exit_tsr); /* stay resident */

    thread_create();
    rb->memcpy(&gConfig, &cfg, sizeof(struct scrobbler_cfg)); /*restore settings */

    if (gConfig.playback)
        return PLUGIN_GOTO_WPS;

    return PLUGIN_OK;
}

/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(const void* parameter)
{
    /* now go ahead and have fun! */
    if (rb->usb_inserted() == true)
        return PLUGIN_USB_CONNECTED;

    if (scrobbler_init_cache() < 0)
        return PLUGIN_ERROR;

    config_set_defaults();

    if (configfile_load(CFG_FILE, config, gCfg_sz, CFG_VER) < 0)
    {
        /* If the loading failed, save a new config file */
        configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
        if (gConfig.verbose)
            rb->splash(HZ, ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS));
    }

    int ret = plugin_main(parameter);
    return ret;
}
