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
GUI part of reversi. Code is inspired by sudoku code by Dave Chapman
which is copyright (c) 2005 Dave Chapman and is released under the
GNU General Public License.


User instructions
-----------------

Use the arrow keys to move cursor, and press TOGGLE to place a stone.

At any time during the game, press MENU to bring up the game menu with
further options:

  - Save
  - Reload
  - Clear

*/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

#include "reversi-game.h"
#include "reversi-strategy.h"
#include "reversi-gui.h"

#include "lib/playback_control.h"

PLUGIN_HEADER

/* Where the board begins */
#define XOFS 4
#define YOFS 4

#if LCD_HEIGHT > LCD_WIDTH
#define MARGIN_W    (XOFS*2+1)
#define MARGIN_H    (YOFS*2+1)
#define MARGIN_C_W  0
#define MARGIN_C_H  2
#else
#define MARGIN_W    (XOFS*2 + 16)
#define MARGIN_H    (YOFS*2+1)
#define MARGIN_C_W  1
#define MARGIN_C_H  0
#endif

#if ( (LCD_WIDTH  - MARGIN_W) / (BOARD_SIZE+MARGIN_C_W)) < \
    ( (LCD_HEIGHT - MARGIN_H) / (BOARD_SIZE+MARGIN_C_H))

#define CELL_PRE    (   ( (LCD_WIDTH * LCD_PIXEL_ASPECT_WIDTH     / \
                            LCD_PIXEL_ASPECT_HEIGHT) - MARGIN_W)     / \
                        (BOARD_SIZE+MARGIN_C_W) )
                        
#define CELL_WIDTH  (CELL_PRE*LCD_PIXEL_ASPECT_HEIGHT / LCD_PIXEL_ASPECT_WIDTH)
#define CELL_HEIGHT (CELL_PRE) 
#else
#define CELL_PRE    (   ( (LCD_HEIGHT * LCD_PIXEL_ASPECT_HEIGHT     / \
                            LCD_PIXEL_ASPECT_WIDTH) - MARGIN_H)     / \
                        (BOARD_SIZE+MARGIN_C_H) )
                        
#define CELL_WIDTH  (CELL_PRE) 
#define CELL_HEIGHT (CELL_PRE*LCD_PIXEL_ASPECT_WIDTH / LCD_PIXEL_ASPECT_HEIGHT)
#endif

/* Total width and height of the board without enclosing box */
#define BOARD_WIDTH  (CELL_WIDTH*BOARD_SIZE)
#define BOARD_HEIGHT (CELL_HEIGHT*BOARD_SIZE)

/* Thickness of the white cells' lines */
#if (CELL_WIDTH >= 10) && (CELL_HEIGHT >= 10)
#define CELL_LINE_THICKNESS CELL_WIDTH/5
#else
#define CELL_LINE_THICKNESS 1
#endif

/* Margins within a cell */
#define STONE_MARGIN 2

#define CURSOR_MARGIN 1

/* Upper left corner of a cell */
#define CELL_X(c) (XOFS + (c)*CELL_WIDTH)
#define CELL_Y(r) (YOFS + (r)*CELL_HEIGHT)

/* Used for touchscreen to convert an X/Y location to a cell location */
#define CELL_C(x) (((x)-XOFS)/CELL_WIDTH)
#define CELL_R(y) (((y)-YOFS)/CELL_HEIGHT)

#if LCD_HEIGHT > LCD_WIDTH
#define LEGEND_X(lc) (CELL_X(lc))
#define LEGEND_Y(lr) (CELL_Y(BOARD_SIZE+lr) + YOFS + 1)
#else
#define LEGEND_X(lc) (CELL_X(BOARD_SIZE+lc) + XOFS + 1)
#define LEGEND_Y(lr) (CELL_Y(lr))
#endif


/* Board state */
static reversi_board_t game;

/* --- Setting values --- */

/* Playing strategies used by white and black players */
const game_strategy_t *white_strategy;
const game_strategy_t *black_strategy;

/* Cursor position */
static int cur_row, cur_col;

/* Color for the next move (BLACK/WHITE) */
static int cur_player;

/* Active cursor wrapping mode */
static cursor_wrap_mode_t cursor_wrap_mode;

static bool quit_plugin;
static bool game_finished;


/* Initialises the state of the game (starts a new game) */
static void reversi_gui_init(void) {
    reversi_init_game(&game);
    game_finished = false;
    cur_player = BLACK;

    /* Place the cursor so that WHITE can make a move */
    cur_row = 2;
    cur_col = 3;
}


/* Draws the cursor in the specified cell. Cursor is drawn in the complement
 * mode, i.e. drawing it twice will result in no changes on the screen.
 */
static void reversi_gui_display_cursor(int row, int col) {
    int old_mode, x, y;
    old_mode = rb->lcd_get_drawmode();
    x = CELL_X(col);
    y = CELL_Y(row);

    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_drawline(x+CURSOR_MARGIN, y+CURSOR_MARGIN,
            x+CELL_WIDTH-CURSOR_MARGIN, y+CELL_HEIGHT-CURSOR_MARGIN);
    rb->lcd_drawline(x+CURSOR_MARGIN, y+CELL_HEIGHT-CURSOR_MARGIN,
            x+CELL_WIDTH-CURSOR_MARGIN, y+CURSOR_MARGIN);

    /* Draw the shadows */
    rb->lcd_hline(x, x+CELL_WIDTH-1, YOFS-3);
    rb->lcd_hline(x, x+CELL_WIDTH-1, YOFS+BOARD_HEIGHT+3);
    rb->lcd_vline(XOFS-3, y, y+CELL_HEIGHT-1);
    rb->lcd_vline(XOFS+BOARD_WIDTH+3, y, y+CELL_HEIGHT-1);

    rb->lcd_set_drawmode(old_mode);
    rb->lcd_update();
}


/* Draws the cell of the specified color (WHITE/BLACK) assuming that
 * the upper left corner of the cell is at (x, y) */
static void reversi_gui_draw_cell(int x, int y, int color) {
    int i;
    if (color == WHITE) {
        for (i = 0; i < CELL_LINE_THICKNESS; i++) {
            rb->lcd_drawrect(x+STONE_MARGIN+i, y+STONE_MARGIN+i,
                    CELL_WIDTH+1-2*(STONE_MARGIN+i), CELL_HEIGHT+1-2*(STONE_MARGIN+i));
        }
    } else if (color == BLACK) {
        rb->lcd_fillrect(x+STONE_MARGIN, y+STONE_MARGIN,
                CELL_WIDTH-STONE_MARGIN-1, CELL_HEIGHT-1-STONE_MARGIN);
    } else {
        /* Cell is free -> nothing to do */
    }
}


/* Draws the complete screen */
static void reversi_gui_display_board(void) {
    int x, y, r, c, x_width, x_height;
    char buf[8];

    /* Clear the display buffer */
    rb->lcd_clear_display();
    rb->lcd_set_drawmode(DRMODE_FG);

    /* Thicker board box */
    rb->lcd_drawrect(XOFS-1, YOFS-1, BOARD_WIDTH+3, BOARD_HEIGHT+3);

    /* Draw the gridlines */
    for (r=0, x=XOFS, y=YOFS; r<=BOARD_SIZE;
            r++, x+=CELL_WIDTH, y+=CELL_HEIGHT) {
        rb->lcd_hline(XOFS, XOFS+BOARD_WIDTH, y);
        rb->lcd_vline(x, YOFS, YOFS+BOARD_HEIGHT);
    }

    /* Draw the stones. This is not the most efficient way but more readable */
    for (r=0; r<BOARD_SIZE; r++) {
        y = CELL_Y(r);
        for (c=0; c<BOARD_SIZE; c++) {
            x = CELL_X(c);
            reversi_gui_draw_cell(x, y, game.board[r][c]);
        }
    }

    /* Draw the cursor */
    reversi_gui_display_cursor(cur_row, cur_col);

    /* Draw the current score */
    reversi_count_occupied_cells(&game, &r, &c);
    rb->lcd_getstringsize("x", &x_width, &x_height);

    x = LEGEND_X(0);
    y = LEGEND_Y(0);
    reversi_gui_draw_cell(x, y, BLACK);
    rb->snprintf(buf, sizeof(buf), "%d", c);
    y += (CELL_HEIGHT-x_height) / 2;
    rb->lcd_putsxy(x + CELL_WIDTH + 2, y, buf);

    y = LEGEND_Y(1);
    reversi_gui_draw_cell(x, y, WHITE);
    rb->snprintf(buf, sizeof(buf), "%d", r);
    y += (CELL_HEIGHT-x_height) / 2;
    rb->lcd_putsxy(x + CELL_WIDTH + 2, y, buf);

    /* Draw the box around the current player */
    r = (cur_player == BLACK ? 0 : 1);
    y = LEGEND_Y(r);
    rb->lcd_drawrect(x, y, CELL_WIDTH+1, CELL_HEIGHT+1);

    /* Update the screen */
    rb->lcd_update();
}


/*
 * Menu related stuff
 */

/* Menu entries and the corresponding values for cursor wrap mode */
#define MENU_TEXT_WRAP_MODE "Cursor wrap mode"
static const struct opt_items cursor_wrap_mode_settings[] = {
    { "Flat board", -1 },
    { "Sphere",     -1 },
    { "Torus",      -1 },
};
static const cursor_wrap_mode_t cursor_wrap_mode_values[3] = {
    WRAP_FLAT, WRAP_SPHERE, WRAP_TORUS };


/* Menu entries and the corresponding values for available strategies */
#define MENU_TEXT_STRAT_WHITE "Strategy for white"
#define MENU_TEXT_STRAT_BLACK "Strategy for black"

static struct opt_items strategy_settings[] = {
    { "Human", -1 },
    { "Naive robot", -1 },
    { "Simple robot", -1 },
    //{ "AB robot", -1 },
};
static const game_strategy_t * const strategy_values[] = {
    &strategy_human, &strategy_naive, &strategy_simple, /*&strategy_ab*/ };


/* Sets the strategy for the specified player. 'player' is the
   pointer to the player to set the strategy for (actually,
   either white_strategy or black_strategy). propmpt is the
   text to show as the prompt in the menu */
static bool reversi_gui_choose_strategy(
            const game_strategy_t **player, const char *prompt) {
    int index = 0, i;
    int num_items = sizeof(strategy_settings)/sizeof(strategy_settings[0]);
    bool result;

    for (i = 0; i < num_items; i++) {
        if ((*player) == strategy_values[i]) {
            index = i;
            break;
        }
    }
    result = rb->set_option(prompt, &index, INT, strategy_settings, num_items, NULL);
    (*player) = strategy_values[index];

    if((*player)->init_func)
        (*player)->init_func(&game);

    return result;
}


/* Returns true iff USB ws connected while in the menu */
static bool reversi_gui_menu(void) {
    int index, num_items, i;
    int result;

    MENUITEM_STRINGLIST(menu, "Reversi Menu", NULL,
                        "Start new game", "Pass the move",
                        MENU_TEXT_STRAT_BLACK, MENU_TEXT_STRAT_WHITE,
                        MENU_TEXT_WRAP_MODE, "Playback Control", "Quit");

    result = rb->do_menu(&menu, NULL, NULL, false);

    switch (result) {
        case 0: /* Start a new game */
            reversi_gui_init();
            break;

        case 1: /* Pass the move to the partner */
            cur_player = reversi_flipped_color(cur_player);
            break;

        case 2: /* Strategy for black */
            reversi_gui_choose_strategy(&black_strategy, MENU_TEXT_STRAT_BLACK);
            break;

        case 3: /* Strategy for white */
            reversi_gui_choose_strategy(&white_strategy, MENU_TEXT_STRAT_WHITE);
            break;

        case 4: /* Cursor wrap mode */
            num_items = sizeof(cursor_wrap_mode_values)/sizeof(cursor_wrap_mode_values[0]);
            index = 0;
            for (i = 0; i < num_items; i++) {
                if (cursor_wrap_mode == cursor_wrap_mode_values[i]) {
                    index = i;
                    break;
                }
            }
            rb->set_option(MENU_TEXT_WRAP_MODE, &index, INT,
                    cursor_wrap_mode_settings, 3, NULL);
            cursor_wrap_mode = cursor_wrap_mode_values[index];
            break;

        case 5:
            playback_control(NULL);
            break;

        case 6: /* Quit */
            quit_plugin = true;
            break;
    }

    return (result == MENU_ATTACHED_USB);
}


/* Calculates the new cursor position if the user wants to move it
 * vertically as specified by delta. Current wrap mode is respected.
 * The cursor is not actually moved.
 *
 * Returns true iff the cursor would be really moved. In any case, the
 * new cursor position is stored in (new_row, new_col).
 */
static bool reversi_gui_cursor_pos_vmove(int row_delta, int *new_row, int *new_col) {
    *new_row = cur_row + row_delta;
    *new_col = cur_col;

    if (*new_row < 0) {
        switch (cursor_wrap_mode) {
            case WRAP_FLAT:
                *new_row = cur_row;
                break;
            case WRAP_SPHERE:
                *new_row = BOARD_SIZE - 1;
                break;
            case WRAP_TORUS:
                *new_row = BOARD_SIZE - 1;
                (*new_col)--;
                if (*new_col < 0) {
                    *new_col = BOARD_SIZE - 1;
                }
                break;
        }
    } else if (*new_row >= BOARD_SIZE) {
        switch (cursor_wrap_mode) {
            case WRAP_FLAT:
                *new_row = cur_row;
                break;
            case WRAP_SPHERE:
                *new_row = 0;
                break;
            case WRAP_TORUS:
                *new_row = 0;
                (*new_col)++;
                if (*new_col >= BOARD_SIZE) {
                    *new_col = 0;
                }
                break;
        }
    }

    return (cur_row != (*new_row)) || (cur_col != (*new_col));
}


/* Calculates the new cursor position if the user wants to move it
 * horisontally as specified by delta. Current wrap mode is respected.
 * The cursor is not actually moved.
 *
 * Returns true iff the cursor would be really moved. In any case, the
 * new cursor position is stored in (new_row, new_col).
 */
static bool reversi_gui_cursor_pos_hmove(int col_delta, int *new_row, int *new_col) {
    *new_row = cur_row;
    *new_col = cur_col + col_delta;

    if (*new_col < 0) {
        switch (cursor_wrap_mode) {
            case WRAP_FLAT:
                *new_col = cur_col;
                break;
            case WRAP_SPHERE:
                *new_col = BOARD_SIZE - 1;
                break;
            case WRAP_TORUS:
                *new_col = BOARD_SIZE - 1;
                (*new_row)--;
                if (*new_row < 0) {
                    *new_row = BOARD_SIZE - 1;
                }
                break;
        }
    } else if (*new_col >= BOARD_SIZE) {
        switch (cursor_wrap_mode) {
            case WRAP_FLAT:
                *new_col = cur_col;
                break;
            case WRAP_SPHERE:
                *new_col = 0;
                break;
            case WRAP_TORUS:
                *new_col = 0;
                (*new_row)++;
                if (*new_row >= BOARD_SIZE) {
                    *new_row = 0;
                }
                break;
        }
    }

    return (cur_row != (*new_row)) || (cur_col != (*new_col));
}


/* Actually moves the cursor to the new position and updates the screen */
static void reversi_gui_move_cursor(int new_row, int new_col) {
    int old_row, old_col;

    old_row = cur_row;
    old_col = cur_col;

    cur_row = new_row;
    cur_col = new_col;

    /* Only update the changed cells since there are no global changes */
    reversi_gui_display_cursor(old_row, old_col);
    reversi_gui_display_cursor(new_row, new_col);
}


/* plugin entry point */
enum plugin_status plugin_start(const void *parameter) {
    bool exit, draw_screen;
    int button;
#ifdef HAVE_TOUCHSCREEN
    int button_x, button_y;
#endif
    int lastbutton = BUTTON_NONE;
    int row, col;
    int w_cnt, b_cnt;
    char msg_buf[30];
    
#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(TOUCHSCREEN_POINT);
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif

    /* Avoid compiler warnings */
    (void)parameter;

    rb->srand(*rb->current_tick); /* Some AIs use rand() */
    white_strategy = &strategy_human;
    black_strategy = &strategy_human;

    reversi_gui_init();
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    cursor_wrap_mode = WRAP_TORUS;
#else
    cursor_wrap_mode = WRAP_FLAT;
#endif
    /* The main game loop */
    exit = false;
    quit_plugin = false;
    draw_screen = true;
    while (!exit && !quit_plugin) {
        const game_strategy_t *cur_strategy = NULL;
        if (draw_screen) {
            reversi_gui_display_board();
            draw_screen = false;
        }
        switch(cur_player) {
            case BLACK:
                cur_strategy = black_strategy;
                break;
            case WHITE:
                cur_strategy = white_strategy;
                break;
        }

        if(cur_strategy->is_robot && !game_finished) {
            move_t m = cur_strategy->move_func(&game, cur_player);
            reversi_make_move(&game, MOVE_ROW(m), MOVE_COL(m), cur_player);
            cur_player = reversi_flipped_color(cur_player);
            draw_screen = true;
            /* TODO: Add some delay to prevent it from being too fast ? */
            /* TODO: Don't duplicate end of game check */
            if (reversi_game_is_finished(&game, cur_player)) {
                reversi_count_occupied_cells(&game, &w_cnt, &b_cnt);
                rb->snprintf(msg_buf, sizeof(msg_buf),
                        "Game over. %s won.",
                        (w_cnt>b_cnt?"WHITE":"BLACK"));
                rb->splash(HZ*2, msg_buf);
                draw_screen = true; /* Must update screen after splash */
                game_finished = true;
            }
            continue;
        }

        button = rb->button_get(true);

        switch (button) {
#ifdef REVERSI_BUTTON_QUIT
            /* Exit game */
            case REVERSI_BUTTON_QUIT:
                exit = true;
                break;
#endif

#ifdef HAVE_TOUCHSCREEN
            case BUTTON_TOUCHSCREEN:
                button_x = rb->button_get_data() >> 16;
                button_y = rb->button_get_data() & 0xffff;
                if( (CELL_R(button_y)>(BOARD_SIZE-1)) || 
                    (CELL_C(button_x)>(BOARD_SIZE-1)) )
                {
                    break;
                } else {
                    reversi_gui_move_cursor(CELL_R(button_y), CELL_C(button_x));
                }
#endif

#ifdef REVERSI_BUTTON_ALT_MAKE_MOVE
            case REVERSI_BUTTON_ALT_MAKE_MOVE:
#endif
            case REVERSI_BUTTON_MAKE_MOVE:
#ifdef REVERSI_BUTTON_MAKE_MOVE_PRE
                if ((button == REVERSI_BUTTON_MAKE_MOVE)
                        && (lastbutton != REVERSI_BUTTON_MAKE_MOVE_PRE))
                    break;
#endif
                if (game_finished) break;
                if (reversi_make_move(&game, cur_row, cur_col, cur_player) > 0) {
                    /* Move was made. Global changes on the board are possible */
                    draw_screen = true; /* Redraw the screen next time */
                    cur_player = reversi_flipped_color(cur_player);
                    if (reversi_game_is_finished(&game, cur_player)) {
                        reversi_count_occupied_cells(&game, &w_cnt, &b_cnt);
                        rb->snprintf(msg_buf, sizeof(msg_buf),
                                "Game over. %s won.",
                                (w_cnt>b_cnt?"WHITE":"BLACK"));
                        rb->splash(HZ*2, msg_buf);
                        draw_screen = true; /* Must update screen after splash */
                        game_finished = true;
                    }
                } else {
                    /* An attempt to make an invalid move */
                    rb->splash(HZ/2, "Illegal move!");
                    draw_screen = true;
                    /* Ignore any button presses during the splash */
                    rb->button_clear_queue();
                }
                break;
            /* Move cursor left */
#ifdef REVERSI_BUTTON_ALT_LEFT
            case REVERSI_BUTTON_ALT_LEFT:
            case (REVERSI_BUTTON_ALT_LEFT | BUTTON_REPEAT):
#endif
            case REVERSI_BUTTON_LEFT:
            case (REVERSI_BUTTON_LEFT | BUTTON_REPEAT):
                if (reversi_gui_cursor_pos_hmove(-1, &row, &col)) {
                    reversi_gui_move_cursor(row, col);
                }
                break;

            /* Move cursor right */
#ifdef REVERSI_BUTTON_ALT_RIGHT
            case REVERSI_BUTTON_ALT_RIGHT:
            case (REVERSI_BUTTON_ALT_RIGHT | BUTTON_REPEAT):
#endif
            case REVERSI_BUTTON_RIGHT:
            case (REVERSI_BUTTON_RIGHT | BUTTON_REPEAT):
                if (reversi_gui_cursor_pos_hmove(1, &row, &col)) {
                    reversi_gui_move_cursor(row, col);
                }
                break;

            /* Move cursor up */
            case REVERSI_BUTTON_UP:
            case (REVERSI_BUTTON_UP | BUTTON_REPEAT):
                if (reversi_gui_cursor_pos_vmove(-1, &row, &col)) {
                    reversi_gui_move_cursor(row, col);
                }
                break;

            /* Move cursor down */
            case REVERSI_BUTTON_DOWN:
            case (REVERSI_BUTTON_DOWN | BUTTON_REPEAT):
                if (reversi_gui_cursor_pos_vmove(1, &row, &col)) {
                    reversi_gui_move_cursor(row, col);
                }
                break;

            case REVERSI_BUTTON_MENU:
#ifdef REVERSI_BUTTON_MENU_PRE
                if (lastbutton != REVERSI_BUTTON_MENU_PRE) {
                    break;
                }
#endif
                if (reversi_gui_menu()) {
                    return PLUGIN_USB_CONNECTED;
                }
                draw_screen = true;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                    /* Quit if USB has been connected */
                    return PLUGIN_USB_CONNECTED;
                }
                break;
        }
        if (button != BUTTON_NONE) {
            lastbutton = button;
        }
    }

    return PLUGIN_OK;
}

#endif
