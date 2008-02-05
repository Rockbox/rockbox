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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>
#include "sprintf.h"
#include "system.h"
#include "albumart.h"
#include "id3.h"
#include "gwps.h"
#include "buffering.h"
#include "dircache.h"
#include "debug.h"
#include "misc.h"
#include "settings.h"


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
    strncpy(buf, fullpath, len);
    buf[len] = 0;
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

/* Look for the first matching album art bitmap in the following list:
 *  ./<trackname><size>.bmp
 *  ./<albumname><size>.bmp
 *  ./cover<size>.bmp
 *  ../<albumname><size>.bmp
 *  ../cover<size>.bmp
 *  ROCKBOX_DIR/albumart/<artist>-<albumname><size>.bmp
 * <size> is the value of the size_string parameter, <trackname> and
 * <albumname> are read from the ID3 metadata.
 * If a matching bitmap is found, its filename is stored in buf.
 * Return value is true if a bitmap was found, false otherwise.
 */
bool search_albumart_files(const struct mp3entry *id3, const char *size_string,
                           char *buf, int buflen)
{
    char path[MAX_PATH + 1];
    char dir[MAX_PATH + 1];
    bool found = false;
    const char *trackname;
    int dirlen;
    int albumlen;

    if (!id3 || !buf)
        return false;

    trackname = id3->path;

    if (strcmp(trackname, "No file!") == 0)
        return false;

    strip_filename(dir, sizeof(dir), trackname);
    dirlen = strlen(dir);
    albumlen = id3->album ? strlen(id3->album) : 0;

    /* the first file we look for is one specific to the track playing */
    strip_extension(trackname, path);
    strcat(path, size_string);
    strcat(path, ".bmp");
    found = file_exists(path);
    if (!found && albumlen > 0)
    {
        /* if it doesn't exist,
         * we look for a file specific to the track's album name */
        snprintf(path, sizeof(path),
                 "%s%s%s.bmp", dir, id3->album, size_string);
        fix_path_part(path, dirlen, albumlen);
        found = file_exists(path);
    }

    if (!found)
    {
        /* if it still doesn't exist, we look for a generic file */
        snprintf(path, sizeof(path),
                 "%scover%s.bmp", dir, size_string);
        found = file_exists(path);
    }

    if (!found && id3->artist && id3->album)
    {
        /* look in the albumart subdir of .rockbox */
        snprintf(path, sizeof(path),
                ROCKBOX_DIR "/albumart/%s-%s%s.bmp",
                id3->artist,
                id3->album,
                size_string);
        fix_path_part(path, strlen(ROCKBOX_DIR "/albumart/"), MAX_PATH);
        found = file_exists(path);
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
            snprintf(path, sizeof(path),
                     "%s%s%s.bmp", dir, id3->album, size_string);
            fix_path_part(path, dirlen, albumlen);
            found = file_exists(path);
        }
    
        if (!found)
        {
            /* if it still doesn't exist, we look in the parent directory
             * for a generic file */
            snprintf(path, sizeof(path),
                     "%scover%s.bmp", dir, size_string);
            found = file_exists(path);
        }
    }

    if (!found)
        return false;

    strncpy(buf, path, buflen);
    DEBUGF("Album art found: %s\n", path);
    return true;
}

/* Look for albumart bitmap in the same dir as the track and in its parent dir.
 * Stores the found filename in the buf parameter.
 * Returns true if a bitmap was found, false otherwise */
bool find_albumart(const struct mp3entry *id3, char *buf, int buflen)
{
    if (!id3 || !buf)
        return false;

    char size_string[9];
    struct wps_data *data = gui_wps[0].data;

    if (!data)
        return false;

    DEBUGF("Looking for album art for %s\n", id3->path);

    /* Write the size string, e.g. ".100x100". */
    snprintf(size_string, sizeof(size_string), ".%dx%d",
             data->albumart_max_width, data->albumart_max_height);

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

#ifdef HAVE_REMOTE_LCD
    /* No album art on RWPS */
    if (data->remote_wps)
        return;
#endif

    struct bitmap *bmp;
    if (bufgetdata(handle_id, 0, (void *)&bmp) <= 0)
        return;

    short x = data->albumart_x;
    short y = data->albumart_y;
    short width = bmp->width;
    short height = bmp->height;

    if (data->albumart_max_width > 0)
    {
        /* Crop if the bitmap is too wide */
        width = MIN(bmp->width, data->albumart_max_width);

        /* Align */
        if (data->albumart_xalign & WPS_ALBUMART_ALIGN_RIGHT)
            x += data->albumart_max_width - width;
        else if (data->albumart_xalign & WPS_ALBUMART_ALIGN_CENTER)
            x += (data->albumart_max_width - width) / 2;
    }

    if (data->albumart_max_height > 0)
    {
        /* Crop if the bitmap is too high */
        height = MIN(bmp->height, data->albumart_max_height);

        /* Align */
        if (data->albumart_yalign & WPS_ALBUMART_ALIGN_BOTTOM)
            y += data->albumart_max_height - height;
        else if (data->albumart_yalign & WPS_ALBUMART_ALIGN_CENTER)
            y += (data->albumart_max_height - height) / 2;
    }

    if (!clear)
    {
        /* Draw the bitmap */
        gwps->display->set_drawmode(DRMODE_FG);
        gwps->display->bitmap_part((fb_data*)bmp->data, 0, 0, bmp->width,
                                   x, y, width, height);
        gwps->display->set_drawmode(DRMODE_SOLID);
    }
    else
    {
        /* Clear the bitmap */
        gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        gwps->display->fillrect(x, y, width, height);
        gwps->display->set_drawmode(DRMODE_SOLID);
    }
}
