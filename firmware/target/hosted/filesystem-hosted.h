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
#ifndef _FILESYSTEM_HOSTED_H_
#define _FILESYSTEM_HOSTED_H_

#include "mv.h"

int os_volume_path(IF_MV(int volume, ) char *buffer, size_t bufsize);
void * os_lc_open(const char *ospath);

#endif /* _FILESYSTEM_HOSTED_H_ */

#ifdef _FILE_H_
#ifndef _FILESYSTEM_HOSTED__FILE_H_
#define _FILESYSTEM_HOSTED__FILE_H_

#ifndef OSFUNCTIONS_DECLARED
int os_modtime(const char *path, time_t modtime);
off_t os_filesize(int osfd);
int os_fsamefile(int osfd1, int osfd2);
int os_relate(const char *path1, const char *path2);
bool os_file_exists(const char *ospath);

#define __OPEN_MODE_ARG \
    , ({                                     \
        mode_t mode = 0;                     \
        if (oflag & O_CREAT)                 \
        {                                    \
            va_list ap;                      \
            va_start(ap, oflag);             \
            mode = va_arg(ap, unsigned int); \
            va_end(ap);                      \
        }                                    \
        mode; })

#define __CREAT_MODE_ARG \
    , mode

#endif /* OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_HOSTED__FILE_H_ */
#endif /* _FILE_H_ */

#ifdef _DIR_H_
#ifndef _FILESYSTEM_HOSTED__DIR_H_
#define _FILESYSTEM_HOSTED__DIR_H_

#ifndef OSFUNCTIONS_DECLARED
int os_opendirfd(const char *osdirname);
int os_opendir_and_fd(const char *osdirname, OS_DIR_T **osdirpp,
                      int *osfdp);

#define __MKDIR_MODE_ARG \
    , 0777
#endif /* OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_HOSTED__DIR_H_ */
#endif /* _DIR_H_ */
