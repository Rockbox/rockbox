/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Henrik Backe
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
#ifndef _FILEHANDLE_H_
#define _FILEHANDLE_H_

#include <stdbool.h>
#include <tree.h>

/* using attribute bits not used by FAT (FAT uses lower 7) */
#define FILE_ATTR_THUMBNAIL 0x0080 /* corresponding .talk file exists */
/* (this also reflects the sort order if by type) */
#define FILE_ATTR_BMARK 0x0100 /* book mark file */
#define FILE_ATTR_M3U   0x0200 /* playlist */
#define FILE_ATTR_AUDIO 0x0300 /* audio file */
#define FILE_ATTR_CFG   0x0400 /* config file */
#define FILE_ATTR_WPS   0x0500 /* wps config file */
#define FILE_ATTR_FONT  0x0600 /* font file */
#define FILE_ATTR_LNG   0x0700 /* binary lang file */
#define FILE_ATTR_ROCK  0x0800 /* binary rockbox plugin */
#define FILE_ATTR_MOD   0x0900 /* firmware file */
#define FILE_ATTR_RWPS  0x0A00 /* remote-wps config file */
#define FILE_ATTR_BMP   0x0B00 /* backdrop bmp file */
#define FILE_ATTR_KBD   0x0C00 /* keyboard file */
#define FILE_ATTR_FMR   0x0D00 /* preset file */
#define FILE_ATTR_CUE   0x0E00 /* cuesheet file */
#define FILE_ATTR_MASK  0xFF00 /* which bits tree.c uses for file types */

struct filetype {
    char* extension;
    int tree_attr;
    int icon;
    int voiceclip;
};
void tree_get_filetypes(const struct filetype**, int*);

/* init the filetypes structs.
   uses audio buffer for storage, so call early in init... */
void  filetype_init(void);
void read_viewer_theme_file(void);
#ifdef HAVE_LCD_COLOR
void read_color_theme_file(void);
#endif

/* Return the attribute (FILE_ATTR_*) of the file */
int filetype_get_attr(const char* file);
#ifdef HAVE_LCD_COLOR
int filetype_get_color(const char* name, int attr);
#endif
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
