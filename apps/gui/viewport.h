/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jonathan Gordon
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

#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__


#include "config.h"
#include "lcd.h"
#include "system.h"
#include "screen_access.h"

/* return the number of text lines in the vp viewport */
int viewport_get_nb_lines(const struct viewport *vp);

#define THEME_STATUSBAR             (BIT_N(0))
#define THEME_UI_VIEWPORT           (BIT_N(1))
#define THEME_BUTTONBAR             (BIT_N(2))
#define THEME_LANGUAGE              (BIT_N(3))
#define THEME_LISTS                 (BIT_N(3))
#define THEME_ALL                   (~(0u))

/* These are needed in checkwps */
void viewport_set_defaults(struct viewport *vp,
                            const enum screen_type screen);
void viewport_set_fullscreen(struct viewport *vp,
                              const enum screen_type screen);
int get_viewport_default_colour(enum screen_type screen, bool fgcolour);

#ifndef __PCTOOL__

/*
 * Initialize the viewportmanager, which in turns initializes the UI vp and
 * statusbar stuff
 */
void viewportmanager_init(void) INIT_ATTR;

void viewportmanager_theme_enable(enum screen_type screen, bool enable,
                                 struct viewport *viewport);
/* Force will cause a redraw even if the theme was previously and
 * currently enabled (i,e the undo doing nothing).
 * Should almost always be set to false except coming out of fully skinned screens */
void viewportmanager_theme_undo(enum screen_type screen, bool force_redraw);

/* call this when a theme changed */
void viewportmanager_theme_changed(const int);

void viewport_set_buffer(struct viewport *vp, struct frame_buffer_t *buffer,
                                                const enum screen_type screen);

#ifdef HAVE_TOUCHSCREEN
bool viewport_point_within_vp(const struct viewport *vp,
                               const int x, const int y);
#endif

#endif /* __PCTOOL__ */

#endif /* __VIEWPORT_H__ */
