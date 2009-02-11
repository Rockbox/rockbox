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

#ifndef GOBAN_DISPLAY_H
#define GOBAN_DISPLAY_H

#include "types.h"
#include "goban.h"

/* Call before using the display */
void setup_display (void);

/* Draw the board and the "footer" */
void draw_screen_display (void);

/* The location of the cursor */
extern unsigned short cursor_pos;

/* True if we should draw variations */
extern bool draw_variations;

/* Used to set the zoom level, loaded in from the config file */
unsigned int saved_circle_size;

/* the size of one intersection on the board, in pixels */
unsigned int intersection_size;

/* Clear the marks from the board */
void clear_marks_display (void);

/* Add a mark to the display */
void set_mark_display (unsigned short pos, unsigned char mark_char);

/* Set to indicate if we should display the 'C' in the footer or not */
void set_comment_display (bool new_val);

/* Move the display so that the position pos is in view, very useful if
   we're zoomed in (otherwise does nothing) */
void move_display (unsigned short pos);

/* These should all be obvious.  Zoom levels start at 1. set_zoom_display
   will set the default zoom level if called with zoom_level == 0 */
void set_zoom_display (unsigned int zoom_level);
unsigned int current_zoom_display (void);
unsigned int min_zoom_display (void);
unsigned int max_zoom_display (void);

/* MIN and MAX intersection sizes */
#define MIN_DEFAULT_INT_SIZE (11)

/* The absolute minimum that is allowed */
#define MIN_INT_SIZE (3)

/* Don't allow one bigger than the size of the screen */
#define MAX_INT_SIZE (min((LCD_BOARD_WIDTH & 1) ? LCD_BOARD_WIDTH : \
                                                  LCD_BOARD_WIDTH - 1, \
                          (LCD_BOARD_HEIGHT & 1) ? LCD_BOARD_HEIGHT : \
                                                   LCD_BOARD_HEIGHT - 1))




/* NOTE: we do one "extra" intersection in each direction which goes off
   the screen, because it makes drawing a little bit prettier (the board
   doesn't end before the edge of the screen this way) */


/* right is a screen boundary if we're on a tall screen, bottom is a
   screen boundary if we're on a wide screen */
#if defined(GBN_TALL_SCREEN)
#define MIN_X ((min_x_int == 0) ? 0 : min_x_int - 1)
#define MIN_Y (min_y_int)
/* always flush with top, no need for extra */

#define MAX_X (min(min_x_int + num_x_ints + 1, board_width))
#define MAX_Y (min(min_y_int + num_y_ints, board_height))
#else
#define MIN_X (min_x_int)
/* always flush with left, no need for extra */

#define MIN_Y ((min_y_int == 0) ? 0 : min_y_int - 1)
#define MAX_X (min(min_x_int + num_x_ints, board_width))
#define MAX_Y (min(min_y_int + num_y_ints + 1, board_height))
#endif

/* These are the same as above, except without the extra intersection is
   board-boundary directions */
#define REAL_MIN_X (min_x_int)
#define REAL_MIN_Y (min_y_int)
#define REAL_MAX_X (min(min_x_int + num_x_ints, board_width))
#define REAL_MAX_Y (min(min_y_int + num_y_ints, board_height))

#endif
