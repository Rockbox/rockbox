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
Audioscrobbler spec at:
http://www.audioscrobbler.net/wiki/Portable_Player_Logging
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
#define EV_STARTUP     MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x01)
#define EV_TRACKCHANGE MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x02)
#define EV_TRACKFINISH MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x03)

#define SCROBBLER_VERSION "1.1"

/* increment this on any code change that effects output */
#define SCROBBLER_REVISION " $Revision$"

#define SCROBBLER_MAX_CACHE 32
/* longest entry I've had is 323, add a safety margin */
#define SCROBBLER_CACHE_LEN 512

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
#define BASE_FILENAME       ".scrobbler-timeless.log"
#define get_timestamp()     (0l)
#define record_timestamp()  ({})
#endif /* CONFIG_RTC */

#define THREAD_STACK_SIZE 4*DEFAULT_STACK_SIZE

/****************** prototypes ******************/
enum plugin_status plugin_start(const void* parameter); /* entry */

/****************** globals ******************/
unsigned char **language_strings; /* for use with str() macro; must be init */
/* communication to the worker thread */
static struct
{
    bool exiting; /* signal to the thread that we want to exit */
    unsigned int id; /* worker thread id */
    struct event_queue queue; /* thread event queue */
    struct queue_sender_list queue_send;
    long stack[THREAD_STACK_SIZE / sizeof(long)];
} gThread;

static struct
{
    char  *buf;
    int    pos;
    size_t size;
    bool   pending;
    bool   force_flush;
} gCache;

static struct lastfm_config
{
    int  savepct;
    int  beeplvl;
    bool playback;
    bool verbose;
} gConfig;

static struct configdata config[] =
{
   {TYPE_INT, 0, 100, { .int_p = &gConfig.savepct },   "SavePct",  NULL},
   {TYPE_BOOL, 0, 1,  { .bool_p = &gConfig.playback }, "Playback", NULL},
   {TYPE_BOOL, 0, 1,  { .bool_p = &gConfig.verbose },  "Verbose",  NULL},
   {TYPE_INT, 0, 10,  { .int_p = &gConfig.beeplvl },   "BeepLvl",  NULL},
};
const int gCfg_sz = sizeof(config)/sizeof(*config);
/****************** config functions *****************/
static void config_set_defaults(void)
{
    gConfig.savepct = 50;
    gConfig.playback = false;
    gConfig.verbose = true;
    gConfig.beeplvl = 10;
}

static int config_settings_menu(void)
{
    int selection = 0;

    struct viewport parentvp[NB_SCREENS];
    FOR_NB_SCREENS(l)
    {
        rb->viewport_set_defaults(&parentvp[l], l);
        rb->viewport_set_fullscreen(&parentvp[l], l);
    }

    MENUITEM_STRINGLIST(settings_menu, ID2P(LANG_SETTINGS), NULL,
                        ID2P(LANG_RESUME_PLAYBACK),
                        "Save Threshold",
                        "Verbose",
                        "Beep Level",
                        ID2P(VOICE_BLANK),
                        ID2P(LANG_CANCEL_0),
                        ID2P(LANG_SAVE_EXIT));

    do {
        selection=rb->do_menu(&settings_menu,&selection, parentvp, true);
        switch(selection) {

            case 0:
                rb->set_bool(str(LANG_RESUME_PLAYBACK), &gConfig.playback);
                break;
            case 1:
                rb->set_int("Save Threshold", "%", UNIT_PERCENT,
                            &gConfig.savepct, NULL, 10, 0, 100, NULL );
                break;
            case 2:
                rb->set_bool("Verbose", &gConfig.verbose);
                break;
            case 3:
                rb->set_int("Beep Level", "", UNIT_INT,
                            &gConfig.beeplvl, NULL, 1, 0, 10, NULL);
                if (gConfig.beeplvl > 0)
                    rb->beep_play(1500, 100, 100 * gConfig.beeplvl);
            case 4: /*sep*/
                continue;
            case 5:
                return -1;
                break;
            case 6:
            {
                int res = configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
                if (res >= 0)
                {
                    logf("Scrobbler cfg saved %s %d bytes", CFG_FILE, gCfg_sz);
                    return PLUGIN_OK;
                }
                logf("Scrobbler cfg FAILED (%d) %s", res, CFG_FILE);
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

int scrobbler_init(void)
{
    gCache.buf = rb->plugin_get_buffer(&gCache.size);

    size_t reqsz = SCROBBLER_MAX_CACHE*SCROBBLER_CACHE_LEN;
    gCache.size = PLUGIN_BUFFER_SIZE - rb->plugin_reserve_buffer(reqsz);

    if (gCache.size < reqsz)
    {
        logf("SCROBBLER: OOM , %ld < req:%ld", gCache.size, reqsz);
        return -1;
    }

    gCache.pos = 0;
    gCache.pending = false;
    gCache.force_flush = true;
    logf("Scrobbler Initialized");
    return 1;
}

static void get_scrobbler_filename(char *path, size_t size)
{
    int used;

    used = rb->snprintf(path, size, "/%s", BASE_FILENAME);

    if (used >= (int)size)
    {
        logf("%s: not enough buffer space for log file", __func__);
        rb->memset(path, 0, size);
    }
}

static void scrobbler_write_cache(void)
{
    int i;
    int fd;
    logf("%s", __func__);
    char scrobbler_file[MAX_PATH];
    get_scrobbler_filename(scrobbler_file, sizeof(scrobbler_file));

    /* If the file doesn't exist, create it.
    Check at each write since file may be deleted at any time */
    if(!rb->file_exists(scrobbler_file))
    {
        fd = rb->open(scrobbler_file, O_RDWR | O_CREAT, 0666);
        if(fd >= 0)
        {
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

    /* write the cache entries */
    fd = rb->open(scrobbler_file, O_WRONLY | O_APPEND);
    if(fd >= 0)
    {
        logf("SCROBBLER: writing %d entries", gCache.pos);
        /* copy data to temporary storage in case data moves during I/O */
        char temp_buf[SCROBBLER_CACHE_LEN];
        for ( i=0; i < gCache.pos; i++ )
        {
            logf("SCROBBLER: write %d", i);
            char* scrobbler_buf = gCache.buf;
            ssize_t len = rb->strlcpy(temp_buf, scrobbler_buf+(SCROBBLER_CACHE_LEN*i),
                                        sizeof(temp_buf));
            if (rb->write(fd, temp_buf, len) != len)
                break;
        }
        rb->close(fd);
    }
    else
    {
        logf("SCROBBLER: error writing file");
    }

    /* clear even if unsuccessful - don't want to overflow the buffer */
    gCache.pos = 0;
}

#if USING_STORAGE_CALLBACK
static void scrobbler_flush_callback(void)
{
    (void) gCache.force_flush;
    if(gCache.pos <= 0)
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

static inline char* str_chk_valid(char *s, char *alt)
{
    return (s != NULL ? s : alt);
}

static unsigned long scrobbler_get_threshold(unsigned long length)
{
    /* length is assumed to be in miliseconds */
    return length / 100 * gConfig.savepct;

}

static void scrobbler_add_to_cache(const struct mp3entry *id)
{
    static uint32_t last_crc = 0;
    int trk_info_len = 0;

    if ( gCache.pos >= SCROBBLER_MAX_CACHE )
        scrobbler_write_cache();

    char rating = 'S'; /* Skipped */
    char* scrobbler_buf = gCache.buf;

    logf("SCROBBLER: add_to_cache[%d]", gCache.pos);

    if (id->elapsed >= scrobbler_get_threshold(id->length))
        rating = 'L'; /* Listened */

    char tracknum[11] = { "" };

    if (id->tracknum > 0)
        rb->snprintf(tracknum, sizeof (tracknum), "%d", id->tracknum);

    char* artist = id->artist ? id->artist : id->albumartist;

    int ret = rb->snprintf(&scrobbler_buf[(SCROBBLER_CACHE_LEN*gCache.pos)],
                       SCROBBLER_CACHE_LEN,
                       "%s\t%s\t%s\t%s\t%d\t%c%n\t%ld\t%s\n",
                       str_chk_valid(artist, UNTAGGED),
                       str_chk_valid(id->album, ""),
                       str_chk_valid(id->title, ""),
                       tracknum,
                       (int)(id->length / 1000),
                       rating,
                       &trk_info_len,
                       get_timestamp(),
                       str_chk_valid(id->mb_track_id, ""));

    if ( ret >= SCROBBLER_CACHE_LEN )
    {
        logf("SCROBBLER: entry too long:");
        logf("SCROBBLER: %s", id->path);
    }
    else
    {
        uint32_t crc = rb->crc_32(&scrobbler_buf[(SCROBBLER_CACHE_LEN*gCache.pos)],
                                  trk_info_len, 0xFFFFFFFF);
        if (crc != last_crc)
        {
            last_crc = crc;
            logf("Added %s", scrobbler_buf);
            gCache.pos++;
#if USING_STORAGE_CALLBACK
            rb->register_storage_idle_func(scrobbler_flush_callback);
#endif
        }
        else
            logf("SCROBBLER: skipping repeat entry: %s", id->path);
    }

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

static void scrobbler_change_event(unsigned short id, void *ev_data)
{
    (void)id;
    logf("%s", __func__);
    struct mp3entry *id3 = ((struct track_event *)ev_data)->id3;

    /*  check if track was resumed > %threshold played ( likely got saved )
        check for blank artist or track name */
    if ((id3->elapsed > scrobbler_get_threshold(id3->length))
        || (!id3->artist && !id3->albumartist) || !id3->title)
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
    logf("flag %d", te->flags);
   return strflags[te->flags&0x7];
}

#endif
static void scrobbler_finish_event(unsigned short id, void *ev_data)
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
    rb->remove_event(PLAYBACK_EVENT_TRACK_CHANGE, scrobbler_change_event);
    rb->remove_event(PLAYBACK_EVENT_TRACK_FINISH, scrobbler_finish_event);
}

static void events_register(void)
{
    rb->add_event(PLAYBACK_EVENT_TRACK_CHANGE, scrobbler_change_event);
    rb->add_event(PLAYBACK_EVENT_TRACK_FINISH, scrobbler_finish_event);
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
                if (gConfig.beeplvl > 0)
                    rb->beep_play(1500, 100, 100 * gConfig.beeplvl);
                break;
            case SYS_POWEROFF:
            case SYS_REBOOT:
                gCache.force_flush = true;
                /*fall through*/
            case EV_EXIT:
#if USING_STORAGE_CALLBACK
                rb->unregister_storage_idle_func(scrobbler_flush_callback, !in_usb);
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
            default:
                logf("default %ld", ev.id);
                break;
        }
    }
}

void thread_create(void)
{
    /* put the thread's queue in the bcast list */
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

/* callback to end the TSR plugin, called before a new one gets loaded */
static int exit_tsr(bool reenter)
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
                rb->queue_send(&gThread.queue, EV_FLUSHCACHE, 0);
                if (gConfig.verbose)
                    rb->splashf(2*HZ, "%s Cache Flushed", str(LANG_AUDIOSCROBBLER));
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
    struct lastfm_config cfg;
    rb->memcpy(&cfg, & gConfig, sizeof(struct lastfm_config));

    /* Resume plugin ? */
    if (parameter == rb->plugin_tsr)
    {

        gConfig.beeplvl = 0;
        gConfig.playback = false;
        gConfig.verbose = false;
    }

    rb->memset(&gThread, 0, sizeof(gThread));
    if (gConfig.verbose)
        rb->splashf(HZ / 2, "%s Started",str(LANG_AUDIOSCROBBLER));
    logf("%s: %s Started", __func__, str(LANG_AUDIOSCROBBLER));

    rb->plugin_tsr(exit_tsr); /* stay resident */

    thread_create();
    rb->memcpy(&gConfig, &cfg, sizeof(struct lastfm_config));

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
    language_strings = rb->language_strings;
    if (scrobbler_init() < 0)
        return PLUGIN_ERROR;

    config_set_defaults();

    if (configfile_load(CFG_FILE, config, gCfg_sz, CFG_VER) < 0)
    {
        /* If the loading failed, save a new config file */
        config_set_defaults();
        configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);

        rb->splash(HZ, ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS));
    }

    int ret = plugin_main(parameter);
    return ret;
}
