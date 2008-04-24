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
 * iPod driver based on code from the ipodlinux project - http://ipodlinux.org
 * Adapted for Rockbox in January 2006
 * Original file: podzilla/usb.c
 * Copyright (C) 2005 Adam Johnston
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
#include "usb.h"
#include "button.h"
#include "sprintf.h"
#include "string.h"
#include "usb-target.h"
#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif
#ifdef IRIVER_H300_SERIES
#include "pcf50606.h"        /* for pcf50606_usb_charging_... */
#endif
#include "logf.h"

/* Conditions under which we want the entire driver */
#if !defined(BOOTLOADER) || \
     (defined(TOSHIBA_GIGABEAT_S) && defined(USE_ROCKBOX_USB) && defined(USB_STORAGE)) || \
     (defined(CREATIVE_ZVM) && defined(HAVE_USBSTACK))
#define USB_FULL_INIT
#endif

extern void dbg_ports(void); /* NASTY! defined in apps/ */

#ifdef HAVE_LCD_BITMAP
bool do_screendump_instead_of_usb = false;
#if defined(USB_FULL_INIT) && defined(BOOTLOADER)
static void screen_dump(void) {}
#else
void screen_dump(void);   /* Nasty again. Defined in apps/ too */
#endif
#endif

#if !defined(SIMULATOR) && !defined(USB_NONE)

#define NUM_POLL_READINGS (HZ/5)
static int countdown;

static int usb_state;

#if defined(HAVE_MMC) && defined(USB_FULL_INIT)
static int usb_mmc_countdown = 0;
#endif

/* FIXME: The extra 0x800 is consumed by fat_mount() when the fsinfo
   needs updating */
#ifdef USB_FULL_INIT
static long usb_stack[(DEFAULT_STACK_SIZE + 0x800)/sizeof(long)];
static const char usb_thread_name[] = "usb";
static struct thread_entry *usb_thread_entry;
#endif
static struct event_queue usb_queue;
static int last_usb_status;
static bool usb_monitor_enabled;
#ifdef HAVE_USBSTACK
static bool exclusive_ata_access;
#endif


#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
static int firewire_countdown;
static bool last_firewire_status;
#endif

#ifdef USB_FULL_INIT
#ifndef HAVE_USBSTACK
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
#endif

static void try_reboot(void)
{
#ifndef HAVE_FLASH_STORAGE
    ata_sleepnow(); /* Immediately spindown the disk. */
    sleep(HZ*2);
#endif

#ifdef IPOD_ARCH  /* The following code is based on ipodlinux */
#if CONFIG_CPU == PP5020
    memcpy((void *)0x40017f00, "diskmode\0\0hotstuff\0\0\1", 21);
#elif CONFIG_CPU == PP5022
    memcpy((void *)0x4001ff00, "diskmode\0\0hotstuff\0\0\1", 21);
#endif /* CONFIG_CPU */
#endif /* IPOD_ARCH */

    system_reboot(); /* Reboot */
}

static void usb_thread(void)
{
    int num_acks_to_expect = -1;
    bool waiting_for_ack;
    struct queue_event ev;

    waiting_for_ack = false;

    while(1)
    {
        queue_wait(&usb_queue, &ev);
        switch(ev.id)
        {
#ifdef HAVE_USBSTACK
            case USB_TRANSFER_COMPLETION:
                usb_core_handle_transfer_completion((struct usb_transfer_completion_event_data*)ev.data);
                break;
#endif
#ifdef HAVE_USB_POWER
            case USB_POWERED:
                usb_state = USB_POWERED;
                break;
#endif
            case USB_INSERTED:
#ifdef HAVE_LCD_BITMAP
                if(do_screendump_instead_of_usb)
                {
                    screen_dump();
                }
                else
#endif
#ifdef HAVE_USB_POWER
#if defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
                if((button_status() & ~USBPOWER_BTN_IGNORE) != USBPOWER_BUTTON)
#else
                if((button_status() & ~USBPOWER_BTN_IGNORE) == USBPOWER_BUTTON)
#endif
                {
                    usb_state = USB_POWERED;
#ifdef HAVE_USBSTACK
                    usb_core_enable_driver(USB_DRIVER_MASS_STORAGE,false);
                    usb_core_enable_driver(USB_DRIVER_CHARGING_ONLY,true);
                    usb_enable(true);
#endif
                }
                else
#endif
                {
#ifdef HAVE_USBSTACK
                    /* Set the state to USB_POWERED for now. if a real
                       connection is detected it will switch to USB_INSERTED */
                    usb_state = USB_POWERED;
                    usb_core_enable_driver(USB_DRIVER_MASS_STORAGE,true);
                    usb_core_enable_driver(USB_DRIVER_CHARGING_ONLY,false);
                    usb_enable(true);
#else
                    /* Tell all threads that they have to back off the ATA.
                       We subtract one for our own thread. */
                    num_acks_to_expect =
                        queue_broadcast(SYS_USB_CONNECTED, 0) - 1;
                    waiting_for_ack = true;
                    DEBUGF("USB inserted. Waiting for ack from %d threads...\n",
                           num_acks_to_expect);
#endif
                }
                break;
#ifdef HAVE_USBSTACK
            case USB_REQUEST_DISK:
                if(!waiting_for_ack)
                {
                    /* Tell all threads that they have to back off the ATA.
                       We subtract one for our own thread. */
                    num_acks_to_expect =
                        queue_broadcast(SYS_USB_CONNECTED, 0) - 1;
                    waiting_for_ack = true;
                    DEBUGF("USB inserted. Waiting for ack from %d threads...\n",
                           num_acks_to_expect);
                }
                break;
            case USB_RELEASE_DISK:
                if(!waiting_for_ack)
                {
                    /* Tell all threads that they have to back off the ATA.
                       We subtract one for our own thread. */
                    num_acks_to_expect =
                        queue_broadcast(SYS_USB_DISCONNECTED, 0) - 1;
                    waiting_for_ack = true;
                    DEBUGF("USB inserted. Waiting for ack from %d threads...\n",
                           num_acks_to_expect);
                }
                break;
#endif
            case SYS_USB_CONNECTED_ACK:
                if(waiting_for_ack)
                {
                    num_acks_to_expect--;
                    if(num_acks_to_expect == 0)
                    {
                        DEBUGF("All threads have acknowledged the connect.\n");
#ifdef HAVE_USBSTACK
#ifndef USE_ROCKBOX_USB
                        /* until we have native mass-storage mode, we want to reboot on
                           usb host connect */
                        usb_enable(true);
                        try_reboot();
#endif  /* USE_ROCKBOX_USB */
#ifdef HAVE_PRIORITY_SCHEDULING
                        thread_set_priority(usb_thread_entry,PRIORITY_REALTIME);
#endif
                        exclusive_ata_access = true;

#else
                        usb_slave_mode(true);
                        cpu_idle_mode(true);
#endif
                        usb_state = USB_INSERTED;
                        waiting_for_ack = false;
                    }
                    else
                    {
                        DEBUGF("usb: got ack, %d to go...\n",
                               num_acks_to_expect);
                    }
                }
                break;

            case USB_EXTRACTED:
#ifdef HAVE_USBSTACK
                usb_enable(false);
#ifdef HAVE_PRIORITY_SCHEDULING
                thread_set_priority(usb_thread_entry,PRIORITY_SYSTEM);
#endif
#endif
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
#ifndef HAVE_USBSTACK
                if(usb_state == USB_INSERTED)
                {
                    /* Only disable the USB mode if we really have enabled it
                       some threads might not have acknowledged the
                       insertion */
                    usb_slave_mode(false);
                    cpu_idle_mode(false);
                }
#endif

                usb_state = USB_EXTRACTED;
#ifdef HAVE_USBSTACK
                if(exclusive_ata_access)
                {
                    exclusive_ata_access = false;
#endif
                    /* Tell all threads that we are back in business */
                    num_acks_to_expect =
                        queue_broadcast(SYS_USB_DISCONNECTED, 0) - 1;
                    waiting_for_ack = true;
                    DEBUGF("USB extracted. Waiting for ack from %d threads...\n",
                           num_acks_to_expect);
#ifdef HAVE_USBSTACK
                }
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
                        waiting_for_ack = false;
                    }
                    else
                    {
                        DEBUGF("usb: got ack, %d to go...\n",
                               num_acks_to_expect);
                    }
                }
                break;

#ifdef HAVE_HOTSWAP
            case SYS_HOTSWAP_INSERTED:
            case SYS_HOTSWAP_EXTRACTED:
#ifdef HAVE_USBSTACK
                usb_core_hotswap_event(1,ev.id == SYS_HOTSWAP_INSERTED);
#else
                if(usb_state == USB_INSERTED)
                {
                    usb_enable(false);
                    usb_mmc_countdown = HZ/2; /* re-enable after 0.5 sec */
                }
                break;
#endif

            case USB_REENABLE:
                if(usb_state == USB_INSERTED)
                    usb_enable(true);  /* reenable only if still inserted */
                break;
#endif
            case USB_REQUEST_REBOOT:
#ifdef HAVE_USB_POWER
                if((button_status() & ~USBPOWER_BTN_IGNORE) != USBPOWER_BUTTON)
#endif
                    try_reboot();
                break;
        }
    }
}
#endif

#ifdef HAVE_USBSTACK
void usb_signal_transfer_completion(struct usb_transfer_completion_event_data* event_data)
{
    queue_post(&usb_queue, USB_TRANSFER_COMPLETION, (intptr_t)event_data);
}
#endif

#ifdef USB_FULL_INIT
static void usb_tick(void)
{
    int current_status;

    if(usb_monitor_enabled)
    {
#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
        int current_firewire_status = firewire_detect();
        if(current_firewire_status != last_firewire_status)
        {
            last_firewire_status = current_firewire_status;
            firewire_countdown = NUM_POLL_READINGS;
        }
        else
        {
            /* Count down until it gets negative */
            if(firewire_countdown >= 0)
                firewire_countdown--;

            /* Report to the thread if we have had 3 identical status
               readings in a row */
            if(firewire_countdown == 0)
            {
                queue_post(&usb_queue, USB_REQUEST_REBOOT, 0);
            }
        }
#endif

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
                queue_post(&usb_queue, current_status, 0);
            }
        }
    }
#ifdef HAVE_MMC
    if(usb_mmc_countdown > 0)
    {
        usb_mmc_countdown--;
        if (usb_mmc_countdown == 0)
            queue_post(&usb_queue, USB_REENABLE, 0);
    }
#endif
}
#endif

void usb_acknowledge(long id)
{
    queue_post(&usb_queue, id, 0);
}

void usb_init(void)
{
    usb_state = USB_EXTRACTED;
#ifdef HAVE_USBSTACK
    exclusive_ata_access = false;
#endif
    usb_monitor_enabled = false;
    countdown = -1;

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
    firewire_countdown = -1;
    last_firewire_status = false;
#endif

    usb_init_device();
#ifdef USB_FULL_INIT
    usb_enable(false);
#endif

    /* We assume that the USB cable is extracted */
    last_usb_status = USB_EXTRACTED;

#ifdef USB_FULL_INIT
    queue_init(&usb_queue, true);
    
    usb_thread_entry = create_thread(usb_thread, usb_stack,
                       sizeof(usb_stack), 0, usb_thread_name
                       IF_PRIO(, PRIORITY_SYSTEM) IF_COP(, CPU));

    tick_add_task(usb_tick);
#endif

}

void usb_wait_for_disconnect(struct event_queue *q)
{
    struct queue_event ev;

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
    struct queue_event ev;

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

#ifdef TOSHIBA_GIGABEAT_S
void usb_stop_monitoring(void)
{
    tick_remove_task(usb_tick);
    usb_monitor_enabled = false;
}
#endif

bool usb_inserted(void)
{
#ifdef HAVE_USB_POWER
    return usb_state == USB_INSERTED || usb_state == USB_POWERED;
#else
    return usb_state == USB_INSERTED;
#endif
}

#ifdef HAVE_USBSTACK
void usb_request_exclusive_ata(void)
{
    /* This is not really a clean place to start boosting the cpu. but it's 
     * currently the best one. We want to get rid of having to boost the cpu
     * for usb anyway */
    trigger_cpu_boost();
    if(!exclusive_ata_access) {
        queue_post(&usb_queue, USB_REQUEST_DISK, 0);
    }
}

void usb_release_exclusive_ata(void)
{
    cancel_cpu_boost();
    if(exclusive_ata_access) {
        queue_post(&usb_queue, USB_RELEASE_DISK, 0);
        exclusive_ata_access = false;
    }
}

bool usb_exclusive_ata(void)
{
    return exclusive_ata_access;
}
#endif

#ifdef HAVE_USB_POWER
bool usb_powered(void)
{
    return usb_state == USB_POWERED;
}

#if CONFIG_CHARGING
bool usb_charging_enable(bool on)
{
    bool rc = false;
#ifdef IRIVER_H300_SERIES
    int irqlevel;
    logf("usb_charging_enable(%s)\n", on ? "on" : "off" );
    irqlevel = disable_irq_save();
    pcf50606_set_usb_charging(on);
    rc = on;
    restore_irq(irqlevel);
#else
    /* TODO: implement it for other targets... */
    (void)on;
#endif
    return rc;
}

bool usb_charging_enabled(void)
{
    bool rc = false;
#ifdef IRIVER_H300_SERIES
    /* TODO: read the state of the GPOOD2 register...
     * (this also means to set the irq level here) */
    rc = pcf50606_usb_charging_enabled();
#else
    /* TODO: implement it for other targets... */
#endif

    logf("usb charging %s", rc ? "enabled" : "disabled" );
    return rc;
}
#endif
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

int usb_detect(void)
{
    return USB_EXTRACTED;
}

void usb_wait_for_disconnect(struct event_queue *q)
{
   (void)q;
}

#endif /* USB_NONE or SIMULATOR */
