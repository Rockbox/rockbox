/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "ata.h"
#include "kernel.h"
#include "thread.h"
#include "led.h"
#include "sh7034.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "usb.h"
#include "power.h"
#include "string.h"
#include "hwcompat.h"
#include "adc.h"

/* use file for an MMC-based system, FIXME in makefile */
#ifdef HAVE_MMC 

#define SECTOR_SIZE     512
#define Q_SLEEP 0

/* for compatibility */
bool old_recorder = false; /* FIXME: get rid of this cross-dependency */
int ata_spinup_time = 0;
static int sleep_timeout = 5*HZ;
char ata_device = 0; /* device 0 (master) or 1 (slave) */
int ata_io_address = 0; /* 0x300 or 0x200, only valid on recorder */
static unsigned short identify_info[SECTOR_SIZE];

static struct mutex ata_mtx;

static bool sleeping = true;

static char ata_stack[DEFAULT_STACK_SIZE];
static const char ata_thread_name[] = "ata";
static struct event_queue ata_queue;
static bool initialized = false;
static bool delayed_write = false;
static unsigned char delayed_sector[SECTOR_SIZE];
static int delayed_sector_num;

static long last_user_activity = -1;
long last_disk_activity = -1;


int ata_read_sectors(unsigned long start,
                     int incount,
                     void* inbuf)
{
    int ret = 0;

    mutex_lock(&ata_mtx);

    last_disk_activity = current_tick;

    led(true);
    sleeping = false;

    /* ToDo: action */
    (void)start;
    (void)incount;
    (void)inbuf;

    led(false);

    mutex_unlock(&ata_mtx);

    /* only flush if reading went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}



int ata_write_sectors(unsigned long start,
                      int count,
                      const void* buf)
{
    int ret = 0;

    if (start == 0)
        panicf("Writing on sector 0\n");

    mutex_lock(&ata_mtx);
    sleeping = false;
    
    last_disk_activity = current_tick;

    led(true);

    /* ToDo: action */
    (void)start;
    (void)count;
    (void)buf;

    led(false);

    mutex_unlock(&ata_mtx);

    /* only flush if writing went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}

extern void ata_delayed_write(unsigned long sector, const void* buf)
{
    memcpy(delayed_sector, buf, SECTOR_SIZE);
    delayed_sector_num = sector;
    delayed_write = true;
}

extern void ata_flush(void)
{
    if ( delayed_write ) {
        DEBUGF("ata_flush()\n");
        delayed_write = false;
        ata_write_sectors(delayed_sector_num, 1, delayed_sector);
    }
}

void ata_spindown(int seconds)
{
    sleep_timeout = seconds * HZ;
}

bool ata_disk_is_active(void)
{
    return !sleeping;
}

static int ata_perform_sleep(void)
{
    int ret = 0;

    mutex_lock(&ata_mtx);

    /* ToDo: is there an equivalent? */

    sleeping = true;
    mutex_unlock(&ata_mtx);
    return ret;
}

int ata_standby(int time)
{
    int ret = 0;

    mutex_lock(&ata_mtx);

    /* ToDo: is there an equivalent? */
    (void)time;

    mutex_unlock(&ata_mtx);
    return ret;
}

int ata_sleep(void)
{
    queue_post(&ata_queue, Q_SLEEP, NULL);
    return 0;
}

void ata_spin(void)
{
    last_user_activity = current_tick;
}

static void ata_thread(void)
{
    static long last_sleep = 0;
    struct event ev;
    
    while (1) {
        while ( queue_empty( &ata_queue ) ) {
            if ( sleep_timeout && !sleeping &&
                 TIME_AFTER( current_tick, 
                             last_user_activity + sleep_timeout ) &&
                 TIME_AFTER( current_tick, 
                             last_disk_activity + sleep_timeout ) )
            {
                ata_perform_sleep();
                last_sleep = current_tick;
            }

            sleep(HZ/4);
        }
        queue_wait(&ata_queue, &ev);
        switch ( ev.id ) {
#ifndef USB_NONE
            case SYS_USB_CONNECTED:
                /* Tell the USB thread that we are safe */
                DEBUGF("ata_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&ata_queue);
                break;
#endif
            case Q_SLEEP:
                last_disk_activity = current_tick - sleep_timeout + (HZ/2);
                break;
        }
    }
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
    int ret = 0;
    
    return ret;
}

int ata_soft_reset(void)
{
    int ret = 0;

    return ret;
}

void ata_enable(bool on)
{
    PBCR1 &= ~0x0C00; /* use PB13 as GPIO, if not modified below */
    if (on)
    {
        /* serial setup */
        PBCR1 |=  0x0800; /* as SCK1 */
    }
    else
    {
        PBDR |= 0x2000; /* drive PB13 high */
        PBIOR |= 0x2000; /* output PB13 */
    }
}

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

int ata_init(void)
{
    int rc = 0;

    mutex_init(&ata_mtx);

    led(false);

    /* Port setup */
    PADR |= 0x1600; /* set all the selects high (=inactive) */
    PAIOR |= 0x1600; /* make outputs for them */

    if(adc_read(ADC_MMC_SWITCH) < 0x200)
    {   /* MMC inserted */
        PADR &= ~0x1000; /* clear PA12 */
    }
    else
    {   /* no MMC, use internal memory */
        PADR |= 0x1000; /* set PA12 */
    }

    sleeping = false;
    ata_enable(true);

    if ( !initialized ) {

        queue_init(&ata_queue);

        last_disk_activity = current_tick;
        create_thread(ata_thread, ata_stack,
                      sizeof(ata_stack), ata_thread_name);
        initialized = true;
    }

    return rc;
}

#endif /* #ifdef HAVE_MMC */
