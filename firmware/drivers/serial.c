/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include "button.h"
#include "config.h"
#include "sh7034.h"
#include "system.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "lcd.h"
#include "serial.h"

/* Recieved byte identifiers */
#define PLAY  0xC1
#define STOP  0xC2
#define PREV  0xC4
#define NEXT  0xC8
#define VOLUP 0xD0
#define VOLDN 0xE0

void serial_setup (void) 
{ 
    char dummy;
    dummy = SSR1;
    SSR1 = 0;
    SMR1 = 0x00;
    SCR1 = 0;
    BRR1 = (FREQ/(32*9600))-1;

    /* let the hardware settle */
    sleep(1);

    SCR1 = 0x50;

    /* This enables the serial Rx interrupt*/
    IPRE = (IPRE & 0x0FFF) | 0x8000; /* Set to medium priority */
}

static void process_byte(int byte)
{
    int btn = 0;

    switch (byte)
    {
        case STOP:
#ifdef HAVE_RECORDER_KEYPAD
            btn = BUTTON_OFF;
#else
            btn = BUTTON_STOP;
#endif
            break;

        case PLAY:
            btn = BUTTON_PLAY;
            break;

        case VOLUP:
            btn = BUTTON_VOL_UP;
            break;

        case VOLDN:
            btn = BUTTON_VOL_DOWN;
            break;

        case PREV:
            btn = BUTTON_LEFT;
            break;

        case NEXT:
            btn = BUTTON_RIGHT;
            break;
    }

    if ( btn ) {
        queue_post(&button_queue, btn, NULL);
        backlight_on();
        queue_post(&button_queue, btn | BUTTON_REL, NULL);
    }
}

#pragma interrupt
void REI1 (void)
{
    SSR1 = SSR1 & ~0x10; /* Clear FER */
    SSR1 = SSR1 & ~0x40; /* Clear RDRF */
}

#pragma interrupt
void RXI1 (void)
{
    unsigned char serial_byte;
    serial_byte = RDR1;
    SSR1 = SSR1 & ~0x40; /* Clear RDRF */
    process_byte(serial_byte);
}
