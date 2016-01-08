 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#include "xrick/system/system.h"

#include "xrick/config.h"
#include "xrick/control.h"
#include "xrick/game.h"
#include "xrick/system/sysmenu_rockbox.h"
#include "xrick/system/rockboxcodes.h"

/*
 * Helper function to set/clear controls according to key press
 */
static inline void checkKey(int key, unsigned button, control_t control)
{
    if (key & button)
    {
        control_set(control);
    }
    else
    {
        control_clear(control);
    }
}

/*
 * Process events, if any, then return
 */
void sysevt_poll(void)
{
    static int previousKey, currentKey;

    /* this is because "Restart Game" is handled via menu */
    if (control_test(Control_END))
    {
        control_clear(Control_END);
    }

    for (;;)
    {
        /* check for USB connection */
        if ((rb->default_event_handler(rb->button_get(false)) == SYS_USB_CONNECTED)
#if defined(HAS_BUTTON_HOLD)
              || rb->button_hold()
#endif
            )
        {
            sysmenu_exec();
        }

        currentKey = rb->button_status();
        if (currentKey != previousKey)
        {
            break;
        }
        else if (game_waitevt)
        {
            rb->yield();
        }
        else /* (currentKey == previousKey) && !game_waitevt */
        {
            return;
        }
    }

#ifdef XRICK_BTN_MENU
    if (currentKey & XRICK_BTN_MENU)
    {
        sysmenu_exec();
    }
#endif

#ifdef XRICK_BTN_PAUSE
    checkKey(currentKey, XRICK_BTN_PAUSE, Control_PAUSE);
#endif

    checkKey(currentKey, XRICK_BTN_UP, Control_UP);

    checkKey(currentKey, XRICK_BTN_DOWN, Control_DOWN);

    checkKey(currentKey, XRICK_BTN_LEFT, Control_LEFT);

    checkKey(currentKey, XRICK_BTN_RIGHT, Control_RIGHT);

    checkKey(currentKey, XRICK_BTN_FIRE, Control_FIRE);

#ifdef XRICK_BTN_UPLEFT
    if (!control_test(Control_UP | Control_LEFT))
    {
        checkKey(currentKey, XRICK_BTN_UPLEFT, Control_UP | Control_LEFT);
    }
#endif /* XRICK_BTN_UPLEFT */

#ifdef XRICK_BTN_UPRIGHT
    if (!control_test(Control_UP | Control_RIGHT))
    {
        checkKey(currentKey, XRICK_BTN_UPRIGHT, Control_UP | Control_RIGHT);
    }
#endif /* XRICK_BTN_UPRIGHT */

#ifdef XRICK_BTN_DOWNLEFT
    if (!control_test(Control_DOWN | Control_LEFT))
    {
        checkKey(currentKey, XRICK_BTN_DOWNLEFT, Control_DOWN | Control_LEFT);
    }
#endif /* XRICK_BTN_DOWNLEFT */

#ifdef XRICK_BTN_DOWNRIGHT
    if (!control_test(Control_DOWN | Control_RIGHT))
    {
        checkKey(currentKey, XRICK_BTN_DOWNRIGHT, Control_DOWN | Control_RIGHT);
    }
#endif /* XRICK_BTN_DOWNRIGHT */

    previousKey = currentKey;
}

/*
 * Wait for an event, then process it and return
 */
void sysevt_wait(void)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    sysevt_poll(); /* sysevt_poll deals with blocking case as well */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
}

/* eof */
