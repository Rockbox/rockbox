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
#include "cpu.h"
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
#include "hwcompat.h"
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif

extern void dbg_ports(void); /* NASTY! defined in apps/ */

#ifdef HAVE_LCD_BITMAP
bool do_screendump_instead_of_usb = false;
void screen_dump(void);   /* Nasty again. Defined in apps/ too */
#endif

#define USB_REALLY_BRAVE

#if !defined(SIMULATOR) && !defined(USB_NONE)

/* Messages from usb_tick and thread states */
#define USB_INSERTED    1
#define USB_EXTRACTED   2
#ifdef HAVE_MMC
#define USB_REENABLE    3
#endif
#ifdef HAVE_USB_POWER
#define USB_POWERED     4

#if CONFIG_KEYPAD == RECORDER_PAD
#define USBPOWER_BUTTON BUTTON_F1
#define USBPOWER_BTN_IGNORE BUTTON_ON
#elif CONFIG_KEYPAD == ONDIO_PAD
#define USBPOWER_BUTTON BUTTON_MENU
#define USBPOWER_BTN_IGNORE BUTTON_OFF
#endif
#endif /* HAVE_USB_POWER */

/* The ADC tick reads one channel per tick, and we want to check 3 successive
   readings on the USB voltage channel. This doesn't apply to the Player, but
   debouncing the USB detection port won't hurt us either. */
#define NUM_POLL_READINGS (NUM_ADC_CHANNELS * 3)
static int countdown;

static int usb_state;

#ifdef HAVE_MMC
static int usb_mmc_countdown = 0;
#endif

/* FIXME: The extra 0x800 is consumed by fat_mount() when the fsinfo
   needs updating */
#ifndef BOOTLOADER
static long usb_stack[(DEFAULT_STACK_SIZE + 0x800)/sizeof(long)];
static const char usb_thread_name[] = "usb";
#endif
static struct event_queue usb_queue;
static bool last_usb_status;
static bool usb_monitor_enabled;

void usb_enable(bool on)
{
#ifdef USB_ENABLE_ONDIOSTYLE
    PACR2 &= ~0x04C0; /* use PA3, PA5 as GPIO */
    if(on)
    {
#ifdef HAVE_MMC
        mmc_enable_int_flash_clock(!mmc_detect());
#endif
        if (!(read_hw_mask() & MMC_CLOCK_POLARITY))
            and_b(~0x20, &PBDRH); /* old circuit needs SCK1 = low while on USB */
        or_b(0x20, &PADRL); /* enable USB */
        and_b(~0x08, &PADRL); /* assert card detect */
    }
    else
    {
        if (!(read_hw_mask() & MMC_CLOCK_POLARITY))
            or_b(0x20, &PBDRH); /* reset SCK1 = high for old circuit */
        and_b(~0x20, &PADRL); /* disable USB */
        or_b(0x08, &PADRL); /* deassert card detect */
    }
    or_b(0x28, &PAIORL); /* output for USB enable and card detect */
#elif defined(USB_GMINISTYLE)
    {
        int smsc_ver = smsc_version();
        if (on) {
            if (smsc_ver < 4) {
                P6 &= ~0x04;
                P10 &= ~0x20;
                
                smsc_delay();
                
                P6 |= 0x08;
                P10 |= 0x20;    

                smsc_delay();
            }
            P6 |= 0x10;
        } else {
            P6 &= ~0x10;
            if (smsc_ver < 4) {
                P6 &= ~0x04;
                P10 &= ~0x20;
            }
        }
    }
#elif defined(USB_IRIVERSTYLE)
    if(on)
    {
        /* Power on the Cypress chip */
#ifdef IRIVER_H100_SERIES
        or_l(0x01000040, &GPIO_OUT);
#else
        and_l(~0x00000008,&GPIO1_OUT);
#endif
        sleep(2);
    }
    else
    {
        /* Power off the Cypress chip */
#ifdef IRIVER_H100_SERIES
        and_l(~0x01000040, &GPIO_OUT);
#else
        or_l(0x00000008,&GPIO1_OUT);
#endif
    }
    
#elif defined(USB_IPODSTYLE)
    /* TODO: Implement USB_IPODSTYLE */
    (void) on;
#else
#ifdef HAVE_LCD_BITMAP
    if(read_hw_mask() & USB_ACTIVE_HIGH)
        on = !on;
#endif
    if(on)
    {
        and_b(~0x04, &PADRH); /* enable USB */
    }
    else
    {
        or_b(0x04, &PADRH);
    }
    or_b(0x04, &PAIORH);
#endif
}

#ifndef BOOTLOADER
static void usb_slave_mode(bool on)
{
    int rc;
    
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
            /* fixme: can we remove this? (already such in main.c) */
            char str[32];
            lcd_clear_display();
            snprintf(str, 31, "ATA error: %d", rc);
            lcd_puts(0, 0, str);
            lcd_puts(0, 1, "Press ON to debug");
            lcd_update();
            while(!(button_get(true) & BUTTON_REL)) {};
            dbg_ports();
            panicf("ata: %d",rc);
        }
    
        rc = disk_mount_all();
        if (rc <= 0) /* no partition */
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
#ifdef HAVE_LCD_BITMAP
                if(do_screendump_instead_of_usb)
                {
                    screen_dump();
                }
                else
#endif
#ifdef HAVE_USB_POWER
                if((button_status() & ~USBPOWER_BTN_IGNORE) == USBPOWER_BUTTON)
                {
                    usb_state = USB_POWERED;
                }
                else
#endif
                {
                    /* Tell all threads that they have to back off the ATA.
                       We subtract one for our own thread. */
                    num_acks_to_expect =
                        queue_broadcast(SYS_USB_CONNECTED, NULL) - 1;
                    waiting_for_ack = true;
                    DEBUGF("USB inserted. Waiting for ack from %d threads...\n",
                           num_acks_to_expect);
                }
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
                        cpu_idle_mode(true);
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
#ifdef HAVE_LCD_BITMAP
                if(do_screendump_instead_of_usb)
                    break;
#endif
#ifdef HAVE_USB_POWER
                if(usb_state == USB_POWERED)
                {
                    usb_state = USB_EXTRACTED;
                    break;
                }
#endif
                if(usb_state == USB_INSERTED)
                {
                    /* Only disable the USB mode if we really have enabled it
                       some threads might not have acknowledged the
                       insertion */
                    usb_slave_mode(false);
                    cpu_idle_mode(false);
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

#ifdef HAVE_MMC
            case SYS_MMC_INSERTED:
            case SYS_MMC_EXTRACTED:
                if(usb_state == USB_INSERTED)
                {
                    usb_enable(false);
                    usb_mmc_countdown = HZ/2; /* re-enable after 0.5 sec */
                }
                break;

            case USB_REENABLE:
                if(usb_state == USB_INSERTED)
                    usb_enable(true);  /* reenable only if still inserted */
                break;
#endif
        }
    }
}
#endif

bool usb_detect(void)
{
    bool current_status;

#ifdef USB_RECORDERSTYLE
    current_status = (adc_read(ADC_USB_POWER) > 500)?true:false;
#endif
#ifdef USB_FMRECORDERSTYLE
    current_status = (adc_read(ADC_USB_POWER) <= 512)?true:false;
#endif
#ifdef USB_PLAYERSTYLE
    current_status = (PADR & 0x8000)?false:true;
#endif
#ifdef USB_IRIVERSTYLE
    current_status = (GPIO1_READ & 0x80)?true:false;
#endif
#ifdef USB_GMINISTYLE
    current_status = (P5 & 0x10)?true:false;
#endif
#ifdef IRIVER_H300
    /* TODO: add proper code code for H300 USB style */
    current_status = (GPIO1_READ & 0x80)?true:false;
#endif
#ifdef USB_IPODSTYLE
    /* TODO: Implement USB_IPODSTYLE */
    current_status = false;
#endif
    return current_status;
}

#ifndef BOOTLOADER
static void usb_tick(void)
{
    bool current_status;

#ifdef USB_GMINISTYLE
    /* Keep usb chip in usb state (?) */
    if (P5 & 0x10) {
        if ((P10 & 0x20) == 0 || (P6 & 0x08) == 0) {
            if (smsc_version() < 4) {
                P6 |= 0x08;
                P10 |= 0x20;
            }
        }
    }
#endif
    
    if(usb_monitor_enabled)
    {
        current_status = usb_detect();
    
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
#ifdef HAVE_MMC
    if(usb_mmc_countdown > 0)
    {
        usb_mmc_countdown--;
        if (usb_mmc_countdown == 0)
            queue_post(&usb_queue, USB_REENABLE, NULL);
    }
#endif
}
#endif

void usb_acknowledge(long id)
{
    queue_post(&usb_queue, id, NULL);
}

void usb_init(void)
{
    usb_state = USB_EXTRACTED;
    usb_monitor_enabled = false;
    countdown = -1;

#ifdef USB_IRIVERSTYLE
    or_l(0x00000080, &GPIO1_FUNCTION); /* GPIO39 is the USB detect input */

#ifdef IRIVER_H300_SERIES
    /* ISD300 3.3V ON */
    or_l(8,&GPIO1_FUNCTION);
    or_l(8,&GPIO1_OUT);
    or_l(8,&GPIO1_ENABLE);

    /* Tristate the SCK/SDA to the ISD300 config EEPROM */
    and_l(~0x03000000, &GPIO_ENABLE);
    or_l(0x03000000, &GPIO_FUNCTION);
#else
    and_l(~0x01000040, &GPIO_OUT);     /* GPIO24 is the Cypress chip power */
    or_l(0x01000040, &GPIO_ENABLE);
    or_l(0x01000040, &GPIO_FUNCTION);
#endif

#endif

    usb_enable(false);

    /* We assume that the USB cable is extracted */
    last_usb_status = false;

#ifndef BOOTLOADER
    queue_init(&usb_queue);
    create_thread(usb_thread, usb_stack, sizeof(usb_stack), usb_thread_name);

    tick_add_task(usb_tick);
#endif
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

int usb_wait_for_disconnect_w_tmo(struct event_queue *q, int ticks)
{
    struct event ev;

    /* Don't return until we get SYS_USB_DISCONNECTED or SYS_TIMEOUT */
    while(1)
    {
        queue_wait_w_tmo(q, &ev, ticks);
        switch(ev.id)
        {
            case SYS_USB_DISCONNECTED:
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                return 0;
                break;
            case SYS_TIMEOUT:
                return 1;
                break;
        }
    }
}

void usb_start_monitoring(void)
{
    usb_monitor_enabled = true;
}

bool usb_inserted(void)
{
    return usb_state == USB_INSERTED;
}

#ifdef HAVE_USB_POWER
bool usb_powered(void)
{
    return usb_state == USB_POWERED;
}
#endif

#else

#ifdef USB_NONE
bool usb_inserted(void)
{
    return false;
}
#endif

/* Dummy simulator functions */
void usb_acknowledge(long id)
{
    id = id;
}

void usb_init(void)
{
}

void usb_start_monitoring(void)
{
}

bool usb_detect(void)
{
    return false;
}

#endif /* USB_NONE or SIMULATOR */
