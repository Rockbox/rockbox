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
#include <applimits.h>
#include <file.h>

#if CONFIG_KEYPAD == IRIVER_H100_PAD
#define TREE_NEXT      BUTTON_DOWN
#define TREE_PREV      BUTTON_UP
#define TREE_EXIT      BUTTON_LEFT
#define TREE_ENTER     BUTTON_RIGHT
#define TREE_RUN       (BUTTON_SELECT | BUTTON_REL)
#define TREE_RUN_PRE   BUTTON_SELECT
#define TREE_MENU      BUTTON_MODE
#define TREE_OFF       BUTTON_OFF
#define TREE_WPS       (BUTTON_ON | BUTTON_REL)
#define TREE_WPS_PRE   BUTTON_ON
#define TREE_PGUP      (BUTTON_ON | BUTTON_UP)
#define TREE_PGDN      (BUTTON_ON | BUTTON_DOWN)
#define TREE_CONTEXT   (BUTTON_SELECT | BUTTON_REPEAT)
#define TREE_CONTEXT2  (BUTTON_ON | BUTTON_SELECT)
#define TREE_POWER_BTN BUTTON_ON

#define TREE_RC_NEXT   BUTTON_RC_FF
#define TREE_RC_PREV   BUTTON_RC_REW
#define TREE_RC_EXIT   BUTTON_RC_STOP
#define TREE_RC_RUN    (BUTTON_RC_ON | BUTTON_REL)
#define TREE_RC_RUN_PRE   BUTTON_RC_ON
#define TREE_RC_MENU   BUTTON_RC_MENU
#define TREE_RC_WPS    (BUTTON_RC_VOL | BUTTON_REL)
#define TREE_RC_WPS_PRE   BUTTON_RC_VOL
#define TREE_RC_CONTEXT   (BUTTON_RC_ON | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == RECORDER_PAD
#define TREE_NEXT      BUTTON_DOWN
#define TREE_PREV      BUTTON_UP
#define TREE_EXIT      BUTTON_LEFT
#define TREE_ENTER     BUTTON_RIGHT
#define TREE_RUN       (BUTTON_PLAY | BUTTON_REL)
#define TREE_RUN_PRE   BUTTON_PLAY
#define TREE_MENU      BUTTON_F1
#define TREE_OFF       BUTTON_OFF
#define TREE_WPS       (BUTTON_ON | BUTTON_REL)
#define TREE_WPS_PRE   BUTTON_ON
#define TREE_PGUP      (BUTTON_ON | BUTTON_UP)
#define TREE_PGDN      (BUTTON_ON | BUTTON_DOWN)
#define TREE_CONTEXT   (BUTTON_PLAY | BUTTON_REPEAT)
#define TREE_CONTEXT2  (BUTTON_ON | BUTTON_PLAY)
#define TREE_POWER_BTN BUTTON_ON

#define TREE_RC_NEXT   BUTTON_RC_RIGHT
#define TREE_RC_PREV   BUTTON_RC_LEFT
#define TREE_RC_EXIT   BUTTON_RC_STOP
#define TREE_RC_RUN    BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == PLAYER_PAD
#define TREE_NEXT      BUTTON_RIGHT
#define TREE_PREV      BUTTON_LEFT
#define TREE_EXIT      BUTTON_STOP
#define TREE_RUN       (BUTTON_PLAY | BUTTON_REL)
#define TREE_RUN_PRE   BUTTON_PLAY
#define TREE_MENU      BUTTON_MENU
#define TREE_WPS       (BUTTON_ON | BUTTON_REL)
#define TREE_WPS_PRE   BUTTON_ON
#define TREE_CONTEXT   (BUTTON_PLAY | BUTTON_REPEAT)
#define TREE_CONTEXT2  (BUTTON_ON | BUTTON_PLAY)
#define TREE_POWER_BTN BUTTON_ON

#define TREE_RC_NEXT   BUTTON_RC_RIGHT
#define TREE_RC_PREV   BUTTON_RC_LEFT
#define TREE_RC_EXIT   BUTTON_RC_STOP
#define TREE_RC_RUN    BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define TREE_NEXT      BUTTON_DOWN
#define TREE_PREV      BUTTON_UP
#define TREE_EXIT      BUTTON_LEFT
#define TREE_RUN       (BUTTON_RIGHT | BUTTON_REL)
#define TREE_RUN_PRE   BUTTON_RIGHT
#define TREE_MENU      (BUTTON_MENU | BUTTON_REPEAT)
#define TREE_MENU_PRE  BUTTON_MENU
#define TREE_WPS       (BUTTON_MENU | BUTTON_REL)
#define TREE_WPS_PRE   BUTTON_MENU
#define TREE_CONTEXT   (BUTTON_RIGHT | BUTTON_REPEAT)
#define TREE_POWER_BTN BUTTON_OFF

#elif CONFIG_KEYPAD == GMINI100_PAD
#define TREE_NEXT      BUTTON_DOWN
#define TREE_PREV      BUTTON_UP
#define TREE_EXIT      BUTTON_LEFT
#define TREE_ENTER     BUTTON_RIGHT
#define TREE_RUN       (BUTTON_PLAY | BUTTON_REL)
#define TREE_RUN_PRE   BUTTON_PLAY
#define TREE_MENU      BUTTON_MENU
#define TREE_WPS       (BUTTON_ON | BUTTON_REL)
#define TREE_WPS_PRE   BUTTON_ON
#define TREE_PGUP      (BUTTON_ON | BUTTON_UP)
#define TREE_PGDN      (BUTTON_ON | BUTTON_DOWN)
#define TREE_CONTEXT   (BUTTON_PLAY | BUTTON_REPEAT)
#define TREE_CONTEXT2  (BUTTON_ON | BUTTON_PLAY)
#define TREE_POWER_BTN BUTTON_ON

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
 
/* browser context for file or db */
struct tree_context {
    int dirlevel;
    int dircursor;
    int dirstart;
    int firstpos; /* which dir entry is on first
                     position in dir buffer */
    int pos_history[MAX_DIR_LEVELS];
    int dirpos[MAX_DIR_LEVELS];
    int cursorpos[MAX_DIR_LEVELS];
    char currdir[MAX_PATH]; /* file use */
    int *dirfilter; /* file use */
    int filesindir;
    int dirsindir; /* file use */
    int dirlength; /* total number of entries in dir, incl. those not loaded */
    int table_history[MAX_DIR_LEVELS]; /* db use */
    int extra_history[MAX_DIR_LEVELS]; /* db use */
    int currtable; /* db use */
    int currextra; /* db use */

    void* dircache;
    int dircache_size;
    char* name_buffer;
    int name_buffer_size;
    int dentry_size;
    bool dirfull;
};

/* using attribute bits not used by FAT (FAT uses lower 7) */

#define TREE_ATTR_THUMBNAIL 0x0080 /* corresponding .talk file exists */

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
#define TREE_ATTR_MASK  0xFF00 /* which bits tree.c uses for file types */

void tree_get_filetypes(const struct filetype**, int*);
void tree_init(void);
void browse_root(void);
void set_current_file(char *path);
bool rockbox_browse(const char *root, int dirfilter);
bool create_playlist(void);
void resume_directory(const char *dir);
char *getcwd(char *buf, int size);
void reload_directory(void);
bool check_rockboxdir(void);
struct tree_context* tree_get_context(void);

#endif
