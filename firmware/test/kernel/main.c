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
#include "thread.h"
#include "kernel.h"
#include "sh7034.h"
#include "debug.h"

void tick_start(unsigned int interval);

unsigned int s1[256];
unsigned int s2[256];

void t1(void);
void t2(void);

struct event_queue main_q;

int tick_add_task(void (*f)(void));

void testfunc(void)
{
    if(current_tick == 5000)
        debugf("Yippie!\n");
}

int main(void)
{
    char buf[40];
    char str[32];
    int i=0;
    struct queue_event *ev;

    /* Clear it all! */
    SSR1 &= ~(SCI_RDRF | SCI_ORER | SCI_PER | SCI_FER);

    /* This enables the serial Rx interrupt, to be able to exit into the
       debugger when you hit CTRL-C */
    SCR1 |= 0x40;
    SCR1 &= ~0x80;
    IPRE |= 0xf000; /* Set to highest priority */

    debugf("OK. Let's go\n");

    kernel_init();

    set_irq_level(0);

    tick_add_task(testfunc);
    
    debugf("sleeping 10s...\n");
    sleep(HZ*10);
    debugf("woke up\n");
    
    queue_init(&main_q);
    
    create_thread(t1, s1, 1024, 0);
    create_thread(t2, s2, 1024, 0);

    while(1)
    {
        ev = queue_wait(&main_q);
        debugf("Thread 0 got an event. ID: %d\n", ev->id);
    }
}

void t1(void)
{
    debugf("Thread 1 started\n");
    while(1)
    {
        sleep(HZ);
        debugf("Thread 1 posting an event\n");
        queue_post(&main_q, 1234, 0);
        queue_post(&main_q, 5678, 0);
    }
}

void t2(void)
{
    debugf("Thread 2 started\n");
    while(1)
    {
        sleep(HZ*3);
        debugf("Thread 2 awakened\n");
    }
}
