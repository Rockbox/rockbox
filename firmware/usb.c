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
#include "adc.h"
#include "usb.h"
#include "button.h"
#include "sprintf.h"

extern void dbg_ports(void); /* NASTY! defined in apps/ */

#define USB_REALLY_BRAVE


#ifndef SIMULATOR

/* Messages from usb_tick */
#define USB_INSERTED    1
#define USB_EXTRACTED   2

/* Thread states */
#define EXTRACTING      1
#define EXTRACTED       2
#define INSERTED        3
#define INSERTING       4

/* The ADC tick reads one channel per tick, and we want to check 3 successive
   readings on the USB voltage channel. This doesn't apply to the Player, but
   debouncing the USB detection port won't hurt us either. */
#define NUM_POLL_READINGS (NUM_ADC_CHANNELS * 3)
static int countdown;

static int usb_state;

static char usb_stack[DEFAULT_STACK_SIZE];
static char usb_thread_name[] = "usb";
static struct event_queue usb_queue;
static bool last_usb_status;
static bool usb_monitor_enabled;
static bool usb_inverted;

static void usb_enable(bool on)
{
    if(usb_inverted)
        on = !on;
    
    if(on)
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
        ata_soft_reset();
        ata_init();
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
        {
            char str[32];
            lcd_clear_display();
            snprintf(str, 31, "ATA error: %d", rc);
            lcd_puts(0, 0, str);
            lcd_puts(0, 1, "Press ON to debug");
            lcd_update();
            while(button_get(true) != BUTTON_ON) {};
            dbg_ports();
            panicf("ata: %d",rc);
        }
    
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
                num_acks_to_expect =
                    queue_broadcast(SYS_USB_CONNECTED, NULL) - 1;
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
                        DEBUGF("All threads have acknowledged the connect.\n");
#ifdef USB_REALLY_BRAVE
                        usb_slave_mode(true);
                        usb_state = USB_INSERTED;
                        usb_display_info();
#else
                        system_reboot();
#endif
                    }
                    else
                    {
                        DEBUGF("usb: got ack, %d to go...\n",
                               num_acks_to_expect);
                    }
                }
                break;

            case USB_EXTRACTED:
                if(usb_state == USB_INSERTED)
                {
                    /* Only disable the USB mode if we really have enabled it
                       some threads might not have acknowledged the
                       insertion */
                    usb_slave_mode(false);
                }

                usb_state = USB_EXTRACTED;
                
                /* Tell all threads that we are back in business */
                num_acks_to_expect =
                    queue_broadcast(SYS_USB_DISCONNECTED, NULL) - 1;
                waiting_for_ack = true;
                DEBUGF("USB extracted. Waiting for ack from %d threads...\n",
                       num_acks_to_expect);
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_USB, false);
#endif
                break;
        
            case SYS_USB_DISCONNECTED_ACK:
                if(waiting_for_ack)
                {
                    num_acks_to_expect--;
                    if(num_acks_to_expect == 0)
                    {
                        DEBUGF("All threads have acknowledged. "
                               "We're in business.\n");
                    }
                    else
                    {
                        DEBUGF("usb: got ack, %d to go...\n",
                               num_acks_to_expect);
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
        current_status = (adc_read(ADC_USB_POWER) > 500)?true:false;
#else
        current_status = (PADR & 0x8000)?false:true;
#endif
    
        /* Only report when the status has changed */
        if(current_status != last_usb_status)
        {
            last_usb_status = current_status;
            countdown = NUM_POLL_READINGS;
        }
        else
        {
            /* Count down until it gets negative */
            if(countdown >= 0)
                countdown--;

            /* Report to the thread if we have had 3 identical status
               readings in a row */
            if(countdown == 0)
            {
                if(current_status)
                    queue_post(&usb_queue, USB_INSERTED, NULL);
                else
                    queue_post(&usb_queue, USB_EXTRACTED, NULL);
            }
        }
    }
}

void usb_acknowledge(int id)
{
    queue_post(&usb_queue, id, NULL);
}

void usb_init(void)
{
    usb_state = USB_EXTRACTED;
    usb_monitor_enabled = false;
    countdown = -1;

    /* This is lame. Really lame. We determine the polarity of the USB
       enable pin by checking how it is set by the Archos firmware. */
    if(PADR & 0x400)
        usb_inverted = false;
    else
        usb_inverted = true;
        
    usb_enable(false);

    /* We assume that the USB cable is extracted */
    last_usb_status = false;
    
    queue_init(&usb_queue);
    create_thread(usb_thread, usb_stack, sizeof(usb_stack), usb_thread_name);

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

#endif

#ifdef HAVE_LCD_BITMAP
#define BMPHEIGHT_usb_logo 32
#define BMPWIDTH_usb_logo 100
static unsigned char usb_logo[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08,
  0x04, 0x04, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0x81, 0x81, 0x81,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0xf1, 0x4f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
  0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0xc0, 
  0x00, 0x00, 0xe0, 0x1c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x06, 0x81, 0xc0, 0xe0, 0xe0, 0xe0, 0xe0,
  0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x1c,
  0x0c, 0x0e, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xc0, 0xc0, 0x80, 0x80, 0x00, 0x00,
  0x00, 0x00, 0xe0, 0x1f, 0x00, 0xf8, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
  0x02, 0x02, 0x02, 0x82, 0x7e, 0x00, 0xc0, 0x3e, 0x01, 
  0x70, 0x4f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
  0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
  0x40, 0x40, 0x40, 0x40, 0x40, 0x80, 0x00, 0x07, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x0f, 0x07, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07, 0x0f,
  0x1f, 0x3f, 0x7b, 0xf3, 0xe3, 0xc3, 0x83, 0x83, 0x83, 0x83, 0xe3, 0xe3, 0xe3,
  0xe3, 0xe3, 0xe3, 0x03, 0x03, 0x03, 0x3f, 0x1f, 0x1f, 0x0f, 0x0f, 0x07, 0x02,
  0xc0, 0x3e, 0x01, 0xe0, 0x9f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0xf0, 0x0f, 0x80, 0x78, 0x07, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0c, 0x10, 0x20, 0x40, 0x40, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x87, 0x87, 0x87,
  0x87, 0x87, 0x87, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf0,
  0x0f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x07, 0x00, 0x00, 0x00, 0x00, 
};

#endif
void usb_display_info(void)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    /* lcd_bitmap() only supports 16 pixels height! */
    lcd_bitmap(usb_logo, 6, 16,
               BMPWIDTH_usb_logo, 8, false);
    lcd_bitmap(usb_logo+BMPWIDTH_usb_logo, 6, 24,
               BMPWIDTH_usb_logo, 8, false);
    lcd_bitmap(usb_logo+BMPWIDTH_usb_logo*2, 6, 32,
               BMPWIDTH_usb_logo, 8, false);
    lcd_bitmap(usb_logo+BMPWIDTH_usb_logo*3, 6, 40,
               BMPWIDTH_usb_logo, 8, false);
    lcd_update();
#else
    lcd_puts(0, 0, "[USB Mode]");
    lcd_icon(ICON_PARAM, false);
    lcd_icon(ICON_AUDIO, false);
    lcd_icon(ICON_USB, true);
#endif
}
