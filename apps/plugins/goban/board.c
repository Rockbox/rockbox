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

#include "goban.h"
#include "board.h"
#include "display.h"
#include "types.h"
#include "game.h"
#include "util.h"

#include "plugin.h"

unsigned int board_width = MAX_BOARD_SIZE;
unsigned int board_height = MAX_BOARD_SIZE;

/* Board has 'invalid' markers around each border */
unsigned char board_data[(MAX_BOARD_SIZE + 2) * (MAX_BOARD_SIZE + 2)];
int white_captures = 0;
int black_captures = 0;

uint8_t board_marks[(MAX_BOARD_SIZE + 2) * (MAX_BOARD_SIZE + 2)];
uint8_t current_mark = 255;

/* there can't be any changes off of the board, so no need to add the
   borders */
uint8_t board_changes[MAX_BOARD_SIZE * MAX_BOARD_SIZE];

unsigned short ko_pos = INVALID_POS;

/* forward declarations */
static void setup_marks (void);
static void make_mark (unsigned short pos);
static int get_liberties_helper (unsigned short pos, unsigned char orig_color);
static int flood_fill_helper (unsigned short pos, unsigned char orig_color,
                              unsigned char color);


/* these aren't "board marks" in the marks on the SGF sense, they are used
   internally to mark already visited points and the like (such as when
   doing liberty counting for groups) */
static void
setup_marks (void)
{
    unsigned int x, y;

    current_mark++;

    if (current_mark == 0)
    {
        current_mark++;

        for (y = 0; y < board_height; ++y)
        {
            for (x = 0; x < board_width; ++x)
            {
                board_marks[POS (x, y)] = 0;
            }
        }
    }
}

static void
make_mark (unsigned short pos)
{
    board_marks[pos] = current_mark;
}

static bool
is_marked (unsigned short pos)
{
    return board_marks[pos] == current_mark;
}

void
clear_board (void)
{
    unsigned int i, x, y;

    /* for the borders */
    for (i = 0; i < (2 + MAX_BOARD_SIZE) * (2 + MAX_BOARD_SIZE); ++i)
    {
        board_data[i] = INVALID;
    }

    /* now make the actual board part */
    for (y = 0; y < board_height; ++y)
    {
        for (x = 0; x < board_width; ++x)
        {
            board_data[POS (x, y)] = EMPTY;
        }
    }

    white_captures = 0;
    black_captures = 0;

    ko_pos = INVALID_POS;
}

bool
set_size_board (int width, int height)
{
    if (width < MIN_BOARD_SIZE || width > MAX_BOARD_SIZE ||
        height < MIN_BOARD_SIZE || height > MAX_BOARD_SIZE)
    {
        return false;
    }
    else
    {
        board_width = width;
        board_height = height;
        setup_display ();
        return true;
    }
}

unsigned char
get_point_board (unsigned short pos)
{
    return board_data[pos];
}

void
set_point_board (unsigned short pos, unsigned char color)
{
    board_data[pos] = color;
}

bool
on_board (unsigned short pos)
{
    if (pos < POS (0, 0) ||
        pos > POS (board_width - 1, board_height - 1) ||
        get_point_board (pos) == INVALID)
    {
        return false;
    }
    else
    {
        return true;
    }
}

int
get_liberties_board (unsigned short pos)
{
    if (!on_board (pos) || get_point_board (pos) == EMPTY)
    {
        return -1;
    }

    setup_marks ();

    int ret_val = 0;
    unsigned char orig_color = get_point_board (pos);

    empty_stack (&parse_stack);
    push_pos_stack (&parse_stack, pos);

    /* Since we only ever test for liberties in order to determine
       captures and the like, there's no reason to count any liberties
       higher than 2 (we sometimes need to know if something has 1 liberty
       for dealing with ko) */
    while (pop_pos_stack (&parse_stack, &pos) && ret_val < 2)
    {
        ret_val += get_liberties_helper (NORTH (pos), orig_color);
        ret_val += get_liberties_helper (SOUTH (pos), orig_color);
        ret_val += get_liberties_helper (EAST (pos), orig_color);
        ret_val += get_liberties_helper (WEST (pos), orig_color);
    }

    /* if there's more than two liberties, the stack isn't empty, so empty
       it */
    empty_stack (&parse_stack);

    return ret_val;
}

static int
get_liberties_helper (unsigned short pos, unsigned char orig_color)
{
    if (on_board (pos) &&
        get_point_board (pos) != OTHER (orig_color) && !is_marked (pos))
    {
        make_mark (pos);

        if (get_point_board (pos) == EMPTY)
        {
            return 1;
        }
        else
        {
            push_pos_stack (&parse_stack, pos);
        }
    }

    return 0;
}


int
flood_fill_board (unsigned short pos, unsigned char color)
{
    if (!on_board (pos) || get_point_board (pos) == color)
    {
        return 0;
    }

    empty_stack (&parse_stack);

    int ret_val = 0;

    unsigned char orig_color = get_point_board (pos);

    set_point_board (pos, color);
    ++ret_val;
    push_pos_stack (&parse_stack, pos);

    while (pop_pos_stack (&parse_stack, &pos))
    {
        ret_val += flood_fill_helper (NORTH (pos), orig_color, color);
        ret_val += flood_fill_helper (SOUTH (pos), orig_color, color);
        ret_val += flood_fill_helper (EAST (pos), orig_color, color);
        ret_val += flood_fill_helper (WEST (pos), orig_color, color);
    }

    return ret_val;
}


static int
flood_fill_helper (unsigned short pos, unsigned char orig_color,
                   unsigned char color)
{
    if (on_board (pos) && get_point_board (pos) == orig_color)
    {
        set_point_board (pos, color);
        push_pos_stack (&parse_stack, pos);
        return 1;
    }

    return 0;
}

bool
legal_move_board (unsigned short pos, unsigned char color, bool allow_suicide)
{
    /* you can always pass */
    if (pos == PASS_POS)
    {
        return true;
    }

    if (!on_board (pos) || (color != BLACK && color != WHITE))
    {
        return false;
    }

    if (pos == ko_pos && color == current_player)
    {
        return false;
    }

    if (get_point_board (pos) != EMPTY)
    {
        return false;
    }

    /* don't need to save the current state, because it's always empty
       since we tested for that above */
    set_point_board (pos, color);

    /* if we have liberties, it can't be illegal */
    if (get_liberties_board (pos) > 0 ||
        /* if we can capture something, it can't be illegal */
        (get_point_board (NORTH (pos)) == OTHER (color) &&
         !get_liberties_board (NORTH (pos))) ||
        (get_point_board (SOUTH (pos)) == OTHER (color) &&
         !get_liberties_board (SOUTH (pos))) ||
        (get_point_board (EAST (pos)) == OTHER (color) &&
         !get_liberties_board (EAST (pos))) ||
        (get_point_board (WEST (pos)) == OTHER (color) &&
         !get_liberties_board (WEST (pos))) ||
        /* if we're allowed to suicide, only multi-stone suicide is legal
           (no ruleset allows single-stone suicide that I know of) */
        (allow_suicide && (get_point_board (NORTH (pos)) == color ||
                           get_point_board (SOUTH (pos)) == color ||
                           get_point_board (EAST (pos)) == color ||
                           get_point_board (WEST (pos)) == color)))
    {
        /* undo our previous set */
        set_point_board (pos, EMPTY);
        return true;
    }
    else
    {
        /* undo our previous set */
        set_point_board (pos, EMPTY);
        return false;
    }
}


unsigned short
WRAP (unsigned short pos)
{
    int x, y;
    if (on_board (pos))
    {
        return pos;
    }
    else
    {
        x = I (pos);
        y = J (pos);

        if (x < 0)
        {
            x = board_width - 1;
        }
        else if ((unsigned int) x >= board_width)
        {
            x = 0;
        }

        if (y < 0)
        {
            y = board_height - 1;
        }
        else if ((unsigned int) y >= board_height)
        {
            y = 0;
        }
        return POS (x, y);
    }
}
