/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*
 * Archos Jukebox Recorder button functions
 */

#include <stdlib.h>
#include "config.h"
#include "sh7034.h"
#include "button.h"
#include "kernel.h"

static struct event_queue button_queue;

#define POLL_FREQUENCY    HZ/10
#define REPEAT_START      3
#define REPEAT_INTERVAL   3

static int button_read(void);

static void button_tick(void)
{
    static int tick=0;
    static int count=0;
    static int lastbtn=0;
    static bool repeat=false;

    /* only poll every X ticks */
    if ( tick++ > POLL_FREQUENCY ) {
        bool post = false;
        int btn = button_read();

        if ( btn ) {
            /* normal keypress */
            if ( btn != lastbtn ) {
                post = true;
            }
            /* repeat? */
            else {
                if ( repeat ) {
                    if ( ! --count ) {
                        post = true;
                        count = REPEAT_INTERVAL;
                    }
                }
                else if (count++ > REPEAT_START) {
                    post = true;
                    repeat = true;
                    count = REPEAT_INTERVAL;
                }
            }
            if ( post )
                queue_post(&button_queue, btn, NULL);
        }
        else
            repeat = false;

        lastbtn = btn;
        tick = 0;
    }
}

int button_get(void)
{
    struct event ev;

    if ( !queue_empty(&button_queue) ) {
        queue_wait(&button_queue, &ev);
        return ev.id;
    }
    return BUTTON_NONE;
}

#ifdef HAVE_RECORDER_KEYPAD

/* AJBR buttons are connected to the CPU as follows:
 *
 * ON and OFF are connected to separate port B input pins.
 *
 * F1, F2, F3, and UP are connected to the AN4 analog input, each through
 * a separate voltage divider.  The voltage on AN4 depends on which button
 * (or none, or a combination) is pressed.
 *
 * DOWN, PLAY, LEFT, and RIGHT are likewise connected to AN5. */
 
/* Button voltage levels on AN4 and AN5 */
#define	LEVEL1		50
#define	LEVEL2		125
#define	LEVEL3		175
#define	LEVEL4		225

/* Number of calls to get_button for a button to stop bouncing
 * and to be considered held down.
 * Should really use a hardware timer for this.
 */
#define BOUNCE_COUNT	200
#define HOLD_COUNT		10000

static int last;	/* Last button pressed */
static int count;	/* Number of calls button has been down */


/*
 *Initialize buttons
 */
void button_init()
{
#ifndef SIMULATOR
    /* Set PB4 and PB8 as input pins */
    PBCR1 &= 0xfffc; /* PB8MD = 00 */
    PBCR2 &= 0xfcff; /* PB4MD = 00 */
    PBIOR &= ~(PBDR_BTN_ON|PBDR_BTN_OFF); /* Inputs */
  
    /* Set A/D to scan AN4 and AN5.
     * This needs to be changed to scan other analog pins
     * for battery level, etc. */
    ADCSR = 0;
    ADCR  = 0;
    ADCSR = ADCSR_ADST | ADCSR_SCAN | 0x5;
#endif
    last = BUTTON_NONE;
    count = 0;
    queue_init(&button_queue);
    tick_add_task(button_tick);
}

/*
 * Get button pressed from hardware
 */
static int get_raw_button (void)
{
    /* Check port B pins for ON and OFF */
    int data = PBDR;
    if ((data & PBDR_BTN_ON) == 0)
        return BUTTON_ON;
    else if ((data & PBDR_BTN_OFF) == 0)
        return BUTTON_OFF;

    /* Check AN4 pin for F1-3 and UP */
    data = ADDRAH;
    if (data >= LEVEL4)
        return BUTTON_F3;
    else if (data >= LEVEL3)
        return BUTTON_UP;
    else if (data >= LEVEL2)
        return BUTTON_F2;
    else if (data >= LEVEL1)
        return BUTTON_F1;

    /* Check AN5 pin for DOWN, PLAY, LEFT, RIGHT */
    data = ADDRBH;
    if (data >= LEVEL4)
        return BUTTON_DOWN;
    else if (data >= LEVEL3)
        return BUTTON_PLAY;
    else if (data >= LEVEL2)
        return BUTTON_LEFT;
    else if (data >= LEVEL1)
        return BUTTON_RIGHT;
  
    return BUTTON_NONE;
}

/*
 * Get the currently pressed button.
 * Returns one of BUTTON_xxx codes, with possibly a modifier bit set.
 * No modifier bits are set when the button is first pressed.
 * BUTTON_HELD bit is while the button is being held.
 * BUTTON_REL bit is set when button has been released.
 */
static int button_read(void)
{
    int btn = get_raw_button();
    int ret;

    /* Last button pressed is still down */
    if (btn != BUTTON_NONE && btn == last) {
        count++;
        if (count == BOUNCE_COUNT)
            return btn;
        else if (count >= HOLD_COUNT)
            return btn | BUTTON_HELD;
        else
            return BUTTON_NONE;
    }

    /* Last button pressed now released */
    if (btn == BUTTON_NONE && last != BUTTON_NONE)
        ret = last | BUTTON_REL;
    else
        ret = BUTTON_NONE;
    
    last = btn;
    count = 0;
    return ret;
}

#elif HAVE_PLAYER_KEYPAD

/* The player has all buttons on port pins:

   LEFT:  PC0
   RIGHT: PC2
   PLAY:  PC3
   STOP:  PA11
   ON:    PA5
   MENU:  PC1
*/

void button_init(void)
{
    /* set port pins as input */
    PAIOR &= ~0x820;
    queue_init(&button_queue);
    tick_add_task(button_tick);
}

static int button_read(void)
{
    int porta = PADR;
    int portc = PCDR;
    int btn = 0;
    
    /* buttons are active low */
    if ( !(portc & 1) )
        btn |= BUTTON_LEFT;
    if ( !(portc & 2) )
        btn |= BUTTON_MENU;
    if ( !(portc & 4) )
        btn |= BUTTON_RIGHT;
    if ( !(portc & 8) )
        btn |= BUTTON_PLAY | BUTTON_UP;
    if ( !(porta & 0x20) )
        btn |= BUTTON_ON;
    if ( !(porta & 0x800) )
        btn |= BUTTON_STOP | BUTTON_DOWN;

    return btn;
}

#endif



/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
