/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#define HAVE_RECORDER_KEYPAD
#include "types.h"
#include "button.h"

#include "X11/keysym.h"

/*
 *Initialize buttons
 */
void button_init()
{
}

/*
 * Get button pressed from hardware
 */
static int get_raw_button (void)
{
    int k = screenhack_handle_events();
    switch(k)
    {
	case XK_KP_Left:
	case XK_Left:
	case XK_KP_4:
	    return BUTTON_LEFT;
	case XK_KP_Right:
	case XK_Right:
	case XK_KP_6:
	    return BUTTON_RIGHT;
	case XK_KP_Up:
	case XK_Up:
	case XK_KP_8:
	    return BUTTON_UP;
	case XK_KP_Down:
	case XK_Down:
	case XK_KP_2:
	    return BUTTON_DOWN;
	default:
	    return 0;
    }
}

/*
 * Get the currently pressed button.
 * Returns one of BUTTON_xxx codes, with possibly a modifier bit set.
 * No modifier bits are set when the button is first pressed.
 * BUTTON_HELD bit is while the button is being held.
 * BUTTON_REL bit is set when button has been released.
 */
int button_get(void)
{
    return get_raw_button();
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */
