/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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

#define DRAM_START             0x10000000
#define IPOD_PP5020_RTC        0x60005010

#define IPOD_HW_REVISION (*((volatile unsigned long*)(0x00002084)))

char version[] = APPSVERSION;

#include "rockbox-16bit.h"
#include "ipodlinux-16bit.h"

typedef struct _image {
    unsigned type;              /* '' */
    unsigned id;                /* */
    unsigned pad1;              /* 0000 0000 */
    unsigned devOffset;         /* byte offset of start of image code */
    unsigned len;               /* length in bytes of image */
    void    *addr;              /* load address */
    unsigned entryOffset;       /* execution start within image */
    unsigned chksum;            /* checksum for image */
    unsigned vers;              /* image version */
    unsigned loadAddr;          /* load address for image */
} image_t;

extern image_t boot_table[];

int line=0;

static void memmove16(void *dest, const void *src, unsigned count)
{
    struct bufstr {
        unsigned _buf[4];
    } *d, *s;

    if (src >= dest) {
        count = (count + 15) >> 4;
        d = (struct bufstr *) dest;
        s = (struct bufstr *) src;
        while (count--)
            *d++ = *s++;
    } else {
        count = (count + 15) >> 4;
        d = (struct bufstr *)(dest + (count <<4));
        s = (struct bufstr *)(src + (count <<4));
        while (count--)
            *--d = *--s;
    }
}

/* get current usec counter */
int timer_get_current(void)
{
    return inl(IPOD_PP5020_RTC);
}

/* check if number of seconds has past */
int timer_check(int clock_start, unsigned int usecs)
{
    if ((inl(IPOD_PP5020_RTC) - clock_start) >= usecs) {
        return 1;
    } else {
        return 0;
    }
}

/* This isn't a sleep, but let's call it that. */
int usleep(unsigned int usecs)
{
    unsigned int start = inl(IPOD_PP5020_RTC);

    while ((inl(IPOD_PP5020_RTC) - start) < usecs) {
       // empty
    }

    return 0;
}

int load_firmware(void)
{
    int fd;
    int rc;
    int len;
    unsigned long chksum;
    char model[5];
    unsigned long sum;
    int i;
    unsigned char *buf = (unsigned char *)DRAM_START;
    char str[80];
    
    fd = open("/rockbox.ipod", O_RDONLY);
    if(fd < 0)
    return -1;

    len = filesize(fd) - 8;

    snprintf(str, 80, "Length: %x", len);
    lcd_puts(0, line++, str);
    lcd_update();

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    
    rc = read(fd, &chksum, 4);
    if(rc < 4)
    return -2;

    snprintf(str, 80, "Checksum: %x", chksum);
    lcd_puts(0, line++, str);
    lcd_update();

    rc = read(fd, model, 4);
    if(rc < 4)
    return -3;

    model[4] = 0;
    
    snprintf(str, 80, "Model name: %s", model);
    lcd_puts(0, line++, str);
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

    snprintf(str, 80, "Sum: %x", sum);
    lcd_puts(0, line++, str);
    lcd_update();

    if(sum != chksum)
    return -5;

    return 0;
}

void* main(void)
{
    char buf[256];
    int imageno=0;
    int i;
    int rc;
    int padding = 0x4400;
    image_t *tblp = boot_table;
    void* entry;
    struct partinfo* pinfo;
    unsigned short* identify_info;

    /* Turn on the backlight */

#if CONFIG_BACKLIGHT==BL_IPOD4G

    /* brightness full */
    outl(0x80000000 | (0xff << 16), 0x7000a010);

    /* set port B03 on
    outl(((0x100 | 1) << 3), 0x6000d824);

#elif CONFIG_BACKLIGHT==BL_IPODNANO

    /* set port B03 on */
    outl(((0x100 | 1) << 3), 0x6000d824);

    /* set port L07 on */
    outl(((0x100 | 1) << 7), 0x6000d12c);

#endif

    system_init();
    kernel_init();
    lcd_init();
    font_init();

#if 0
    /* ADC and button drivers are not yet implemented */
    adc_init();
    button_init();
#endif

    /* Notes: iPod Color/Photo LCD is 220x176, Nano is 176x138 */

    /* Display the 42x47 pixel iPodLinux logo */
    lcd_bitmap((unsigned char*)ipllogo, 20,6, 42,47);

    /* Display the 100x31 pixel Rockbox logo */
    lcd_bitmap((unsigned char*)rockboxlogo, 74,16, 100,31);

    line=7;

    lcd_setfont(FONT_SYSFIXED);

    lcd_puts(0, line++, "iPodLinux/Rockbox boot loader");
    snprintf(buf, sizeof(buf), "Version: 20%s", version);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), "IPOD version: 0x%08x", IPOD_HW_REVISION);
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
//        while(button_get(true) != SYS_USB_CONNECTED) {};
    }

    pinfo = disk_partinfo(1);
    snprintf(buf, sizeof(buf), "Partition 1: 0x%02x %ld MB", 
                  pinfo->type, pinfo->size / 2048);
    lcd_puts(0, line++, buf);
    lcd_update();

#if 0
    /* The following code will load and run an ipodlinux kernel - we will
       enable it once the button driver is written and we can detect key
       presses */
    int fd=open("/linux.bin",O_RDONLY);
    if (fd >= 0) {
      i=filesize(fd);
      int n=read(fd,(void*)DRAM_START,i);
      if (n==i) {
          /* We return the entry point for the loaded kernel */
          return DRAM_START;
      } else {
          /* What do we do now?  We may have overwritten the copy of the 
             original firmware with our incomplete copy of the Linux 
             kernel... */
      }
    }
#endif

    /* Pause for 5 seconds so we can see what's happened*/
    usleep(5000000);

    /* If everything else failed, try the original firmware */
    lcd_puts(0, line, "Loading original firmware...");
    lcd_update();

    entry = tblp->addr + tblp->entryOffset;
    if (imageno || ((int)tblp->addr & 0xffffff) != 0) {
        memmove16(tblp->addr, tblp->addr + tblp->devOffset - padding,
                  tblp->len);
    }

    /* Return the start address in loaded image */
    return entry;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void reset_poweroff_timer(void)
{
}

void screen_dump(void)
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
