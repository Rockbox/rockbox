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
#include "debug.h"

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

extern int screenhack_handle_events(bool *release, bool *repeat);

static int get_raw_button (void)
{
    int k;
    bool release=false; /* is this a release event */
    bool repeat=false;  /* is the key a repeated one */
    int ev=screenhack_handle_events(&release, &repeat);
    switch(ev)
    {
	case XK_KP_Left:
	case XK_Left:
	case XK_KP_4:
	    k = BUTTON_LEFT;
            break;

	case XK_KP_Right:
	case XK_Right:
	case XK_KP_6:
	    k = BUTTON_RIGHT;
            break;

	case XK_KP_Up:
	case XK_Up:
	case XK_KP_8:
	    k = BUTTON_UP;
            break;

	case XK_KP_Down:
	case XK_Down:
	case XK_KP_2:
	    k = BUTTON_DOWN;
            break;

#ifdef HAVE_RECORDER_KEYPAD
	case XK_KP_Space:
	case XK_KP_5:
	case XK_KP_Begin:
	case XK_space:
	    k = BUTTON_PLAY;
            break;

	case XK_KP_Enter:
	case XK_A:
	case XK_a:
	    k = BUTTON_OFF;
            break;

	case XK_KP_Add:
	case XK_Q:
	case XK_q:
	    k = BUTTON_ON;
            break;

	case XK_KP_Divide:
	case XK_1:
	    k = BUTTON_F1;
            break;

	case XK_KP_Multiply:
	case XK_2:
	    k = BUTTON_F2;
            break;

	case XK_KP_Subtract:
	case XK_3:
	    k = BUTTON_F3;
            break;
#else
	case XK_KP_Add:
	case XK_Q:
	case XK_q:
	    k = BUTTON_ON;
            break;

	case XK_KP_Enter:
	case XK_Return:
	    k = BUTTON_MENU;
            break;
#endif

	default:
	    k = 0;
            if(ev)
                DEBUGF("received ev %d\n", ev);
            break;
    }

    if(release)
        /* return a release event */
        k |= BUTTON_REL;

    if(repeat)
        k |= BUTTON_REPEAT;

    return k;
}

/*
 * Timeout after TICKS unless a key is pressed.
 */
int button_get_w_tmo(int ticks)
{
    int bits;
    int i=0;

    for(i=0; i< ticks; i++) {
        bits = get_raw_button();
        if(!bits)
            x11_sleep(1);
        else
            break;
    }

    return bits;
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

    if(!block)
      /* delay a bit */
      x11_sleep(1);

    return bits;
}
