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

#define ITEM_HDR "#ARTIST #ALBUM #TITLE #TRACKNUM #LENGTH #RATING #TIMESTAMP #MUSICBRAINZ_TRACKID\n"

#define CFG_FILE "/lastfm_scrobbler.cfg"
#define CFG_VER  3

#if CONFIG_RTC
#define BASE_FILENAME       HOME_DIR "/.scrobbler.log"
#define HDR_STR_TIMELESS
#else /* !CONFIG_RTC */
#define HDR_STR_TIMELESS    " Timeless"
#define BASE_FILENAME       HOME_DIR "/.scrobbler-timeless.log"
#endif /* CONFIG_RTC */

/****************** prototypes ******************/
enum plugin_status plugin_start(const void* parameter); /* entry */
static int view_playback_log(void);
static int export_scrobbler_file(void);

struct scrobbler_entry
{
     unsigned long timestamp;
     unsigned long elapsed;
     unsigned long length;
     char *path;
};

static struct scrobbler_cfg
{
    int  savepct;
    int  minms;
    bool remove_dup;
    bool delete_log;

} gConfig;

static struct configdata config[] =
{
    {TYPE_BOOL, 0, 1, { .bool_p =  &gConfig.remove_dup }, "RemoveDupes", NULL},
    {TYPE_BOOL, 0, 1, { .bool_p =  &gConfig.delete_log }, "DeleteLog",   NULL},
    {TYPE_INT, 0, 100, { .int_p = &gConfig.savepct },     "SavePct",     NULL},
    {TYPE_INT, 0, 10000, { .int_p = &gConfig.minms },     "MinMs",       NULL},
};
const int gCfg_sz = sizeof(config)/sizeof(*config);

/****************** config functions *****************/
static void config_set_defaults(void)
{
    gConfig.savepct = 50;
    gConfig.minms = 500;
    gConfig.remove_dup = true;
    gConfig.delete_log = true;
}

static int scrobbler_menu(bool resume)
{
    int selection = resume ? 5 : 0; /* if resume we are returning from log view */

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

    MENUITEM_STRINGLIST_CUSTOM(settings_menu, ID2P(LANG_AUDIOSCROBBLER), NULL,
                        "Remove duplicates",
                        "Delete playback log",
                        "Save threshold",
                        "Minimum elapsed",
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
    if (crc == 0)
    {
        crc = rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF);
    }

    bool has_log = rb->file_exists(ROCKBOX_DIR "/playback.log");

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
        switch(selection) {

            case 0: /* remove duplicates */
                rb->set_bool("Remove log duplicates", &gConfig.remove_dup);
                break;
            case 1: /* delete log */
                rb->set_bool("Delete playback log", &gConfig.delete_log);
                break;
            case 2: /* % of track played to indicate listened status */
                rb->set_int("Save Threshold", "%", UNIT_PERCENT,
                            &gConfig.savepct, NULL, 10, 0, 100, NULL );
                break;
            case 3: /* tracks played less than this will not be logged */
                rb->set_int("Minimum Elapsed", "ms", UNIT_MS,
                            &gConfig.minms, NULL, 100, 0, 10000, NULL );
                break;
            case 4: /* sep */
                break;
            case 5: /* view playback log */
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
            case 6: /* set defaults */
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
            case 7: /*sep*/
                continue;
            case 8: /* Cancel */
                has_log = false;
                if (crc != rb->crc_32(&gConfig, sizeof(struct scrobbler_cfg), 0xFFFFFFFF))
                {
                    /* there are changes to save */
                    if (!rb->yesno_pop(ID2P(LANG_SAVE_CHANGES)))
                    {
                        return -1;
                    }
                }
            case 9: /* Export & exit */
            {
                res = configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
                if (res >= 0)
                {
                    logf("SCROBBLER: cfg saved %s %d bytes", CFG_FILE, gCfg_sz);
                }
                else
                {
                    logf("SCROBBLER: cfg FAILED (%d) %s", res, CFG_FILE);
                }
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
                if (has_log)
                {
                    rb->cpu_boost(true);
                    return export_scrobbler_file();
                    rb->cpu_boost(false);
                }
#else
                if (has_log)
                    return export_scrobbler_file();
#endif
                return PLUGIN_OK;
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

static inline const char* str_chk_valid(const char *s, const char *alt)
{
    return (s != NULL ? s : alt);
}

static void scrobbler_get_filename(char *path, size_t size)
{
    int used;

    used = rb->snprintf(path, size, "/%s", BASE_FILENAME);

    if (used >= (int)size)
    {
        logf("%s: not enough buffer space for log filename", __func__);
        rb->memset(path, 0, size);
    }
}

static unsigned long scrobbler_get_threshold(unsigned long length_ms)
{
    /* length is assumed to be in miliseconds */
    return length_ms / 100 * gConfig.savepct;
}

static int create_log_entry(struct scrobbler_entry *entry, int output_fd)
{
    #define SEP "\t"
    #define EOL "\n"
    struct mp3entry id3, *id;
    char *path = rb->strrchr(entry->path, '/');
    if (!path)
        path = entry->path;
    else
        path++; /* remove slash */
    char rating = 'S'; /* Skipped */
    if (entry->elapsed >= scrobbler_get_threshold(entry->length))
        rating = 'L'; /* Listened */

#if (CONFIG_RTC)
    unsigned long timestamp = entry->timestamp;
#else
    unsigned long timestamp = 0U;
#endif

    if (!rb->get_metadata(&id3, -1, entry->path))
    {
        /* failure to read metadata not fatal, write what we have */
        rb->fdprintf(output_fd,
                   "%s"SEP"%s"SEP"%s"SEP"%s"SEP"%d"SEP"%c"SEP"%lu"SEP"%s"EOL"",
                   "",
                   "",
                   path,
                   "-1",
                   (int)(entry->length / 1000),
                   rating,
                   timestamp,
                   "");
        return PLUGIN_OK;
    }
    if (!output_fd)
        return PLUGIN_ERROR;
    id = &id3;

    char* artist = id->artist ? id->artist : id->albumartist;

    char tracknum[11] = { "" };

    if (id->tracknum > 0)
        rb->snprintf(tracknum, sizeof (tracknum), "%d", id->tracknum);

    rb->fdprintf(output_fd,
                   "%s"SEP"%s"SEP"%s"SEP"%s"SEP"%d"SEP"%c"SEP"%lu"SEP"%s"EOL"",
                   str_chk_valid(artist, UNTAGGED),
                   str_chk_valid(id->album, ""),
                   str_chk_valid(id->title, path),
                   tracknum,
                   (int)(entry->length / 1000),
                   rating,
                   timestamp,
                   str_chk_valid(id->mb_track_id, ""));
    #undef SEP
    #undef EOL
    return PLUGIN_OK;
}

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
    rb->splashf(100, "Opening %s", plugin);
    if (rb->file_exists(plugin))
    {
        return rb->plugin_open(plugin, "-scrobbler_view_pbl");
    }
    return PLUGIN_ERROR;
}

static int open_create_scrobbler_log(void)
{
    int fd;
    char scrobbler_file[MAX_PATH];

    scrobbler_get_filename(scrobbler_file, sizeof(scrobbler_file));

    /* If the file doesn't exist, create it. */
    if(!rb->file_exists(scrobbler_file))
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
        fd = rb->open(scrobbler_file, O_WRONLY | O_APPEND);

    return fd;
}

static bool playbacklog_parse_entry(struct scrobbler_entry *entry, char *begin)
{
    char *sep;
    memset(entry, 0, sizeof(*entry));

    sep = rb->strchr(begin, ':');
    if (!sep)
        return false;

    entry->timestamp = rb->atoi(begin);

    begin = sep + 1;
    sep = rb->strchr(begin, ':');
    if (!sep)
        return false;

    entry->elapsed = rb->atoi(begin);

    begin = sep + 1;
    sep = rb->strchr(begin, ':');
    if (!sep)
        return false;

    entry->length = rb->atoi(begin);

    begin = sep + 1;
    if (*begin == '\0')
        return false;

    entry->path = begin;

    if (entry->length == 0 || entry->elapsed > entry->length)
    {
        return false;
    }
    return true; /* success */
}

static inline bool pbl_cull_duplicates(int fd, struct scrobbler_entry *curentry,
                                     int cur_line, char*buf, size_t bufsz)
{
    /* child function of remove_duplicates */
    int line_num = cur_line;
    int rd, start_pos, pos;
    struct scrobbler_entry compare;
    pos = rb->lseek(fd, 0, SEEK_CUR);
    while(1)
    {
        if ((rd = rb->read_line(fd, buf, bufsz)) <= 0)
            break; /* EOF */

        /* save start of entry in case we need to remove it */
        start_pos = pos;
        pos += rd;
        line_num++;
        if (buf[0] == '#' || buf[0] == '\0') /* skip comments and empty lines */
            continue;
        if (!playbacklog_parse_entry(&compare, buf))
            continue;

        rb->yield();

        unsigned long length = compare.length;
        if (curentry->length != length
            || rb->strcmp(curentry->path, compare.path) != 0)
            continue; /* different track */

        /* if this is two distinct plays keep both */
        if  ((cur_line + 1 == line_num) /* unless back to back then its probably a resume */
            || (curentry->timestamp <= compare.timestamp + length
            && compare.timestamp <= curentry->timestamp + length))
        {

            if (curentry->elapsed >= compare.elapsed)
            {
                /* compare entry is not the greatest elapsed */
                /*logf("entry %s (%lu) @ %d culled\n", compare.path, compare.elapsed, line_num);*/
                rb->lseek(fd, start_pos, SEEK_SET);
                rb->write(fd, "#", 1); /* make this entry a comment */
                rb->lseek(fd, pos, SEEK_SET);
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

static void playbacklog_remove_duplicates(int fd, char *buf, size_t bufsz)
{
    logf("%s()\n", __func__);
    struct scrobbler_entry entry;
    char tmp_buf[SCROBBLER_MAXENTRY_LEN];
    int start_pos, pos = 0;
    int rd;
    int line_num = 0;
    rb->lseek(fd, 0, SEEK_SET);

    while(1)
    {
        if ((rd = rb->read_line(fd, buf, bufsz)) <= 0)
            break;  /* EOF */

        /* save start of entry in case we need to remove it */
        start_pos = pos;
        pos += rd;
        line_num++;
        if (buf[0] == '#' || buf[0] == '\0') /* skip comments and empty lines */
            continue;
        if (!playbacklog_parse_entry(&entry, buf))
        {
            /*logf("%s failed parsing entry @ %d\n", __func__, line_num);*/
            continue;
        }
        /*logf("current entry %s (%lu) @ %d", entry.path, entry.elapsed, line_num);*/

        if (!pbl_cull_duplicates(fd, &entry, line_num, tmp_buf, sizeof(tmp_buf)))
        {
            rb->lseek(fd, start_pos, SEEK_SET);
            /*logf("entry: %s @ %d is a duplicate", entry.path, line_num);*/
            rb->write(fd, "#", 1); /* make this entry a comment */
        }
        rb->lseek(fd, pos, SEEK_SET);
    }
}

static int export_scrobbler_file(void)
{
    const char* filename = ROCKBOX_DIR "/playback.log";
    rb->splash(0, ID2P(LANG_WAIT));
    static char buf[SCROBBLER_MAXENTRY_LEN];
    struct scrobbler_entry entry;

    int tracks_saved = 0;
    int line_num = 0;
    int rd = 0;

    rb->remove(ROCKBOX_DIR "/playback.old");

    int fd_copy = rb->open(ROCKBOX_DIR "/playback.old", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd_copy < 0)
    {
        logf("Scrobbler Error opening: %s\n", ROCKBOX_DIR "/playback.old");
        rb->splashf(HZ *2, "Scrobbler Error opening: %s", ROCKBOX_DIR "/playback.old");
        return PLUGIN_ERROR;
    }
    rb->add_playbacklog(NULL); /* ensure the log has been flushed */

    /* We don't want any writes while copying and (possibly) deleting the log */
    bool log_enabled = rb->global_settings->playback_log;
    rb->global_settings->playback_log = false;

    int fd = rb->open_utf8(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->global_settings->playback_log = log_enabled; /* restore logging */
        logf("Scrobbler Error opening: %s\n", filename);
        rb->splashf(HZ *2, "Scrobbler Error opening: %s", filename);
        return PLUGIN_ERROR;
    }
    /* copy loop playback.log => playback.old */
    while(rb->read_line(fd, buf, sizeof(buf)) > 0)
    {
        line_num++;
        if (buf[0] == '#' || buf[0] == '\0') /* skip comments and empty lines */
            continue;
        /* parse entry will fail if elapsed > length or other invalid entry */
        if (!playbacklog_parse_entry(&entry, buf))
        {
            logf("%s failed parsing entry @ line: %d\n", __func__, line_num);
            continue;
        }
        /* don't copy entries that do not meet users minimum play length */
        if ((int) entry.elapsed < gConfig.minms)
        {
            logf("Skipping path:'%s' @ line: %d\nelapsed: %ld length: %ld\nmin: %d\n",
             entry.path, line_num, entry.elapsed, entry.length, gConfig.minms);
            continue;
        }
        /* add a space to beginning of every line playbacklog_remove_duplicates
         * will use this to prepend '#' to entries that will be ignored */
        rb->fdprintf(fd_copy, " %s\n", buf);
        tracks_saved++;
    }
    rb->close(fd);
    logf("%s %d tracks copied\n", __func__, tracks_saved);

    if (gConfig.delete_log && tracks_saved > 0)
    {
        rb->remove(filename);
    }
    rb->global_settings->playback_log = log_enabled; /* restore logging */

    if (gConfig.remove_dup && tracks_saved > 0)
        playbacklog_remove_duplicates(fd_copy, buf, sizeof(buf));

    rb->lseek(fd_copy, 0, SEEK_SET);

    tracks_saved = 0;
    int scrobbler_fd = open_create_scrobbler_log();
    line_num = 0;
    while (1)
    {
        if ((rd = rb->read_line(fd_copy, buf, sizeof(buf))) <= 0)
            break;
        line_num++;
        if (buf[0] == '#' || buf[0] == '\0') /* skip comments and empty lines */
            continue;
        if (!playbacklog_parse_entry(&entry, buf))
        {
            logf("%s failed parsing entry @ line: %d\n", __func__, line_num);
            continue;
        }

        logf("Read (%d) @ line: %d: timestamp: %lu\nelapsed: %ld\nlength: %ld\npath: '%s'\n",
             rd, line_num, entry.timestamp, entry.elapsed, entry.length, entry.path);
        int ret = create_log_entry(&entry, scrobbler_fd);
        if (ret == PLUGIN_ERROR)
            goto entry_error;
        tracks_saved++;
        /* process our valid entry */
    }

    logf("%s %d tracks saved", __func__, tracks_saved);
    rb->close(scrobbler_fd);
    rb->close(fd_copy);

    rb->splashf(HZ *2, "%d tracks saved", tracks_saved);

    //ROCKBOX_DIR "/playback.log"

    return PLUGIN_OK;
entry_error:
    if (scrobbler_fd > 0)
        rb->close(scrobbler_fd);
    rb->close(fd_copy);
    return PLUGIN_ERROR;
    (void)line_num;
}

/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(const void* parameter)
{
    bool resume;
    const char * param_str = (const char*) parameter;
    resume = (parameter && param_str[0] == '-' && rb->strcmp(param_str, "-resume") == 0);

    logf("Resume %s", resume ? "YES" : "NO");

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

    return scrobbler_menu(resume);
}
