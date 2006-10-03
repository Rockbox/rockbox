/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _FILEUTILS_H_
#define _FILEUTILS_H_

enum {
    DIRWALKER_ERROR = -1,
    DIRWALKER_OK = 0,
    DIRWALKER_IGNORE, /* only valid from folder callback */
    DIRWALKER_LEAVEDIR, /* only valid from file callback */
    DIRWALKER_QUIT
};

enum {
    DIRWALKER_RETURN_ERROR = -1,
    DIRWALKER_RETURN_SUCCESS,
    DIRWALKER_RETURN_FORCED
};

int dirwalker(const char *start_dir, bool recurse, bool is_forward,
                     int (*folder_callback)(const char* dir, void *param),void* folder_param,
                     int (*file_callback)(const char* folder,const char* file,
                     const int file_attr,void *param),void* file_param);

#endif /* _FILEUTILS_H_ */
