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
#include <stdarg.h>
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

/* Size of the buffer to store the loaded firmware image */
#define MAX_LOADSIZE (10*1024*1024)

/* A buffer to load the iriver firmware or Rockbox into */
unsigned char loadbuffer[MAX_LOADSIZE];

char version[] = APPSVERSION;

#define DRAM_START              0x10000000

int line=0;
char printfbuf[256];

void reset_screen(void)
{
    lcd_clear_display();
    line = 0;
}

void printf(const char *format, ...)
{
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    
    ptr = printfbuf;
    len = vsnprintf(ptr, sizeof(printfbuf), format, ap);
    va_end(ap);

    lcd_puts(0, line++, ptr);
    lcd_update();
    if(line >= (LCD_HEIGHT/SYSFONT_HEIGHT))
        line = 0;
}

/* Load original mi4 firmware. This function expects a file called
   "/System/OF.bin" on the player. It should be a mi4 firmware decrypted 
   and header stripped using mi4code. It reads the file in to a memory
   buffer called buf. The rest of the loading is done in main() and crt0.S
*/
int load_original_firmware(unsigned char* buf)
{
    int fd;
    int rc;
    int len;
    
    fd = open("/System/OF.bin", O_RDONLY);

    len = filesize(fd);
    
    if (len > MAX_LOADSIZE)
        return -6;

    rc = read(fd, buf, len);
    if(rc < len)
        return -4;

    close(fd);
    return len;
}

/* Load Rockbox firmware (rockbox.*) */
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

    printf("Length: %x", len);
    
    if (len > MAX_LOADSIZE)
        return -6;

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    
    rc = read(fd, &chksum, 4);
    chksum=betoh32(chksum); /* Rockbox checksums are big-endian */
    if(rc < 4)
        return -2;

    printf("Checksum: %x", chksum);

    rc = read(fd, model, 4);
    if(rc < 4)
        return -3;

    model[4] = 0;
    
    printf("Model name: %s", model);

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    rc = read(fd, buf, len);
    if(rc < len)
        return -4;

    close(fd);

    sum = MODEL_NUMBER;
    
    for(i = 0;i < len;i++) {
        sum += buf[i];
    }

    printf("Sum: %x", sum);
    
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
    button_init();

    line=0;

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version: 20%s", version);
    printf(MODEL_NAME);

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
      printf(buf);
    } else {
      printf("ATA: %d", i);
    }

    disk_init();
    rc = disk_mount_all();
    if (rc<=0)
    {
        printf("No partition found");
    }

    pinfo = disk_partinfo(0);
    printf("Partition 0: 0x%02x %ld MB", pinfo->type, pinfo->size / 2048);

    i=button_read_device();
    if(i==BUTTON_LEFT)
    {
        printf("Loading original firmware...");
        rc=load_original_firmware(loadbuffer);
    } else {
        printf("Loading Rockbox...");
        rc=load_rockbox(loadbuffer);
    }

    if (rc < 0) {
            printf("Rockbox error: %d",rc);
            while(1) {}
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
