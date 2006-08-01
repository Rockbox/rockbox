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
 * Based on ipod.c by Dave Chapman
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
#include "panic.h"
#include "power.h"
#include "file.h"

#define XSC(X) #X
#define SC(X) XSC(X)

#define DRAM_START              0x10000000

#define BUTTON_LEFT  1
#define BUTTON_MENU  2
#define BUTTON_RIGHT 3
#define BUTTON_PLAY  4
#define BUTTON_HOLD  5

/* Size of the buffer to store the loaded Rockbox/Linux image */
#define MAX_LOADSIZE (4*1024*1024)

char version[] = APPSVERSION;

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

#if CONFIG_KEYPAD == IPOD_4G_PAD && !defined(IPOD_MINI)
/* check if number of seconds has past */
int timer_check(int clock_start, unsigned int usecs)
{
    if ((USEC_TIMER - clock_start) >= usecs) {
        return 1;
    } else {
        return 0;
    }
}

static void ser_opto_keypad_cfg(int val)
{
    int start_time;

    outl(inl(0x6000d004) & ~0x80, 0x6000d004);

    outl(inl(0x7000c104) | 0xc000000, 0x7000c104);
    outl(val, 0x7000c120);
    outl(inl(0x7000c100) | 0x80000000, 0x7000c100);

    outl(inl(0x6000d024) & ~0x10, 0x6000d024);
    outl(inl(0x6000d014) | 0x10, 0x6000d014);

    start_time = USEC_TIMER;
    do {
        if ((inl(0x7000c104) & 0x80000000) == 0) {
            break;
        }
    } while (timer_check(start_time, 1500) != 0);

    outl(inl(0x7000c100) & ~0x80000000, 0x7000c100);

    outl(inl(0x6000d004) | 0x80, 0x6000d004);
    outl(inl(0x6000d024) | 0x10, 0x6000d024);
    outl(inl(0x6000d014) & ~0x10, 0x6000d014);

    outl(inl(0x7000c104) | 0xc000000, 0x7000c104);
    outl(inl(0x7000c100) | 0x60000000, 0x7000c100);
}

int opto_keypad_read(void)
{
    int loop_cnt, had_io = 0;

    for (loop_cnt = 5; loop_cnt != 0;)
    {
        int key_pressed = 0;
        int start_time;
        unsigned int key_pad_val;

        ser_opto_keypad_cfg(0x8000023a);

        start_time = USEC_TIMER;
        do {
            if (inl(0x7000c104) & 0x4000000) {
                had_io = 1;
                break;
            }

            if (had_io != 0) {
                break;
            }
        } while (timer_check(start_time, 1500) != 0);

        key_pad_val = inl(0x7000c140);
        if ((key_pad_val & ~0x7fff0000) != 0x8000023a) {
            loop_cnt--;
        } else {
            key_pad_val = (key_pad_val << 11) >> 27;
            key_pressed = 1;
        }

        outl(inl(0x7000c100) | 0x60000000, 0x7000c100);
        outl(inl(0x7000c104) | 0xc000000, 0x7000c104);

        if (key_pressed != 0) {
            return key_pad_val ^ 0x1f;
        }
    }

    return 0;
}
#endif

static int key_pressed(void)
{
    unsigned char state;

#if CONFIG_KEYPAD == IPOD_4G_PAD
#ifdef IPOD_MINI /* mini 1G only */
    state = GPIOA_INPUT_VAL & 0x3f;
    if ((state & 0x10) == 0) return BUTTON_LEFT;
    if ((state & 0x2) == 0) return BUTTON_MENU;
    if ((state & 0x4) == 0) return BUTTON_PLAY;
    if ((state & 0x8) == 0) return BUTTON_RIGHT;
#else
    state = opto_keypad_read();
    if ((state & 0x4) == 0) return BUTTON_LEFT;
    if ((state & 0x10) == 0) return BUTTON_MENU;
    if ((state & 0x8) == 0) return BUTTON_PLAY;
    if ((state & 0x2) == 0) return BUTTON_RIGHT;
#endif
#elif CONFIG_KEYPAD == IPOD_3G_PAD
    state = inb(0xcf000030);
    if (((state & 0x20) == 0)) return BUTTON_HOLD; /* hold on */
    if ((state & 0x08) == 0) return BUTTON_LEFT;
    if ((state & 0x10) == 0) return BUTTON_MENU;
    if ((state & 0x04) == 0) return BUTTON_PLAY;
    if ((state & 0x01) == 0) return BUTTON_RIGHT;
#endif
    return 0;
}

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

    if (len > MAX_LOADSIZE)
        return -6;

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    
    rc = read(fd, &chksum, 4);
    chksum=betoh32(chksum); /* Rockbox checksums are big-endian */
    if(rc < 4)
        return -2;

    rc = read(fd, model, 4);
    if(rc < 4)
        return -3;

    model[4] = 0;
    
    snprintf(str, 80, "Model: %s", model);
    lcd_puts(0, line++, str);
    snprintf(str, 80, "Checksum: %x", chksum);
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

    return len;
}


/* A buffer to load the Linux kernel or Rockbox into */
unsigned char loadbuffer[MAX_LOADSIZE];

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

    /* set port B03 on */
    outl(((0x100 | 1) << 3), 0x6000d824);

#elif CONFIG_BACKLIGHT==BL_IPODMINI
    /* set port B03 on */
    outl(((0x100 | 1) << 3), 0x6000d824);

#elif CONFIG_BACKLIGHT==BL_IPODNANO

    /* set port B03 on */
    outl(((0x100 | 1) << 3), 0x6000d824);

    /* set port L07 on */
    outl(((0x100 | 1) << 7), 0x6000d12c);
#elif CONFIG_BACKLIGHT==BL_IPOD3G
    outl(inl(IPOD_LCD_BASE) | 0x2, IPOD_LCD_BASE);
#endif

    /* Return the start address in loaded image */
    return entry;
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

void system_reboot(void)
{

}
