/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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

#ifndef __PATHS_H__
#define __PATHS_H__

#include <stdbool.h>
#include "autoconf.h"
#include "string-extra.h"


/* name of directory where configuration, fonts and other data
 * files are stored */
#ifdef __PCTOOL__
#undef WPS_DIR
#else

/* ROCKBOX_DIR is now defined in autoconf.h for flexible build types */
#ifndef ROCKBOX_DIR
#error ROCKBOX_DIR not defined (should be in autoconf.h)
#endif
#define ROCKBOX_DIR_LEN (sizeof(ROCKBOX_DIR)-1)
#endif /* def __PCTOOL__ */

/* NOTE:  target-specific hosted HOME_DIR resides in filesystem-app.c */
#if !defined(APPLICATION) || defined(SAMSUNG_YPR0) || defined(SAMSUNG_YPR1) || \
    defined(DX50) || defined(DX90) || defined(SONY_NWZ_LINUX) || \
    defined(HIBY_LINUX) || defined(FIIO_M3K)

#define HOME_DIR "/"

/* make sure both are the same for native builds */
#undef ROCKBOX_LIBRARY_PATH
#define ROCKBOX_LIBRARY_PATH ROCKBOX_DIR

#define PLUGIN_DIR          ROCKBOX_DIR "/rocks"
#define CODECS_DIR          ROCKBOX_DIR "/codecs"

#else /* APPLICATION */

#define HOME_DIR "<HOME>" /* replaced at runtime */
#define HAVE_SPECIAL_DIRS

#define PLUGIN_DIR          ROCKBOX_LIBRARY_PATH "/rockbox/rocks"
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
#define CODECS_DIR          ROCKBOX_BINARY_PATH
#else
#define CODECS_DIR          ROCKBOX_LIBRARY_PATH "/rockbox/codecs"
#endif

#endif /* !APPLICATION || SAMSUNG_YPR0 */

#define REC_BASE_DIR        HOME_DIR "/Recordings"
#define PLAYLIST_CATALOG_DEFAULT_DIR HOME_DIR "/Playlists"

#define LANG_DIR            ROCKBOX_DIR "/langs"

#define PLUGIN_GAMES_DIR    PLUGIN_DIR "/games"
#define PLUGIN_APPS_DIR     PLUGIN_DIR "/apps"
#define PLUGIN_DEMOS_DIR    PLUGIN_DIR "/demos"
#define VIEWERS_DIR         PLUGIN_DIR "/viewers"

#if defined(APPLICATION) && \
        !(defined(SAMSUNG_YPR0) || defined(SAMSUNG_YPR1) || \
          defined(DX50) || defined(DX90) || defined(SONY_NWZ_LINUX) || defined(HIBY_LINUX) || defined(FIIO_M3K))

#define PLUGIN_DATA_DIR          ROCKBOX_DIR "/rocks.data"
#define PLUGIN_GAMES_DATA_DIR    PLUGIN_DATA_DIR
#define PLUGIN_APPS_DATA_DIR     PLUGIN_DATA_DIR
#define PLUGIN_DEMOS_DATA_DIR    PLUGIN_DATA_DIR
#define VIEWERS_DATA_DIR         PLUGIN_DATA_DIR
#else
#define PLUGIN_DATA_DIR          PLUGIN_DIR
#define PLUGIN_GAMES_DATA_DIR    PLUGIN_DIR "/games"
#define PLUGIN_APPS_DATA_DIR     PLUGIN_DIR "/apps"
#define PLUGIN_DEMOS_DATA_DIR    PLUGIN_DIR "/demos"
#define VIEWERS_DATA_DIR         PLUGIN_DIR "/viewers"
#endif

#define WPS_DIR             ROCKBOX_DIR "/wps"
#define SBS_DIR             WPS_DIR
#define THEME_DIR           ROCKBOX_DIR "/themes"
#define FONT_DIR            ROCKBOX_DIR "/fonts"
#define ICON_DIR            ROCKBOX_DIR "/icons"

#define BACKDROP_DIR        ROCKBOX_DIR "/backdrops"
#define EQS_DIR             ROCKBOX_DIR "/eqs"

/* need to fix this once the application gets record/radio abilities */
#define RECPRESETS_DIR      ROCKBOX_DIR "/recpresets"
#define FMPRESET_PATH       ROCKBOX_DIR "/fmpresets"

#define DIRCACHE_FILE       ROCKBOX_DIR "/dircache.dat"
#define CODEPAGE_DIR        ROCKBOX_DIR "/codepages"

#define VIEWERS_CONFIG      ROCKBOX_DIR "/viewers.config"
#define CONFIGFILE          ROCKBOX_DIR "/config.cfg"
#define FIXEDSETTINGSFILE   ROCKBOX_DIR "/fixed.cfg"

#define PLAYLIST_CONTROL_FILE   ROCKBOX_DIR "/.playlist_control"
#define NVRAM_FILE              ROCKBOX_DIR "/nvram.bin"
#define GLYPH_CACHE_FILE        ROCKBOX_DIR "/.glyphcache"

#endif /* __PATHS_H__ */
