/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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

#ifndef _GUI_STATUSBAR_H_
#define _GUI_STATUSBAR_H_

#include "status.h"
#include "screen_access.h"
#include "events.h"

#define STATUSBAR_X_POS                         0
#define STATUSBAR_Y_POS                         0 /* MUST be a multiple of 8 */
#define STATUSBAR_HEIGHT                        8

struct status_info {
    int battlevel;
    int batt_charge_step;
    int volume;
    int playmode;
    int repeat;
#if CONFIG_CHARGING
    bool inserted;
#endif
#ifdef HAVE_USB_POWER
    bool usb_inserted;
#endif
    bool battery_state;
    bool shuffle;
    bool keylock;
#ifdef HAS_REMOTE_BUTTON_HOLD
    bool keylockremote;
#endif
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    bool led; /* disk LED simulation in the status bar */
#endif

};

struct gui_statusbar
{
    long battery_icon_switch_tick;

    long volume_icon_switch_tick;
    int last_volume;
    bool redraw_volume; /* true if the volume gauge needs updating */

    struct status_info info;
    struct status_info lastinfo;
#if CONFIG_RTC
    struct tm *time;
    int	last_tm_min;
#endif
    struct screen * display;
};


extern struct gui_syncstatusbar statusbars;

/*
 * Attach the status bar to a screen
 * (The previous screen attachement is lost)
 *  - bar : the statusbar structure
 *  - display : the screen to attach
 */
#define gui_statusbar_set_screen(gui_statusbar, _display) \
    (gui_statusbar)->display = (_display);


/*
 * Draws the status bar on the attached screen
 * - bar : the statusbar structure
 */
extern void gui_statusbar_draw(struct gui_statusbar * bar, bool force_redraw);


struct gui_syncstatusbar
{
    struct gui_statusbar statusbars[NB_SCREENS];
};

extern void gui_syncstatusbar_init(struct gui_syncstatusbar * bars);
extern void gui_syncstatusbar_draw(struct gui_syncstatusbar * bars, bool force_redraw);
extern int  gui_statusbar_height(void);
#endif /*_GUI_STATUSBAR_H_*/
