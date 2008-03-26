/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Linus Nielsen Feltzing
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
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "scroll_engine.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "usb.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "powermgmt.h"
#include "file.h"
#include "pcf50606.h"
#include "common.h"
#include "rbunicode.h"
#include "isp1362.h"

#include <stdarg.h>

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

#define DRAM_START 0x31000000

char version[] = APPSVERSION;

/* Reset the cookie for the crt0 crash check */
inline void __reset_cookie(void)
{
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
}

void start_iriver_fw(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" lea.l 8,%a0");
    asm(" jmp (%a0)");
}

void start_firmware(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" move.l %0,%%d0" :: "i"(DRAM_START));
    asm(" movec.l %d0,%vbr");
    asm(" move.l %0,%%sp" :: "m"(*(int *)DRAM_START));
    asm(" move.l %0,%%a0" :: "m"(*(int *)(DRAM_START+4)));
    asm(" jmp (%a0)");
}

void shutdown(void)
{
    printf("Shutting down...");
    
    /* We need to gracefully spin down the disk to prevent clicks. */
    if (ide_powered())
    {
        /* Make sure ATA has been initialized. */
        ata_init();
        
        /* And put the disk into sleep immediately. */
        ata_sleepnow();
    }

    sleep(HZ*2);
    
    _backlight_off();
    _remote_backlight_off();
    
    __reset_cookie();
    power_off();
}

/* Print the battery voltage (and a warning message). */
void check_battery(void)
{
    int battery_voltage, batt_int, batt_frac;
    
    battery_voltage = battery_adc_voltage();
    batt_int = battery_voltage / 1000;
    batt_frac = (battery_voltage % 1000) / 10;

    printf("Batt: %d.%02dV", batt_int, batt_frac);

    if (battery_voltage <= 310) 
    {
        printf("WARNING! BATTERY LOW!!");
        sleep(HZ*2);
    }
}

/* From the pcf50606 driver */
extern unsigned char pcf50606_intregs[3];

/* From common.c */
extern int line;        
extern int remote_line;

void main(void)
{
    int i;
    int rc;
    bool rc_on_button = false;
    bool on_button = false;
    bool rec_button = false;
    bool hold_status = false;
    int data;
    bool rtc_alarm;
    int button;
    int mask;

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

    /* Check the interrupt registers if it was an RTC alarm */
    rtc_alarm = (pcf50606_intregs[0] & 0x80)?true:false;

    /* Turn off if we believe the start was accidental */
    if(!(rtc_alarm || on_button || rc_on_button ||
         (usb_detect() == USB_INSERTED) || charger_inserted())) {
        __reset_cookie();
        power_off();
    }

    /* get rid of a nasty humming sound during boot */
    mask = disable_irq_save();
    pcf50606_write(0x3b, 0x00);  /* GPOOD2 high Z */
    pcf50606_write(0x3b, 0x07);  /* GPOOD2 low */
    restore_irq(mask);

    /* Start with the main backlight OFF. */
    _backlight_init();
    _backlight_off();
    
    _remote_backlight_on();

    system_init();
    kernel_init();

    /* Set up waitstates for the peripherals */
    set_cpu_frequency(0); /* PLL off */
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);
    enable_irq();

    isp1362_init();

    adc_init();
    button_init();
    
    backlight_init();

    lcd_init();
    lcd_remote_init();
    font_init();

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version %s", version);

    sleep(HZ/50); /* Allow the button driver to check the buttons */
    rec_button = ((button_status() & BUTTON_REC) == BUTTON_REC)
        || ((button_status() & BUTTON_RC_REC) == BUTTON_RC_REC);

    check_battery();

    if(rtc_alarm)
        printf("RTC alarm detected");
    
    /* Don't start if the Hold button is active on the device you
       are starting with */
    if ((on_button && button_hold()) ||
         (rc_on_button && remote_button_hold()))
    {
        hold_status = true;
    }
    if (hold_status && !rtc_alarm && (usb_detect() != USB_INSERTED) && 
        !charger_inserted())
    {
        if (detect_original_firmware())
        {
            printf("Hold switch on");
            shutdown();
        }
    }

    /* Holding REC while starting runs the original firmware */
    if (detect_original_firmware() && rec_button)
    {
        printf("Starting original firmware...");
        start_iriver_fw();
    }

    if(charger_inserted())
    {
        const char charging_msg[] = "Charging...";
        const char complete_msg[] = "Charging complete";
        const char *msg;
        int w, h;
        bool blink_toggle = false;
        bool request_start = false;

        cpu_idle_mode(true);
        
        while(charger_inserted() && !request_start)
        {
            button = button_get_w_tmo(HZ);

            switch(button)
            {
            case BUTTON_ON:
                request_start = true;
                break;
                
            case BUTTON_NONE: /* Timeout */

                if(charging_state())
                {
                    /* To be replaced with a nice animation */
                    blink_toggle = !blink_toggle;
                    msg = charging_msg;
                }
                else
                {
                    blink_toggle = true;
                    msg = complete_msg;
                }
                
                font_getstringsize(msg, &w, &h, FONT_SYSFIXED);
                reset_screen();
                if(blink_toggle)
                    lcd_putsxy((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, msg);

                check_battery();
                break;
            }

            if(usb_detect() == USB_INSERTED)
                request_start = true;
        }
        if(!request_start)
        {
            __reset_cookie();
            power_off();
        }

        cpu_idle_mode(false);
    }
    
    usb_init();

    /* A hack to enter USB mode without using the USB thread */
    if(usb_detect() == USB_INSERTED)
    {
        const char msg[] = "Bootloader USB mode";
        int w, h;
        font_getstringsize(msg, &w, &h, FONT_SYSFIXED);
        reset_screen();
        lcd_putsxy((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, msg);
        lcd_update();

        lcd_remote_puts(0, 3, msg);
        lcd_remote_update();

        ide_power_enable(true);
        ata_enable(false);
        sleep(HZ/20);
        usb_enable(true);
        cpu_idle_mode(true);
        while (usb_detect() == USB_INSERTED)
        {
            /* Print the battery status. */
            line = 0;
            remote_line = 0;
            check_battery();
            
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);
        }

        cpu_idle_mode(false);
        usb_enable(false);

        reset_screen();
        lcd_update();
    }

    rc = ata_init();
    if(rc)
    {
        reset_screen();
        printf("ATA error: %d", rc);
        printf("Insert USB cable and press");
        printf("a button");
        while(!(button_get(true) & BUTTON_REL));
    }


    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        reset_screen();
        printf("No partition found");
        while(button_get(true) != SYS_USB_CONNECTED) {};
    }

    printf("Loading firmware");
    i = load_firmware((unsigned char *)DRAM_START, BOOTFILE, MAX_LOADSIZE);
    if(i < 0)
        printf("Error: %s", strerror(i));

    if (i == EOK)
        start_firmware();

    if (!detect_original_firmware())
    {
        printf("No firmware found on disk");
        sleep(HZ*2);
        shutdown();
    }
    else {
        sleep(HZ*2);
        start_iriver_fw();
    }
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void screen_dump(void)
{
}

int usb_screen(void)
{
   return 0;
}

unsigned short *bidi_l2v(const unsigned char *str, int orientation)
{
    static unsigned short utf16_buf[SCROLL_LINE_SIZE];
    unsigned short *target;
    (void)orientation;
    
    target = utf16_buf;
    
    while (*str)
        str = utf8decode(str, target++);
    *target = 0;
    return utf16_buf;
}
