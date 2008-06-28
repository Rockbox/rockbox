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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Reversi. Code is heavily based on othello code by Claudio Clemens which is
 * copyright (c) 2003-2006 Claudio Clemens <asturio at gmx dot net> and is
 * released under the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any later version.
 */

#include <stddef.h>
#include "reversi-game.h"

/*
 * Constants for directions. The values are chosen so that
 * they can be bit combined.
 */
#define DIR_UP 1        /* UP         */
#define DIR_DO 2        /* DOWN       */
#define DIR_LE 4        /* LEFT       */
#define DIR_RI 8        /* RIGHT      */
#define DIR_UL 16       /* UP LEFT    */
#define DIR_UR 32       /* UP RIGHT   */
#define DIR_DL 64       /* DOWN LEFT  */
#define DIR_DR 128      /* DOWN RIGHT */

/* Array of directions for easy iteration through all of them */
static int DIRECTIONS[] =
    {DIR_UP, DIR_DO, DIR_LE, DIR_RI, DIR_UL, DIR_UR, DIR_DL, DIR_DR};
#define NUM_OF_DIRECTIONS 8


/* Initializes a reversi game */
void reversi_init_game(reversi_board_t *game) {
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            game->board[r][c] = FREE;
        }
    }
    game->board[3][3] = WHITE;
    game->board[4][4] = WHITE;
    game->board[3][4] = BLACK;
    game->board[4][3] = BLACK;

    /* Invalidate the history */
    c = sizeof(game->history) / sizeof(game->history[0]);
    for (r = 0; r < c; r++) {
        game->history[r] = MOVE_INVALID;
    }
}


/* Returns the 'flipped' color, e.g. WHITE for BLACK and vice versa */
int reversi_flipped_color(const int color) {
    switch (color) {
        case WHITE:
            return BLACK;

        case BLACK:
            return WHITE;

        default:
            return FREE;
    }
}


/* Counts and returns the number of occupied cells on the board.
 * If white_count and/or black_count is not null, the number of
 * white/black stones is placed there. */
int reversi_count_occupied_cells(const reversi_board_t *game,
        int *white_count, int *black_count) {
    int w_cnt, b_cnt, r, c;
    w_cnt = b_cnt = 0;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (game->board[r][c] == WHITE) {
                w_cnt++;
            } else if (game->board[r][c] == BLACK) {
                b_cnt++;
            }
        }
    }
    if (white_count != NULL) {
        *white_count = w_cnt;
    }
    if (black_count != NULL) {
        *black_count = b_cnt;
    }
    return w_cnt + b_cnt;
}


/* Returns the number of free cells on the board */
static int reversi_count_free_cells(const reversi_board_t *game) {
    int cnt;
    cnt = reversi_count_occupied_cells(game, NULL, NULL);
    return BOARD_SIZE*BOARD_SIZE - cnt;
}


/* Checks whether the game is finished. That means that nobody
 * can make a move. Note that the implementation is not correct
 * as a game may be finished even if there are free cells
 */
bool reversi_game_is_finished(const reversi_board_t *game, int player) {
    return (reversi_count_free_cells(game) == 0)
           || (reversi_count_passes(game,player) == 2);
}


/* Returns the total number of moves made so far */
int reversi_count_moves(const reversi_board_t *game) {
    int cnt;
    cnt = reversi_count_occupied_cells(game, NULL, NULL);
    return cnt - INIT_STONES;
}


/* Returns the number of moves made by the specified
 * player (WHITE or BLACK) so far
 */
static int reversi_count_player_moves(const reversi_board_t *game,
        const int player) {
    int moves, cnt, i;
    moves = reversi_count_moves(game);
    cnt = 0;
    for (i = 0; i < moves; i++) {
        if (MOVE_PLAYER(game->history[i]) == player) {
            cnt++;
        }
    }
    return cnt;
}

/* Returns the number of moves available for the specified player */
int reversi_count_player_available_moves(const reversi_board_t *game,
        const int player) {
    int cnt = 0, row, col;
    for(row=0;row<BOARD_SIZE;row++) {
        for(col=0;col<BOARD_SIZE;col++) {
            if(reversi_is_valid_move(game, row, col, player)) cnt++;
        }
    }
    return cnt;
}

/* Returns the number of players who HAVE to pass (2 == game is stuck) */
int reversi_count_passes(const reversi_board_t *game, const int player) {
    if(reversi_count_player_available_moves(game,player)==0) {
        if(reversi_count_player_available_moves(game,
                    reversi_flipped_color(player))==0) {
            return 2;
        }
        return 1;
    }
    return 0;
}

/* Returns the number of moves made by WHITE so far */
int reversi_count_white_moves(const reversi_board_t *game) {
    return reversi_count_player_moves(game, WHITE);
}


/* Returns the number of moves made by BLACK so far */
int reversi_count_black_moves(const reversi_board_t *game) {
    return reversi_count_player_moves(game, BLACK);
}


/* Checks whether the specified position is on the board
 * (and not beyond)
 */
static bool reversi_is_position_on_board(const int row, const int col) {
    return (row >= 0) && (row < BOARD_SIZE) &&
           (col >= 0) && (col < BOARD_SIZE);
}


/* Returns the delta for row to move in the specified direction */
static int reversi_row_delta(const int direction) {
    switch (direction) {
        case DIR_UP:
        case DIR_UL:
        case DIR_UR:
            return -1;

        case DIR_DO:
        case DIR_DL:
        case DIR_DR:
            return 1;

        default:
            return 0;
    }
}


/* Returns the delta for column to move in the specified direction */
static int reversi_column_delta(const int direction) {
    switch (direction) {
        case DIR_LE:
        case DIR_UL:
        case DIR_DL:
            return -1;

        case DIR_RI:
        case DIR_UR:
        case DIR_DR:
            return 1;

        default:
            return 0;
    }
}


/* Checks if some stones would be captured in the specified direction
 * if a stone were placed in the specified cell by the specified player.
 *
 * Returns 0 if no stones would be captured or 'direction' otherwise
 */
static int reversi_is_valid_direction(const reversi_board_t *game,
        const int row, const int col, const int player, const int direction) {
    int row_delta, col_delta, r, c;
    int other_color;
    int flip_cnt; /* Number of stones that would be flipped */

    row_delta = reversi_row_delta(direction);
    col_delta = reversi_column_delta(direction);
    other_color = reversi_flipped_color(player);

    r = row + row_delta;
    c = col + col_delta;

    flip_cnt = 0;
    while (reversi_is_position_on_board(r, c) &&
            (game->board[r][c] == other_color)) {
        r += row_delta;
        c += col_delta;
        flip_cnt++;
    }

    if ((flip_cnt > 0) && reversi_is_position_on_board(r, c) &&
            (game->board[r][c] == player)) {
        return direction;
    } else {
        return 0;
    }
}


/* Checks whether the move at the specified position would be valid.
 * Params:
 *   - game: current state of the game
 *   - row: 0-based row number of the move in question
 *   - col: 0-based column number of the move in question
 *   - player: who is about to make the move (WHITE/BLACK)
 *
 * Checks if the place is empty, the coordinates are legal,
 * and some stones can be captured.
 *
 * Returns 0 if the move is not valid or, otherwise, the or'd
 * directions in which stones would be captured.
 */
int reversi_is_valid_move(const reversi_board_t *game,
        const int row, const int col, const int player) {
    int dirs, i;
    dirs = 0;

    /* Check if coordinates are legal */
    if (!reversi_is_position_on_board(row, col)) {
        return dirs;
    }

    /* Check if the place is free */
    if (game->board[row][col] != FREE) {
        return dirs;
    }

    /* Check the directions of capture */
    for (i = 0; i < NUM_OF_DIRECTIONS; i++) {
        dirs |= reversi_is_valid_direction(game, row, col, player, DIRECTIONS[i]);
    }

    return dirs;
}


/* Flips the stones in the specified direction after the specified
 * player has placed a stone in the specified cell. The move is
 * assumed to be valid.
 *
 * Returns the number of flipped stones in that direction
 */
static int reversi_flip_stones(reversi_board_t *game,
        const int row, const int col, const int player, const int direction) {
    int row_delta, col_delta, r, c;
    int other_color;
    int flip_cnt; /* Number of stones flipped */

    row_delta = reversi_row_delta(direction);
    col_delta = reversi_column_delta(direction);
    other_color = reversi_flipped_color(player);

    r = row + row_delta;
    c = col + col_delta;

    flip_cnt = 0;
    while (reversi_is_position_on_board(r, c) &&
            (game->board[r][c] == other_color)) {
        game->board[r][c] = player;
        r += row_delta;
        c += col_delta;
        flip_cnt++;
    }

    return flip_cnt;
}


/* Tries to make a move (place a stone) at the specified position.
 * If the move is valid the board is changed. Otherwise nothing happens.
 *
 * Params:
 *   - game: current state of the game
 *   - row: 0-based row number of the move to make
 *   - col: 0-based column number of the move to make
 *   - player: who makes the move (WHITE/BLACK)
 *
 * Returns the number of flipped (captured) stones (>0) iff the move
 * was valid or 0 if the move was not valid. Note that in the case of
 * a valid move, the stone itself is not counted.
 */
int reversi_make_move(reversi_board_t *game,
        const int row, const int col, const int player) {
    int dirs, cnt, i;

    dirs = reversi_is_valid_move(game, row, col, player);
    if (dirs == 0) {
        return 0;
    }

    /* Place the stone into the cell */
    game->board[row][col] = player;

    /* Capture stones in all possible directions */
    cnt = 0;
    for (i = 0; i < NUM_OF_DIRECTIONS; i++) {
        if (dirs & DIRECTIONS[i]) {
            cnt += reversi_flip_stones(game, row, col, player, DIRECTIONS[i]);
        }
    }

    /* Remember the move */
    i = reversi_count_moves(game);
    game->history[i-1] = MAKE_MOVE(row, col, player);

    return cnt;
}
