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
#include "led.h"
#include "sh7034.h"
#include "system.h"
#include "debug.h"
#include "panic.h"

#define SECTOR_SIZE     512
#define ATA_DATA        (*((volatile unsigned short*)0x06104100))
#define ATA_ERROR       (*((volatile unsigned char*)0x06100101))
#define ATA_FEATURE     ATA_ERROR
#define ATA_NSECTOR     (*((volatile unsigned char*)0x06100102))
#define ATA_SECTOR      (*((volatile unsigned char*)0x06100103))
#define ATA_LCYL        (*((volatile unsigned char*)0x06100104))
#define ATA_HCYL        (*((volatile unsigned char*)0x06100105))
#define ATA_SELECT      (*((volatile unsigned char*)0x06100106))
#define ATA_COMMAND     (*((volatile unsigned char*)0x06100107))
#define ATA_STATUS      (*((volatile unsigned char*)0x06100107))

#define ATA_CONTROL1    ((volatile unsigned char*)0x06200206)
#define ATA_CONTROL2    ((volatile unsigned char*)0x06200306)

#define ATA_CONTROL     (*ata_control)
#define ATA_ALT_STATUS  ATA_CONTROL

#define SELECT_DEVICE1  0x10
#define SELECT_LBA      0x40

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01

#define CONTROL_nIEN    0x02
#define CONTROL_SRST    0x04

#define CMD_READ_SECTORS           0x20
#define CMD_WRITE_SECTORS          0x30
#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_STANDBY                0xE2
#define CMD_SLEEP                  0xE6
#define CMD_SECURITY_FREEZE_LOCK   0xF5

static struct mutex ata_mtx;
static char device; /* device 0 (master) or 1 (slave) */

static volatile unsigned char* ata_control;

bool old_recorder = false;

static int wait_for_bsy(void)
{
    int timeout = current_tick + HZ*4;
    while (TIME_BEFORE(current_tick, timeout) && (ATA_ALT_STATUS & STATUS_BSY))
        yield();

    if (TIME_BEFORE(current_tick, timeout))
    {
        return 1;
    }
    else
    {
        return 0; /* timeout */
    }
}

static int wait_for_rdy(void)
{
    if (!wait_for_bsy())
        return 0;
    return ATA_ALT_STATUS & STATUS_RDY;
}

static int wait_for_start_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_BSY|STATUS_DRQ)) == STATUS_DRQ;
}

static int wait_for_end_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_RDY|STATUS_DRQ)) == STATUS_RDY;
}    

int ata_read_sectors(unsigned long start,
                     unsigned char count,
                     void* buf)
{
    int i;
    int ret = 0;

    mutex_lock(&ata_mtx);
    
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        return -1;
    }

    led(true);

    ATA_NSECTOR = count;
    ATA_SECTOR  = start & 0xff;
    ATA_LCYL    = (start >> 8) & 0xff;
    ATA_HCYL    = (start >> 16) & 0xff;
    ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA | device;
    ATA_COMMAND = CMD_READ_SECTORS;

    for (i=0; i<count; i++) {
        int j;
        if (!wait_for_start_of_transfer())
        {
            mutex_unlock(&ata_mtx);
            return -1;
        }

        for (j=0; j<SECTOR_SIZE/2; j++)
            ((unsigned short*)buf)[j] = SWAB16(ATA_DATA);

#ifdef USE_INTERRUPT
        /* reading the status register clears the interrupt */
        j = ATA_STATUS;
#endif
        buf += SECTOR_SIZE; /* Advance one sector */
    }

    led(false);

    if(!wait_for_end_of_transfer())
        ret = -1;

    mutex_unlock(&ata_mtx);
    return ret;
}

#ifdef DISK_WRITE
int ata_write_sectors(unsigned long start,
                      unsigned char count,
                      void* buf)
{
    int i;

    mutex_lock(&ata_mtx);
    
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        return 0;
    }

    led(true);

    ATA_NSECTOR = count;
    ATA_SECTOR  = start & 0xff;
    ATA_LCYL    = (start >> 8) & 0xff;
    ATA_HCYL    = (start >> 16) & 0xff;
    ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA | device;
    ATA_COMMAND = CMD_WRITE_SECTORS;

    for (i=0; i<count; i++) {
        int j;
        if (!wait_for_start_of_transfer())
        {
            mutex_unlock(&ata_mtx);
            return 0;
        }

        for (j=0; j<SECTOR_SIZE/2; j++)
            ATA_DATA = SWAB16(((unsigned short*)buf)[j]);

#ifdef USE_INTERRUPT
        /* reading the status register clears the interrupt */
        j = ATA_STATUS;
#endif
        buf += SECTOR_SIZE;
    }

    led(false);

    i = wait_for_end_of_transfer();

    mutex_unlock(&ata_mtx);
    return i;
}
#endif

static int check_registers(void)
{
    if ( ATA_STATUS & STATUS_BSY )
        return -1;
    
    ATA_NSECTOR = 0xa5;
    ATA_SECTOR  = 0x5a;
    ATA_LCYL    = 0xaa;
    ATA_HCYL    = 0x55;

    if ((ATA_NSECTOR == 0xa5) &&
        (ATA_SECTOR  == 0x5a) &&
        (ATA_LCYL    == 0xaa) &&
        (ATA_HCYL    == 0x55))
        return 0;

    return -2;
}

static int freeze_lock(void)
{
    if (!wait_for_rdy())
        return -1;

    ATA_COMMAND = CMD_SECURITY_FREEZE_LOCK;

    if (!wait_for_rdy())
        return -1;

    return 0;
}

int ata_spindown(int time)
{
    int ret = 0;

    mutex_lock(&ata_mtx);
    
    if(!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        return -1;
    }

    if ( time == -1 ) {
        ATA_COMMAND = CMD_STANDBY_IMMEDIATE;
    }
    else {
        if (time > 255)
        {
            mutex_unlock(&ata_mtx);
            return -1;
        }
        ATA_NSECTOR = time & 0xff;
        ATA_COMMAND = CMD_STANDBY;
    }

    if (!wait_for_rdy())
        ret = -1;

    mutex_unlock(&ata_mtx);
    return ret;
}

int ata_hard_reset(void)
{
    int ret;
    
    mutex_lock(&ata_mtx);
    
    PADR &= ~0x0200;

    sleep(2);

    PADR |= 0x0200;

    ret =  wait_for_rdy();

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    mutex_unlock(&ata_mtx);
    return ret;
}

int ata_soft_reset(void)
{
    int ret;
    int retry_count;
    
    mutex_lock(&ata_mtx);
    
    ATA_SELECT = SELECT_LBA | device;
    ATA_CONTROL = CONTROL_nIEN|CONTROL_SRST;
    sleep(HZ/20000); /* >= 5us */

    ATA_CONTROL = CONTROL_nIEN;
    sleep(HZ/400); /* >2ms */

    /* This little sucker can take up to 30 seconds */
    retry_count = 8;
    do
    {
	ret = wait_for_rdy();
    } while(!ret && retry_count--);

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;
    
    mutex_unlock(&ata_mtx);
    return ret;
}

static int master_slave_detect(void)
{
    /* master? */
    ATA_SELECT = 0;
    if ( ATA_STATUS & STATUS_RDY ) {
        device = 0;
        DEBUGF("Found master harddisk\n");
    }
    else {
        /* slave? */
        ATA_SELECT = SELECT_DEVICE1;
        if ( ATA_STATUS & STATUS_RDY ) {
            device = SELECT_DEVICE1;
            DEBUGF("Found slave harddisk\n");
        }
        else
            return -1;
    }
    return 0;
}

static int io_address_detect(void)
{
    unsigned char tmp = ATA_STATUS & 0xf9; /* Mask the IDX and CORR bits */
    unsigned char dummy;
    
    /* We compare the STATUS register with the ALT_STATUS register, which
       is located at the same address as CONTROL. If they are the same, we
       assume that we have the correct address.

       We can't read the ATA_STATUS directly, since the read data will stay
       on the data bus if the following read does not assert the Chip Select
       to the ATA controller. We read a register that we know exists to make
       sure that the data on the bus isn't identical to the STATUS register
       contents. */
    ATA_SECTOR = 0;
    dummy = ATA_SECTOR;
    if(tmp == ((*ATA_CONTROL2) & 0xf9))
    {
        DEBUGF("CONTROL is at 0x306\n");
        old_recorder = true;
        ata_control = ATA_CONTROL2;
    }
    else
    {
        DEBUGF("CONTROL is at 0x206\n");
        old_recorder = false;
        ata_control = ATA_CONTROL1;
    }

    /* Let's check again, to be sure */
    if(tmp != ATA_CONTROL)
    {
        DEBUGF("ATA I/O address detection failed\n");
        return -1;
    }
    return 0;
}

void ata_enable(bool on)
{
    if(on)
	PADR &= ~0x80; /* enable ATA */
    else
	PADR |= 0x80;   /* disable ATA */

    PAIOR |= 0x80;
}

int ata_init(void)
{
    mutex_init(&ata_mtx);
    
    led(false);

    ata_enable(true);

    if (master_slave_detect())
        return -1;

    if (io_address_detect())
        return -2;

    if (check_registers())
        return -3;

    if (freeze_lock())
        return -4;

    if (ata_spindown(1))
        return -5;

    ATA_SELECT = SELECT_LBA;
    ATA_CONTROL = CONTROL_nIEN;

    return 0;
}
