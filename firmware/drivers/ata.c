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
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01

#define CONTROL_nIEN    0x02
#define CONTROL_SRST    0x04

#define CMD_READ_SECTORS           0x20
#define CMD_WRITE_SECTORS          0x30
#define CMD_READ_MULTIPLE          0xC4
#define CMD_WRITE_MULTIPLE         0xC5
#define CMD_SET_MULTIPLE_MODE      0xC6
#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_STANDBY                0xE2
#define CMD_IDENTIFY               0xEC
#define CMD_SLEEP                  0xE6
#define CMD_SECURITY_FREEZE_LOCK   0xF5

#define Q_SLEEP 0

#define READ_TIMEOUT 5*HZ

static struct mutex ata_mtx;
char ata_device; /* device 0 (master) or 1 (slave) */
int ata_io_address; /* 0x300 or 0x200, only valid on recorder */
static volatile unsigned char* ata_control;

bool old_recorder = false;
int ata_spinup_time = 0;
static bool sleeping = false;
static int sleep_timeout = 5*HZ;
static bool poweroff = false;
#ifdef HAVE_ATA_POWER_OFF
static int poweroff_timeout = 2*HZ;
#endif
static char ata_stack[DEFAULT_STACK_SIZE];
static char ata_thread_name[] = "ata";
static struct event_queue ata_queue;
static bool initialized = false;
static bool delayed_write = false;
static unsigned char delayed_sector[SECTOR_SIZE];
static int delayed_sector_num;

static long last_user_activity = -1;
long last_disk_activity = -1;

static int multisectors; /* number of supported multisectors */
static unsigned short identify_info[SECTOR_SIZE];

static int ata_power_on(void);
static int perform_soft_reset(void);
static int set_multiple_mode(int sectors);

static int wait_for_bsy(void) __attribute__ ((section (".icode")));
static int wait_for_bsy(void)
{
    int timeout = current_tick + HZ*10;
    while (TIME_BEFORE(current_tick, timeout) && (ATA_ALT_STATUS & STATUS_BSY))
        yield();

    if (TIME_BEFORE(current_tick, timeout))
        return 1;
    else
        return 0; /* timeout */
}

static int wait_for_rdy(void) __attribute__ ((section (".icode")));
static int wait_for_rdy(void)
{
    int timeout;
    
    if (!wait_for_bsy())
        return 0;

    timeout = current_tick + HZ*10;
    
    while (TIME_BEFORE(current_tick, timeout) &&
           !(ATA_ALT_STATUS & STATUS_RDY))
        yield();

    if (TIME_BEFORE(current_tick, timeout))
        return STATUS_RDY;
    else
        return 0; /* timeout */
}

static int wait_for_start_of_transfer(void) __attribute__ ((section (".icode")));
static int wait_for_start_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_BSY|STATUS_DRQ)) == STATUS_DRQ;
}

static int wait_for_end_of_transfer(void) __attribute__ ((section (".icode")));
static int wait_for_end_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_RDY|STATUS_DRQ)) == STATUS_RDY;
}    

int ata_read_sectors(unsigned long start,
                     int count,
                     void* buf) __attribute__ ((section (".icode")));
int ata_read_sectors(unsigned long start,
                     int incount,
                     void* inbuf)
{
    int ret = 0;
    int timeout;
    int count;
    void* buf;

    last_disk_activity = current_tick;

    mutex_lock(&ata_mtx);

    led(true);

    if ( sleeping ) {
        if (poweroff) {
            if (ata_power_on()) {
                mutex_unlock(&ata_mtx);
                return -1;
            }
        }
        else {
            if (perform_soft_reset()) {
                mutex_unlock(&ata_mtx);
                return -1;
            }
        }
        sleeping = false;
        poweroff = false;
        ata_spinup_time = current_tick - last_disk_activity;
    }

    ATA_SELECT = ata_device;
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        return -2;
    }

    timeout = current_tick + READ_TIMEOUT;

 retry:
    buf = inbuf;
    count = incount;
    while (TIME_BEFORE(current_tick, timeout)) {

        if ( count == 256 )
            ATA_NSECTOR = 0; /* 0 means 256 sectors */
        else
            ATA_NSECTOR = (unsigned char)count;

        ATA_SECTOR  = start & 0xff;
        ATA_LCYL    = (start >> 8) & 0xff;
        ATA_HCYL    = (start >> 16) & 0xff;
        ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA | ata_device;
        ATA_COMMAND = CMD_READ_MULTIPLE;

        while (count) {
            int j;
            int sectors;
            int wordcount;

            if (!wait_for_start_of_transfer()) {
                ret = -4;
                goto retry;
            }

            if ( ATA_ALT_STATUS & (STATUS_ERR | STATUS_DF) ) {
                ret = -5;
                goto retry;
            }
             
            /* if destination address is odd, use byte copying,
               otherwise use word copying */

            if (count >= multisectors )
                sectors = multisectors;
            else
                sectors = count;

            wordcount = sectors * SECTOR_SIZE / 2;

            if ( (unsigned int)buf & 1 ) {
                for (j=0; j < wordcount; j++) {
                    unsigned short tmp = SWAB16(ATA_DATA);
                    ((unsigned char*)buf)[j*2] = tmp >> 8;
                    ((unsigned char*)buf)[j*2+1] = tmp & 0xff;
                }
            }
            else {
                for (j=0; j < wordcount; j++)
                    ((unsigned short*)buf)[j] = SWAB16(ATA_DATA);
            }

#ifdef USE_INTERRUPT
            /* reading the status register clears the interrupt */
            j = ATA_STATUS;
#endif
            buf += sectors * SECTOR_SIZE; /* Advance one chunk of sectors */
            count -= sectors;
        }

        if(!wait_for_end_of_transfer()) {
            ret = -3;
            goto retry;
        }

        ret = 0;
        break;
    }
    led(false);

    mutex_unlock(&ata_mtx);

    if ( delayed_write )
        ata_flush();

    last_disk_activity = current_tick;

    return ret;
}

int ata_write_sectors(unsigned long start,
                      int count,
                      void* buf)
{
    int i;
    int ret = 0;

    last_disk_activity = current_tick;

    if (start == 0)
        panicf("Writing on sector 0\n");

    mutex_lock(&ata_mtx);
    
    if ( sleeping ) {
        if (poweroff) {
            if (ata_power_on()) {
                mutex_unlock(&ata_mtx);
                return -1;
            }
        }
        else {
            if (perform_soft_reset()) {
                mutex_unlock(&ata_mtx);
                return -1;
            }
        }
        sleeping = false;
        poweroff = false;
        ata_spinup_time = current_tick - last_disk_activity;
    }
    
    ATA_SELECT = ata_device;
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        return -2;
    }

    led(true);

    if ( count == 256 )
        ATA_NSECTOR = 0; /* 0 means 256 sectors */
    else
        ATA_NSECTOR = (unsigned char)count;
    ATA_SECTOR  = start & 0xff;
    ATA_LCYL    = (start >> 8) & 0xff;
    ATA_HCYL    = (start >> 16) & 0xff;
    ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA | ata_device;
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

    if(!wait_for_end_of_transfer())
        ret = -3;

    led(false);

    mutex_unlock(&ata_mtx);

    if ( delayed_write )
        ata_flush();

    last_disk_activity = current_tick;

    return ret;
}

extern void ata_delayed_write(unsigned long sector, void* buf)
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
    ATA_SELECT = ata_device;

    if (!wait_for_rdy())
        return -1;

    ATA_COMMAND = CMD_SECURITY_FREEZE_LOCK;

    if (!wait_for_rdy())
        return -2;

    return 0;
}

void ata_spindown(int seconds)
{
    sleep_timeout = seconds * HZ;
}

#ifdef HAVE_ATA_POWER_OFF
void ata_poweroff(bool enable)
{
    if (enable)
        poweroff_timeout = 2*HZ;
    else
        poweroff_timeout = 0;
}
#endif

bool ata_disk_is_active(void)
{
    return !sleeping;
}

static int ata_perform_sleep(void)
{
    int ret = 0;

    mutex_lock(&ata_mtx);

    ATA_SELECT = ata_device;

    if(!wait_for_rdy()) {
        DEBUGF("ata_perform_sleep() - not RDY\n");
        mutex_unlock(&ata_mtx);
        return -1;
    }

    ATA_COMMAND = CMD_SLEEP;

    if (!wait_for_rdy())
    {
        DEBUGF("ata_perform_sleep() - CMD failed\n");
        ret = -2;
    }

    sleeping = true;
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
            if ( sleep_timeout &&
                 !sleeping &&
                 TIME_AFTER( current_tick, 
                             last_user_activity + sleep_timeout ) &&
                 TIME_AFTER( current_tick, 
                             last_disk_activity + sleep_timeout ) )
            {
                ata_perform_sleep();
                last_sleep = current_tick;
            }

#ifdef HAVE_ATA_POWER_OFF
            if ( sleeping && poweroff_timeout && !poweroff &&
                 TIME_AFTER( current_tick, last_sleep + poweroff_timeout ))
            {
                mutex_lock(&ata_mtx);
                ide_power_enable(false);
                mutex_unlock(&ata_mtx);
                poweroff = true;
            }
#endif

            sleep(HZ/4);
        }
        queue_wait(&ata_queue, &ev);
        switch ( ev.id ) {
            case SYS_USB_CONNECTED:
                if (poweroff) {
                    mutex_lock(&ata_mtx);
                    led(true);
                    ata_power_on();
                    led(false);
                    mutex_unlock(&ata_mtx);
                }

                /* Tell the USB thread that we are safe */
                DEBUGF("ata_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&ata_queue);
                break;

            case Q_SLEEP:
                last_disk_activity = current_tick - sleep_timeout + (HZ/2);
                break;
        }
    }
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
    int ret;
    
    /* state HRR0 */
    PADR &= ~0x0200; /* assert _RESET */
    sleep(1); /* > 25us */

    /* state HRR1 */
    PADR |= 0x0200; /* negate _RESET */
    sleep(1); /* > 2ms */

    /* state HRR2 */
    ATA_SELECT = ata_device; /* select the right device */
    ret = wait_for_bsy();

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    sleeping = false;
    return ret;
}

static int perform_soft_reset(void)
{
    int ret;
    int retry_count;
    
    ATA_SELECT = SELECT_LBA | ata_device;
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

    sleeping = false;
    return ret;
}

int ata_soft_reset(void)
{
    int ret;
    
    mutex_lock(&ata_mtx);

    ret = perform_soft_reset();

    mutex_unlock(&ata_mtx);
    return ret;
}

static int ata_power_on(void)
{
    ide_power_enable(true);
    if( ata_hard_reset() )
        return -1;

    if (set_multiple_mode(multisectors))
        return -2;

    if (freeze_lock())
        return -3;

    sleeping = false;
    poweroff = false;
    return 0;
}

static int master_slave_detect(void)
{
    /* master? */
    ATA_SELECT = 0;
    if ( ATA_STATUS & STATUS_RDY ) {
        ata_device = 0;
        DEBUGF("Found master harddisk\n");
    }
    else {
        /* slave? */
        ATA_SELECT = SELECT_DEVICE1;
        if ( ATA_STATUS & STATUS_RDY ) {
            ata_device = SELECT_DEVICE1;
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
        ata_io_address = 0x300; /* For debug purposes only */
        old_recorder = true;
        ata_control = ATA_CONTROL2;
    }
    else
    {
        DEBUGF("CONTROL is at 0x206\n");
        ata_io_address = 0x200; /* For debug purposes only */
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

static int identify(void)
{
    int i;

    ATA_SELECT = ata_device;

    if(!wait_for_rdy()) {
        DEBUGF("identify() - not RDY\n");
        return -1;
    }

    ATA_COMMAND = CMD_IDENTIFY;

    if (!wait_for_start_of_transfer())
    {
        DEBUGF("identify() - CMD failed\n");
        return -2;
    }

    for (i=0; i<SECTOR_SIZE/2; i++)
        /* the IDENTIFY words are already swapped */
        identify_info[i] = ATA_DATA;
    
    return 0;
}

static int set_multiple_mode(int sectors)
{
    ATA_SELECT = ata_device;

    if(!wait_for_rdy()) {
        DEBUGF("set_multiple_mode() - not RDY\n");
        return -1;
    }

    ATA_NSECTOR = sectors;
    ATA_COMMAND = CMD_SET_MULTIPLE_MODE;

    if (!wait_for_rdy())
    {
        DEBUGF("set_multiple_mode() - CMD failed\n");
        return -2;
    }

    return 0;
}

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

int ata_init(void)
{
    mutex_init(&ata_mtx);

    led(false);

    ata_enable(true);

    if ( !initialized ) {
        if (master_slave_detect())
            return -1;
        
        if (io_address_detect())
            return -2;
        
        if (check_registers())
            return -3;
        
        if (freeze_lock())
            return -4;

        if (identify())
            return -5;
        multisectors = identify_info[47] & 0xff;
        DEBUGF("ata: %d sectors per ata request\n",multisectors);
        
        queue_init(&ata_queue);
        create_thread(ata_thread, ata_stack,
                      sizeof(ata_stack), ata_thread_name);
        initialized = true;
    }
    if (set_multiple_mode(multisectors))
        return -6;

    return 0;
}
