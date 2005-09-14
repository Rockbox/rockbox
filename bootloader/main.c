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
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "uda1380.h"

#define DRAM_START 0x31000000

int line = 0;

int usb_screen(void)
{
   return 0;
}

char version[] = APPSVERSION;

static void usb_enable(bool on)
{
    and_l(~0x01000000, &GPIO_OUT);      /* GPIO24 is the Cypress chip power */
    or_l(0x01000000, &GPIO_ENABLE);
    or_l(0x01000000, &GPIO_FUNCTION);
    
    or_l(0x00000080, &GPIO1_FUNCTION); /* GPIO39 is the USB detect input */

    if(on)
    {
        /* Power on the Cypress chip */
        or_l(0x01000000, &GPIO_OUT);
        sleep(2);
    }
    else
    {
        /* Power off the Cypress chip */
        and_l(~0x01000000, &GPIO_OUT);
    }
}

bool usb_detect(void)
{
    return (GPIO1_READ & 0x80)?true:false;
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
    char str[80];
    
    fd = open("/rockbox.iriver", O_RDONLY);
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
    char buf[256];
    bool rc_on_button = false;
    bool on_button = false;
    int data;
    int adc_battery, battery_voltage, batt_int, batt_frac;

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

    /* Backlight ON */
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);

    /* Set the default state of the hard drive power to OFF */
    ide_power_enable(false);
    
    power_init();

    /* Power on the hard drive early, to speed up the loading */
    if(!((on_button && button_hold()) ||
         (rc_on_button && remote_button_hold()))) {
        ide_power_enable(true);
    }
    
    system_init();
    kernel_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* Set up waitstates for the peripherals */
    set_cpu_frequency(0); /* PLL off */
#endif

    uda1380_reset();
    
    backlight_init();
    set_irq_level(0);
    lcd_init();
    font_init();
    adc_init();
    button_init();

    lcd_setfont(FONT_SYSFIXED);

    lcd_puts(0, line++, "Rockbox boot loader");
    snprintf(buf, sizeof(buf), "Version %s", version);
    lcd_puts(0, line++, buf);
    lcd_update();

    sleep(HZ/50); /* Allow the button driver to check the buttons */

    /* Holding REC while starting runs the original firmware */
    if(((button_status() & BUTTON_REC) == BUTTON_REC) ||
       ((button_status() & BUTTON_RC_REC) == BUTTON_RC_REC)) {
        lcd_puts(0, 8, "Starting original firmware...");
        lcd_update();
        start_iriver_fw();
    }

    /* Don't start if the Hold button is active on the device you
       are starting with */
    if((on_button && button_hold()) ||
       (rc_on_button && remote_button_hold())) {
        lcd_puts(0, 8, "HOLD switch on, power off...");
        lcd_update();
        sleep(HZ*2);
        /* Reset the cookie for the crt0 crash check */
        asm(" move.l #0,%d0");
        asm(" move.l %d0,0x10017ffc");
        power_off();
    }

    adc_battery = adc_read(ADC_BATTERY);

    battery_voltage = (adc_battery * BATTERY_SCALE_FACTOR) / 10000;
    batt_int = battery_voltage / 100;
    batt_frac = battery_voltage % 100;
    
    snprintf(buf, 32, "Batt: %d.%02dV", batt_int, batt_frac);
    lcd_puts(0, line++, buf);
    lcd_update();

    if(battery_voltage <= 300) {
        line++;
        lcd_puts(0, line++, "WARNING! BATTERY LOW!!");
        lcd_update();
        sleep(HZ*2);
    }
    
    rc = ata_init();
    if(rc)
    {
        char str[32];
        lcd_clear_display();
        snprintf(str, 31, "ATA error: %d", rc);
        lcd_puts(0, line++, str);
        lcd_puts(0, line++, "Insert USB cable and press");
        lcd_puts(0, line++, "a button");
        lcd_update();
        while(!(button_get(true) & BUTTON_REL));
    }

    /* A hack to enter USB mode without using the USB thread */
    if(usb_detect())
    {
        lcd_clear_display();
        lcd_puts(0, 7, "    Bootloader USB mode");
        lcd_update();

        ata_spin();
        ata_enable(false);
        usb_enable(true);
        while(usb_detect())
        {
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);
            
            /* Backlight OFF */
            or_l(0x00020000, &GPIO1_OUT);
        }

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
        lcd_puts(0, 0, "No partition found");
        while(button_get(true) != SYS_USB_CONNECTED) {};
    }

    lcd_puts(0, line++, "Loading firmware");
    lcd_update();
    i = load_firmware();
    snprintf(buf, 256, "Result: %d", i);
    lcd_puts(0, line++, buf);
    lcd_update();

    if(i == 0)
        start_firmware();
    
    start_iriver_fw();
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
