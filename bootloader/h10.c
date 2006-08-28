/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
 * 
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"

/* Size of the buffer to store the loaded Rockbox/iriver image */
#define MAX_LOADSIZE (5*1024*1024)

/* A buffer to load the iriver firmware or Rockbox into */
unsigned char loadbuffer[MAX_LOADSIZE];

char version[] = APPSVERSION;

#define DRAM_START              0x10000000

int line=0;

/* Load original iriver firmware. This function expects a file called
   "/System/Original.mi4" on the player. It should be decrypted
   and have the header stripped using mi4code. It reads the file in to a memory
   buffer called buf. The rest of the loading is done in main() and crt0.S
*/
int load_iriver(unsigned char* buf)
{
    int fd;
    int rc;
    int len;
    
    fd = open("/System/Original.mi4", O_RDONLY);

    len = filesize(fd);
    
    if (len > MAX_LOADSIZE)
        return -6;

    rc = read(fd, buf, len);
    if(rc < len)
        return -4;

    close(fd);
    return len;
}

/* Load Rockbox firmware (rockbox.h10) */
int load_rockbox(unsigned char* buf)
{
    int fd;
    int rc;
    int len;
    unsigned long chksum;
    char model[5];
    unsigned long sum;
    int i;
    char str[80];
    
    fd = open("/.rockbox/" BOOTFILE, O_RDONLY);
    if(fd < 0)
    {
        fd = open("/" BOOTFILE, O_RDONLY);
        if(fd < 0)
            return -1;
    }

    len = filesize(fd) - 8;

    snprintf(str, sizeof(str), "Length: %x", len);
    lcd_puts(0, line++ ,str);
    lcd_update();
    
    if (len > MAX_LOADSIZE)
        return -6;

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    
    rc = read(fd, &chksum, 4);
    chksum=betoh32(chksum); /* Rockbox checksums are big-endian */
    if(rc < 4)
        return -2;

    snprintf(str, sizeof(str), "Checksum: %x", chksum);
    lcd_puts(0, line++ ,str);
    lcd_update();

    rc = read(fd, model, 4);
    if(rc < 4)
        return -3;

    model[4] = 0;
    
    snprintf(str, sizeof(str), "Model name: %s", model);
    lcd_puts(0, line++ ,str);
    lcd_update();

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    rc = read(fd, buf, len);
    if(rc < len)
        return -4;

    close(fd);

    sum = MODEL_NUMBER;
    
    for(i = 0;i < len;i++) {
        sum += buf[i];
    }

    snprintf(str, sizeof(str), "Sum: %x", sum);
    lcd_puts(0, line++ ,str);
    lcd_update();

    if(sum != chksum)
        return -5;

    return len;
}

void* main(void)
{
    char buf[256];
    int i;
    int rc;
    unsigned short* identify_info;
    struct partinfo* pinfo;

    system_init();
    kernel_init();
    lcd_init();
    font_init();

    line=0;

    lcd_setfont(FONT_SYSFIXED);

    lcd_puts(0, line++, "Rockbox boot loader");
    snprintf(buf, sizeof(buf), "Version: 20%s", version);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), "iriver H10");
    lcd_puts(0, line++, buf);
    lcd_update();

    i=ata_init();
    if (i==0) {
      identify_info=ata_get_identify();
      /* Show model */
      for (i=0; i < 20; i++) {
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);
      }
      buf[40]=0;
      for (i=39; i && buf[i]==' '; i--) {
        buf[i]=0;
      }
      lcd_puts(0, line++, buf);
      lcd_update();
    } else {
      snprintf(buf, sizeof(buf), "ATA: %d", i);
      lcd_puts(0, line++, buf);
      lcd_update();
    }

    disk_init();
    rc = disk_mount_all();
    if (rc<=0)
    {
        lcd_puts(0, line++, "No partition found");
        lcd_update();
    }

    pinfo = disk_partinfo(0);
    snprintf(buf, sizeof(buf), "Partition 0: 0x%02x %ld MB", 
                  pinfo->type, pinfo->size / 2048);
    lcd_puts(0, line++, buf);
    lcd_update();

    i=button_read_device();
    if(i==BUTTON_LEFT)
    {
        lcd_puts(0, line, "Loading iriver firmware...");
        lcd_update();
        rc=load_iriver(loadbuffer);
    } else {
        lcd_puts(0, line, "Loading Rockbox...");
        lcd_update();
        rc=load_rockbox(loadbuffer);
    }

    if (rc < 0) {
            snprintf(buf, sizeof(buf), "Rockbox error: %d",rc);
            lcd_puts(0, line++, buf);
            lcd_update();
    }
    
    memcpy((void*)DRAM_START,loadbuffer,rc);
    
    return (void*)DRAM_START;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void reset_poweroff_timer(void)
{
}

int dbg_ports(void)
{
   return 0;
}

void mpeg_stop(void)
{
}

void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}

void sys_poweroff(void)
{
}
