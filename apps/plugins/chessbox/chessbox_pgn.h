/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 Mauricio Peccorini 
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
#include "gnuchess.h"

/* button definitions */
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_MENU
#define CB_DOWN    BUTTON_PLAY
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    (BUTTON_SELECT | BUTTON_PLAY)
#define CB_LEVEL   (BUTTON_SELECT | BUTTON_RIGHT)
#define CB_RESTART (BUTTON_SELECT | BUTTON_LEFT)
#define CB_MENU    (BUTTON_SELECT | BUTTON_MENU)

#define CB_SCROLL_UP     (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_REC
#define CB_RESTART (BUTTON_SELECT | BUTTON_PLAY)
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_MODE
#define CB_RESTART BUTTON_REC
#define CB_MENU    BUTTON_OFF

#define CB_RC_QUIT BUTTON_RC_STOP

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_EQ
#define CB_MENU    BUTTON_MODE

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == RECORDER_PAD
#define CB_SELECT  BUTTON_PLAY
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_F1
#define CB_RESTART BUTTON_F3
#define CB_MENU    BUTTON_OFF

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_F1
#define CB_RESTART BUTTON_F3
#define CB_MENU    BUTTON_OFF

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == ONDIO_PAD
#define CB_SELECT_PRE BUTTON_MENU
#define CB_SELECT  (BUTTON_MENU|BUTTON_REL)
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY_PRE BUTTON_MENU
#define CB_PLAY    (BUTTON_MENU|BUTTON_REPEAT)
#define CB_LEVEL   (BUTTON_MENU|BUTTON_OFF)
#define CB_RESTART (BUTTON_MENU|BUTTON_LEFT)
#define CB_MENU    BUTTON_OFF

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_A
#define CB_LEVEL   BUTTON_MENU
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_MENU
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define CB_SELECT  BUTTON_REW
#define CB_UP      BUTTON_SCROLL_UP
#define CB_DOWN    BUTTON_SCROLL_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_FF
#define CB_RESTART (BUTTON_REW | BUTTON_PLAY)
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    (BUTTON_SELECT | BUTTON_RIGHT)
#define CB_LEVEL   BUTTON_REC
#define CB_RESTART (BUTTON_SELECT | BUTTON_REPEAT)
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_SCROLL_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_SCROLL_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_VOL_UP
#define CB_LEVEL   BUTTON_HOME
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_VOL_UP
#define CB_LEVEL   BUTTON_REC
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == MROBE500_PAD
#define CB_SELECT  BUTTON_RC_MODE
#define CB_UP      BUTTON_RC_PLAY
#define CB_DOWN    BUTTON_RC_DOWN
#define CB_LEFT    BUTTON_RC_REW
#define CB_RIGHT   BUTTON_RC_FF
#define CB_PLAY    BUTTON_RC_HEART
#define CB_LEVEL   BUTTON_RC_VOL_DOWN
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_RC_PLAY|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_RC_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_RC_REW|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RC_FF|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_DISPLAY
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define CB_SELECT  BUTTON_RC_PLAY
#define CB_UP      BUTTON_RC_VOL_UP
#define CB_DOWN    BUTTON_RC_VOL_DOWN
#define CB_LEFT    BUTTON_RC_REW
#define CB_RIGHT   BUTTON_RC_FF
#define CB_PLAY    BUTTON_RC_MODE
#define CB_LEVEL   BUTTON_RC_MENU
#define CB_RESTART (BUTTON_RC_PLAY|BUTTON_REPEAT)
#define CB_MENU    BUTTON_RC_REC

#define CB_SCROLL_UP     (BUTTON_RC_VOL_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_RC_VOL_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_RC_REW|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RC_FF|BUTTON_REPEAT)

#define CB_RC_QUIT BUTTON_REC

#elif CONFIG_KEYPAD == COWOND2_PAD
#define CB_LEVEL   BUTTON_PLUS
#define CB_RESTART BUTTON_MINUS
#define CB_MENU    (BUTTON_MENU|BUTTON_REL)

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD

#define CB_SELECT  BUTTON_PLAY
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_SELECT
#define CB_LEVEL   BUTTON_CUSTOM
#define CB_MENU    BUTTON_MENU

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef CB_LEVEL
#define CB_LEVEL          BUTTON_TOPLEFT
#endif
#ifndef CB_RESTART
#define CB_RESTART        BUTTON_TOPRIGHT
#endif
#ifndef CB_MENU
#define CB_MENU           (BUTTON_BOTTOMLEFT|BUTTON_REL)
#endif
#ifndef CB_PLAY
#define CB_PLAY          (BUTTON_CENTER|BUTTON_REPEAT)
#endif
#ifndef CB_SELECT
#define CB_SELECT         BUTTON_CENTER
#endif
#ifndef CB_UP
#define CB_UP             BUTTON_TOPMIDDLE
#endif
#ifndef CB_DOWN
#define CB_DOWN           BUTTON_BOTTOMMIDDLE
#endif
#ifndef CB_LEFT
#define CB_LEFT           BUTTON_MIDLEFT
#endif
#ifndef CB_RIGHT
#define CB_RIGHT          BUTTON_MIDRIGHT
#endif
#ifndef CB_SCROLL_UP
#define CB_SCROLL_UP     (BUTTON_TOPMIDDLE|BUTTON_REPEAT)
#endif
#ifndef CB_SCROLL_DOWN
#define CB_SCROLL_DOWN   (BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT)
#endif
#ifndef CB_SCROLL_LEFT
#define CB_SCROLL_LEFT   (BUTTON_MIDLEFT|BUTTON_REPEAT)
#endif
#ifndef CB_SCROLL_RIGHT
#define CB_SCROLL_RIGHT  (BUTTON_MIDRIGHT|BUTTON_REPEAT)
#endif
#endif

/* structure to represent the plies */
struct pgn_ply_node {
    unsigned short player;
    char pgn_text[9];
    unsigned short row_from;
    unsigned short column_from;
    unsigned short row_to;
    unsigned short column_to;
    unsigned short taken_piece;
    unsigned short promotion_piece;
    bool enpassant;
    bool castle;
    bool promotion;
    bool checkmate;
    bool draw;
    struct pgn_ply_node* prev_node;
    struct pgn_ply_node* next_node;
};

/* structure to list the games */
struct pgn_game_node {
    unsigned short game_number;
    char white_player[20];
    char black_player[20];
    char game_date[11];
    char result[8];
    int pgn_line;
    struct pgn_ply_node* first_ply;
    struct pgn_game_node* next_node;
};

/* Return the list of games in a PGN file.
 * Parsing of the games themselves is postponed until
 * the user selects a game, that obviously saves processing
 * and speeds up response when the user selects the file
 */
struct pgn_game_node* pgn_list_games(const struct plugin_api* api,
                                     const char* filename);

/* Show the list of games found in a file and allow the user
 * to select a game to be parsed and showed
 */
struct pgn_game_node* pgn_show_game_list(const struct plugin_api* api,
                                         struct pgn_game_node* first_game);

/* Parse the pgn string of a game and assign it to the move
 * list in the structure
 */
void pgn_parse_game(const struct plugin_api* api, const char* filename,
                    struct pgn_game_node* selected_game);

/* Initialize a new game structure with default values and make
 * it ready to store the history of a newly played match
 */
struct pgn_game_node* pgn_init_game(const struct plugin_api* api);

/* Add a new ply to the game structure based on the positions */
void pgn_append_ply(const struct plugin_api* api, struct pgn_game_node* game,
                    unsigned short ply_player, char *move_buffer, bool is_mate);

/* Set the result of the game if it was reached during the opponent's ply
 */
void pgn_set_result(const struct plugin_api* api, struct pgn_game_node* game,
                    bool is_mate);

/* Store a complete game in the PGN history file
 */
void pgn_store_game(const struct plugin_api* api, struct pgn_game_node* game);
