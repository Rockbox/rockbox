/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#include <ctype.h>
#include "system.h"
#include "pathfuncs.h"
#include "string-extra.h"

#ifdef HAVE_MULTIVOLUME
#include <stdio.h>
#include "storage.h"

enum storage_name_dec_indexes
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    STORAGE_DEC_IDX_ATA,
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
    STORAGE_DEC_IDX_MMC,
#endif
#if (CONFIG_STORAGE & STORAGE_SD)
    STORAGE_DEC_IDX_SD,
#endif
#if (CONFIG_STORAGE & STORAGE_NAND)
    STORAGE_DEC_IDX_NAND,
#endif
#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    STORAGE_DEC_IDX_RAMDISK,
#endif
#if (CONFIG_STORAGE & STORAGE_HOSTFS)
    STORAGE_DEC_IDX_HOSTFS,
#endif
    STORAGE_NUM_DEC_IDX,
};

static const char * const storage_dec_names[STORAGE_NUM_DEC_IDX+1] =
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    [STORAGE_DEC_IDX_ATA]     = ATA_VOL_DEC,
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
    [STORAGE_DEC_IDX_MMC]     = MMC_VOL_DEC,
#endif
#if (CONFIG_STORAGE & STORAGE_SD)
    [STORAGE_DEC_IDX_SD]      = SD_VOL_DEC,
#endif
#if (CONFIG_STORAGE & STORAGE_NAND)
    [STORAGE_DEC_IDX_NAND]    = NAND_VOL_DEC,
#endif
#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    [STORAGE_DEC_IDX_RAMDISK] = RAMDISK_VOL_DEC,
#endif
#if (CONFIG_STORAGE & STORAGE_HOSTFS)
    [STORAGE_DEC_IDX_HOSTFS]  = HOSTFS_VOL_DEC,
#endif
    [STORAGE_NUM_DEC_IDX]     = DEFAULT_VOL_DEC,
};

static const unsigned char storage_dec_indexes[STORAGE_NUM_TYPES+1] =
{
    [0 ... STORAGE_NUM_TYPES] = STORAGE_NUM_DEC_IDX,
#if (CONFIG_STORAGE & STORAGE_ATA)
    [STORAGE_ATA_NUM]     = STORAGE_DEC_IDX_ATA,
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
    [STORAGE_MMC_NUM]     = STORAGE_DEC_IDX_MMC,
#endif
#if (CONFIG_STORAGE & STORAGE_SD)
    [STORAGE_SD_NUM]      = STORAGE_DEC_IDX_SD,
#endif
#if (CONFIG_STORAGE & STORAGE_NAND)
    [STORAGE_NAND_NUM]    = STORAGE_DEC_IDX_NAND,
#endif
#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    [STORAGE_RAMDISK_NUM] = STORAGE_DEC_IDX_RAMDISK,
#endif
#if (CONFIG_STORAGE & STORAGE_HOSTFS)
    [STORAGE_HOSTFS_NUM]  = STORAGE_DEC_IDX_HOSTFS,
#endif
};

/* Returns on which volume this is and sets *nameptr to the portion of the
 * path after the volume specifier, which could be the null if the path is
 * just a volume root. If *nameptr > name, then a volume specifier was
 * found. If 'greedy' is 'true', then it all separators after the volume
 * specifier are consumed, if one was found.
 */
int path_strip_volume(const char *name, const char **nameptr, bool greedy)
{
    int volume = 0;
    const char *t = name;
    int c, v = 0;

    /* format: "/<xxx##>/foo/bar"
     * the "xxx" is pure decoration; only an unbroken trailing string of
     * digits within the brackets is parsed as the volume number and of
     * those, only the last ones VOL_MUM_MAX allows.
     */
    c = *(t = GOBBLE_PATH_SEPCH(t)); /* skip all leading slashes */
    if (c != VOL_START_TOK)     /* missing start token? no volume */
        goto volume0;

    do
    {
        switch (c)
        {
        case '0' ... '9':       /* digit; parse volume number */
            v = (v * 10 + c - '0') % VOL_NUM_MAX;
            break;
        case '\0':
        case PATH_SEPCH:        /* no closing bracket; no volume */
            goto volume0;
        default:                /* something else; reset volume */
            v = 0;
        }
    }
    while ((c = *++t) != VOL_END_TOK);  /* found end token? */

    if (!(c = *++t))            /* no more path and no '/' is ok */
        ;
    else if (c != PATH_SEPCH)   /* more path and no separator after end */
        goto volume0;
    else if (greedy)
        t = GOBBLE_PATH_SEPCH(++t); /* strip remaining separators */

    /* if 'greedy' is true and **nameptr == '\0' then it's only a volume
       root whether or not it has trailing separators */

    volume = v;
    name = t;
volume0:
    if (nameptr)
        *nameptr = name;
    return volume;
}

/* Returns the volume specifier decorated with the storage type name.
 * Assumes the supplied buffer size is at least {VOL_MAX_LEN}+1.
 */
int get_volume_name(int volume, char *buffer)
{
    if (volume < 0)
    {
        *buffer = '\0';
        return 0;
    }

    volume %= VOL_NUM_MAX; /* as path parser would have it */

    int type = storage_driver_type(volume_drive(volume));
    if (type < 0 || type > STORAGE_NUM_TYPES)
        type = STORAGE_NUM_TYPES;

    const char *voldec = storage_dec_names[storage_dec_indexes[type]];
    return snprintf(buffer, VOL_MAX_LEN + 1, "%c%s%d%c",
                    VOL_START_TOK, voldec, volume, VOL_END_TOK);
}
#endif /* HAVE_MULTIVOLUME */

/* Just like path_strip_volume() but strips a leading drive specifier and
 * returns the drive number (A=0, B=1, etc.). -1 means no drive was found.
 * If 'greedy' is 'true', all separators after the volume are consumed.
 */
int path_strip_drive(const char *name, const char **nameptr, bool greedy)
{
    int c = toupper(*name);

    if (c >= 'A' && c <= 'Z' && name[1] == PATH_DRVSEPCH)
    {
        name = &name[2];
        if (greedy)
            name = GOBBLE_PATH_SEPCH(name);

        *nameptr = name;
        return c - 'A';
    }

    *nameptr = name;
    return -1;
}

/* Strips leading and trailing whitespace from a path (anything <= ' ')
 * Returns the first non-whitespace character in *nameptr and the length
 * up to the next whitespace character or null. Returns the null and length
 * 0 if the string is only whitespace.
 */
size_t path_trim_whitespace(const char *name, const char **nameptr)
{
    int c;
    while ((c = *name) <= ' ' && c)
        ++name;

    const char *first = name;
    const char *last = name;

    while (1)
    {
        if (c < ' ')
        {
            *nameptr = first;
            return last - first;
        }

        while ((c = *++name) > ' ');
        last = name;
        while (c == ' ') c = *++name;
    }
}

/* Similar to basename() but always returns a pointer into the path string
 * and never uses static storage. Returns the length of the last component
 * and *nameptr is set to point to its first character.
 * Also pays no mind to '/' vs. '//'. */
size_t path_basename(const char *path, const char **basename)
{
    const char *p = path;
    size_t length = 0;

    while (*(p = GOBBLE_PATH_SEPCH(p)))
    {
        path = p;
        p = GOBBLE_PATH_COMP(++p);
        length = p - path;
    }

    if (length == 0 && p > path)
    {
        /* root - return last slash */
        path = p - 1;
        length = 1;
    }
    /* else path is an empty string */

    *basename = path;
    return length;
}

/* Strips the basename from the path but will not strip the root if it's either
 * just a system root or a volume specifier. Returns the length up to the last
 * component and *nameptr is set to point to the first character of it. Will
 * leave the separators before the basename intact.
 * i.e. "/foo/bar" becomes "/foo/"
 */
size_t path_dirname(const char *name, const char **nameptr)
{
    const char *basename = name;

#ifdef HAVE_MULTIVOLUME
    path_strip_volume(name, &basename, true);
    if (basename == name)
#endif
        basename = GOBBLE_PATH_SEPCH(basename);

    if (*basename)
        path_basename(basename, &basename);

    *nameptr = basename;
    return basename - name;
}

/* Removes trailing separators from a path. Will not strip away the root but
 * will leave just '/'. Returns the length of the stripped path and *nameptr
 * is set to point to the separator following the last component or the null
 * if the path did not end with a separator.
 */
size_t path_strip_trailing_separators(const char *name, const char **nameptr)
{
    const char *p;
    size_t length = path_basename(name, &p);

    if (*p == PATH_SEPCH && length == 1 && p > name)
        p = name + 1; /* root with multiple separators */
    else
        p += length;  /* point past end of basename */

    if (nameptr)
        *nameptr = p;

    return p - name;
}

/* Transforms "wrong" separators into the correct ones. 'dstpath' and 'path'
 * may be the same buffer but otherwise should be non-overlapping buffers.
 */
void path_correct_separators(char *dstpath, const char *path)
{
    char *dstp = dstpath;
    const char *p = path;

    while (1)
    {
        const char *next = strchr(p, PATH_BADSEPCH);
        if (!next)
            break;

        size_t size = next - p;

        if (dstpath != path)
            memcpy(dstp, p, size);

        dstp += size;
        *dstp++ = PATH_SEPCH;
        p = next + 1;
    }

    if (dstpath != path)
        strcpy(dstp, p);
}

/* Appends one path to another, adding separators between components if needed.
 * Return value and behavior is otherwise as strlcpy so that truncation may be
 * detected.
 */
size_t path_append(char *buf, const char *basepath,
                   const char *component, size_t bufsize)
{
    const char *base = basepath && basepath[0] ? basepath : buf;
    if (!base)
        return bufsize; /* won't work to get lengths from buf */

    if (!buf)
        bufsize = 0;

    if (path_is_absolute(component))
    {
        /* 'component' is absolute; replace all */
        basepath = component;
        component = "";
    }

    /* if basepath is not null or empty, buffer contents are replaced,
       otherwise buf contains the base path */
    size_t len = base == buf ? strlen(buf) : strlcpy(buf, basepath, bufsize);

    bool separate = false;

    if (!basepath || !component)
        separate = !len || base[len-1] != '/';
    else if (component[0])
        separate = len && base[len-1] != '/';

    /* caller might lie about size of buf yet use buf as the base */
    if (base == buf && bufsize && len >= bufsize)
        buf[bufsize - 1] = '\0';

    buf += len;
    bufsize -= MIN(len, bufsize);

    if (separate && (len++, bufsize > 0) && --bufsize > 0)
        *buf++ = '/';

    return len + strlcpy(buf, component ?: "", bufsize);
}

/* Returns the location and length of the next path component, consuming the
 * input in the process. If 'want_dots' is true, then '.' and '..' components
 * are returned as any other component, otherwise they are merely consumed.
 *
 * Returns: 0 if the input has been consumed
 *          -1 if ".." was found (only if want_dots==false)
 *          The length of the component otherwise
 */
ssize_t parse_path_component(const char **pathp, const char **namep,
                             bool want_dots)
{
    const char *p, *name = *pathp;
    int c;

    /* a component starts at a non-separator and continues until the next
       separator or null */
    while (1)
    {
        if (!(c = *(name = GOBBLE_PATH_SEPCH(name))))
        {
            *pathp = name;
            return 0;
        }

        p = name;

        if (c != '.')
            break;

        /* handle dot and dot-dot */
        unsigned int dots = 1;
        while ((c = *++p) == '.' && dots++ < 2);

        if (!IS_COMP_TERMINATOR(c))
            break;

        /* is dot or dot-dot */
        if (want_dots)
        {
            *pathp = p;
            *namep = name;
            return dots;
        }
        else if (dots == 2)
        {
            *pathp = p;
            return -1;
        }
        else
        {
            name = p;
        }
    }

    p = GOBBLE_PATH_COMP(++p);
    *pathp = p;

    *namep = name;
    return p - name;
}
