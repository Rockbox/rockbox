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
#include "ata.h"
#include "kernel.h"
#include "led.h"

#define ATA_DATA        (*((volatile unsigned short*)0x06104100))
#define ATA_ERROR       (*((volatile unsigned char*)0x06100101))
#define ATA_FEATURE     ATA_ERROR
#define ATA_NSECTOR     (*((volatile unsigned char*)0x06100102))
#define ATA_SECTOR      (*((volatile unsigned char*)0x06100103))
#define ATA_LCYL        (*((volatile unsigned char*)0x06100104))
#define ATA_HCYL        (*((volatile unsigned char*)0x06100105))
#define ATA_SELECT      (*((volatile unsigned char*)0x06100106))
#define ATA_COMMAND     (*((volatile unsigned char*)0x06100107))
#define ATA_STATUS      ATA_COMMAND
#define ATA_CONTROL     (*((volatile unsigned char*)0x06100306))
#define ATA_ALT_STATUS  ATA_CONTROL

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

static int wait_for_bsy(void)
{
    char timeout = current_tick + HZ;
    while (TIME_BEFORE(current_tick, timeout) && (ATA_ALT_STATUS & STATUS_BSY))
        yield();

    if (TIME_BEFORE(current_tick, timeout))
        return 1;
    else
        return 0; /* timeout */
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

    if (!wait_for_rdy())
        return 0;

    led_turn_on();

    ATA_NSECTOR = count;
    ATA_SECTOR  = start & 0xff;
    ATA_LCYL    = (start >> 8) & 0xff;
    ATA_HCYL    = (start >> 16) & 0xff;
    ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA;
    ATA_COMMAND = CMD_READ_SECTORS;

    for (i=0; i<count; i++) {
        int j;
        if (!wait_for_start_of_transfer())
            return 0;

        for (j=0; j<256; j++)
            ((unsigned short*)buf)[j] = SWAB16(ATA_DATA);

#ifdef USE_INTERRUPT
        /* reading the status register clears the interrupt */
        j = ATA_STATUS;
#endif
    }

    led_turn_off();

    return wait_for_end_of_transfer();
}

int ata_write_sectors(unsigned long start,
                      unsigned char count,
                      void* buf)
{
    int i;

    if (!wait_for_rdy())
        return 0;

    led_turn_on ();

    ATA_NSECTOR = count;
    ATA_SECTOR  = start & 0xff;
    ATA_LCYL    = (start >> 8) & 0xff;
    ATA_HCYL    = (start >> 16) & 0xff;
    ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA;
    ATA_COMMAND = CMD_WRITE_SECTORS;

    for (i=0; i<count; i++) {
        int j;
        if (!wait_for_start_of_transfer())
            return 0;

        for (j=0; j<256; j++)
            ATA_DATA = SWAB16(((unsigned short*)buf)[j]);

#ifdef USE_INTERRUPT
        /* reading the status register clears the interrupt */
        j = ATA_STATUS;
#endif
    }

    led_turn_off ();

    return wait_for_end_of_transfer();
}

static int check_registers(void)
{
    if ( ATA_STATUS & STATUS_BSY )
        return 0;
    
    ATA_NSECTOR = 0xa5;
    ATA_SECTOR  = 0x5a;
    ATA_LCYL    = 0xaa;
    ATA_HCYL    = 0x55;

    if ((ATA_NSECTOR == 0xa5) &&
        (ATA_SECTOR  == 0x5a) &&
        (ATA_LCYL    == 0xaa) &&
        (ATA_HCYL    == 0x55))
        return 1;
    else
        return 0;
}

static int check_harddisk(void)
{
    if (!wait_for_rdy())
        return 0;

    if ((ATA_NSECTOR == 1) &&
        (ATA_SECTOR == 1) &&
        (ATA_LCYL == 0) &&
        (ATA_HCYL == 0) &&
        (ATA_SELECT == 0))
        return 1;
    else
        return 0;
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
    if (!wait_for_rdy())
        return -1;

    if ( time == -1 ) {
        ATA_COMMAND = CMD_STANDBY_IMMEDIATE;
    }
    else {
        if (time > 255)
            return -1;
        ATA_NSECTOR = time & 0xff;
        ATA_COMMAND = CMD_STANDBY;
    }

    if (!wait_for_rdy())
        return -1;

    return 0;
}

int ata_hard_reset(void)
{
    clear_bit(1,PADR);
    sleep(HZ/500);

    set_bit(1,PADR);
    sleep(HZ/500);

    return wait_for_rdy();
}

int ata_soft_reset(void)
{
    ATA_SELECT = SELECT_LBA;
    ATA_CONTROL = CONTROL_nIEN|CONTROL_SRST;
    sleep(HZ/20000); /* >= 5us */

    ATA_CONTROL = CONTROL_nIEN;
    sleep(HZ/400); /* >2ms */

    return wait_for_rdy();
}

int ata_init(void)
{
    led_turn_off();

    /* activate ATA */
    PADR |= 0x80;

    if (!ata_hard_reset())
        return -1;
            
    if (!check_registers())
        return -2;

    if (!check_harddisk())
        return -3;

    if (!freeze_lock())
        return -4;

    ATA_SELECT = SELECT_LBA;
    ATA_CONTROL = CONTROL_nIEN;

    return 0;
}
