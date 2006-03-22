/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "usb.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "uda1380.h"

#include "pcf50606.h"

#include <stdarg.h>

#define DRAM_START 0x31000000

int line = 0;

int usb_screen(void)
{
   return 0;
}

char version[] = APPSVERSION;

char printfbuf[256];

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
    if(line >= 16)
        line = 0;
}

void start_iriver_fw(void)
{
    asm(" move.w #0x2700,%sr");
    /* Reset the cookie for the crt0 crash check */
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" lea.l 8,%a0");
    asm(" jmp (%a0)");
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
    
    fd = open("/.rockbox/" BOOTFILE, O_RDONLY);
    if(fd < 0)
    {
        fd = open("/" BOOTFILE, O_RDONLY);
        if(fd < 0)
            return -1;
    }

    len = filesize(fd) - 8;

    printf("Length: %x", len);
    lcd_update();

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    
    rc = read(fd, &chksum, 4);
    if(rc < 4)
        return -2;

    printf("Checksum: %x", chksum);
    lcd_update();

    rc = read(fd, model, 4);
    if(rc < 4)
        return -3;

    model[4] = 0;
    
    printf("Model name: %s", model);
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

    printf("Sum: %x", sum);
    lcd_update();

    if(sum != chksum)
        return -5;

    return 0;
}


void start_firmware(void)
{
    asm(" move.w #0x2700,%sr");
    /* Reset the cookie for the crt0 crash check */
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
    asm(" move.l %0,%%d0" :: "i"(DRAM_START));
    asm(" movec.l %d0,%vbr");
    asm(" move.l %0,%%sp" :: "m"(*(int *)DRAM_START));
    asm(" move.l %0,%%a0" :: "m"(*(int *)(DRAM_START+4)));
    asm(" jmp (%a0)");
}

void main(void)
{
    int i;
    int rc;
    bool rc_on_button = false;
    bool on_button = false;
    int data;
    int adc_battery, battery_voltage, batt_int, batt_frac;

#ifdef IAUDIO_X5
    (void)rc_on_button;
    (void)on_button;
    (void)data;
    power_init();

    system_init();
    kernel_init();

    set_cpu_frequency(CPUFREQ_NORMAL);

    set_irq_level(0);
    lcd_init();
    font_init();
    adc_init();
    button_init();

    printf("Rockbox boot loader");
    printf("Version %s", version);
    lcd_update();

    adc_battery = adc_read(ADC_BATTERY);

    battery_voltage = (adc_battery * BATTERY_SCALE_FACTOR) / 10000;
    batt_int = battery_voltage / 100;
    batt_frac = battery_voltage % 100;
    
    printf("Batt: %d.%02dV", batt_int, batt_frac);
    lcd_update();

    rc = ata_init();
    if(rc)
    {
        printf("ATA error: %d", rc);
        sleep(HZ*5);
        power_off();
    }

    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        printf("No partition found");
        sleep(HZ*5);
        power_off();
    }

    printf("Loading firmware");
    lcd_update();
    i = load_firmware();
    printf("Result: %d", i);
    lcd_update();
    
    if(i == 0)
        start_firmware();

    power_off();
    
#else
    /* We want to read the buttons as early as possible, before the user
       releases the ON button */

    /* Set GPIO33, GPIO37, GPIO38  and GPIO52 as general purpose inputs
       (The ON and Hold buttons on the main unit and the remote) */
    or_l(0x00100062, &GPIO1_FUNCTION);
    and_l(~0x00100062, &GPIO1_ENABLE);

    data = GPIO1_READ;
    if ((data & 0x20) == 0)
        on_button = true;
    
    if ((data & 0x40) == 0)
        rc_on_button = true;

    /* Set the default state of the hard drive power to OFF */
    ide_power_enable(false);
    
    power_init();

    /* Turn off if neither ON button is pressed */
    if(!(on_button || rc_on_button || usb_detect()))
        power_off();
    
    /* Backlight ON */
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
    and_l(~0x00020000, &GPIO1_OUT);

    /* Remote backlight ON */
#ifdef HAVE_REMOTE_LCD
#ifdef IRIVER_H300_SERIES
    or_l(0x00000002, &GPIO1_ENABLE);
    and_l(~0x00000002, &GPIO1_OUT);
#else
    or_l(0x00000800, &GPIO_ENABLE);
    or_l(0x00000800, &GPIO_FUNCTION);
    and_l(~0x00000800, &GPIO_OUT);
#endif
#endif

    /* Power on the hard drive early, to speed up the loading.
       Some H300 don't like this, so we only do it for the H100 */
#ifndef IRIVER_H300_SERIES
    if(!((on_button && button_hold()) ||
         (rc_on_button && remote_button_hold()))) {
        ide_power_enable(true);
    }
#endif
    
    system_init();
    kernel_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* Set up waitstates for the peripherals */
    set_cpu_frequency(0); /* PLL off */
#endif

#ifdef HAVE_UDA1380
    uda1380_reset();
#endif
    
    backlight_init();
    set_irq_level(0);
    lcd_init();
    font_init();
    adc_init();
    button_init();

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version %s", version);
    lcd_update();

    sleep(HZ/50); /* Allow the button driver to check the buttons */

    /* Holding REC while starting runs the original firmware */
    if(((button_status() & BUTTON_REC) == BUTTON_REC) ||
       ((button_status() & BUTTON_RC_REC) == BUTTON_RC_REC)) {
        printf("Starting original firmware...");
        lcd_update();
        start_iriver_fw();
    }

    /* Don't start if the Hold button is active on the device you
       are starting with */
    if(!usb_detect() && ((on_button && button_hold()) ||
       (rc_on_button && remote_button_hold()))) {
        printf("HOLD switch on, power off...");
        lcd_update();
        sleep(HZ*2);

        /* Backlight OFF */
#ifdef HAVE_REMOTE_LCD
#ifdef IRIVER_H300_SERIES
        or_l(0x00000002, &GPIO1_OUT);
#else
        or_l(0x00000800, &GPIO_OUT);
#endif
#endif
        /* Reset the cookie for the crt0 crash check */
        asm(" move.l #0,%d0");
        asm(" move.l %d0,0x10017ffc");
        power_off();
    }

    usb_init();
    
    adc_battery = adc_read(ADC_BATTERY);

    battery_voltage = (adc_battery * BATTERY_SCALE_FACTOR) / 10000;
    batt_int = battery_voltage / 100;
    batt_frac = battery_voltage % 100;
    
    printf("Batt: %d.%02dV", batt_int, batt_frac);
    lcd_update();

    if(battery_voltage <= 300) {
        printf("WARNING! BATTERY LOW!!");
        lcd_update();
        sleep(HZ*2);
    }
    
    rc = ata_init();
    if(rc)
    {
        lcd_clear_display();
        printf("ATA error: %d", rc);
        printf("Insert USB cable and press");
        printf("a button");
        lcd_update();
        while(!(button_get(true) & BUTTON_REL));
    }

    /* A hack to enter USB mode without using the USB thread */
    if(usb_detect())
    {
        const char msg[] = "Bootloader USB mode";
        int w, h;
        font_getstringsize(msg, &w, &h, FONT_SYSFIXED);
        lcd_clear_display();
        lcd_putsxy((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, msg);
        lcd_update();

#ifdef IRIVER_H300_SERIES
        sleep(HZ);
#endif
    
        ata_spin();
        ata_enable(false);
        usb_enable(true);
        cpu_idle_mode(true);
        while(usb_detect())
        {
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);
            
            /* Backlight OFF */
            or_l(0x00020000, &GPIO1_OUT);
        }

        cpu_idle_mode(false);
        usb_enable(false);
        ata_init(); /* Reinitialize ATA and continue booting */
        
        lcd_clear_display();
        line = 0;
        lcd_update();
    }
    
    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        lcd_clear_display();
        printf("No partition found");
        lcd_update();
        while(button_get(true) != SYS_USB_CONNECTED) {};
    }

    printf("Loading firmware");
    lcd_update();
    i = load_firmware();
    printf("Result: %d", i);
    lcd_update();

    if(i == 0)
        start_firmware();
    
    start_iriver_fw();
#endif /* IAUDIO_X5 */
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

void sys_poweroff(void)
{
}
