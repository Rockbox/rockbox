/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Jens Arnold
 * Copyright (C) 2011 by Thomas Martitz
 *
 * Rockbox simulator specific tasks
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
#include "kernel.h"
#include "screendump.h"
#include "thread.h"
#include "debug.h"
#include "usb.h"
#include "mv.h"
#include "ata_idle_notify.h"
#ifdef WIN32
#include <windows.h>
#endif

static void sim_thread(void);
static long sim_thread_stack[DEFAULT_STACK_SIZE/sizeof(long)];
            /* stack isn't actually used in the sim */
static const char sim_thread_name[] = "sim";
static struct event_queue sim_queue;

/* possible events for the sim thread */
enum {
    SIM_SCREENDUMP,
    SIM_USB_INSERTED,
    SIM_USB_EXTRACTED,
#ifdef HAVE_MULTIDRIVE
    SIM_EXT_INSERTED,
    SIM_EXT_EXTRACTED,
#endif
};

static int usb_state = USB_EXTRACTED;

#ifdef HAVE_MULTIDRIVE
static bool is_ext_inserted = false;
extern void sim_ext_status(int drive, bool inserted);
#endif

static int vol_drive[NUM_VOLUMES] = { [0 ... NUM_VOLUMES-1] = -1 };

static int get_free_volume(void)
{
    for (int i = 0; i < NUM_VOLUMES; i++)
    {
        if (vol_drive[i] == -1) /* unassigned? */
            return i;
    }

    return -1; /* none found */
}

static void usb_set_host_present(bool present)
{
    usb_host_present = preset;
    if (!usb_host_present)
    {
        /* in usb.c, this is only done for exclusive storage
         * do it here anyway but don't depend on the acks */
        disk_mount_all();
        queue_broadcast(SYS_USB_DISCONNECTED, 0);
        return;
    }

    if(TIME_AFTER(current_tick, last_broadcast_tick + HZ*5))
    {
        num_acks_to_expect = 0;
        last_broadcast_tick = current_tick;
    }

    num_acks_to_expect += queue_broadcast(SYS_USB_CONNECTED, 0) - 1;
    DEBUGF("USB inserted. Waiting for %d acks...\n",
           num_acks_to_expect);
}

bool usb_handle_connected_ack(void)
{
    if(num_acks_to_expect > 0 && --num_acks_to_expect == 0)
    {
        DEBUGF("All threads have acknowledged the connect.\n");
        disk_unmount_all();
        return true;
    }
    else
    {
        DEBUGF("usb: got ack, %d to go...\n",
               num_acks_to_expect);
    }

    return false;
}

void sim_thread(void)
{
    struct queue_event ev;
    long last_broadcast_tick = current_tick;
    int num_acks_to_expect = 0;
    
    while (1)
    {
        queue_wait_w_tmo(&sim_queue, &ev, 5*HZ);
        switch(ev.id)
        {
            case SYS_TIMEOUT:
                call_storage_idle_notifys(false);
                break;
    
            case SIM_SCREENDUMP:
                screen_dump();
#ifdef HAVE_REMOTE_LCD
                remote_screen_dump();
#endif
                break;
            case SIM_USB_INSERTED:
            /* from firmware/usb.c: */
                /* Tell all threads that they have to back off the storage.
                   We subtract one for our own thread. Expect an ACK for every
                   listener for each broadcast they received. If it has been too
                   long, the user might have entered a screen that didn't ACK
                   when inserting the cable, such as a debugging screen. In that
                   case, reset the count or else USB would be locked out until
                   rebooting because it most likely won't ever come. Simply
                   resetting to the most recent broadcast count is racy. */
                if (usb_state != USB_EXTRACTED)
                    break;

                usb_state = USB_POWERED;
                usb_set_host_present(true);
                break;
            case SYS_USB_CONNECTED_ACK:
                if (usb_handle_connected_ack())
                    usb_state = USB_INSERTED;
                break;
            case SIM_USB_EXTRACTED:
                if (usb_state == USB_EXTRACTED)
                    break;

                usb_state = USB_EXTRACTED;
                usb_set_host_present(false);
                break;
#ifdef HAVE_MULTIDRIVE
            case SIM_EXT_INSERTED:
            case SIM_EXT_EXTRACTED:
                if (is_ext_inserted == (ev.id == SIM_EXT_INSERTED))
                    break;

                is_ext_inserted = !is_ext_inserted;

                if (is_ext_inserted)
                    disk_mount(ev.id);
                else
                    disk_unmount(ev.id);

                queue_broadcast(is_ext_inserted ?
                                SYS_HOTSWAP_INSERTED : SYS_HOTSWAP_EXTRACTED, 0);
                sleep(HZ/20);
                queue_broadcast(SYS_FS_CHANGED, 0);
                break;
#endif /* HAVE_MULTIDRIVE */
            default:
                DEBUGF("sim_tasks: unhandled event: %ld\n", ev.id);
                break;
        }
    }
}

void sim_tasks_init(void)
{
    queue_init(&sim_queue, false);
    
    create_thread(sim_thread, sim_thread_stack, sizeof(sim_thread_stack), 0,
                  sim_thread_name IF_PRIO(,PRIORITY_BACKGROUND) IF_COP(,CPU));
}

void sim_trigger_screendump(void)
{
    queue_post(&sim_queue, SIM_SCREENDUMP, 0);
}

void sim_trigger_usb(bool inserted)
{
    if (inserted)
        queue_post(&sim_queue, SIM_USB_INSERTED, 0);
    else
        queue_post(&sim_queue, SIM_USB_EXTRACTED, 0);
}

int usb_detect(void)
{
    return usb_state == USB_EXTRACTED ? USB_EXTRACTED : USB_INSERTED;
}

void usb_init(void)
{
}

void usb_start_monitoring(void)
{
}

void usb_acknowledge(long id)
{
    queue_post(&sim_queue, id, 0);
}

void usb_wait_for_disconnect(struct event_queue *q)
{
    struct queue_event ev;

    /* Don't return until we get SYS_USB_DISCONNECTED */
    while(1)
    {
        queue_wait(q, &ev);
        if(ev.id == SYS_USB_DISCONNECTED)
            return;
    }
}

int disk_mount(int drive)
{
    if (!CHECK_DRV(drive))
        return 0;

    int volume = get_free_volume();
    volume_drive[drive] = volume;
    volume_onmount_internal(IF_MV(volume));

    return 1;
}

int disk_unmount(int drive)
{
    if (!CHECK_DRV(drive))
        return 0;

    int unmounted = 0;

    for (int i = 0; i < NUM_VOLUMES; i++)
    {
        if (vol_drive[i] == drive)
        {   /* force releasing resources */
            vol_drive[i] = -1;
            volume_onunmount_internal(IF_MV(i));
            unounted++;
        }
    }

    return unmounted;
}

int disk_mount_all(void)
{
    int mounted = 0;

    disk_writer_lock();

    volume_onunmount_internal(IF_MV(-1));

    for (int i = 0; i < NUM_VOLUMES; i++)
        vol_drive[i] = -1; /* mark all as unassigned */

    for (int i = 0; i < NUM_DRIVES; i++)
    {
        
}

int disk_unmount_all(void)
{
}

#ifdef HAVE_MULTIDRIVE
void sim_trigger_external(bool inserted)
{
    int drive = 1; /* Can do others! */
    if (inserted)
        queue_post(&sim_queue, SIM_EXT_INSERTED, drive);
    else
        queue_post(&sim_queue, SIM_EXT_EXTRACTED, drive);
}

bool hostfs_present(int drive)
{
    return drive > 0  ? is_ext_inserted : true;
}

bool hostfs_removable(int drive)
{
    return drive > 0;
}

#ifdef HAVE_MULTIVOLUME
bool volume_removable(int volume)
{
    /* volume == drive for now */
    return hostfs_removable(volume);
}

bool volume_present(int volume)
{
    /* volume == drive for now */
    return hostfs_present(volume);
}

int volume_drive(int volume)
{
    /* volume == drive for now */
    return volume;
}
#endif /* HAVE_MULTIVOLUME */

#if (CONFIG_STORAGE & STORAGE_MMC)
bool mmc_touched(void)
{
    return hostfs_present(1);
}
#endif

#ifdef CONFIG_STORAGE_MULTI
int hostfs_driver_type(int drive)
{
    /* Hack alert */
#if (CONFIG_STORAGE & STORAGE_ATA)
 #define SIMEXT1_TYPE_NUM   STORAGE_ATA_NUM
#elif (CONFIG_STORAGE & STORAGE_SD)
 #define SIMEXT1_TYPE_NUM   STORAGE_SD_NUM
#elif (CONFIG_STORAGE & STORAGE_MMC)
 #define SIMEXT1_TYPE_NUM   STORAGE_MMC_NUM
#elif (CONFIG_STORAGE & STORAGE_NAND)
 #define SIMEXT1_TYPE_NUM   STORAGE_NAND_NUM
#elif (CONFIG_STORAGE & STORAGE_RAMDISK)
 #define SIMEXT1_TYPE_NUM   STORAGE_RAMDISK_NUM
#else
#error Unknown storage driver
#endif /* CONFIG_STORAGE */

    return drive > 0 ? SIMEXT1_TYPE_NUM : STORAGE_HOSTFS_NUM;
}
#endif /* CONFIG_STORAGE_MULTI */

#endif /* CONFIG_STORAGE & STORAGE_MMC */
