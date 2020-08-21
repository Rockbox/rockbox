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
#ifndef _PATHFUNCS_H_
#define _PATHFUNCS_H_

#include <sys/types.h>
#define __need_size_t
#include <stddef.h>
#include <stdbool.h>
#include "config.h"

/* useful char constants that could be reconfigured if desired */
#define PATH_SEPCH    '/'
#define PATH_SEPSTR   "/"
#define PATH_ROOTSTR  "/"
#define PATH_BADSEPCH '\\'
#define PATH_DRVSEPCH ':'

/* a nicer way to check for "." and ".." than two strcmp() calls */
static inline bool is_dotdir_name(const char *name)
{
    return name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]));
}

static inline bool name_is_dot(const char *name)
{
    return name[0] == '.' && !name[1];
}

static inline bool name_is_dot_dot(const char *name)
{
    return name[0] == '.' && name[1] == '.' && !name[2];
}

/* return a pointer to the character following path separators */
#define GOBBLE_PATH_SEPCH(p) \
    ({ int _c;                          \
       const char *_p = (p);            \
       while ((_c = *_p) == PATH_SEPCH) \
            ++_p;                       \
       _p; })

/* return a pointer to the character following a path component which may
   be a separator or the terminating nul */
#define GOBBLE_PATH_COMP(p) \
    ({ int _c;                                \
       const char *_p = (p);                  \
       while ((_c = *_p) && _c != PATH_SEPCH) \
           ++_p;                              \
       _p; })

/* does the character terminate a path component? */
#define IS_COMP_TERMINATOR(c) \
    ({ int _c = (c);               \
       !_c || _c == PATH_SEPCH; })

#ifdef HAVE_MULTIVOLUME
int path_strip_volume(const char *name, const char **nameptr, bool greedy);
int get_volume_name(int volume, char *name);
#endif

int path_strip_drive(const char *name, const char **nameptr, bool greedy);
size_t path_basename(const char *name, const char **nameptr);
size_t path_dirname(const char *name, const char **nameptr);
size_t path_strip_trailing_separators(const char *name, const char **nameptr);
void path_correct_separators(char *dstpath, const char *path);

/* constants useable in basepath and component */
#define PA_SEP_HARD NULL /* separate even if base is empty */
#define PA_SEP_SOFT ""   /* separate only if base is nonempty */
size_t path_append(char *buffer, const char *basepath, const char *component,
                   size_t bufsize);
ssize_t parse_path_component(const char **pathp, const char **namep);

/* return true if path begins with a root '/' component and is not NULL */
static inline bool path_is_absolute(const char *path)
{
    return path && path[0] == PATH_SEPCH;
}

#endif /* _PATHFUNCS_H_ */
