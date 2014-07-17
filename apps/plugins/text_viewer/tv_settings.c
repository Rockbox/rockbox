/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "plugin.h"
#include "tv_bookmark.h"
#include "tv_settings.h"

/* global settings file
 * binary file, so dont use .cfg
 *
 * setting file format
 *
 * part               byte count
 * -------------------------------
 * 'TVGS'                 4
 * version                1
 * word_mode              1
 * line_mode              1
 * windows                1 (when version <= 0x32, this value is view_mode)
 * alignment              1
 * encoding               1
 * vertical_scrollbar     1
 * (unused)               1 (for compatibility)
 * overlap_page_mode      1
 * header_mode            1
 * footer_mode            1
 * vertical_scroll_mode   1
 * autoscroll_speed       1
 * horizontal_scrollbar   1
 * horizontal_scroll_mode 1
 * narrow_mode            1
 * indent_spaces          1
 * statusbar              1
 * night_mode             1
 * (reserved)             10
 * font name              MAX_PATH
 */

#define VIEWER_GLOBAL_SETTINGS_FILE      VIEWERS_DATA_DIR "/viewer.dat"
#define TV_GLOBAL_SETTINGS_FILE          VIEWERS_DATA_DIR "/tv_global.dat"

#define TV_GLOBAL_SETTINGS_HEADER        "\x54\x56\x47\x53" /* "TVGS" */
#define TV_GLOBAL_SETTINGS_VERSION       0x39
#define TV_GLOBAL_SETTINGS_HEADER_SIZE   5
#define TV_GLOBAL_SETTINGS_FIRST_VERSION 0x31

/* preferences and bookmarks at each file
 * binary file, so dont use .cfg
 *
 * setting file format
 *
 * part                   byte count
 * -----------------------------------
 * 'TVS'                      3
 * version                    1
 * file count                 2
 * [1st file]
 *   file path                MAX_PATH
 *   next file pos            2 (preferences size + bookmark count * bookmark size + 1)
 *   [preferences]
 *     word_mode              1
 *     line_mode              1
 *     windows                1 (when version <= 0x33, this value is view_mode)
 *     alignment              1
 *     encoding               1
 *     vertical_scrollbar     1
 *     (unused)               1 (for compatibility)
 *     overlap_page_mode      1
 *     header_mode            1
 *     footer_mode            1
 *     vertical_scroll_mode   1
 *     autoscroll_speed       1
 *     horizontal_scrollbar   1
 *     horizontal_scroll_mode 1
 *     narrow_mode            1
 *     indent_spaces          1
 *     statusbar              1
 *     night_mode             1
 *     (reserved)             10
 *     font name              MAX_PATH
 *   bookmark count           1
 *   [1st bookmark]
 *     file_position          4
 *     page                   2
 *     line                   1
 *     flag                   1
 *   [2nd bookmark]
 *   ...
 *   [last bookmark]
 * [2nd file]
 * ...
 * [last file]
 */
#define VIEWER_SETTINGS_FILE      VIEWERS_DATA_DIR "/viewer_file.dat"
#define TV_SETTINGS_FILE          VIEWERS_DATA_DIR "/tv_file.dat"

/* temporary file */
#define TV_SETTINGS_TMP_FILE      VIEWERS_DATA_DIR "/tv_file.tmp"

#define TV_SETTINGS_HEADER        "\x54\x56\x53" /* "TVS" */
#define TV_SETTINGS_VERSION       0x3A
#define TV_SETTINGS_HEADER_SIZE   4
#define TV_SETTINGS_FIRST_VERSION 0x32

#define TV_PREFERENCES_SIZE       (28 + MAX_PATH)
#define TV_MAX_FILE_RECORD_SIZE   (MAX_PATH+2 + TV_PREFERENCES_SIZE + TV_MAX_BOOKMARKS*SERIALIZE_BOOKMARK_SIZE+1)

static off_t stored_preferences_offset = 0;
static int stored_preferences_size = 0;

/* ----------------------------------------------------------------------------
 * read/write the preferences
 * ----------------------------------------------------------------------------
 */

static bool tv_read_preferences(int pfd, int version, struct tv_preferences *prefs)
{
    unsigned char buf[TV_PREFERENCES_SIZE];
    const unsigned char *p = buf;
    int read_size = TV_PREFERENCES_SIZE;

    if (version == 0)
        read_size -= 17;
    else if (version == 1)
        read_size -= 16;

    if (rb->read(pfd, buf, read_size) < 0)
        return false;

    prefs->word_mode = *p++;
    prefs->line_mode = *p++;

    prefs->windows = *p++;
    if (version <= 1)
        prefs->windows++;

    if (version > 0)
        prefs->alignment = *p++;
    else
        prefs->alignment = AL_LEFT;

    prefs->encoding           = *p++;
    prefs->vertical_scrollbar = (*p++ != 0);
    /* skip need_scrollbar */
    p++;
    prefs->overlap_page_mode  = (*p++ != 0);

    if (version < 7)
    {
        prefs->statusbar = false;
        if (*p > 1)
        {
            prefs->header_mode = ((*p & 1) != 0);
            prefs->statusbar   = true;
        }
        else
            prefs->header_mode = (*p != 0);

        if (*(++p) > 1)
        {
            prefs->footer_mode = ((*p & 1) != 0);
            prefs->statusbar   = true;
        }
        else
            prefs->footer_mode = (*p != 0);

        p++;
    }
    else
    {
        prefs->header_mode = (*p++ != 0);
        prefs->footer_mode = (*p++ != 0);
    }

    prefs->vertical_scroll_mode = *p++;
    prefs->autoscroll_speed     = *p++;

    if (version > 2)
        prefs->horizontal_scrollbar = (*p++ != 0);
    else
        prefs->horizontal_scrollbar = false;

    if (version > 3)
        prefs->horizontal_scroll_mode = *p++;
    else
        prefs->horizontal_scroll_mode = HS_SCREEN;

    if (version > 4)
        prefs->narrow_mode = *p++;
    else
        prefs->narrow_mode = NM_PAGE;

    if (version > 5)
        prefs->indent_spaces = *p++;
    else
        prefs->indent_spaces = 2;

    if (version > 6)
        prefs->statusbar = (*p++ != 0);
    if (version > 6)
        prefs->night_mode = (*p++ != 0);

    rb->strlcpy(prefs->font_name, buf + read_size - MAX_PATH, MAX_PATH);

    prefs->font_id = rb->global_status->font_id[SCREEN_MAIN];

    return true;
}

static void tv_serialize_preferences(unsigned char *buf, const struct tv_preferences *prefs)
{
    unsigned char *p = buf;

    rb->memset(buf, 0, TV_PREFERENCES_SIZE);
    *p++ = prefs->word_mode;
    *p++ = prefs->line_mode;
    *p++ = prefs->windows;
    *p++ = prefs->alignment;
    *p++ = prefs->encoding;
    *p++ = prefs->vertical_scrollbar;
    /* skip need_scrollbar */
    p++;
    *p++ = prefs->overlap_page_mode;
    *p++ = prefs->header_mode;
    *p++ = prefs->footer_mode;
    *p++ = prefs->vertical_scroll_mode;
    *p++ = prefs->autoscroll_speed;
    *p++ = prefs->horizontal_scrollbar;
    *p++ = prefs->horizontal_scroll_mode;
    *p++ = prefs->narrow_mode;
    *p++ = prefs->indent_spaces;
    *p++ = prefs->statusbar;
    *p++ = prefs->night_mode;

    rb->strlcpy(buf + 28, prefs->font_name, MAX_PATH);
}

static bool tv_write_preferences(int pfd, const struct tv_preferences *prefs)
{
    unsigned char buf[TV_PREFERENCES_SIZE];
    
    tv_serialize_preferences(buf, prefs);

    return (rb->write(pfd, buf, TV_PREFERENCES_SIZE) >= 0);
}

/* ----------------------------------------------------------------------------
 * convert vewer.rock's settings file to text_viewer.rock's settings file
 * ----------------------------------------------------------------------------
 */

static bool tv_convert_settings(int sfd, int dfd, int old_ver)
{
    struct tv_preferences new_prefs;
    off_t old_pos;
    off_t new_pos;
    unsigned char buf[MAX_PATH + 2];
    int settings_size;

    rb->read(sfd, buf, MAX_PATH + 2);
    rb->write(dfd, buf, MAX_PATH + 2);

    settings_size = (buf[MAX_PATH] << 8) | buf[MAX_PATH + 1];

    old_pos = rb->lseek(sfd, 0, SEEK_CUR);
    new_pos = rb->lseek(dfd, 0, SEEK_CUR);

    /*
     * when the settings size != preferences size + bookmarks size,
     * settings data are considered to be old version.
     */
    if (old_ver > 0 && ((settings_size - TV_PREFERENCES_SIZE) % 8) == 0)
        old_ver = 0;

    if (!tv_read_preferences(sfd, old_ver, &new_prefs))
        return false;

    if (!tv_write_preferences(dfd, &new_prefs))
        return false;

    settings_size -= (rb->lseek(sfd, 0, SEEK_CUR) - old_pos);

    if (settings_size > 0)
    {
        rb->read(sfd, buf, settings_size);
        rb->write(dfd, buf, settings_size);
    }

    settings_size = rb->lseek(dfd, 0, SEEK_CUR) - new_pos;
    buf[0] = settings_size >> 8;
    buf[1] = settings_size;
    rb->lseek(dfd, new_pos - 2, SEEK_SET);
    rb->write(dfd, buf, 2);
    rb->lseek(dfd, settings_size, SEEK_CUR);
    return true;
}

static void tv_convert_settings_file(void)
{
    unsigned char buf[TV_SETTINGS_HEADER_SIZE + 2];
    int sfd;
    int tfd;
    int i;
    int fcount;
    int version;
    bool res = false;

    if ((sfd = rb->open(VIEWER_SETTINGS_FILE, O_RDONLY)) < 0)
        return;

    if ((tfd = rb->open(TV_SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
    {
        rb->close(sfd);
        return;
    }

    if (rb->read(sfd, buf, TV_SETTINGS_HEADER_SIZE + 2) >= 0)
    {
        version = buf[TV_SETTINGS_HEADER_SIZE - 1] - TV_SETTINGS_FIRST_VERSION;
        fcount  = (buf[TV_SETTINGS_HEADER_SIZE] << 8) | buf[TV_SETTINGS_HEADER_SIZE + 1];
        buf[TV_SETTINGS_HEADER_SIZE - 1] = TV_SETTINGS_VERSION;

        if (rb->write(tfd, buf, TV_SETTINGS_HEADER_SIZE + 2) >= 0)
        {
            res = true;
            for (i = 0; i < fcount; i++)
            {
                if (!tv_convert_settings(sfd, tfd, version))
                {
                    res = false;
                    break;
                }
            }
        }
    }

    rb->close(sfd);
    rb->close(tfd);

    if (res)
        rb->rename(TV_SETTINGS_TMP_FILE, TV_SETTINGS_FILE);
    else
        rb->remove(TV_SETTINGS_TMP_FILE);

    return;
}

/* ----------------------------------------------------------------------------
 * load/save the global settings
 * ----------------------------------------------------------------------------
 */

bool tv_load_global_settings(struct tv_preferences *prefs)
{
    unsigned char buf[TV_GLOBAL_SETTINGS_HEADER_SIZE];
    int fd;
    int version;
    bool res = false;

    /*
     * the viewer.rock's setting file read when the text_viewer.rock's setting file
     * does not read.
     */
    if ((fd = rb->open(TV_GLOBAL_SETTINGS_FILE, O_RDONLY)) < 0)
        fd = rb->open(VIEWER_GLOBAL_SETTINGS_FILE, O_RDONLY);

    if (fd >= 0)
    {
        if ((rb->read(fd, buf, TV_GLOBAL_SETTINGS_HEADER_SIZE) > 0) &&
            (rb->memcmp(buf, TV_GLOBAL_SETTINGS_HEADER, TV_GLOBAL_SETTINGS_HEADER_SIZE - 1) == 0))
        {
            version = buf[TV_GLOBAL_SETTINGS_HEADER_SIZE - 1] - TV_GLOBAL_SETTINGS_FIRST_VERSION;
            res = tv_read_preferences(fd, version, prefs);
        }
        rb->close(fd);
    }
    return res;
}

bool tv_save_global_settings(const struct tv_preferences *prefs)
{
    unsigned char buf[TV_GLOBAL_SETTINGS_HEADER_SIZE];
    int fd;
    bool res;

    if ((fd = rb->open(TV_SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
         return false;

    rb->memcpy(buf, TV_GLOBAL_SETTINGS_HEADER, TV_GLOBAL_SETTINGS_HEADER_SIZE - 1);
    buf[TV_GLOBAL_SETTINGS_HEADER_SIZE - 1] = TV_GLOBAL_SETTINGS_VERSION;

    res = (rb->write(fd, buf, TV_GLOBAL_SETTINGS_HEADER_SIZE) >= 0) &&
          (tv_write_preferences(fd, prefs));
    rb->close(fd);

    if (res)
    {
        rb->remove(TV_GLOBAL_SETTINGS_FILE);
        rb->rename(TV_SETTINGS_TMP_FILE, TV_GLOBAL_SETTINGS_FILE);
    }
    else
        rb->remove(TV_SETTINGS_TMP_FILE);

    return res;
}

/* ----------------------------------------------------------------------------
 * load/save the settings
 * ----------------------------------------------------------------------------
 */

bool tv_load_settings(const unsigned char *file_name)
{
    unsigned char buf[MAX_PATH+2];
    unsigned int fcount;
    unsigned int i;
    bool res = false;
    int fd;
    int version;
    unsigned int size;
    struct tv_preferences prefs;
    off_t current_pref_offset;

    if (!rb->file_exists(TV_SETTINGS_FILE))
        tv_convert_settings_file();

    if ((fd = rb->open(TV_SETTINGS_FILE, O_RDONLY)) >= 0)
    {
        if ((rb->read(fd, buf, TV_SETTINGS_HEADER_SIZE + 2) >= 0) &&
            (rb->memcmp(buf, TV_SETTINGS_HEADER, TV_SETTINGS_HEADER_SIZE - 1) == 0))
        {
            version = buf[TV_SETTINGS_HEADER_SIZE - 1] - TV_SETTINGS_FIRST_VERSION;
            fcount = (buf[TV_SETTINGS_HEADER_SIZE] << 8) | buf[TV_SETTINGS_HEADER_SIZE+1];
            
            current_pref_offset = rb->lseek(fd, 0, SEEK_CUR);

            for (i = 0; i < fcount; i++)
            {
                if (rb->read(fd, buf, MAX_PATH+2) >= 0)
                {
                    size = (buf[MAX_PATH] << 8) | buf[MAX_PATH+1];
                    if (rb->strcmp(buf, file_name) == 0)
                    {
                        if (tv_read_preferences(fd, version, &prefs))
                            res = tv_deserialize_bookmarks(fd);
                        
                        if (res) {
                            stored_preferences_offset = current_pref_offset;
                            stored_preferences_size = size;
                        }

                        break;
                    }
                    current_pref_offset = rb->lseek(fd, size, SEEK_CUR);
                }
            }
            rb->close(fd);
        }
        else
        {
            /* when the settings file is illegal, removes it */
            rb->close(fd);
            rb->remove(TV_SETTINGS_FILE);
        }
    }
    if (!res)
    {
        /* specifications are acquired from the global settings */
        if (!tv_load_global_settings(&prefs))
            tv_set_default_preferences(&prefs);
    }
    rb->strlcpy(prefs.file_name, file_name, MAX_PATH);
    return tv_set_preferences(&prefs);
}

bool tv_save_settings(void)
{
    unsigned char buf[TV_MAX_FILE_RECORD_SIZE];
    unsigned char preferences_buf[TV_MAX_FILE_RECORD_SIZE];
    unsigned int fcount = 0;
    unsigned int new_fcount = 0;
    unsigned int i;
    int ofd = -1;
    int tfd;
    off_t size;
    off_t preferences_buf_size;
    bool res = true;

    /* add reading page to bookmarks */
    tv_create_system_bookmark();

    /* storing preferences record in memory */
    rb->memset(preferences_buf, 0, MAX_PATH);
    rb->strlcpy(preferences_buf, preferences->file_name, MAX_PATH);
    preferences_buf_size = MAX_PATH + 2;

    tv_serialize_preferences(preferences_buf + preferences_buf_size, preferences);
    preferences_buf_size += TV_PREFERENCES_SIZE;
    preferences_buf_size += tv_serialize_bookmarks(preferences_buf + preferences_buf_size);
    size = preferences_buf_size - (MAX_PATH + 2);
    preferences_buf[MAX_PATH + 0] = size >> 8;
    preferences_buf[MAX_PATH + 1] = size;

    
    /* Just overwrite preferences if possible*/
    if ( (stored_preferences_offset > 0) && (stored_preferences_size == size) )
    {
        DEBUGF("Saving preferences: overwriting\n");
        if ((tfd = rb->open(TV_SETTINGS_FILE, O_WRONLY)) < 0)
            return false;
        rb->lseek(tfd, stored_preferences_offset, SEEK_SET);
        res = (rb->write(tfd, preferences_buf, preferences_buf_size) >= 0);
        rb->close(tfd);
        return res;
    }


    if (!rb->file_exists(TV_SETTINGS_FILE))
        tv_convert_settings_file();

    
    /* Try appending preferences */
    if ( (stored_preferences_offset == 0) &&
         ( (tfd = rb->open(TV_SETTINGS_FILE, O_RDWR)) >= 0) )
    {
        DEBUGF("Saving preferences: appending\n");
        rb->lseek(tfd, 0, SEEK_END);
        if (rb->write(tfd, preferences_buf, preferences_buf_size) < 0)
            return false;

        rb->lseek(tfd, TV_SETTINGS_HEADER_SIZE, SEEK_SET);
        rb->read(tfd, buf, 2);
        fcount = (buf[0] << 8) | buf[1];
        fcount ++;
        buf[0] = fcount >> 8;
        buf[1] = fcount;
        rb->lseek(tfd, TV_SETTINGS_HEADER_SIZE, SEEK_SET);
        res = rb->write(tfd, buf, 2) >= 0;

        rb->close(tfd);
        return res;
    }

    /* create header for the temporary file */
    rb->memcpy(buf, TV_SETTINGS_HEADER, TV_SETTINGS_HEADER_SIZE - 1);
    buf[TV_SETTINGS_HEADER_SIZE - 1] = TV_SETTINGS_VERSION;

    if ((tfd = rb->open(TV_SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
        return false;

    if (rb->write(tfd, buf, TV_SETTINGS_HEADER_SIZE + 2) < 0)
    {
        rb->close(tfd);
        return false;
    }

    if ((ofd = rb->open(TV_SETTINGS_FILE, O_RDONLY)) >= 0)
    {
        res = ((rb->read(ofd, buf, TV_SETTINGS_HEADER_SIZE + 2) >= 0) &&
               (rb->memcmp(buf, TV_SETTINGS_HEADER, TV_SETTINGS_HEADER_SIZE - 1) == 0));

        if (res)
        {
            fcount = (buf[TV_SETTINGS_HEADER_SIZE] << 8) | buf[TV_SETTINGS_HEADER_SIZE + 1];
            for (i = 0; i < fcount; i++)
            {
                if (rb->read(ofd, buf, MAX_PATH + 2) < 0)
                {
                    res = false;
                    break;
                }

                size = (buf[MAX_PATH] << 8) | buf[MAX_PATH + 1];
                if (rb->strcmp(buf, preferences->file_name) == 0)
                    rb->lseek(ofd, size, SEEK_CUR);
                else
                {
                    if ((rb->read(ofd, buf + (MAX_PATH + 2), size) < 0) ||
                        (rb->write(tfd, buf, size + MAX_PATH + 2) < 0))
                    {
                        res = false;
                        break;
                    }
                    new_fcount++;
                }
            }
        }
        rb->close(ofd);
    }

    if (res)
    {
        /* save to current read file's preferences and bookmarks */
        res = false;

        if (rb->write(tfd, preferences_buf, preferences_buf_size) >= 0)
        {
            rb->lseek(tfd, TV_SETTINGS_HEADER_SIZE, SEEK_SET);

            new_fcount++;
            buf[0] = new_fcount >> 8;
            buf[1] = new_fcount;
            res = (rb->write(tfd, buf, 2) >= 0);
        }
    }
    rb->close(tfd);

    if (res)
    {
        rb->remove(TV_SETTINGS_FILE);
        rb->rename(TV_SETTINGS_TMP_FILE, TV_SETTINGS_FILE);
    }
    else
        rb->remove(TV_SETTINGS_TMP_FILE);

    return res;
}
