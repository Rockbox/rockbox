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
#include "button.h"
#include "kernel.h"

#include "X11/keysym.h"

static int release_mask;
static int repeat_mask;

/*
 *Initialize buttons
 */
void button_init()
{
}

int button_set_repeat(int newmask)
{
    int oldmask = repeat_mask;
    repeat_mask = newmask;
    return oldmask;
}

int button_set_release(int newmask)
{
    int oldmask = release_mask;
    release_mask = newmask;
    return oldmask;
}

/*
 * Translate X keys to Recorder keys
 *
 * We simulate recorder keys on the numeric keypad:
 *
 * 4,6,8,2 = Left, Right, Up, Down
 * 5 = Play/pause
 * Div,Mul,Sub = The tree menu keys
 * +,Enter = On, Off
 *
 * Alternative Keys For Laptop or VNC Users
 * Recorder:
 * Space=Play Q=On A=Off 1,2,3 = F1,F2,F3
 * Player:
 * Q=On Return=Menu
 */

/* from uibasic.c */
extern int screenhack_handle_events (void);

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

#ifdef HAVE_RECORDER_KEYPAD
	case XK_KP_Space:
	case XK_KP_5:
	case XK_KP_Begin:
	case XK_space:
	    return BUTTON_PLAY;

	case XK_KP_Enter:
	case XK_A:
	case XK_a:
	    return BUTTON_OFF;

	case XK_KP_Add:
	case XK_Q:
	case XK_q:
	    return BUTTON_ON;

	case XK_KP_Divide:
	case XK_1:
	    return BUTTON_F1;

	case XK_KP_Multiply:
	case XK_2:
	    return BUTTON_F2;

	case XK_KP_Subtract:
	case XK_3:
	    return BUTTON_F3;
#else
	case XK_KP_Add:
	case XK_Q:
	case XK_q:
	    return BUTTON_ON;

	case XK_KP_Enter:
	case XK_Return:
	    return BUTTON_MENU;
#endif

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
int button_get(bool block)
{
    int bits;
    do {
        bits = get_raw_button();
        if(block && !bits)
            x11_sleep(HZ/10);
        else
            break;
    } while(1);

    return bits;
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */
