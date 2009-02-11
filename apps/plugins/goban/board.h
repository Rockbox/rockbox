/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2009 Joshua Simmons <mud at majidejima dot com>
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

#ifndef GOBAN_BOARD_H
#define GOBAN_BOARD_H

#include "types.h"
#include "goban.h"              /* for LCD_BOARD_SIZE */

#define WHITE 1
#define BLACK 2
#define EMPTY 0
#define NONE 0
#define INVALID 4

#define OTHER(color) (color == BLACK ? WHITE : BLACK)

                       /* MAX_BOARD_SIZE no longer dependent on screen
                          size since zooming was implemented */
#define MAX_BOARD_SIZE 19
#define MIN_BOARD_SIZE 1

#define INVALID_POS ((unsigned short) -5003)
#define PASS_POS ((unsigned short) -5002)

#define POS(i, j) ((i + 1) + (j + 1) * (MAX_BOARD_SIZE + 2))
#define WEST(i) (i - 1)
#define EAST(i) (i + 1)
#define NORTH(i) (i - (MAX_BOARD_SIZE + 2))
#define SOUTH(i) (i + (MAX_BOARD_SIZE + 2))

unsigned short WRAP (unsigned short pos);

#define I(pos) (pos % (MAX_BOARD_SIZE + 2) - 1)
#define J(pos) (pos / (MAX_BOARD_SIZE + 2) - 1)

/* Clear the data from the board, including marks and such.  Should be
   called after setting the board size */
void clear_board (void);

/* Set the size of the board.  Follow with a call to clear_board() and a
   call to setup_display(). Returns false on failure (generally, invalid
   width or height) */
bool set_size_board (int width, int height);

/* Returns true if the given move is legal. allow_suicide should be true
   if suicide is a valid move with the current ruleset */
bool legal_move_board (unsigned short pos, unsigned char color,
                       bool allow_suicide);

/* Returns true if the pos is on the board */
bool on_board (unsigned short pos);

/* Get the color on the board at the given pos. Should be
   BLACK/WHITE/EMPTY, and sometimes INVALID. */
unsigned char get_point_board (unsigned short pos);

/* Set the color of point at pos, which must be on the board */
void set_point_board (unsigned short pos, unsigned char color);

/* Get the number of liberties of the group of which pos is a stone.
   Returns less than zero if pos is empty. If the number of liberties of
   the group is greater than 2, 2 is returned. */
int get_liberties_board (unsigned short pos);

/* A simple flood fill algorithm for capturing or uncapturing stones.
   Returns the number of locations changed. */
int flood_fill_board (unsigned short pos, unsigned char color);

/* The size of the board */
extern unsigned int board_width;
extern unsigned int board_height;

/* The number of captures for each player */
extern int black_captures;
extern int white_captures;

/* If there is a ko which cannot be retaken, this is set to the point
   which may not be played at.  Otherwise this is INVALID_POS. */
extern unsigned short ko_pos;

#endif
