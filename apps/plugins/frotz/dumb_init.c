/* dumb-init.c
 * $Id: dumb-init.c,v 1.1.1.1 2002/03/26 22:38:34 feedle Exp $
 *
 * Copyright 1997,1998 Alva Petrofsky <alva@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 *
 * Changes for Rockbox copyright 2009 Torne Wuff
 */

#include "dumb_frotz.h"
#include "lib/pluginlib_exit.h"

static int user_screen_width = LCD_WIDTH / SYSFONT_WIDTH;
static int user_screen_height = LCD_HEIGHT / SYSFONT_HEIGHT;
static int user_interpreter_number = -1;
static int user_random_seed = -1;
static int user_tandy_bit = 0;


void os_init_screen(void)
{
  if (h_version == V3 && user_tandy_bit)
    h_config |= CONFIG_TANDY;

  if (h_version >= V5 && f_setup.undo_slots == 0)
    h_flags &= ~UNDO_FLAG;

  h_screen_rows = user_screen_height;
  h_screen_cols = user_screen_width;

  if (user_interpreter_number > 0)
    h_interpreter_number = user_interpreter_number;
  else {
    /* Use ms-dos for v6 (because that's what most people have the
     * graphics files for), but don't use it for v5 (or Beyond Zork
     * will try to use funky characters).  */
    h_interpreter_number = h_version == 6 ? INTERP_MSDOS : INTERP_DEC_20;
  }
  h_interpreter_version = 'F';

  if (h_version >= V4)
    h_config |= CONFIG_TIMEDINPUT;

  if (h_version >= V5)
    h_flags &= ~(MOUSE_FLAG | MENU_FLAG);

  dumb_init_output();

  h_flags &= ~GRAPHICS_FLAG;
}

int os_random_seed (void)
{
  if (user_random_seed == -1)
    /* Use the rockbox tick as seed value */
    return ((int)*rb->current_tick) & 0x7fff;
  else return user_random_seed;
}

void os_restart_game (int stage) { (void)stage; }

void os_fatal (const char *s)
{
  rb->splash(HZ*10, s);
  exit(1);
}

void os_init_setup(void)
{
	f_setup.attribute_assignment = 0;
	f_setup.attribute_testing = 0;
	f_setup.context_lines = 0;
	f_setup.object_locating = 0;
	f_setup.object_movement = 0;
	f_setup.left_margin = 0;
	f_setup.right_margin = 0;
	f_setup.ignore_errors = 0;
	f_setup.piracy = 0;
	f_setup.undo_slots = MAX_UNDO_SLOTS;
	f_setup.expand_abbreviations = 0;
	f_setup.script_cols = 80;
	f_setup.save_quetzal = 1;
	f_setup.sound = 1;
	f_setup.err_report_mode = ERR_DEFAULT_REPORT_MODE;

}
