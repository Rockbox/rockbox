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
#define TREE_ATTR_MPA 0x40 /* mpeg audio file */
#define TREE_ATTR_M3U 0x80 /* playlist */
#define TREE_ATTR_WPS 0x100 /* wps config file */
#define TREE_ATTR_MOD 0x200 /* firmware file */
#define TREE_ATTR_CFG 0x400 /* config file */
#define TREE_ATTR_TXT 0x500 /* text file */
#define TREE_ATTR_FONT 0x800 /* font file */
#define TREE_ATTR_LNG  0x1000 /* binary lang file */
#define TREE_ATTR_ROCK 0x2000 /* binary rockbox plugin */
#define TREE_ATTR_UCL 0x4000 /* rockbox flash image */ 
#define TREE_ATTR_CH8 0x8000 /* chip-8 game */ 
#define TREE_ATTR_MASK 0xffd0 /* which bits tree.c uses (above + DIR) */

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
