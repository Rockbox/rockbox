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
#include "sh7034.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "debug.h"

#ifndef SIMULATOR

#define USB_INSERTED    1
#define USB_EXTRACTED   2

static char usb_stack[0x100];
static struct event_queue usb_queue;
static bool usb_connected;

static void usb_thread(void)
{
    int num_acks_to_expect = -1;
    bool waiting_for_ack;
    struct event ev;

    waiting_for_ack = false;
    
    while(1)
    {
	queue_wait(&usb_queue, &ev);
	switch(ev.id)
	{
	case USB_INSERTED:
	    /* Tell all threads that they have to back off the ATA.
	       We subtract one for our own thread. */
	    num_acks_to_expect = queue_broadcast(SYS_USB_CONNECTED, NULL) - 1;
	    waiting_for_ack = true;
	    DEBUGF("USB inserted. Waiting for ack from %d threads...\n");
	    break;
	    
	case SYS_USB_CONNECTED_ACK:
	    if(waiting_for_ack)
	    {
		num_acks_to_expect--;
		if(num_acks_to_expect == 0)
		{
		    /* This is where we are supposed to be cool and keep
		       the Rockbox firmware running while the USB is enabled,
		       maybe even play some games and stuff. However, the
		       current firmware isn't quite ready for this yet.
		       Let's just chicken out and reboot. */
		    DEBUGF("All threads have acknowledged. Rebooting...\n");
		    system_reboot();
		}
		else
		{
		    DEBUGF("usb: got ack, %d to go...\n", num_acks_to_expect);
		}
	    }
	}
    }
}

static void usb_tick(void)
{
    int current_status;

#ifdef ARCHOS_RECORDER
    current_status = PCDR & 0x04;
#else
    current_status = PADR & 0x8000;
#endif

    if(current_status && !usb_connected)
    {
	usb_connected = true;
	queue_post(&usb_queue, USB_INSERTED, NULL);
    }
}

void usb_acknowledge(int id)
{
   queue_post(&usb_queue, id, NULL);
}

void usb_init(void)
{
    int rc;
    
    usb_connected = false;
    
    queue_init(&usb_queue);
    create_thread(usb_thread, usb_stack, sizeof(usb_stack));

    rc = tick_add_task(usb_tick);
}

#else

/* Dummy simulator functions */
void usb_acknowledge(int id)
{
    id = id;
}

void usb_init(void)
{
}

#endif
