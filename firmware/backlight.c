/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include "sh7034.h"
#include "kernel.h"
#include "thread.h"
#include "i2c.h"
#include "debug.h"
#include "rtc.h"
#include "usb.h"

#define BACKLIGHT_ON 1
#define BACKLIGHT_OFF 2

static void backlight_thread(void);
static char backlight_stack[0x100];
static struct event_queue backlight_queue;

static int backlight_timer;
static int backlight_timeout = 5;

void backlight_thread(void)
{
    struct event ev;
    
    while(1)
    {
        queue_wait(&backlight_queue, &ev);
        switch(ev.id)
        {
            case BACKLIGHT_ON:
                backlight_timer = HZ*backlight_timeout;
                if(backlight_timer)
                {
#ifdef HAVE_RTC
                    rtc_write(0x13, 0x10);
#else
                    PADR |= 0x40;
#endif
                }
                break;
		
            case BACKLIGHT_OFF:
#ifdef HAVE_RTC
                rtc_write(0x13, 0x00);
#else
                PADR &= ~0x40;
#endif                
                break;
		
	    case SYS_USB_CONNECTED:
		/* Tell the USB thread that we are safe */
		DEBUGF("backlight_thread got SYS_USB_CONNECTED\n");
		usb_acknowledge(SYS_USB_CONNECTED_ACK);

		/* Wait until the system reboots */
		while(1)
		    yield();
		break;
        }
    }
}

void backlight_on(void)
{
    queue_post(&backlight_queue, BACKLIGHT_ON, NULL);
}

void backlight_off(void)
{
    queue_post(&backlight_queue, BACKLIGHT_OFF, NULL);
}

void backlight_time(int seconds)
{
    backlight_timeout = seconds;
    backlight_on();
}

void backlight_tick(void)
{
    if(backlight_timer)
    {
        backlight_timer--;
        if(backlight_timer == 0)
        {
            backlight_off();
        }
    }
}

void backlight_init(void)
{
    queue_init(&backlight_queue);
    create_thread(backlight_thread, backlight_stack, sizeof(backlight_stack));
    backlight_on();
}
