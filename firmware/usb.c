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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "panic.h"
#include "lcd.h"
#include "usb-target.h"
#include "usb.h"
#include "button.h"
#include "sprintf.h"
#include "string.h"
#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif
#include "logf.h"  
#include "screendump.h"

/* Conditions under which we want the entire driver */
#if !defined(BOOTLOADER) || (CONFIG_CPU == SH7034) || \
     (defined(TOSHIBA_GIGABEAT_S) && defined(USE_ROCKBOX_USB) && defined(USB_STORAGE)) || \
     (defined(HAVE_USBSTACK) && (defined(CREATIVE_ZVx) || \
     defined(CPU_TCC77X) || defined(CPU_TCC780X))) || \
     (CONFIG_USBOTG == USBOTG_JZ4740)
#define USB_FULL_INIT
#endif

#ifdef HAVE_LCD_BITMAP
bool do_screendump_instead_of_usb = false;
#endif

#if !defined(SIMULATOR) && !defined(USB_NONE)

static int usb_state;

#if (CONFIG_STORAGE & STORAGE_MMC) && defined(USB_FULL_INIT)
static int usb_mmc_countdown = 0;
#endif

/* FIXME: The extra 0x800 is consumed by fat_mount() when the fsinfo
   needs updating */
#ifdef USB_FULL_INIT
static long usb_stack[(DEFAULT_STACK_SIZE + 0x800)/sizeof(long)];
static const char usb_thread_name[] = "usb";
static unsigned int usb_thread_entry = 0;
#ifndef USB_STATUS_BY_EVENT
#define NUM_POLL_READINGS (HZ/5)
static int countdown;
#endif /* USB_STATUS_BY_EVENT */
#endif /* USB_FULL_INIT */
static struct event_queue usb_queue;
static bool usb_monitor_enabled;
static int last_usb_status;
#ifdef HAVE_USBSTACK
static bool exclusive_storage_access;
#endif

#ifdef USB_FIREWIRE_HANDLING
static int firewire_countdown;
static bool last_firewire_status;
#endif

#ifdef USB_FULL_INIT

#if defined(USB_FIREWIRE_HANDLING) \
    || (defined(HAVE_USBSTACK) && !defined(USE_ROCKBOX_USB))
static void try_reboot(void)
{
#ifdef HAVE_DISK_STORAGE
    storage_sleepnow(); /* Immediately spindown the disk. */
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
#endif /* USB_FIRWIRE_HANDLING || (HAVE_USBSTACK && !USE_ROCKBOX_USB) */

#ifdef HAVE_USBSTACK
/* inline since branch is chosen at compile time */
static inline void usb_slave_mode(bool on)
{
#ifdef USE_ROCKBOX_USB
    int rc;

    if (on)
    {
        trigger_cpu_boost();
#ifdef HAVE_PRIORITY_SCHEDULING
        thread_set_priority(THREAD_ID_CURRENT, PRIORITY_REALTIME);
#endif
        usb_attach();
    }
    else /* usb_state == USB_INSERTED (only!) */
    {
#ifndef USB_DETECT_BY_DRV
        usb_enable(false);
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
        thread_set_priority(THREAD_ID_CURRENT, PRIORITY_SYSTEM);
#endif
        /* Entered exclusive mode */
        rc = disk_mount_all();
        if (rc <= 0) /* no partition */
            panicf("mount: %d",rc);

        cancel_cpu_boost();
    }
#else /* !USB_ROCKBOX_USB */
    if (on)
    {
        /* until we have native mass-storage mode, we want to reboot on
           usb host connect */
        try_reboot();
    }
#endif /* USE_ROCKBOX_USB */
}

void usb_signal_transfer_completion(
    struct usb_transfer_completion_event_data* event_data)
{
    queue_post(&usb_queue, USB_TRANSFER_COMPLETION, (intptr_t)event_data);
}
#else  /* !HAVE_USBSTACK */
/* inline since branch is chosen at compile time */
static inline void usb_slave_mode(bool on)
{
    int rc;
    
    if(on)
    {
        DEBUGF("Entering USB slave mode\n");
        storage_soft_reset();
        storage_init();
        storage_enable(false);
        usb_enable(true);
        cpu_idle_mode(true);
    }
    else
    {
        DEBUGF("Leaving USB slave mode\n");

        cpu_idle_mode(false);

        /* Let the ISDx00 settle */
        sleep(HZ*1);
        
        usb_enable(false);

        rc = storage_init();
        if(rc)
            panicf("storage: %d",rc);
    
        rc = disk_mount_all();
        if (rc <= 0) /* no partition */
            panicf("mount: %d",rc);
    }
}
#endif /* HAVE_USBSTACK */

#ifdef HAVE_USB_POWER
static inline bool usb_power_button(void)
{
#if (defined(IRIVER_H10) || defined (IRIVER_H10_5GB)) && !defined(USE_ROCKBOX_USB)
    return (button_status() & ~USBPOWER_BTN_IGNORE) != USBPOWER_BUTTON;
#else
    return (button_status() & ~USBPOWER_BTN_IGNORE) == USBPOWER_BUTTON;
#endif
}

#ifdef USB_FIREWIRE_HANDLING
static inline bool usb_reboot_button(void)
{
    return (button_status() & ~USBPOWER_BTN_IGNORE) != USBPOWER_BUTTON;
}
#endif
#endif /* HAVE_USB_POWER */

static void usb_thread(void)
{
    int num_acks_to_expect = 0;
    struct queue_event ev;

    while(1)
    {
        queue_wait(&usb_queue, &ev);
        switch(ev.id)
        {
#ifdef USB_DRIVER_CLOSE
            case USB_QUIT:
                return;
#endif
#ifdef HAVE_USBSTACK
            case USB_TRANSFER_COMPLETION:
                usb_core_handle_transfer_completion(
                    (struct usb_transfer_completion_event_data*)ev.data);
                break;
#endif
#ifdef USB_DETECT_BY_DRV
            /* In this case, these events the handle cable insertion USB
             * driver determines INSERTED/EXTRACTED state. */
            case USB_POWERED:
                /* Set the state to USB_POWERED for now and enable the driver
                 * to detect a bus reset only. If a bus reset is detected,
                 * USB_INSERTED will be received. */
                usb_state = USB_POWERED;
                usb_enable(true);
                break;

            case USB_UNPOWERED:
                usb_enable(false);
                /* This part shouldn't be obligatory for anything that can
                 * reliably detect removal of the data lines. USB_EXTRACTED
                 * could be posted on that event while bus power remains
                 * available. */
                queue_post(&usb_queue, USB_EXTRACTED, 0);
                break;
#endif /* USB_DETECT_BY_DRV */
            case USB_INSERTED:
#ifdef HAVE_LCD_BITMAP
                if(do_screendump_instead_of_usb)
                {
                    usb_state = USB_SCREENDUMP;
                    screen_dump();
#ifdef HAVE_REMOTE_LCD
                    remote_screen_dump();
#endif
                    break;
                }
#endif
#ifdef HAVE_USB_POWER
                if(usb_power_button())
                {
                    /* Only charging is desired */
                    usb_state = USB_POWERED;
#ifdef HAVE_USBSTACK
#ifdef USB_STORAGE
                    usb_core_enable_driver(USB_DRIVER_MASS_STORAGE, false);
#endif                
#ifdef USB_CHARGING_ONLY
                    usb_core_enable_driver(USB_DRIVER_CHARGING_ONLY, true);
#endif                
                    usb_attach();
#endif
                    break;
                }
#endif /* HAVE_USB_POWER */
#ifdef HAVE_USBSTACK
#ifdef HAVE_USB_POWER
                /* Set the state to USB_POWERED for now. If permission to connect
                 * by threads and storage is granted it will be changed to
                 * USB_CONNECTED. */
                usb_state = USB_POWERED;
#endif
#ifdef USB_STORAGE
                usb_core_enable_driver(USB_DRIVER_MASS_STORAGE, true);
#endif                
#ifdef USB_CHARGING_ONLY
                usb_core_enable_driver(USB_DRIVER_CHARGING_ONLY, false);
#endif                

                /* Check any drivers enabled at this point for exclusive storage
                 * access requirements. */
                exclusive_storage_access = usb_core_any_exclusive_storage();

                if(!exclusive_storage_access)
                {
                    usb_attach();
                    break;
                }
#endif /* HAVE_USBSTACK */
                /* Tell all threads that they have to back off the storage.
                   We subtract one for our own thread. */
                num_acks_to_expect = queue_broadcast(SYS_USB_CONNECTED, 0) - 1;
                DEBUGF("USB inserted. Waiting for ack from %d threads...\n",
                       num_acks_for_connect);
                break;

            case SYS_USB_CONNECTED_ACK:
                if(num_acks_to_expect > 0 && --num_acks_to_expect == 0)
                {
                    DEBUGF("All threads have acknowledged the connect.\n");
                    usb_slave_mode(true);
                    usb_state = USB_INSERTED;
                }
                else
                {
                    DEBUGF("usb: got ack, %d to go...\n",
                           num_acks_to_expect);
                }
                break;

            case USB_EXTRACTED:
#ifdef HAVE_LCD_BITMAP
                if(usb_state == USB_SCREENDUMP)
                {
                    usb_state = USB_EXTRACTED;
                    break; /* Connected for screendump only */
                }
#endif
#ifndef HAVE_USBSTACK /* Stack must undo this if POWERED state was transitional */
#ifdef HAVE_USB_POWER
                if(usb_state == USB_POWERED)
                {
                    usb_state = USB_EXTRACTED;
                    break;
                }
#endif
#endif /* HAVE_USBSTACK */
                if(usb_state == USB_INSERTED)
                {
                    /* Only disable the USB mode if we really have enabled it
                       some threads might not have acknowledged the
                       insertion */
                    usb_slave_mode(false);
                }

                usb_state = USB_EXTRACTED;
#ifdef HAVE_USBSTACK
                if(!exclusive_storage_access)
                {
#ifndef USB_DETECT_BY_DRV /* Disabled handling USB_UNPOWERED */
                    usb_enable(false);
#endif
                    break;
                }

#endif /* HAVE_USBSTACK */
                num_acks_to_expect = usb_release_exclusive_storage();

                break;

            case SYS_USB_DISCONNECTED_ACK:
                if(num_acks_to_expect > 0 && --num_acks_to_expect == 0)
                {
                    DEBUGF("All threads have acknowledged. "
                           "We're in business.\n");
                }
                else
                {
                    DEBUGF("usb: got ack, %d to go...\n",
                           num_acks_to_expect);
                }
                break;

#ifdef HAVE_HOTSWAP
            case SYS_HOTSWAP_INSERTED:
            case SYS_HOTSWAP_EXTRACTED:
#ifdef HAVE_USBSTACK
                usb_core_hotswap_event(1,ev.id == SYS_HOTSWAP_INSERTED);
#else  /* !HAVE_USBSTACK */
                if(usb_state == USB_INSERTED)
                {
                    usb_enable(false);
#if (CONFIG_STORAGE & STORAGE_MMC)
                    usb_mmc_countdown = HZ/2; /* re-enable after 0.5 sec */
#endif /* STORAGE_MMC */
                }
#endif /* HAVE_USBSTACK */
                break;

#if (CONFIG_STORAGE & STORAGE_MMC)
            case USB_REENABLE:
                if(usb_state == USB_INSERTED)
                    usb_enable(true);  /* reenable only if still inserted */
                break;
#endif /* STORAGE_MMC */
#endif /* HAVE_HOTSWAP */

#ifdef USB_FIREWIRE_HANDLING
            case USB_REQUEST_REBOOT:
#ifdef HAVE_USB_POWER
                if (usb_reboot_button())
#endif
                    try_reboot();
                break;
#endif /* USB_FIREWIRE_HANDLING */
        }
    }
}

#ifdef USB_STATUS_BY_EVENT
void usb_status_event(int current_status)
{
    /* Caller isn't expected to filter for changes in status.
     * current_status:
     *   USB_DETECT_BY_DRV: USB_POWERED, USB_UNPOWERED, USB_INSERTED (driver)
     *                else: USB_INSERTED, USB_EXTRACTED
     */
    if(usb_monitor_enabled)
    {
        int oldstatus = disable_irq_save(); /* Dual-use function */

        if(last_usb_status != current_status)
        {
            last_usb_status = current_status;
            queue_post(&usb_queue, current_status, 0);
        }

        restore_irq(oldstatus);
    }
}

void usb_start_monitoring(void)
{
    int oldstatus = disable_irq_save(); /* Sync to event */
    int status = usb_detect();

    usb_monitor_enabled = true;

#ifdef USB_DETECT_BY_DRV
    status = (status == USB_INSERTED) ? USB_POWERED : USB_UNPOWERED;
#endif
    usb_status_event(status);

#ifdef USB_FIREWIRE_HANDLING
    if (firewire_detect())
        usb_firewire_connect_event();
#endif

    restore_irq(oldstatus);
}

#ifdef USB_FIREWIRE_HANDLING
void usb_firewire_connect_event(void)
{
    queue_post(&usb_queue, USB_REQUEST_REBOOT, 0);
}
#endif /* USB_FIREWIRE_HANDLING */
#else /* !USB_STATUS_BY_EVENT */
static void usb_tick(void)
{
    int current_status;

    if(usb_monitor_enabled)
    {
#ifdef USB_FIREWIRE_HANDLING
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
#endif /* USB_FIREWIRE_HANDLING */

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
#if (CONFIG_STORAGE & STORAGE_MMC)
    if(usb_mmc_countdown > 0)
    {
        usb_mmc_countdown--;
        if (usb_mmc_countdown == 0)
            queue_post(&usb_queue, USB_REENABLE, 0);
    }
#endif
}

void usb_start_monitoring(void)
{
    usb_monitor_enabled = true;
}
#endif /* USB_STATUS_BY_EVENT */
#endif /* USB_FULL_INIT */

void usb_acknowledge(long id)
{
    queue_post(&usb_queue, id, 0);
}

void usb_init(void)
{
    /* We assume that the USB cable is extracted */
    usb_state = USB_EXTRACTED;
#ifdef USB_DETECT_BY_DRV
    last_usb_status = USB_UNPOWERED;
#else
    last_usb_status = USB_EXTRACTED;
#endif
    usb_monitor_enabled = false;

#ifdef HAVE_USBSTACK
    exclusive_storage_access = false;
#endif

#ifdef USB_FIREWIRE_HANDLING
    firewire_countdown = -1;
    last_firewire_status = false;
#endif

    usb_init_device();
#ifdef USB_FULL_INIT
    usb_enable(false);

    queue_init(&usb_queue, true);
    
    usb_thread_entry = create_thread(usb_thread, usb_stack,
                       sizeof(usb_stack), 0, usb_thread_name
                       IF_PRIO(, PRIORITY_SYSTEM) IF_COP(, CPU));

#ifndef USB_STATUS_BY_EVENT
    countdown = -1;
    tick_add_task(usb_tick);
#endif
#endif /* USB_FULL_INIT */
}

void usb_wait_for_disconnect(struct event_queue *q)
{
#ifdef USB_FULL_INIT
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
#else
    (void)q;
#endif /* USB_FULL_INIT */
}

int usb_wait_for_disconnect_w_tmo(struct event_queue *q, int ticks)
{
#ifdef USB_FULL_INIT
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
            case SYS_TIMEOUT:
                return 1;
        }
    }
#else
    (void)q; (void)ticks;
    return 0;
#endif /* USB_FULL_INIT */
}

#ifdef USB_DRIVER_CLOSE
void usb_close(void)
{
    unsigned int thread = usb_thread_entry;
    usb_thread_entry = 0;

    if (thread == 0)
        return;

#ifndef USB_STATUS_BY_EVENT
    tick_remove_task(usb_tick);
#endif
    usb_monitor_enabled = false;

    queue_post(&usb_queue, USB_QUIT, 0);
    thread_wait(thread);
}
#endif /* USB_DRIVER_CLOSE */

bool usb_inserted(void)
{
#ifdef HAVE_USB_POWER
    return usb_state == USB_INSERTED || usb_state == USB_POWERED;
#else
    return usb_state == USB_INSERTED;
#endif
}

#ifdef HAVE_USBSTACK
bool usb_exclusive_storage(void)
{
    return exclusive_storage_access;
}
#endif

int usb_release_exclusive_storage(void)
{
    int num_acks_to_expect;
#ifdef HAVE_USBSTACK
    exclusive_storage_access = false;
#endif /* HAVE_USBSTACK */
    /* Tell all threads that we are back in business */
    num_acks_to_expect =
        queue_broadcast(SYS_USB_DISCONNECTED, 0) - 1;
    DEBUGF("USB extracted. Waiting for ack from %d threads...\n",
           num_acks_to_expect);
    return num_acks_to_expect;
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

int usb_detect(void)
{
    return USB_EXTRACTED;
}

void usb_wait_for_disconnect(struct event_queue *q)
{
   (void)q;
}

#endif /* USB_NONE or SIMULATOR */
