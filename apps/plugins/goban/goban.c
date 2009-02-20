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
#include "plugin.h"

#if PLUGIN_BUFFER_SIZE < 0x10000 && !defined(SIMULATOR)

#include "lib/overlay.h"

PLUGIN_HEADER

enum plugin_status plugin_start(const void* parameter)
{
    return run_overlay(parameter, PLUGIN_GAMES_DIR "/goban.ovl", "Goban");
}
#endif

#include "lib/playback_control.h"
#include "lib/configfile.h"

PLUGIN_HEADER

#include "goban.h"
#include "game.h"
#include "board.h"
#include "display.h"
#include "sgf.h"
#include "sgf_storage.h"
#include "types.h"
#include "util.h"

enum play_mode_t play_mode = MODE_PLAY;

#if defined(GBN_BUTTON_NAV_MODE)

#define NAV_MODE_BOARD 0
#define NAV_MODE_TREE 1

int nav_mode = NAV_MODE_BOARD;

#endif

/* the stack that uses this buffer is used for both storing intersections
 * in board algorithms (could store one short for each board location), and
 * for parsing (could store an indefinite number of ints, related to how
 * many levels deep branches go (in other words, how many branches off of
 * branches there are).  50 should take care of any reasonable file.
 */
#define PARSE_STACK_BUFFER_SIZE (max(MAX_BOARD_SIZE * MAX_BOARD_SIZE * sizeof(unsigned short), 50 * sizeof(int)))

/* used in SGF file parsing and outputting as well as in liberty counting
   and capturing/uncapturing */
struct stack_t parse_stack;
char parse_stack_buffer[PARSE_STACK_BUFFER_SIZE];

static void global_setup (void);
static void global_cleanup (void);

static bool do_main_menu (void);
static void do_gameinfo_menu (void);
static enum prop_type_t menu_selection_to_prop (int selection);
static void do_context_menu (void);
static void do_options_menu (void);

static bool do_comment_edit (void);
static bool do_zoom (void);
static void set_defaults (void);

bool auto_show_comments = true;
bool disable_shutdown = false;
unsigned int autosave_time = 0;
unsigned int autosave_counter = 0;

#define SETTINGS_VERSION 2
#define SETTINGS_MIN_VERSION 1
#define SETTINGS_FILENAME "goban.cfg"

static struct configdata config[] =
{            /* INT in MAX_INT_SIZE is intersection, not integer */
    {TYPE_INT, 0, MAX_INT_SIZE,
        { .int_p = &saved_circle_size },
        "stone size", NULL},
    {TYPE_BOOL, 0, 1,
        { .bool_p = &draw_variations },
        "draw variations",
        NULL},
    {TYPE_BOOL, 0, 1,
        { .bool_p = &auto_show_comments },
        "auto show comments",
        NULL},
    {TYPE_BOOL, 0, 1,
        { .bool_p = &disable_shutdown },
        "disable shutdown",
        NULL},
    {TYPE_INT, 0, MAX_AUTOSAVE,
        { .int_p = &autosave_time },
        "autosave time",
        NULL}
};

static void
set_defaults (void)
{
    saved_circle_size = 0;
    draw_variations = true;
    auto_show_comments = true;

    disable_shutdown = false;
    autosave_time = 7;
}

static void
komi_formatter (char *dest, size_t size, int menu_item, const char *unknown)
{
    (void) unknown;
    snprint_fixed (dest, size, menu_item);
}

static void
ruleset_formatter (char *dest, size_t size, int menu_item, const char *unknown)
{
    (void) unknown;
    rb->snprintf (dest, size, "%s", ruleset_names[menu_item]);
}

static void
autosave_formatter (char *dest, size_t size, int menu_item, const char *
unknown)
{
    (void) unknown;
    if (menu_item == 0)
    {
        rb->snprintf (dest, size, "Off");
    }
    else
    {
        rb->snprintf (dest, size, "%d minute%s", menu_item,
                      menu_item == 1 ? "" : "s");
    }
}

static void
time_formatter (char *dest, size_t size, int menu_item, const char *unknown)
{
    int time_values[4];         /* days hours minutes seconds */
    int min_set, max_set;
    int temp;

    (void) unknown;

    time_values[0] = menu_item / (24 * 60 * 60);
    menu_item %= (24 * 60 * 60);
    time_values[1] = menu_item / (60 * 60);
    menu_item %= (60 * 60);
    time_values[2] = menu_item / 60;
    time_values[3] = menu_item % 60;

    min_set = 500;
    max_set = -1;
    int i;
    for (i = 0;
         (unsigned int) i < (sizeof (time_values) / sizeof (time_values[0]));
         ++i)
    {
        if (time_values[i])
        {
            if (i < min_set)
            {
                min_set = i;
            }

            if (i > max_set)
            {
                max_set = i;
            }
        }
    }

    if (max_set == -1)
    {
        rb->snprintf (dest, size, "0");
        return;
    }

    for (i = min_set; i <= 3; ++i)
    {
        if (i <= max_set)
        {
            if (i == 0 || i == 1 || i == min_set)
            {
                rb->snprintf (dest, size, "%d", time_values[i]);
            }
            else
            {
                rb->snprintf (dest, size, "%02d", time_values[i]);
            }
            temp = rb->strlen (dest);
            dest += temp;
            size -= temp;
        }
        else if (i != 3)
        {
            continue;
        }

        if (i == 0)             /* days */
        {
            rb->snprintf (dest, size, " d ");
        }
        else if (i == 3)        /* seconds, print the final units */
        {
            if (min_set == 0 || min_set == 1)
            {
                rb->snprintf (dest, size, " h");
            }
            else if (min_set == 2)
            {
                rb->snprintf (dest, size, " m");
            }
            else
            {
                rb->snprintf (dest, size, " s");
            }
        }
        else if (i != max_set)
        {
            rb->snprintf (dest, size, ":");
        }

        temp = rb->strlen (dest);
        dest += temp;
        size -= temp;
    }
}

enum plugin_status
plugin_start (const void *parameter)
{
    int btn;

    rb->mkdir ("/sgf");

    global_setup ();

#ifdef GBN_TEST
    run_tests ();
    return PLUGIN_OK;
#endif

    if (!(parameter && load_game (parameter)))
    {
        if (parameter)
        {
            rb->splashf (2 * HZ, "Loading %s failed.", (char *) parameter);
        }

        if (!load_game (DEFAULT_SAVE))
        {
            rb->strcpy (save_file, DEFAULT_SAVE);

            if (!setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 0, 0))
            {
                return PLUGIN_ERROR;
            }
        }
    }
    else
    {
        /* game loaded */
        if (rb->strcmp (save_file, DEFAULT_SAVE))
        {
            /* delete the scratch file if we loaded a game and it wasn't
             * from the scratch file
             */
            rb->remove (DEFAULT_SAVE);
        }
    }

    draw_screen_display ();

    autosave_counter = 0;
    for (;;)
    {
        btn = rb->button_get_w_tmo (HZ * 30);

        if (disable_shutdown)
        {
            /* tell rockbox we're not idle */
            rb->reset_poweroff_timer ();
        }

        bool is_idle = false;

        switch (btn)
        {

#if defined(GBN_BUTTON_NAV_MODE)
        case GBN_BUTTON_NAV_MODE:
        case GBN_BUTTON_NAV_MODE | BUTTON_REPEAT:
            if (nav_mode == NAV_MODE_TREE)
            {
                nav_mode = NAV_MODE_BOARD;
                rb->splash (2 * HZ / 3, "board navigation mode");
                draw_screen_display ();
            }
            else
            {
                nav_mode = NAV_MODE_TREE;
                rb->splash (2 * HZ / 3, "tree navigation mode");
                draw_screen_display ();
            }
            break;
#endif

#if defined(GBN_BUTTON_ADVANCE)
        case GBN_BUTTON_ADVANCE:
        case GBN_BUTTON_ADVANCE | BUTTON_REPEAT:
            if (has_more_nodes_sgf ())
            {
                if (!redo_node_sgf ())
                {
                    rb->splash (2 * HZ, "redo failed");
                }
                draw_screen_display ();
            }
            break;
#endif

#if defined(GBN_BUTTON_RETREAT)
        case GBN_BUTTON_RETREAT:
        case GBN_BUTTON_RETREAT | BUTTON_REPEAT:
            if (has_prev_nodes_sgf ())
            {
                if (!undo_node_sgf ())
                {
                    rb->splash (3 * HZ / 2, "Undo Failed");
                }
                draw_screen_display ();
            }
            break;
#endif

        case GBN_BUTTON_PLAY:
            if (play_mode == MODE_PLAY || play_mode == MODE_FORCE_PLAY)
            {
                if (!play_move_sgf (cursor_pos, current_player))
                {
                    rb->splash (HZ / 3, "Illegal Move");
                }
            }
            else if (play_mode == MODE_ADD_BLACK)
            {
                if (!add_stone_sgf (cursor_pos, BLACK))
                {
                    rb->splash (HZ / 3, "Illegal");
                }
            }
            else if (play_mode == MODE_ADD_WHITE)
            {
                if (!add_stone_sgf (cursor_pos, WHITE))
                {
                    rb->splash (HZ / 3, "Illegal");
                }
            }
            else if (play_mode == MODE_REMOVE)
            {
                if (!add_stone_sgf (cursor_pos, EMPTY))
                {
                    rb->splash (HZ / 3, "Illegal");
                }
            }
            else if (play_mode == MODE_MARK)
            {
                if (!add_mark_sgf (cursor_pos, PROP_MARK))
                {
                    rb->splash (HZ / 3, "Couldn't Mark");
                }
            }
            else if (play_mode == MODE_CIRCLE)
            {
                if (!add_mark_sgf (cursor_pos, PROP_CIRCLE))
                {
                    rb->splash (HZ / 3, "Couldn't Mark");
                }
            }
            else if (play_mode == MODE_SQUARE)
            {
                if (!add_mark_sgf (cursor_pos, PROP_SQUARE))
                {
                    rb->splash (HZ / 3, "Couldn't Mark");
                }
            }
            else if (play_mode == MODE_TRIANGLE)
            {
                if (!add_mark_sgf (cursor_pos, PROP_TRIANGLE))
                {
                    rb->splash (HZ / 3, "Couldn't Mark");
                }
            }
            else if (play_mode == MODE_LABEL)
            {
                if (!add_mark_sgf (cursor_pos, PROP_LABEL))
                {
                    rb->splash (HZ / 3, "Couldn't Label");
                }
            }
            else
            {
                rb->splash (HZ, "mode not implemented");
            }

            draw_screen_display ();
            break;

        case GBN_BUTTON_RIGHT:
        case GBN_BUTTON_RIGHT | BUTTON_REPEAT:
#if defined(GBN_BUTTON_NAV_MODE)
            if (nav_mode == NAV_MODE_TREE)
            {
                if (has_more_nodes_sgf ())
                {
                    if (!redo_node_sgf ())
                    {
                        rb->splash (2 * HZ, "Redo Failed");
                    }
                    draw_screen_display ();
                }
            }
            else
            {
#endif
                cursor_pos = WRAP (EAST (cursor_pos));
                draw_screen_display ();
#if defined(GBN_BUTTON_NAV_MODE)
            }
#endif
            break;

        case GBN_BUTTON_LEFT:
        case GBN_BUTTON_LEFT | BUTTON_REPEAT:
#if defined(GBN_BUTTON_NAV_MODE)
            if (nav_mode == NAV_MODE_TREE)
            {
                if (has_prev_nodes_sgf ())
                {
                    if (!undo_node_sgf ())
                    {
                        rb->splash (2 * HZ, "Undo Failed");
                    }
                    draw_screen_display ();
                }
            }
            else
            {
#endif
                cursor_pos = WRAP (WEST (cursor_pos));
                draw_screen_display ();
#if defined(GBN_BUTTON_NAV_MODE)
            }
#endif
            break;

        case GBN_BUTTON_DOWN:
        case GBN_BUTTON_DOWN | BUTTON_REPEAT:
            cursor_pos = WRAP (SOUTH (cursor_pos));
            draw_screen_display ();
            break;

        case GBN_BUTTON_UP:
        case GBN_BUTTON_UP | BUTTON_REPEAT:
            cursor_pos = WRAP (NORTH (cursor_pos));
            draw_screen_display ();
            break;

        case GBN_BUTTON_MENU:
            if (do_main_menu ())
            {
                save_game (DEFAULT_SAVE);

                global_cleanup ();
                return PLUGIN_OK;
            }

            draw_screen_display ();
            break;

#if defined(GBN_BUTTON_CONTEXT)
        case GBN_BUTTON_CONTEXT:
            do_context_menu ();
            draw_screen_display ();
            break;
#endif

#if defined(GBN_BUTTON_NEXT_VAR)
        case GBN_BUTTON_NEXT_VAR:
        case GBN_BUTTON_NEXT_VAR | BUTTON_REPEAT:
        {
            int temp;
            if ((temp = next_variation_sgf ()) >= 0)
            {
                draw_screen_display ();
                rb->splashf (2 * HZ / 3, "%d of %d", temp,
                             num_variations_sgf ());
                draw_screen_display ();
            }
            else
            {
                if (num_variations_sgf () > 1)
                {
                    rb->splashf (HZ, "Error %d in next_variation_sgf", temp);
                }
                draw_screen_display ();
            }
            break;
        }
#endif

        case BUTTON_NONE:
            is_idle = true;
        default:
            if (rb->default_event_handler (btn) == SYS_USB_CONNECTED)
            {
                return PLUGIN_USB_CONNECTED;
            }
            break;
        };

        if (is_idle && autosave_dirty)
        {
            ++autosave_counter;

            if (autosave_time != 0 &&
                autosave_counter / 2 >= autosave_time)
                /* counter is in 30 second increments, autosave_time is in
                 * minutes
                 */
            {
                DEBUGF("autosaving\n");
                rb->splash(HZ / 4, "Autosaving...");
                save_game(DEFAULT_SAVE);
                draw_screen_display();
                autosave_counter = 0;
            }
        }
        else
        {
            autosave_counter = 0;
        }
    }

    return PLUGIN_OK;
}

static void
global_cleanup (void)
{
    cleanup_sgf ();
    configfile_save(SETTINGS_FILENAME, config,
                    sizeof(config)/sizeof(*config),
                    SETTINGS_VERSION);
}

static void
global_setup (void)
{
    setup_stack (&parse_stack, parse_stack_buffer,
                 sizeof (parse_stack_buffer));
    setup_display ();
    setup_sgf ();

    set_defaults();
    if (configfile_load(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_MIN_VERSION ) < 0)
    {
        /* If the loading failed, save a new config file (as the disk is
           already spinning) */

        /* set defaults again just in case (don't know if they can ever
         * be messed up by configfile_load, and it's basically free anyway)
         */
        set_defaults();

        configfile_save(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_VERSION);
    }
}

enum main_menu_selections
{
    MAIN_NEW = 0,
    MAIN_SAVE,
    MAIN_SAVE_AS,
    MAIN_GAME_INFO,
    MAIN_PLAYBACK,
    MAIN_ZOOM,
    MAIN_OPTIONS,
    MAIN_CONTEXT,
    MAIN_QUIT
};

static bool
do_main_menu (void)
{
    int selection = 0;
    MENUITEM_STRINGLIST (menu, "Rockbox Goban", NULL,
                         "New",
                         "Save",
                         "Save As",
                         "Game Info",
                         "Playback Control",
                         "Zoom Level",
                         "Options",
                         "Context Menu",
                         "Quit");

    /* for "New" in menu */
    int new_handi = 0, new_bs = MAX_BOARD_SIZE, new_komi = 15;

    char new_save_file[SAVE_FILE_LENGTH];


    bool done = false;

    while (!done)
    {
        selection = rb->do_menu (&menu, &selection, NULL, false);

        switch (selection)
        {
        case MAIN_NEW:
            rb->set_int ("board size", "lines", UNIT_INT,
                         &new_bs, NULL, 1, MIN_BOARD_SIZE, MAX_BOARD_SIZE,
                         NULL);

            rb->set_int ("handicap", "stones", UNIT_INT,
                         &new_handi, NULL, 1, 0, 9, NULL);

            if (new_handi > 0)
            {
                new_komi = 1;
            }
            else
            {
                new_komi = 13;
            }

            rb->set_int ("komi", "moku", UNIT_INT, &new_komi, NULL,
                         1, -300, 300, &komi_formatter);

            setup_game (new_bs, new_bs, new_handi, new_komi);
            draw_screen_display ();
            done = true;
            break;

        case MAIN_SAVE:
            if (!save_game (save_file))
            {
                rb->splash (2 * HZ, "Save Failed!");
            }
            else
            {
                rb->splash (2 * HZ / 3, "Saved");
            }
            done = true;
            draw_screen_display ();
            break;

        case MAIN_SAVE_AS:
            rb->strcpy (new_save_file, save_file);

            if (!rb->kbd_input (new_save_file, SAVE_FILE_LENGTH))
            {
                break;
            }

            if (!save_game (new_save_file))
            {
                rb->splash (2 * HZ, "Save Failed!");
            }
            else
            {
                rb->strcpy (save_file, new_save_file);
                rb->splash (2 * HZ / 3, "Saved");
            }

            done = true;
            draw_screen_display ();
            break;

        case MAIN_GAME_INFO:
            do_gameinfo_menu ();
            break;

        case MAIN_PLAYBACK:
            if (!audio_stolen_sgf ())
            {
                playback_control (NULL);
            }
            else
            {
                rb->splash (1 * HZ, "Audio has been disabled!");
            }
            break;

        case MAIN_ZOOM:
            if (do_zoom ())
            {
                return true;
            }
            done = true;
            draw_screen_display ();
            break;

        case MAIN_OPTIONS:
            do_options_menu();
            break;

        case MAIN_CONTEXT:
            do_context_menu ();
            done = true;
            break;

        case MAIN_QUIT:
        case MENU_ATTACHED_USB:
            return true;

        case GO_TO_ROOT:
        case GO_TO_PREVIOUS:
        default:

            done = true;
            break;
        };
    }

    return false;
}

void
zoom_preview (int current)
{
    set_zoom_display (current);
    draw_screen_display ();
    rb->splash (0, "Preview");
}

static bool
do_zoom (void)
{
    unsigned int zoom_level;
    unsigned int old_val;
    bool done = false;

    unsigned int min_val = min_zoom_display ();
    unsigned int max_val = max_zoom_display ();

    zoom_level = old_val = current_zoom_display ();

    int action;

    zoom_preview (zoom_level);
    while (!done)
    {
        switch (action = rb->get_action (CONTEXT_LIST, TIMEOUT_BLOCK))
        {
        case ACTION_STD_OK:
            set_zoom_display (zoom_level);
            done = true;
            rb->splash (HZ / 2, "Zoom Set");
            saved_circle_size = intersection_size;
            break;

        case ACTION_STD_CANCEL:
            set_zoom_display (old_val);
            done = true;
            rb->splash (HZ / 2, "Cancelled");
            break;

        case ACTION_STD_CONTEXT:
            zoom_level = old_val;
            zoom_preview (zoom_level);
            break;

        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            zoom_level = zoom_level * 3 / 2;

            /* 1 * 3 / 2 is 1 again... */
            if (zoom_level == 1)
            {
                ++zoom_level;
            }

            if (zoom_level > max_val)
            {
                zoom_level = min_val;
            }

            zoom_preview (zoom_level);
            break;

        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            zoom_level = zoom_level * 2 / 3;

            if (zoom_level < min_val)
            {
                zoom_level = max_val;
            }
            zoom_preview (zoom_level);
            break;

        case ACTION_NONE:
            break;

        default:
            if (rb->default_event_handler (action) == SYS_USB_CONNECTED)
            {
                return true;
            }
            break;
        }
    }

    return false;
}

enum gameinfo_menu_selections
{
    GINFO_BASIC_INFO = 0,
    GINFO_TIME_LIMIT,
    GINFO_OVERTIME,
    GINFO_RESULT,
    GINFO_HANDICAP,
    GINFO_KOMI,
    GINFO_RULESET,
    GINFO_BLACK_PLAYER,
    GINFO_BLACK_RANK,
    GINFO_BLACK_TEAM,
    GINFO_WHITE_PLAYER,
    GINFO_WHITE_RANK,
    GINFO_WHITE_TEAM,
    GINFO_DATE,
    GINFO_EVENT,
    GINFO_PLACE,
    GINFO_ROUND,
    GINFO_DONE
};


static void
do_gameinfo_menu (void)
{
    MENUITEM_STRINGLIST (gameinfo_menu, "Game Info", NULL,
                         "Basic Info",
                         "Time Limit",
                         "Overtime",
                         "Result",
                         "Handicap",
                         "Komi",
                         "Ruleset",
                         "Black Player",
                         "Black Rank",
                         "Black Team",
                         "White Player",
                         "White Rank",
                         "White Team",
                         "Date",
                         "Event",
                         "Place",
                         "Round",
                         "Done");
    /* IMPORTANT:
     *
     * if you edit this string list, make sure you keep
     * menu_selection_to_prop function in line with it!! (see the bottom
     * of this file).
     */

    int new_ruleset = 0;

    char *gameinfo_string;
    int gameinfo_string_size;

    bool done = false;
    int selection = 0;

    while (!done)
    {
        selection = rb->do_menu (&gameinfo_menu, &selection, NULL, false);

        switch (selection)
        {
        case GINFO_OVERTIME:
        case GINFO_RESULT:
        case GINFO_BLACK_PLAYER:
        case GINFO_BLACK_RANK:
        case GINFO_BLACK_TEAM:
        case GINFO_WHITE_PLAYER:
        case GINFO_WHITE_RANK:
        case GINFO_WHITE_TEAM:
        case GINFO_DATE:
        case GINFO_EVENT:
        case GINFO_PLACE:
        case GINFO_ROUND:
            if (!get_header_string_and_size (&header,
                                             menu_selection_to_prop
                                             (selection), &gameinfo_string,
                                             &gameinfo_string_size))
            {
                rb->splash (3 * HZ, "Couldn't get header string");
                break;
            }

            rb->kbd_input (gameinfo_string, gameinfo_string_size);
            sanitize_string (gameinfo_string);
            set_game_modified();
            break;

            /* these need special handling in some way, so they are
               separate */

        case GINFO_BASIC_INFO:
            metadata_summary();
            break;

        case GINFO_TIME_LIMIT:
            rb->set_int ("Time Limit", "", UNIT_INT, &header.time_limit,
                         NULL, 60, 0, 24 * 60 * 60, &time_formatter);
            set_game_modified();
            break;

        case GINFO_HANDICAP:
            rb->splashf (0, "%d stones.  Start a new game to set handicap",
                         header.handicap);
            rb->action_userabort(TIMEOUT_BLOCK);
            break;

        case GINFO_KOMI:
            rb->set_int ("Komi", "moku", UNIT_INT, &header.komi, NULL,
                         1, -300, 300, &komi_formatter);
            set_game_modified();
            break;

        case GINFO_RULESET:
            new_ruleset = 0;
            rb->set_int ("Ruleset", "", UNIT_INT, &new_ruleset, NULL,
                         1, 0, NUM_RULESETS - 1, &ruleset_formatter);

            rb->strcpy (header.ruleset, ruleset_names[new_ruleset]);
            set_game_modified();
            break;

        case GINFO_DONE:
        case GO_TO_ROOT:
        case GO_TO_PREVIOUS:
        case MENU_ATTACHED_USB:
        default:
            done = true;
            break;
        };
    }
}

enum context_menu_selections
{
    CTX_PLAY = 0,
    CTX_ADD_BLACK,
    CTX_ADD_WHITE,
    CTX_ERASE,
    CTX_PASS,
    CTX_NEXT_VAR,
    CTX_FORCE,
    CTX_MARK,
    CTX_CIRCLE,
    CTX_SQUARE,
    CTX_TRIANGLE,
    CTX_LABEL,
    CTX_COMMENT,
    CTX_DONE
};

static void
do_context_menu (void)
{
    int selection;
    bool done = false;
    int temp;

    MENUITEM_STRINGLIST (context_menu, "Context Menu", NULL,
                         "Play Mode (default)",
                         "Add Black Mode",
                         "Add White Mode",
                         "Erase Stone Mode",
                         "Pass",
                         "Next Variation",
                         "Force Play Mode",
                         "Mark Mode",
                         "Circle Mode",
                         "Square Mode",
                         "Triangle Mode",
                         "Label Mode",
                         "Add/Edit Comment",
                         "Done");

    while (!done)
    {
        selection = rb->do_menu (&context_menu, &selection, NULL, false);

        switch (selection)
        {
        case CTX_PLAY:
            play_mode = MODE_PLAY;
            done = true;
            break;

        case CTX_ADD_BLACK:
            play_mode = MODE_ADD_BLACK;
            done = true;
            break;

        case CTX_ADD_WHITE:
            play_mode = MODE_ADD_WHITE;
            done = true;
            break;

        case CTX_ERASE:
            play_mode = MODE_REMOVE;
            done = true;
            break;

        case CTX_PASS:
            if (!play_move_sgf (PASS_POS, current_player))
            {
                rb->splash (HZ, "Error while passing!");
            }
            done = true;
            break;

        case CTX_NEXT_VAR:
            if ((temp = next_variation_sgf ()) >= 0)
            {
                draw_screen_display ();
                rb->splashf (2 * HZ / 3, "%d of %d", temp,
                             num_variations_sgf ());
                draw_screen_display ();
            }
            else
            {
                if (num_variations_sgf () > 1)
                {
                    rb->splashf (HZ, "Error %d in next_variation_sgf", temp);
                }
                else
                {
                    rb->splash (HZ, "No next variation");
                }
            }
            break;

        case CTX_FORCE:
            play_mode = MODE_FORCE_PLAY;
            done = true;
            break;

        case CTX_MARK:
            play_mode = MODE_MARK;
            done = true;
            break;

        case CTX_CIRCLE:
            play_mode = MODE_CIRCLE;
            done = true;
            break;

        case CTX_SQUARE:
            play_mode = MODE_SQUARE;
            done = true;
            break;

        case CTX_TRIANGLE:
            play_mode = MODE_TRIANGLE;
            done = true;
            break;

        case CTX_LABEL:
            play_mode = MODE_LABEL;
            done = true;
            break;

        case CTX_COMMENT:
            if (!do_comment_edit ())
            {
                DEBUGF ("Editing comment failed\n");
                rb->splash (HZ, "Read or write failed!\n");
            }
            done = true;
            break;

        case CTX_DONE:
        case GO_TO_ROOT:
        case GO_TO_PREVIOUS:
        case MENU_ATTACHED_USB:
        default:
            done = true;
            break;
        };
    }
}

enum options_menu_selections
{
    OMENU_SHOW_VARIATIONS = 0,
    OMENU_DISABLE_POWEROFF,
    OMENU_AUTOSAVE_TIME,
    OMENU_AUTO_COMMENT
};

static void
do_options_menu (void)
{
    int selection;
    bool done = false;

    MENUITEM_STRINGLIST (options_menu, "Options Menu", NULL,
                         "Show Child Variations?",
                         "Disable Idle Poweroff?",
                         "Idle Autosave Time",
                         "Automatically Show Comments?");

    while (!done)
    {
        selection = rb->do_menu (&options_menu, &selection, NULL, false);

        switch (selection)
        {
        case OMENU_SHOW_VARIATIONS:
            rb->set_bool("Draw Variations?", &draw_variations);
            clear_marks_display ();
            set_all_marks_sgf ();
            if (draw_variations)
            {
                mark_child_variations_sgf ();
            }
            break;

        case OMENU_DISABLE_POWEROFF:
            rb->set_bool("Disable Idle Poweroff?", &disable_shutdown);
            break;

        case OMENU_AUTOSAVE_TIME:
            rb->set_int("Idle Autosave Time", "minutes", UNIT_INT,
                        &autosave_time, NULL, 1, 0, MAX_AUTOSAVE,
                        &autosave_formatter);
            autosave_counter = 0;
            break;

        case OMENU_AUTO_COMMENT:
            rb->set_bool("Auto Show Comments?", &auto_show_comments);
            break;

        case GO_TO_ROOT:
        case GO_TO_PREVIOUS:
        case MENU_ATTACHED_USB:
        default:
            done = true;
            break;
        };
    }

}

static bool
do_comment_edit (void)
{
    char cbuffer[512];

    rb->memset (cbuffer, 0, sizeof (cbuffer));

    if (read_comment_sgf (cbuffer, sizeof (cbuffer)) < 0)
    {
        return false;
    }

    if (!rb->kbd_input (cbuffer, sizeof (cbuffer)))
    {
        /* user didn't edit, no reason to write it back */
        return true;
    }

    if (write_comment_sgf (cbuffer) < 0)
    {
        return false;
    }

    return true;
}

static enum prop_type_t
menu_selection_to_prop (int selection)
{
    switch (selection)
    {
    case GINFO_OVERTIME:
        return PROP_OVERTIME;
    case GINFO_RESULT:
        return PROP_RESULT;
    case GINFO_BLACK_PLAYER:
        return PROP_BLACK_NAME;
    case GINFO_BLACK_RANK:
        return PROP_BLACK_RANK;
    case GINFO_BLACK_TEAM:
        return PROP_BLACK_TEAM;
    case GINFO_WHITE_PLAYER:
        return PROP_WHITE_NAME;
    case GINFO_WHITE_RANK:
        return PROP_WHITE_RANK;
    case GINFO_WHITE_TEAM:
        return PROP_WHITE_TEAM;
    case GINFO_DATE:
        return PROP_DATE;
    case GINFO_EVENT:
        return PROP_EVENT;
    case GINFO_PLACE:
        return PROP_PLACE;
    case GINFO_ROUND:
        return PROP_ROUND;
    default:
        DEBUGF ("Tried to get prop from invalid menu selection!!!\n");
        return PROP_PLACE;      /* just pick one, there's a major bug if
                                   we got here */
    };
}
