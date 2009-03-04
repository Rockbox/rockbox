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

#include "board.h"
#include "goban.h"
#include "display.h"
#include "game.h"
#include "sgf.h"

/* for xlcd_filltriangle */
#include "lib/xlcd.h"

unsigned int intersection_size = 0;

#define LINE_OFFSET (intersection_size / 2)

/* pixel offsets for the board on the LCD */
int board_x = 0;
int board_y = 0;
int board_pixel_width = 0;
int board_pixel_height = 0;

/* current cursor position in board coordinates (intersections, not
   pixels) */
unsigned short cursor_pos = POS (0, 0);

/* we way need to "move" our notion of which intersections to draw when
   the cursor moves, since we can be zoomed in (and often will be) this is
   used to signal when we need to do that */
unsigned short last_cursor_pos = INVALID_POS;
unsigned int last_int_size = MAX_INT_SIZE + 1;

unsigned int min_x_int = 0;
unsigned int min_y_int = 0;
unsigned int num_x_ints = 0;
unsigned int num_y_ints = 0;

int extend_t = 0, extend_b = 0, extend_l = 0, extend_r = 0;

bool draw_variations = true;
unsigned int saved_circle_size = 0;
bool has_comment = false;

unsigned char display_marks[MAX_BOARD_SIZE * MAX_BOARD_SIZE];


/* function prototypes */

static int pixel_x (unsigned short pos);
static int pixel_y (unsigned short pos);

static void draw_circle (int c_x, int c_y, int r, bool filled);

static void draw_cursor (unsigned short pos);
static void draw_stone_raw (int pixel_x, int pixel_y, bool black);
static void draw_stone (unsigned short pos, bool black);
static void draw_all_stones (void);

static void draw_hoshi (unsigned short pos);
static void draw_footer (void);
static void draw_all_marks (void);
static void draw_all_hoshi (void);

static unsigned int unzoomed_int_size (void);
static void cursor_updated (void);

void
clear_marks_display (void)
{
    rb->memset (display_marks, ' ', sizeof (display_marks));
    has_comment = false;
}

void
set_mark_display (unsigned short pos, unsigned char mark_char)
{
    if (!on_board (pos))
    {
        return;
    }

    if ((mark_char == 'b' || mark_char == 'w') &&
        display_marks[I (pos) + J (pos) * board_width] != ' ')
    {
        /* don't overwrite real board marks with last-move or variation
           marks */
        return;
    }

    display_marks[I (pos) + J (pos) * board_width] = mark_char;
}


void
set_comment_display (bool new_val)
{
    has_comment = new_val;
}


static void
draw_all_marks (void)
{
    unsigned int x, y;
    for (x = MIN_X; x < MAX_X; ++x)
    {
        for (y = MIN_Y; y < MAX_Y; ++y)
        {
            if (display_marks[x + y * board_width] != ' ')
            {
#if LCD_DEPTH > 1
                if (display_marks[x + y * board_width] != 'b' &&
                    display_marks[x + y * board_width] != 'w')
                {
                    rb->lcd_set_foreground (MARK_COLOR);
                }
                else
                {
                    rb->lcd_set_foreground (CURSOR_COLOR);
                }
                rb->lcd_set_drawmode (DRMODE_FG);
#else
                rb->lcd_set_drawmode (DRMODE_FG + DRMODE_COMPLEMENT);
#endif

                if (display_marks[x + y * board_width] & (1 << 7))
                {
                    char to_display[2];
                    int width, height;

                    to_display[0] =
                        display_marks[x + y * board_width] & (~(1 << 7));
                    to_display[1] = '\0';

                    rb->lcd_getstringsize (to_display, &width, &height);

                    int display_x =
                        pixel_x (POS (x, y)) + LINE_OFFSET - (width / 2);
                    int display_y =
                        pixel_y (POS (x, y)) + LINE_OFFSET - (height / 2);

                    if (display_x < 0)
                    {
                        display_x = 0;
                    }

                    if (display_y < 0)
                    {
                        display_y = 0;
                    }

                    if (display_x + width >= LCD_WIDTH)
                    {
                        display_x = LCD_WIDTH - 1 - width;
                    }

                    if (display_y + height >= LCD_HEIGHT)
                    {
                        display_y = LCD_HEIGHT - 1 - height;
                    }

                    rb->lcd_putsxy (display_x, display_y, to_display);
                    continue;
                }

                switch (display_marks[x + y * board_width])
                {
                    /* moves, 'mark', 'square' */
                case 'b':
                case 'w':
                    if (intersection_size <= 5)
                    {
                        DEBUGF ("screen is too small to mark current move\n");
                        break;
                    }
                case 'm':
                    if (intersection_size <= 5)
                    {
                        rb->lcd_drawpixel (pixel_x (POS (x, y)) + LINE_OFFSET +
                                           1,
                                           pixel_y (POS (x, y)) + LINE_OFFSET +
                                           1);
                        rb->lcd_drawpixel (pixel_x (POS (x, y)) + LINE_OFFSET -
                                           1,
                                           pixel_y (POS (x, y)) + LINE_OFFSET -
                                           1);
                    }
                    else
                    {
                        rb->lcd_drawrect (pixel_x (POS (x, y)) + LINE_OFFSET -
                                          intersection_size / 6,
                                          pixel_y (POS (x, y)) + LINE_OFFSET -
                                          intersection_size / 6,
                                          (intersection_size / 6) * 2 + 1,
                                          (intersection_size / 6) * 2 + 1);
                    }
                    break;
                case 's':
                    if (intersection_size <= 5)
                    {
                        rb->lcd_drawpixel (pixel_x (POS (x, y)) + LINE_OFFSET +
                                           1,
                                           pixel_y (POS (x, y)) + LINE_OFFSET +
                                           1);
                        rb->lcd_drawpixel (pixel_x (POS (x, y)) + LINE_OFFSET -
                                           1,
                                           pixel_y (POS (x, y)) + LINE_OFFSET -
                                           1);
                    }
                    else
                    {
                        rb->lcd_fillrect (pixel_x (POS (x, y)) + LINE_OFFSET -
                                          intersection_size / 6,
                                          pixel_y (POS (x, y)) + LINE_OFFSET -
                                          intersection_size / 6,
                                          (intersection_size / 6) * 2 + 1,
                                          (intersection_size / 6) * 2 + 1);
                    }
                    break;

                case 'c':
                    if (intersection_size > 7)
                    {
                        draw_circle (pixel_x (POS (x, y)) + LINE_OFFSET,
                                     pixel_y (POS (x, y)) + LINE_OFFSET,
                                     (intersection_size - 1) / 4, true);
                        break;
                    }

                    /* purposely don't break here, draw small the same as
                       a triangle */

                case 't':
                    if (intersection_size <= 7)
                    {
                        rb->lcd_drawpixel (pixel_x (POS (x, y)) + LINE_OFFSET -
                                           1,
                                           pixel_y (POS (x, y)) + LINE_OFFSET +
                                           1);
                        rb->lcd_drawpixel (pixel_x (POS (x, y)) + LINE_OFFSET +
                                           1,
                                           pixel_y (POS (x, y)) + LINE_OFFSET -
                                           1);
                    }
                    else
                    {
                        xlcd_filltriangle (pixel_x (POS (x, y)) + LINE_OFFSET,
                                           pixel_y (POS (x, y)) + LINE_OFFSET -
                                           intersection_size / 4,
                                           pixel_x (POS (x, y)) + LINE_OFFSET +
                                           intersection_size / 4,
                                           pixel_y (POS (x, y)) + LINE_OFFSET +
                                           intersection_size / 4,
                                           pixel_x (POS (x, y)) + LINE_OFFSET -
                                           intersection_size / 4,
                                           pixel_y (POS (x, y)) + LINE_OFFSET +
                                           intersection_size / 4);
                    }
                    break;
                default:
                    DEBUGF ("tried to display unknown mark '%c' %d\n",
                            display_marks[x + y * board_width],
                            display_marks[x + y * board_width]);
                    break;
                };

                rb->lcd_set_drawmode (DRMODE_SOLID);
                /* don't have to undo the colors for LCD_DEPTH > 1, most
                   functions assume bg and fg get clobbered */
            }
        }
    }
}


static void
draw_circle (int c_x, int c_y, int r, bool filled)
{
    int f = 1 - r;
    int x = 0;
    int y = r;

    /* draw the points on the axes to make the loop easier */
    rb->lcd_drawpixel (c_x, c_y + r);
    rb->lcd_drawpixel (c_x, c_y - r);

    if (filled)
    {
        rb->lcd_hline (c_x - r, c_x + r, c_y);
    }
    else
    {
        rb->lcd_drawpixel (c_x + r, c_y);
        rb->lcd_drawpixel (c_x - r, c_y);
    }

    /* Now walk from the very top of the circle to 1/8th of the way around
       to the right.  For each point, draw the 8 symmetrical points. */
    while (x < y)
    {
        /* walk one pixel to the right */
        ++x;

        /* And then adjust our discriminant, and adjust y if we've
           ventured outside of the circle. This boils down to walking a
           tightrope between being inside and outside the circle.  The
           updating functions are taken from expanding the discriminant
           function f(x, y) = x^2 + y^2 - r^2 after substituting in x + 1
           and y - 1 and then subtracting out f(x, y) */
        if (f <= 0)
        {
            f += 2 * x + 1;
        }
        else
        {
            --y;
            f += (x - y) * 2 + 1;
        }

        if (filled)
        {
            /* each line takes care of 2 points on the circle so we only
               need 4 */
            rb->lcd_hline (c_x - y, c_x + y, c_y + x);
            rb->lcd_hline (c_x - y, c_x + y, c_y - x);
            rb->lcd_hline (c_x - x, c_x + x, c_y + y);
            rb->lcd_hline (c_x - x, c_x + x, c_y - y);
        }
        else
        {
            /* Draw all 8 symmetrical points */
            rb->lcd_drawpixel (c_x + x, c_y + y);
            rb->lcd_drawpixel (c_x + y, c_y + x);
            rb->lcd_drawpixel (c_x + y, c_y - x);
            rb->lcd_drawpixel (c_x + x, c_y - y);
            rb->lcd_drawpixel (c_x - x, c_y + y);
            rb->lcd_drawpixel (c_x - y, c_y + x);
            rb->lcd_drawpixel (c_x - y, c_y - x);
            rb->lcd_drawpixel (c_x - x, c_y - y);
        }
    }
}


void
draw_screen_display (void)
{
#if LCD_DEPTH > 1
    int saved_fg = rb->lcd_get_foreground ();
    int saved_bg = rb->lcd_get_background ();
#endif
    int saved_drmode = rb->lcd_get_drawmode ();

    if (cursor_pos != last_cursor_pos || intersection_size != last_int_size)
    {
        cursor_updated ();
    }

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop (NULL);

    rb->lcd_set_foreground (BOARD_COLOR);
    rb->lcd_set_background (BACKGROUND_COLOR);
    rb->lcd_set_drawmode (DRMODE_SOLID);

#else
    rb->lcd_set_drawmode (DRMODE_SOLID + DRMODE_INVERSEVID);
#endif

    rb->lcd_clear_display ();

    rb->lcd_fillrect (pixel_x (POS (MIN_X, MIN_Y)),
                      pixel_y (POS (MIN_X, MIN_Y)),
                      (MAX_X - MIN_X) * intersection_size,
                      (MAX_Y - MIN_Y) * intersection_size);

#if LCD_DEPTH > 1
    rb->lcd_set_foreground (LINE_COLOR);
#else
    rb->lcd_set_drawmode (DRMODE_SOLID);
#endif

    unsigned int i;
    for (i = MIN_Y; i < MAX_Y; ++i)
    {
        rb->lcd_hline (pixel_x (POS (MIN_X, i)) + LINE_OFFSET + extend_l,
                       pixel_x (POS (MAX_X - 1, i)) + LINE_OFFSET + extend_r,
                       pixel_y (POS (MIN_X, i)) + LINE_OFFSET);
    }

    for (i = MIN_X; i < MAX_X; ++i)
    {
        rb->lcd_vline (pixel_x (POS (i, MIN_Y)) + LINE_OFFSET,
                       pixel_y (POS (i, MIN_Y)) + LINE_OFFSET + extend_t,
                       pixel_y (POS (i, MAX_Y - 1)) + LINE_OFFSET + extend_b);
    }

    draw_all_hoshi ();
    draw_all_stones ();
    draw_cursor (cursor_pos);

    if (draw_variations)
    {
        mark_child_variations_sgf ();
    }

    draw_all_marks ();

    draw_footer ();
    rb->lcd_update ();

#if LCD_DEPTH > 1
    rb->lcd_set_foreground (saved_fg);
    rb->lcd_set_background (saved_bg);
#endif
    rb->lcd_set_drawmode (saved_drmode);
}



#if defined(GBN_WIDE_SCREEN)

/* the size of the string, in pixels, when drawn vertically */
static void
vert_string_size (char *string, int *width, int *height)
{
    int temp_width = 0;
    int temp_height = 0;

    int ret_width = 0;
    int ret_height = 0;

    char temp_buffer[2];

    temp_buffer[0] = temp_buffer[1] = 0;

    if (!string)
    {
        return;
    }

    while (*string)
    {
        temp_buffer[0] = *string;
        rb->lcd_getstringsize (temp_buffer, &temp_width, &temp_height);

        ret_height += temp_height;

        if (ret_width < temp_width)
        {
            ret_width = temp_width;
        }

        ++string;
    }

    if (width)
    {
        *width = ret_width;
    }

    if (height)
    {
        *height = ret_height;
    }
}

static void
putsxy_vertical (int x, int y, int width, char *str)
{
    int temp_width = 0;
    int temp_height = 0;
    char temp_buffer[2];

    temp_buffer[0] = temp_buffer[1] = 0;

    if (!str)
    {
        return;
    }

    while (*str)
    {
        temp_buffer[0] = *str;
        rb->lcd_getstringsize (temp_buffer, &temp_width, &temp_height);
        DEBUGF ("putting %s at %d %d\n", temp_buffer,
                x + (width - temp_width) / 2, y);
        rb->lcd_putsxy (x + (width - temp_width) / 2, y, temp_buffer);
        y += temp_height;
        ++str;
    }
}

#endif /* GBN_WIDE_SCREEN */


static void
draw_footer (void)
{
    char captures_buffer[16];
    char display_flags[16] = "";
    int size_x, size_y;
    int vert_x, vert_y;

    (void) vert_x;
    (void) vert_y;

#if LCD_DEPTH > 1
    rb->lcd_set_background (BACKGROUND_COLOR);
    rb->lcd_set_foreground (BLACK_COLOR);
#else
    rb->lcd_set_drawmode (DRMODE_SOLID + DRMODE_INVERSEVID);
#endif

    rb->snprintf (captures_buffer, sizeof (captures_buffer),
                  "%d", white_captures);


    rb->lcd_getstringsize (captures_buffer, &size_x, &size_y);
#if defined(GBN_TALL_SCREEN)
    rb->lcd_putsxy (size_y + 2, LCD_HEIGHT - size_y, captures_buffer);
#else
    vert_string_size (captures_buffer, &vert_x, &vert_y);
    if (board_pixel_width + size_x <= LCD_WIDTH)
    {
        rb->lcd_putsxy (LCD_WIDTH - size_x - 1, vert_x + 2, captures_buffer);
    }
    else
    {
        putsxy_vertical (LCD_WIDTH - vert_x - 1, vert_x + 2, vert_x,
                         captures_buffer);
    }
#endif

#if LCD_DEPTH == 1
    rb->lcd_set_drawmode (DRMODE_SOLID);
#endif

#if defined(GBN_TALL_SCREEN)
    draw_circle (size_y / 2,
                 LCD_HEIGHT - (size_y / 2), (size_y - 1) / 2, true);

#if LCD_DEPTH == 1
    rb->lcd_set_drawmode (DRMODE_SOLID + DRMODE_INVERSEVID);
    draw_circle (size_y / 2,
                 LCD_HEIGHT - (size_y / 2), (size_y - 1) / 2, false);
#endif /* LCD_DEPTH */

#else /* !GBN_TALL_SCREEN */
    draw_circle (LCD_WIDTH - 1 - vert_x / 2,
                 (vert_x / 2), (vert_x - 1) / 2, true);

#if LCD_DEPTH == 1
    rb->lcd_set_drawmode (DRMODE_SOLID + DRMODE_INVERSEVID);
    draw_circle (LCD_WIDTH - 1 - vert_x / 2,
                 (vert_x / 2), (vert_x - 1) / 2, false);
#endif /* LCD_DEPTH */

#endif /* GBN_TALL_SCREEN */


#if LCD_DEPTH > 1
    rb->lcd_set_foreground (WHITE_COLOR);
#endif
    rb->snprintf (captures_buffer, sizeof (captures_buffer),
                  "%d", black_captures);

    rb->lcd_getstringsize (captures_buffer, &size_x, &size_y);
#if defined(GBN_TALL_SCREEN)
    rb->lcd_putsxy (LCD_WIDTH - (size_y + 1) - size_x,
                    LCD_HEIGHT - size_y, captures_buffer);

    draw_circle (LCD_WIDTH - (size_y / 2),
                 LCD_HEIGHT - (size_y / 2), (size_y - 1) / 2, true);
#else
    vert_string_size (captures_buffer, &vert_x, &vert_y);
    if (board_pixel_width + size_x <= LCD_WIDTH)
    {
        rb->lcd_putsxy (LCD_WIDTH - size_x - 1,
                        LCD_HEIGHT - vert_x - size_y - 3, captures_buffer);
    }
    else
    {
        putsxy_vertical (LCD_WIDTH - vert_x - 1,
                         LCD_HEIGHT - vert_x - 3 - vert_y,
                         vert_x, captures_buffer);
    }

    draw_circle (LCD_WIDTH - 1 - vert_x / 2,
                 LCD_HEIGHT - 1 - vert_x / 2, (vert_x - 1) / 2, true);
#endif


#if LCD_DEPTH > 1
    rb->lcd_set_foreground (BLACK_COLOR);
#endif

    if (has_comment)
    {
        rb->strcat (display_flags, "C");
    }

    if (has_more_nodes_sgf ())
    {
        rb->strcat (display_flags, "+");
    }

    if (num_variations_sgf () > 1)
    {
        rb->strcat (display_flags, "*");
    }



    rb->snprintf (captures_buffer, sizeof (captures_buffer),
                  "%d%s", move_num, display_flags);

    rb->lcd_getstringsize (captures_buffer, &size_x, &size_y);
#if defined(GBN_TALL_SCREEN)
    rb->lcd_putsxy ((LCD_WIDTH - size_x) / 2,
                    LCD_HEIGHT - size_y, captures_buffer);
#else
    if (board_pixel_width + size_x <= LCD_WIDTH)
    {
        rb->lcd_putsxy (LCD_WIDTH - size_x - 1,
                        (LCD_HEIGHT - size_y) / 2, captures_buffer);
    }
    else
    {
        vert_string_size (captures_buffer, &vert_x, &vert_y);
        putsxy_vertical (LCD_WIDTH - vert_x - 1,
                         (LCD_HEIGHT - vert_y) / 2, vert_x, captures_buffer);
    }
#endif


    rb->lcd_set_drawmode (DRMODE_SOLID);
}







static int
pixel_x (unsigned short pos)
{
    return board_x + (I (pos) - min_x_int) * intersection_size;
}

static int
pixel_y (unsigned short pos)
{
    return board_y + (J (pos) - min_y_int) * intersection_size;
}



void
move_display (unsigned short pos)
{
    if (!on_board (pos))
    {
        return;
    }

    while ((unsigned) I (pos) >= REAL_MAX_X)
    {
        cursor_pos = EAST (cursor_pos);
        cursor_updated ();
    }
    while ((unsigned) I (pos) < REAL_MIN_X)
    {
        cursor_pos = WEST (cursor_pos);
        cursor_updated ();
    }

    while ((unsigned) J (pos) >= REAL_MAX_Y)
    {
        cursor_pos = SOUTH (cursor_pos);
        cursor_updated ();
    }
    while ((unsigned) J (pos) < REAL_MIN_Y)
    {
        cursor_pos = NORTH (cursor_pos);
        cursor_updated ();
    }
}


static void
cursor_updated (void)
{
    if (!on_board(cursor_pos))
    {
        cursor_pos = WRAP (cursor_pos);
    }

    if (intersection_size != last_int_size ||
        ((unsigned) I (cursor_pos)) < REAL_MIN_X + 1 ||
        ((unsigned) I (cursor_pos)) > REAL_MAX_X - 2 ||
        ((unsigned) J (cursor_pos)) < REAL_MIN_Y + 1 ||
        ((unsigned) J (cursor_pos)) > REAL_MAX_Y - 2)
    {
        if ((unsigned) I (cursor_pos) < (num_x_ints / 2))
        {
            min_x_int = 0;
        }
        else
        {
            min_x_int = min (I (cursor_pos) - (num_x_ints / 2),
                             board_width - num_x_ints);
        }

        if ((unsigned) J (cursor_pos) < (num_y_ints / 2))
        {
            min_y_int = 0;
        }
        else
        {

            min_y_int = min (J (cursor_pos) - (num_y_ints / 2),
                             board_height - num_y_ints);
        }
    }

    /* these are used in line drawing to extend the lines if there is more
       board in that direction */
    if (MIN_X)
    {
        extend_l = -1 * LINE_OFFSET;
    }
    else
    {
        extend_l = 0;
    }

    if (MIN_Y)
    {
        extend_t = -1 * LINE_OFFSET;
    }
    else
    {
        extend_t = 0;
    }

    if (MAX_X != board_width)
    {
        extend_r = LINE_OFFSET;
    }
    else
    {
        extend_r = 0;
    }

    if (MAX_Y != board_height)
    {
        extend_b = LINE_OFFSET;
    }
    else
    {
        extend_b = 0;
    }

    last_cursor_pos = cursor_pos;
    last_int_size = intersection_size;
}

static unsigned int
unzoomed_int_size (void)
{
    int int_size = min ((LCD_BOARD_WIDTH / board_width),
                        (LCD_BOARD_HEIGHT / board_height));

    if (!(int_size & 1))
    {
        --int_size;
    }

    if (int_size < 0)
    {
        int_size = 1;
    }

    return max(int_size, MIN_INT_SIZE);
}

unsigned int
current_zoom_display (void)
{
    return (intersection_size - unzoomed_int_size ()) / 2 + 1;
}

unsigned int
max_zoom_display (void)
{
    return (MAX_INT_SIZE - unzoomed_int_size ()) / 2 + 1;
}

unsigned int
min_zoom_display (void)
{
    if (MIN_INT_SIZE >= unzoomed_int_size())
    {
        return (MIN_INT_SIZE - unzoomed_int_size ()) / 2 + 1;
    }
    else
    {
        return 1;
    }
}

void
set_zoom_display (unsigned int zoom_level)
{
    unsigned int unzoomed = unzoomed_int_size ();

    if (saved_circle_size < MIN_INT_SIZE ||
        saved_circle_size > MAX_INT_SIZE)
    {
        saved_circle_size = MIN_DEFAULT_INT_SIZE;
    }

    if (zoom_level == 0)
    {
        /* default zoom, we get to set it however we want */
        intersection_size = max (unzoomed, saved_circle_size);
    }
    else
    {
        intersection_size = unzoomed + 2 * (zoom_level - 1);
    }

    if (intersection_size > MAX_INT_SIZE)
    {
        intersection_size = MAX_INT_SIZE;
    }

    /* now intersection_size has been set appropriately, so set up all of
       the derived values used for display */

    num_x_ints = min (LCD_BOARD_WIDTH / intersection_size, board_width);
    num_y_ints = min (LCD_BOARD_HEIGHT / intersection_size, board_height);

    board_pixel_width = num_x_ints * intersection_size;
    board_pixel_height = num_y_ints * intersection_size;

#if defined(GBN_TALL_SCREEN)
    board_x = (LCD_WIDTH - board_pixel_width) / 2;
    board_y = 0;
#elif defined(GBN_WIDE_SCREEN)
    board_x = 0;
    board_y = (LCD_HEIGHT - board_pixel_height) / 2;
#else
#error screen dimensions have not been evaluated properly
#endif
}


/* Call every time the board size might have changed! */
void
setup_display (void)
{
    set_zoom_display (0);       /* 0 means set to default */

    /* The cursor starts out in the top right of the board
     * (on the hoshi point for most board sizes), unless the board
     * is really small in which case the cursor starts at the center
     * of the board.
     */
    int start_x, start_y;
    if (board_width >= 7)
    {
        start_x = board_width - 4;
    }
    else
    {
        start_x = board_width / 2;
    }

    if (board_height >= 7)
    {
        start_y = 3;
    }
    else
    {
        start_y = board_height / 2;
    }
    cursor_pos = POS (start_x, start_y);
    last_cursor_pos = INVALID_POS;
    last_int_size = -1;

    clear_marks_display ();
}

static void
draw_cursor (unsigned short pos)
{
    if (!on_board (pos))
    {
        return;
    }

#if LCD_DEPTH > 1
    rb->lcd_set_foreground (CURSOR_COLOR);
#else
    rb->lcd_set_drawmode (DRMODE_COMPLEMENT);
#endif

    rb->lcd_drawrect (pixel_x (pos),
                      pixel_y (pos), intersection_size, intersection_size);

    rb->lcd_set_drawmode (DRMODE_SOLID);
}

static void
draw_stone_raw (int pixel_x, int pixel_y, bool black)
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground (black ? BLACK_COLOR : WHITE_COLOR);
#else
    if (black)
    {
        rb->lcd_set_drawmode (DRMODE_SOLID);
    }
    else
    {
        rb->lcd_set_drawmode (DRMODE_SOLID + DRMODE_INVERSEVID);
    }
#endif

    draw_circle (pixel_x + LINE_OFFSET,
                 pixel_y + LINE_OFFSET, LINE_OFFSET, true);

#if defined(OUTLINE_STONES)
#if LCD_DEPTH > 1
    rb->lcd_set_foreground (black ? WHITE_COLOR : BLACK_COLOR);
#else
    if (black)
    {
        rb->lcd_set_drawmode (DRMODE_SOLID + DRMODE_INVERSEVID);
    }
    else
    {
        rb->lcd_set_drawmode (DRMODE_SOLID);
    }
#endif /* LCD_DEPTH > 1 */

    if (!black)
    {
        draw_circle (pixel_x + LINE_OFFSET,
                     pixel_y + LINE_OFFSET, LINE_OFFSET, false);
    }

#endif /* OUTLINE_STONES */

    rb->lcd_set_drawmode (DRMODE_SOLID);
}


static void
draw_stone (unsigned short pos, bool black)
{
    if (!on_board (pos))
    {
        return;
    }

    draw_stone_raw (pixel_x (pos), pixel_y (pos), black);
}


static void
draw_all_stones (void)
{
    unsigned int x, y;
    unsigned short temp_pos;

    for (x = MIN_X; x < MAX_X; ++x)
    {
        for (y = MIN_Y; y < MAX_Y; ++y)
        {
            temp_pos = POS (x, y);
            if (get_point_board (temp_pos) == EMPTY)
            {
                continue;
            }

            draw_stone (temp_pos, get_point_board (temp_pos) == BLACK);
        }
    }
}

static void
draw_hoshi (unsigned short pos)
{
    /* color and drawmode are already set before this function (all lines
       and hoshi and stuff are drawn together) */

    if (!on_board(pos))
    {
        return;
    }

    if ((unsigned) I (pos) < MIN_X ||
        (unsigned) I (pos) >= MAX_X ||
        (unsigned) J (pos) < MIN_Y ||
        (unsigned) J (pos) >= MAX_Y)
    {
        return;
    }
    if (intersection_size > 8)
    {
        rb->lcd_fillrect (pixel_x (pos) + LINE_OFFSET - 1,
                          pixel_y (pos) + LINE_OFFSET - 1, 3, 3);
    }
    else
    {
        rb->lcd_drawpixel (pixel_x (pos) + LINE_OFFSET - 1,
                           pixel_y (pos) + LINE_OFFSET - 1);
        rb->lcd_drawpixel (pixel_x (pos) + LINE_OFFSET + 1,
                           pixel_y (pos) + LINE_OFFSET + 1);
    }
}


static void
draw_all_hoshi (void)
{
    if (board_width != board_height)
    {
        return;
    }

    if (board_width == 19)
    {
        draw_hoshi (POS (3, 3));
        draw_hoshi (POS (3, 9));
        draw_hoshi (POS (3, 15));

        draw_hoshi (POS (9, 3));
        draw_hoshi (POS (9, 9));
        draw_hoshi (POS (9, 15));

        draw_hoshi (POS (15, 3));
        draw_hoshi (POS (15, 9));
        draw_hoshi (POS (15, 15));
    }
    else if (board_width == 9)
    {
        draw_hoshi (POS (2, 2));
        draw_hoshi (POS (2, 6));

        draw_hoshi (POS (4, 4));

        draw_hoshi (POS (6, 2));
        draw_hoshi (POS (6, 6));
    }
    else if (board_width == 13)
    {
        draw_hoshi (POS (3, 3));
        draw_hoshi (POS (3, 9));

        draw_hoshi (POS (6, 6));

        draw_hoshi (POS (9, 3));
        draw_hoshi (POS (9, 9));

    }
}
