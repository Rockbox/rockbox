/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2002 Henrik Backe
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _FILEHANDLE_H_
#define _FILEHANDLE_H_

#include <stdbool.h>
#include <tree.h>
#include <menu.h>

int filetype_get_attr(const char*);
#ifdef HAVE_LCD_BITMAP
const char* filetype_get_icon(int);
#else
int   filetype_get_icon(int);
#endif
char* filetype_get_plugin(const struct entry*);
void  filetype_init(void);
bool  filetype_supported(int);
int   filetype_load_menu(struct menu_item*, int);
int   filetype_load_plugin(const char*, char*);

struct file_type {
#ifdef HAVE_LCD_BITMAP
    const unsigned char* icon; /* the icon which shall be used for it, NULL if unknown */
#else
    int icon; /* the icon which shall be used for it, -1 if unknown */
#endif
    char* plugin; /* Which plugin to use, NULL if unknown */
    bool  no_extension;
};

struct ext_type {
    char* extension; /* extension for which the file type is recognized */
    struct file_type*  type;
};

#endif
