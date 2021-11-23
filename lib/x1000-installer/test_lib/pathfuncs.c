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

#include "pathfuncs.h"
#include "strlcpy.h"
#include "system.h"
#include <string.h>

static const char* GOBBLE_PATH_SEPCH(const char* p)
{
    int c;
    while((c = *p) == PATH_SEPCH)
        ++p;
    return p;
}

static const char* GOBBLE_PATH_COMP(const char* p)
{
    int c;
    while((c = *p) && c != PATH_SEPCH)
        ++p;
    return p;
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
    size_t len = base == buf ? strlen(buf) : my_strlcpy(buf, basepath, bufsize);

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

    return len + my_strlcpy(buf, component ?: "", bufsize);
}
