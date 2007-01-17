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
#include "panic.h"
#include "power.h"
#include "file.h"

#define XSC(X) #X
#define SC(X) XSC(X)

#if (CONFIG_CPU == PP5020)
#define DRAM_START              0x10000000
#else
#define IPOD_LCD_BASE           0xc0001000
#define DRAM_START              0x28000000
#endif
#define IPOD_HW_REVISION (*((volatile unsigned long*)(0x00002084)))

/* We copy the hardware revision to the last four bytes of SDRAM and then
   re-read it after we have re-mapped SDRAM to 0x0 in Rockbox */
#define TMP_IPOD_HW_REVISION (*((volatile unsigned long*)(0x11fffffc)))

#define BUTTON_LEFT  1
#define BUTTON_MENU  2
#define BUTTON_RIGHT 3
#define BUTTON_PLAY  4
#define BUTTON_HOLD  5

/* Size of the buffer to store the loaded Rockbox/Linux/AppleOS image */

/* The largest known current (December 2006) firmware is about 7.5MB
   (Apple's firmware for the ipod video) so we set this to 8MB. */

#define MAX_LOADSIZE (8*1024*1024)

char version[] = APPSVERSION;

int line=0;

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

/* This function is the same on all ipods */
bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x20)?false:true;
}

int load_rockbox(unsigned char* buf, char* firmware)
{
    int fd;
    int rc;
    int len;
    unsigned long chksum;
    char model[5];
    unsigned long sum;
    int i;
    char filename[MAX_PATH];

    snprintf(filename,sizeof(filename),"/.rockbox/%s",firmware);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        snprintf(filename,sizeof(filename),"/%s",firmware);
        fd = open(filename, O_RDONLY);
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
    
    printf("Model: %s", model);
    printf("Checksum: %x", chksum);
    printf("Loading %s", firmware);

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


int load_linux(unsigned char* buf) {
    int fd;
    int rc;
    int len;

    fd=open("/linux.bin",O_RDONLY);
    if (fd < 0)
        return -1;

    len=filesize(fd);
    if (len > MAX_LOADSIZE)
        return -6;

    rc=read(fd,buf,len);

    if (rc < len)
        return -4;

    printf("Loaded Linux: %d bytes", len);

    return len;
}


/* A buffer to load the Linux kernel or Rockbox into */
unsigned char loadbuffer[MAX_LOADSIZE];

void fatal_error(void)
{
    bool holdstatus=false;

    /* System font is 6 pixels wide */
#if LCD_WIDTH >= (30*6)
    printf("Press MENU+SELECT to reboot");
    printf("then SELECT+PLAY for disk mode");
#else
    printf("Press MENU+SELECT to");
    printf("reboot then SELECT+PLAY");
    printf("for disk mode");
#endif

    while (1) {
        if (button_hold() != holdstatus) {
            if (button_hold()) {
                holdstatus=true;
                lcd_puts(0, line, "Hold switch on!");
            } else {
                holdstatus=false;
                lcd_puts(0, line, "               ");
            }
            lcd_update();
        }
        udelay(100000); /* 100ms */
    }

}


void* main(void)
{
    char buf[256];
    int i;
    int rc;
    bool haveretailos;
    bool button_was_held;
    struct partinfo* pinfo;
    unsigned short* identify_info;

    /* Check the button hold status as soon as possible - to 
       give the user maximum chance to turn it off in order to
       reset the settings in rockbox. */
    button_was_held = button_hold();

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

    TMP_IPOD_HW_REVISION = IPOD_HW_REVISION;
    ipod_hw_rev = IPOD_HW_REVISION;

    system_init();
    kernel_init();
    lcd_init();
    font_init();

#ifdef HAVE_LCD_COLOR
    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();
#endif

#if 0
    /* ADC and button drivers are not yet implemented */
    adc_init();
    button_init();
#endif

    line=0;

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version: 20%s", version);
    printf("IPOD version: 0x%08x", IPOD_HW_REVISION);

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
        fatal_error();
    }

    pinfo = disk_partinfo(1);
    printf("Partition 1: 0x%02x %ld MB", 
           pinfo->type, pinfo->size / 2048);

    /* See if there is an Apple firmware image in RAM */
    haveretailos = (memcmp((void*)(DRAM_START+0x20),"portalplayer",12)==0);

    /* We don't load Rockbox if the hold button is enabled. */
    if (!button_was_held) {
        /* Check for a keypress */
        i=key_pressed();

        if ((i!=BUTTON_MENU) && (i!=BUTTON_PLAY)) {
            printf("Loading Rockbox...");
            rc=load_rockbox(loadbuffer, BOOTFILE);
            if (rc < 0) {
                printf("Rockbox error: %d",rc);
            } else {
                printf("Rockbox loaded.");
                memcpy((void*)DRAM_START,loadbuffer,rc);
                return (void*)DRAM_START;
            }
        }

        if (i==BUTTON_PLAY) {
            printf("Loading Linux...");
            rc=load_linux(loadbuffer);
            if (rc < 0) {
                printf("Linux error: %d",rc);
            } else {
                memcpy((void*)DRAM_START,loadbuffer,rc);
                return (void*)DRAM_START;
            }
        }
    }


    /* If either the hold switch was on, or loading Rockbox/IPL
       failed, then try the Apple firmware */

    printf("Loading original firmware...");

    /* First try an apple_os.ipod file on the FAT32 partition
       (either in .rockbox or the root) 
     */

    rc=load_rockbox(loadbuffer, "apple_os.ipod");

    /* Only report errors if the file was found */
    if (rc < -1) {
        printf("apple_os.ipod error: %d",rc);
    } else if (rc > 0) {
        printf("apple_os.ipod loaded.");
        memcpy((void*)DRAM_START,loadbuffer,rc);
        return (void*)DRAM_START;
    }

    if (haveretailos) {
        /* We have a copy of the retailos in RAM, lets just run it. */
        return (void*)DRAM_START;
    }

    /* Everything failed - just loop forever */
    printf("No RetailOS detected");

    fatal_error();

    /* We never get here, but keep gcc happy */
    return (void*)0;
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
