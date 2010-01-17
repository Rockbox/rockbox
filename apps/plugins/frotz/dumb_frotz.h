/* dumb-frotz.h
 * $Id: dumb-frotz.h,v 1.1.1.1 2002/03/26 22:38:34 feedle Exp $
 * Frotz os functions for a standard C library and a dumb terminal.
 * Now you can finally play Zork Zero on your Teletype.
 *
 * Copyright 1997, 1998 Alembic Petrofsky <alembic@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 *
 * Changes for Rockbox copyright 2009 Torne Wuff
 */
#include "frotz.h"

/* from ../common/setup.h */
extern f_setup_t f_setup;

/* dumb-output.c */
void dumb_init_output(void);
void dumb_show_screen(bool show_cursor);
void dumb_dump_screen(void);
