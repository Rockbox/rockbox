/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <windows.h>
#include "uisw32.h"
#include "config.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "misc.h"

/* how long until repeat kicks in */
#define REPEAT_START      6

/* the speed repeat starts at */
#define REPEAT_INTERVAL_START   4

/* speed repeat finishes at */
#define REPEAT_INTERVAL_FINISH  2

struct event_queue button_queue;

static int btn = 0;    /* Hopefully keeps track of currently pressed keys... */

void button_event(int key, bool pressed)
{
    bool post = false;
    int new_btn = 0;
    int diff = 0;
    static int count = 0;
    static int lastbtn;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;

    switch (key)
    {
    case VK_NUMPAD4:
    case VK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case VK_NUMPAD6:
    case VK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;

    case VK_NUMPAD8:
    case VK_UP:
#ifdef BUTTON_UP
        new_btn = BUTTON_UP;
#elif defined BUTTON_PLAY
        new_btn = BUTTON_PLAY;
#endif
        break;

    case VK_NUMPAD2:
    case VK_DOWN:
#ifdef BUTTON_DOWN
        new_btn = BUTTON_DOWN;
#elif defined BUTTON_STOP
        new_btn = BUTTON_STOP;
#endif
        break;

#ifdef BUTTON_ON
    case VK_ADD:
        new_btn = BUTTON_ON;
        break;
#endif

#ifdef BUTTON_OFF
    case VK_RETURN:
        new_btn = BUTTON_OFF;
        break;
#endif

#ifdef BUTTON_F1
    case VK_DIVIDE:
    case VK_F1:
        new_btn = BUTTON_F1;
        break;
    case VK_MULTIPLY:
    case VK_F2:
        new_btn = BUTTON_F2;
        break;
    case VK_SUBTRACT:
    case VK_F3:
        new_btn = BUTTON_F3;
        break;
#endif

#ifdef BUTTON_PLAY
    case VK_NUMPAD5:
    case VK_SPACE:
        new_btn = BUTTON_PLAY;
        break;
#endif

#ifdef HAVE_LCD_BITMAP
    case VK_NUMPAD0:
    case VK_F5:
        if(pressed)
        {
            screen_dump();
            return;
        }
        break;
#endif

#ifdef BUTTON_MENU
#if CONFIG_KEYPAD == PLAYER_PAD
    case VK_RETURN:
#elif CONFIG_KEYPAD == ONDIO_PAD
    case VK_INSERT:
#endif
        new_btn = BUTTON_MENU;
        break;
#endif
    }

    if (pressed)
        btn |= new_btn;
    else
        btn &= !new_btn;

    /* Lots of stuff copied from real button.c. Not good, I think... */

    /* Find out if a key has been released */
    diff = btn ^ lastbtn;

    if(diff && (btn & diff) == 0)
    {
        queue_post(&button_queue, BUTTON_REL | diff, NULL);
    }

    if ( btn )
    {
        /* normal keypress */
        if ( btn != lastbtn )
        {
            post = true;
            repeat = false;
            repeat_speed = REPEAT_INTERVAL_START;

        }
        else /* repeat? */
        {
            if ( repeat )
            {
                count--;
                if (count == 0)
                {
                    post = true;
                    /* yes we have repeat */
                    repeat_speed--;
                    if (repeat_speed < REPEAT_INTERVAL_FINISH)
                       repeat_speed = REPEAT_INTERVAL_FINISH;
                    count = repeat_speed;

                    repeat_count++;
                }
            }
            else
            {
                if (count++ > REPEAT_START)
                {
                    post = true;
                    repeat = true;
                    repeat_count = 0;
                    /* initial repeat */
                    count = REPEAT_INTERVAL_START;
                }
            }
        }

        if ( post )
        {
            if(repeat)
                queue_post(&button_queue, BUTTON_REPEAT | btn, NULL);
            else
                queue_post(&button_queue, btn, NULL);

            backlight_on();
        }
        }
    else
    {
        repeat = false;
        count = 0;
    }

    lastbtn = btn & ~(BUTTON_REL | BUTTON_REPEAT);
}

int button_status(void)
{
    return btn;
}
   
void button_init(void)
{
}

/* Again copied from real button.c... */

int button_get(bool block)
{
    struct event ev;

    if ( block || !queue_empty(&button_queue) ) {
        queue_wait(&button_queue, &ev);
        return ev.id;
    }
    return BUTTON_NONE;
}

int button_get_w_tmo(int ticks)
{
    struct event ev;
    queue_wait_w_tmo(&button_queue, &ev, ticks);
    return (ev.id != SYS_TIMEOUT)? ev.id: BUTTON_NONE;
} 

void button_clear_queue(void)
{
    queue_empty(&button_queue);
}
