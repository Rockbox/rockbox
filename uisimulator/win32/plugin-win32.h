/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _PLUGIN_WIN32_H_
#define _PLUGIN_WIN32_H_

#define BOOL win32_BOOL /* Avoid conflicts with BOOL/INT defined as */
#define INT win32_INT   /*  enum values in settings.h */

#include <windows.h>
#include "plugin.h"
#include "file.h"

#define RTLD_NOW 0

#undef filesize
#define filesize win32_filesize

#undef ftruncate
#define ftruncate NULL

typedef enum plugin_status (*plugin_fn)(struct plugin_api* api, void* param);

#define dlopen(_x_, _y_) LoadLibrary(_x_)
#define dlsym(_x_, _y_) (plugin_fn)GetProcAddress(_x_, _y_)
#define dlclose(_x_) FreeLibrary(_x_)
#define dlerror() "Unknown"

int strcasecmp (const char *a, const char *b);
int strncasecmp (const char *a, const char *b, size_t n);

#endif
