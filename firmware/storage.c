/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Frank Gevaerts
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
#include "storage.h"
#include "kernel.h"

#ifdef CONFIG_STORAGE_MULTI

#define DRIVER_MASK     0xff000000
#define DRIVER_OFFSET   24
#define DRIVE_MASK      0x00ff0000
#define DRIVE_OFFSET    16
#define PARTITION_MASK  0x0000ff00

static unsigned int storage_drivers[NUM_DRIVES];
static unsigned int num_drives;
#endif


#ifdef HAVE_IO_PRIORITY

/* Same for flash? */
#define STORAGE_MINIMUM_IDLE_TIME (HZ/10)
#define STORAGE_DELAY_UNIT  (HZ/20)

unsigned int storage_last_thread[NUM_DRIVES];
unsigned int storage_last_activity[NUM_DRIVES];

static bool storage_should_wait(int drive, int prio)
{
    int other_prio = thread_get_io_priority(storage_last_thread[drive]);
    if(TIME_BEFORE(current_tick,storage_last_activity[drive]+STORAGE_MINIMUM_IDLE_TIME))
    {
        if(prio<=other_prio)
        {
            /* There is another active thread, but we have lower priority */
            return false;
        }
        else
        {
            /* There is another active thread, but it has lower priority */
            return true;
        }
    }
    else
    {
        /* There's nothing going on anyway */
        return false;
    }
}

static void storage_wait_turn(IF_MD_NONVOID(int drive))
{
#ifndef HAVE_MULTIDRIVE
    int drive=0;
#endif
    int my_prio = thread_get_io_priority(thread_get_current());
    int loops=my_prio;
    while(storage_should_wait(drive, my_prio) && (loops--)>=0)
    {
        sleep(STORAGE_DELAY_UNIT);
    }

    storage_last_thread[drive] = thread_get_current();
    storage_last_activity[drive] = current_tick;
}
#endif

int storage_read_sectors(IF_MD2(int drive,) unsigned long start, int count,
                         void* buf)
{
#ifdef HAVE_IO_PRIORITY
    storage_wait_turn(IF_MD(drive));
#endif

#ifdef CONFIG_STORAGE_MULTI
    int driver=(storage_drivers[drive] & DRIVER_MASK)>>DRIVER_OFFSET;
    int ldrive=(storage_drivers[drive] & DRIVE_MASK)>>DRIVE_OFFSET;

    switch (driver)
    {
#if (CONFIG_STORAGE & STORAGE_ATA)
    case STORAGE_ATA:
        return ata_read_sectors(IF_MD2(ldrive,) start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    case STORAGE_MMC:
        return mmc_read_sectors(IF_MD2(ldrive,) start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    case STORAGE_SD:
        return sd_read_sectors(IF_MD2(ldrive,) start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    case STORAGE_NAND:
        return nand_read_sectors(IF_MD2(ldrive,) start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    case STORAGE_RAMDISK:
        return ramdisk_read_sectors(IF_MD2(ldrive,) start,count,buf);
#endif
    }
    
    return -1;
#else /* CONFIG_STORAGE_MULTI */
    return STORAGE_FUNCTION(read_sectors)(IF_MD2(drive,)start,count,buf);
#endif /* CONFIG_STORAGE_MULTI */

}

int storage_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                          const void* buf)
{
#ifdef HAVE_IO_PRIORITY
    storage_wait_turn(IF_MD(drive));
#endif

#ifdef CONFIG_STORAGE_MULTI
    int driver=(storage_drivers[drive] & DRIVER_MASK)>>DRIVER_OFFSET;
    int ldrive=(storage_drivers[drive] & DRIVE_MASK)>>DRIVE_OFFSET;
    
    switch (driver)
    {
#if (CONFIG_STORAGE & STORAGE_ATA)
    case STORAGE_ATA:
        return ata_write_sectors(IF_MD2(ldrive,)start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    case STORAGE_MMC:
        return mmc_write_sectors(IF_MD2(ldrive,)start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    case STORAGE_SD:
        return sd_write_sectors(IF_MD2(ldrive,)start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    case STORAGE_NAND:
        return nand_write_sectors(IF_MD2(ldrive,)start,count,buf);
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    case STORAGE_RAMDISK:
        return ramdisk_write_sectors(IF_MD2(ldrive,)start,count,buf);
#endif
    }
    
    return -1;
#else /* CONFIG_STORAGE_MULTI */
    return STORAGE_FUNCTION(write_sectors)(IF_MD2(drive,)start,count,buf);
#endif /* CONFIG_STORAGE_MULTI */
}

#ifdef CONFIG_STORAGE_MULTI

#define DRIVER_MASK     0xff000000
#define DRIVER_OFFSET   24
#define DRIVE_MASK      0x00ff0000
#define DRIVE_OFFSET    16
#define PARTITION_MASK  0x0000ff00

static unsigned int storage_drivers[NUM_DRIVES];
static unsigned int num_drives;

int storage_num_drives(void)
{
    return num_drives;
}

int storage_init(void)
{
    int rc=0;
    int i;
    num_drives=0;
    
#if (CONFIG_STORAGE & STORAGE_ATA)
    if ((rc=ata_init())) return rc;
    
    int ata_drives = ata_num_drives(num_drives);
    for (i=0; i<ata_drives; i++)
    {
        storage_drivers[num_drives++] = 
            (STORAGE_ATA<<DRIVER_OFFSET) | (i << DRIVE_OFFSET);
    }
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    if ((rc=mmc_init())) return rc;
    
    int mmc_drives = mmc_num_drives(num_drives);
    for (i=0; i<mmc_drives ;i++)
    {
        storage_drivers[num_drives++] =
            (STORAGE_MMC<<DRIVER_OFFSET) | (i << DRIVE_OFFSET);
    }
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    if ((rc=sd_init())) return rc;
    
    int sd_drives = sd_num_drives(num_drives);
    for (i=0; i<sd_drives; i++)
    {
        storage_drivers[num_drives++] =
            (STORAGE_SD<<DRIVER_OFFSET) | (i << DRIVE_OFFSET);
    }
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    if ((rc=nand_init())) return rc;
    
    int nand_drives = nand_num_drives(num_drives);
    for (i=0; i<nand_drives; i++)
    {
        storage_drivers[num_drives++] =
            (STORAGE_NAND<<DRIVER_OFFSET) | (i << DRIVE_OFFSET);
    }
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    if ((rc=ramdisk_init())) return rc;
    
    int ramdisk_drives = ramdisk_num_drives(num_drives);
    for (i=0; i<ramdisk_drives; i++)
    {
        storage_drivers[num_drives++] =
            (STORAGE_RAMDISK<<DRIVER_OFFSET) | (i << DRIVE_OFFSET);
    }
#endif

    return 0;
}


void storage_enable(bool on)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_enable(on);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    mmc_enable(on);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    sd_enable(on);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    nand_enable(on);
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    ramdisk_enable(on);
#endif
}

void storage_sleep(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_sleep();
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    mmc_sleep();
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    sd_sleep();
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    nand_sleep();
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    ramdisk_sleep();
#endif
}

void storage_sleepnow(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_sleepnow();
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    mmc_sleepnow();
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    //sd_sleepnow();
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    nand_sleepnow();
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    ramdisk_sleepnow();
#endif
}

bool storage_disk_is_active(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    if (ata_disk_is_active()) return true;
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    if (mmc_disk_is_active()) return true;
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    //if (sd_disk_is_active()) return true;
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    //if (nand_disk_is_active()) return true;
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    if (ramdisk_disk_is_active()) return true;
#endif

    return false;
}

int storage_soft_reset(void)
{
    int rc=0;
    
#if (CONFIG_STORAGE & STORAGE_ATA)
    if ((rc=ata_soft_reset())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    if ((rc=mmc_soft_reset())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    //if ((rc=sd_soft_reset())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    //if ((rc=nand_soft_reset())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    if ((rc=ramdisk_soft_reset())) return rc;
#endif

    return rc;
}

#ifdef HAVE_STORAGE_FLUSH
int storage_flush(void)
{
    int rc=0;
    
#if (CONFIG_STORAGE & STORAGE_ATA)
    //if ((rc=ata_flush())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    //if ((rc=mmc_flush())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    //if ((rc=sd_flush())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    if ((rc=nand_flush())) return rc;
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    //if ((rc=ramdisk_flush())) return rc;
#endif

    return rc;
}
#endif

void storage_spin(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_spin();
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    mmc_spin();
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    sd_spin();
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    nand_spin();
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    ramdisk_spin();
#endif
}

void storage_spindown(int seconds)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_spindown(seconds);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    mmc_spindown(seconds);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    sd_spindown(seconds);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    nand_spindown(seconds);
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    ramdisk_spindown(seconds);
#endif
}

#if (CONFIG_LED == LED_REAL)
void storage_set_led_enabled(bool enabled)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_set_led_enabled(enabled);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    mmc_set_led_enabled(enabled);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    sd_set_led_enabled(enabled);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    nand_set_led_enabled(enabled);
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    ramdisk_set_led_enabled(enabled);
#endif
}
#endif /* CONFIG_LED == LED_REAL */

long storage_last_disk_activity(void)
{
    long max=0;
    long t;
    
#if (CONFIG_STORAGE & STORAGE_ATA)
    t=ata_last_disk_activity();
    if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    t=mmc_last_disk_activity();
    if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    t=sd_last_disk_activity();
    if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    t=nand_last_disk_activity();
    if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    t=ramdisk_last_disk_activity();
    if (t>max) max=t;
#endif

    return max;
}

int storage_spinup_time(void)
{
    int max=0;
    int t;
    
#if (CONFIG_STORAGE & STORAGE_ATA)
    t=ata_spinup_time();
    if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    t=mmc_spinup_time();
    if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    //t=sd_spinup_time();
    //if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    t=nand_spinup_time();
    if (t>max) max=t;
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    t=ramdisk_spinup_time();
    if (t>max) max=t;
#endif

    return max;
}

#ifdef STORAGE_GET_INFO
void storage_get_info(int drive, struct storage_info *info)
{
    int driver=(storage_drivers[drive] & DRIVER_MASK)>>DRIVER_OFFSET;
    int ldrive=(storage_drivers[drive] & DRIVE_MASK)>>DRIVE_OFFSET;
    
    switch(driver)
    {
#if (CONFIG_STORAGE & STORAGE_ATA)
    case STORAGE_ATA:
        return ata_get_info(ldrive,info);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    case STORAGE_MMC:
        return mmc_get_info(ldrive,info);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    case STORAGE_SD:
        return sd_get_info(ldrive,info);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    case STORAGE_NAND:
        return nand_get_info(ldrive,info);
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    case STORAGE_RAMDISK:
        return ramdisk_get_info(ldrive,info);
#endif
    }
}
#endif /* STORAGE_GET_INFO */

#ifdef HAVE_HOTSWAP
bool storage_removable(int drive)
{
    bool ret = false;
    
    int driver=(storage_drivers[drive] & DRIVER_MASK)>>DRIVER_OFFSET;
    int ldrive=(storage_drivers[drive] & DRIVE_MASK)>>DRIVE_OFFSET;
    
    switch(driver)
    {
#if (CONFIG_STORAGE & STORAGE_ATA)
    case STORAGE_ATA:
        ret = ata_removable(ldrive);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    case STORAGE_MMC:
        ret = mmc_removable(ldrive);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    case STORAGE_SD:
        ret = sd_removable(ldrive);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    case STORAGE_NAND:
        ret = false;
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    case STORAGE_RAMDISK:
        ret = false;
#endif
    }
    
    return ret;
}

bool storage_present(int drive)
{
    bool ret = false;
    
    int driver=(storage_drivers[drive] & DRIVER_MASK)>>DRIVER_OFFSET;
    int ldrive=(storage_drivers[drive] & DRIVE_MASK)>>DRIVE_OFFSET;
    
    switch(driver)
    {
#if (CONFIG_STORAGE & STORAGE_ATA)
    case STORAGE_ATA:
        ret = ata_present(ldrive);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
    case STORAGE_MMC:
        ret = mmc_present(ldrive);
#endif

#if (CONFIG_STORAGE & STORAGE_SD)
    case STORAGE_SD:
        ret = sd_present(ldrive);
#endif

#if (CONFIG_STORAGE & STORAGE_NAND)
    case STORAGE_NAND:
        ret = true;
#endif

#if (CONFIG_STORAGE & STORAGE_RAMDISK)
    case STORAGE_RAMDISK:
        ret = true;
#endif
    }
    
    return ret;
}
#endif

#endif /*CONFIG_STORAGE_MULTI*/
