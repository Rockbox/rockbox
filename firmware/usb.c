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
#include "disk.h"
#include "panic.h"
#include "lcd.h"
#include "usb.h"
#include "button.h"
#include "string.h"
#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif
#include "logf.h"
#include "screendump.h"

/* Conditions under which we want the entire driver */
#if !defined(BOOTLOADER) || (CONFIG_CPU == SH7034) || \
     (defined(HAVE_USBSTACK) && defined(HAVE_BOOTLOADER_USB_MODE)) || \
     (defined(HAVE_USBSTACK) && defined(IPOD_NANO2G)) || \
     (defined(HAVE_USBSTACK) && (defined(CREATIVE_ZVx))) || \
     (defined(HAVE_USBSTACK) && (defined(OLYMPUS_MROBE_500))) || \
     defined(CPU_TCC77X) || defined(CPU_TCC780X) || \
     (CONFIG_USBOTG == USBOTG_JZ4740) || \
     (CONFIG_USBOTG == USBOTG_JZ4760)
/* TODO: condition should be reset to be only the original
   (defined(HAVE_USBSTACK) && defined(HAVE_BOOTLOADER_USB_MODE)) */
#define USB_FULL_INIT
#endif

#ifdef HAVE_LCD_BITMAP
bool do_screendump_instead_of_usb = false;
#endif

#if !defined(SIMULATOR) && !defined(USB_NONE)

/* We assume that the USB cable is extracted */
static int usb_state = USB_EXTRACTED;
#if (CONFIG_STORAGE & STORAGE_MMC) && defined(USB_FULL_INIT) && !defined(HAVE_USBSTACK)
static int usb_mmc_countdown = 0;
#endif

/* Make sure there's enough stack space for screendump */
#ifdef USB_FULL_INIT
static long usb_stack[(DEFAULT_STACK_SIZE + DUMP_BMP_LINESIZE)/sizeof(long)];
static const char usb_thread_name[] = "usb";
static unsigned int usb_thread_entry = 0;
static bool usb_monitor_enabled = false;
#endif /* USB_FULL_INIT */
static struct event_queue usb_queue SHAREDBSS_ATTR;
static bool exclusive_storage_access = false;
#ifdef USB_ENABLE_HID
static bool usb_hid = true;
#endif

#ifdef USB_FULL_INIT
static bool usb_host_present = false;
static int usb_num_acks_to_expect = 0;
static long usb_last_broadcast_tick = 0;
#ifdef HAVE_USB_POWER
static bool usb_charging_only = false;
#endif

static int usb_release_exclusive_storage(void);

#if defined(USB_FIREWIRE_HANDLING)
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

#ifdef IPOD_NANO2G
    memcpy((void *)0x0002bf00, "diskmodehotstuff\1\0\0\0", 20);
#endif

    system_reboot(); /* Reboot */
}
#endif /* USB_FIRWIRE_HANDLING */

/* Screen dump */
#ifdef HAVE_LCD_BITMAP
static inline bool usb_do_screendump(void)
{
    if(do_screendump_instead_of_usb)
    {
        screen_dump();
#ifdef HAVE_REMOTE_LCD
        remote_screen_dump();
#endif /* HAVE_REMOTE_LCD */
        return true;
    }
    return false;
}
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_USB_POWER
static bool usb_charge_only_setting = false;
void usb_set_charge_setting(bool charge_only)
{
    usb_charge_only_setting = charge_only;
}
#endif

/* Power (charging-only) button */
static inline void usb_detect_charging_only(bool detect)
{
#ifdef HAVE_USB_POWER
    if (detect)
        detect = button_status() & ~USBPOWER_BTN_IGNORE;

    usb_charging_only = (usb_charge_only_setting && !detect) || 
        (!usb_charge_only_setting && detect);
#endif
    (void)detect;
} 

#ifdef USB_FIREWIRE_HANDLING
static inline bool usb_reboot_button(void)
{
#ifdef HAVE_USB_POWER
    return (button_status() & ~USBPOWER_BTN_IGNORE);
#else
    return false;
#endif
}
#endif /* USB_FIREWIRE_HANDLING */


/*--- Routines that differ depending upon the presence of a USB stack ---*/

#ifdef HAVE_USBSTACK
/* Enable / disable USB when the stack is enabled - otherwise a noop */
static inline void usb_stack_enable(bool enable)
{
    usb_enable(enable);
}

#ifdef HAVE_HOTSWAP
static inline void usb_handle_hotswap(long id)
{
    switch(id)
    {
    case SYS_HOTSWAP_INSERTED:
    case SYS_HOTSWAP_EXTRACTED:
        usb_core_hotswap_event(1, id == SYS_HOTSWAP_INSERTED);
        break;
    }
    /* Note: No MMC storage handling is needed with the stack atm. */
}
#endif /* HAVE_HOTSWAP */

static inline bool usb_configure_drivers(int for_state)
{
    switch(for_state)
    {
    case USB_POWERED:
#ifdef USB_ENABLE_STORAGE
        usb_core_enable_driver(USB_DRIVER_MASS_STORAGE, false);
#endif
#ifdef USB_ENABLE_HID
#ifdef USB_ENABLE_CHARGING_ONLY
        usb_core_enable_driver(USB_DRIVER_HID, false);
#else
        usb_core_enable_driver(USB_DRIVER_HID, true);
#endif /* USB_ENABLE_CHARGING_ONLY */
#endif /* USB_ENABLE_HID */

#ifdef USB_ENABLE_CHARGING_ONLY
        usb_core_enable_driver(USB_DRIVER_CHARGING_ONLY, true);
#endif
        exclusive_storage_access = false;

        usb_attach(); /* Powered only: attach now. */
        break;
        /* USB_POWERED: */
    
    case USB_INSERTED:
#ifdef USB_ENABLE_STORAGE
        usb_core_enable_driver(USB_DRIVER_MASS_STORAGE, true);
#endif
#ifdef USB_ENABLE_HID
        usb_core_enable_driver(USB_DRIVER_HID, usb_hid);
#endif
#ifdef USB_ENABLE_CHARGING_ONLY
        usb_core_enable_driver(USB_DRIVER_CHARGING_ONLY, false);
#endif
        /* Check any drivers enabled at this point for exclusive storage
         * access requirements. */
        exclusive_storage_access = usb_core_any_exclusive_storage();

        if(exclusive_storage_access)
            return true;

        usb_attach(); /* Not exclusive: attach now. */
        break;
        /* USB_INSERTED: */

    case USB_EXTRACTED:
        if(exclusive_storage_access)
            usb_release_exclusive_storage();
        break;
        /* USB_EXTRACTED: */
    }

    return false;
}

static inline void usb_slave_mode(bool on)
{
    int rc;

    if(on)
    {
        trigger_cpu_boost();
#ifdef HAVE_PRIORITY_SCHEDULING
        thread_set_priority(thread_self(), PRIORITY_REALTIME);
#endif
        disk_unmount_all();
        usb_attach();
    }
    else /* usb_state == USB_INSERTED (only!) */
    {
#ifdef HAVE_PRIORITY_SCHEDULING
        thread_set_priority(thread_self(), PRIORITY_SYSTEM);
#endif
        /* Entered exclusive mode */
        rc = disk_mount_all();
        if(rc <= 0) /* no partition */
            panicf("mount: %d",rc);

        cancel_cpu_boost();
    }
}

void usb_signal_transfer_completion(
    struct usb_transfer_completion_event_data* event_data)
{
    queue_post(&usb_queue, USB_TRANSFER_COMPLETION, (intptr_t)event_data);
}

void usb_signal_notify(long id, intptr_t data)
{
    queue_post(&usb_queue, id, data);
}

#else  /* !HAVE_USBSTACK */

static inline void usb_stack_enable(bool enable)
{
    (void)enable;
}

#ifdef HAVE_HOTSWAP
static inline void usb_handle_hotswap(long id)
{
#if (CONFIG_STORAGE & STORAGE_MMC)
    switch(id)
    {
    case SYS_HOTSWAP_INSERTED:
    case SYS_HOTSWAP_EXTRACTED:
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
    }
#endif /* STORAGE_MMC */
    (void)id;
}
#endif /* HAVE_HOTSWAP */

static inline bool usb_configure_drivers(int for_state)
{
    switch(for_state)
    {
    case USB_POWERED:
        exclusive_storage_access = false;
        break;
    case USB_INSERTED:
        exclusive_storage_access = true;
        return true;
    case USB_EXTRACTED:
        if(exclusive_storage_access)
            usb_release_exclusive_storage();
        break;
    }

    return false;
}

static inline void usb_slave_mode(bool on)
{
    int rc;

    if(on)
    {
        DEBUGF("Entering USB slave mode\n");
        disk_unmount_all();
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

        sleep(HZ/10);
        rc = disk_mount_all();
        if(rc <= 0) /* no partition */
            panicf("mount: %d",rc);
    }
}
#endif /* HAVE_USBSTACK */

static void usb_set_host_present(bool present)
{
    if(usb_host_present == present)
        return;

    usb_host_present = present;

    if(!usb_host_present)
    {
        usb_configure_drivers(USB_EXTRACTED);
        return;
    }

#ifdef HAVE_USB_POWER
    if (usb_charging_only)
    {
        /* Only charging is desired */
        usb_configure_drivers(USB_POWERED);
        return;
    }
#endif

    if(!usb_configure_drivers(USB_INSERTED))
        return; /* Exclusive storage access not required */

    /* Tell all threads that they have to back off the storage.
       We subtract one for our own thread. Expect an ACK for every
       listener for each broadcast they received. If it has been too
       long, the user might have entered a screen that didn't ACK
       when inserting the cable, such as a debugging screen. In that
       case, reset the count or else USB would be locked out until
       rebooting because it most likely won't ever come. Simply
       resetting to the most recent broadcast count is racy. */
    if(TIME_AFTER(current_tick, usb_last_broadcast_tick + HZ*5))
    {
        usb_num_acks_to_expect = 0;
        usb_last_broadcast_tick = current_tick;
    }

    usb_num_acks_to_expect += queue_broadcast(SYS_USB_CONNECTED, 0) - 1;
    DEBUGF("usb: waiting for %d acks...\n", usb_num_acks_to_expect);
}

static bool usb_handle_connected_ack(void)
{
    if(usb_num_acks_to_expect > 0 && --usb_num_acks_to_expect == 0)
    {
        DEBUGF("usb: all threads have acknowledged the connect.\n");
        if(usb_host_present)
        {
            usb_slave_mode(true);
            return true;
        }
    }
    else
    {
        DEBUGF("usb: got ack, %d to go...\n", usb_num_acks_to_expect);
    }

    return false;
}

/*--- General driver code ---*/
static void NORETURN_ATTR usb_thread(void)
{
    struct queue_event ev;

    while(1)
    {
        queue_wait(&usb_queue, &ev);

        switch(ev.id)
        {
        /*** Main USB thread duties ***/

#ifdef HAVE_USBSTACK
        case USB_NOTIFY_SET_ADDR:
        case USB_NOTIFY_SET_CONFIG:
            if(usb_state <= USB_EXTRACTED)
                break;
            usb_core_handle_notify(ev.id, ev.data);
            break;
        case USB_TRANSFER_COMPLETION:
            if(usb_state <= USB_EXTRACTED)
                break;

#ifdef USB_DETECT_BY_REQUEST
            usb_set_host_present(true);
#endif

            usb_core_handle_transfer_completion(
                (struct usb_transfer_completion_event_data*)ev.data);
            break;
#endif /* HAVE_USBSTACK */

        case USB_INSERTED:
            if(usb_state != USB_EXTRACTED)
                break;

#ifdef HAVE_LCD_BITMAP
            if(usb_do_screendump())
            {
                usb_state = USB_SCREENDUMP;
                break;
            }
#endif

            usb_state = USB_POWERED;
            usb_stack_enable(true);

            usb_detect_charging_only(true);

#ifndef USB_DETECT_BY_REQUEST
            usb_set_host_present(true);
#endif
            break;
            /* USB_INSERTED */

        case SYS_USB_CONNECTED_ACK:
            if(usb_handle_connected_ack())
                usb_state = USB_INSERTED;
            break;
            /* SYS_USB_CONNECTED_ACK */

        case USB_EXTRACTED:
            if(usb_state == USB_EXTRACTED)
                break;

            if(usb_state == USB_POWERED || usb_state == USB_INSERTED)
                usb_stack_enable(false);

            /* Only disable the USB slave mode if we really have enabled
               it. Some expected acks may not have been received. */
            if(usb_state == USB_INSERTED)
                usb_slave_mode(false);

            usb_state = USB_EXTRACTED;

            usb_detect_charging_only(false);
            usb_set_host_present(false);
            break;
            /* USB_EXTRACTED: */

        /*** Miscellaneous USB thread duties ***/

        /* HOTSWAP */
#ifdef HAVE_HOTSWAP
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
#if (CONFIG_STORAGE & STORAGE_MMC)
        case USB_REENABLE:
#endif
            usb_handle_hotswap(ev.id);
            break;
#endif

        /* FIREWIRE */
#ifdef USB_FIREWIRE_HANDLING
        case USB_REQUEST_REBOOT:
            if(usb_reboot_button())
                try_reboot();
            break;
#endif

        /* CHARGING */
#if defined(HAVE_USB_CHARGING_ENABLE) && defined(HAVE_USBSTACK)
        case USB_CHARGER_UPDATE:
            usb_charging_maxcurrent_change(usb_charging_maxcurrent());
            break;
#endif

        /* CLOSE */
#ifdef USB_DRIVER_CLOSE
        case USB_QUIT:
            thread_exit();
            break;
#endif
        } /* switch */
    } /* while */
}

#if defined(HAVE_USB_CHARGING_ENABLE) && defined(HAVE_USBSTACK)
void usb_charger_update(void)
{
    queue_post(&usb_queue, USB_CHARGER_UPDATE, 0);
}
#endif

#ifdef USB_STATUS_BY_EVENT
void usb_status_event(int current_status)
{
    /* Caller isn't expected to filter for changes in status.
     * current_status:
     *   USB_INSERTED, USB_EXTRACTED
     */
    if(usb_monitor_enabled)
    {
        int oldstatus = disable_irq_save(); /* Dual-use function */
        queue_remove_from_head(&usb_queue, current_status);
        queue_post(&usb_queue, current_status, 0);
        restore_irq(oldstatus);
    }
}

void usb_start_monitoring(void)
{
    int oldstatus = disable_irq_save(); /* Sync to event */
    int status = usb_detect();

    usb_monitor_enabled = true;

    /* An event may have been missed because it was sent before monitoring
     * was enabled due to the connector already having been inserted before
     * before or during boot. */
    usb_status_event(status);

#ifdef USB_FIREWIRE_HANDLING
    if(firewire_detect())
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
    #define NUM_POLL_READINGS (HZ/5)
    static int usb_countdown = -1;
    static int last_usb_status = USB_EXTRACTED;
#ifdef USB_FIREWIRE_HANDLING
    static int firewire_countdown = -1;
    static int last_firewire_status = false;
#endif    

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

        int current_status = usb_detect();

        /* Only report when the status has changed */
        if(current_status != last_usb_status)
        {
            last_usb_status = current_status;
            usb_countdown = NUM_POLL_READINGS;
        }
        else
        {
            /* Count down until it gets negative */
            if(usb_countdown >= 0)
                usb_countdown--;

            /* Report to the thread if we have had 3 identical status
               readings in a row */
            if(usb_countdown == 0)
            {
                queue_post(&usb_queue, current_status, 0);
            }
        }
    }
#if (CONFIG_STORAGE & STORAGE_MMC)
    if(usb_mmc_countdown > 0)
    {
        usb_mmc_countdown--;
        if(usb_mmc_countdown == 0)
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
    /* Do required hardware inits first. For software USB the driver has
     * to make sure this won't trigger a transfer completion before the
     * queue and thread are created. */
    usb_init_device();

#ifdef USB_FULL_INIT
    usb_enable(false);

    queue_init(&usb_queue, true);

    usb_thread_entry = create_thread(usb_thread, usb_stack,
                       sizeof(usb_stack), 0, usb_thread_name
                       IF_PRIO(, PRIORITY_SYSTEM) IF_COP(, CPU));

#ifndef USB_STATUS_BY_EVENT
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
            return;
    }
#endif /* USB_FULL_INIT */
    (void)q;
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
                return 0;
            case SYS_TIMEOUT:
                return 1;
        }
    }
#endif /* USB_FULL_INIT */
    (void)q; (void)ticks;
    return 0;
}

#ifdef USB_DRIVER_CLOSE
void usb_close(void)
{
    unsigned int thread = usb_thread_entry;
    usb_thread_entry = 0;

    if(thread == 0)
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
    return usb_state == USB_INSERTED || usb_state == USB_POWERED;
}

#ifdef HAVE_USBSTACK
bool usb_exclusive_storage(void)
{
    /* Storage isn't actually exclusive until slave mode has been entered */
    return exclusive_storage_access && usb_state == USB_INSERTED;
}
#endif /* HAVE_USBSTACK */

int usb_release_exclusive_storage(void)
{
    int bccount;
    exclusive_storage_access = false;
    /* Tell all threads that we are back in business */
    bccount = queue_broadcast(SYS_USB_DISCONNECTED, 0) - 1;
    DEBUGF("USB extracted. Broadcast to %d threads...\n", bccount);
    return bccount;
}

#ifdef HAVE_USB_POWER
bool usb_powered_only(void)
{
    return usb_state == USB_POWERED;
}
#endif /* HAVE_USB_POWER */

#ifdef USB_ENABLE_HID
void usb_set_hid(bool enable)
{
    usb_hid = enable;
    usb_core_enable_driver(USB_DRIVER_HID, usb_hid);
}
#endif /* USB_ENABLE_HID */

#elif defined(USB_NONE)
/* Dummy functions for USB_NONE  */

bool usb_inserted(void)
{
    return false;
}

void usb_acknowledge(long id)
{
    (void)id;
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
#endif /* USB_NONE */

