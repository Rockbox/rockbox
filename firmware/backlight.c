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
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "i2c.h"
#include "debug.h"
#include "rtc.h"
#include "usb.h"
#include "power.h"
#include "system.h"

#ifdef HAVE_BACKLIGHT

const char backlight_timeout_value[19] =
{
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60, 90
};

#define BACKLIGHT_ON 1
#define BACKLIGHT_OFF 2

static void backlight_thread(void);
static char backlight_stack[DEFAULT_STACK_SIZE];
static const char backlight_thread_name[] = "backlight";
static struct event_queue backlight_queue;

static bool charger_was_inserted = 0;
static bool backlight_on_when_charging = 0;

static int backlight_timer;
static unsigned int backlight_timeout = 5;

static void __backlight_off(void)
{
#ifdef IRIVER_H100
    GPIO1_OUT  |= 0x00020000;
#else
#ifdef HAVE_RTC
    /* Disable square wave */
    rtc_write(0x0a, rtc_read(0x0a) & ~0x40);
#else
    and_b(~0x40, &PAIORH);
#endif
#endif
}

static void __backlight_on(void)
{
#ifdef IRIVER_H100
    GPIO1_OUT  &= ~0x00020000;
#else
#ifdef HAVE_RTC
    /* Enable square wave */
    rtc_write(0x0a, rtc_read(0x0a) | 0x40);
#else
    and_b(~0x40, &PADRH);
    or_b(0x40, &PAIORH);
#endif
#endif
}

void backlight_thread(void)
{
    struct event ev;
    
    while(1)
    {
        queue_wait(&backlight_queue, &ev);
        switch(ev.id)
        {
            case BACKLIGHT_ON:
                if( backlight_on_when_charging && charger_inserted() )
                {
                    /* Forcing to zero keeps the lights on */
                    backlight_timer = 0;
                }
                else
                {
                    backlight_timer = HZ*backlight_timeout_value[backlight_timeout];
                }

                if(backlight_timer < 0) /* Backlight == OFF in the setting? */
                {
                    backlight_timer = 0; /* Disable the timeout */
                    __backlight_off();
                }
                else 
                {
                    __backlight_on();
                }
                break;
                
            case BACKLIGHT_OFF:
                __backlight_off();
                break;
                
            case SYS_USB_CONNECTED:
                /* Tell the USB thread that we are safe */
                DEBUGF("backlight_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                break;

            case SYS_USB_DISCONNECTED:
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
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

int backlight_get_timeout(void)
{
    return backlight_timeout;
}

void backlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use 0 */
        index=0;
    backlight_timeout = index; /* index in the backlight_timeout_value table */
    backlight_on();
}

bool backlight_get_on_when_charging(void)
{
    return backlight_on_when_charging;
}

void backlight_set_on_when_charging(bool yesno)
{
    backlight_on_when_charging = yesno;
    backlight_on();
}

void backlight_tick(void)
{
    bool charger_is_inserted = charger_inserted();
    if( backlight_on_when_charging &&
        (charger_was_inserted != charger_is_inserted) )
    {
        backlight_on();
    }
    charger_was_inserted = charger_is_inserted;
    
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
    create_thread(backlight_thread, backlight_stack,
                  sizeof(backlight_stack), backlight_thread_name);

#ifdef IRIVER_H100
    GPIO1_ENABLE  |= 0x00020000;
    GPIO1_FUNCTION |= 0x00020000;
#endif
    backlight_on();
}

#else /* no backlight, empty dummy functions */

void backlight_init(void) {}
void backlight_on(void) {}
void backlight_off(void) {}
void backlight_tick(void) {}
int  backlight_get_timeout(void) {return 0;}
void backlight_set_timeout(int index) {(void)index;}
bool backlight_get_on_when_charging(void) {return 0;}
void backlight_set_on_when_charging(bool yesno) {(void)yesno;}

#endif /* #ifdef HAVE_BACKLIGHT */

