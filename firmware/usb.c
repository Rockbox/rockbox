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
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "panic.h"
#include "lcd.h"


#define USB_REALLY_BRAVE


#ifndef SIMULATOR

#define USB_INSERTED    1
#define USB_EXTRACTED   2

static char usb_stack[0x100];
static struct event_queue usb_queue;
static bool last_usb_status;
static bool usb_monitor_enabled;

static void usb_enable(bool on)
{
#ifdef ARCHOS_RECORDER
    if(!on) /* The pin is inverted on the Recorder */
#else
    if(on)
#endif
        PADR &= ~0x400; /* enable USB */
    else
        PADR |= 0x400;
    PAIOR |= 0x400;
}

static void usb_slave_mode(bool on)
{
    int rc;
    struct partinfo* pinfo;
    
    if(on)
    {
	DEBUGF("Entering USB slave mode\n");
	ata_enable(false);
	usb_enable(true);
    }
    else
    {
	DEBUGF("Leaving USB slave mode\n");

	/* Let the ISDx00 settle */
	sleep(HZ*1);

	usb_enable(false);

	rc = ata_init();
	if(rc)
	    panicf("ata: %d",rc);
	
	pinfo = disk_init();
	if (!pinfo)
	    panicf("disk: NULL");
	
	rc = fat_mount(pinfo[0].start);
	if(rc)
	    panicf("mount: %d",rc);
    }
}

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
	    DEBUGF("USB inserted. Waiting for ack from %d threads...\n",
		   num_acks_to_expect);
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
		    DEBUGF("All threads have acknowledged. Continuing...\n");
#ifdef USB_REALLY_BRAVE
		    usb_slave_mode(true);
#else
		    system_reboot();
#endif
		}
		else
		{
		    DEBUGF("usb: got ack, %d to go...\n", num_acks_to_expect);
		}
	    }
	    break;

	case USB_EXTRACTED:
	    /* First disable the USB mode */
	    usb_slave_mode(false);
	    
	    /* Tell all threads that we are back in business */
	    num_acks_to_expect =
	       queue_broadcast(SYS_USB_DISCONNECTED, NULL) - 1;
	    waiting_for_ack = true;
	    DEBUGF("USB extracted. Waiting for ack from %d threads...\n",
		   num_acks_to_expect);
	    break;
	    
	case SYS_USB_DISCONNECTED_ACK:
	    if(waiting_for_ack)
	    {
		num_acks_to_expect--;
		if(num_acks_to_expect == 0)
		{
		    DEBUGF("All threads have acknowledged. We're in business.\n");
		}
		else
		{
		    DEBUGF("usb: got ack, %d to go...\n", num_acks_to_expect);
		}
	    }
	    break;
	}
    }
}

static void usb_tick(void)
{
    bool current_status;

    if(usb_monitor_enabled)
    {
#ifdef ARCHOS_RECORDER
	/* If AN2 reads more than about 500, the USB is inserted */
	current_status = (adc_read(2) > 500);
#else
	current_status = (PADR & 0x8000)?false:true;
#endif
	
	/* Only report when the status has changed */
	if(current_status != last_usb_status)
	{
	    last_usb_status = current_status;
	    if(current_status)
		queue_post(&usb_queue, USB_INSERTED, NULL);
	    else
		queue_post(&usb_queue, USB_EXTRACTED, NULL);
	}
    }
}

void usb_acknowledge(int id)
{
   queue_post(&usb_queue, id, NULL);
}

void usb_init(void)
{
    usb_monitor_enabled = false;
    
    usb_enable(false);

    /* We assume that the USB cable is extracted */
    last_usb_status = false;
    
    queue_init(&usb_queue);
    create_thread(usb_thread, usb_stack, sizeof(usb_stack));

    tick_add_task(usb_tick);
}

void usb_wait_for_disconnect(struct event_queue *q)
{
    struct event ev;

    /* Don't return until we get SYS_USB_DISCONNECTED */
    while(1)
    {
	queue_wait(q, &ev);
	if(ev.id == SYS_USB_DISCONNECTED)
	{
	    usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
	    return;
	}
    }
}

void usb_start_monitoring(void)
{
    usb_monitor_enabled = true;
}

void usb_display_info(void)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_puts(4, 3, "[USB Mode]");
    lcd_update();
#else
    lcd_puts(0, 0, "[USB Mode]");
#endif
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

void usb_start_monitoring(void)
{
}

void usb_display_info(void)
{
}

#endif
