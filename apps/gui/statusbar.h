/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin FERRARE
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _GUI_STATUSBAR_H_
#define _GUI_STATUSBAR_H_

#include "config.h"
#include "status.h"

#define STATUSBAR_X_POS                         0
#define STATUSBAR_Y_POS                         0 /* MUST be a multiple of 8 */
#define STATUSBAR_HEIGHT                        8

struct status_info {
    int battlevel;
    int volume;
#ifdef HAVE_RTC
    int hour;
    int minute;
#endif
    int playmode;
    int repeat;
    bool inserted;
    bool shuffle;
    bool keylock;
    bool battery_safe;
    bool redraw_volume; /* true if the volume gauge needs updating */
#if CONFIG_LED == LED_VIRTUAL
    bool led; /* disk LED simulation in the status bar */
#endif
#ifdef HAVE_USB_POWER
    bool usb_power;
#endif
#ifdef HAVE_CHARGING
    int battery_charge_step;
#endif
};

struct gui_statusbar
{
    /* Volume icon stuffs */
    long volume_icon_switch_tick;
    int last_volume;

    long battery_icon_switch_tick;

#ifdef HAVE_CHARGING
    int battery_charge_step;
#endif

    struct status_info info;
    struct status_info lastinfo;

    struct screen * display;
};

/*
 * Initializes a status bar
 *  - bar : the bar to initialize
 */
extern void gui_statusbar_init(struct gui_statusbar * bar);

/*
 * Attach the status bar to a screen
 * (The previous screen attachement is lost)
 *  - bar : the statusbar structure
 *  - display : the screen to attach
 */
extern void gui_statusbar_set_screen(struct gui_statusbar * bar, struct screen * display);

/*
 * Draws the status bar on the attached screen
 * - bar : the statusbar structure
 */
extern void gui_statusbar_draw(struct gui_statusbar * bar, bool force_redraw);

void gui_statusbar_icon_battery(struct screen * display, int percent);
bool gui_statusbar_icon_volume(struct gui_statusbar * bar, int percent);
void gui_statusbar_icon_play_state(struct screen * display, int state);
void gui_statusbar_icon_play_mode(struct screen * display, int mode);
void gui_statusbar_icon_shuffle(struct screen * display);
void gui_statusbar_icon_lock(struct screen * display);
#if CONFIG_LED == LED_VIRTUAL
void gui_statusbar_led(struct screen * display);
#endif

#ifdef HAVE_RTC
void gui_statusbar_time(struct screen * display, int hour, int minute);
#endif


struct gui_syncstatusbar
{
    struct gui_statusbar statusbars[NB_SCREENS];
};

extern void gui_syncstatusbar_init(struct gui_syncstatusbar * bars);
extern void gui_syncstatusbar_draw(struct gui_syncstatusbar * bars, bool force_redraw);

#endif /*_GUI_STATUSBAR_H_*/
