/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2007 Antoine Cellerier
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

#include "reversi-strategy.h"

/**
 * Simple strategy:
 *   A very simple strategy. Makes the highest scoring move taking the other
 *   player's next best move into account.
 *   From Algorithm by Claudio Clemens in hinversi, simpleClient.c
 */

static void reversi_copy_board(reversi_board_t *dst,
        const reversi_board_t *src) {
    int i;
    dst->rb = src->rb;
    dst->rb->memcpy(dst->history,src->history,
                    (BOARD_SIZE*BOARD_SIZE - INIT_STONES)*sizeof(move_t));
    for(i=0;i<BOARD_SIZE;i++) {
        dst->rb->memcpy(dst->board[i],src->board[i],BOARD_SIZE*sizeof(int));
    }
}

static int reversi_sim_move(const reversi_board_t *game, const int row,
        const int col, const int player) {
    reversi_board_t game_clone;
    reversi_copy_board(&game_clone,game);
    return reversi_make_move(&game_clone,row,col,player);
}

static int reversi_get_max_moves(const reversi_board_t *game, int player) {
    int max = -BOARD_SIZE*BOARD_SIZE;
    int row, col;
    for(row=0; row<BOARD_SIZE; row++) {
        for(col=0; col<BOARD_SIZE; col++) {
            int v = reversi_sim_move(game,row,col,player);
            if(v>max)
                max = v;
        }
    }
    return max;
}

static int reversi_sim_move2(const reversi_board_t *game, const int row,
        const int col, const int player) {
    /* Takes the other player's next best move into account */
    int score;
    reversi_board_t game_clone;
    reversi_copy_board(&game_clone,game);
    score = reversi_make_move(&game_clone,row,col,player);
    return score - reversi_get_max_moves(&game_clone,
                                         reversi_flipped_color(player));
}


static move_t simple_move_func(const reversi_board_t *game, int player) {
    int max = -BOARD_SIZE*BOARD_SIZE;
    int count = 0;
    int row, col;
    int r;
    for(row=0; row<BOARD_SIZE; row++) {
        for(col=0; col<BOARD_SIZE; col++) {
            int v = reversi_sim_move2(game,row,col,player);
            if(v>max) {
                max = v;
                count = 1;
            }
            else if(v==max) {
                count ++;
            }
        }
    }

    if(!count) return MOVE_INVALID;

    /* chose one of the moves which scores highest */
    r = game->rb->rand()%count;
    row = 0;
    col = 0;
    while(true) {
        if(reversi_sim_move2(game, row, col, player)==max) {
            r--;
            if(r<0) {
                return MAKE_MOVE(row,col,player);
            }
        }
        col ++;
        if(col==BOARD_SIZE) {
            col = 0;
            row ++;
            if(row==BOARD_SIZE) {
                row = 0;
            }
        }
    }
}

const game_strategy_t strategy_simple = {
    true,
    NULL,
    simple_move_func
};
