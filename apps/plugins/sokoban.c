/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Eric Linenberg
 * February 2003: Robert Hak performs a cleanup/rewrite/feature addition.
 *                Eric smiles.  Bjorn cries.  Linus say 'huh?'.
 * March 2007: Sean Morrisey performs a major rewrite/feature addition.
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
#include "plugin.h"
#include "lib/playback_control.h"

PLUGIN_HEADER

#define SOKOBAN_TITLE        "Sokoban"

#define SOKOBAN_LEVELS_FILE  PLUGIN_GAMES_DIR "/sokoban.levels"
#define SOKOBAN_SAVE_FILE    PLUGIN_GAMES_DIR "/sokoban.save"
#define SOKOBAN_SAVE_FOLDER  "/games"

#include "pluginbitmaps/sokoban_tiles.h"
#define SOKOBAN_TILESIZE BMPWIDTH_sokoban_tiles

/* If tilesize is 0 (which it is during dependency generation) gcc will abort
   (div by 0) and this plugin won't get any dependencies
*/
#if SOKOBAN_TILESIZE < 1
#define SOKOBAN_TILESIZE 10
#endif

/* SOKOBAN_TILESIZE is the number of pixels for each block.
 * Set dynamically so all targets can support levels
 * that fill their entire screen, less the stat box.
 * 16 rows & 20 cols minimum */
#if LCD_WIDTH > LCD_HEIGHT /* horizontal layout*/
#define ROWS (LCD_HEIGHT/SOKOBAN_TILESIZE)
#if (LCD_WIDTH+4) >= (20*SOKOBAN_TILESIZE+40) /* wide or narrow stats box */
#define COLS ((LCD_WIDTH-40)/SOKOBAN_TILESIZE)
#else
#define COLS ((LCD_WIDTH-32)/SOKOBAN_TILESIZE)
#endif
#else /* vertical layout*/
#define ROWS ((LCD_HEIGHT-25)/SOKOBAN_TILESIZE)
#define COLS (LCD_WIDTH/SOKOBAN_TILESIZE)
#endif

/* Use either all but 16k of the plugin buffer for level data
 * or 128k, which ever is less */
#if PLUGIN_BUFFER_SIZE - 0x4000 < 0x20000
#define MAX_LEVEL_DATA       (PLUGIN_BUFFER_SIZE - 0x4000)
#else
#define MAX_LEVEL_DATA       0x20000
#endif

/* Number of levels for which to allocate buffer indexes */
#define MAX_LEVELS           MAX_LEVEL_DATA/70

/* Use 4k plus remaining plugin buffer (-12k for prog) for undo, up to 64k */
#if PLUGIN_BUFFER_SIZE - MAX_LEVEL_DATA - 0x3000 > 0x10000
#define MAX_UNDOS            0x10000
#else
#define MAX_UNDOS            (PLUGIN_BUFFER_SIZE - MAX_LEVEL_DATA - 0x3000)
#endif

/* Move/push definitions for undo */
#define SOKOBAN_PUSH_LEFT    'L'
#define SOKOBAN_PUSH_RIGHT   'R'
#define SOKOBAN_PUSH_UP      'U'
#define SOKOBAN_PUSH_DOWN    'D'
#define SOKOBAN_MOVE_LEFT    'l'
#define SOKOBAN_MOVE_RIGHT   'r'
#define SOKOBAN_MOVE_UP      'u'
#define SOKOBAN_MOVE_DOWN    'd'

#define SOKOBAN_MOVE_DIFF    (SOKOBAN_MOVE_LEFT-SOKOBAN_PUSH_LEFT)
#define SOKOBAN_MOVE_MIN     SOKOBAN_MOVE_DOWN

/* variable button definitions */
#if (CONFIG_KEYPAD == RECORDER_PAD) || \
    (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_OFF
#define SOKOBAN_UNDO BUTTON_ON
#define SOKOBAN_REDO BUTTON_PLAY
#define SOKOBAN_LEVEL_DOWN BUTTON_F1
#define SOKOBAN_LEVEL_REPEAT BUTTON_F2
#define SOKOBAN_LEVEL_UP BUTTON_F3
#define SOKOBAN_PAUSE BUTTON_PLAY
#define BUTTON_SAVE BUTTON_ON
#define BUTTON_SAVE_NAME "ON"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_OFF
#define SOKOBAN_UNDO_PRE BUTTON_MENU
#define SOKOBAN_UNDO (BUTTON_MENU | BUTTON_REL)
#define SOKOBAN_REDO (BUTTON_MENU | BUTTON_DOWN)
#define SOKOBAN_LEVEL_DOWN (BUTTON_MENU | BUTTON_LEFT)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_MENU | BUTTON_UP)
#define SOKOBAN_LEVEL_UP (BUTTON_MENU | BUTTON_RIGHT)
#define SOKOBAN_PAUSE BUTTON_MENU
#define BUTTON_SAVE BUTTON_MENU
#define BUTTON_SAVE_NAME "MENU"

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_OFF
#define SOKOBAN_UNDO BUTTON_REC
#define SOKOBAN_REDO BUTTON_MODE
#define SOKOBAN_LEVEL_DOWN (BUTTON_ON | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT BUTTON_ON
#define SOKOBAN_LEVEL_UP (BUTTON_ON | BUTTON_UP)
#define SOKOBAN_PAUSE BUTTON_ON
#define BUTTON_SAVE BUTTON_MODE
#define BUTTON_SAVE_NAME "MODE"

#define SOKOBAN_RC_MENU BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_MENU
#define SOKOBAN_DOWN BUTTON_PLAY
#define SOKOBAN_MENU (BUTTON_SELECT | BUTTON_MENU)
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO (BUTTON_SELECT | BUTTON_PLAY)
#define SOKOBAN_LEVEL_DOWN (BUTTON_SELECT | BUTTON_LEFT)
#define SOKOBAN_LEVEL_UP (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

/* FIXME: if/when simultaneous button presses work for X5/M5,
 * add level up/down */
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_LEVEL_REPEAT BUTTON_REC
#define SOKOBAN_REDO BUTTON_PLAY
#define SOKOBAN_PAUSE BUTTON_PLAY
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_SCROLL_UP
#define SOKOBAN_DOWN BUTTON_SCROLL_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_REW
#define SOKOBAN_UNDO (BUTTON_REW | BUTTON_REL)
#define SOKOBAN_REDO BUTTON_FF
#define SOKOBAN_LEVEL_DOWN (BUTTON_PLAY | BUTTON_SCROLL_DOWN)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_PLAY | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP (BUTTON_PLAY | BUTTON_SCROLL_UP)
#define SOKOBAN_PAUSE BUTTON_PLAY
#define BUTTON_SAVE BUTTON_PLAY
#define BUTTON_SAVE_NAME "PLAY"

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO BUTTON_SELECT
#define SOKOBAN_REDO BUTTON_A
#define SOKOBAN_LEVEL_DOWN BUTTON_VOL_DOWN
#define SOKOBAN_LEVEL_REPEAT BUTTON_MENU
#define SOKOBAN_LEVEL_UP BUTTON_VOL_UP
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO BUTTON_REC
#define SOKOBAN_LEVEL_DOWN (BUTTON_SELECT | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP (BUTTON_SELECT | BUTTON_UP)
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define SOKOBAN_LEFT        BUTTON_LEFT
#define SOKOBAN_RIGHT       BUTTON_RIGHT
#define SOKOBAN_UP          BUTTON_UP
#define SOKOBAN_DOWN        BUTTON_DOWN
#define SOKOBAN_MENU        BUTTON_POWER
#define SOKOBAN_UNDO_PRE    BUTTON_SELECT
#define SOKOBAN_UNDO       (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO       (BUTTON_SELECT | BUTTON_LEFT)
#define SOKOBAN_LEVEL_DOWN (BUTTON_SELECT | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP   (BUTTON_SELECT | BUTTON_UP)
#define SOKOBAN_PAUSE       BUTTON_SELECT
#define BUTTON_SAVE         BUTTON_SELECT
#define BUTTON_SAVE_NAME    "SELECT"

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO BUTTON_REC
#define SOKOBAN_LEVEL_DOWN BUTTON_VOL_DOWN
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP BUTTON_VOL_UP
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO BUTTON_HOME
#define SOKOBAN_LEVEL_DOWN BUTTON_VOL_DOWN
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP BUTTON_VOL_UP
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == SANSA_M200_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO_PRE BUTTON_SELECT
#define SOKOBAN_UNDO (BUTTON_SELECT | BUTTON_REL)
#define SOKOBAN_REDO (BUTTON_SELECT | BUTTON_UP)
#define SOKOBAN_LEVEL_DOWN BUTTON_VOL_DOWN
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP BUTTON_VOL_UP
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_MENU
#define SOKOBAN_UNDO BUTTON_VOL_UP
#define SOKOBAN_REDO BUTTON_VOL_DOWN
#define SOKOBAN_LEVEL_DOWN BUTTON_PREV
#define SOKOBAN_LEVEL_REPEAT BUTTON_PLAY
#define SOKOBAN_LEVEL_UP BUTTON_NEXT
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == MROBE100_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_POWER
#define SOKOBAN_UNDO BUTTON_SELECT
#define SOKOBAN_REDO BUTTON_MENU
#define SOKOBAN_LEVEL_DOWN (BUTTON_DISPLAY | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_DISPLAY | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP (BUTTON_DISPLAY | BUTTON_UP)
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_SELECT
#define BUTTON_SAVE_NAME "SELECT"

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define SOKOBAN_LEFT BUTTON_RC_REW
#define SOKOBAN_RIGHT BUTTON_RC_FF
#define SOKOBAN_UP BUTTON_RC_VOL_UP
#define SOKOBAN_DOWN BUTTON_RC_VOL_DOWN
#define SOKOBAN_MENU BUTTON_RC_REC
#define SOKOBAN_UNDO BUTTON_RC_MODE
#define SOKOBAN_REDO BUTTON_RC_MENU
#define SOKOBAN_PAUSE BUTTON_RC_PLAY
#define BUTTON_SAVE BUTTON_RC_PLAY
#define BUTTON_SAVE_NAME "PLAY"

#define SOKOBAN_RC_MENU BUTTON_REC

#elif CONFIG_KEYPAD == COWOND2_PAD
#define SOKOBAN_MENU BUTTON_MENU
#define SOKOBAN_MENU_NAME "[MENU]"

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_STOP
#define SOKOBAN_DOWN BUTTON_PLAY
#define SOKOBAN_MENU BUTTON_MENU
#define SOKOBAN_UNDO BUTTON_VOLDOWN
#define SOKOBAN_REDO BUTTON_VOLUP
#define SOKOBAN_PAUSE (BUTTON_MENU|BUTTON_LEFT)
#define BUTTON_SAVE (BUTTON_MENU|BUTTON_PLAY)
#define BUTTON_SAVE_NAME "MENU+PLAY"

#define SOKOBAN_RC_MENU (BUTTON_MENU|BUTTON_STOP)

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_MENU
#define SOKOBAN_UNDO BUTTON_BACK
#define SOKOBAN_REDO (BUTTON_BACK | BUTTON_PLAY)
#define SOKOBAN_LEVEL_DOWN (BUTTON_SELECT | BUTTON_DOWN)
#define SOKOBAN_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_RIGHT)
#define SOKOBAN_LEVEL_UP (BUTTON_SELECT | BUTTON_UP)
#define SOKOBAN_PAUSE BUTTON_PLAY
#define BUTTON_SAVE BUTTON_CUSTOM
#define BUTTON_SAVE_NAME "CUSTOM"

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define SOKOBAN_LEFT BUTTON_LEFT
#define SOKOBAN_RIGHT BUTTON_RIGHT
#define SOKOBAN_UP BUTTON_UP
#define SOKOBAN_DOWN BUTTON_DOWN
#define SOKOBAN_MENU BUTTON_MENU
#define SOKOBAN_UNDO BUTTON_VIEW
#define SOKOBAN_REDO (BUTTON_SELECT | BUTTON_VIEW)
#define SOKOBAN_LEVEL_DOWN BUTTON_VOL_DOWN
#define SOKOBAN_LEVEL_REPEAT BUTTON_POWER
#define SOKOBAN_LEVEL_UP BUTTON_VOL_UP
#define SOKOBAN_PAUSE BUTTON_SELECT
#define BUTTON_SAVE BUTTON_PLAYLIST
#define BUTTON_SAVE_NAME "PLAYLIST"

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef SOKOBAN_LEFT
#define SOKOBAN_LEFT          BUTTON_MIDLEFT
#endif
#ifndef SOKOBAN_RIGHT
#define SOKOBAN_RIGHT         BUTTON_MIDRIGHT
#endif
#ifndef SOKOBAN_UP
#define SOKOBAN_UP            BUTTON_TOPMIDDLE
#endif
#ifndef SOKOBAN_DOWN
#define SOKOBAN_DOWN          BUTTON_BOTTOMMIDDLE
#endif
#ifndef SOKOBAN_MENU
#define SOKOBAN_MENU          BUTTON_TOPLEFT
#define SOKOBAN_MENU_NAME     "[TOPLEFT]"
#endif
#ifndef SOKOBAN_UNDO
#define SOKOBAN_UNDO          BUTTON_BOTTOMRIGHT
#define SOKOBAN_UNDO_NAME     "[BOTTOMRIGHT]"
#endif
#ifndef SOKOBAN_REDO
#define SOKOBAN_REDO          BUTTON_BOTTOMLEFT
#define SOKOBAN_REDO_NAME     "[BOTTOMLEFT]"
#endif
#ifndef SOKOBAN_PAUSE
#define SOKOBAN_PAUSE         BUTTON_CENTER
#define SOKOBAN_PAUSE_NAME    "[CENTER]"
#endif
#ifndef SOKOBAN_LEVEL_REPEAT
#define SOKOBAN_LEVEL_REPEAT  BUTTON_TOPRIGHT
#define SOKOBAN_LEVEL_REPEAT_NAME "[TOPRIGHT]"
#endif
#ifndef BUTTON_SAVE
#define BUTTON_SAVE           BUTTON_CENTER
#define BUTTON_SAVE_NAME      "CENTER"
#endif
#endif

#define SOKOBAN_FONT FONT_SYSFIXED


/* The Location, Undo and LevelInfo structs are OO-flavored.
 * (oooh!-flavored as Schnueff puts it.)  It makes more you have to know,
 * but the overall data layout becomes more manageable. */

/* Level data & stats */
struct LevelInfo {
    int index;               /* Level index (level number - 1) */
    int moves;               /* Moves & pushes for the stats */
    int pushes;
    short boxes_to_go;       /* Number of unplaced boxes remaining in level */
    short height;            /* Height & width for centering level display */
    short width;
};

struct Location {
    short row;
    short col;
};

/* Our full undo history */
static struct UndoInfo {
    int count;               /* How many undos have been done */
    int current;             /* Which history is the current undo */
    int max;                 /* Which history is the max redoable */
    char history[MAX_UNDOS];
} undo_info;

/* Our playing board */
static struct BoardInfo {
    char board[ROWS][COLS];  /* The current board data */
    struct LevelInfo level;  /* Level data & stats */
    struct Location player;  /* Where the player is */
    int max_level;           /* The number of levels we have */
} current_info;

static struct BufferedBoards {
    char filename[MAX_PATH];   /* Filename of the levelset we're using */
    char data[MAX_LEVEL_DATA]; /* Buffered level data */
    int index[MAX_LEVELS + 1]; /* Where each buffered board begins & ends */
    int start;                 /* Index of first buffered board */
    int end;                   /* Index of last buffered board */
    short prebuffered_boards;  /* Number of boards before current to store */
} buffered_boards;


static char buf[ROWS*(COLS + 1)]; /* Enough for a whole board or a filename */


static void init_undo(void)
{
    undo_info.count = 0;
    undo_info.current = 0;
    undo_info.max = 0;
}

static void get_delta(char direction, short *d_r, short *d_c)
{
    switch (direction) {
        case SOKOBAN_PUSH_LEFT:
        case SOKOBAN_MOVE_LEFT:
            *d_r = 0;
            *d_c = -1;
            break;
        case SOKOBAN_PUSH_RIGHT:
        case SOKOBAN_MOVE_RIGHT:
            *d_r = 0;
            *d_c = 1;
            break;
        case SOKOBAN_PUSH_UP:
        case SOKOBAN_MOVE_UP:
            *d_r = -1;
            *d_c = 0;
            break;
        case SOKOBAN_PUSH_DOWN:
        case SOKOBAN_MOVE_DOWN:
            *d_r = 1;
            *d_c = 0;
    }
}

static bool undo(void)
{
    char undo;
    short r, c;
    short d_r = 0, d_c = 0; /* delta row & delta col */
    char *space_cur, *space_next, *space_prev;
    bool undo_push = false;

    /* If no more undos or we've wrapped all the way around, quit */
    if (undo_info.count == 0 || undo_info.current - 1 == undo_info.max)
        return false;

    /* Move to previous undo in the list */
    if (undo_info.current == 0 && undo_info.count > 1)
        undo_info.current = MAX_UNDOS - 1;
    else
        undo_info.current--;

    undo_info.count--;

    undo = undo_info.history[undo_info.current];

    if (undo < SOKOBAN_MOVE_MIN)
        undo_push = true;

    get_delta(undo, &d_r, &d_c);

    r = current_info.player.row;
    c = current_info.player.col;

    /* Give the 3 spaces we're going to use better names */
    space_cur = &current_info.board[r][c];
    space_next = &current_info.board[r + d_r][c + d_c];
    space_prev = &current_info.board[r - d_r][c - d_c];

    /* Update board info */
    if (undo_push) {
        /* Moving box from goal to floor */
        if (*space_next == '*' && *space_cur == '@')
            current_info.level.boxes_to_go++;
        /* Moving box from floor to goal */
        else if (*space_next == '$' && *space_cur == '+')
            current_info.level.boxes_to_go--;

        /* Move box off of next space... */
        *space_next = (*space_next == '*' ? '.' : ' ');
        /* ...and on to current space */
        *space_cur = (*space_cur == '+' ? '*' : '$');

        current_info.level.pushes--;
    } else
        /* Just move player off of current space */
        *space_cur = (*space_cur == '+' ? '.' : ' ');
    /* Move player back to previous space */
    *space_prev = (*space_prev == '.' ? '+' : '@');

    /* Update position */
    current_info.player.row -= d_r;
    current_info.player.col -= d_c;

    current_info.level.moves--;

    return true;
}

static void add_undo(char undo)
{
    undo_info.history[undo_info.current] = undo;

    /* Wrap around if MAX_UNDOS exceeded */
    if (undo_info.current < (MAX_UNDOS - 1))
        undo_info.current++;
    else
        undo_info.current = 0;

    if (undo_info.count < MAX_UNDOS)
        undo_info.count++;
}

static bool move(char direction, bool redo)
{
    short r, c;
    short d_r = 0, d_c = 0; /* delta row & delta col */
    char *space_cur, *space_next, *space_beyond;
    bool push = false;

    get_delta(direction, &d_r, &d_c);

    r = current_info.player.row;
    c = current_info.player.col;

    /* Check for out-of-bounds */
    if (r + 2*d_r < 0 || r + 2*d_r >= ROWS ||
        c + 2*d_c < 0 || c + 2*d_c >= COLS)
        return false;

    /* Give the 3 spaces we're going to use better names */
    space_cur = &current_info.board[r][c];
    space_next = &current_info.board[r + d_r][c + d_c];
    space_beyond = &current_info.board[r + 2*d_r][c + 2*d_c];

    if (*space_next == '$' || *space_next == '*') {
        /* Change direction from move to push for undo */
        if (direction >= SOKOBAN_MOVE_MIN)
            direction -= SOKOBAN_MOVE_DIFF;
        push = true;
    }
    else if (direction < SOKOBAN_MOVE_MIN)
        /* Change back to move if redo/solution playback push is invalid */
        direction += SOKOBAN_MOVE_DIFF;

    /* Update board info */
    if (push) {
        /* Moving box from goal to floor */
        if (*space_next == '*' && *space_beyond == ' ')
            current_info.level.boxes_to_go++;
        /* Moving box from floor to goal */
        else if (*space_next == '$' && *space_beyond == '.')
            current_info.level.boxes_to_go--;
        /* Check for invalid move */
        else if (*space_beyond != '.' && *space_beyond != ' ')
            return false;

        /* Move player onto next space */
        *space_next = (*space_next == '*' ? '+' : '@');
        /* Move box onto space beyond next */
        *space_beyond = (*space_beyond == '.' ? '*' : '$');

        current_info.level.pushes++;
    } else {
        /* Check for invalid move */
        if (*space_next != '.' && *space_next != ' ')
            return false;

        /* Move player onto next space */
        *space_next = (*space_next == '.' ? '+' : '@');
    }
    /* Move player off of current space */
    *space_cur = (*space_cur == '+' ? '.' : ' ');

    /* Update position */
    current_info.player.row += d_r;
    current_info.player.col += d_c;

    current_info.level.moves++;

    /* Update undo_info.max to current on every normal move,
     * except if it's the same as a redo. */
        /* normal move and either */
    if (!redo &&
          /* moves have been undone... */
        ((undo_info.max != undo_info.current &&
          /* ...and the current move is NOT the same as the one in history */
          undo_info.history[undo_info.current] != direction) ||
         /* or moves have not been undone */
         undo_info.max == undo_info.current)) {
        add_undo(direction);
        undo_info.max = undo_info.current;
    } else /* redo move or move was same as redo */
        add_undo(direction); /* add_undo to update current */

    return true;
}

#ifdef SOKOBAN_REDO
static bool redo(void)
{
    /* If no moves have been undone, quit */
    if (undo_info.current == undo_info.max)
        return false;

    return move(undo_info.history[(undo_info.current < MAX_UNDOS ?
                                   undo_info.current : 0)], true);
}
#endif

static void init_boards(void)
{
    rb->strncpy(buffered_boards.filename, SOKOBAN_LEVELS_FILE, MAX_PATH);

    current_info.level.index = 0;
    current_info.player.row = 0;
    current_info.player.col = 0;
    current_info.max_level = 0;

    buffered_boards.start = 0;
    buffered_boards.end = 0;
    buffered_boards.prebuffered_boards = 0;

    init_undo();
}

static bool read_levels(bool initialize)
{
    int fd = 0;
    short len;
    short lastlen = 0;
    short row = 0;
    int level_count = 0;

    int i = 0;
    int level_len = 0;
    bool index_set = false;

    /* Get the index of the first level to buffer */
    if (current_info.level.index > buffered_boards.prebuffered_boards &&
        !initialize)
        buffered_boards.start = current_info.level.index -
                                buffered_boards.prebuffered_boards;
    else
        buffered_boards.start = 0;

    if ((fd = rb->open(buffered_boards.filename, O_RDONLY)) < 0) {
        rb->splashf(HZ*2, "Unable to open %s", buffered_boards.filename);
        return false;
    }

    do {
        len = rb->read_line(fd, buf, sizeof(buf));

        /* Correct len when trailing \r's or \n's are counted */
        if (len > 2 && buf[len - 2] == '\0')
            len -= 2;
        else if (len > 1 && buf[len - 1] == '\0')
            len--;

        /* Skip short lines & lines with non-level data */
        if (len >= 3 && ((buf[0] >= '1' && buf[0] <= '9') || buf[0] == '#' ||
            buf[0] == ' ' || buf[0] == '-' || buf[0] == '_')) {
            if (level_count >= buffered_boards.start) {
                /* Set the index of this level */
                if (!index_set &&
                    level_count - buffered_boards.start < MAX_LEVELS) {
                    buffered_boards.index[level_count - buffered_boards.start]
                                                                           = i;
                    index_set = true;
                }
                /* Copy buffer to board data */
                if (i + level_len + len < MAX_LEVEL_DATA) {
                    rb->memcpy(&buffered_boards.data[i + level_len], buf, len);
                    buffered_boards.data[i + level_len + len] = '\n';
                }
            }
            level_len += len + 1;
            row++;

        /* If newline & level is tall enough or is RLE */
        } else if (buf[0] == '\0' && (row > 2 || lastlen > 22)) {
            level_count++;
            if (level_count >= buffered_boards.start) {
                i += level_len;
                if (i < MAX_LEVEL_DATA)
                    buffered_boards.end = level_count;
                else if (!initialize)
                    break;
            }
            row = 0;
            level_len = 0;
            index_set = false;

        } else if (len > 22)
            len = 1;

    } while ((lastlen = len));

    /* Set the index of the end of the last level */
    if (level_count - buffered_boards.start < MAX_LEVELS)
        buffered_boards.index[level_count - buffered_boards.start] = i;

    if (initialize) {
        current_info.max_level = level_count;
        buffered_boards.prebuffered_boards = buffered_boards.end/2;
    }

    rb->close(fd);

    return true;
}

static void load_level(void)
{
    int c, r;
    int i, n;
    int level_size;
    int index = current_info.level.index - buffered_boards.start;
    char *level;

    /* Get the buffered board index of the current level */
    if (current_info.level.index < buffered_boards.start ||
        current_info.level.index >= buffered_boards.end) {
        read_levels(false);
        if (current_info.level.index > buffered_boards.prebuffered_boards)
            index = buffered_boards.prebuffered_boards;
        else
            index = current_info.level.index;
    }
    level = &buffered_boards.data[buffered_boards.index[index]];

    /* Reset level info */
    current_info.level.moves = 0;
    current_info.level.pushes = 0;
    current_info.level.boxes_to_go = 0;
    current_info.level.width = 0;

    /* Clear board */
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            current_info.board[r][c] = 'X';

    level_size = buffered_boards.index[index + 1] -
                 buffered_boards.index[index];

    for (r = 0, c = 0, n = 1, i = 0; i < level_size; i++) {
        if (level[i] == '\n' || level[i] == '|') {
            if (c > 3) {
                /* Update max width of level & go to next row */
                if (c > current_info.level.width)
                    current_info.level.width = c;
                c = 0;
                r++;
                if (r >= ROWS)
                    break;
            }
        } else if (c < COLS) {
            /* Read RLE character's length into n */
            if (level[i] >= '0' && level[i] <= '9') {
                n = level[i++] - '0';
                if (level[i] >= '0' && level[i] <= '9')
                    n = n*10 + level[i++] - '0';
            }

            /* Cleanup & replace */
            if (level[i] == '%')
                level[i] = '*';
            else if (level[i] == '-' || level[i] == '_')
                level[i] = ' ';

            if (n > 1) {
                if (c + n >= COLS)
                    n = COLS - c;

                if (level[i] == '.')
                    current_info.level.boxes_to_go += n;

                /* Put RLE character n times */
                while (n--)
                    current_info.board[r][c++] = level[i];
                n = 1;

            } else {
                if (level[i] == '.' || level[i] == '+')
                    current_info.level.boxes_to_go++;

                if (level[i] == '@' ||level[i] == '+') {
                    current_info.player.row = r;
                    current_info.player.col = c;
                }

                current_info.board[r][c++] = level[i];
            }
        }
    }

    current_info.level.height = r;

#if LCD_DEPTH > 2
    /* Fill in blank space outside level on color targets */
    for (r = 0; r < ROWS; r++)
        for (c = 0; current_info.board[r][c] == ' ' && c < COLS; c++)
            current_info.board[r][c] = 'X';

    for (c = 0; c < COLS; c++) {
        for (r = 0; (current_info.board[r][c] == ' ' ||
             current_info.board[r][c] == 'X') && r < ROWS; r++)
            current_info.board[r][c] = 'X';
        for (r = ROWS - 1; (current_info.board[r][c] == ' ' ||
             current_info.board[r][c] == 'X') && r >= 0; r--)
            current_info.board[r][c] = 'X';
    }
#endif
}

static void update_screen(void)
{
    int c, r;
    int rows, cols;

#if LCD_WIDTH - (COLS*SOKOBAN_TILESIZE) < 32
#define STAT_HEIGHT 25
#define STAT_X (LCD_WIDTH - 120)/2
#define STAT_Y (LCD_HEIGHT - STAT_HEIGHT)
#define BOARD_WIDTH LCD_WIDTH
#define BOARD_HEIGHT (LCD_HEIGHT - STAT_HEIGHT)
    rb->lcd_putsxy(STAT_X + 4, STAT_Y + 4, "Level");
    rb->snprintf(buf, sizeof(buf), "%d", current_info.level.index + 1);
    rb->lcd_putsxy(STAT_X + 7, STAT_Y + 14, buf);
    rb->lcd_putsxy(STAT_X + 41, STAT_Y + 4, "Moves");
    rb->snprintf(buf, sizeof(buf), "%d", current_info.level.moves);
    rb->lcd_putsxy(STAT_X + 44, STAT_Y + 14, buf);
    rb->lcd_putsxy(STAT_X + 79, STAT_Y + 4, "Pushes");
    rb->snprintf(buf, sizeof(buf), "%d", current_info.level.pushes);
    rb->lcd_putsxy(STAT_X + 82, STAT_Y + 14, buf);

    rb->lcd_drawrect(STAT_X, STAT_Y, 38, STAT_HEIGHT);
    rb->lcd_drawrect(STAT_X + 37, STAT_Y, 39, STAT_HEIGHT);
    rb->lcd_drawrect(STAT_X + 75, STAT_Y, 45, STAT_HEIGHT);
#else
#if LCD_WIDTH - (COLS*SOKOBAN_TILESIZE) > 40
#define STAT_X (LCD_WIDTH - 40)
#else
#define STAT_X COLS*SOKOBAN_TILESIZE
#endif
#define STAT_Y (LCD_HEIGHT - 64)/2
#define STAT_WIDTH (LCD_WIDTH - STAT_X)
#define BOARD_WIDTH (LCD_WIDTH - STAT_WIDTH)
#define BOARD_HEIGHT LCD_HEIGHT
    rb->lcd_putsxy(STAT_X + 1, STAT_Y + 2, "Level");
    rb->snprintf(buf, sizeof(buf), "%d", current_info.level.index + 1);
    rb->lcd_putsxy(STAT_X + 4, STAT_Y + 12, buf);
    rb->lcd_putsxy(STAT_X + 1, STAT_Y + 23, "Moves");
    rb->snprintf(buf, sizeof(buf), "%d", current_info.level.moves);
    rb->lcd_putsxy(STAT_X + 4, STAT_Y + 33, buf);
#if STAT_WIDTH < 38
    rb->lcd_putsxy(STAT_X + 1, STAT_Y + 44, "Push");
#else
    rb->lcd_putsxy(STAT_X + 1, STAT_Y + 44, "Pushes");
#endif
    rb->snprintf(buf, sizeof(buf), "%d", current_info.level.pushes);
    rb->lcd_putsxy(STAT_X + 4, STAT_Y + 54, buf);

    rb->lcd_drawrect(STAT_X, STAT_Y + 0, STAT_WIDTH, 64);
    rb->lcd_hline(STAT_X, LCD_WIDTH - 1, STAT_Y + 21);
    rb->lcd_hline(STAT_X, LCD_WIDTH - 1, STAT_Y + 42);

#endif

    /* load the board to the screen */
    for (rows = 0; rows < ROWS; rows++) {
        for (cols = 0; cols < COLS; cols++) {
            c = cols*SOKOBAN_TILESIZE +
                (BOARD_WIDTH - current_info.level.width*SOKOBAN_TILESIZE)/2;
            r = rows*SOKOBAN_TILESIZE +
                (BOARD_HEIGHT - current_info.level.height*SOKOBAN_TILESIZE)/2;

            switch(current_info.board[rows][cols]) {
                case 'X': /* blank space outside of level */
                    break;

                case ' ': /* floor */
                    rb->lcd_bitmap_part(sokoban_tiles, 0, 0*SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE, c, r, SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE);
                    break;

                case '#': /* wall */
                    rb->lcd_bitmap_part(sokoban_tiles, 0, 1*SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE, c, r, SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE);
                    break;

                case '$': /* box */
                    rb->lcd_bitmap_part(sokoban_tiles, 0, 2*SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE, c, r, SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE);
                    break;

                case '*': /* box on goal */
                    rb->lcd_bitmap_part(sokoban_tiles, 0, 3*SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE, c, r, SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE);
                    break;

                case '.': /* goal */
                    rb->lcd_bitmap_part(sokoban_tiles, 0, 4*SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE, c, r, SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE);
                    break;

                case '@': /* player */
                    rb->lcd_bitmap_part(sokoban_tiles, 0, 5*SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE, c, r, SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE);
                    break;

                case '+': /* player on goal */
                    rb->lcd_bitmap_part(sokoban_tiles, 0, 6*SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE, c, r, SOKOBAN_TILESIZE,
                                        SOKOBAN_TILESIZE);
                    break;
            }
        }
    }

    /* print out the screen */
    rb->lcd_update();
}

static void draw_level(void)
{
    load_level();
    rb->lcd_clear_display();
    update_screen();
}

static bool save(char *filename, bool solution)
{
    int fd;
    char *loc;
    DIR *dir;
    char dirname[MAX_PATH];

    rb->splash(0, "Saving...");

    /* Create dir if it doesn't exist */
    if ((loc = rb->strrchr(filename, '/')) != NULL) {
        rb->strncpy(dirname, filename, loc - filename);
        dirname[loc - filename] = '\0';
        if(!(dir = rb->opendir(dirname)))
            rb->mkdir(dirname);
        else
            rb->closedir(dir);
    }

    if (filename[0] == '\0' ||
        (fd = rb->open(filename, O_WRONLY|O_CREAT|O_TRUNC)) < 0) {
        rb->splashf(HZ*2, "Unable to open %s", filename);
        return false;
    }

    /* Sokoban: S/P for solution/progress : level number : current undo */
    rb->snprintf(buf, sizeof(buf), "Sokoban:%c:%d:%d\n", (solution ? 'S' : 'P'),
                 current_info.level.index + 1, undo_info.current);
    rb->write(fd, buf, rb->strlen(buf));

    /* Filename of levelset */
    rb->write(fd, buffered_boards.filename,
              rb->strlen(buffered_boards.filename));
    rb->write(fd, "\n", 1);

    /* Full undo history */
    rb->write(fd, undo_info.history, undo_info.max);

    rb->close(fd);

    return true;
}

static bool load(char *filename, bool silent)
{
    int fd;
    int i, n;
    int len;
    int button;
    bool play_solution;
    bool paused = false;
    unsigned short speed = 2;
    int delay[] = {HZ/2, HZ/3, HZ/4, HZ/6, HZ/8, HZ/12, HZ/16, HZ/25};

    if (filename[0] == '\0' || (fd = rb->open(filename, O_RDONLY)) < 0) {
        if (!silent)
            rb->splashf(HZ*2, "Unable to open %s", filename);
        return false;
    }

    /* Read header, level number, & current undo */
    rb->read_line(fd, buf, sizeof(buf));

    /* If we're opening a level file, not a solution/progress file */
    if (rb->strncmp(buf, "Sokoban", 7) != 0) {
        rb->close(fd);

        rb->strncpy(buffered_boards.filename, filename, MAX_PATH);
        if (!read_levels(true))
            return false;

        current_info.level.index = 0;
        load_level();

        /* If there aren't any boxes to go or the player position wasn't set,
         * the file probably wasn't a Sokoban level file */
        if (current_info.level.boxes_to_go == 0 ||
            current_info.player.row == 0 || current_info.player.col == 0) {
            if (!silent)
                rb->splash(HZ*2, "File is not a Sokoban level file");
            return false;
        }

    } else {

        /* Read filename of levelset */
        rb->read_line(fd, buffered_boards.filename,
                      sizeof(buffered_boards.filename));

        /* Read full undo history */
        len = rb->read_line(fd, undo_info.history, MAX_UNDOS);

        /* Correct len when trailing \r's or \n's are counted */
        if (len > 2 && undo_info.history[len - 2] == '\0')
            len -= 2;
        else if (len > 1 && undo_info.history[len - 1] == '\0')
            len--;

        rb->close(fd);

        /* Check to see if we're going to play a solution or resume progress */
        play_solution = (buf[8] == 'S');

        /* Get level number */
        for (n = 0, i = 10; buf[i] >= '0' && buf[i] <= '9' && i < 15; i++)
            n = n*10 + buf[i] - '0';
        current_info.level.index = n - 1;

        /* Get undo index */
        for (n = 0, i++; buf[i] >= '0' && buf[i] <= '9' && i < 21; i++)
            n = n*10 + buf[i] - '0';
        if (n > len)
            n = len;
        undo_info.max = len;

        if (current_info.level.index < 0) {
            if (!silent)
                rb->splash(HZ*2, "Error loading level");
            return false;
        }
        if (!read_levels(true))
            return false;
        if (current_info.level.index >= current_info.max_level) {
            if (!silent)
                rb->splash(HZ*2, "Error loading level");
            return false;
        }

        load_level();

        if (play_solution) {
            rb->lcd_clear_display();
            update_screen();
            rb->sleep(2*delay[speed]);

            /* Replay solution until menu button is pressed */
            i = 0;
            while (true) {
                if (i < len) {
                    if (!move(undo_info.history[i], true)) {
                        n = i;
                        break;
                    }
                    rb->lcd_clear_display();
                    update_screen();
                    i++;
                } else
                    paused = true;

                rb->sleep(delay[speed]);

                while ((button = rb->button_get(false)) || paused) {
                    switch (button) {
                        case SOKOBAN_MENU:
                            /* Pretend the level is complete so we'll quit */
                            current_info.level.boxes_to_go = 0;
                            return true;

                        case SOKOBAN_PAUSE:
                            /* Toggle pause state */
                            paused = !paused;
                            break;

                        case SOKOBAN_LEFT:
                        case SOKOBAN_LEFT | BUTTON_REPEAT:
                            /* Go back one move */
                            if (paused) {
                                if (undo())
                                    i--;
                                rb->lcd_clear_display();
                                update_screen();
                            }
                            break;

                        case SOKOBAN_RIGHT:
                        case SOKOBAN_RIGHT | BUTTON_REPEAT:
                            /* Go forward one move */
                            if (paused) {
                                if (redo())
                                    i++;
                                rb->lcd_clear_display();
                                update_screen();
                            }
                            break;

                        case SOKOBAN_UP:
                        case SOKOBAN_UP | BUTTON_REPEAT:
                            /* Speed up */
                            if (speed < sizeof(delay)/sizeof(int) - 1)
                                speed++;
                            break;

                        case SOKOBAN_DOWN:
                        case SOKOBAN_DOWN | BUTTON_REPEAT:
                            /* Slow down */
                            if (speed > 0)
                                speed--;
                    }

                    if (paused)
                        rb->sleep(HZ/33);
                }
            }

            /* If level is complete, wait for keypress before quitting */
            if (current_info.level.boxes_to_go == 0)
                rb->button_get(true);

        } else {
            /* Advance to current undo */
            for (i = 0; i < n; i++) {
                if (!move(undo_info.history[i], true)) {
                    n = i;
                    break;
                }
            }

            rb->button_clear_queue();
            rb->lcd_clear_display();
        }

        undo_info.current = n;
    }

    return true;
}

static int sokoban_menu(void)
{
    int button;
    int selection = 0;
    int i;
    bool menu_quit;
    int start_selected = 0;
    int prev_level = current_info.level.index;

    MENUITEM_STRINGLIST(menu, "Sokoban Menu", NULL,
                        "Resume", "Select Level", "Audio Playback", "Keys",
                        "Load Default Level Set", "Quit Without Saving",
                        "Save Progress & Quit");

    do {
        menu_quit = true;
        selection = rb->do_menu(&menu, &start_selected, NULL, false);

        switch (selection) {
            case 0: /* Resume */
                break;

            case 1: /* Select level */
                current_info.level.index++;
                rb->set_int("Select Level", "", UNIT_INT,
                            &current_info.level.index, NULL, 1, 1,
                            current_info.max_level, NULL);
                current_info.level.index--;
                if (prev_level != current_info.level.index) {
                    init_undo();
                    draw_level();
                } else
                    menu_quit = false;
                break;

            case 2: /* Audio playback control */
                playback_control(NULL);
                menu_quit = false;
                break;

            case 3: /* Keys */
                FOR_NB_SCREENS(i)
                    rb->screens[i]->clear_display();
                rb->lcd_setfont(SOKOBAN_FONT);

#if (CONFIG_KEYPAD == RECORDER_PAD) || \
    (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
                rb->lcd_putsxy(3,  6, "[OFF] Menu");
                rb->lcd_putsxy(3, 16, "[ON] Undo");
                rb->lcd_putsxy(3, 26, "[PLAY] Redo");
                rb->lcd_putsxy(3, 36, "[F1] Down a Level");
                rb->lcd_putsxy(3, 46, "[F2] Restart Level");
                rb->lcd_putsxy(3, 56, "[F3] Up a Level");
#elif CONFIG_KEYPAD == ONDIO_PAD
                rb->lcd_putsxy(3,  6, "[OFF] Menu");
                rb->lcd_putsxy(3, 16, "[MODE] Undo");
                rb->lcd_putsxy(3, 26, "[MODE+DOWN] Redo");
                rb->lcd_putsxy(3, 36, "[MODE+LEFT] Previous Level");
                rb->lcd_putsxy(3, 46, "[MODE+UP] Restart Level");
                rb->lcd_putsxy(3, 56, "[MODE+RIGHT] Up Level");
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
                rb->lcd_putsxy(3,  6, "[STOP] Menu");
                rb->lcd_putsxy(3, 16, "[REC] Undo");
                rb->lcd_putsxy(3, 26, "[MODE] Redo");
                rb->lcd_putsxy(3, 36, "[PLAY+DOWN] Previous Level");
                rb->lcd_putsxy(3, 46, "[PLAY] Restart Level");
                rb->lcd_putsxy(3, 56, "[PLAY+UP] Next Level");
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
                rb->lcd_putsxy(3,  6, "[SELECT+MENU] Menu");
                rb->lcd_putsxy(3, 16, "[SELECT] Undo");
                rb->lcd_putsxy(3, 26, "[SELECT+PLAY] Redo");
                rb->lcd_putsxy(3, 36, "[SELECT+LEFT] Previous Level");
                rb->lcd_putsxy(3, 46, "[SELECT+RIGHT] Next Level");
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
                rb->lcd_putsxy(3,  6, "[POWER] Menu");
                rb->lcd_putsxy(3, 16, "[SELECT] Undo");
                rb->lcd_putsxy(3, 26, "[PLAY] Redo");
                rb->lcd_putsxy(3, 36, "[REC] Restart Level");
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
                rb->lcd_putsxy(3,  6, "[POWER] Menu");
                rb->lcd_putsxy(3, 16, "[REW] Undo");
                rb->lcd_putsxy(3, 26, "[FF] Redo");
                rb->lcd_putsxy(3, 36, "[PLAY+DOWN] Previous Level");
                rb->lcd_putsxy(3, 46, "[PLAY+RIGHT] Restart Level");
                rb->lcd_putsxy(3, 56, "[PLAY+UP] Next Level");
#elif CONFIG_KEYPAD == GIGABEAT_PAD
                rb->lcd_putsxy(3,  6, "[POWER] Menu");
                rb->lcd_putsxy(3, 16, "[SELECT] Undo");
                rb->lcd_putsxy(3, 26, "[A] Redo");
                rb->lcd_putsxy(3, 36, "[VOL-] Previous Level");
                rb->lcd_putsxy(3, 46, "[MENU] Restart Level");
                rb->lcd_putsxy(3, 56, "[VOL+] Next Level");
#elif CONFIG_KEYPAD == SANSA_E200_PAD
                rb->lcd_putsxy(3,  6, "[POWER] Menu");
                rb->lcd_putsxy(3, 16, "[SELECT] Undo");
                rb->lcd_putsxy(3, 26, "[REC] Redo");
                rb->lcd_putsxy(3, 36, "[SELECT+DOWN] Previous Level");
                rb->lcd_putsxy(3, 46, "[SELECT+RIGHT] Restart Level");
                rb->lcd_putsxy(3, 56, "[SELECT+UP] Next Level");
#endif

#ifdef HAVE_TOUCHSCREEN
                rb->lcd_putsxy(3,  6, SOKOBAN_MENU_NAME " Menu");
                rb->lcd_putsxy(3, 16, SOKOBAN_UNDO_NAME " Undo");
                rb->lcd_putsxy(3, 26, SOKOBAN_REDO_NAME " Redo");
                rb->lcd_putsxy(3, 36, SOKOBAN_PAUSE_NAME " Pause");
                rb->lcd_putsxy(3, 46, SOKOBAN_LEVEL_REPEAT_NAME " Restart Level");
#endif

                FOR_NB_SCREENS(i)
                    rb->screens[i]->update();

                /* Display until keypress */
                do {
                    rb->sleep(HZ/20);
                    button = rb->button_get(false);
                } while (!button || button & BUTTON_REL ||
                         button & BUTTON_REPEAT);

                menu_quit = false;
                break;

            case 4: /* Load default levelset */
                init_boards();
                if (!read_levels(true))
                    return 5; /* Quit */
                load_level();
                break;

            case 5: /* Quit */
                break;

            case 6: /* Save & quit */
                save(SOKOBAN_SAVE_FILE, false);
                rb->reload_directory();
        }

    } while (!menu_quit);

    /* Restore font */
    rb->lcd_setfont(SOKOBAN_FONT);

    FOR_NB_SCREENS(i) {
        rb->screens[i]->clear_display();
        rb->screens[i]->update();
    }

    return selection;
}

static bool sokoban_loop(void)
{
    bool moved;
    int i = 0, button = 0, lastbutton = 0;
    short r = 0, c = 0;
    int w, h;
    char *loc;

    while (true) {
        moved = false;

        r = current_info.player.row;
        c = current_info.player.col;

        button = rb->button_get(true);

        switch(button)
        {
#ifdef SOKOBAN_RC_MENU
            case SOKOBAN_RC_MENU:
#endif
            case SOKOBAN_MENU:
                switch (sokoban_menu()) {
                    case 5: /* Quit */
                    case 6: /* Save & quit */
                        return PLUGIN_OK;
                }
                update_screen();
                break;

            case SOKOBAN_UNDO:
#ifdef SOKOBAN_UNDO_PRE
                if (lastbutton != SOKOBAN_UNDO_PRE)
                    break;
#else       /* repeat can't work here for Ondio, iPod, et al */
            case SOKOBAN_UNDO | BUTTON_REPEAT:
#endif
                undo();
                rb->lcd_clear_display();
                update_screen();
                break;

#ifdef SOKOBAN_REDO
            case SOKOBAN_REDO:
            case SOKOBAN_REDO | BUTTON_REPEAT:
                moved = redo();
                rb->lcd_clear_display();
                update_screen();
                break;
#endif

#ifdef SOKOBAN_LEVEL_UP
            case SOKOBAN_LEVEL_UP:
            case SOKOBAN_LEVEL_UP | BUTTON_REPEAT:
                /* next level */
                init_undo();
                if (current_info.level.index + 1 < current_info.max_level)
                    current_info.level.index++;

                draw_level();
                break;
#endif

#ifdef SOKOBAN_LEVEL_DOWN
            case SOKOBAN_LEVEL_DOWN:
            case SOKOBAN_LEVEL_DOWN | BUTTON_REPEAT:
                /* previous level */
                init_undo();
                if (current_info.level.index > 0)
                    current_info.level.index--;

                draw_level();
                break;
#endif

#ifdef SOKOBAN_LEVEL_REPEAT
            case SOKOBAN_LEVEL_REPEAT:
            case SOKOBAN_LEVEL_REPEAT | BUTTON_REPEAT:
                /* same level */
                init_undo();
                draw_level();
                break;
#endif

            case SOKOBAN_LEFT:
            case SOKOBAN_LEFT | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_LEFT, false);
                break;

            case SOKOBAN_RIGHT:
            case SOKOBAN_RIGHT | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_RIGHT, false);
                break;

            case SOKOBAN_UP:
            case SOKOBAN_UP | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_UP, false);
                break;

            case SOKOBAN_DOWN:
            case SOKOBAN_DOWN | BUTTON_REPEAT:
                moved = move(SOKOBAN_MOVE_DOWN, false);
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }

        lastbutton = button;

        if (moved) {
            rb->lcd_clear_display();
            update_screen();
        }

        /* We have completed this level */
        if (current_info.level.boxes_to_go == 0) {

            if (moved) {
                rb->lcd_clear_display();

                /* Show level complete message & stats */
                rb->snprintf(buf, sizeof(buf), "Level %d Complete!",
                             current_info.level.index + 1);
                rb->lcd_getstringsize(buf, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - h*3, buf);

                rb->snprintf(buf, sizeof(buf), "%4d Moves ",
                             current_info.level.moves);
                rb->lcd_getstringsize(buf, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - h, buf);

                rb->snprintf(buf, sizeof(buf), "%4d Pushes",
                             current_info.level.pushes);
                rb->lcd_getstringsize(buf, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2, buf);

                if (undo_info.count < MAX_UNDOS) {
                    rb->snprintf(buf, sizeof(buf), "%s: Save solution",
                                 BUTTON_SAVE_NAME);
                    rb->lcd_getstringsize(buf, &w, &h);
                    rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 + h*2, buf);
                }

                rb->lcd_update();
                rb->sleep(HZ/4);
                rb->button_clear_queue();

                /* Display for 4 seconds or until new keypress */
                for (i = 0; i < 80; i++) {
                    rb->sleep(HZ/20);
                    button = rb->button_get(false);
                    if (button && !(button & BUTTON_REL) &&
                        !(button & BUTTON_REPEAT))
                        break;
                }

                if (button == BUTTON_SAVE) {
                    if (undo_info.count < MAX_UNDOS) {
                        /* Set filename to current levelset plus level number
                         * and .sok extension. Use SAVE_FOLDER if using the
                         * default levelset, since it's in a hidden folder. */
                        if (rb->strcmp(buffered_boards.filename,
                                       SOKOBAN_LEVELS_FILE) == 0) {
                            rb->snprintf(buf, sizeof(buf),
                                         "%s/sokoban.%d.sok",
                                         SOKOBAN_SAVE_FOLDER,
                                         current_info.level.index + 1);
                        } else {
                            if ((loc = rb->strrchr(buffered_boards.filename,
                                                   '.')) != NULL)
                                *loc = '\0';
                            rb->snprintf(buf, sizeof(buf), "%s.%d.sok",
                                         buffered_boards.filename,
                                         current_info.level.index + 1);
                            if (loc != NULL)
                                *loc = '.';
                        }

                        if (!rb->kbd_input(buf, MAX_PATH))
                            save(buf, true);
                    } else
                        rb->splash(HZ*2, "Solution too long to save");

                    rb->lcd_setfont(SOKOBAN_FONT); /* Restore font */
                }
            }

            FOR_NB_SCREENS(i) {
                rb->screens[i]->clear_display();
                rb->screens[i]->update();
            }

            current_info.level.index++;

            /* clear undo stats */
            init_undo();

            if (current_info.level.index >= current_info.max_level) {
                /* Show levelset complete message */
                rb->snprintf(buf, sizeof(buf), "You WIN!!");
                rb->lcd_getstringsize(buf, &w, &h);
                rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - h/2, buf);

                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                /* Display for 4 seconds or until keypress */
                for (i = 0; i < 80; i++) {
                    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
                    rb->lcd_update();
                    rb->sleep(HZ/10);

                    button = rb->button_get(false);
                    if (button && !(button & BUTTON_REL))
                        break;
                }
                rb->lcd_set_drawmode(DRMODE_SOLID);

                /* Reset to first level & show quit menu */
                current_info.level.index = 0;

                switch (sokoban_menu()) {
                    case 5: /* Quit */
                    case 6: /* Save & quit */
                        return PLUGIN_OK;
                }
            }

            load_level();
            update_screen();
        }

    } /* end while */

    return PLUGIN_OK;
}


enum plugin_status plugin_start(const void* parameter)
{
    int w, h;

    (void)(parameter);

    rb->lcd_setfont(SOKOBAN_FONT);

    rb->lcd_clear_display();
    rb->lcd_getstringsize(SOKOBAN_TITLE, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - h/2, SOKOBAN_TITLE);
    rb->lcd_update();
    rb->sleep(HZ); /* Show title for 1 second */

    init_boards();

    if (parameter == NULL) {
        /* Attempt to resume saved progress, otherwise start at beginning */
        if (!load(SOKOBAN_SAVE_FILE, true)) {
            init_boards();
            if (!read_levels(true))
                return PLUGIN_OK;
            load_level();
        }

    } else {
        /* The plugin is being used to open a file */
        if (load((char*) parameter, false)) {
            /* If we loaded & played a solution, quit */
            if (current_info.level.boxes_to_go == 0)
                return PLUGIN_OK;
        } else
            return PLUGIN_OK;
    }

    rb->lcd_clear_display();
    update_screen();

    return sokoban_loop();
}
