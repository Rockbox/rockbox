/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _TREE_H_
#define _TREE_H_

#include <stdbool.h>

struct entry {
    short attr; /* FAT attributes + file type flags */
    char *name;
};

/* using attribute not used by FAT */
#define TREE_ATTR_MPA  0x0100 /* mpeg audio file */
#define TREE_ATTR_M3U  0x0200 /* playlist */
#define TREE_ATTR_WPS  0x0300 /* wps config file */
#define TREE_ATTR_MOD  0x0400 /* firmware file */
#define TREE_ATTR_CFG  0x0500 /* config file */
#define TREE_ATTR_TXT  0x0600 /* text file */
#define TREE_ATTR_FONT 0x0700 /* font file */
#define TREE_ATTR_LNG  0x0800 /* binary lang file */
#define TREE_ATTR_ROCK 0x0900 /* binary rockbox plugin */
#define TREE_ATTR_UCL  0x0A00 /* rockbox flash image */ 
#define TREE_ATTR_CH8  0x0B00 /* chip-8 game */ 
#define TREE_ATTR_RVF  0x0C00 /* rockbox video file */ 
#define TREE_ATTR_BMARK 0x0D00 /* book mark file */
#define TREE_ATTR_JPEG 0x0E00 /* JPEG image */
#define TREE_ATTR_MASK 0xFFC0 /* which bits tree.c uses (above) */

void tree_init(void);
void browse_root(void);
void set_current_file(char *path);
bool rockbox_browse(char *root, int dirfilter);
bool create_playlist(void);
void resume_directory(char *dir);
char *getcwd(char *buf, int size);
void reload_directory(void);
struct entry* load_and_sort_directory(char *dirname, int *dirfilter,
                                      int *num_files, bool *buffer_full);

#endif
