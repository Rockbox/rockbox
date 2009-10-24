/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id:
*
* Copyright (C) 2009 Clément Pit--Claudel
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
#include "lib/configfile.h"
#include "lib/playback_control.h"
#include "lib/pluginlib_actions.h"

PLUGIN_HEADER

/* Limits */
#define MAX_PIECES_COUNT 5
#define MAX_COLORS_COUNT 8
#define MAX_GUESSES_COUNT 10

const struct button_mapping *plugin_contexts[] =
            {generic_directions, generic_actions};

/* Mapping */
#define EXIT                PLA_QUIT
#define VALIDATE            PLA_FIRE
#define PREV_PIECE          PLA_LEFT
#define PREV_PIECE_REPEAT   PLA_LEFT_REPEAT
#define NEXT_PIECE          PLA_RIGHT
#define NEXT_PIECE_REPEAT   PLA_RIGHT_REPEAT
#define PREV_COLOR          PLA_UP
#define PREV_COLOR_REPEAT   PLA_UP_REPEAT
#define NEXT_COLOR          PLA_DOWN
#define NEXT_COLOR_REPEAT   PLA_DOWN_REPEAT

/*
 * Screen structure:
 * * (guesses_count) lines of guesses,
 * * 1 center line of solution (hidden),
 * * 1 line showing available colors.
 *
 * Status vars:
 * * quit: exit the plugin
 * * leave: restart the plugin (leave the current game)
 * * game_ended: the game has ended
 * * found: the combination has been found
 *
 * Colors used are taken from the Tango project.
 *
 * Due to integer truncations, 2 vars are used for some objects' dimensions
 * (eg. true_guess_w, true_score_w). The actual dimension of these objects is
 * stored in the corresponding var. without the "true" prefix.
 */

struct mm_score {
    int correct;
    int misplaced;
};

struct mm_line {
    struct mm_score score;
    int pieces[MAX_PIECES_COUNT];
};

const int colors[MAX_COLORS_COUNT] = {
                                        LCD_RGBPACK(252, 233,  79),
                                        LCD_RGBPACK(206,  92,   0),
                                        LCD_RGBPACK(143,  89,   2),
                                        LCD_RGBPACK( 78, 154,   6),
                                        /* LCD_RGBPACK( 32,  74, 135), */
                                        LCD_RGBPACK( 52, 101, 164),
                                        /* LCD_RGBPACK(114, 159, 207), */
                                        LCD_RGBPACK(117,  80, 123),
                                        /* LCD_RGBPACK(173, 127, 168), */
                                        LCD_RGBPACK(164,   0,   0),
                                        LCD_RGBPACK(238, 238, 236),
                                     };

/* Flags */
static bool quit, leave, usb;
static bool found, game_ended;

/* Settings */
static int pieces_count;
static int colors_count;
static int guesses_count;
static int pieces_tmp = 5;
static int colors_tmp = 7;
static int guesses_tmp = 10;
static bool labeling = false, framing = false;

/* Display */
#define ALUMINIUM   LCD_RGBPACK(136, 138, 133)

#define MARGIN  5
#define X_MARGIN    (LCD_WIDTH / 20)
#define Y_MARGIN    (LCD_HEIGHT / 20)
#define GAME_H      (LCD_HEIGHT - (2 * Y_MARGIN))
#define LINE_W      (LCD_WIDTH  - (2 * X_MARGIN))

#define CONFIG_FILE_NAME "codebuster.cfg"

static struct configdata config[] = {
    {TYPE_INT, 0, MAX_PIECES_COUNT, { .int_p = &pieces_tmp }, "pieces", NULL},
    {TYPE_INT, 0, MAX_COLORS_COUNT, { .int_p = &colors_tmp }, "colors", NULL},
    {TYPE_INT, 0, MAX_GUESSES_COUNT, { .int_p = &guesses_tmp }, "guesses", NULL},
    {TYPE_BOOL, 0, 1, { .bool_p = &labeling }, "labeling", NULL},
    {TYPE_BOOL, 0, 1, { .bool_p = &framing }, "framing", NULL},
};
static bool settings_changed = false;

static int line_h;
static int piece_w, tick_w;
static int true_guess_w, true_score_w, guess_w, score_w;

/* Guesses and solution */
struct mm_line solution, hidden;
struct mm_line guesses[MAX_GUESSES_COUNT];

/* Alias for pluginlib_getaction */
static inline int get_button(void) {
    return pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts, 2);
}

/* Computes the margin to center an element */
static inline int get_margin(int width, int full_w) {
    return ((full_w - width) / 2);
}

static inline bool stop_game(void) {
    return (quit || leave || found);
}

static void fill_color_rect(int x, int y, int w, int h, int color) {
    rb->lcd_set_foreground(color);
    rb->lcd_fillrect(x, y, w, h);
    rb->lcd_set_foreground(LCD_WHITE);
}

static void overfill_rect(int x, int y, int w, int h) {
    rb->lcd_fillrect(x - 2, y - 2, w + 4, h + 4);
}

static void draw_piece(int x, int y, int w, int h, int color_id, bool emph) {
    int color = LCD_BLACK;

    if (color_id >= 0)
        color = colors[color_id];
    else if (color_id == -2) /* Hidden piece */
        color = ALUMINIUM;

    if (emph)
        overfill_rect(x, y, w, h);

    if (color_id == -1) /* Uninitialised color */
        rb->lcd_drawrect(x, y, w, h);
    else
        fill_color_rect(x, y, w, h, color);

    if (!emph && framing)
        rb->lcd_drawrect(x, y, w, h);

    if (labeling && color_id >= 0) {
        char text[2];
        rb->snprintf(text, 2, "%d", color_id);

        int fw, fh; rb->font_getstringsize(text, &fw, &fh, FONT_SYSFIXED);
        rb->lcd_putsxy(x + get_margin(fw, w), y + get_margin(fh, h), text);
    }   
}

/* Compute the score for a given guess (expressed in ticks) */
static void validate_guess(struct mm_line* guess) {
    bool solution_match[pieces_count];
    bool guess_match[pieces_count];

    guess->score.misplaced = 0;
    guess->score.correct = 0;

    int guess_pos;

    /* Initialisation with 0s */
    for (guess_pos = 0; guess_pos < pieces_count; guess_pos++)
        solution_match[guess_pos] = guess_match[guess_pos] = false;

    /* 1st step : detect correctly positioned pieces */
    for (guess_pos = 0; guess_pos < pieces_count; guess_pos++) {
        if (solution.pieces[guess_pos] == guess->pieces[guess_pos]) {
            guess->score.correct += 1;

            guess_match[guess_pos] = solution_match[guess_pos]
                                   = true;
        }
    }

    /* Second step : detect mispositioned pieces */
    for (guess_pos = 0; guess_pos < pieces_count; guess_pos++) {
        if (guess_match[guess_pos]) continue;

        int sol_pos;
        for (sol_pos = 0; sol_pos < pieces_count; sol_pos++) {
            if (guess_match[guess_pos]) break;
            if (solution_match[sol_pos]) continue;

            if (guess->pieces[guess_pos] == solution.pieces[sol_pos]) {
                guess->score.misplaced += 1;

                solution_match[sol_pos] = true;
                break;
            }
        }
    }
}

static void draw_guess(int line, struct mm_line* guess, int cur_guess,
                        int cur_piece, bool show_score) {
    int cur_y = (Y_MARGIN + MARGIN) + 2 * line_h * line;
    int l_margin = X_MARGIN + (show_score ? 0 : get_margin(guess_w, LINE_W));

    int piece;
    for (piece = 0; piece < pieces_count; piece++) {
        int cur_x = l_margin + 2 * piece_w * piece;
        draw_piece(cur_x, cur_y, piece_w, line_h, guess->pieces[piece],
                   line == cur_guess && piece == cur_piece);
    }
}

static void draw_score(int line, struct mm_line* guess) {
    int cur_y = Y_MARGIN + 2 * line_h * line;
    int l_margin = X_MARGIN + true_guess_w + MARGIN;

    int tick = 0;
    for (; tick < guess->score.correct; tick++) {
        int cur_x = l_margin + 2 * tick_w * tick;

        fill_color_rect(cur_x, cur_y, tick_w, line_h, LCD_RGBPACK(239, 41, 41));
    }

    for (; tick < guess->score.correct + guess->score.misplaced; tick++) {
        int cur_x = l_margin + 2 * tick_w * tick;

        fill_color_rect(cur_x, cur_y, tick_w, line_h, LCD_RGBPACK(211, 215, 207));
    }
}

static void draw_board(int cur_guess, int cur_piece) {
    rb->lcd_clear_display();

    int line = 0;
    for (; line < guesses_count; line++) {
        draw_guess(line, &guesses[line], cur_guess, cur_piece, true);
        if (line < cur_guess) draw_score(line, &guesses[line]);
    }

    int color;
    int colors_margin = 2;
    int cur_y = (Y_MARGIN + MARGIN) + 2 * line_h * line;
    int color_w = (LINE_W - colors_margin * (colors_count - 1)) / colors_count;

    for (color = 0; color < colors_count; color++) {
        int cur_x = X_MARGIN + color * (color_w + colors_margin);
        draw_piece(cur_x, cur_y, color_w, line_h, color,
                   color == guesses[cur_guess].pieces[cur_piece]);
    }

    line++;

    if(game_ended)
        draw_guess(line, &solution, cur_guess, cur_piece, false);
    else
        draw_guess(line, &hidden, cur_guess, cur_piece, false);

    rb->lcd_update();
}

static void init_vars(void) {
    quit = leave = usb = found = game_ended = false;

    int guess, piece;
    for (guess = 0; guess < guesses_count; guess++) {
        for (piece = 0; piece < pieces_count; piece++)
            guesses[guess].pieces[piece] = -1;
    }
    for (piece = 0; piece < pieces_count; piece++) {
        guesses[0].pieces[piece] = 0;
        hidden.pieces[piece] = -2;
    }
}

static void init_board(void) {
    
    pieces_count = pieces_tmp;
    colors_count = colors_tmp;
    guesses_count = guesses_tmp;

    line_h = GAME_H / (2 * (guesses_count + 2) - 1);

    true_score_w = LINE_W * 0.25;
    true_guess_w = LINE_W - (true_score_w + MARGIN);

    tick_w  = true_score_w / (2 * pieces_count - 1);
    piece_w = true_guess_w / (2 * pieces_count - 1);

    /* Readjust (due to integer divisions) */
    score_w = tick_w  * (2 * pieces_count - 1);
    guess_w = piece_w * (2 * pieces_count - 1);
}

static void randomize_solution(void) {
    int piece_id;
    for (piece_id = 0; piece_id < pieces_count; piece_id++)
        solution.pieces[piece_id] = rb->rand() % colors_count;
}

static void settings_menu(void) {
    MENUITEM_STRINGLIST(settings_menu, "Settings", NULL,
                        "Number of colors", "Number of pieces",
                        "Number of guesses", "Labels ?", "Frames ?");

    int cur_item =0;

    bool menu_quit = false;
    while(!menu_quit) {

        switch(rb->do_menu(&settings_menu, &cur_item, NULL, false)) {
            case 0:
                rb->set_int("Number of colors", "", UNIT_INT, &colors_tmp,
                            NULL, -1, MAX_COLORS_COUNT, 1, NULL);
                break;
            case 1:
                rb->set_int("Number of pieces", "", UNIT_INT, &pieces_tmp,
                            NULL, -1, MAX_PIECES_COUNT, 1, NULL);
                break;
            case 2:
                rb->set_int("Number of guesses", "", UNIT_INT, &guesses_tmp,
                            NULL, -1, MAX_GUESSES_COUNT, 1, NULL);
                break;
            case 3:
                rb->set_bool("Display labels ?", &labeling);
                break;
            case 4:
                rb->set_bool("Display frames ?", &framing);
                break;
            case GO_TO_PREVIOUS:
                menu_quit = true;
                break;
            default:
                break;
        }
    }
}

static bool resume;
static int menu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = ((intptr_t)this_item);
    if ((action == ACTION_REQUEST_MENUITEM) && (!resume && (i==0)))
        return ACTION_EXIT_MENUITEM;
    return action;
}

static void main_menu(void) {
    MENUITEM_STRINGLIST(main_menu, "Codebuster Menu", menu_cb,
                        "Resume Game", "Start New Game", "Settings",
                        "Playback Control", "Quit");

    int cur_item =0;

    bool menu_quit = false;
    while(!menu_quit) {

        switch(rb->do_menu(&main_menu, &cur_item, NULL, false)) {
            case 0:
                resume = true;
                menu_quit = true;
                break;
            case 1:
                leave = true;
                menu_quit = true;
                break;
            case 2:
                settings_menu();
                settings_changed = true;
                break;
            case 3:
                playback_control(NULL);
                break;
            case 4:
                quit = menu_quit = true;
                break;
            case MENU_ATTACHED_USB:
                usb = menu_quit = true;
                break;
            default:
                break;
        }
    }
}

enum plugin_status plugin_start(const void* parameter) {
    (void)parameter;

    rb->srand(*rb->current_tick);
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    
    configfile_load(CONFIG_FILE_NAME,config,5,0);

    main_menu();
    while (!quit) {
        init_board();
        randomize_solution();
        init_vars();
        
        draw_board(0, 0);
        int button = 0, guess = 0, piece = 0;
        for (guess = 0; guess < guesses_count && !stop_game(); guess++) {
            while(!stop_game()) {
                draw_board(guess, piece);

                if ((button = get_button()) == VALIDATE) break;

                switch (button) {

                    case EXIT:
                        resume = true;
                        main_menu();
                        break;

                    case NEXT_PIECE:
                    case NEXT_PIECE_REPEAT:
                        piece = (piece + 1) % pieces_count;
                        break;

                    case PREV_PIECE:
                    case PREV_PIECE_REPEAT:
                        piece = (piece + pieces_count - 1) % pieces_count;
                        break;


                    case NEXT_COLOR:
                    case NEXT_COLOR_REPEAT:
                         guesses[guess].pieces[piece] =
                            (guesses[guess].pieces[piece] + 1)
                            % colors_count;
                        break;

                    case PREV_COLOR:
                    case PREV_COLOR_REPEAT:
                        guesses[guess].pieces[piece] =
                            (guesses[guess].pieces[piece] + colors_count - 1)
                            % colors_count;
                        break;

                    default:
                        if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                            quit = usb = true;
                }

                if (guesses[guess].pieces[piece] == -1)
                    guesses[guess].pieces[piece] = 0;
            }

            if (!quit) {
                validate_guess(&guesses[guess]);

                if (guesses[guess].score.correct == pieces_count)
                    found = true;

                if (guess + 1 < guesses_count && !found)
                    guesses[guess + 1] = guesses[guess];
            }
        }

        game_ended = true;
        resume = false;
        if (!quit && !leave) {
            draw_board(guess, piece);

            if (found)
                rb->splash(HZ, "Well done :)");
            else
                rb->splash(HZ, "Wooops :(");
        do {
            button = rb->button_get(true);
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                quit = usb = true;
            }
        } while( ( button == BUTTON_NONE )
              || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
            main_menu();
        }
    }
    if (settings_changed)
        configfile_save(CONFIG_FILE_NAME,config,5,0);
    
    rb->lcd_setfont(FONT_UI);
    return (usb) ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
