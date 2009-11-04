/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
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

#include <string.h>
#include "sprintf.h"
#include "system.h"
#include "albumart.h"
#include "metadata.h"
#include "buffering.h"
#include "dircache.h"
#include "misc.h"
#include "settings.h"
#include "wps.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

#if defined(HAVE_JPEG) || defined(PLUGIN)
#define USE_JPEG_COVER
#endif

/* Strip filename from a full path
 *
 * buf      - buffer to extract directory to.
 * buf_size - size of buffer.
 * fullpath - fullpath to extract from.
 *
 * Split the directory part of the given fullpath and store it in buf
 *   (including last '/').
 * The function return parameter is a pointer to the filename
 *   inside the given fullpath.
 */
static char* strip_filename(char* buf, int buf_size, const char* fullpath)
{
    char* sep;
    int   len;

    if (!buf || buf_size <= 0 || !fullpath)
        return NULL;

    /* if 'fullpath' is only a filename return immediately */
    sep = strrchr(fullpath, '/');
    if (sep == NULL)
    {
        buf[0] = 0;
        return (char*)fullpath;
    }

    len = MIN(sep - fullpath + 1, buf_size - 1);
    strlcpy(buf, fullpath, len + 1);
    return (sep + 1);
}

/* Make sure part of path only contain chars valid for a FAT32 long name.
 * Double quotes are replaced with single quotes, other unsupported chars 
 * are replaced with an underscore.
 *
 * path   - path to modify.
 * offset - where in path to start checking.
 * count  - number of chars to check.
 */
static void fix_path_part(char* path, int offset, int count)
{
    static const char invalid_chars[] = "*/:<>?\\|";
    int i;
    
    path += offset;
    
    for (i = 0; i <= count; i++, path++)
    {
        if (*path == 0)
            return;
        if (*path == '"')
            *path = '\'';
        else if (strchr(invalid_chars, *path))
            *path = '_';
    }
}

#ifdef USE_JPEG_COVER
static const char * extensions[] = { "jpeg", "jpg", "bmp" };
static int extension_lens[] = { 4, 3, 3 };
/* Try checking for several file extensions, return true if a file is found and
 * leaving the path modified to include the matching extension.
 */
static bool try_exts(char *path, int len)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        if (extension_lens[i] + len > MAX_PATH)
            continue;
        strcpy(path + len, extensions[i]);
        if (file_exists(path))
            return true;
    }
    return false;
}
#define EXT
#else
#define EXT "bmp"
#define try_exts(path, len) file_exists(path)
#endif

/* Look for the first matching album art bitmap in the following list:
 *  ./<trackname><size>.{jpeg,jpg,bmp}
 *  ./<albumname><size>.{jpeg,jpg,bmp}
 *  ./cover<size>.bmp
 *  ../<albumname><size>.{jpeg,jpg,bmp}
 *  ../cover<size>.{jpeg,jpg,bmp}
 *  ROCKBOX_DIR/albumart/<artist>-<albumname><size>.{jpeg,jpg,bmp}
 * <size> is the value of the size_string parameter, <trackname> and
 * <albumname> are read from the ID3 metadata.
 * If a matching bitmap is found, its filename is stored in buf.
 * Return value is true if a bitmap was found, false otherwise.
 *
 * If the first symbol in size_string is a colon (e.g. ":100x100")
 * then the colon is skipped ("100x100" will be used) and the track
 * specific image (./<trackname><size>.bmp) is tried last instead of first.
 */
bool search_albumart_files(const struct mp3entry *id3, const char *size_string,
                           char *buf, int buflen)
{
    char path[MAX_PATH + 1];
    char dir[MAX_PATH + 1];
    bool found = false;
    int track_first = 1;
    int pass;
    const char *trackname;
    const char *artist;
    int dirlen;
    int albumlen;
    int pathlen;

    if (!id3 || !buf)
        return false;

    trackname = id3->path;

    if (strcmp(trackname, "No file!") == 0)
        return false;

    if (*size_string == ':')
    {
        size_string++;
        track_first = 0;
    }

    strip_filename(dir, sizeof(dir), trackname);
    dirlen = strlen(dir);
    albumlen = id3->album ? strlen(id3->album) : 0;

    for(pass = 0; pass < 2 - track_first; pass++)
    {
        if (track_first || pass)
        {
            /* the first file we look for is one specific to the
               current track */
            strip_extension(path, sizeof(path) - strlen(size_string) - 4,
                            trackname);
            strcat(path, size_string);
            strcat(path, "." EXT);
#ifdef USE_JPEG_COVER
            pathlen = strlen(path);
#endif
            found = try_exts(path, pathlen);
        }
        if (pass)
            break;
        if (!found && albumlen > 0)
        {
            /* if it doesn't exist,
            * we look for a file specific to the track's album name */
            pathlen = snprintf(path, sizeof(path),
                            "%s%s%s." EXT, dir, id3->album, size_string);
            fix_path_part(path, dirlen, albumlen);
            found = try_exts(path, pathlen);
        }

        if (!found)
        {
            /* if it still doesn't exist, we look for a generic file */
            pathlen = snprintf(path, sizeof(path),
                            "%scover%s." EXT, dir, size_string);
            found = try_exts(path, pathlen);
        }

#ifdef USE_JPEG_COVER
        if (!found && !*size_string)
        {
            snprintf (path, sizeof(path), "%sfolder.jpg", dir);
            found = file_exists(path);
        }
#endif

        artist = id3->albumartist != NULL ? id3->albumartist : id3->artist;

        if (!found && artist && id3->album)
        {
            /* look in the albumart subdir of .rockbox */
            pathlen = snprintf(path, sizeof(path),
                            ROCKBOX_DIR "/albumart/%s-%s%s." EXT,
                            artist,
                            id3->album,
                            size_string);
            fix_path_part(path, strlen(ROCKBOX_DIR "/albumart/"), MAX_PATH);
            found = try_exts(path, pathlen);
        }

        if (!found)
        {
            /* if it still doesn't exist,
            * we continue to search in the parent directory */
            strcpy(path, dir);
            path[dirlen - 1] = 0;
            strip_filename(dir, sizeof(dir), path);
            dirlen = strlen(dir);
        }

        /* only try parent if there is one */
        if (dirlen > 0)
        {
            if (!found && albumlen > 0)
            {
                /* we look in the parent directory
                * for a file specific to the track's album name */
                pathlen = snprintf(path, sizeof(path),
                                "%s%s%s." EXT, dir, id3->album, size_string);
                fix_path_part(path, dirlen, albumlen);
                found = try_exts(path, pathlen);
            }

            if (!found)
            {
                /* if it still doesn't exist, we look in the parent directory
                * for a generic file */
                pathlen = snprintf(path, sizeof(path),
                                "%scover%s." EXT, dir, size_string);
                found = try_exts(path, pathlen);
            }
        }
        if (found)
            break;
    }

    if (!found)
        return false;

    strlcpy(buf, path, buflen);
    logf("Album art found: %s", path);
    return true;
}

#ifndef PLUGIN
/* Look for albumart bitmap in the same dir as the track and in its parent dir.
 * Stores the found filename in the buf parameter.
 * Returns true if a bitmap was found, false otherwise */
bool find_albumart(const struct mp3entry *id3, char *buf, int buflen,
                    struct dim *dim)
{
    if (!id3 || !buf)
        return false;

    char size_string[9];
    logf("Looking for album art for %s", id3->path);

    /* Write the size string, e.g. ".100x100". */
    snprintf(size_string, sizeof(size_string), ".%dx%d",
              dim->width, dim->height);

    /* First we look for a bitmap of the right size */
    if (search_albumart_files(id3, size_string, buf, buflen))
        return true;

    /* Then we look for generic bitmaps */
    *size_string = 0;
    return search_albumart_files(id3, size_string, buf, buflen);
}

/* Draw the album art bitmap from the given handle ID onto the given WPS.
   Call with clear = true to clear the bitmap instead of drawing it. */
void draw_album_art(struct gui_wps *gwps, int handle_id, bool clear)
{
    if (!gwps || !gwps->data || !gwps->display || handle_id < 0)
        return;

    struct wps_data *data = gwps->data;
    struct skin_albumart *aa = data->albumart;

    if (!aa)
        return;
        
    struct bitmap *bmp;
    if (bufgetdata(handle_id, 0, (void *)&bmp) <= 0)
        return;

    short x = aa->x;
    short y = aa->y;
    short width = bmp->width;
    short height = bmp->height;

    if (aa->width > 0)
    {
        /* Crop if the bitmap is too wide */
        width = MIN(bmp->width, aa->width);

        /* Align */
        if (aa->xalign & WPS_ALBUMART_ALIGN_RIGHT)
            x += aa->width - width;
        else if (aa->xalign & WPS_ALBUMART_ALIGN_CENTER)
            x += (aa->width - width) / 2;
    }

    if (aa->height > 0)
    {
        /* Crop if the bitmap is too high */
        height = MIN(bmp->height, aa->height);

        /* Align */
        if (aa->yalign & WPS_ALBUMART_ALIGN_BOTTOM)
            y += aa->height - height;
        else if (aa->yalign & WPS_ALBUMART_ALIGN_CENTER)
            y += (aa->height - height) / 2;
    }

    if (!clear)
    {
        /* Draw the bitmap */
        gwps->display->bitmap_part((fb_data*)bmp->data, 0, 0, 
                                    STRIDE(gwps->display->screen_type, 
                                        bmp->width, bmp->height),
                                   x, y, width, height);
#ifdef HAVE_LCD_INVERT
        if (global_settings.invert) {
            gwps->display->set_drawmode(DRMODE_COMPLEMENT);
            gwps->display->fillrect(x, y, width, height);
            gwps->display->set_drawmode(DRMODE_SOLID);
        }
#endif
    }
    else
    {
        /* Clear the bitmap */
        gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        gwps->display->fillrect(x, y, width, height);
        gwps->display->set_drawmode(DRMODE_SOLID);
    }
}

#endif /* PLUGIN */
