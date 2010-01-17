/* main.c - Frotz V2.40 main function
 *	Copyright (c) 1995-1997 Stefan Jokisch
 *
 * Changes for Rockbox copyright 2009 Torne Wuff
 *
 * This file is part of Frotz.
 *
 * Frotz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Frotz is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/*
 * This is an interpreter for Infocom V1 to V6 games. It also supports
 * the recently defined V7 and V8 games. Please report bugs to
 *
 *    s.jokisch@avu.de
 *
 */

#include "frotz.h"

#ifndef MSDOS_16BIT
#define cdecl
#endif

extern void interpret (void);
extern void init_memory (void);
extern void init_undo (void);
extern void reset_memory (void);
extern void init_buffer (void);
extern void init_sound (void);
extern void init_process (void);
extern void script_close (void);
extern void record_close (void);
extern void replay_close (void);

/* Story file name, id number and size */

const char *story_name = 0;

enum story story_id = UNKNOWN;
long story_size = 0;

/* Story file header data */

zbyte h_version = 0;
zbyte h_config = 0;
zword h_release = 0;
zword h_resident_size = 0;
zword h_start_pc = 0;
zword h_dictionary = 0;
zword h_objects = 0;
zword h_globals = 0;
zword h_dynamic_size = 0;
zword h_flags = 0;
zbyte h_serial[6] = { 0, 0, 0, 0, 0, 0 };
zword h_abbreviations = 0;
zword h_file_size = 0;
zword h_checksum = 0;
zbyte h_interpreter_number = 0;
zbyte h_interpreter_version = 0;
zbyte h_screen_rows = 0;
zbyte h_screen_cols = 0;
zword h_screen_width = 0;
zword h_screen_height = 0;
zbyte h_font_height = 1;
zbyte h_font_width = 1;
zword h_functions_offset = 0;
zword h_strings_offset = 0;
zbyte h_default_background = 0;
zbyte h_default_foreground = 0;
zword h_terminating_keys = 0;
zword h_line_width = 0;
zbyte h_standard_high = 1;
zbyte h_standard_low = 0;
zword h_alphabet = 0;
zword h_extension_table = 0;
zbyte h_user_name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

zword hx_table_size = 0;
zword hx_mouse_x = 0;
zword hx_mouse_y = 0;
zword hx_unicode_table = 0;

/* Stack data */

zword stack[STACK_SIZE];
zword *sp = 0;
zword *fp = 0;
zword frame_count = 0;

/* IO streams */

bool ostream_screen = TRUE;
bool ostream_script = FALSE;
bool ostream_memory = FALSE;
bool ostream_record = FALSE;
bool istream_replay = FALSE;
bool message = FALSE;

/* Current window and mouse data */

int cwin = 0;
int mwin = 0;

int mouse_y = 0;
int mouse_x = 0;

/* Window attributes */

bool enable_wrapping = FALSE;
bool enable_scripting = FALSE;
bool enable_scrolling = FALSE;
bool enable_buffering = FALSE;

/* User options */

/*
int option_attribute_assignment = 0;
int option_attribute_testing = 0;
int option_context_lines = 0;
int option_object_locating = 0;
int option_object_movement = 0;
int option_left_margin = 0;
int option_right_margin = 0;
int option_ignore_errors = 0;
int option_piracy = 0;
int option_undo_slots = MAX_UNDO_SLOTS;
int option_expand_abbreviations = 0;
int option_script_cols = 80;
int option_save_quetzal = 1;
*/

int option_sound = 1;
char *option_zcode_path;


/*
 * z_piracy, branch if the story file is a legal copy.
 *
 *	no zargs used
 *
 */

void z_piracy (void)
{

    branch (!f_setup.piracy);

}/* z_piracy */

/*
 * main
 *
 * Prepare and run the game.
 *
 */

int cdecl frotz_main (void)
{

    os_init_setup ();

    init_buffer ();

    init_err ();

    init_memory ();

    init_process ();

    init_sound ();

    os_init_screen ();

    init_undo ();

    z_restart ();

    interpret ();

    script_close ();

    record_close ();

    replay_close ();

    reset_memory ();

    os_reset_screen ();

    return 0;

}/* main */
