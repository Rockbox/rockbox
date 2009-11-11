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

void viewport_set_defaults(struct viewport *vp,
                            const enum screen_type screen);

/* Used to specify which screens the statusbar (SB) should be displayed on.
 *
 * The parameter is a bit OR'ed combination of the following (screen is
 * SCREEN_MAIN or SCREEN_REMOTE from screen_access.h):
 *
 * VP_SB_HIDE_ALL means "hide the SB on all screens"
 * VP_SB_ONSCREEN(screen) means "display the SB on the given screen
 *                              as specified by the SB setting for that screen"
 * VP_SB_IGNORE_SETTING(screen) means "ignore the SB setting for that screen"
 * VP_SB_ALLSCREENS means "VP_SB_ONSCREEN for all screens"
 * 
 * In most cases, VP_SB_ALLSCREENS should be used which means display the SB
 * as specified by the settings.
 * For the WPS (and other possible exceptions) use VP_SB_IGNORE_SETTING() to
 * FORCE the statusbar on for the given screen (i.e it will show regardless
 * of the setting)
 *
 * Returns the status before the call. This value can be used to restore the
 * SB "displaying rules".
 */


#define THEME_STATUSBAR             (BIT_N(0))
#define THEME_UI_VIEWPORT           (BIT_N(1))
#define THEME_BUTTONBAR             (BIT_N(2))
#define THEME_LANGUAGE              (BIT_N(3))
#define THEME_ALL                   (~(0u))

#define VP_SB_HIDE_ALL                          0
#define VP_SB_ONSCREEN(screen)                  BIT_N(screen)
#define VP_SB_IGNORE_SETTING(screen)            BIT_N(4+screen)
#define VP_SB_ALLSCREENS            (VP_SB_ONSCREEN(0)|VP_SB_ONSCREEN(1))

#ifndef __PCTOOL__
/*
 * Initialize the viewportmanager, which in turns initializes the UI vp and
 * statusbar stuff
 */
void viewportmanager_init(void);
int viewportmanager_get_statusbar(void);
int viewportmanager_set_statusbar(const int enabled);


/*
 * Initializes the given viewport with maximum dimensions minus status- and
 * buttonbar
 */
void viewport_set_fullscreen(struct viewport *vp,
                              const  enum screen_type screen);

#ifdef HAVE_LCD_BITMAP

/* call this when a theme changed */
void viewportmanager_theme_changed(const int);

#ifdef HAVE_TOUCHSCREEN
bool viewport_point_within_vp(const struct viewport *vp,
                               const int x, const int y);
#endif

#else /* HAVE_LCD_CHARCELL */
#define viewport_set_current_vp(a)
#define viewport_get_current_vp() NULL
#define viewportmanager_theme_changed(a)
#endif

#endif /* __PCTOOL__ */

#ifdef HAVE_LCD_BITMAP

/*
 * Parse a viewport definition (vp_def), which looks like:
 *
 * Screens with depth > 1:
 *   X|Y|width|height|font|foregorund color|background color
 * Screens with depth = 1:
 *   X|Y|width|height|font
 *
 * | is a separator and can be specified via the parameter
 *
 * Returns the pointer to the char after the last character parsed
 * if everything went OK or NULL if an error happened (some values
 * not specified in the definition)
 */
const char* viewport_parse_viewport(struct viewport *vp,
                                    enum screen_type screen,
                                    const char *vp_def,
                                    const char separator);
#endif /* HAVE_LCD_BITMAP */
#endif /* __VIEWPORT_H__ */
