/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2006 Alexander Levin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _REVERSI_GAME_H
#define _REVERSI_GAME_H

#include <stdbool.h>
#include "plugin.h"

#define WHITE  1      /* WHITE constant, it always plays first (as in chess) */
#define BLACK -1      /* BLACK constant */
#define FREE   0      /* Free place constant */

#define BOARD_SIZE  8
#define INIT_STONES 4

/* Description of a move. A move is stored as a byte in the following format:
 * - bit  7   : 0 for valid entries (i.e. those containing a move info,
 *              1 for invalid entries
 * - bits 6..4: row
 * - bit  3   : 0 if it's white move, 1 if it's black move
 * - bits 2..0: column
 */
typedef unsigned char move_t;

#define MOVE_ROW(h) (((h) >> 4) & 0x7)
#define MOVE_COL(h) ((h) & 0x7)
#define MOVE_PLAYER(h) (((h) & 0x8) ? BLACK : WHITE)
#define MAKE_MOVE(r,c,player) ( ((r)<<4) | ((c)&0x7) | \
           ((player) == WHITE ? 0 : 0x8) )
#define MOVE_INVALID 0x80


/* State of a board */
typedef struct _reversi_board_t {
    /* The current state of the game (BLACK/WHITE/FREE) */
    int board[BOARD_SIZE][BOARD_SIZE];

    /* Game history. First move (mostly, but not necessarily, black) is stored
     * in history[0], second move (mostly, but not necessarily, white) is
     * stored in history[1] etc.
     */
    move_t history[BOARD_SIZE*BOARD_SIZE - INIT_STONES];

    const struct plugin_api *rb;
} reversi_board_t;


void reversi_init_game(reversi_board_t *game);
int reversi_flipped_color(const int color);
bool reversi_game_is_finished(const reversi_board_t *game, int cur_player);
int reversi_count_occupied_cells(const reversi_board_t *game,
        int *white_count, int *black_count);
int reversi_count_moves(const reversi_board_t *game);
int reversi_count_white_moves(const reversi_board_t *game);
int reversi_count_black_moves(const reversi_board_t *game);
int reversi_make_move(reversi_board_t *game, const int row,
        const int col, const int player);
int reversi_is_valid_move(const reversi_board_t *game,
        const int row, const int col, const int player);
int reversi_count_player_available_moves(const reversi_board_t *game,
        const int player);
int reversi_count_passes(const reversi_board_t *game, const int player);


#endif
