/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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
#include <stdio.h>
#include "config.h"
#include "system.h"
#include "fat.h"
#include "dir.h"
#include "stdlib.h"
#include "string.h"
#include "debug.h"
#include "file.h"
#include "filefuncs.h"
#include "string-extra.h"
#include "storage.h"
#include "file_internal.h"

#ifndef __PCTOOL__
#ifdef HAVE_MULTIVOLUME

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
   path after the volume specifier, which could be the nul if the path is
   just a volume root. If *nameptr > name, then a volume specifier was
   found. If 'greedy' is 'true', then it consumes all trailing separators
   too. */
int strip_volume(const char *name, const char **nameptr, bool greedy)
{
    int volume = 0;
    const char *t = name;
    int c, v;

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

/* Returns the volume specifier decorated with the storage type name;
   assumes the buffer length is {VOL_MAX_LEN}+1 */
int get_volume_name(int volume, char *buf)
{
    if (volume < 0)
    {
        *buf = '\0';
        return 0;
    }

    volume %= VOL_NUM_MAX; /* as path parser would have it */

    int type = storage_driver_type(volume / NUM_VOLUMES_PER_DRIVE);
    if (type < 0 || type > STORAGE_NUM_TYPES)
        type = STORAGE_NUM_TYPES;
            
    const char *voldec = storage_dec_names[storage_dec_indexes[type]];
    return snprintf(buf, VOL_MAX_LEN + 1, "%c%s%d%c",
                    VOL_START_TOK, voldec, volume, VOL_END_TOK);
}
#endif /* #ifdef HAVE_MULTIVOLUME */
#endif /* __PCTOOL__ */

#if (CONFIG_PLATFORM & PLATFORM_NATIVE) || defined(SIMULATOR)
struct dirinfo dir_get_info(DIR *dirp, struct dirent *entry)
{
    (void)dirp;
    return entry->info;
}
#endif /* (CONFIG_PLATFORM & PLATFORM_NATIVE) || defined(SIMULATOR) */

/* similar to basename() but always returns a pointer into the path string
   and never uses static storage; the return value is the length of the
   substring; also pays no mind to '/' vs. '//'. */
size_t path_basename(const char *path, const char **basename)
{
    const char *p = path;
    size_t rc = 0;

    while (*(p = GOBBLE_PATH_SEPCH(p)))
    {
        path = p;
        p = GOBBLE_PATH_COMP(++p);
        rc = p - path;
    }

    if (rc == 0 && p > path)
    {
        /* root - return last slash */
        path = p - 1;
        rc = 1;
    }
    /* else path is an empty string */

    *basename = path;
    return rc;
}

/* append one path to another, adding separators between components if needed
 * return value and behavior is otherwise as strlcpy */
size_t append_to_path(char *buf, const char *basepath,
                      const char *component, size_t bufsize)
{
    size_t baselen;
    size_t slashlen = 0;

    if (!basepath)
    {
        /* basepath is in destination buffer (concat) */
        basepath = buf;
        baselen = basepath[0] ? strlen(basepath) : 0;
    }
    else
    {
        baselen = strlcpy(buf, basepath, bufsize);
    }

    if ((baselen == 0 || basepath[baselen-1] != PATH_SEPCH) && component[0])
        slashlen = 1;

    if (baselen >= bufsize && bufsize > 0)
        buf[bufsize - 1] = '\0';

    buf += baselen;
    bufsize -= MIN(baselen, bufsize);

    component = GOBBLE_PATH_SEPCH(component);

    if (slashlen && bufsize > 0 && --bufsize)
        *buf++ = PATH_SEPCH;

    return baselen + slashlen + strlcpy(buf, component, bufsize);
}

/* return the location and length of the next path component, consuming the
 * input in the process;
 *
 * returns:  0 if the input has been consumed
 *          -1 if ".." was found
 *          the length of the component otherwise
 */
ssize_t parse_path_component(const char **pathp, const char **namep)
{
    const char *p, *name = *pathp;
    int c;

    /* a component starts at a non-separator and continues until the next
       separator or nul */
    while (1)
    {
        if (!(c = *(name = GOBBLE_PATH_SEPCH(name))))
        {
            *pathp = name;
            return 0;
        }

        p = name;

        if (c != PATH_DOTCH)
            break;

        /* handle dot and dot-dot */
        unsigned int dots = 1;
        while ((c = *++p) == PATH_DOTCH && dots++ < 2);

        if (!IS_COMP_TERMINATOR(c))
            break;

        if (dots == 2)
        {
            /* is dot-dot */
            *pathp = p;
            return -1;
        }

        /* is dot; just dispose of those */
        name = p;
    }

    p = GOBBLE_PATH_COMP(++p);
    *pathp = p;

    *namep = name;
    return p - name;
}

/* Native versions are far more efficient because they don't use the descriptor
   pool and share common code */
#if !(CONFIG_PLATFORM & PLATFORM_NATIVE)
/* Test file/directory existence */
bool file_exists(const char *path)
{
#ifdef DEBUG
    if (!path || !*path)
    {
        DEBUGF("%s(%p): Invalid parameter!\n", __func__, path);
        return false;
    }
#endif /* DEBUG */

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return false;

    close(fd);
    return true;
}

/* Test directory existence */
bool dir_exists(const char *dirname)
{
#ifdef DEBUG
    if (!dirname || !*dirname)
    {
        DEBUGF("%s(%p): Invalid parameter!\n", __func__, dirname);
        return false;
    }
#endif /* DEBUG */

    DIR *dirp = opendir(dirname);
    if (!dirp)
        return false;

    closedir(dirp);
    return true;
}
#endif /* !(CONFIG_PLATFORM & PLATFORM_NATIVE) */
