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

#ifdef HAVE_RECORDER_KEYPAD
#define TREE_NEXT  BUTTON_DOWN
#define TREE_PREV  BUTTON_UP
#define TREE_EXIT  BUTTON_LEFT
#define TREE_ENTER BUTTON_RIGHT
#define TREE_RUN   BUTTON_PLAY
#define TREE_MENU  (BUTTON_F1 | BUTTON_REL)
#define TREE_SHIFT BUTTON_ON
#define TREE_OFF   BUTTON_OFF

#define TREE_RC_NEXT  BUTTON_RC_RIGHT
#define TREE_RC_REV   BUTTON_RC_LEFT
#define TREE_RC_EXIT  BUTTON_RC_STOP
#define TREE_RC_ENTER BUTTON_RC_PLAY
#define TREE_RC_RUN   BUTTON_RC_PLAY

#elif defined HAVE_PLAYER_KEYPAD
#define TREE_NEXT  BUTTON_RIGHT
#define TREE_PREV  BUTTON_LEFT
#define TREE_EXIT  BUTTON_STOP
#define TREE_ENTER BUTTON_PLAY
#define TREE_RUN   BUTTON_PLAY
#define TREE_MENU  (BUTTON_MENU | BUTTON_REL)
#define TREE_SHIFT BUTTON_ON

#define TREE_RC_NEXT  BUTTON_RC_RIGHT
#define TREE_RC_REV   BUTTON_RC_LEFT
#define TREE_RC_EXIT  BUTTON_RC_STOP
#define TREE_RC_ENTER BUTTON_RC_PLAY
#define TREE_RC_RUN   BUTTON_RC_PLAY

#elif defined HAVE_ONDIO_KEYPAD
#define TREE_NEXT  BUTTON_DOWN
#define TREE_PREV  BUTTON_UP
#define TREE_EXIT  BUTTON_LEFT
#define TREE_ENTER BUTTON_RIGHT
#define TREE_RUN   BUTTON_RIGHT
#define TREE_MENU  (BUTTON_MENU | BUTTON_REPEAT)
#define TREE_SHIFT BUTTON_MENU

#endif

struct entry {
    short attr; /* FAT attributes + file type flags */
    unsigned long time_write; /* Last write time */
    char *name;
};

struct filetype {
    char* extension;
    int tree_attr;
    int icon;
    int voiceclip;
};
 

/* using attribute not used by FAT */
/* (this also reflects the sort order if by type) */
#define TREE_ATTR_BMARK 0x0100 /* book mark file */
#define TREE_ATTR_M3U   0x0200 /* playlist */
#define TREE_ATTR_MPA   0x0300 /* mpeg audio file */
#define TREE_ATTR_CFG   0x0400 /* config file */
#define TREE_ATTR_WPS   0x0500 /* wps config file */
#define TREE_ATTR_FONT  0x0600 /* font file */
#define TREE_ATTR_LNG   0x0700 /* binary lang file */
#define TREE_ATTR_ROCK  0x0800 /* binary rockbox plugin */
#define TREE_ATTR_MOD   0x0900 /* firmware file */
#define TREE_ATTR_MASK  0xFFC0 /* which bits tree.c uses (above) */

void tree_get_filetypes(const struct filetype**, int*);
void tree_init(void);
void browse_root(void);
void set_current_file(char *path);
bool rockbox_browse(const char *root, int dirfilter);
bool create_playlist(void);
void resume_directory(const char *dir);
char *getcwd(char *buf, int size);
void reload_directory(void);
struct entry* load_and_sort_directory(const char *dirname, const int *dirfilter,
                                      int *num_files, bool *buffer_full);
bool check_rockboxdir(void);

#endif
