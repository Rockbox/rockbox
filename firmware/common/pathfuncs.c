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
#if (CONFIG_STORAGE & STORAGE_USB)
    STORAGE_DEC_IDX_USB,
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
#if (CONFIG_STORAGE & STORAGE_USB)
    [STORAGE_DEC_IDX_USB]     = USB_VOL_DEC,
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
#if (CONFIG_STORAGE & STORAGE_USB)
    [STORAGE_USB_NUM]     = STORAGE_DEC_IDX_USB,
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

/* Strips directory components from the path
 * ""      *nameptr->NUL,   len=0: ""
 * "/"     *nameptr->/,     len=1: "/"
 * "//"    *nameptr->2nd /, len=1: "/"
 * "/a"    *nameptr->a,     len=1: "a"
 * "a/"    *nameptr->a,     len=1: "a"
 * "/a/bc" *nameptr->b,     len=2: "bc"
 * "d"     *nameptr->d,     len=1: "d"
 * "ef/gh" *nameptr->g,     len=2: "gh"
 *
 * Notes: * Doesn't do suffix removal at this time.
 *        * In the same string, path_dirname() returns a pointer with the
 *          same or lower address as path_basename().
 *        * Pasting a separator between the returns of path_dirname() and
 *          path_basename() will result in a path equivalent to the input.
 */
size_t path_basename(const char *name, const char **nameptr)
{
    const char *p = name;
    const char *q = p;
    const char *r = q;

    while (*(p = GOBBLE_PATH_SEPCH(p)))
    {
        q = p;
        p = GOBBLE_PATH_COMP(++p);
        r = p;
    }

    if (r == name && p > name)
        q = p, r = q--; /* root - return last slash */
    /* else path is an empty string */

    *nameptr = q;
    return r - q;
}

/* Strips the trailing component from the path
 * ""      *nameptr->NUL,   len=0: ""
 * "/"     *nameptr->/,     len=1: "/"
 * "//"    *nameptr->2nd /, len=1: "/"
 * "/a"    *nameptr->/,     len=1: "/"
 * "a/"    *nameptr->a,     len=0: ""
 * "/a/bc" *nameptr->/,     len=2: "/a"
 * "d"     *nameptr->d,     len=0: ""
 * "ef/gh" *nameptr->e,     len=2: "ef"
 *
 * Notes: * Interpret len=0 as ".".
 *        * In the same string, path_dirname() returns a pointer with the
 *          same or lower address as path_basename().
 *        * Pasting a separator between the returns of path_dirname() and
 *          path_basename() will result in a path equivalent to the input.
 *
 */
size_t path_dirname(const char *name, const char **nameptr)
{
    const char *p = GOBBLE_PATH_SEPCH(name);
    const char *q = name;
    const char *r = p;

    while (*(p = GOBBLE_PATH_COMP(p)))
    {
        const char *s = p;

        if (!*(p = GOBBLE_PATH_SEPCH(p)))
            break;

        q = s;
    }

    if (q == name && r > name)
        name = r, q = name--; /* root - return last slash */

    *nameptr = name;
    return q - name;
}

/* Removes trailing separators from a path
 * ""     *nameptr->NUL,   len=0: ""
 * "/"    *nameptr->/,     len=1: "/"
 * "//"   *nameptr->2nd /, len=1: "/"
 * "/a/"  *nameptr->/,     len=2: "/a"
 * "//b/" *nameptr->1st /, len=3: "//b"
 * "/c/"  *nameptr->/,     len=2: "/c"
 */
size_t path_strip_trailing_separators(const char *name, const char **nameptr)
{
    const char *p;
    size_t len = path_basename(name, &p);

    if (len == 1 && *p == '/' && p > name)
    {
        *nameptr = p;
        name = p - 1; /* root with multiple separators */
    }
    else
    {
        *nameptr = name;
        p += len; /* length to end of basename */
    }

    return p - name;
}

/* Transforms "wrong" separators into the correct ones
 * "c:\windows\system32" -> "c:/windows/system32"
 *
 * 'path' and 'dstpath' may either be the same buffer or non-overlapping
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
            memcpy(dstp, p, size); /* not in-place */

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
 *
 * For basepath and component:
 * PA_SEP_HARD adds a separator even if the base path is empty
 * PA_SEP_SOFT adds a separator only if the base path is not empty
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
        separate = !len || base[len-1] != PATH_SEPCH;
    else if (component[0])
        separate = len && base[len-1] != PATH_SEPCH;

    /* caller might lie about size of buf yet use buf as the base */
    if (base == buf && bufsize && len >= bufsize)
        buf[bufsize - 1] = '\0';

    buf += len;
    bufsize -= MIN(len, bufsize);

    if (separate && (len++, bufsize > 0) && --bufsize > 0)
        *buf++ = PATH_SEPCH;

    return len + strlcpy(buf, component ?: "", bufsize);
}

/* Returns the location and length of the next path component, consuming the
 * input in the process.
 *
 * "/a/bc/d" breaks into:
 *   start:  *namep->1st /
 *   call 1: *namep->a,   *pathp->2nd / len=1: "a"
 *   call 2: *namep->b,   *pathp->3rd / len=2: "bc"
 *   call 3: *namep->d,   *pathp->NUL,  len=1: "d"
 *   call 4: *namep->NUL, *pathp->NUL,  len=0: ""
 *
 * Returns: 0 if the input has been consumed
 *          The length of the component otherwise
 */
ssize_t parse_path_component(const char **pathp, const char **namep)
{
    /* a component starts at a non-separator and continues until the next
       separator or null */
    const char *p = GOBBLE_PATH_SEPCH(*pathp);
    const char *name = p;

    if (*p)
        p = GOBBLE_PATH_COMP(++p);

    *pathp = p;
    *namep = name;

    return p - name;
}
