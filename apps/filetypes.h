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
/* init the filetypes structs.
   uses audio buffer for storage, so call early in init... */
void  filetype_init(void);
void read_viewer_theme_file(void);

/* Return the attribute (TREE_ATTR_*) of the file */
int filetype_get_attr(const char* file);
int filetype_get_icon(int attr);
/* return the plugin filename associated with the file */
char* filetype_get_plugin(const struct entry* file);

/* returns true if the attr is supported */
bool  filetype_supported(int attr);

/* List avialable viewers */
int filetype_list_viewers(const char* current_file);

/* start a plugin with file as the argument (called from onplay.c) */
int filetype_load_plugin(const char* plugin, char* file);


#endif
