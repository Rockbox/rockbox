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
#include <stdlib.h>
#include "config.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"
#include "misc.h"

#include "X11/keysym.h"

extern int screenhack_handle_events(bool *release);

struct event_queue button_queue;

static int button_state = 0; /* keeps track of pressed keys */
static long lastbtn;   /* Last valid button status */

/* how often we check to see if a button is pressed */
#define POLL_FREQUENCY    HZ/25

/* how long until repeat kicks in */
#define REPEAT_START      8

/* the speed repeat starts at */
#define REPEAT_INTERVAL_START   4

/* speed repeat finishes at */
#define REPEAT_INTERVAL_FINISH  2

/* mostly copied from real button.c */
void button_read (void);

void button_tick(void)
{
    static int tick = 0;
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
    int diff;
    int btn;

    /* only poll every X ticks */
    if ( ++tick >= POLL_FREQUENCY )
    {
        bool post = false;
        button_read();
        btn = button_state;

        /* Find out if a key has been released */
        diff = btn ^ lastbtn;
        if(diff && (btn & diff) == 0)
        {
            queue_post(&button_queue, BUTTON_REL | diff, NULL);
        }
        else
        {
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
                        if (count == 0) {
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
                    if (repeat)
                        queue_post(&button_queue, BUTTON_REPEAT | btn, NULL);
                    else
                        queue_post(&button_queue, btn, NULL);
                }
            }
            else
            {
                repeat = false;
                count = 0;
            }
        }
        lastbtn = btn & ~(BUTTON_REL | BUTTON_REPEAT);
        tick = 0;
    }
}

/*
 * Read X keys and translate to rockbox buttons
 */

void button_read (void)
{
    int k;
    bool release = false; /* is this a release event */
    int ev = screenhack_handle_events(&release);

    switch (ev)
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
#ifdef BUTTON_UP
            k = BUTTON_UP;
#elif defined BUTTON_PLAY
            k = BUTTON_PLAY;
#endif
            break;

        case XK_KP_Down:
        case XK_Down:
        case XK_KP_2:
#ifdef BUTTON_DOWN
            k = BUTTON_DOWN;
#elif defined BUTTON_STOP
            k = BUTTON_STOP;
#endif
            break;

#ifdef BUTTON_ON
        case XK_KP_Add:
        case XK_Q:
        case XK_q:
            k = BUTTON_ON;
            break;
#endif

#ifdef BUTTON_OFF
        case XK_KP_Enter:
        case XK_A:
        case XK_a:
            k = BUTTON_OFF;
            break;
#endif

#ifdef BUTTON_F1
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
#elif defined(BUTTON_REC)
        case XK_KP_Divide:
        case XK_1:
            k = BUTTON_REC;
            break;
#endif

        case XK_KP_Space:
        case XK_KP_5:
        case XK_KP_Begin:
        case XK_space:
#ifdef BUTTON_PLAY
            k = BUTTON_PLAY;
#elif defined(BUTTON_SELECT)
            k = BUTTON_SELECT;
#endif
            break;

#ifdef HAVE_LCD_BITMAP
        case XK_5:
            if(!release)
                screen_dump();
            break;
#endif

        case XK_KP_Separator:
        case XK_KP_Insert:
        case XK_Insert:
#ifdef BUTTON_MENU
            k = BUTTON_MENU;
#elif defined(BUTTON_MODE)
            k = BUTTON_MODE;
#endif
            break;

        default:
            k = 0;
            if(ev)
                DEBUGF("received ev %d\n", ev);
            break;
    }

    if (release)
        button_state &= ~k;
    else
        button_state |= k;
}

/* Again copied from real button.c... */

long button_get(bool block)
{
    struct event ev;

    if ( block || !queue_empty(&button_queue) ) 
    {
        queue_wait(&button_queue, &ev);
        return ev.id;
    }
    return BUTTON_NONE;
}

long button_get_w_tmo(int ticks)
{
    struct event ev;
    queue_wait_w_tmo(&button_queue, &ev, ticks);
    return (ev.id != SYS_TIMEOUT)? ev.id: BUTTON_NONE;
} 

void button_init(void)
{
}

int button_status(void)
{
    return lastbtn;
}

void button_clear_queue(void)
{
    queue_clear(&button_queue);
}

#if CONFIG_KEYPAD == IRIVER_H100_PAD
bool button_hold(void) {
    /* temp fix for hold button on irivers */
    return false;
}
bool remote_button_hold(void) {
    /* temp fix for hold button on irivers */
    return false;
}
#endif
