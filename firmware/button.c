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

#include "config.h"

#ifdef HAVE_RECORDER_KEYPAD

#include "types.h"
#include "sh7034.h"
#include "button.h"

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
int button_get(void)
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

#endif /* HAVE_RECORDER_KEYPAD */

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */
